import QtQuick
import Noctalia
import "../Commons"
import "../Commons/SculptedSymbols.js" as SculptedSymbols

// An icon from Noctalia's Tabler icon font: NGlyph { icon: "search" }
Text {
  id: root

  property string icon: ""
  property real size: Style.baseGlyphSize

  readonly property string sculptedSymbol:SculptedSymbols.forGlyph(icon)
  text: sculptedSymbol ? "" : Glyphs.get(icon)
  style: Text.Raised
  styleColor: "#202638"
  font.family: Glyphs.family
  font.pixelSize: Math.round(size)
  color: Theme.colors.onSurface
  horizontalAlignment: Text.AlignHCenter
  verticalAlignment: Text.AlignVCenter
  width: Math.round(size)
  height: Math.round(size)
  SculptedIcon {
    anchors.fill:parent
    visible:root.sculptedSymbol.length>0
    symbol:root.sculptedSymbol
    condition:root.icon
    disabled:root.icon.indexOf("mute")>=0||root.icon.indexOf("-off")>=0
  }
  Behavior on color {
    ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
  }
}
