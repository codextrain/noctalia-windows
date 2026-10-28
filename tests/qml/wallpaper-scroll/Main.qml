import QtQuick
import QtQuick.Window
import Noctalia
import "../../../qml/Panels/Wallpaper" as WallpaperUI
Window {
 visible:true;width:640;height:300;x:100;y:100;flags:Qt.Tool|Qt.FramelessWindowHint
 property real savedPhase:0
 property var dots:null
 property int step:0
 property real savedY:0
 property int savedIndex:0
 property var savedDelegate:null
 function items(n,prefix){let out=[];for(let i=0;i<n;i++)out.push({key:prefix+i,thumb:"",full:"",target:"",source:"wallhaven",resolution:"1920x1080",fileSize:"",name:"Test "+i});return out;}
 WallpaperUI.OnlineWallpaperGrid {id:grid;anchors.fill:parent;results:[];loading:false}
 Timer {interval:250;running:true;repeat:true;onTriggered:{
  if(step===0){grid.results=items(48,"a");grid.forceLayout();grid.contentY=600;savedY=grid.contentY;savedIndex=grid.indexAt(20,savedY+20);savedDelegate=grid.itemAtIndex(savedIndex);}
  if(step===1){grid.results=items(72,"a");Config.setValue("probe.append",grid.count===72&&Math.abs(grid.contentY-savedY)<1);Config.setValue("probe.sameDelegate",grid.itemAtIndex(savedIndex)===savedDelegate);}
  if(step===2){grid.contentY=1100;savedY=grid.contentY;grid.results=items(96,"a");Config.setValue("probe.latestPosition",Math.abs(grid.contentY-savedY)<1);}
  if(step===3){grid.revision++;grid.results=items(24,"b");Config.setValue("probe.newSearch",grid.count===24&&Math.abs(grid.contentY)<1);}
  if(step===4){grid.contentY=200;grid.revision++;grid.results=items(24,"b");Config.setValue("probe.sameQueryRefresh",Math.abs(grid.contentY)<1);}
  if(step===5){const scroll=grid.children.find(c=>c.objectName==="wallpaper-scrollbar");scroll.setProgress(0.5);Config.setValue("probe.scrollMiddle",Math.abs(grid.contentY-grid.originY-scroll.scrollRange*0.5)<1);scroll.setProgress(1);Config.setValue("probe.scrollEnd",grid.atYEnd);grid.loading=true;const indicator=grid.children.find(c=>c.objectName==="wallpaper-loading-indicator");dots=indicator.children.find(c=>c.objectName==="pulse-dots");savedPhase=dots.phase;Config.setValue("probe.indicatorVisible",indicator.visible);}
  if(step===6){Config.setValue("probe.pulses",dots.phase!==savedPhase);grid.loading=false;savedPhase=dots.phase;}
  if(step===7){Config.setValue("probe.stopsWhenHidden",dots.phase===savedPhase);grid.active=false;Config.setValue("probe.inactive",grid.count===0);Shell.quit();}
  step++;
 }}
 Timer {interval:7000;running:true;onTriggered:Shell.quit()}
}
