import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"

// Home tab shortcut tile (Noctalia control_center.shortcuts): glyph + label, `active` = primary.
Rectangle {
  id: root

  property string icon: ""
  property string label: ""
  property bool active: false
  readonly property bool hovered: mouse.containsMouse
  readonly property color fg: active ? Theme.colors.onPrimary : hovered ? Theme.colors.onHover : Theme.colors.onSurface
  signal clicked
  signal rightClicked

  radius: Style.radiusXl
  color: active ? Theme.colors.primary : hovered ? Theme.colors.hover : Theme.colors.surfaceContainer
  border.width: active || hovered ? 0 : Style.borderWidth
  border.color: Theme.colors.outlineVariant

  Behavior on color {
    ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
  }

  Column {
    anchors.centerIn: parent
    spacing: Style.spaceXs + 2
    NGlyph {
      anchors.horizontalCenter: parent.horizontalCenter
      icon: root.icon
      size: 22
      color: root.fg
    }
    NText {
      anchors.horizontalCenter: parent.horizontalCenter
      text: root.label
      font.pixelSize: Style.fontSizeCaption
      font.weight: Font.DemiBold
      color: root.fg
    }
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    hoverEnabled: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    cursorShape: Qt.PointingHandCursor
    onClicked: event => event.button === Qt.RightButton ? root.rightClicked() : root.clicked()
  }
}
