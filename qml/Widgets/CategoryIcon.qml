import QtQuick
import QtQuick.Window
import "../Commons"

// Theme-colored vector marks, supersampled for crisp small-size rendering.
Item {
 id:root
 property string category:"home"
 property color ink:Theme.colors.primary
 width:24;height:24
 onCategoryChanged:requestPaint()
 onInkChanged:requestPaint()
 function requestPaint(){drawing.requestPaint();}
 Canvas {
  id:drawing
  readonly property real rasterScale:Math.max(1,Screen.devicePixelRatio)*2
  width:root.width*rasterScale;height:root.height*rasterScale
  transformOrigin:Item.TopLeft;scale:1/rasterScale;smooth:true
 onPaint:{
  const c=getContext("2d");c.reset();c.scale(width/24,height/24);
  c.lineCap="round";c.lineJoin="round";c.strokeStyle=ink;c.lineWidth=1.7;
  function line(p){c.beginPath();p.forEach((v,i)=>i?c.lineTo(v[0],v[1]):c.moveTo(v[0],v[1]));c.stroke();}
  function box(x,y,w,h){c.strokeRect(x,y,w,h);}
  function circle(x,y,r){c.beginPath();c.arc(x,y,r,0,Math.PI*2);c.stroke();}
  if(category==="home") {line([[3,11],[12,3],[21,11]]);line([[5,10],[5,21],[10,21],[10,15],[14,15],[14,21],[19,21],[19,10]]);}
  else if(category==="media") {circle(12,12,9);circle(12,12,2);c.beginPath();c.arc(12,12,6,-1.5,-.25);c.stroke();c.beginPath();c.arc(12,12,6,1.65,2.9);c.stroke();}
  else if(category==="audio") {line([[3,9],[7,9],[12,5],[12,19],[7,15],[3,15],[3,9]]);c.beginPath();c.arc(12,12,5,-.8,.8);c.stroke();c.beginPath();c.arc(12,12,9,-.8,.8);c.stroke();}
  else if(category==="monitor") {box(3,4,18,13);line([[12,17],[12,21]]);line([[8,21],[16,21]]);}
  else if(category==="system") {line([[2,12],[6,12],[9,5],[13,19],[16,10],[18,12],[22,12]]);}
  else if(category==="hardware") {box(6,6,12,12);box(9,9,6,6);for(let n=8;n<=16;n+=4){line([[n,3],[n,6]]);line([[n,18],[n,21]]);line([[3,n],[6,n]]);line([[18,n],[21,n]]);}}
  else if(category==="power") {box(2,6,18,12);line([[22,10],[22,14]]);line([[12,8],[8,13],[13,13],[10,16]]);}
  else if(category==="network") {for(let r=5;r<=13;r+=4){c.beginPath();c.arc(12,20,r,-2.35,-.79);c.stroke();}circle(12,20,.8);}
  else if(category==="bluetooth") {line([[8,7],[17,16],[12,21],[12,3],[17,8],[8,17]]);}
  else if(category==="weather") {circle(7,7,3);line([[7,1],[7,2]]);line([[1,7],[2,7]]);line([[2.5,2.5],[3.5,3.5]]);c.beginPath();c.moveTo(6,20);c.bezierCurveTo(0,20,1,12,7,12);c.bezierCurveTo(8,5,18,7,18,13);c.bezierCurveTo(24,12,24,20,18,20);c.closePath();c.stroke();}
  else if(category==="calendar") {box(3,5,18,16);line([[3,10],[21,10]]);line([[7,3],[7,7]]);line([[17,3],[17,7]]);for(let x=7;x<=17;x+=5)for(let y=14;y<=18;y+=4){c.fillStyle=ink;c.fillRect(x-.7,y-.7,1.4,1.4);}}
 }
}
}
