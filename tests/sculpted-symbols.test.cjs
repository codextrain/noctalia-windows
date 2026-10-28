const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const logic=vm.createContext({});vm.runInContext(fs.readFileSync(path.join(__dirname,'../qml/Commons/SculptedSymbols.js'),'utf8'),logic);
test('all control center categories have a sculpted symbol',()=>{
 for(const icon of ['home','disc','volume','device-desktop','activity-heartbeat','cpu-usage','battery-charging-2','wifi','bluetooth','weather-cloud-sun','calendar-event'])assert.notEqual(logic.forGlyph(icon),'');
});
test('weather and audio retain their dynamic state families',()=>{
 assert.equal(logic.forGlyph('weather-cloud-lightning'),'weather');assert.equal(logic.forGlyph('volume-mute'),'volume');
 assert.equal(logic.forGlyph('temperature'),'temperature');assert.equal(logic.forGlyph('wind'),'wind');assert.equal(logic.forGlyph('droplet'),'droplet');
 assert.equal(logic.forGlyph('unknown-symbol'),'');
});
