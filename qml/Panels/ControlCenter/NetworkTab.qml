import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// Wi-Fi on/off, current connection and nearby networks (click to connect; asks for the
// password when the network is new).
Item {
  id: tab

  property string expanded: "" // SSID whose password field is open

  Component.onCompleted: Wifi.watching = true
  Component.onDestruction: Wifi.watching = false

  function signalGlyph(s) {
    return s > 66 ? "wifi" : s > 33 ? "wifi-2" : "wifi-1";
  }

  NCard {
    id: statusCard
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: 72

    NGlyph {
      id: netIcon
      anchors.left: parent.left
      anchors.verticalCenter: parent.verticalCenter
      icon: Wifi.connected ? tab.signalGlyph(Wifi.signal) : Network.connected ? "network" : "wifi-off"
      size: 28
      color: Network.connected ? Theme.colors.primary : Theme.colors.error
    }
    Column {
      anchors.left: netIcon.right
      anchors.leftMargin: Style.spaceMd
      anchors.right: wifiToggle.left
      anchors.rightMargin: Style.spaceMd
      anchors.verticalCenter: parent.verticalCenter
      NText {
        width: parent.width
        font.weight: Font.Bold
        text: Wifi.connected ? Wifi.ssid : Network.connected ? "Cabo de rede" : "Sem conexão"
      }
      NText {
        width: parent.width
        font.pixelSize: Style.fontSizeCaption
        font.weight: Font.Normal
        color: Theme.colors.onSurfaceVariant
        text: Wifi.connected ? "Conectado · sinal " + Wifi.signal + "%"
            : !Wifi.available ? "Nenhum adaptador Wi-Fi"
            : Wifi.enabled ? "Wi-Fi ligado" : "Wi-Fi desligado"
      }
    }
    NToggle {
      id: wifiToggle
      visible: Wifi.available
      anchors.right: parent.right
      anchors.verticalCenter: parent.verticalCenter
      checked: Wifi.enabled
      onToggled: on => Wifi.setEnabled(on)
    }
  }

  NCard {
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: statusCard.bottom
    anchors.topMargin: Style.spaceMd
    anchors.bottom: parent.bottom
    visible: Wifi.available && Wifi.enabled

    Item {
      id: listHeader
      width: parent.width
      height: 28
      NSectionTitle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: "Redes disponíveis"
      }
      Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: Style.spaceXs
        NIconButton { icon: "refresh"; onClicked: Wifi.scan() }
        NIconButton {
          icon: "external-link"
          onClicked: {
            Panels.close();
            Shell.openUri("ms-settings:network-wifi");
          }
        }
      }
    }

    NText {
      id: errorText
      anchors.top: listHeader.bottom
      width: parent.width
      visible: Wifi.lastError.length > 0
      text: Wifi.lastError
      color: Theme.colors.error
      font.pixelSize: Style.fontSizeCaption
    }

    ListView {
      anchors.top: errorText.visible ? errorText.bottom : listHeader.bottom
      anchors.topMargin: Style.spaceSm
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      clip: true
      spacing: 2
      boundsBehavior: Flickable.StopAtBounds
      model: Wifi.networks

      delegate: Column {
        id: netRow
        required property var modelData
        readonly property bool open: tab.expanded === modelData.ssid
        width: ListView.view.width
        spacing: Style.spaceXs

        NListItem {
          width: parent.width
          icon: tab.signalGlyph(netRow.modelData.signal)
          title: netRow.modelData.ssid
          subtitle: netRow.modelData.connected ? "Conectado"
                  : Wifi.connecting === netRow.modelData.ssid ? "Conectando..."
                  : netRow.modelData.saved ? "Salva" : netRow.modelData.secured ? "Protegida" : "Aberta"
          trailingIcon: netRow.modelData.secured ? "lock" : ""
          checked: netRow.modelData.connected
          onClicked: {
            if (netRow.modelData.connected) {
              Wifi.disconnect();
            } else if (netRow.modelData.saved || !netRow.modelData.secured) {
              tab.expanded = "";
              Wifi.connectTo(netRow.modelData.ssid, "");
            } else {
              tab.expanded = netRow.open ? "" : netRow.modelData.ssid;
            }
          }
        }

        Row {
          visible: netRow.open
          width: parent.width
          spacing: Style.spaceSm
          NTextInput {
            id: pass
            width: parent.width - connectButton.width - Style.spaceSm
            height: 36
            icon: "lock"
            placeholder: "Senha da rede"
            input.echoMode: TextInput.Password
            onVisibleChanged: if (visible) input.forceActiveFocus()
            onKeyPressed: event => {
              if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                Wifi.connectTo(netRow.modelData.ssid, pass.text);
                tab.expanded = "";
                event.accepted = true;
              }
            }
          }
          Rectangle {
            id: connectButton
            width: 100
            height: 36
            radius: Style.radiusMd
            color: Theme.colors.primary
            NText {
              anchors.centerIn: parent
              text: "Conectar"
              color: Theme.colors.onPrimary
              font.weight: Font.Bold
            }
            MouseArea {
              anchors.fill: parent
              cursorShape: Qt.PointingHandCursor
              onClicked: {
                Wifi.connectTo(netRow.modelData.ssid, pass.text);
                tab.expanded = "";
              }
            }
          }
        }
      }
    }
  }
}
