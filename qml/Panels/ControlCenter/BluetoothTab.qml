import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// Bluetooth on/off and paired devices. Pairing and connecting new devices open Windows'
// own Bluetooth page (Windows has no public API for that from a regular app).
Item {
  id: tab

  Component.onCompleted: Bluetooth.refresh()

  NCard {
    id: statusCard
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: 72

    NGlyph {
      id: btIcon
      anchors.left: parent.left
      anchors.verticalCenter: parent.verticalCenter
      icon: "bluetooth"
      size: 28
      color: Bluetooth.enabled ? Theme.colors.primary : Theme.colors.onSurfaceVariant
    }
    Column {
      anchors.left: btIcon.right
      anchors.leftMargin: Style.spaceMd
      anchors.right: btToggle.left
      anchors.rightMargin: Style.spaceMd
      anchors.verticalCenter: parent.verticalCenter
      NText {
        font.weight: Font.Bold
        text: "Bluetooth"
      }
      NText {
        font.pixelSize: Style.fontSizeCaption
        font.weight: Font.Normal
        color: Theme.colors.onSurfaceVariant
        text: !Bluetooth.available ? "Nenhum adaptador Bluetooth encontrado"
            : Bluetooth.enabled ? "Ligado" : "O Bluetooth está desligado"
      }
    }
    NToggle {
      id: btToggle
      visible: Bluetooth.available
      enabled: !Bluetooth.busy
      anchors.right: parent.right
      anchors.verticalCenter: parent.verticalCenter
      checked: Bluetooth.enabled
      onToggled: on => Bluetooth.setEnabled(on)
    }
  }

  NCard {
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: statusCard.bottom
    anchors.topMargin: Style.spaceMd
    anchors.bottom: parent.bottom
    visible: Bluetooth.available && Bluetooth.enabled

    Column {
      id: devCol
      width: parent.width
      spacing: Style.spaceSm

      NSectionTitle { text: "Emparelhados" }
      NText {
        visible: Bluetooth.devices.length === 0
        width: parent.width
        wrapMode: Text.Wrap
        text: "Nenhum dispositivo emparelhado."
        font.weight: Font.Normal
        color: Theme.colors.onSurfaceVariant
      }
      Repeater {
        model: Bluetooth.devices
        delegate: NListItem {
          required property var modelData
          width: devCol.width
          icon: modelData.glyph
          title: modelData.name
          subtitle: modelData.connected ? "Conectado" : "Desconectado"
          checked: modelData.connected
          onClicked: {
            Panels.close();
            Shell.openUri("ms-settings:bluetooth");
          }
        }
      }
      NListItem {
        width: devCol.width
        icon: "plus"
        title: "Adicionar dispositivo"
        onClicked: {
          Panels.close();
          Shell.openUri("ms-settings:connecteddevices");
        }
      }
    }
  }
}
