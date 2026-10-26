import QtQuick
import Noctalia
import "../Commons"

Text {
  font.family: Style.fontFamily
  font.pixelSize: Style.fontSizeBody
  font.weight: Font.Medium
  color: Theme.colors.onSurface
  verticalAlignment: Text.AlignVCenter
  elide: Text.ElideRight

  Behavior on color {
    ColorAnimation { duration: Style.animFast; easing.type: Easing.OutCubic }
  }
}
