import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left click = control center. Right click opens the shell menu.
BarWidget {
  id: root
  tooltipText: "Central de controle"
  onClicked: Shell.quickSettings()
  onRightClicked: if (bar) bar.openShellMenu(root)

  NGlyph { icon: "noctalia"; color: root.foreground }
}
