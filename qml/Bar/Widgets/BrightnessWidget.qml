import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left = control center > monitor, scroll = brightness ±5%. Hidden when no
// display supports brightness control.
BarWidget {
  id: root
  shown: Brightness.available
  tooltipText: "Brilho: " + Math.round(Brightness.value * 100) + "%"
  onClicked: Panels.toggle("control-center:monitor")
  onWheelUp: Brightness.change(0.05)
  onWheelDown: Brightness.change(-0.05)

  SculptedIcon { symbol: "brightness" }
}
