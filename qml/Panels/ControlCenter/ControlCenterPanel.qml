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
  panelWidth: 832
  panelHeight: 560

  readonly property var tabs: [
    { key: "home", title: "Início", icon: "home", source: "HomeTab.qml" },
    { key: "media", title: "Mídia", icon: "disc", source: "MediaTab.qml" },
    { key: "audio", title: "Áudio", icon: "volume", source: "AudioTab.qml" },
    { key: "monitor", title: "Monitor", icon: "device-desktop", source: "MonitorTab.qml" },
    { key: "system", title: "Sistema", icon: "activity-heartbeat", source: "SystemTab.qml" },
    { key: "hardware", title: "Sobre este PC", icon: "cpu-usage", source: "HardwareTab.qml" },
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
      width: 178
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
            objectName:"cc-category-"+modelData.key
            required property var modelData
            Accessible.name: modelData.title
            Accessible.role: Accessible.Button
            readonly property bool isCurrent: cc.currentTab === modelData.key
            readonly property bool hovered: tabMouse.containsMouse
            width: sidebar.width - 16
            height: 32 + Style.spaceXs * 2
            radius: Style.radiusLg
            color: "transparent"
            border.width:1
            border.color:isCurrent ? Theme.colors.primary : hovered ? Style.alpha(Theme.colors.primary,0.65) : Style.alpha(Theme.colors.outlineVariant,0.20)

            Behavior on border.color {
              ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
            }

            Rectangle {
              id:iconTile
              width:30;height:30;radius:9
              anchors.left:parent.left;anchors.leftMargin:9;anchors.verticalCenter:parent.verticalCenter
              gradient:Gradient {
                GradientStop {position:0;color:Style.alpha(Theme.colors.primary,0.12)}
                GradientStop {position:1;color:Style.alpha(Theme.colors.primary,0.03)}
              }
              border.width:1;border.color:Style.alpha(Theme.colors.primary,0.12)
              CategoryIcon {
                anchors.centerIn:parent;width:21;height:21;category:tabButton.modelData.key
                ink:Theme.colors.onSurfaceVariant
              }
            }
            NText {
              anchors.left:iconTile.right;anchors.leftMargin:10;anchors.right:parent.right;anchors.rightMargin:5
              anchors.verticalCenter:parent.verticalCenter
              text:tabButton.modelData.title
              font.pixelSize:13;font.weight:tabButton.isCurrent?Font.DemiBold:Font.Medium
              color:Theme.colors.onSurface
              elide:Text.ElideRight
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
