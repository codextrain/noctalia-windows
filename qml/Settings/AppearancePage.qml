import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

Column {
  spacing: Style.spaceMd

  SettingSection {
    title: "Cores"
    SettingRow {
      label: "Modo"
      description: "Automático segue o modo claro/escuro do Windows."
      NSegmented {
        model: [{ key: "dark", text: "Escuro" }, { key: "light", text: "Claro" }, { key: "auto", text: "Automático" }]
        current: Config.theme.mode ?? "dark"
        onSelected: key => Theme.setMode(key)
      }
    }
    SettingRow {
      label: "Origem das cores"
      description: "Paleta pronta ou cores geradas a partir do papel de parede (Material You)."
      NSegmented {
        model: [{ key: "builtin", text: "Paleta" }, { key: "wallpaper", text: "Papel de parede" }]
        current: Config.theme.source ?? "builtin"
        onSelected: key => key === "wallpaper" ? Theme.useWallpaper(Config.theme.wallpaper_scheme ?? "m3-content")
                                               : Theme.setBuiltin(Config.theme.builtin ?? "Noctalia")
      }
    }
  }

  // palette chooser with color previews
  SettingSection {
    title: Config.theme.source === "wallpaper" ? "Estilo das cores do papel de parede" : "Paleta"
    Flow {
      width: parent.width
      spacing: Style.spaceSm
      visible: Config.theme.source !== "wallpaper"
      Repeater {
        model: Theme.palettes
        delegate: Rectangle {
          id: chip
          required property string modelData
          readonly property bool isCurrent: (Config.theme.builtin ?? "Noctalia") === modelData
          width: 150
          height: 44
          radius: Style.radiusLg
          color: chipMouse.containsMouse ? Theme.colors.hover : Theme.colors.surface
          border.width: isCurrent ? 2 : Style.borderWidth
          border.color: isCurrent ? Theme.colors.primary : Theme.colors.outlineVariant
          NText {
            anchors.left: parent.left
            anchors.leftMargin: Style.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            text: chip.modelData
            font.weight: chip.isCurrent ? Font.Bold : Font.Normal
            color: chipMouse.containsMouse ? Theme.colors.onHover : Theme.colors.onSurface
          }
          NGlyph {
            visible: chip.isCurrent
            anchors.right: parent.right
            anchors.rightMargin: Style.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            icon: "check"
            color: Theme.colors.primary
          }
          MouseArea {
            id: chipMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: Theme.setBuiltin(chip.modelData)
          }
        }
      }
    }
    Flow {
      width: parent.width
      spacing: Style.spaceSm
      visible: Config.theme.source === "wallpaper"
      Repeater {
        model: [
          { key: "m3-content", text: "Conteúdo" }, { key: "m3-tonal-spot", text: "Tonal" },
          { key: "m3-fruit-salad", text: "Salada de frutas" }, { key: "m3-rainbow", text: "Arco-íris" },
          { key: "m3-monochrome", text: "Monocromático" }, { key: "vibrant", text: "Vibrante" },
          { key: "faithful", text: "Fiel" }, { key: "soft", text: "Suave" }, { key: "muted", text: "Discreto" },
          { key: "dysfunctional", text: "Disfuncional" },
        ]
        delegate: NListItem {
          required property var modelData
          width: 160
          title: modelData.text
          checked: (Config.theme.wallpaper_scheme ?? "m3-content") === modelData.key
          onClicked: Theme.useWallpaper(modelData.key)
        }
      }
    }
  }

  SettingSection {
    title: "Interface"
    SettingRow {
      label: "Fonte"
      description: "Vazio = Segoe UI Variable."
      TextBox { value: Config.shell.font_family ?? ""; onEdited: v => Config.setValue("shell.font_family", v) }
    }
    SettingRow {
      label: "Arredondamento dos cantos"
      description: "0 = quadrado, 1 = padrão, 2 = bem arredondado."
      NumberBox { value: Config.shell.corner_radius_scale ?? 1; step: 0.25; to: 3; decimals: 2; onEdited: v => Config.setValue("shell.corner_radius_scale", v) }
    }
    SettingRow {
      label: "Escala"
      NumberBox { value: Config.shell.ui_scale ?? 1; step: 0.05; from: 0.5; to: 2.5; decimals: 2; onEdited: v => Config.setValue("shell.ui_scale", v) }
    }
    SettingRow {
      label: "Animações"
      NToggle { checked: Config.shell.animation_enabled ?? true; onToggled: on => Config.setValue("shell.animation_enabled", on) }
    }
    SettingRow {
      label: "Velocidade das animações"
      NumberBox { value: Config.shell.animation_speed ?? 1; step: 0.25; from: 0.25; to: 4; decimals: 2; onEdited: v => Config.setValue("shell.animation_speed", v) }
    }
    SettingRow {
      label: "Formato da hora"
      description: "HH:mm = 24h · hh:mm AP = 12h · HH:mm:ss = com segundos"
      TextBox { value: Config.shell.time_format ?? "HH:mm"; onEdited: v => Config.setValue("shell.time_format", v) }
    }
  }
}
