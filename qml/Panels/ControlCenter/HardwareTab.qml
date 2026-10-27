import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

Item {
  id: tab
  Component.onCompleted: Hardware.ensureLoaded()
  Row {
    id: actions
    width: parent.width
    spacing: Style.spaceSm
    NText {
      width: parent.width - 40
      text: Hardware.busy ? "Consultando componentes…" : "Componentes e sistema operacional"
      color: Theme.colors.onSurfaceVariant
      anchors.verticalCenter: parent.verticalCenter
    }
    NIconButton {
      icon: "refresh"
      opacity: Hardware.busy ? 0.4 : 1
      enabled: !Hardware.busy
      onClicked: Hardware.refresh()
    }
  }
  Flickable {
    anchors.top: actions.bottom
    anchors.topMargin: Style.spaceMd
    anchors.bottom: parent.bottom
    width: parent.width
    contentHeight: entries.height
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    Column {
      id: entries
      width: parent.width
      spacing: Style.spaceSm
      NText {
        width: parent.width
        visible: Hardware.error.length > 0
        text: Hardware.error
        wrapMode: Text.Wrap
        color: Theme.colors.error
      }
      Repeater {
        model: Hardware.components
        delegate: NCard {
          required property var modelData
          width: entries.width
          height: detail.height + 2 * Style.panelPadding
          Column {
            id: detail
            width: parent.width
            spacing: Style.spaceXs
            NText { text: modelData.label; color: Theme.colors.primary; font.weight: Font.Bold }
            NText { width: parent.width; text: modelData.value; wrapMode: Text.Wrap; font.weight: Font.Normal }
          }
        }
      }
    }
  }
}
