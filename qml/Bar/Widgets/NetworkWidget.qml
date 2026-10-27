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

  NGlyph {
    icon: Network.glyph
    color: Network.connected ? root.foreground : Theme.colors.error
  }
}
