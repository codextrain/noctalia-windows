import QtQuick
import QtQuick.Window
import Noctalia
import "../Commons"
import "../Widgets"

// Noctalia's dock: pinned + running apps with magnification, at the bottom of a monitor.
// Click = open / focus / minimize · right click = windows, pin, close · scroll = cycle windows
Window {
  id: dock

  required property var modelData // QScreen
  screen: modelData

  readonly property var cfg: Config.dock
  readonly property bool atTop: cfg.position === "top"
  readonly property int iconSize: cfg.icon_size ?? 48
  readonly property int spacing: cfg.item_spacing ?? 6
  readonly property int padMain: cfg.main_axis_padding ?? 16
  readonly property int padCross: cfg.cross_axis_padding ?? 8
  readonly property int marginEdge: cfg.margin_edge ?? 8
  readonly property real magScale: (cfg.magnification ?? true) ? (cfg.magnification_scale ?? 1.45) : 1.0
  readonly property int dockHeight: iconSize + 2 * padCross + 6 // +6: running dots
  readonly property int headroom: Math.ceil(iconSize * (magScale - 1)) + 4
  readonly property bool hovering: dockMouse.containsMouse
  property real mouseX: -1000

  readonly property bool barOnSameEdge: (Config.bar.position === "top") === atTop
  readonly property int barReserve: barOnSameEdge ? (Config.bar.margin_edge ?? 10) + (Config.bar.thickness ?? 34)
                                                    + (Config.bar.margin_opposite_edge ?? 6) : 0

  flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus | Qt.NoDropShadowWindowHint
  color: "transparent"
  objectName: "dock"
  title: "Noctalia Dock"
  visible: cfg.enabled ?? false

  // Layout without magnification: icon centers are computed from this, never from the
  // (zoomed) live positions, so magnification can't feed back into itself.
  readonly property bool launcherStart: (cfg.launcher_position ?? "start") === "start"
  readonly property bool launcherEnd: (cfg.launcher_position ?? "start") === "end"
  readonly property int slotCount: Dock.items.length + (launcherStart || launcherEnd ? 1 : 0)
  readonly property real baseRowWidth: slotCount * iconSize + Math.max(0, slotCount - 1) * spacing
  function slotCenter(i) { return (width - baseRowWidth) / 2 + i * (iconSize + spacing) + iconSize / 2; }

  width: Math.min(screen.width - 40, baseRowWidth + 2 * padMain + Math.ceil(2 * iconSize * (magScale - 1)) + 40)
  // Constant height (room for magnified icons included) and that whole strip is reserved, so
  // the dock never covers maximized windows and the window never resizes under the mouse.
  height: dockHeight + marginEdge + (magScale > 1 ? headroom : 0)
  x: screen.virtualX + (screen.width - width) / 2
  y: atTop ? screen.virtualY + appBar.offset + barReserve
           : screen.virtualY + screen.height - appBar.offset - height

  AppBar {
    id: appBar
    window: dock
    edge: dock.atTop ? "top" : "bottom"
    thickness: dock.height
    active: dock.cfg.reserve_space ?? true
  }

  NTooltip { id: tooltip }
  function showTip(item, text) { tooltip.show(item, text, atTop); }
  function hideTip() { tooltip.hide(); }
  NContextMenu { id: menu }

  function openMenu(item, entry) {
    const entries = [];
    for (const w of entry.windows)
      entries.push({ icon: w.focused ? "app-window" : "app-window", text: w.title.length > 40 ? w.title.slice(0, 40) + "…" : w.title,
                     action: () => Dock.focusWindow(w.handle) });
    if (entries.length > 0)
      entries.push({ separator: true });
    entries.push({ icon: "plus", text: entry.running ? "Nova janela" : "Abrir", action: () => Dock.launchNew(entry.key) });
    if (entry.appId.length > 0)
      entries.push(entry.pinned ? { icon: "pinned-off", text: "Desfixar do dock", action: () => Dock.setPinned(entry.appId, false) }
                                : { icon: "pin", text: "Fixar no dock", action: () => Dock.setPinned(entry.appId, true) });
    if (entry.running)
      entries.push({ icon: "x", text: entry.windows.length > 1 ? "Fechar todas as janelas" : "Fechar janela", action: () => Dock.closeAll(entry.key) });
    menu.model = entries;
    menu.open(item, dock.atTop);
  }

  Item {
    id: body
    anchors.fill: parent

    // background pill (stays at the base size; icons grow out of it)
    Rectangle {
      id: bg
      width: row.width + 2 * dock.padMain
      height: dock.dockHeight
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.bottom: dock.atTop ? undefined : parent.bottom
      anchors.bottomMargin: dock.atTop ? 0 : dock.marginEdge
      anchors.top: dock.atTop ? parent.top : undefined
      anchors.topMargin: dock.atTop ? dock.marginEdge : 0
      radius: Math.min(dock.cfg.radius ?? 16, height / 2) * Style.radiusScale
      color: Style.alpha(Theme.colors.surface, dock.cfg.background_opacity ?? 0.88)
      border.width: Style.borderWidth
      border.color: Theme.colors.outlineVariant

      Behavior on width {
        NumberAnimation { duration: Style.animFast }
      }

      Repeater {
        model: (dock.cfg.shadow ?? true) ? 8 : 0
        delegate: Rectangle {
          required property int index
          z: -1
          x: -index - 1
          y: -index + 2
          width: bg.width + 2 * (index + 1)
          height: bg.height + 2 * (index + 1)
          radius: bg.radius + index + 1
          color: Qt.rgba(0, 0, 0, 0.05 * (1 - index / 8))
        }
      }
    }

    MouseArea {
      id: dockMouse
      anchors.fill: parent
      hoverEnabled: true
      acceptedButtons: Qt.NoButton
      onPositionChanged: event => dock.mouseX = event.x
      onExited: dock.mouseX = -1000
    }

    Row {
      id: row
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.bottom: dock.atTop ? undefined : bg.bottom
      anchors.bottomMargin: dock.atTop ? 0 : dock.padCross + 6
      anchors.top: dock.atTop ? bg.top : undefined
      anchors.topMargin: dock.atTop ? dock.padCross : 0
      spacing: dock.spacing

      // launcher button
      DockItem {
        visible: dock.launcherStart
        slot: 0
        dockWindow: dock
        isLauncher: true
        entry: ({ key: "launcher", name: "Aplicativos", windows: [], running: false, focused: false })
      }

      Repeater {
        model: Dock.items
        delegate: DockItem {
          required property var modelData
          required property int index
          slot: index + (dock.launcherStart ? 1 : 0)
          dockWindow: dock
          entry: modelData
        }
      }

      DockItem {
        visible: dock.launcherEnd
        slot: Dock.items.length
        dockWindow: dock
        isLauncher: true
        entry: ({ key: "launcher", name: "Aplicativos", windows: [], running: false, focused: false })
      }
    }
  }
}
