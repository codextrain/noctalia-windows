import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

GridView {
    id: og
    property bool active: true
    property bool loading: active && WallStore.busy
    readonly property bool scrollbarDragging:sidebar.dragging
    signal requestMore()
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    property var results: WallStore.results
    property int revision: 0
    property int appliedRevision: -1
    property var previousKeys: []
    property bool syncing: false
    ListModel { id: stableResults; dynamicRoles: true }
    model: stableResults
    function syncResults() {
      if (!stableResults || syncing) return;
      syncing=true;
      const incoming=active ? results : [];
      let appendOnly=appliedRevision===revision && incoming.length>=previousKeys.length;
      for(let i=0;appendOnly && i<previousKeys.length;i++)
        appendOnly=String(incoming[i].key)===previousKeys[i];
      if(!appendOnly) stableResults.clear();
      const start=appendOnly ? previousKeys.length : 0;
      for(let i=0;i<start;i++) {
        const payload=JSON.parse(JSON.stringify(incoming[i]));
        if(JSON.stringify(stableResults.get(i).payload)!==JSON.stringify(payload))
          stableResults.set(i,{payload:payload});
      }
      for(let i=start;i<incoming.length;i++)
        stableResults.append({payload:JSON.parse(JSON.stringify(incoming[i]))});
      previousKeys=incoming.map(item=>String(item.key));
      appliedRevision=revision;
      forceLayout();
      if(!appendOnly)positionViewAtBeginning();
      syncing=false;
    }
    onResultsChanged: syncResults()
    onActiveChanged: syncResults()
    Component.onCompleted: syncResults()
    cellWidth: Math.floor((width - 18) / 4)
    cellHeight: Math.round(cellWidth * 0.62) + 8
    cacheBuffer: 600
    ViewportScrollBar {
      id:sidebar
      parent:og;target:og;anchors.right:parent.right;anchors.top:parent.top;anchors.bottom:parent.bottom
      onScrollFinished:if(og.atYEnd && og.active && og.visible && !og.syncing)og.requestMore()
    }
    Rectangle {
      objectName:"wallpaper-loading-indicator"
      parent:og;z:110
      width:70;height:28;radius:14
      x:(og.width-width)/2;y:og.count===0?(og.height-height)/2:og.height-height-6
      color:Style.glassColor(Theme.colors.surfaceContainer,1)
      visible:og.loading
      PulseDots {anchors.centerIn:parent;running:og.loading}
    }

    // (explicit Component: Qt 6.4 rejects an inline delegate inside an inline component)
    delegate: Component { Item {
      id: cell
      required property var payload
      readonly property var modelData: payload
      readonly property string localPath: WallStore.downloaded[modelData.key] ?? ""
      readonly property bool isCurrent: localPath.length > 0 && localPath.toLowerCase() === Wallpaper.current.toLowerCase()
      width: og.cellWidth
      height: og.cellHeight

      Rectangle {
        id: card
        anchors.fill: parent
        anchors.margins: 4
        radius: Style.radiusLg
        color: Theme.colors.surfaceContainer
        border.width: cell.isCurrent ? 3 : cellMouse.containsMouse ? 2 : 0
        border.color: cell.isCurrent ? Theme.colors.primary : Theme.colors.hover
        clip: true

        Image {
          id: img
          anchors.fill: parent
          anchors.margins: card.border.width
          source: cell.modelData.thumb
          sourceSize: Qt.size(400, 250)
          fillMode: Image.PreserveAspectCrop
          asynchronous: true
          cache: true
        }
        NGlyph {
          anchors.centerIn: parent
          visible: img.status === Image.Error || img.status === Image.Null
          icon: img.status === Image.Error ? "photo" : "loader"
          color: Theme.colors.onSurfaceVariant
        }
        PulseDots {anchors.centerIn:parent;visible:img.status===Image.Loading;running:visible}

        // info strip
        Rectangle {
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.bottom: parent.bottom
          anchors.margins: card.border.width
          height: 22
          visible: cellMouse.containsMouse
          color: Style.alpha(Theme.colors.surface, 0.82)
          NText {
            anchors.left: parent.left
            anchors.leftMargin: Style.spaceSm
            anchors.right: parent.right
            anchors.rightMargin: 34
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideMiddle
            font.pixelSize: Style.fontSizeMini
            text: [cell.modelData.resolution, cell.modelData.fileSize, cell.modelData.source === "github" ? cell.modelData.name : ""]
                  .filter(s => s && s.length).join(" · ")
          }
        }

        // downloaded badge / download button
        Rectangle {
          anchors.right: parent.right
          anchors.top: parent.top
          anchors.margins: 6
          width: 26
          height: 26
          radius: 13
          visible: cell.localPath.length > 0 || cellMouse.containsMouse || dlMouse.containsMouse
          color: cell.localPath.length > 0 ? Theme.colors.primary
               : dlMouse.containsMouse ? Theme.colors.hover : Style.alpha(Theme.colors.surface, 0.85)
          NGlyph {
            anchors.centerIn: parent
            size: 15
            icon: cell.localPath.length > 0 ? "check" : "download"
            color: cell.localPath.length > 0 ? Theme.colors.onPrimary : Theme.colors.onSurface
          }
          MouseArea {
            id: dlMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: WallStore.download(cell.modelData, false)
          }
        }
      }
      MouseArea {
        id: cellMouse
        anchors.fill: parent
        z: -1
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: WallStore.download(cell.modelData, true)
      }
    } }

    NText {
      anchors.centerIn:parent; width:parent.width*0.8; horizontalAlignment:Text.AlignHCenter; wrapMode:Text.Wrap; color:Theme.colors.onSurfaceVariant
      visible: og.count === 0
      text: og.loading ? "" : WallStore.error.length > 0 ? "" : "Nada por aqui ainda."
    }
  }
