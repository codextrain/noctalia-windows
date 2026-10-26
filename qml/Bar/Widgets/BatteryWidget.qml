import QtQuick
import Noctalia
import ".."
import "../../Commons"
import "../../Widgets"

// Noctalia: left click = control center > power. Hidden on desktops without a battery.
BarWidget {
  id: root

  readonly property bool low: Battery.percent >= 0 && Battery.percent < 15 && !Battery.plugged

  shown: Battery.present
  tooltipText: {
    let t = "Bateria: " + (Battery.percent >= 0 ? Battery.percent + "%" : "desconhecida");
    if (Battery.charging)
      t += " (carregando)";
    else if (Battery.plugged)
      t += " (na tomada)";
    else if (Battery.minutesLeft > 0)
      t += " · " + Math.floor(Battery.minutesLeft / 60) + "h" + String(Battery.minutesLeft % 60).padStart(2, "0") + " restantes";
    return t;
  }
  onClicked: Shell.openUri("ms-settings:batterysaver")

  NGlyph {
    icon: Battery.glyph
    color: root.low ? Theme.colors.error : root.foreground
    anchors.verticalCenter: parent.verticalCenter
  }
  NText {
    text: Battery.percent >= 0 ? Battery.percent + "%" : ""
    visible: text.length > 0
    color: root.low ? Theme.colors.error : root.foreground
    font.pixelSize: Style.fontSizeCaption
    anchors.verticalCenter: parent.verticalCenter
  }
}
