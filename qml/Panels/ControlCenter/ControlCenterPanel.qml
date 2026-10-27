import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"
import ".."

// Noctalia's control center: hangs under the bar, compact icon sidebar on the left, the
// selected tab on the right. Opened with Panels.open("control-center") or a specific tab:
// Panels.open("control-center:audio").
PanelWindow {
  id: cc

  panelName: "control-center"
  placement: "bar"
  panelWidth: 700
  panelHeight: 560

  readonly property var tabs: [
    { key: "home", title: "Início", icon: "home", source: "HomeTab.qml" },
    { key: "media", title: "Mídia", icon: "disc", source: "MediaTab.qml" },
    { key: "audio", title: "Áudio", icon: "volume", source: "AudioTab.qml" },
    { key: "monitor", title: "Monitor", icon: "device-desktop", source: "MonitorTab.qml" },
    { key: "system", title: "Sistema", icon: "activity-heartbeat", source: "SystemTab.qml" },
    { key: "power", title: "Energia", icon: "battery-charging-2", source: "PowerTab.qml" },
    { key: "network", title: "Rede", icon: "wifi", source: "NetworkTab.qml" },
    { key: "bluetooth", title: "Bluetooth", icon: "bluetooth", source: "BluetoothTab.qml" },
    { key: "weather", title: "Clima", icon: "weather-cloud-sun", source: "WeatherTab.qml" },
    { key: "calendar", title: "Calendário", icon: "calendar-event", source: "CalendarTab.qml" },
  ]
  property string currentTab: "home"
  readonly property var currentMeta: tabs.find(t => t.key === currentTab) ?? tabs[0]

  // "control-center:audio" -> audio tab; plain "control-center" -> home
  onPanelArgChanged: {
    if (!wanted)
      return;
    currentTab = tabs.some(t => t.key === panelArg) ? panelArg : "home";
  }
  onOpened: currentTab = tabs.some(t => t.key === panelArg) ? panelArg : "home"

  Item {
    anchors.fill: parent
    anchors.margins: Style.panelPadding

    // ── sidebar ──────────────────────────────────────────────────────────────
    Rectangle {
      id: sidebar
      width: 32 + 2 * Style.spaceMd
      anchors.top: parent.top
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      radius: Style.radiusXl
      color: Theme.colors.surfaceContainer

      Column {
        anchors.top: parent.top
        anchors.topMargin: Style.spaceMd
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Style.spaceXs

        Repeater {
          model: cc.tabs
          delegate: Rectangle {
            id: tabButton
            required property var modelData
            readonly property bool isCurrent: cc.currentTab === modelData.key
            readonly property bool hovered: tabMouse.containsMouse
            width: 32 + Style.spaceXs * 2
            height: 32 + Style.spaceXs * 2
            radius: Style.radiusLg
            color: isCurrent ? Theme.colors.primary : hovered ? Theme.colors.hover : "transparent"

            Behavior on color {
              ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
            }

            NGlyph {
              anchors.centerIn: parent
              icon: tabButton.modelData.icon
              size: 21
              color: tabButton.isCurrent ? Theme.colors.onPrimary : tabButton.hovered ? Theme.colors.onHover : Theme.colors.onSurface
            }
            MouseArea {
              id: tabMouse
              anchors.fill: parent
              hoverEnabled: true
              cursorShape: Qt.PointingHandCursor
              onClicked: cc.currentTab = tabButton.modelData.key
            }
          }
        }
      }
    }

    // ── content ──────────────────────────────────────────────────────────────
    Item {
      id: content
      anchors.left: sidebar.right
      anchors.leftMargin: Style.panelPadding
      anchors.right: parent.right
      anchors.top: parent.top
      anchors.bottom: parent.bottom

      Item {
        id: header
        width: parent.width
        height: 32

        NText {
          anchors.left: parent.left
          anchors.verticalCenter: parent.verticalCenter
          text: cc.currentMeta.title
          font.pixelSize: Style.fontSizeHeader
          font.weight: Font.Bold
        }

        Row {
          anchors.right: parent.right
          anchors.verticalCenter: parent.verticalCenter
          spacing: Style.spaceXs

          NIconButton {
            icon: "settings"
            onClicked: Shell.openSettings("")
          }
          NIconButton {
            icon: "x"
            onClicked: Panels.close()
          }
        }
      }

      Loader {
        id: tabLoader
        anchors.top: header.bottom
        anchors.topMargin: Style.spaceMd
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        active: cc.shown
        source: cc.currentMeta.source

        onLoaded: {
          item.opacity = 0;
          tabFade.restart();
        }
        NumberAnimation {
          id: tabFade
          target: tabLoader.item
          property: "opacity"
          from: 0
          to: 1
          duration: Style.animNormal
          easing.type: Easing.OutCubic
        }
      }
    }
  }
}
