import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// System tray: opens the Windows "hidden icons" flyout (Discord, Steam, OneDrive...).
// Windows doesn't let another program show those icons itself, so we open its own list.
BarWidget {
  id: root
  tooltipText: "Ícones da bandeja"
  onClicked: Shell.trayOverflow()
  NGlyph { icon: "chevron-up"; color: root.foreground }
}
