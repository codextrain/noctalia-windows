import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"
import "../../Commons/WindowIndicators.js" as WindowIndicators

// Numbered windows managed by NiriWin, scoped to this bar's monitor.
Item {
  id: root
  property var bar: null
  property var entries: []
  property bool loading: false
  readonly property bool shown: entries.length > 0
  readonly property real capsuleHeight: bar ? bar.capsuleHeight : 26
  readonly property string monitorName: bar && bar.screen ? bar.screen.name : ""
  readonly property string statusUrl: Shell.env("NOCTALIA_NIRIWIN_STATUS")
  implicitHeight: capsuleHeight
  implicitWidth: row.implicitWidth + 2 * Style.barCapsulePadding

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
  Timer { interval: 250; running: true; repeat: true; onTriggered: root.refresh() }
  Rectangle {
    anchors.fill: parent
    radius: height / 2
    visible: root.bar && root.bar.capsule
    color: Theme.colors.surfaceContainer
  }
  Row {
    id: row
    anchors.centerIn: parent
    spacing: Style.spaceXs
    Repeater {
      model: root.entries
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
          width: parent.width + Style.spaceXs
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
}
