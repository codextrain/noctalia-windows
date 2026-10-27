import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left click = control center > network.
BarWidget {
  id: root
  tooltipText: Network.label
  onClicked: Panels.toggle("control-center:network")
  onRightClicked: Shell.openUri("ms-settings:network-status")

  SculptedIcon { symbol: "network"; disabled: !Network.connected }
}
