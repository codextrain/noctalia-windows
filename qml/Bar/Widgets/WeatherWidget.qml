import QtQuick
import Noctalia
import ".."
import "../../Commons"
import "../../Widgets"

// Noctalia: weather glyph + temperature; left = control center > weather.
BarWidget {
  id: root
  shown: Weather.available
  tooltipText: Weather.description + " · " + Weather.locationName
  onClicked: Panels.toggle("control-center:weather")

  NGlyph {
    icon: Weather.glyph
    color: root.foreground
    anchors.verticalCenter: parent.verticalCenter
  }
  NText {
    anchors.verticalCenter: parent.verticalCenter
    text: Weather.temperatureText
    font.pixelSize: Style.fontSizeCaption
    color: root.foreground
  }
}
