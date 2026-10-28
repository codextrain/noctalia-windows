import QtQuick
import QtQuick.Window
import Noctalia
import "../../../qml/Panels/Wallpaper" as UI
Window {
 id:win;visible:true;x:100;y:100;width:640;height:300;flags:Qt.Tool|Qt.FramelessWindowHint|Qt.WindowStaysOnTopHint
 property var scroll:null
 property int releases:0
 property real maxProgress:0
 Timer {interval:50;repeat:true;running:true;onTriggered:if(win.scroll)win.maxProgress=Math.max(win.maxProgress,win.scroll.progress);}
 UI.OnlineWallpaperGrid {id:grid;anchors.fill:parent;results:[];loading:false}
 Connections {target:win.scroll;function onScrollFinished(){win.releases++;}}
 Timer {interval:300;running:true;onTriggered:{
  let items=[];for(let i=0;i<80;i++)items.push({key:"t"+i,thumb:"",source:"wallhaven"});grid.results=items;grid.forceLayout();
  Panels.focusWindow(win);
  scroll=grid.children.find(c=>c.objectName==="wallpaper-scrollbar");const p=scroll.mapToGlobal(scroll.width/2,scroll.thumbHeight/2);
  Config.setValue("probe",{x:p.x,y:p.y,end:p.y+190,ready:true});done.start();
 }}
 Timer {id:quitDelay;interval:200;onTriggered:Shell.quit()}
 Timer {id:done;interval:4000;onTriggered:{
  const result={dragWorks:win.maxProgress>0.6&&scroll.progress<0.1,maxProgress:win.maxProgress,position:grid.contentY,releases:win.releases,interactiveRestored:grid.interactive};
  scroll.beginDrag();scroll.visible=false;result.cancelRestored=grid.interactive&&!scroll.dragging;
  grid.interactive=false;scroll.visible=true;scroll.beginDrag();scroll.endDrag(false);result.preservesDisabled=!grid.interactive;
  console.log("SCROLL_TEST_RESULT:"+JSON.stringify(result));Config.setValue("probe",result);quitDelay.start();
 }}
}