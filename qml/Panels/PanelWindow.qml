import QtQuick
import QtQuick.Window
import Noctalia
import "../Commons"

// Base for Noctalia panels (launcher, control center, clipboard, wallpaper...).
//
//  - one floating window per panel, shown when Panels.current === panelName
//  - surface + outline_variant border + radius_xl + soft shadow (Noctalia's panel chrome)
//  - opens with a quick fade/scale, closes on Esc or when you click outside the shell
//
//   PanelWindow { panelName: "launcher"; panelWidth: 760; panelHeight: 620; ...content... }
Window {
  id: panel

  property string panelName: ""
  property int panelWidth: 600
  property int panelHeight: 400
  // "center" = middle of the screen; "bar" = hanging under the bar (attached panels)
  property string placement: "center"
  property real barAnchorX: -1 // for placement "bar": horizontal center, -1 = screen center
  default property alias content: surface.data
  readonly property alias surfaceItem: surface
  readonly property bool wanted: Panels.current === panelName
  property bool shown: false // true while open (and during the closing animation)

  signal opened
  signal closed

  readonly property int shadowPad: 28
  readonly property var targetScreen: Qt.application.screens[Panels.screenIndex] ?? Qt.application.screens[0]
  readonly property int barReserve: (Config.bar.margin_edge ?? 10) + (Config.bar.thickness ?? 34)
                                  + (Config.bar.margin_opposite_edge ?? 6)
  readonly property bool barAtTop: Config.bar.position !== "bottom"

  flags: Qt.FramelessWindowHint | Qt.Tool | Qt.WindowStaysOnTopHint | Qt.NoDropShadowWindowHint
  color: "transparent"
  title: "Noctalia " + panelName
  objectName: panelName
  visible: false
  screen: targetScreen

  width: panelWidth + 2 * shadowPad
  height: panelHeight + 2 * shadowPad
  x: {
    const s = targetScreen;
    const cx = placement === "bar" && barAnchorX >= 0 ? barAnchorX : s.virtualX + s.width / 2;
    const left = Math.round(cx - width / 2);
    return Math.max(s.virtualX - shadowPad + 8, Math.min(left, s.virtualX + s.width - width + shadowPad - 8));
  }
  y: {
    const s = targetScreen;
    if (placement === "bar")
      return barAtTop ? s.virtualY + barReserve - shadowPad + 6
                      : s.virtualY + s.height - barReserve - height + shadowPad - 6;
    return Math.round(s.virtualY + (s.height - height) / 2);
  }

  onWantedChanged: {
    if (wanted) {
      closeAnim.stop();
      shown = true;
      visible = true;
      openAnim.restart();
      Panels.focusWindow(panel);
      opened();
    } else if (shown) {
      openAnim.stop();
      closeAnim.restart();
    }
  }

  // Clicking anywhere outside the shell closes the panel (like Noctalia).
  Connections {
    target: Qt.application
    function onStateChanged() {
      if (panel.wanted && Qt.application.state !== Qt.ApplicationActive)
        Panels.close();
    }
  }

  Shortcut {
    sequence: "Escape"
    enabled: panel.wanted
    onActivated: Panels.close()
  }

  Item {
    id: chrome
    x: panel.shadowPad
    y: panel.shadowPad
    width: panel.panelWidth
    height: panel.panelHeight
    transformOrigin: placement === "bar" ? (panel.barAtTop ? Item.Top : Item.Bottom) : Item.Center

    // shadow
    Repeater {
      model: 12
      delegate: Rectangle {
        required property int index
        readonly property real spread: index * 1.6 + 1
        x: -spread
        y: -spread + spread * 0.35
        width: chrome.width + 2 * spread
        height: chrome.height + 2 * spread
        radius: Style.radiusXl + spread
        color: Qt.rgba(0, 0, 0, 0.55 * 0.06 * Math.pow(1 - index / 12, 1.8))
      }
    }

    Rectangle {
      id: surface
      anchors.fill: parent
      radius: Style.radiusXl
      color: Theme.colors.surface
      border.width: Style.borderWidth
      border.color: Theme.colors.outlineVariant
      clip: false
    }

    ParallelAnimation {
      id: openAnim
      NumberAnimation { target: chrome; property: "opacity"; from: 0; to: 1; duration: Style.animNormal; easing.type: Easing.OutCubic }
      NumberAnimation { target: chrome; property: "scale"; from: 0.96; to: 1; duration: Style.animNormal; easing.type: Easing.OutCubic }
    }
    SequentialAnimation {
      id: closeAnim
      ParallelAnimation {
        NumberAnimation { target: chrome; property: "opacity"; to: 0; duration: Style.animFast; easing.type: Easing.InCubic }
        NumberAnimation { target: chrome; property: "scale"; to: 0.97; duration: Style.animFast; easing.type: Easing.InCubic }
      }
      ScriptAction {
        script: {
          panel.visible = false;
          panel.shown = false;
          panel.closed();
        }
      }
    }
  }
}
