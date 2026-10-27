import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// Noctalia home: profile card + date/time card, media card + shortcut grid, quick sliders.
Item {
  id: home

  Component.onCompleted: SystemMonitor.acquire()
  Component.onDestruction: SystemMonitor.release()

  function formatUptime(sec) {
    const d = Math.floor(sec / 86400);
    const h = Math.floor((sec % 86400) / 3600);
    const m = Math.floor((sec % 3600) / 60);
    if (d > 0)
      return d + "d " + h + "h";
    if (h > 0)
      return h + "h " + m + "min";
    return m + "min";
  }

  property date now: new Date()
  Timer {
    interval: 1000
    running: true
    repeat: true
    onTriggered: home.now = new Date()
  }

  Column {
    anchors.fill: parent
    spacing: Style.spaceMd

    Row {
      width: parent.width
      height: 136
      spacing: Style.spaceMd

      // profile
      NCard {
        width: Math.round((parent.width - Style.spaceMd) * 0.62)
        height: parent.height

        NGlyph {
          id: avatar
          icon: "user-circle"
          size: 64
          color: Theme.colors.primary
          anchors.left: parent.left
          anchors.verticalCenter: parent.verticalCenter
        }
        Column {
          anchors.left: avatar.right
          anchors.leftMargin: Style.spaceMd
          anchors.right: parent.right
          anchors.verticalCenter: parent.verticalCenter
          spacing: 2

          NText {
            width: parent.width
            text: Shell.userName.length > 0 ? Shell.userName.charAt(0).toUpperCase() + Shell.userName.slice(1) : "Usuário"
            font.pixelSize: Math.round(Style.fontSizeTitle * 1.12)
            font.weight: Font.Bold
          }
          NText {
            width: parent.width
            text: Shell.userName.toLowerCase() + "@" + SystemMonitor.computerName
            font.pixelSize: Style.fontSizeCaption
            font.weight: Font.Normal
            color: Theme.colors.onSurfaceVariant
          }
          NText {
            width: parent.width
            text: "Tempo ativo – " + home.formatUptime(SystemMonitor.uptime)
            font.pixelSize: Style.fontSizeCaption
            font.weight: Font.Normal
            color: Theme.colors.onSurfaceVariant
          }
          NText {
            width: parent.width
            text: "Noctalia para Windows " + Shell.version + " · " + Shell.osName
            font.pixelSize: Style.fontSizeCaption
            font.weight: Font.Normal
            color: Theme.colors.onSurfaceVariant
          }
        }
      }

      // date & time
      NCard {
        width: parent.width - Style.spaceMd - Math.round((parent.width - Style.spaceMd) * 0.62)
        height: parent.height

        Column {
          anchors.centerIn: parent
          spacing: 2
          NText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatDateTime(home.now, Config.shell.time_format ?? "HH:mm")
            font.pixelSize: Math.round(Style.fontSizeTitle * 1.7)
            font.weight: Font.Bold
          }
          NText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: {
              const d = home.now.toLocaleDateString(Qt.locale(), "dddd, d 'de' MMMM");
              return d.charAt(0).toUpperCase() + d.slice(1);
            }
            font.pixelSize: Math.round(Style.fontSizeBody * 0.9)
            font.weight: Font.Normal
            color: Theme.colors.onSurfaceVariant
          }
          Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Style.spaceXs
            visible: typeof Weather !== "undefined" && Weather.available
            NGlyph {
              icon: typeof Weather !== "undefined" ? Weather.glyph : "weather-cloud-sun"
              size: Style.fontSizeBody
              color: Theme.colors.primary
            }
            NText {
              text: typeof Weather !== "undefined" ? Weather.temperatureText : "--"
              font.pixelSize: Style.fontSizeCaption
              color: Theme.colors.onSurfaceVariant
            }
          }
        }
      }
    }

    Row {
      width: parent.width
      height: 150
      spacing: Style.spaceMd

      // media
      NCard {
        id: mediaCard
        width: Math.round((parent.width - Style.spaceMd) * 0.62)
        height: parent.height
        readonly property bool hasMedia: typeof Media !== "undefined" && Media.available

        Rectangle {
          id: art
          width: 92
          height: 92
          radius: Style.radiusLg
          anchors.left: parent.left
          anchors.verticalCenter: parent.verticalCenter
          color: Theme.colors.surfaceContainerHigh
          clip: true

          Image {
            anchors.fill: parent
            visible: mediaCard.hasMedia && status === Image.Ready
            source: mediaCard.hasMedia ? Media.artUrl : ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
          }
          NGlyph {
            anchors.centerIn: parent
            visible: !mediaCard.hasMedia || Media.artUrl.length === 0
            icon: "disc"
            size: 40
            color: Theme.colors.onSurfaceVariant
          }
        }

        Column {
          anchors.left: art.right
          anchors.leftMargin: Style.spaceMd
          anchors.right: parent.right
          anchors.verticalCenter: parent.verticalCenter
          spacing: 2

          NText {
            width: parent.width
            text: mediaCard.hasMedia ? Media.title : "Nada tocando"
            font.weight: Font.Bold
          }
          NText {
            width: parent.width
            text: mediaCard.hasMedia ? Media.artist : "nenhum reprodutor ativo"
            font.pixelSize: Style.fontSizeCaption
            font.weight: Font.Normal
            color: Theme.colors.onSurfaceVariant
          }
          Item { width: 1; height: Style.spaceSm }
          Row {
            visible: mediaCard.hasMedia
            spacing: Style.spaceXs
            NIconButton { icon: "media-prev"; onClicked: Media.previous() }
            NIconButton { icon: Media.playing ? "media-pause" : "media-play"; active: true; onClicked: Media.playPause() }
            NIconButton { icon: "media-next"; onClicked: Media.next() }
          }
        }
      }

      // shortcuts (2 x 2)
      Grid {
        id: shortcuts
        width: parent.width - Style.spaceMd - mediaCard.width
        height: parent.height
        columns: 2
        spacing: Style.spaceSm
        readonly property real cellW: (width - spacing) / 2
        readonly property real cellH: (height - spacing) / 2

        ShortcutButton {
          width: shortcuts.cellW
          height: shortcuts.cellH
          icon: Network.wifi ? "wifi" : "network"
          label: "Rede"
          active: Network.connected
          onClicked: Panels.open("control-center:network")
        }
        ShortcutButton {
          width: shortcuts.cellW
          height: shortcuts.cellH
          icon: "bluetooth"
          label: "Bluetooth"
          active: typeof Bluetooth !== "undefined" && Bluetooth.enabled
          onClicked: Panels.open("control-center:bluetooth")
        }
        ShortcutButton {
          width: shortcuts.cellW
          height: shortcuts.cellH
          icon: "wallpaper-selector"
          label: "Papel de parede"
          onClicked: Panels.open("wallpaper")
        }
        ShortcutButton {
          width: shortcuts.cellW
          height: shortcuts.cellH
          icon: "shutdown"
          label: "Sessão"
          onClicked: Panels.open("launcher:session")
        }
      }
    }

    // quick sliders
    NCard {
      width: parent.width
      height: parent.height - y
      padding: Style.spaceMd

      Column {
        anchors.fill: parent
        spacing: Style.spaceXs

        VolumeRow {
          width: parent.width
          value: Audio.volume
          muted: Audio.muted
          available: Audio.available
          onMoved: v => Audio.setVolume(v)
          onMuteToggled: Audio.toggleMute()
        }
        VolumeRow {
          width: parent.width
          visible: Audio.micAvailable
          icon: "microphone"
          mutedIcon: "microphone-mute"
          value: Audio.micVolume
          muted: Audio.micMuted
          available: Audio.micAvailable
          onMoved: v => Audio.setMicVolume(v)
          onMuteToggled: Audio.toggleMicMute()
        }
        VolumeRow {
          width: parent.width
          visible: typeof Brightness !== "undefined" && Brightness.available
          icon: "brightness-high"
          mutedIcon: "brightness-low"
          value: typeof Brightness !== "undefined" ? Brightness.value : 0
          available: visible
          onMoved: v => Brightness.setValue(v)
        }
      }
    }
  }
}
