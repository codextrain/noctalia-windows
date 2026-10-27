import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

// [-]  34  [+]
Row {
  id: box
  property real value: 0
  property real step: 1
  property real from: 0
  property real to: 100
  property int decimals: 0
  signal edited(real value)
  spacing: Style.spaceXs

  function set(v) {
    v = Math.max(from, Math.min(to, Math.round(v / step) * step));
    edited(decimals > 0 ? Number(v.toFixed(decimals)) : Math.round(v));
  }

  NIconButton { icon: "minus"; onClicked: box.set(box.value - box.step) }
  Rectangle {
    width: 56
    height: 32
    radius: Style.radiusMd
    color: Theme.colors.surface
    border.width: Style.borderWidth
    border.color: Theme.colors.outlineVariant
    NText {
      anchors.centerIn: parent
      text: box.decimals > 0 ? Number(box.value).toFixed(box.decimals) : Math.round(box.value)
    }
    MouseArea {
      anchors.fill: parent
      onWheel: event => box.set(box.value + (event.angleDelta.y > 0 ? box.step : -box.step))
    }
  }
  NIconButton { icon: "plus"; onClicked: box.set(box.value + box.step) }
}
