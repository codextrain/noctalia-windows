import QtQuick
import Noctalia
import "../Commons"

// An icon from Noctalia's Tabler icon font: NGlyph { icon: "search" }
Text {
  id: root

  property string icon: ""
  property real size: Style.baseGlyphSize

  text: icon === "vitoria-cat" ? "" : Glyphs.get(icon)
  font.family: Glyphs.family
  font.pixelSize: Math.round(size)
  color: Theme.colors.onSurface
  horizontalAlignment: Text.AlignHCenter
  verticalAlignment: Text.AlignVCenter
  width: Math.round(size)
  height: Math.round(size)
  Canvas {
    id: catCanvas
    anchors.fill: parent
    visible: root.icon === "vitoria-cat"
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    Connections { target: root; function onColorChanged() { catCanvas.requestPaint(); } }
    onPaint: {
      const c = getContext("2d");
      c.reset(); c.scale(width / 24, height / 24);
      c.strokeStyle = root.color; c.lineWidth = 1.6; c.lineJoin = "round"; c.lineCap = "round";
      c.beginPath(); c.moveTo(4,11); c.lineTo(3,3); c.lineTo(9,7);
      c.quadraticCurveTo(12,6,15,7); c.lineTo(21,3); c.lineTo(20,11);
      c.bezierCurveTo(24,24,0,24,4,11); c.stroke();
      c.beginPath(); c.moveTo(8,12); c.lineTo(8,13); c.moveTo(16,12); c.lineTo(16,13);
      c.moveTo(11,15); c.lineTo(12,16); c.lineTo(13,15);
      c.moveTo(12,16); c.lineTo(12,18);
      c.moveTo(2,14); c.lineTo(6,15); c.moveTo(2,18); c.lineTo(6,17);
      c.moveTo(22,14); c.lineTo(18,15); c.moveTo(22,18); c.lineTo(18,17); c.stroke();
    }
  }

  Behavior on color {
    ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
  }
}
