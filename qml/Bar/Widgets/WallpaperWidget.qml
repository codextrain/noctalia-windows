import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left click = wallpaper panel. Right click = random wallpaper.
BarWidget {
  id: root
  tooltipText: "Papel de parede"
  onClicked: Panels.toggle("wallpaper")
  onRightClicked: Wallpaper.random()

  NGlyph { icon: "wallpaper-selector"; color: root.foreground }
}
