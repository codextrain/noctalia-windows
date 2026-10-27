import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left click = clipboard history panel.
BarWidget {
  id: root
  tooltipText: "Área de transferência"
  onClicked: Panels.toggle("clipboard")
  NGlyph { icon: "clipboard"; color: root.foreground }
}
