import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

// One setting: label (+ description) on the left, the control on the right.
Item {
  id: row
  property string label: ""
  property string description: ""
  default property alias control: holder.data
  property real controlWidth: holder.childrenRect.width

  width: parent ? parent.width : 400
  implicitHeight: Math.max(48, texts.implicitHeight + 16)

  Column {
    id: texts
    anchors.left: parent.left
    anchors.right: holder.left
    anchors.rightMargin: Style.spaceLg
    anchors.verticalCenter: parent.verticalCenter
    spacing: 2
    NText {
      width: parent.width
      text: row.label
      font.weight: Font.DemiBold
    }
    NText {
      width: parent.width
      visible: row.description.length > 0
      text: row.description
      wrapMode: Text.Wrap
      elide: Text.ElideNone
      font.pixelSize: Style.fontSizeCaption
      font.weight: Font.Normal
      color: Theme.colors.onSurfaceVariant
    }
  }

  Item {
    id: holder
    anchors.right: parent.right
    anchors.verticalCenter: parent.verticalCenter
    width: childrenRect.width
    height: childrenRect.height
  }
}
