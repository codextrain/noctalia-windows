import QtQuick
import QtQuick.Window
import Noctalia
import ".."
import "../../Commons"

BarWidget {
  id:root
  property int frame:0
  readonly property bool running:GpuTelemetry.state.available && GpuTelemetry.state.running
  tooltipText:GpuTelemetry.state.available ? "GPU " + Math.round(GpuTelemetry.state.percent) + "% · " + (running ? "Correndo" : "Dormindo") : "GPU: aguardando uma leitura disponível"
  onClicked:Panels.toggle("control-center:system")
  Item {
    width:40*Style.uiScale;height:26*Style.uiScale
    anchors.verticalCenter:parent.verticalCenter
  Canvas {
    id:cat
    objectName:"gpu-cat-canvas"
    readonly property real rasterScale:Math.max(1,Screen.devicePixelRatio)*2
    width:40 * Style.uiScale*rasterScale;height:26 * Style.uiScale*rasterScale
    transformOrigin:Item.Center;scale:1/rasterScale;smooth:true
    anchors.centerIn:parent
    onPaint:{
      const c=getContext("2d");c.reset();c.clearRect(0,0,width,height);c.scale(width/40,height/26);
      const fur=c.createLinearGradient(8,5,25,24);fur.addColorStop(0,"#fff8e7");fur.addColorStop(0.45,"#dfc6a9");fur.addColorStop(1,"#997d6c");
      c.fillStyle="rgba(0,0,0,0.25)";c.beginPath();c.moveTo(5,24);c.bezierCurveTo(3,21,35,21,35,24);c.bezierCurveTo(35,27,3,27,5,24);c.fill();
      c.fillStyle=fur;c.strokeStyle="#b89c80";c.lineWidth=2.5;c.lineCap="round";c.lineJoin="round";
      if(root.running){
        const p=root.frame*Math.PI/4;const bob=Math.sin(p*2)*0.6;
        c.save();c.translate(0,bob);
        // Four paws, with opposite front/rear phases.
        for(let i=0;i<4;i++){
          const front=i<2;const phase=p+(i%2)*Math.PI+(front?0:Math.PI/2);
          const x=front?24:13;const swing=Math.sin(phase)*5;
          c.beginPath();c.moveTo(x,15);c.lineTo(x+swing*0.5,19);c.lineTo(x+swing,22-Math.max(0,Math.cos(phase))*3);c.stroke();
        }
        c.beginPath();c.moveTo(12,13);c.bezierCurveTo(6,13,7,5+Math.sin(p)*2,2,6);c.stroke();
        c.beginPath();c.moveTo(10,13);c.bezierCurveTo(10,7,24,7,27,12);c.bezierCurveTo(28,18,13,19,10,13);c.fill();
        c.beginPath();c.arc(29,9,5,0,Math.PI*2);c.fill();
        c.beginPath();c.moveTo(25,7);c.lineTo(24,1);c.lineTo(29,5);c.moveTo(29,5);c.lineTo(33,1);c.lineTo(34,9);c.closePath();c.fill();
        c.fillStyle="#403a3a";c.fillRect(30,7,1.5,1.5);c.restore();
      }else{
        const breath=Math.sin(root.frame*Math.PI/4)*0.4;
        c.beginPath();c.moveTo(7,19);c.bezierCurveTo(4,8-breath,23,7-breath,27,16);c.bezierCurveTo(29,23,11,24,7,19);c.fill();
        c.beginPath();c.arc(28,16,5,0,Math.PI*2);c.fill();
        c.beginPath();c.moveTo(24,14);c.lineTo(23,9);c.lineTo(28,12);c.lineTo(31,10);c.lineTo(33,16);c.closePath();c.fill();
        c.fillStyle="#403a3a";c.strokeStyle="#403a3a";c.lineWidth=1;
        c.beginPath();c.moveTo(27,16);c.lineTo(30,16);c.stroke();
        c.strokeStyle="#c3a88a";c.lineWidth=3;
        c.beginPath();c.moveTo(8,18);c.bezierCurveTo(0,13,2,24,15,23);c.stroke();
        c.fillStyle="#dfd5c8";c.font="bold 9px sans-serif";
        c.fillText(GpuTelemetry.state.available ? "z" : "?",34,9);
      }
    }
  }
  }
  Timer {interval:GpuTelemetry.state.interval;running:root.visible && Style.animationsEnabled;repeat:true;onTriggered:{root.frame=(root.frame+1)%8;cat.requestPaint();}}
  Connections {target:GpuTelemetry;function onStateChanged(){cat.requestPaint();}}
  Connections {target:Theme;function onChanged(){cat.requestPaint();}}
}
