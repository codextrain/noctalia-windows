import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left click = toggle launcher. Right click = Windows Start menu.
BarWidget {
  id: root
  tooltipText: "Buscar aplicativos"
  onClicked: Panels.toggle("launcher")
  onRightClicked: Shell.startMenu()

  NGlyph { icon: "search"; color: root.foreground }
}
