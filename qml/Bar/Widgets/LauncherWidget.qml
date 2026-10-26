import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left click = toggle launcher. Until our launcher exists, open Windows Search.
BarWidget {
  id: root
  tooltipText: "Buscar aplicativos"
  onClicked: Shell.search()
  onRightClicked: Shell.startMenu()

  NGlyph { icon: "search"; color: root.foreground }
}
