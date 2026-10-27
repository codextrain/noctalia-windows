import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

Column {
  spacing: Style.spaceMd

  SettingSection {
    title: "Windows"
    SettingRow {
      label: "Iniciar com o Windows"
      NToggle { checked: Shell.autostart; onToggled: on => Shell.autostart = on }
    }
    SettingRow {
      label: "Esconder a barra de tarefas do Windows"
      description: "Liga o \"ocultar automaticamente\" dela enquanto o Noctalia roda e devolve ao sair."
      NToggle { checked: Config.windows.autohide_taskbar ?? false; onToggled: on => Config.setValue("windows.autohide_taskbar", on) }
    }
    SettingRow {
      label: "Esconder a barra em tela cheia"
      description: "Jogos e vídeos em tela cheia."
      NToggle { checked: Config.windows.hide_on_fullscreen ?? true; onToggled: on => Config.setValue("windows.hide_on_fullscreen", on) }
    }
  }

  SettingSection {
    title: "NiriWin"
    SettingRow {
      label: "Pasta do NiriWin"
      description: Shell.niriStatusUrl.length > 0
                   ? "Encontrado: " + decodeURIComponent(Shell.niriStatusUrl.replace("file:///", "")).replace(/\//g, "\\")
                   : "Não encontrado. Com o NiriWin aberto ele é achado sozinho; se não, cole aqui a pasta dele (ex.: C:\\NiriWin)."
      TextBox {
        width: 260
        value: Config.windows.niriwin_path ?? ""
        placeholder: "automático"
        onEdited: v => Config.setValue("windows.niriwin_path", v)
      }
    }
  }

  SettingSection {
    title: "Arquivo de configuração"
    SettingRow {
      label: Config.path
      description: Config.lastError.length > 0 ? Config.lastError : "Tudo que você muda aqui é salvo nesse arquivo."
      Row {
        spacing: Style.spaceXs
        NIconButton { icon: "external-link"; onClicked: Config.openInEditor() }
        NIconButton { icon: "folder"; onClicked: Config.openFolder() }
        NIconButton { icon: "refresh"; onClicked: Config.reload() }
      }
    }
  }

  SettingSection {
    title: "Sobre"
    SettingRow {
      label: "Noctalia para Windows " + Shell.version
      description: "Port não oficial do Noctalia (noctalia.dev), MIT. Ícones Tabler. " + Shell.osName
      NIconButton { icon: "external-link"; onClicked: Shell.openUri("https://github.com/codextrain/noctalia-windows") }
    }
  }
}
