import QtQuick
import Noctalia
import "../Commons"

// Thin progress bar 0..1.
Rectangle {
  id: root
  property real value: 0
  property color accent: Theme.colors.primary
  implicitHeight: 8
  radius: height / 2
  color: Theme.colors.surfaceContainerHighest

  Rectangle {
    width: Math.max(parent.height, parent.width * Math.max(0, Math.min(1, root.value)))
    height: parent.height
    radius: parent.radius
    color: root.accent
    Behavior on width {
      NumberAnimation { duration: Style.animNormal; easing.type: Easing.OutCubic }
    }
  }
}
