pragma Singleton
import QtQuick
import Noctalia
import "GpuCatState.js" as CatState
QtObject {
  id:root
  property var state: ({available:false,percent:0,running:false,interval:650})
  property double lastStamp:0
  property bool reading:false
  function refresh() {
    const url=Shell.env("NOCTALIA_GPU_STATE");
    if(!url.length){state=({available:false,percent:0,running:false,interval:650});return;}
    if(reading)return;
    reading=true;
    const request=new XMLHttpRequest();
    request.onreadystatechange=function(){
      if(request.readyState!==XMLHttpRequest.DONE)return;
      root.reading=false;
      const value=CatState.read(request.responseText,Date.now());
      if(!value){if(root.lastStamp<=0 || Date.now()-root.lastStamp>5000)root.state=({available:false,percent:0,running:false,interval:650});return;}
      if(value.stamp!==root.lastStamp){root.state=CatState.next(root.state,value.percent);root.lastStamp=value.stamp;}
    };
    try{request.open("GET",url);request.send();}catch(e){reading=false;state=({available:false,percent:0,running:false,interval:650});}
  }
  property Timer polling:Timer {interval:500;running:true;repeat:true;triggeredOnStart:true;onTriggered:root.refresh()}
}
