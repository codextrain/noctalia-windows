import QtQuick
import Noctalia
import ".."
import "../../Commons"
import "../../Widgets"

// Current weather: glyph, temperature and live condition in a single row.
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
    objectName: "weather-temperature"
    anchors.verticalCenter: parent.verticalCenter
    text: Weather.temperatureText
    font.pixelSize: Style.fontSizeCaption
    color: root.foreground
  }
  NText {
    objectName: "weather-condition"
    anchors.verticalCenter: parent.verticalCenter
    width:Math.min(implicitWidth,180 * Style.uiScale)
    text:Weather.description
    font.pixelSize:Style.fontSizeCaption
    color:root.foreground
    opacity:0.85
  }
}
