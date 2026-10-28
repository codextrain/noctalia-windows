import QtQuick
import Noctalia
import "../Commons"

Row {
  id:root
  objectName:"pulse-dots"
  property bool running:true
  property real phase:0
  spacing:5
  NumberAnimation on phase {
    from:0;to:Math.PI*2;duration:950;loops:Animation.Infinite
    running:root.running && root.visible && Style.animationsEnabled
  }
  Repeater {
    model:3
    delegate:Rectangle {
      required property int index
      width:7;height:7;radius:4
      color:Theme.colors.primary
      opacity:Style.animationsEnabled ? 0.3+0.7*(Math.sin(root.phase-index*0.8)+1)/2 : 1
      scale:Style.animationsEnabled ? 0.8+0.25*(Math.sin(root.phase-index*0.8)+1)/2 : 1
    }
  }
}
