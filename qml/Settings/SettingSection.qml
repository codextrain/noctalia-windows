import QtQuick
import Noctalia
import "../Commons"
import "../Widgets"

// A titled card holding SettingRows.
NCard {
  id: section
  property string title: ""
  default property alias rows: col.data
  width: parent ? parent.width : 600
  height: col.implicitHeight + 2 * padding + titleText.height + Style.spaceSm

  NSectionTitle {
    id: titleText
    text: section.title
  }
  Column {
    id: col
    anchors.top: titleText.bottom
    anchors.topMargin: Style.spaceSm
    width: parent.width
    spacing: 0
  }
}
