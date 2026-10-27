import QtQuick
import Noctalia
import "../../Commons"
import "../../Widgets"
import "../../Commons/RunningApps.js" as Apps
Item {
  id:root
  property var bar:null
  readonly property var screen:bar ? bar.screen : null
  readonly property var entries:Apps.current(Style.niriState,Dock.items,screen ? screen.name : "",Date.now(),screen ? screen.virtualX : undefined,screen ? screen.virtualY : undefined)
  readonly property bool shown:entries.length>0
  readonly property real capsuleHeight:bar ? bar.capsuleHeight : 26
  implicitWidth:row.implicitWidth
  implicitHeight:capsuleHeight
  Component.onCompleted:RunningAppsFeed.acquire()
  Component.onDestruction:RunningAppsFeed.release()
  Row {
    id:row
    anchors.centerIn:parent
    spacing:Style.spaceXs
    Repeater {
      model:root.entries
      delegate:Rectangle {
        id:button
        required property var modelData
        width:Math.max(28*Style.uiScale,root.capsuleHeight)
        height:root.capsuleHeight
        radius:Style.radiusMd
        color:mouse.containsMouse ? Theme.colors.hover : "transparent"
        Image {
          id:icon
          objectName:"running-app-icon"
          property string windowId:button.modelData.id
          anchors.centerIn:parent
          width:Math.min(20*Style.uiScale,parent.height-4)
          height:width
          source:button.modelData.icon
          sourceSize:Qt.size(width*2,height*2)
          fillMode:Image.PreserveAspectFit
          asynchronous:true
          mipmap:true
        }
        NGlyph {anchors.centerIn:parent;visible:icon.status!==Image.Ready;icon:"app-window";size:18*Style.uiScale;color:Theme.colors.onSurface}
        Rectangle {visible:button.modelData.active;anchors.bottom:parent.bottom;anchors.horizontalCenter:parent.horizontalCenter;width:12*Style.uiScale;height:2;radius:1;color:Theme.colors.primary}
        MouseArea {
          id:mouse
          anchors.fill:parent
          hoverEnabled:true
          cursorShape:Qt.PointingHandCursor
          onClicked:{if(root.bar)root.bar.hideTooltip();Dock.focusWindow(button.modelData.id);}
          onContainsMouseChanged:{if(!root.bar)return;if(containsMouse)root.bar.showTooltip(button,button.modelData.title);else root.bar.hideTooltip();}
        }
      }
    }
  }
}
