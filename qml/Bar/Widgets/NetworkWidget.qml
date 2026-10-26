import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left click = control center > network.
BarWidget {
  id: root
  tooltipText: Network.label
  onClicked: Shell.quickSettings()
  onRightClicked: Shell.openUri("ms-settings:network-status")

  NGlyph {
    icon: Network.glyph
    color: Network.connected ? root.foreground : Theme.colors.error
  }
}
