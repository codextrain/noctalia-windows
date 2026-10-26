import QtQuick
import QtQuick.Window
import Noctalia
import "../Commons"
import "../Widgets"
import "Widgets"

// One floating Noctalia bar per monitor.
//
// Geometry (top bar; bottom is mirrored), all values from config "bar":
//
//   screen edge ─────────────────────────────────────────────────────────────
//                   margin_edge (gap above the bar)
//          ╭──────────────────── bar (thickness, radius) ─────────────────╮
//   margin_ends    start widgets        center widgets       end widgets    margin_ends
//          ╰──────────────────────────────────────────────────────────────╯
//                   margin_opposite_edge (room for the drop shadow)
//   ─────────────── maximized windows start here ───────────────────────────
//
// The whole strip is reserved with the Windows app-bar API, like Noctalia's exclusive zone.
Window {
  id: barWindow
  objectName: "bar"

  required property var modelData // the QScreen this bar lives on
  screen: modelData

  // ── config ────────────────────────────────────────────────────────────────
  readonly property var cfg: Config.bar
  readonly property bool atTop: cfg.position !== "bottom"
  readonly property int thickness: Math.round((cfg.thickness ?? 34) * Style.uiScale)
  readonly property int marginEdge: cfg.margin_edge ?? 10
  readonly property int marginOpposite: cfg.margin_opposite_edge ?? 6
  readonly property int padding: cfg.padding ?? 14
  readonly property int widgetSpacing: cfg.widget_spacing ?? 6
  readonly property real radius: (cfg.radius ?? 12) * Style.radiusScale
  readonly property real backgroundOpacity: cfg.background_opacity ?? 1.0
  readonly property bool shadowEnabled: cfg.shadow ?? true
  readonly property bool capsule: cfg.capsule ?? false
  readonly property bool reserveSpace: cfg.reserve_space ?? true
  readonly property bool hideOnFullscreen: Config.windows.hide_on_fullscreen ?? true
  readonly property real capsuleHeight: Math.round(thickness * Style.barCapsuleThickness)

  // Never let margin_ends squeeze the bar below 480px on small screens.
  readonly property int marginEnds: Math.max(0, Math.min(cfg.margin_ends ?? 180, Math.floor((screen.width - 480) / 2)))
  readonly property int sidePad: shadowEnabled ? 14 : 0 // room for the shadow left/right of the bar
  readonly property int reservedThickness: marginEdge + thickness + marginOpposite
  readonly property bool hiddenForFullscreen: hideOnFullscreen && appBar.fullscreenApp

  // ── window ────────────────────────────────────────────────────────────────
  flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
         | Qt.NoDropShadowWindowHint
  color: "transparent"
  visible: true
  title: "Noctalia Bar"

  width: screen.width - 2 * marginEnds + 2 * sidePad
  height: reservedThickness
  x: screen.virtualX + marginEnds - sidePad
  // While a fullscreen app runs we park the window off-screen instead of hiding it, so the
  // reserved space and the fullscreen notifications keep working.
  y: hiddenForFullscreen ? -10000
                         : atTop ? screen.virtualY + appBar.offset
                                 : screen.virtualY + screen.height - appBar.offset - reservedThickness

  AppBar {
    id: appBar
    window: barWindow
    edge: barWindow.atTop ? "top" : "bottom"
    thickness: barWindow.reservedThickness
    active: barWindow.reserveSpace
  }

  // ── tooltip & menus (shared by all widgets of this bar) ───────────────────
  NTooltip { id: tooltip }

  function showTooltip(item, text) { tooltip.show(item, text, atTop); }
  function updateTooltip(item, text) {
    if (tooltip.target === item) {
      tooltip.text = text;
      if (tooltip.visible)
        tooltip.place();
    }
  }
  function hideTooltip() { tooltip.hide(); }

  NContextMenu { id: menu }

  function openShellMenu(item) {
    menu.model = [
      { icon: "settings", text: "Editar configuração", action: () => Config.openInEditor() },
      { icon: "folder", text: "Abrir pasta da configuração", action: () => Config.openFolder() },
      { icon: "refresh", text: "Recarregar configuração", action: () => Config.reload() },
      { separator: true },
      { icon: "x", text: "Sair do Noctalia", action: () => Shell.quit() },
    ];
    menu.open(item, atTop);
  }

  function openSessionMenu(item) {
    menu.model = [
      { icon: "lock", text: "Bloquear", action: () => Shell.session("lock") },
      { icon: "logout", text: "Sair da conta", action: () => Shell.session("logout") },
      { icon: "suspend", text: "Suspender", action: () => Shell.session("suspend") },
      { separator: true },
      { icon: "reboot", text: "Reiniciar", action: () => Shell.session("reboot") },
      { icon: "shutdown", text: "Desligar", action: () => Shell.session("shutdown") },
    ];
    menu.open(item, atTop);
  }

  // ── widget factory: config names → components (same names as Noctalia) ───
  readonly property var widgetComponents: ({
    "launcher": launcherComponent,
    "workspaces": workspacesComponent,
    "clock": clockComponent,
    "network": networkComponent,
    "volume": volumeComponent,
    "battery": batteryComponent,
    "notifications": notificationsComponent,
    "control-center": controlCenterComponent,
    "session": sessionComponent,
  })

  Component { id: launcherComponent; LauncherWidget {} }
  Component { id: workspacesComponent; WorkspacesWidget {} }
  Component { id: clockComponent; ClockWidget {} }
  Component { id: networkComponent; NetworkWidget {} }
  Component { id: volumeComponent; VolumeWidget {} }
  Component { id: batteryComponent; BatteryWidget {} }
  Component { id: notificationsComponent; NotificationsWidget {} }
  Component { id: controlCenterComponent; ControlCenterWidget {} }
  Component { id: sessionComponent; SessionWidget {} }

  component Section: Row {
    id: section
    property var names: []
    spacing: barWindow.widgetSpacing

    Repeater {
      model: section.names
      delegate: Loader {
        required property var modelData
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
        sourceComponent: barWindow.widgetComponents[modelData] ?? null
        active: sourceComponent !== null
        visible: item ? item.shown : false // `shown`, not `visible`: a hidden Loader would hide its item
        onLoaded: item.bar = barWindow
        Component.onCompleted: {
          if (!barWindow.widgetComponents[modelData])
            console.warn("noctalia-win: widget desconhecido na barra:", modelData);
        }
      }
    }
  }

  // ── drawing ───────────────────────────────────────────────────────────────
  Item {
    id: barArea
    x: barWindow.sidePad
    y: barWindow.atTop ? barWindow.marginEdge : barWindow.marginOpposite
    width: barWindow.width - 2 * barWindow.sidePad
    height: barWindow.thickness
    visible: !barWindow.hiddenForFullscreen

    // Soft drop shadow, direction "down", alpha 0.55 (Noctalia's [shell.shadow] defaults).
    // Built from stacked translucent rounded rects so it needs no extra Qt modules.
    Repeater {
      model: barWindow.shadowEnabled ? 10 : 0
      delegate: Rectangle {
        required property int index
        readonly property real spread: index + 1
        x: -spread
        y: -spread * 0.6 + spread * 0.5
        width: barArea.width + 2 * spread
        height: barArea.height + 2 * spread * 0.8
        radius: barWindow.radius + spread
        color: Qt.rgba(0, 0, 0, 0.55 * barWindow.backgroundOpacity * 0.075 * Math.pow(1 - index / 10, 1.6))
      }
    }

    Rectangle {
      id: background
      anchors.fill: parent
      radius: barWindow.radius
      color: Style.alpha(Theme.colors.surface, barWindow.backgroundOpacity)

      Behavior on color {
        ColorAnimation { duration: Style.animNormal }
      }
    }

    // Clicking empty bar space: right click opens the control center (Noctalia's dead-zone default).
    MouseArea {
      anchors.fill: parent
      acceptedButtons: Qt.RightButton
      onClicked: Shell.quickSettings()
    }

    Section {
      id: startSection
      names: barWindow.cfg.start ?? []
      anchors.left: parent.left
      anchors.leftMargin: barWindow.padding - Style.barCapsulePadding
      anchors.verticalCenter: parent.verticalCenter
    }

    Section {
      id: centerSection
      names: barWindow.cfg.center ?? []
      anchors.centerIn: parent
    }

    Section {
      id: endSection
      names: barWindow.cfg.end ?? []
      anchors.right: parent.right
      anchors.rightMargin: barWindow.padding - Style.barCapsulePadding
      anchors.verticalCenter: parent.verticalCenter
    }
  }
}
