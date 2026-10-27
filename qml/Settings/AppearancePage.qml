import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

Column {
  id: appearancePage
  spacing: Style.spaceMd
  readonly property int windowRounding: Config.windows.corner_rounding_percent ?? 25
  property string roundingError: ""
  property string blurError: ""
  function setBlurEnabled(enabled) {
    const ok=Config.setValue("windows.blur_enabled",enabled);
    blurError=ok ? "" : "Não foi possível salvar o blur.";
    return ok;
  }
  function setBlurIntensity(value) {
    const percent=Math.max(0,Math.min(100,Math.round(value)));
    const ok=Config.setValue("windows.blur_intensity_percent",percent);
    blurError=ok ? "" : "Não foi possível salvar a intensidade.";
    return ok;
  }
  function setWindowRounding(value) {
    const percent = Math.max(0, Math.min(100, Math.round(value)));
    const ok = Config.setValue("windows.corner_rounding_percent", percent);
    roundingError = ok ? "" : "Não foi possível salvar o arredondamento.";
    return ok;
  }

  SettingSection {
    title: "Janelas dos aplicativos"
    SettingRow {
      label: "Blur dos aplicativos compatíveis"
      description: "O Noctalia fica excluído para evitar margens brancas. Aplicativos com fundo opaco podem não mostrar o efeito."
      NToggle { checked: Config.windows.blur_enabled ?? false; onToggled: enabled => appearancePage.setBlurEnabled(enabled) }
    }
    SettingRow {
      label: "Intensidade no Noctalia (%)"
      description: "Indisponível nesta versão: o efeito gerou margens brancas. Os fundos originais foram preservados."
      NumberBox { enabled:false; opacity:0.45; value: Config.windows.blur_intensity_percent ?? 50; from:0; to:100; step:5; onEdited: value => appearancePage.setBlurIntensity(value) }
    }
    NText { width:parent.width; visible:appearancePage.blurError.length>0; text:appearancePage.blurError; wrapMode:Text.WordWrap }
    SettingRow {
      label: "Arredondamento das janelas"
      description: "0% = cantos retos; 100% = máximo. Aplicado pelo NiriWin. Pode alterar a sombra de alguns aplicativos."
      Column {
        spacing: 6
        Row {
          spacing: 12
          Rectangle {
            id: roundingSlider
            width: 180; height: 32; color: "transparent"
            readonly property int percent: sliderMouse.pressed ? Math.round(Math.max(0, Math.min(100, sliderMouse.mouseX / width * 100))) : appearancePage.windowRounding
            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: 6; radius: 3; color: Theme.colors.outlineVariant }
            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width * roundingSlider.percent / 100; height: 6; radius: 3; color: Theme.colors.primary }
            Rectangle { x: (parent.width - width) * roundingSlider.percent / 100; anchors.verticalCenter: parent.verticalCenter; width: 16; height: 16; radius: 8; color: Theme.colors.primary }
            MouseArea {
              id: sliderMouse
              anchors.fill: parent
              preventStealing: true
              cursorShape: Qt.PointingHandCursor
              onReleased: mouse => appearancePage.setWindowRounding(mouse.x / width * 100)
              onWheel: event => appearancePage.setWindowRounding(appearancePage.windowRounding + (event.angleDelta.y > 0 ? 1 : -1))
            }
          }
          NText { text: roundingSlider.percent + "%"; width: 38; height: 32; horizontalAlignment: Text.AlignRight }
          Rectangle { width: 40; height: 32; radius: Math.min(16, roundingSlider.percent * 0.16); color: Theme.colors.primary }
        }
        NumberBox { value: appearancePage.windowRounding; from: 0; to: 100; step: 1; onEdited: value => appearancePage.setWindowRounding(value) }
      }
    }
    NText { width: parent.width; visible: appearancePage.roundingError.length > 0; text: appearancePage.roundingError; wrapMode: Text.WordWrap }
  }

  SettingSection {
    title: "Cores"
    NText {
      width:parent.width
      visible:Config.theme.source === "wallpaper"
      text:WallpaperThemeSync.statusText
      font.pixelSize:Style.fontSizeCaption
      color:Theme.colors.onSurfaceVariant
      wrapMode:Text.WordWrap
    }
    SettingRow {
      visible:Config.theme.source === "wallpaper"
      label:"Atualizar cores agora"
      description:"Recalcula a paleta usando a imagem atual."
      NIconButton { icon:"refresh"; enabled:!Theme.generating; onClicked:WallpaperThemeSync.forceRefresh() }
    }
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
