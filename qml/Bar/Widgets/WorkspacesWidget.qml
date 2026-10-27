import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"
import "../../Commons/WindowIndicators.js" as WindowIndicators

// Two modes:
//  - NiriWin running (NOCTALIA_NIRIWIN_STATUS set and fresh): one numbered pill per window on
//    this bar's monitor, the active one highlighted.
//  - otherwise: Windows virtual desktops drawn like Noctalia's default workspace pills
//    (16px high, focused = 2.2x wide, occupied = secondary, empty = secondary at 55%).
//    Scroll up/down = previous/next desktop.
Item {
  id: root
  property var bar: null
  property var entries: []
  property bool loading: false
  readonly property bool niriMode: entries.length > 0
  readonly property bool shown: niriMode || Workspaces.model.length > 0
  readonly property real capsuleHeight: bar ? bar.capsuleHeight : 26
  readonly property real pillHeight: Style.baseGlyphSize
  readonly property real gap: Style.spaceXs
  readonly property string monitorName: bar && bar.screen ? bar.screen.name : ""
  readonly property string statusUrl: Shell.env("NOCTALIA_NIRIWIN_STATUS")
  implicitHeight: capsuleHeight
  implicitWidth: (niriMode ? niriRow.implicitWidth : desktopRow.implicitWidth) + 2 * Style.barCapsulePadding

  function refresh() {
    if (loading) return;
    if (!statusUrl.length || !monitorName.length) { entries = []; return; }
    loading = true;
    const request = new XMLHttpRequest();
    request.onreadystatechange = function() {
      if (request.readyState !== XMLHttpRequest.DONE) return;
      root.loading = false;
      const screen = root.bar ? root.bar.screen : null;
      root.entries = WindowIndicators.read(request.responseText, root.monitorName, Date.now(), screen ? screen.virtualX : undefined, screen ? screen.virtualY : undefined);
    };
    try { request.open("GET", statusUrl); request.send(); }
    catch (e) { loading = false; entries = []; }
  }
  Component.onCompleted: refresh()
  // Only poll when NiriWin's status file was configured.
  Timer { interval: 250; running: root.statusUrl.length > 0; repeat: true; onTriggered: root.refresh() }

  Rectangle {
    anchors.fill: parent
    radius: height / 2
    visible: root.bar && root.bar.capsule
    color: Theme.colors.surfaceContainer
  }

  // ── NiriWin: numbered windows ─────────────────────────────────────────────
  Row {
    id: niriRow
    visible: root.niriMode
    anchors.centerIn: parent
    spacing: root.gap
    Repeater {
      model: root.niriMode ? root.entries : []
      delegate: Rectangle {
        id: dot
        required property var modelData
        width: Math.max(modelData.active ? 32 : 22, numberText.implicitWidth + 10)
        height: 22
        radius: height / 2
        color: modelData.active ? Theme.colors.primary : Style.alpha(Theme.colors.secondary, 0.65)
        NText {
          id: numberText
          anchors.centerIn: parent
          text: String(dot.modelData.number)
          font.pixelSize: 12
          font.weight: Font.Bold
          color: dot.modelData.active ? Theme.colors.onPrimary : Theme.colors.onSurface
        }
        Behavior on color { ColorAnimation { duration: Style.animFast } }
        MouseArea {
          anchors.centerIn: parent
          width: parent.width + root.gap
          height: root.capsuleHeight
          acceptedButtons: Qt.NoButton
          hoverEnabled: true
          onContainsMouseChanged: {
            if (!root.bar) return;
            if (containsMouse) root.bar.showTooltip(dot, "Janela " + dot.modelData.number + (dot.modelData.active ? " — ativa" : ""));
            else root.bar.hideTooltip();
          }
        }
      }
    }
  }

  // ── Windows virtual desktops ──────────────────────────────────────────────
  MouseArea {
    anchors.fill: parent
    enabled: !root.niriMode
    acceptedButtons: Qt.NoButton
    onWheel: event => {
      if (event.angleDelta.y > 0)
        Workspaces.previous();
      else if (event.angleDelta.y < 0)
        Workspaces.next();
    }
  }

  Row {
    id: desktopRow
    visible: !root.niriMode
    anchors.centerIn: parent
    spacing: root.gap

    Repeater {
      model: root.niriMode ? [] : Workspaces.model

      delegate: Rectangle {
        id: pill
        required property var modelData

        readonly property bool focused: modelData.focused
        readonly property bool occupied: modelData.occupied
        readonly property bool hovered: pillMouse.containsMouse

        width: Math.round(root.pillHeight * (focused ? 2.2 : 1.0))
        height: root.pillHeight
        radius: height / 2
        anchors.verticalCenter: parent.verticalCenter
        color: hovered ? Theme.colors.hover
             : focused ? Theme.colors.primary
             : occupied ? Theme.colors.secondary
             : Style.alpha(Theme.colors.secondary, 0.55)

        Behavior on width {
          NumberAnimation { duration: Style.animNormal; easing.type: Easing.OutCubic }
        }
        Behavior on color {
          ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
        }

        MouseArea {
          id: pillMouse
          anchors.centerIn: parent
          width: parent.width + root.gap
          height: root.capsuleHeight
          hoverEnabled: true
          cursorShape: Qt.PointingHandCursor
          onClicked: {
            if (root.bar)
              root.bar.hideTooltip();
            Workspaces.activate(pill.modelData.index);
          }
          onContainsMouseChanged: {
            if (!root.bar)
              return;
            if (containsMouse)
              root.bar.showTooltip(pill, "Área de trabalho " + pill.modelData.name);
            else
              root.bar.hideTooltip();
          }
        }
      }
    }
  }
}
