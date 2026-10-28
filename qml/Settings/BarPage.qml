import QtQuick
import QtQuick.Window
import "../Commons/BarLayout.js" as BarLayout
import Noctalia
import "../Commons"
import "../Widgets"

Column {
  id: page
  spacing: Style.spaceMd

  readonly property var allWidgets: [
    { key: "running-apps", text: "Aplicativos da área atual", icon: "apps" },
    { key: "gpu-cat", text: "Gato da GPU", icon: "cat" },
    { key: "launcher", text: "Launcher", icon: "search" }, { key: "wallpaper", text: "Papel de parede", icon: "alien" },
    { key: "workspaces", text: "Janelas", icon: "layout-grid" }, { key: "clock", text: "Relógio", icon: "clock" },
    { key: "media", text: "Mídia", icon: "disc" }, { key: "notifications", text: "Notificações", icon: "bell" },
    { key: "clipboard", text: "Área de transferência", icon: "clipboard" }, { key: "network", text: "Rede", icon: "wifi" },
    { key: "bluetooth", text: "Bluetooth", icon: "bluetooth" }, { key: "volume", text: "Volume", icon: "volume" },
    { key: "brightness", text: "Brilho", icon: "brightness-high" }, { key: "battery", text: "Bateria", icon: "battery-4" },
    { key: "weather", text: "Clima", icon: "weather-cloud-sun" }, { key: "sysmon", text: "Monitor do sistema", icon: "cpu-usage" },
    { key: "tray", text: "Bandeja", icon: "chevron-up" },
    { key: "control-center", text: "Central de controle", icon: "vitoria-wolf" }, { key: "session", text: "Sessão", icon: "shutdown" },
  ]
  function meta(key) { return allWidgets.find(w => w.key === key) ?? { key: key, text: key, icon: "apps" }; }
  readonly property var layout: BarLayout.read(Config.bar)
  readonly property var used: layout.start.concat(layout.center, layout.end)
  property bool dragging: false
  property string saveError: ""
  function moveWidget(key, section, index) {
    const next = BarLayout.move(Config.bar, key, section, index);
    if (!next) return;
    const json = JSON.stringify(next);
    const ok = typeof Config.setBarLayoutJson === "function"
        ? Config.setBarLayoutJson(json)
        : Config.setValue("bar.widget_layout_json", json);
    saveError = ok ? "" : "Não foi possível salvar. Sua organização anterior foi mantida.";
  }
  function beginDrag(key, mouse, area) {
    dragGhost.widgetKey = key;
    const p = area.mapToItem(dragGhost.parent, mouse.x, mouse.y);
    dragGhost.x = p.x - dragGhost.width / 2;
    dragGhost.y = p.y - dragGhost.height / 2;
  }
  function finishDrag() {
    if (dragging) dragGhost.Drag.drop();
    dragging = false;
  }
  BarPinnedApps { width: parent.width }
  Rectangle {
    id: dragGhost
    parent: page.Window.window ? page.Window.window.contentItem : page
    property string widgetKey: ""
    width: 180; height: 36; z: 10000
    radius: Style.radiusMd
    color: Theme.colors.primary
    visible: page.dragging
    Drag.active: page.dragging
    Drag.source: dragGhost
    Drag.keys: ["noctalia-bar-widget"]
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2
    Drag.supportedActions: Qt.MoveAction
    NText { anchors.centerIn: parent; width: parent.width - 20; text: page.meta(dragGhost.widgetKey).text; color: Theme.colors.surface; horizontalAlignment: Text.AlignHCenter }
  }
  component WidgetTile: Rectangle {
    id: tile
    required property string widgetKey
    property bool removable: false
    width: parent.width; height: 36
    radius: Style.radiusMd
    color: tileMouse.containsMouse ? Theme.colors.hover : Theme.colors.surface
    border.width: 1; border.color: Theme.colors.outlineVariant
    Row {
      anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter
      spacing: 6
      NGlyph { icon: "grip-vertical"; size: 14; color: Theme.colors.onSurfaceVariant; anchors.verticalCenter: parent.verticalCenter }
      NText { text: page.meta(tile.widgetKey).text; width: tile.width - (tile.removable ? 68 : 34); font.pixelSize: Style.fontSizeCaption; anchors.verticalCenter: parent.verticalCenter }
    }
    MouseArea {
      id: tileMouse
      anchors.fill: parent
      anchors.rightMargin: tile.removable ? 30 : 0
      hoverEnabled: true
      cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
      preventStealing: true
      drag.target: dragGhost
      drag.axis: Drag.XAndYAxis
      onPressed: mouse => page.beginDrag(tile.widgetKey, mouse, tileMouse)
      onPositionChanged: if (pressed && drag.active) page.dragging = true
      onReleased: page.finishDrag()
      onCanceled: page.dragging = false
    }
    NIconButton {
      visible: tile.removable
      anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
      width: 28; height: 30; icon: "x"
      onClicked: page.moveWidget(tile.widgetKey, "", 0)
    }
  }

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
    NText { width: parent.width; wrapMode: Text.WordWrap; text: "Arraste um widget para Esquerda, Centro ou Direita. Arraste novamente para mudar a ordem."; font.pixelSize: Style.fontSizeCaption; color: Theme.colors.onSurfaceVariant }
    NText { width: parent.width; visible: page.saveError.length > 0; text: page.saveError; wrapMode: Text.WordWrap }
    Row {
      width: parent.width
      spacing: 12
      Repeater {
        model: [{ key: "start", text: "Esquerda" }, { key: "center", text: "Centro" }, { key: "end", text: "Direita" }]
        delegate: Column {
          id: sectionCol
          required property var modelData
          readonly property var items: page.layout[modelData.key]
          property int insertionIndex: items.length
          width: (parent.width - 24) / 3
          spacing: 8
          NText { text: sectionCol.modelData.text; font.weight: Font.Bold; color: Theme.colors.primary }
          Rectangle {
            id: dropBox
            width: sectionCol.width
            height: Math.max(120, Math.min(180, sectionCol.items.length * 42 + 24))
            clip: true
            radius: Style.radiusMd
            color: destination.containsDrag ? Style.alpha(Theme.colors.primary, 0.12) : Theme.colors.surfaceContainer
            border.width: destination.containsDrag ? 2 : 1
            border.color: destination.containsDrag ? Theme.colors.primary : Theme.colors.outlineVariant
            Flickable {
              id: sectionScroll
              anchors.fill: parent; anchors.margins: 8
              contentHeight: sectionCol.items.length * 42 + 8
              boundsBehavior: Flickable.StopAtBounds
              clip: true
              Column {
              y: 4; width: sectionScroll.width; spacing: 6
              Repeater {
                model: sectionCol.items
                delegate: WidgetTile {
                  required property string modelData
                  widgetKey: modelData
                  removable: true
                }
              }
            }
            }
            NText { anchors.centerIn: parent; width: parent.width - 16; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; visible: sectionCol.items.length === 0; text: "Solte aqui"; font.pixelSize: Style.fontSizeCaption; color: Theme.colors.onSurfaceVariant }
            Rectangle { x: 8; y: 9 + sectionCol.insertionIndex * 42 - sectionScroll.contentY; width: parent.width - 16; height: 3; radius: 1; color: Theme.colors.primary; visible: destination.containsDrag }
            DropArea {
              id: destination
              anchors.fill: parent
              keys: ["noctalia-bar-widget"]
              onEntered: drag => sectionCol.insertionIndex = Math.max(0, Math.min(sectionCol.items.length, Math.floor((drag.y - 12 + sectionScroll.contentY + 21) / 42)))
              onPositionChanged: drag => sectionCol.insertionIndex = Math.max(0, Math.min(sectionCol.items.length, Math.floor((drag.y - 12 + sectionScroll.contentY + 21) / 42)))
              onDropped: drop => {
                const key = drop.source.widgetKey;
                const section = sectionCol.modelData.key;
                const index = sectionCol.insertionIndex;
                drop.acceptProposedAction();
                Qt.callLater(() => page.moveWidget(key, section, index));
              }
            }
          }
        }
      }
    }
    NText { text: "Disponíveis"; font.weight: Font.Bold; color: Theme.colors.primary }
    NText { width: parent.width; text: "Para remover, arraste de volta para esta área ou use o ×."; wrapMode: Text.WordWrap; font.pixelSize: Style.fontSizeCaption; color: Theme.colors.onSurfaceVariant }
    Rectangle {
      width: parent.width
      height: Math.max(90, availableFlow.implicitHeight + 20)
      radius: Style.radiusMd
      color: unusedDrop.containsDrag ? Style.alpha(Theme.colors.primary, 0.12) : Theme.colors.surfaceContainer
      border.width: unusedDrop.containsDrag ? 2 : 1
      border.color: unusedDrop.containsDrag ? Theme.colors.primary : Theme.colors.outlineVariant
      Flow {
        id: availableFlow
        x: 10; y: 10; width: parent.width - 20; spacing: 6
        Repeater {
          model: page.allWidgets.filter(w => page.used.indexOf(w.key) < 0)
          delegate: WidgetTile {
            required property var modelData
            widgetKey: modelData.key
            width: Math.min(180, availableFlow.width)
          }
        }
      }
      DropArea {
        id: unusedDrop
        anchors.fill: parent
        keys: ["noctalia-bar-widget"]
        onDropped: drop => {
          const key = drop.source.widgetKey;
          drop.acceptProposedAction();
          Qt.callLater(() => page.moveWidget(key, "", 0));
        }
      }
    }
  }
}
