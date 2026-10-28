import QtQuick
import "../Commons"

Item {
  width:22 * Style.uiScale
  height:width
  Image {
    objectName:"vitoria-wolf-image"
    anchors.fill:parent
    source:Qt.resolvedUrl("../../assets/vitoria-wolf.png")
    sourceSize:Qt.size(Math.ceil(width*2),Math.ceil(height*2))
    fillMode:Image.PreserveAspectFit
    asynchronous:true
    smooth:true
    mipmap:true
  }
}
