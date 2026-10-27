import QtQuick
import Noctalia
import "../Commons"

// Single-line text field in Noctalia's style. Exposes the inner TextInput as `input`.
Rectangle {
  id: root

  property alias text: field.text
  property alias input: field
  property string placeholder: ""
  property string icon: "search"
  // Key presses from the field (set event.accepted = true to swallow them).
  signal keyPressed(var event)

  implicitHeight: 38
  radius: Style.radiusMd
  color: Theme.colors.surface
  border.width: Style.borderWidth
  border.color: field.activeFocus ? Theme.colors.primary : Theme.colors.outlineVariant

  Behavior on border.color {
    ColorAnimation { duration: Style.animFast }
  }

  NGlyph {
    id: leadIcon
    visible: root.icon.length > 0
    icon: root.icon
    color: Theme.colors.onSurfaceVariant
    anchors.left: parent.left
    anchors.leftMargin: Style.spaceMd
    anchors.verticalCenter: parent.verticalCenter
  }

  TextInput {
    id: field
    anchors.left: leadIcon.visible ? leadIcon.right : parent.left
    anchors.leftMargin: leadIcon.visible ? Style.spaceSm : Style.spaceMd
    anchors.right: parent.right
    anchors.rightMargin: Style.spaceMd
    anchors.verticalCenter: parent.verticalCenter
    font.family: Style.fontFamily
    font.pixelSize: Style.fontSizeBody
    color: Theme.colors.onSurface
    selectionColor: Theme.colors.primary
    selectedTextColor: Theme.colors.onPrimary
    clip: true
    selectByMouse: true
    Keys.onPressed: event => root.keyPressed(event)

    cursorDelegate: Rectangle {
      width: 2
      color: Theme.colors.primary
      visible: field.cursorVisible
    }

    Text {
      anchors.fill: parent
      verticalAlignment: Text.AlignVCenter
      text: root.placeholder
      visible: field.text.length === 0
      font: field.font
      color: Theme.colors.onSurfaceVariant
      elide: Text.ElideRight
    }
  }
}
