import QtQuick
import QtQuick.Window
import "../Commons"

Item {
  width:22 * Style.uiScale
  height:width
  Image {
    objectName:"vitoria-wolf-image"
    anchors.fill:parent
    source:Qt.resolvedUrl("../../assets/vitoria-wolf.png")
    sourceSize:Qt.size(Math.ceil(width*Math.max(1,Screen.devicePixelRatio)*3),Math.ceil(height*Math.max(1,Screen.devicePixelRatio)*3))
    fillMode:Image.PreserveAspectFit
    asynchronous:true
    smooth:true
    mipmap:true
  }
}
