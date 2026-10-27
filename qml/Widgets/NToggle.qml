import QtQuick
import Noctalia
import "../Commons"

// On/off switch.
Rectangle {
  id: root
  property bool checked: false
  property bool enabled: true
  signal toggled(bool checked)

  implicitWidth: 44
  implicitHeight: 24
  radius: height / 2
  opacity: enabled ? 1 : 0.45
  color: checked ? Theme.colors.primary : Theme.colors.surfaceContainerHighest
  border.width: checked ? 0 : Style.borderWidth
  border.color: Theme.colors.outline

  Behavior on color {
    ColorAnimation { duration: Style.animFast }
  }

  Rectangle {
    width: parent.height - 6
    height: width
    radius: width / 2
    anchors.verticalCenter: parent.verticalCenter
    x: root.checked ? parent.width - width - 3 : 3
    color: root.checked ? Theme.colors.onPrimary : Theme.colors.onSurfaceVariant
    Behavior on x {
      NumberAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
    }
  }

  MouseArea {
    anchors.fill: parent
    enabled: root.enabled
    cursorShape: Qt.PointingHandCursor
    onClicked: root.toggled(!root.checked)
  }
}
