import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"
import "../Commons/LocationPolicy.js" as LocationPolicy

Column {
  id: page
  property string locationError: ""
  spacing: Style.spaceMd

  SettingSection {
    title: "Clima"
    SettingRow { label: "Mostrar clima"; NToggle { checked: Config.weather.enabled ?? true; onToggled: on => Config.setValue("weather.enabled", on) } }
    SettingRow {
      label: "Cidade"
      description: page.locationError || "Ex.: São Bento do Sul, SC. Confirmar uma cidade desativa a localização por IP."
      TextBox { value: Config.location.address ?? ""; onEdited: v => {
        const next=LocationPolicy.manual(Config.location,v);
        page.locationError=next?"":"Digite o nome de uma cidade.";
        if(next)Config.setValue("location",next);
      } }
    }
    SettingRow {
      label: "Descobrir pela internet"
      description: "O IP pode indicar uma cidade vizinha. Deixe desligado para usar a cidade escolhida acima."
      NToggle { checked: Config.location.auto_locate ?? false; onToggled: on => Config.setValue("location.auto_locate", on) }
    }
    SettingRow {
      label: "Unidade"
      NSegmented {
        model: [{ key: "celsius", text: "°C" }, { key: "fahrenheit", text: "°F" }]
        current: Config.weather.unit ?? "celsius"
        onSelected: key => Config.setValue("weather.unit", key)
      }
    }
  }

  SettingSection {
    title: "Papel de parede"
    SettingRow {
      label: "Pasta"
      description: "Vazio = Imagens\\Wallpapers (ou Imagens)."
      TextBox { width: 320; value: Config.wallpaper.directory ?? ""; onEdited: v => Config.setValue("wallpaper.directory", v) }
    }
    SettingRow {
      label: "Pasta dos downloads (Wallhaven e coleções)"
      description: "Vazio = Imagens\\Wallpapers. Cada fonte ganha uma subpasta."
      TextBox { width: 320; value: Config.wallpaper.download_directory ?? ""; onEdited: v => Config.setValue("wallpaper.download_directory", v) }
    }
    SettingRow {
      label: "Chave da API do Wallhaven"
      description: "Opcional: 64 por página em vez de 24 (wallhaven.cc → Settings → Account)."
      TextBox { width: 320; value: Config.wallpaper.wallhaven?.api_key ?? ""; onEdited: v => Config.setValue("wallpaper.wallhaven.api_key", v.trim()) }
    }
    SettingRow {
      label: "Token do GitHub"
      description: "Opcional: sem ele, o GitHub deixa abrir umas 30 coleções por hora."
      TextBox { width: 320; value: Config.wallpaper.github_token ?? ""; onEdited: v => Config.setValue("wallpaper.github_token", v.trim()) }
    }
    SettingRow {
      label: "Ajuste"
      NSegmented {
        model: [{ key: "crop", text: "Preencher" }, { key: "fit", text: "Ajustar" }, { key: "stretch", text: "Esticar" }, { key: "center", text: "Centro" }, { key: "span", text: "Estender" }]
        current: Config.wallpaper.fill_mode ?? "crop"
        onSelected: key => Config.setValue("wallpaper.fill_mode", key)
      }
    }
    SettingRow {
      label: "Trocar sozinho"
      NToggle { checked: Config.wallpaper.automation?.enabled ?? false; onToggled: on => Config.setValue("wallpaper.automation.enabled", on) }
    }
    SettingRow {
      label: "A cada (minutos)"
      NumberBox {
        value: Math.round((Config.wallpaper.automation?.interval_seconds ?? 1800) / 60)
        from: 1; to: 1440; step: 5
        onEdited: v => Config.setValue("wallpaper.automation.interval_seconds", v * 60)
      }
    }
  }

  SettingSection {
    title: "OSD (aviso de volume e brilho)"
    SettingRow { label: "Mostrar"; NToggle { checked: Config.osd.enabled ?? true; onToggled: on => Config.setValue("osd.enabled", on) } }
    SettingRow {
      label: "Posição"
      NSegmented {
        model: [{ key: "top_left", text: "↖" }, { key: "top_center", text: "↑" }, { key: "top_right", text: "↗" },
                { key: "bottom_left", text: "↙" }, { key: "bottom_center", text: "↓" }, { key: "bottom_right", text: "↘" }]
        current: Config.osd.position ?? "top_right"
        onSelected: key => Config.setValue("osd.position", key)
      }
    }
    SettingRow { label: "Tempo na tela (ms)"; NumberBox { value: Config.osd.hide_delay_ms ?? 1400; step: 100; from: 250; to: 10000; onEdited: v => Config.setValue("osd.hide_delay_ms", v) } }
  }

  SettingSection {
    title: "Área de transferência"
    SettingRow { label: "Guardar histórico"; NToggle { checked: Config.shell.clipboard_enabled ?? true; onToggled: on => Config.setValue("shell.clipboard_enabled", on) } }
    SettingRow { label: "Máximo de itens"; NumberBox { value: Config.shell.clipboard_history_max_entries ?? 100; step: 10; from: 10; to: 1000; onEdited: v => Config.setValue("shell.clipboard_history_max_entries", v) } }
  }
}
