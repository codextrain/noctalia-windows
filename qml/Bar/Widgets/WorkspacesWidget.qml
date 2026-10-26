import QtQuick
import Noctalia
import ".."
import "../../Commons"

// Windows virtual desktops drawn exactly like Noctalia's default workspace pills:
//   pill height = 16px, inactive = 1x, focused = 2.2x wide, 4px gap,
//   focused = primary, occupied = secondary, empty = secondary at 55% alpha,
//   hovered pill = hover color. Scroll up/down = previous/next desktop.
Item {
  id: root

  property var bar: null
  property bool shown: true
  readonly property real capsuleHeight: bar ? bar.capsuleHeight : 26
  readonly property real pillHeight: Style.baseGlyphSize
  readonly property real gap: Style.spaceXs

  implicitHeight: capsuleHeight
  implicitWidth: row.implicitWidth + 2 * Style.barCapsulePadding

  Rectangle {
    anchors.fill: parent
    radius: height / 2
    visible: root.bar && root.bar.capsule
    color: Theme.colors.surfaceContainer
  }

  // Wheel anywhere on the widget switches desktops.
  MouseArea {
    anchors.fill: parent
    acceptedButtons: Qt.NoButton
    onWheel: event => {
      if (event.angleDelta.y > 0)
        Workspaces.previous();
      else if (event.angleDelta.y < 0)
        Workspaces.next();
    }
  }

  Row {
    id: row
    anchors.centerIn: parent
    spacing: root.gap

    Repeater {
      model: Workspaces.model

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
          // Generous hit target: the full capsule height and half the gap on each side.
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
