import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left click = session menu (lock, log out, suspend, reboot, shut down).
BarWidget {
  id: root
  tooltipText: "Sessão"
  onClicked: if (bar) bar.openSessionMenu(root)

  NGlyph { icon: "shutdown"; color: root.foreground }
}
