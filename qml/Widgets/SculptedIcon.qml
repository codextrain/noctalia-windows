import QtQuick
import "../Commons"

// Static miniature clay/metal objects. Paint only when resized or state changes.
Canvas {
  id: root
  property string symbol: "network"
  property bool disabled: false
  property string condition: ""
  width: 22 * Style.uiScale
  height: width
  onSymbolChanged: requestPaint()
  onDisabledChanged: requestPaint()
  onConditionChanged: requestPaint()
  onWidthChanged: requestPaint()
  onHeightChanged: requestPaint()
  onPaint: {
    const c=getContext("2d"); c.reset(); c.scale(width/64,height/64);
    c.lineCap="round"; c.lineJoin="round";
    function ball(x,y,rx,ry,light,base,dark) {
      c.save(); c.translate(x,y); c.scale(rx,ry);
      const g=c.createRadialGradient(-0.35,-0.45,0.02,0,0,1);
      g.addColorStop(0,light);g.addColorStop(0.4,base);g.addColorStop(1,dark);
      c.fillStyle=g;c.beginPath();c.arc(0,0,1,0,Math.PI*2);c.fill();c.restore();
    }
    function line(points,width,color) {
      c.strokeStyle=color;c.lineWidth=width;c.beginPath();
      points.forEach((p,i)=>i?c.lineTo(p[0],p[1]):c.moveTo(p[0],p[1]));c.stroke();
    }
    function plate(points,top,bottom) {
      const g=c.createLinearGradient(10,8,48,54);g.addColorStop(0,top);g.addColorStop(1,bottom);
      c.fillStyle=g;c.beginPath();points.forEach((p,i)=>i?c.lineTo(p[0],p[1]):c.moveTo(p[0],p[1]));c.closePath();c.fill();
    }
    ball(33,56,25,5,"#00000066","#00000033","#00000000");
    if(symbol==="network") {
      line([[32,43],[32,53]],6,"#8fabc5");ball(32,54,14,3,"#e5effb","#88a2bd","#334d68");
      ball(31,27,22,23,"#f1ffff",disabled?"#8994a8":"#5cc4f0",disabled?"#424d65":"#2350a0");
      c.strokeStyle="#d9f8ffbb";c.lineWidth=2;
      c.beginPath();c.arc(31,27,15,-1.3,1.3);c.stroke();
      c.beginPath();c.arc(31,27,15,1.85,4.45);c.stroke();
      line([[10,26],[51,26]],2,"#d8f9ffb0");
      c.beginPath();c.moveTo(14,15);c.quadraticCurveTo(31,23,48,15);c.moveTo(14,39);c.quadraticCurveTo(31,32,48,39);c.stroke();
      if(disabled)line([[12,48],[51,10]],5,"#ed8c9a");
    } else if(symbol==="volume") {
      plate([[10,25],[20,25],[35,13],[39,16],[39,51],[34,54],[20,42],[10,42]],"#d6d7ff","#5d58aa");
      plate([[10,25],[20,25],[34,15],[34,51],[20,41],[10,41]],"#c2c9ff","#8070c9");
      line([[13,28],[13,37]],2,"#edf0ff");
      if(disabled){line([[46,26],[56,39]],4,"#ff9caa");line([[56,26],[46,39]],4,"#ff9caa");}
      else {for(let i=0;i<2;i++){c.lineWidth=4;c.strokeStyle=i?"#75c9ef":"#b3ecff";c.beginPath();c.arc(36,33,12+i*10,-0.8,0.8);c.stroke();}}
    } else if(symbol==="brightness") {
      for(let i=0;i<8;i++){const a=i*Math.PI/4;line([[32+22*Math.cos(a),30+22*Math.sin(a)],[32+28*Math.cos(a),30+28*Math.sin(a)]],5,"#d68a3c");line([[31+22*Math.cos(a),29+22*Math.sin(a)],[31+27*Math.cos(a),29+27*Math.sin(a)]],3,"#ffe5a1");}
      ball(32,30,17,18,"#fffbd3","#ffc768","#c77c31");
    } else if(symbol==="power") {
      ball(32,32,26,26,"#8390ad","#3f4a66","#151e31");
      c.strokeStyle="#555278";c.lineWidth=8;c.beginPath();c.arc(32,32,15,-0.94,4.08);c.stroke();
      c.strokeStyle="#c8b5ff";c.lineWidth=5;c.beginPath();c.arc(31,30,15,-0.94,4.08);c.stroke();
      line([[31,10],[31,29]],6,"#baa0f1");line([[30,11],[30,26]],2,"#f3e6ff");
    } else if(symbol==="cat") {
      plate([[10,28],[9,5],[25,16],[39,16],[55,5],[54,29]],"#fff1d7","#9c7a66");
      plate([[13,10],[15,26],[25,19]],"#f4c9b4","#bd8a86");plate([[51,10],[49,26],[39,19]],"#f4c9b4","#bd8a86");
      ball(32,33,24,22,"#fff9e9","#e0cbb1","#9d8376");
      ball(23,32,3,4,"#769aa0","#233945","#14212c");ball(41,32,3,4,"#769aa0","#233945","#14212c");
      ball(29,42,8,5,"#fffaf0","#f3e6d2","#c5ab91");ball(37,42,7,5,"#fffaf0","#f3e6d2","#c5ab91");
      plate([[29,37],[36,37],[32,41]],"#e8a0a4","#a66577");line([[32,41],[32,45]],1.5,"#79675f");
      line([[8,37],[20,39]],1.5,"#8f7b73");line([[8,43],[20,42]],1.5,"#8f7b73");line([[44,39],[56,37]],1.5,"#8f7b73");line([[44,42],[56,43]],1.5,"#8f7b73");
    } else if(symbol==="search") {
      line([[39,40],[55,56]],10,"#526f9c");line([[38,39],[53,53]],5,"#b2cef4");
      ball(27,26,22,22,"#edfaff","#89aeda","#385c90");ball(27,26,16,16,"#e5faff","#5c91b7","#244665");
      line([[18,18],[23,13]],3,"#f2fcff");
    } else if(symbol==="chevron") {
      line([[12,39],[32,21],[52,39]],11,"#4f6485");line([[12,35],[32,17],[52,35]],7,"#dce9ff");
    } else if(symbol==="home") {
      plate([[12,28],[34,13],[53,28],[53,54],[12,54]],"#fff0d9","#b6a58e");
      plate([[6,29],[31,6],[58,29],[53,34],[31,16],[11,34]],"#d4c7fa","#7568ac");
      plate([[27,36],[39,36],[39,54],[27,54]],"#91c8dd","#416f98");
    } else if(symbol==="disc") {
      ball(32,31,26,26,"#f7fbff","#8cb6cd","#505a91");
      c.strokeStyle="#c9e4f0";c.lineWidth=2;c.beginPath();c.arc(32,31,18,0,Math.PI*2);c.stroke();
      ball(32,31,9,9,"#d9c8f3","#7d70ad","#303754");ball(32,31,3,3,"#30374c","#1a2235","#0c1223");
    } else if(symbol==="monitor"||symbol==="system") {
      line([[32,44],[32,53]],6,"#8b9fb7");ball(32,55,16,3,"#e2eef9","#899bb6","#44546d");
      plate([[4,8],[57,8],[60,12],[60,44],[56,47],[4,47]],"#e3e6ff","#67758f");
      plate([[8,12],[54,12],[54,39],[8,39]],"#6393a7","#19273f");
      if(symbol==="system")line([[11,28],[20,28],[25,18],[32,34],[37,25],[50,25]],3,"#b8f0cb");
      else line([[12,17],[29,17]],2,"#b2e1eb");
    } else if(symbol==="hardware") {
      for(let i=0;i<4;i++){const p=18+i*9;line([[p,4],[p,12]],4,"#b2c4ce");line([[p,50],[p,58]],4,"#829aaa");line([[4,p],[12,p]],4,"#b2c4ce");line([[50,p],[58,p]],4,"#829aaa");}
      plate([[11,11],[51,11],[54,15],[54,51],[50,54],[11,54]],"#b7cdb2","#41675d");
      plate([[20,20],[44,20],[44,44],[20,44]],"#e4e0c7","#8b9b93");line([[23,23],[39,23]],2,"#f8f8e3");
    } else if(symbol==="battery") {
      plate([[7,16],[52,16],[55,20],[55,49],[7,49]],"#ccdfdf","#5e7a85");
      line([[57,27],[57,38]],5,"#9bb1b8");
      plate([[12,22],[46,22],[46,42],[12,42]],"#b5e7ba","#4c9a78");
      plate([[30,23],[22,33],[29,33],[27,41],[37,29],[30,29]],"#fff0ad","#dac474");
    } else if(symbol==="bluetooth") {
      ball(32,31,23,28,"#cadfff","#6e9de0","#394982");
      line([[23,20],[42,39],[31,49],[31,13],[42,23],[23,42]],4,"#f5f4ff");
    } else if(symbol==="calendar") {
      plate([[10,11],[51,11],[55,16],[55,54],[10,54]],"#fbf5e8","#b9b1a2");
      plate([[10,11],[51,11],[55,16],[55,25],[10,25]],"#efb2ba","#b56d8c");
      line([[21,7],[21,17]],4,"#c4d1db");line([[43,7],[43,17]],4,"#c4d1db");
      for(let y=33;y<=43;y+=10)for(let x=21;x<=43;x+=11)ball(x,y,2.5,2.5,"#9194b2","#5a6583","#3f4c67");
    } else if(symbol==="temperature") {
      line([[30,12],[30,44]],14,"#d3e8ef");line([[30,14],[30,44]],7,"#d49aaa");
      ball(30,46,13,13,"#ffe9e5","#dc929e","#9d576f");line([[29,16],[29,38]],2,"#fff5ef");
    } else if(symbol==="droplet") {
      const g=c.createLinearGradient(13,12,46,55);g.addColorStop(0,"#d1f8ff");g.addColorStop(.5,"#77bee1");g.addColorStop(1,"#3662a2");
      c.fillStyle=g;c.beginPath();c.moveTo(32,5);c.bezierCurveTo(26,19,10,30,14,43);c.bezierCurveTo(19,65,57,55,49,33);c.bezierCurveTo(44,22,36,13,32,5);c.fill();
      line([[21,34],[19,41]],3,"#e3faff");
    } else if(symbol==="wind") {
      c.lineWidth=6;c.strokeStyle="#7196b4";c.beginPath();c.moveTo(7,22);c.lineTo(41,22);c.bezierCurveTo(58,22,54,6,44,10);c.moveTo(8,35);c.lineTo(48,35);c.moveTo(15,47);c.lineTo(34,47);c.bezierCurveTo(49,47,45,59,37,56);c.stroke();
      line([[7,20],[40,20]],2,"#e5f6ff");line([[8,33],[48,33]],2,"#d9efff");line([[15,45],[33,45]],2,"#d9efff");
    } else if(symbol==="weather") {
      const night=condition.indexOf("moon")>=0;
      if(condition!=="weather-cloud"&&condition.indexOf("haze")<0&&condition.indexOf("rain")<0&&condition.indexOf("lightning")<0&&condition.indexOf("snow")<0)
        ball(23,22,17,17,night?"#fcfcff":"#fff9cb",night?"#b8c9ed":"#ffd079",night?"#7486ac":"#d9903c");
      if(night)ball(31,16,14,14,"#27344e","#27344e","#182139");
      if(condition.indexOf("cloud")>=0||condition.indexOf("rain")>=0||condition.indexOf("snow")>=0||condition.indexOf("storm")>=0){
        ball(26,38,19,12,"#f5fbff","#d0e4f0","#7d9cb5");ball(36,30,14,15,"#fffaff","#d2e2ed","#8da7be");ball(47,39,12,10,"#f4faff","#cbdeeb","#779bb9");
      }
      if(condition.indexOf("rain")>=0)for(let i=0;i<3;i++)line([[22+i*12,51],[19+i*12,57]],3,"#78bbef");
      if(condition.indexOf("snow")>=0)for(let i=0;i<3;i++)ball(20+i*13,55,2,2,"#ffffff","#d4f3ff","#8db3d6");
      if(condition.indexOf("lightning")>=0)plate([[34,41],[26,52],[33,52],[30,61],[43,47],[35,47]],"#fff3b8","#e5bb69");
    }
  }
}
