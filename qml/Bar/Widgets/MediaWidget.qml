import QtQuick
import Noctalia
import ".."
import "../../Commons"
import "../../Widgets"

// Noctalia media widget: player glyph + "Title · Artist" (max ~220px).
// Left = control center > media, right = play/pause, scroll = next / previous.
BarWidget {
  id: root
  shown: Media.available
  tooltipText: Media.title + (Media.artist.length > 0 ? " — " + Media.artist : "")
  onClicked: Panels.toggle("control-center:media")
  onRightClicked: Media.playPause()
  onWheelUp: Media.next()
  onWheelDown: Media.previous()

  NGlyph {
    icon: Media.playing ? "media-pause" : "media-play"
    color: root.foreground
    anchors.verticalCenter: parent.verticalCenter
  }
  NText {
    width: Math.min(implicitWidth, 220)
    anchors.verticalCenter: parent.verticalCenter
    text: Media.title + (Media.artist.length > 0 ? " · " + Media.artist : "")
    font.pixelSize: Style.fontSizeCaption
    color: root.foreground
  }
}
