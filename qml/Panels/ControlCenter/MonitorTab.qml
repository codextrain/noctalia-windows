import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// One card per display: name, resolution and a brightness slider (laptop panel via WMI,
// external monitors via DDC/CI).
Flickable {
  id: tab
  contentHeight: col.implicitHeight
  clip: true
  boundsBehavior: Flickable.StopAtBounds

  Component.onCompleted: Brightness.refresh()

  Column {
    id: col
    width: tab.width
    spacing: Style.spaceMd

    NText {
      visible: Brightness.loading && Brightness.displays.length === 0
      text: "Procurando monitores..."
      color: Theme.colors.onSurfaceVariant
    }

    Repeater {
      model: Brightness.displays
      delegate: NCard {
        id: card
        required property var modelData
        width: col.width
        height: 104

        NGlyph {
          id: dispIcon
          icon: card.modelData.internal ? "device-laptop" : "device-desktop"
          size: 24
          color: Theme.colors.primary
          anchors.left: parent.left
          anchors.top: parent.top
        }
        Column {
          anchors.left: dispIcon.right
          anchors.leftMargin: Style.spaceMd
          anchors.right: parent.right
          anchors.top: parent.top
          NText {
            width: parent.width
            text: card.modelData.name
            font.weight: Font.Bold
          }
          NText {
            width: parent.width
            text: card.modelData.width + " × " + card.modelData.height + " · " + card.modelData.refresh + " Hz"
            font.pixelSize: Style.fontSizeCaption
            font.weight: Font.Normal
            color: Theme.colors.onSurfaceVariant
          }
        }
        VolumeRow {
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.bottom: parent.bottom
          visible: card.modelData.supported
          icon: "brightness-high"
          mutedIcon: "brightness-low"
          value: card.modelData.brightness
          onMoved: v => Brightness.setDisplay(card.modelData.id, v)
        }
        NText {
          anchors.left: parent.left
          anchors.bottom: parent.bottom
          visible: !card.modelData.supported
          text: "Este monitor não aceita controle de brilho (ative DDC/CI no menu do monitor)."
          font.pixelSize: Style.fontSizeCaption
          font.weight: Font.Normal
          color: Theme.colors.onSurfaceVariant
        }
      }
    }

    NListItem {
      width: col.width
      icon: "refresh"
      title: "Procurar monitores novamente"
      onClicked: Brightness.refresh()
    }
  }
}
