import QtQuick
import Noctalia
import "../Commons"

// Base for every bar widget. Reproduces Noctalia's bar behavior:
//  - a pill-shaped hover highlight (on_surface at 10% alpha, 100ms ease-out),
//  - hit area extended by the capsule padding,
//  - optional capsule background when bar.capsule = true,
//  - a tooltip after a short hover.
Item {
  id: root

  property var bar: null                 // the BarWindow (set by the bar)
  property string tooltipText: ""
  property bool interactive: true
  property bool shown: true             // set false to remove the widget from the bar
  readonly property bool hovered: mouse.containsMouse
  readonly property color foreground: Theme.colors.onSurface
  default property alias content: contentRow.data
  property alias spacing: contentRow.spacing

  signal clicked(var mouse)
  signal rightClicked(var mouse)
  signal middleClicked(var mouse)
  signal wheelUp
  signal wheelDown

  readonly property real capsuleHeight: bar ? bar.capsuleHeight : 26
  readonly property real hPad: Style.barCapsulePadding

  implicitHeight: capsuleHeight
  implicitWidth: Math.max(capsuleHeight, contentRow.implicitWidth + 2 * hPad)

  // Capsule background (bar.capsule = true in config)
  Rectangle {
    anchors.fill: parent
    radius: height / 2
    visible: root.bar && root.bar.capsule
    color: Theme.colors.surfaceContainer
  }

  // Hover highlight
  Rectangle {
    id: hoverBox
    anchors.fill: parent
    radius: height / 2
    color: Style.alpha(Theme.colors.onSurface, 0.10)
    opacity: root.interactive && root.hovered ? 1 : 0
    Behavior on opacity {
      NumberAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
    }
  }

  Row {
    id: contentRow
    anchors.centerIn: parent
    spacing: Style.spaceXs + 2
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    hoverEnabled: true
    enabled: root.interactive
    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
    cursorShape: Qt.PointingHandCursor

    onClicked: event => {
      if (root.bar)
        root.bar.hideTooltip();
      if (event.button === Qt.RightButton)
        root.rightClicked(event);
      else if (event.button === Qt.MiddleButton)
        root.middleClicked(event);
      else
        root.clicked(event);
    }
    onWheel: event => {
      if (event.angleDelta.y > 0)
        root.wheelUp();
      else if (event.angleDelta.y < 0)
        root.wheelDown();
    }
    onContainsMouseChanged: {
      if (!root.bar)
        return;
      if (containsMouse && root.tooltipText.length > 0)
        root.bar.showTooltip(root, root.tooltipText);
      else
        root.bar.hideTooltip();
    }
  }

  // Keep a visible tooltip in sync when the text changes (e.g. volume while scrolling).
  onTooltipTextChanged: {
    if (bar && hovered && tooltipText.length > 0)
      bar.updateTooltip(root, tooltipText);
  }
}
