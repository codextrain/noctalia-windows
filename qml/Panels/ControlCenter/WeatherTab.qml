import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// Current weather and the next days (Open-Meteo).
Item {
  id: tab

  Component.onCompleted: if (!Weather.available) Weather.refresh()

  Column {
    anchors.centerIn: parent
    width: parent.width
    spacing: Style.spaceSm
    visible: !Weather.available
    NGlyph {
      anchors.horizontalCenter: parent.horizontalCenter
      icon: "weather-cloud-sun"
      size: 56
      color: Theme.colors.onSurfaceVariant
    }
    NText {
      width: parent.width
      horizontalAlignment: Text.AlignHCenter
      wrapMode: Text.Wrap
      text: Weather.loading ? "Buscando previsão..." : Weather.error.length > 0 ? Weather.error : "Dados de clima indisponíveis"
      color: Theme.colors.onSurfaceVariant
    }
  }

  Column {
    anchors.fill: parent
    spacing: Style.spaceMd
    visible: Weather.available

    NCard {
      width: parent.width
      height: 150

      NGlyph {
        id: bigIcon
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        icon: Weather.glyph
        size: 72
        color: Theme.colors.primary
      }
      Column {
        anchors.left: bigIcon.right
        anchors.leftMargin: Style.spaceLg
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        NText {
          text: Weather.temperatureText
          font.pixelSize: 40
          font.weight: Font.Bold
        }
        NText {
          text: Weather.description
          font.weight: Font.DemiBold
        }
        NText {
          text: Weather.locationName
          font.pixelSize: Style.fontSizeCaption
          font.weight: Font.Normal
          color: Theme.colors.onSurfaceVariant
        }
      }
      Column {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: Style.spaceXs
        Repeater {
          model: [
            { icon: "temperature", text: "Sensação " + Math.round(Weather.apparent) + Weather.unit },
            { icon: "droplet", text: "Umidade " + Weather.humidity + "%" },
            { icon: "wind", text: "Vento " + Math.round(Weather.wind) + " km/h" },
          ]
          delegate: Row {
            required property var modelData
            spacing: Style.spaceXs
            NGlyph { icon: modelData.icon; size: Style.fontSizeBody; color: Theme.colors.onSurfaceVariant }
            NText {
              text: modelData.text
              font.pixelSize: Style.fontSizeCaption
              font.weight: Font.Normal
              color: Theme.colors.onSurfaceVariant
            }
          }
        }
      }
    }

    Row {
      width: parent.width
      spacing: Style.spaceSm
      Repeater {
        model: Weather.daily
        delegate: NCard {
          required property var modelData
          width: (tab.width - Style.spaceSm * 5) / 6
          height: 150
          padding: Style.spaceSm
          Column {
            anchors.centerIn: parent
            spacing: Style.spaceSm
            NText {
              anchors.horizontalCenter: parent.horizontalCenter
              text: modelData.dayName
              font.weight: Font.Bold
            }
            NGlyph {
              anchors.horizontalCenter: parent.horizontalCenter
              icon: modelData.glyph
              size: 30
              color: Theme.colors.primary
            }
            NText {
              anchors.horizontalCenter: parent.horizontalCenter
              text: modelData.max + "°"
              font.weight: Font.Bold
            }
            NText {
              anchors.horizontalCenter: parent.horizontalCenter
              text: modelData.min + "°"
              font.weight: Font.Normal
              color: Theme.colors.onSurfaceVariant
            }
          }
        }
      }
    }
  }
}
