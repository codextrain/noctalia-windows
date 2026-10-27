const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const logic=vm.createContext({});vm.runInContext(fs.readFileSync(path.join(__dirname,'../qml/Commons/TooltipPosition.js'),'utf8'),logic);
test('tooltips at both bar ends stay within the primary screen',()=>{
 for(const px of [0,20,1900,1920]){
  const p=logic.position(px,30,250,40,true,12,0,0,1920,1080,8);
  assert.ok(p.x>=8&&p.x+250<=1912);assert.equal(p.y,42);
 }
});
test('negative monitor origins and different vertical offsets are preserved',()=>{
 const p=logic.position(-1915,220,300,40,true,12,-1920,188,1920,1080,8);
 assert.equal(p.x,-1912);assert.equal(p.y,232);
});
test('bottom and top overflow flip or clamp inside the same monitor',()=>{
 const bottom=logic.position(500,1070,250,40,true,12,0,0,1920,1080,8);
 assert.equal(bottom.y,1018);
 const top=logic.position(500,10,250,40,false,12,0,0,1920,1080,8);
 assert.equal(top.y,22);
});
