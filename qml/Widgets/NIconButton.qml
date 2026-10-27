import QtQuick
import Noctalia
import "../Commons"

// Ghost icon button (Noctalia ButtonVariant::Ghost): transparent, hover = hover color.
Rectangle {
  id: root

  property string icon: ""
  property real glyphSize: Style.fontSizeBody
  property string tooltip: ""
  property bool active: false // "pressed"/toggled look
  readonly property bool hovered: mouse.containsMouse
  signal clicked

  implicitWidth: 32
  implicitHeight: 32
  radius: Style.radiusMd
  color: hovered ? Theme.colors.hover : active ? Style.alpha(Theme.colors.primary, 0.18) : "transparent"

  Behavior on color {
    ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
  }

  NGlyph {
    anchors.centerIn: parent
    icon: root.icon
    size: root.glyphSize
    color: root.hovered ? Theme.colors.onHover : active ? Theme.colors.primary : Theme.colors.onSurface
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    hoverEnabled: true
    cursorShape: Qt.PointingHandCursor
    onClicked: root.clicked()
  }
}
