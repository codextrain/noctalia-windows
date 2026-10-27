import QtQuick
import Noctalia
import "../Commons"

// A selectable row: [glyph] title / subtitle [trailing glyph]. `checked` = primary fill.
Rectangle {
  id: root

  property string icon: ""
  property string title: ""
  property string subtitle: ""
  property string trailingIcon: ""
  property bool checked: false
  readonly property bool hovered: mouse.containsMouse
  readonly property color fg: checked ? Theme.colors.onPrimary : hovered ? Theme.colors.onHover : Theme.colors.onSurface
  signal clicked

  implicitHeight: subtitle.length > 0 ? 48 : 38
  radius: Style.radiusMd
  color: checked ? Theme.colors.primary : hovered ? Theme.colors.hover : "transparent"

  Behavior on color {
    ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
  }

  NGlyph {
    id: lead
    visible: root.icon.length > 0
    icon: root.icon
    color: root.fg
    anchors.left: parent.left
    anchors.leftMargin: Style.spaceMd
    anchors.verticalCenter: parent.verticalCenter
  }

  Column {
    anchors.left: lead.visible ? lead.right : parent.left
    anchors.leftMargin: Style.spaceMd
    anchors.right: trail.visible ? trail.left : parent.right
    anchors.rightMargin: Style.spaceSm
    anchors.verticalCenter: parent.verticalCenter

    NText {
      width: parent.width
      text: root.title
      font.weight: Font.DemiBold
      color: root.fg
    }
    NText {
      width: parent.width
      visible: root.subtitle.length > 0
      text: root.subtitle
      font.pixelSize: Style.fontSizeCaption
      font.weight: Font.Normal
      color: root.checked ? Style.alpha(Theme.colors.onPrimary, 0.8)
           : root.hovered ? Style.alpha(Theme.colors.onHover, 0.8) : Theme.colors.onSurfaceVariant
    }
  }

  NGlyph {
    id: trail
    visible: root.trailingIcon.length > 0
    icon: root.trailingIcon
    color: root.fg
    anchors.right: parent.right
    anchors.rightMargin: Style.spaceMd
    anchors.verticalCenter: parent.verticalCenter
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    hoverEnabled: true
    cursorShape: Qt.PointingHandCursor
    onClicked: root.clicked()
  }
}
