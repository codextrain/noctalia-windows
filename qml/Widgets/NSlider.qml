import QtQuick
import Noctalia
import "../Commons"

// Horizontal slider 0..1 in Noctalia's style. `moved(value)` fires while dragging;
// bind `value` from the model and update it from `moved`.
Item {
  id: root

  property real value: 0
  property real wheelStep: 0.05
  property bool enabled: true
  property color accent: Theme.colors.primary
  readonly property bool pressed: drag.pressed
  property real displayValue: pressed ? dragValue : value
  property real dragValue: 0
  signal moved(real value)

  implicitHeight: 22
  implicitWidth: 200
  opacity: enabled ? 1 : 0.45

  Rectangle {
    id: track
    anchors.verticalCenter: parent.verticalCenter
    width: parent.width
    height: 6
    radius: 3
    color: Theme.colors.surfaceContainerHighest

    Rectangle {
      width: Math.max(height, parent.width * Math.max(0, Math.min(1, root.displayValue)))
      height: parent.height
      radius: parent.radius
      color: root.accent
    }
  }

  Rectangle {
    id: handle
    width: 16
    height: 16
    radius: 8
    anchors.verticalCenter: parent.verticalCenter
    x: (root.width - width) * Math.max(0, Math.min(1, root.displayValue))
    color: root.accent
    border.width: 3
    border.color: Theme.colors.surface
    scale: drag.containsMouse || drag.pressed ? 1.15 : 1
    Behavior on scale {
      NumberAnimation { duration: Style.animFast }
    }
  }

  MouseArea {
    id: drag
    anchors.fill: parent
    anchors.margins: -6
    enabled: root.enabled
    hoverEnabled: true
    cursorShape: Qt.PointingHandCursor
    preventStealing: true

    function valueAt(mx) {
      return Math.max(0, Math.min(1, (mx - 6 - handle.width / 2) / (root.width - handle.width)));
    }
    onPressed: event => {
      root.dragValue = valueAt(event.x);
      root.moved(root.dragValue);
    }
    onPositionChanged: event => {
      if (!pressed)
        return;
      root.dragValue = valueAt(event.x);
      root.moved(root.dragValue);
    }
    onWheel: event => {
      if (!event.angleDelta.y) return;
      const step = event.angleDelta.y > 0 ? root.wheelStep : -root.wheelStep;
      root.moved(Math.max(0, Math.min(1, root.value + step)));
    }
  }
}
