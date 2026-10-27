import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

Column {
  spacing: Style.spaceMd

  SettingSection {
    title: "Dock"
    SettingRow {
      label: "Mostrar o dock"
      description: "Dica: com o dock ligado, ative \"Esconder a barra de tarefas do Windows\" em Sistema."
      NToggle { checked: Config.dock.enabled ?? false; onToggled: on => Config.setValue("dock.enabled", on) }
    }
    SettingRow {
      label: "Posição"
      NSegmented {
        model: [{ key: "bottom", text: "Embaixo" }, { key: "top", text: "Topo" }]
        current: Config.dock.position ?? "bottom"
        onSelected: key => Config.setValue("dock.position", key)
      }
    }
    SettingRow { label: "Tamanho dos ícones"; NumberBox { value: Config.dock.icon_size ?? 48; step: 4; from: 24; to: 96; onEdited: v => Config.setValue("dock.icon_size", v) } }
    SettingRow { label: "Ampliar ao passar o mouse"; NToggle { checked: Config.dock.magnification ?? true; onToggled: on => Config.setValue("dock.magnification", on) } }
    SettingRow {
      label: "Ampliação"
      NumberBox { value: Config.dock.magnification_scale ?? 1.45; step: 0.05; from: 1; to: 2.5; decimals: 2; onEdited: v => Config.setValue("dock.magnification_scale", v) }
    }
    SettingRow { label: "Mostrar apps abertos"; NToggle { checked: Config.dock.show_running ?? true; onToggled: on => Config.setValue("dock.show_running", on) } }
    SettingRow {
      label: "Botão do launcher"
      NSegmented {
        model: [{ key: "start", text: "Início" }, { key: "end", text: "Fim" }, { key: "none", text: "Nenhum" }]
        current: Config.dock.launcher_position ?? "start"
        onSelected: key => Config.setValue("dock.launcher_position", key)
      }
    }
    SettingRow {
      label: "Opacidade do fundo"
      NumberBox { value: Config.dock.background_opacity ?? 0.88; step: 0.05; decimals: 2; to: 1; onEdited: v => Config.setValue("dock.background_opacity", v) }
    }
    NText {
      width: parent.width
      wrapMode: Text.Wrap
      font.pixelSize: Style.fontSizeCaption
      color: Theme.colors.onSurfaceVariant
      text: "Para fixar apps no dock, clique com o botão direito num app aberto no dock → \"Fixar no dock\"."
    }
  }

  SettingSection {
    title: "Widgets na área de trabalho"
    SettingRow {
      label: "Mostrar widgets"
      description: "Arraste para mover, use a rodinha do mouse para mudar o tamanho."
      NToggle { checked: Config.desktopWidgets.enabled ?? false; onToggled: on => Config.setValue("desktop_widgets.enabled", on) }
    }
    Repeater {
      model: Config.desktopWidgets.widgets ?? []
      delegate: SettingRow {
        required property var modelData
        required property int index
        label: ({ clock: "Relógio", media: "Mídia", sysmon: "Monitor do sistema", weather: "Clima" })[modelData.type] ?? modelData.type
        NIconButton {
          icon: "trash"
          onClicked: {
            const l = (Config.desktopWidgets.widgets ?? []).slice();
            l.splice(index, 1);
            Config.setValue("desktop_widgets.widgets", l);
          }
        }
      }
    }
    Flow {
      width: parent.width
      spacing: Style.spaceXs
      Repeater {
        model: [{ key: "clock", text: "Relógio" }, { key: "media", text: "Mídia" }, { key: "sysmon", text: "Monitor do sistema" }, { key: "weather", text: "Clima" }]
        delegate: NListItem {
          required property var modelData
          width: 170
          icon: "plus"
          title: modelData.text
          onClicked: {
            const l = (Config.desktopWidgets.widgets ?? []).slice();
            l.push({ type: modelData.key, x: 120 + 40 * l.length, y: 160 + 40 * l.length, scale: 1 });
            Config.setValue("desktop_widgets.widgets", l);
            Config.setValue("desktop_widgets.enabled", true);
          }
        }
      }
    }
  }
}
