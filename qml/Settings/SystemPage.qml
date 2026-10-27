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
      description: "Oculta de verdade, sem aparecer ao passar o mouse. Desligue esta opção para mostrar novamente. Requer NiriWin ativo; pausar ou parar restaura a barra."
      NToggle { checked: Config.windows.hide_taskbar_permanently ?? false; onToggled: on => {
        if(on) Config.setValue("windows.autohide_taskbar",false);
        Config.setValue("windows.hide_taskbar_permanently",on);
      } }
    }
    SettingRow {
      label: "Mostrar a barra do Windows"
      description: "Desativa a ocultação completa."
      NIconButton {icon:"eye";onClicked:Config.setValue("windows.hide_taskbar_permanently",false)}
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
