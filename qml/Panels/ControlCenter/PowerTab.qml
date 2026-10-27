import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// Battery status (when there is one) and Windows power plans.
Column {
  id: tab
  spacing: Style.spaceMd

  Component.onCompleted: Battery.refreshPlans()

  NCard {
    width: tab.width
    height: 110
    visible: Battery.present

    NGlyph {
      id: batIcon
      icon: Battery.glyph
      size: 48
      color: Battery.percent < 15 && !Battery.plugged ? Theme.colors.error : Theme.colors.primary
      anchors.left: parent.left
      anchors.verticalCenter: parent.verticalCenter
    }
    Column {
      anchors.left: batIcon.right
      anchors.leftMargin: Style.spaceMd
      anchors.right: parent.right
      anchors.verticalCenter: parent.verticalCenter
      spacing: Style.spaceXs
      NText {
        text: Battery.percent + "%"
        font.pixelSize: Style.fontSizeHeader
        font.weight: Font.Bold
      }
      NText {
        font.weight: Font.Normal
        color: Theme.colors.onSurfaceVariant
        text: Battery.charging ? "Carregando"
            : Battery.plugged ? "Conectado à tomada"
            : Battery.minutesLeft > 0 ? Math.floor(Battery.minutesLeft / 60) + "h " + Battery.minutesLeft % 60 + "min restantes"
            : "Na bateria"
      }
      NProgress {
        width: parent.width
        value: Battery.percent / 100
        accent: Battery.percent < 15 && !Battery.plugged ? Theme.colors.error : Theme.colors.primary
      }
    }
  }

  NCard {
    width: tab.width
    height: planCol.implicitHeight + 2 * padding

    Column {
      id: planCol
      width: parent.width
      spacing: Style.spaceSm
      NSectionTitle { text: "Plano de energia" }
      Repeater {
        model: Battery.plans
        delegate: NListItem {
          required property var modelData
          width: planCol.width
          icon: modelData.name.toLowerCase().indexOf("econom") >= 0 ? "powersaver"
              : modelData.name.toLowerCase().indexOf("desempenho") >= 0 || modelData.name.toLowerCase().indexOf("performance") >= 0 ? "performance"
              : "balanced"
          title: modelData.name
          checked: modelData.active
          onClicked: Battery.setPlan(modelData.guid)
        }
      }
      NListItem {
        width: planCol.width
        icon: "external-link"
        title: "Mais opções de energia do Windows"
        onClicked: {
          Panels.close();
          Shell.openUri("ms-settings:powersleep");
        }
      }
    }
  }
}
