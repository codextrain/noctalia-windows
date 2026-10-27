import QtQuick
import QtQuick.Window
import Noctalia
import "../Commons"
import "../Widgets"

// Noctalia desktop widgets: clock, media, sysmon or weather cards that live on the desktop,
// behind your windows. Drag to move (position is saved); scroll to resize.
Window {
  id: w

  required property var modelData // {type, x, y, scale}
  required property int index

  readonly property string type: modelData.type ?? "clock"
  property real widgetScale: modelData.scale ?? 1.0

  flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnBottomHint | Qt.WindowDoesNotAcceptFocus | Qt.NoDropShadowWindowHint
  color: "transparent"
  visible: true
  title: "Noctalia " + type
  x: modelData.x ?? 80
  y: modelData.y ?? 120
  width: Math.round(content.implicitWidth * widgetScale) + 24
  height: Math.round(content.implicitHeight * widgetScale) + 24

  function save() {
    const list = (Config.desktopWidgets.widgets ?? []).slice();
    if (index < list.length) {
      list[index] = Object.assign({}, list[index], { x: Math.round(w.x), y: Math.round(w.y), scale: Math.round(w.widgetScale * 100) / 100 });
      Config.setValue("desktop_widgets.widgets", list);
    }
  }

  Component.onCompleted: SystemMonitor.acquire()
  Component.onDestruction: SystemMonitor.release()

  property date now: new Date()
  Timer {
    interval: 1000
    running: w.type === "clock"
    repeat: true
    onTriggered: w.now = new Date()
  }

  Rectangle {
    id: card
    anchors.centerIn: parent
    width: content.implicitWidth
    height: content.implicitHeight
    scale: w.widgetScale
    radius: Style.radiusXl * 1.5
    color: Style.alpha(Theme.colors.surface, 0.8)
    border.width: Style.borderWidth
    border.color: Theme.colors.outlineVariant

    Item {
      id: content
      anchors.fill: parent
      implicitWidth: loader.item ? loader.item.implicitWidth + 2 * Style.spaceLg : 200
      implicitHeight: loader.item ? loader.item.implicitHeight + 2 * Style.spaceLg : 100
      Loader {
        id: loader
        anchors.centerIn: parent
        sourceComponent: w.type === "media" ? mediaComp : w.type === "sysmon" ? sysmonComp
                       : w.type === "weather" ? weatherComp : clockComp
      }
    }
  }

  MouseArea {
    anchors.fill: parent
    property point start
    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
    onPressed: event => start = Qt.point(event.x, event.y)
    onPositionChanged: event => {
      if (!pressed)
        return;
      w.x += event.x - start.x;
      w.y += event.y - start.y;
    }
    onReleased: w.save()
    onWheel: event => {
      w.widgetScale = Math.max(0.5, Math.min(3, w.widgetScale + (event.angleDelta.y > 0 ? 0.1 : -0.1)));
      saveTimer.restart();
    }
  }
  Timer { id: saveTimer; interval: 600; onTriggered: w.save() }

  Component {
    id: clockComp
    Column {
      spacing: 0
      NText {
        anchors.horizontalCenter: parent.horizontalCenter
        text: Qt.formatDateTime(w.now, Config.shell.time_format ?? "HH:mm")
        font.pixelSize: 64
        font.weight: Font.Bold
        color: Theme.colors.primary
      }
      NText {
        anchors.horizontalCenter: parent.horizontalCenter
        text: {
          const d = w.now.toLocaleDateString(Qt.locale(), "dddd, d 'de' MMMM");
          return d.charAt(0).toUpperCase() + d.slice(1);
        }
        font.pixelSize: Style.fontSizeTitle
        color: Theme.colors.onSurfaceVariant
      }
    }
  }

  Component {
    id: mediaComp
    Row {
      spacing: Style.spaceMd
      Rectangle {
        width: 72
        height: 72
        radius: Style.radiusLg
        color: Theme.colors.surfaceContainerHigh
        clip: true
        Image { anchors.fill: parent; source: Media.artUrl; fillMode: Image.PreserveAspectCrop; asynchronous: true }
        NGlyph { anchors.centerIn: parent; visible: Media.artUrl.length === 0; icon: "disc"; size: 32; color: Theme.colors.onSurfaceVariant }
      }
      Column {
        anchors.verticalCenter: parent.verticalCenter
        width: 200
        NText { width: parent.width; text: Media.available ? Media.title : "Nada tocando"; font.weight: Font.Bold }
        NText { width: parent.width; text: Media.artist; font.pixelSize: Style.fontSizeCaption; color: Theme.colors.onSurfaceVariant }
        Row {
          visible: Media.available
          NIconButton { icon: "media-prev"; onClicked: Media.previous() }
          NIconButton { icon: Media.playing ? "media-pause" : "media-play"; active: true; onClicked: Media.playPause() }
          NIconButton { icon: "media-next"; onClicked: Media.next() }
        }
      }
    }
  }

  Component {
    id: sysmonComp
    Column {
      spacing: Style.spaceSm
      width: 220
      Repeater {
        model: [
          { icon: "cpu-usage", label: "CPU", value: SystemMonitor.cpu / 100 },
          { icon: "memory", label: "RAM", value: SystemMonitor.memTotal > 0 ? SystemMonitor.memUsed / SystemMonitor.memTotal : 0 },
          { icon: "storage", label: "Disco", value: SystemMonitor.diskTotal > 0 ? SystemMonitor.diskUsed / SystemMonitor.diskTotal : 0 },
        ]
        delegate: Column {
          required property var modelData
          width: 220
          spacing: 3
          Row {
            spacing: Style.spaceXs
            NGlyph { icon: modelData.icon; size: Style.fontSizeBody; color: Theme.colors.primary }
            NText { text: modelData.label + "  " + Math.round(modelData.value * 100) + "%"; font.pixelSize: Style.fontSizeCaption }
          }
          NProgress { width: 220; value: modelData.value }
        }
      }
    }
  }

  Component {
    id: weatherComp
    Row {
      spacing: Style.spaceMd
      NGlyph { icon: Weather.glyph; size: 56; color: Theme.colors.primary; anchors.verticalCenter: parent.verticalCenter }
      Column {
        anchors.verticalCenter: parent.verticalCenter
        NText { text: Weather.temperatureText; font.pixelSize: 36; font.weight: Font.Bold }
        NText { text: Weather.description; color: Theme.colors.onSurfaceVariant }
        NText { text: Weather.locationName; font.pixelSize: Style.fontSizeCaption; color: Theme.colors.onSurfaceVariant }
      }
    }
  }
}
