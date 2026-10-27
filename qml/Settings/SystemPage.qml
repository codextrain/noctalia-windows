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
