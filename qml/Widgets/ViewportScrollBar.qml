import QtQuick
import Noctalia
import "../Commons"

Item {
  id:root
  objectName:"wallpaper-scrollbar"
  property var target:null
  readonly property real scrollRange:target?Math.max(0,target.contentHeight-target.height):0
  readonly property real thumbHeight:target?Math.min(height,Math.max(28,height*target.height/Math.max(1,target.contentHeight))):height
  readonly property real progress:target&&scrollRange>0?Math.max(0,Math.min(1,(target.contentY-target.originY)/scrollRange)):0
  property real grabOffset:0
  property bool dragging:false
  property var dragTarget:null
  property bool savedInteractive:true
  signal scrollFinished()
  width:14;visible:scrollRange>0;z:100
  Accessible.role:Accessible.ScrollBar
  Accessible.name:"Rolagem dos wallpapers"
  function beginDrag() {
    if(!target)return;
    dragTarget=target;savedInteractive=target.interactive;dragging=true;
    target.cancelFlick();target.interactive=false;
  }
  function endDrag(notify) {
    if(!dragging)return;
    dragging=false;
    if(dragTarget)dragTarget.interactive=savedInteractive;
    dragTarget=null;
    if(notify)scrollFinished();
  }
  onVisibleChanged:if(!visible)endDrag(false)
  onTargetChanged:endDrag(false)
  Component.onDestruction:endDrag(false)
  function setProgress(value) {
    if(!target)return;
    target.cancelFlick();
    target.contentY=target.originY+Math.max(0,Math.min(1,value))*scrollRange;
  }
  Rectangle {anchors.fill:parent;radius:7;color:Style.alpha(Theme.colors.onSurface,0.07)}
  Rectangle {
    id:thumb
    anchors.horizontalCenter:parent.horizontalCenter
    width:mouse.containsMouse||mouse.pressed?10:7
    height:root.thumbHeight;y:root.progress*(root.height-height)
    radius:5;color:mouse.pressed?Theme.colors.primary:Style.alpha(Theme.colors.onSurface,mouse.containsMouse?0.65:0.4)
  }
  MouseArea {
    id:mouse;anchors.fill:parent;hoverEnabled:true
    acceptedButtons:Qt.LeftButton
    preventStealing:true
    cursorShape:Qt.PointingHandCursor
    onPressed:event=>{
      root.beginDrag();
      root.grabOffset=event.y>=thumb.y&&event.y<=thumb.y+thumb.height?event.y-thumb.y:thumb.height/2;
      root.setProgress((event.y-root.grabOffset)/Math.max(1,root.height-thumb.height));
    }
    onPositionChanged:event=>{if(root.dragging)root.setProgress((event.y-root.grabOffset)/Math.max(1,root.height-thumb.height));}
    onReleased:root.endDrag(true)
    onCanceled:root.endDrag(false)
    onWheel:event=>{if(root.target){root.setProgress(root.progress-event.angleDelta.y/120*80/Math.max(1,root.scrollRange));event.accepted=true;}}
  }
}
