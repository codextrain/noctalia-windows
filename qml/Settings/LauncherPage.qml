import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

Column {
  spacing: Style.spaceMd

  SettingSection {
    title: "Atalhos globais"
    SettingRow {
      label: "Launcher"
      description: "Ex.: Alt+Space, Ctrl+Space, Win+Alt+Space, F12. Vazio = desligado."
      TextBox { value: Config.keybinds.launcher ?? ""; onEdited: v => Config.setValue("keybinds.launcher", v) }
    }
    SettingRow { label: "Central de controle"; TextBox { value: Config.keybinds["control-center"] ?? ""; onEdited: v => Config.setValue("keybinds.control-center", v) } }
    SettingRow { label: "Área de transferência"; TextBox { value: Config.keybinds.clipboard ?? ""; onEdited: v => Config.setValue("keybinds.clipboard", v) } }
    SettingRow { label: "Papel de parede"; TextBox { value: Config.keybinds.wallpaper ?? ""; onEdited: v => Config.setValue("keybinds.wallpaper", v) } }
    NText {
      width: parent.width
      visible: Panels.hotkeyErrors.length > 0
      wrapMode: Text.Wrap
      color: Theme.colors.error
      font.pixelSize: Style.fontSizeCaption
      text: "Não deu para registrar: " + Panels.hotkeyErrors.join(" · ")
    }
  }

  SettingSection {
    title: "Launcher"
    SettingRow {
      label: "Abrir em"
      NSegmented {
        model: [{ key: "recent", text: "Recente" }, { key: "pinned", text: "Fixado" }, { key: "all", text: "Todos" }]
        current: Config.launcher.default_view ?? "recent"
        onSelected: key => Config.setValue("launcher.default_view", key)
      }
    }
    SettingRow { label: "Largura"; NumberBox { value: Config.launcher.width ?? 760; step: 20; from: 560; to: 1200; onEdited: v => Config.setValue("launcher.width", v) } }
    SettingRow { label: "Grade de ícones"; NToggle { checked: Config.launcher.app_grid ?? false; onToggled: on => Config.setValue("launcher.app_grid", on) } }
    SettingRow { label: "Compacto"; NToggle { checked: Config.launcher.compact ?? false; onToggled: on => Config.setValue("launcher.compact", on) } }
    SettingRow { label: "Mostrar ícones"; NToggle { checked: Config.launcher.show_icons ?? true; onToggled: on => Config.setValue("launcher.show_icons", on) } }
    SettingRow {
      label: "Priorizar os mais usados"
      NToggle { checked: Config.launcher.sort_by_usage ?? true; onToggled: on => Config.setValue("launcher.sort_by_usage", on) }
    }
    SettingRow {
      label: "Lembrar o que eu abro"
      description: "Usado na aba Recente."
      NToggle { checked: Config.launcher.track_usage ?? true; onToggled: on => Config.setValue("launcher.track_usage", on) }
    }
    SettingRow {
      label: "Colar automaticamente"
      description: "Emoji, resultados da calculadora e itens da área de transferência vão direto para a janela que você estava usando."
      NToggle { checked: Config.launcher.auto_paste ?? true; onToggled: on => Config.setValue("launcher.auto_paste", on) }
    }
    SettingRow {
      label: "Prefixo dos provedores"
      description: "Caractere antes de calc, emo, win, session."
      TextBox { width: 80; value: Config.launcher.provider_prefix ?? "/"; onEdited: v => Config.setValue("launcher.provider_prefix", v) }
    }
    SettingRow {
      label: "Histórico de uso"
      NListItem { width: 160; icon: "trash"; title: "Limpar"; onClicked: Launcher.resetUsage() }
    }
  }
}
