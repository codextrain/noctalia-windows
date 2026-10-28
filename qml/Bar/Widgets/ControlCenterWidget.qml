import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left click = control center. Right click opens the shell menu.
BarWidget {
  id: root
  tooltipText: "Vitória Shell · Central de controle"
  onClicked: Panels.toggle("control-center")
  onRightClicked: if (bar) bar.openShellMenu(root)

  WolfIcon {}
}
