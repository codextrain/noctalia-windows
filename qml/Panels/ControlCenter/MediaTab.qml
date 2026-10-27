import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// Now playing: cover, title/artist, seek bar and controls (Windows media session).
Item {
  id: tab

  function fmt(sec) {
    sec = Math.max(0, Math.floor(sec));
    const m = Math.floor(sec / 60);
    const s = sec % 60;
    return m + ":" + (s < 10 ? "0" : "") + s;
  }

  NCard {
    anchors.fill: parent

    Column {
      anchors.centerIn: parent
      width: Math.min(parent.width, 420)
      spacing: Style.spaceMd
      visible: Media.available

      Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        width: 220
        height: 220
        radius: Style.radiusXl
        color: Theme.colors.surfaceContainerHigh
        clip: true
        Image {
          anchors.fill: parent
          source: Media.artUrl
          visible: status === Image.Ready
          fillMode: Image.PreserveAspectCrop
          asynchronous: true
        }
        NGlyph {
          anchors.centerIn: parent
          visible: Media.artUrl.length === 0
          icon: "disc"
          size: 80
          color: Theme.colors.onSurfaceVariant
        }
      }

      Column {
        width: parent.width
        spacing: 2
        NText {
          width: parent.width
          horizontalAlignment: Text.AlignHCenter
          text: Media.title
          font.pixelSize: Style.fontSizeTitle
          font.weight: Font.Bold
        }
        NText {
          width: parent.width
          horizontalAlignment: Text.AlignHCenter
          text: Media.artist + (Media.album.length > 0 ? " — " + Media.album : "")
          font.weight: Font.Normal
          color: Theme.colors.onSurfaceVariant
        }
      }

      Item {
        width: parent.width
        height: 34
        visible: Media.duration > 0
        NSlider {
          id: seek
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.top: parent.top
          value: Media.duration > 0 ? Media.position / Media.duration : 0
          enabled: Media.canSeek
          onMoved: v => Media.seek(v * Media.duration)
        }
        NText {
          anchors.left: parent.left
          anchors.bottom: parent.bottom
          text: tab.fmt(Media.position)
          font.pixelSize: Style.fontSizeMini
          color: Theme.colors.onSurfaceVariant
        }
        NText {
          anchors.right: parent.right
          anchors.bottom: parent.bottom
          text: tab.fmt(Media.duration)
          font.pixelSize: Style.fontSizeMini
          color: Theme.colors.onSurfaceVariant
        }
      }

      Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Style.spaceMd
        NIconButton { width: 44; height: 44; glyphSize: 22; icon: "media-prev"; onClicked: Media.previous() }
        NIconButton {
          width: 52
          height: 52
          radius: 26
          glyphSize: 26
          icon: Media.playing ? "media-pause" : "media-play"
          active: true
          onClicked: Media.playPause()
        }
        NIconButton { width: 44; height: 44; glyphSize: 22; icon: "media-next"; onClicked: Media.next() }
      }
    }

    Column {
      anchors.centerIn: parent
      spacing: Style.spaceSm
      visible: !Media.available
      NGlyph {
        anchors.horizontalCenter: parent.horizontalCenter
        icon: "disc"
        size: 64
        color: Theme.colors.onSurfaceVariant
      }
      NText {
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Nada tocando"
        font.pixelSize: Style.fontSizeTitle
        font.weight: Font.Bold
      }
      NText {
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Toque algo no Spotify, no navegador ou em outro player."
        font.weight: Font.Normal
        color: Theme.colors.onSurfaceVariant
      }
    }
  }
}
