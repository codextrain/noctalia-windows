import QtQuick
import Noctalia
import "../Commons"

// An icon from Noctalia's Tabler icon font: NGlyph { icon: "search" }
Text {
  id: root

  property string icon: ""
  property real size: Style.baseGlyphSize

  text: Glyphs.get(icon)
  font.family: Glyphs.family
  font.pixelSize: Math.round(size)
  color: Theme.colors.onSurface
  horizontalAlignment: Text.AlignHCenter
  verticalAlignment: Text.AlignVCenter
  width: Math.round(size)
  height: Math.round(size)

  Behavior on color {
    ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
  }
}
