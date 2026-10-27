import QtQuick
import Noctalia
import ".."
import "../../Widgets"

// Noctalia: left = control center > bluetooth, right = toggle Bluetooth.
BarWidget {
  id: root
  readonly property int connectedCount: Bluetooth.devices.filter(d => d.connected).length
  shown: Bluetooth.available
  tooltipText: !Bluetooth.enabled ? "Bluetooth desligado"
             : connectedCount > 0 ? connectedCount + (connectedCount === 1 ? " dispositivo conectado" : " dispositivos conectados")
             : "Bluetooth ligado"
  onClicked: Panels.toggle("control-center:bluetooth")
  onRightClicked: Bluetooth.setEnabled(!Bluetooth.enabled)

  NGlyph {
    icon: Bluetooth.enabled ? (root.connectedCount > 0 ? "bluetooth-connected" : "bluetooth") : "bluetooth-off"
    color: Bluetooth.enabled ? root.foreground : Theme.colors.onSurfaceVariant
  }
}
