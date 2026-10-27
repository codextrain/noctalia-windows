import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

Column {
  id: page
  spacing: Style.spaceMd

  readonly property var allWidgets: [
    { key: "launcher", text: "Launcher", icon: "search" }, { key: "wallpaper", text: "Papel de parede", icon: "wallpaper-selector" },
    { key: "workspaces", text: "Áreas de trabalho", icon: "layout-grid" }, { key: "clock", text: "Relógio", icon: "clock" },
    { key: "media", text: "Mídia", icon: "disc" }, { key: "notifications", text: "Notificações", icon: "bell" },
    { key: "clipboard", text: "Área de transferência", icon: "clipboard" }, { key: "network", text: "Rede", icon: "wifi" },
    { key: "bluetooth", text: "Bluetooth", icon: "bluetooth" }, { key: "volume", text: "Volume", icon: "volume" },
    { key: "brightness", text: "Brilho", icon: "brightness-high" }, { key: "battery", text: "Bateria", icon: "battery-4" },
    { key: "weather", text: "Clima", icon: "weather-cloud-sun" }, { key: "sysmon", text: "Monitor do sistema", icon: "cpu-usage" },
    { key: "tray", text: "Bandeja", icon: "chevron-up" },
    { key: "control-center", text: "Central de controle", icon: "noctalia" }, { key: "session", text: "Sessão", icon: "shutdown" },
  ]
  function meta(key) { return allWidgets.find(w => w.key === key) ?? { key: key, text: key, icon: "apps" }; }
  function list(section) { return (Config.bar[section] ?? []).slice(); }
  function save(section, l) { Config.setValue("bar." + section, l); }
  readonly property var used: list("start").concat(list("center"), list("end"))

  SettingSection {
    title: "Barra"
    SettingRow {
      label: "Posição"
      NSegmented {
        model: [{ key: "top", text: "Topo" }, { key: "bottom", text: "Embaixo" }]
        current: Config.bar.position ?? "top"
        onSelected: key => Config.setValue("bar.position", key)
      }
    }
    SettingRow { label: "Altura"; NumberBox { value: Config.bar.thickness ?? 34; from: 24; to: 64; onEdited: v => Config.setValue("bar.thickness", v) } }
    SettingRow {
      label: "Distância das laterais"
      description: "0 = barra de ponta a ponta."
      NumberBox { value: Config.bar.margin_ends ?? 180; step: 10; to: 800; onEdited: v => Config.setValue("bar.margin_ends", v) }
    }
    SettingRow { label: "Distância da borda"; NumberBox { value: Config.bar.margin_edge ?? 10; to: 60; onEdited: v => Config.setValue("bar.margin_edge", v) } }
    SettingRow { label: "Cantos"; NumberBox { value: Config.bar.radius ?? 12; to: 40; onEdited: v => Config.setValue("bar.radius", v) } }
    SettingRow {
      label: "Opacidade do fundo"
      NumberBox { value: Config.bar.background_opacity ?? 1; step: 0.05; decimals: 2; to: 1; onEdited: v => Config.setValue("bar.background_opacity", v) }
    }
    SettingRow { label: "Cápsulas atrás dos widgets"; NToggle { checked: Config.bar.capsule ?? false; onToggled: on => Config.setValue("bar.capsule", on) } }
    SettingRow { label: "Sombra"; NToggle { checked: Config.bar.shadow ?? true; onToggled: on => Config.setValue("bar.shadow", on) } }
    SettingRow {
      label: "Reservar espaço"
      description: "Janelas maximizadas não ficam por baixo da barra."
      NToggle { checked: Config.bar.reserve_space ?? true; onToggled: on => Config.setValue("bar.reserve_space", on) }
    }
  }

  SettingSection {
    title: "Widgets"

    Row {
      width: parent.width
      spacing: Style.spaceMd
      Repeater {
        model: [{ key: "start", text: "Esquerda" }, { key: "center", text: "Centro" }, { key: "end", text: "Direita" }]
        delegate: Column {
          id: sectionCol
          required property var modelData
          readonly property var items: page.list(modelData.key)
          width: (parent.width - 2 * Style.spaceMd) / 3
          spacing: 2
          NText { text: sectionCol.modelData.text; font.weight: Font.Bold; color: Theme.colors.primary }
          Repeater {
            model: sectionCol.items
            delegate: Rectangle {
              id: wRow
              required property string modelData
              required property int index
              width: sectionCol.width
              height: 34
              radius: Style.radiusMd
              color: Theme.colors.surface
              Row {
                anchors.left: parent.left
                anchors.leftMargin: Style.spaceSm
                anchors.verticalCenter: parent.verticalCenter
                spacing: Style.spaceSm
                NGlyph { icon: page.meta(wRow.modelData).icon; size: Style.fontSizeBody; anchors.verticalCenter: parent.verticalCenter }
                NText {
                  text: page.meta(wRow.modelData).text
                  font.pixelSize: Style.fontSizeCaption
                  width: sectionCol.width - 120
                  anchors.verticalCenter: parent.verticalCenter
                }
              }
              Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                NIconButton {
                  width: 26; height: 26; icon: "chevron-up"
                  onClicked: {
                    const l = page.list(sectionCol.modelData.key);
                    if (wRow.index > 0) { l.splice(wRow.index - 1, 0, l.splice(wRow.index, 1)[0]); page.save(sectionCol.modelData.key, l); }
                  }
                }
                NIconButton {
                  width: 26; height: 26; icon: "chevron-down"
                  onClicked: {
                    const l = page.list(sectionCol.modelData.key);
                    if (wRow.index < l.length - 1) { l.splice(wRow.index + 1, 0, l.splice(wRow.index, 1)[0]); page.save(sectionCol.modelData.key, l); }
                  }
                }
                NIconButton {
                  width: 26; height: 26; icon: "x"
                  onClicked: {
                    const l = page.list(sectionCol.modelData.key);
                    l.splice(wRow.index, 1);
                    page.save(sectionCol.modelData.key, l);
                  }
                }
              }
            }
          }
        }
      }
    }

    Item { width: 1; height: Style.spaceMd }
    NText { text: "Adicionar (clique para pôr na direita, botão direito para a esquerda)"; font.pixelSize: Style.fontSizeCaption; color: Theme.colors.onSurfaceVariant }
    Flow {
      width: parent.width
      spacing: Style.spaceXs
      Repeater {
        model: page.allWidgets.filter(w => page.used.indexOf(w.key) < 0)
        delegate: Rectangle {
          id: addChip
          required property var modelData
          height: 30
          width: addRow.implicitWidth + 2 * Style.spaceMd
          radius: 15
          color: addMouse.containsMouse ? Theme.colors.hover : Theme.colors.surface
          border.width: Style.borderWidth
          border.color: Theme.colors.outlineVariant
          Row {
            id: addRow
            anchors.centerIn: parent
            spacing: Style.spaceXs
            NGlyph { icon: "plus"; size: 13; color: addMouse.containsMouse ? Theme.colors.onHover : Theme.colors.primary; anchors.verticalCenter: parent.verticalCenter }
            NText { text: addChip.modelData.text; font.pixelSize: Style.fontSizeCaption; color: addMouse.containsMouse ? Theme.colors.onHover : Theme.colors.onSurface; anchors.verticalCenter: parent.verticalCenter }
          }
          MouseArea {
            id: addMouse
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            onClicked: event => {
              const section = event.button === Qt.RightButton ? "start" : "end";
              const l = page.list(section);
              l.push(addChip.modelData.key);
              page.save(section, l);
            }
          }
        }
      }
    }
  }
}
