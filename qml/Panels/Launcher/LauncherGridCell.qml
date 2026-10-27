import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// One app in grid mode: big icon with the name below (two lines max).
Item {
  id: cell

  property var item: ({})
  property bool selected: false
  readonly property bool hovered: mouse.containsMouse
  readonly property color fg: selected ? Theme.colors.onPrimary : hovered ? Theme.colors.onHover : Theme.colors.onSurface

  signal activated
  signal menuRequested(point windowPos)

  Rectangle {
    anchors.fill: parent
    anchors.margins: 3
    radius: Style.radiusLg
    color: cell.selected ? Theme.colors.primary : cell.hovered ? Theme.colors.hover : "transparent"

    Behavior on color {
      ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
    }

    Image {
      id: img
      width: 48
      height: 48
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.top: parent.top
      anchors.topMargin: Style.spaceMd
      source: cell.item.icon ?? ""
      sourceSize: Qt.size(96, 96)
      asynchronous: true
      mipmap: true
      fillMode: Image.PreserveAspectFit
    }

    NGlyph {
      anchors.centerIn: img
      visible: img.status !== Image.Ready
      icon: cell.item.glyph ?? "apps"
      size: 34
      color: cell.fg
    }

    Text {
      anchors.top: img.bottom
      anchors.topMargin: Style.spaceSm
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.leftMargin: Style.spaceXs
      anchors.rightMargin: Style.spaceXs
      horizontalAlignment: Text.AlignHCenter
      text: cell.item.title ?? ""
      wrapMode: Text.Wrap
      maximumLineCount: 2
      elide: Text.ElideRight
      font.family: Style.fontFamily
      font.pixelSize: Style.fontSizeCaption
      font.weight: Font.DemiBold
      color: cell.fg
    }
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    hoverEnabled: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    cursorShape: Qt.PointingHandCursor
    onClicked: event => {
      if (event.button === Qt.RightButton)
        cell.menuRequested(mapToItem(null, event.x, event.y));
      else
        cell.activated();
    }
  }
}
