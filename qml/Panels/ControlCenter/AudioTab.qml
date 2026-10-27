import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// Output + input volume, device choice and per-app volume (Windows volume mixer).
Flickable {
  id: tab
  contentHeight: col.implicitHeight
  clip: true
  boundsBehavior: Flickable.StopAtBounds

  Component.onCompleted: {
    Audio.refreshDevices();
    Audio.watchSessions = true;
  }
  Component.onDestruction: Audio.watchSessions = false

  Column {
    id: col
    width: tab.width
    spacing: Style.spaceMd

    NCard {
      width: parent.width
      height: outCol.implicitHeight + 2 * padding
      Column {
        id: outCol
        width: parent.width
        spacing: Style.spaceSm
        NSectionTitle { text: "Saída" }
        VolumeRow {
          width: parent.width
          value: Audio.volume
          muted: Audio.muted
          available: Audio.available
          onMoved: v => Audio.setVolume(v)
          onMuteToggled: Audio.toggleMute()
        }
        Repeater {
          model: Audio.outputs
          delegate: NListItem {
            required property var modelData
            width: outCol.width
            icon: modelData.isDefault ? "volume" : "volume-2"
            title: modelData.name
            checked: modelData.isDefault
            onClicked: Audio.setDefaultDevice(modelData.id)
          }
        }
      }
    }

    NCard {
      width: parent.width
      height: inCol.implicitHeight + 2 * padding
      visible: Audio.inputs.length > 0
      Column {
        id: inCol
        width: parent.width
        spacing: Style.spaceSm
        NSectionTitle { text: "Entrada" }
        VolumeRow {
          width: parent.width
          icon: "microphone"
          mutedIcon: "microphone-mute"
          value: Audio.micVolume
          muted: Audio.micMuted
          available: Audio.micAvailable
          onMoved: v => Audio.setMicVolume(v)
          onMuteToggled: Audio.toggleMicMute()
        }
        Repeater {
          model: Audio.inputs
          delegate: NListItem {
            required property var modelData
            width: inCol.width
            icon: "microphone"
            title: modelData.name
            checked: modelData.isDefault
            onClicked: Audio.setDefaultDevice(modelData.id)
          }
        }
      }
    }

    NCard {
      width: parent.width
      height: appCol.implicitHeight + 2 * padding
      Column {
        id: appCol
        width: parent.width
        spacing: Style.spaceSm
        NSectionTitle { text: "Volume de aplicativos" }
        NText {
          visible: Audio.sessions.length === 0
          text: "Nenhum aplicativo tocando som no momento."
          font.weight: Font.Normal
          color: Theme.colors.onSurfaceVariant
        }
        Repeater {
          model: Audio.sessions
          delegate: Item {
            id: appRow
            required property var modelData
            width: appCol.width
            height: 44

            Image {
              id: appIcon
              width: 24
              height: 24
              anchors.left: parent.left
              anchors.top: parent.top
              anchors.topMargin: 2
              source: appRow.modelData.exePath.length > 0
                      ? "image://icons/file/" + encodeURIComponent(appRow.modelData.exePath) : ""
              sourceSize: Qt.size(48, 48)
              asynchronous: true
            }
            NText {
              anchors.left: appIcon.right
              anchors.leftMargin: Style.spaceSm
              anchors.right: parent.right
              anchors.verticalCenter: appIcon.verticalCenter
              text: appRow.modelData.name + (appRow.modelData.active ? "  ♪" : "")
              font.pixelSize: Style.fontSizeCaption
            }
            VolumeRow {
              anchors.left: parent.left
              anchors.right: parent.right
              anchors.bottom: parent.bottom
              height: 22
              value: appRow.modelData.volume
              muted: appRow.modelData.muted
              onMoved: v => Audio.setSessionVolume(appRow.modelData.id, v)
              onMuteToggled: Audio.setSessionMuted(appRow.modelData.id, !appRow.modelData.muted)
            }
          }
        }
      }
    }
  }
}
