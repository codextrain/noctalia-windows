import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left click = control center > notifications. For now: Windows notification center.
BarWidget {
  id: root
  tooltipText: "Notificações"
  onClicked: Shell.notificationCenter()

  NGlyph { icon: "bell"; color: root.foreground }
}
