const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const logic=vm.createContext({});vm.runInContext(fs.readFileSync(path.join(__dirname,'../qml/Commons/RunningApps.js'),'utf8'),logic);
function state(){return {running:true,updated_at_ms:1000,windows:[
 {id:'10',monitor:'\\\\.\\DISPLAY1',monitor_x:0,monitor_y:0,workspace:1,number:1,active:true},
 {id:'20',monitor:'\\\\.\\DISPLAY1',monitor_x:0,monitor_y:0,workspace:1,number:2,active:false},
 {id:'30',monitor:'\\\\.\\DISPLAY1',workspace:2,number:3,active:false},
 {id:'40',monitor:'\\\\.\\DISPLAY2',workspace:1,number:1,active:true}]};}
const apps=[{name:'Chrome',icon:'chrome',windows:[{handle:'10',title:'Chrome atual'},{handle:'30',title:'Chrome outra area'}]},
 {name:'ChatGPT',icon:'gpt',windows:[{handle:'20',title:'ChatGPT'}]},{name:'Outro monitor',icon:'other',windows:[{handle:'40',title:'Outro monitor'}]},
 {name:'Fixado',icon:'pinned',windows:[]}];
function read(s=state(),monitor='DISPLAY1',now=1100,x,y){return JSON.parse(JSON.stringify(logic.current(s,apps,monitor,now,x,y)));}
test('shows only icons belonging to the active group on this monitor',()=>{
 assert.deepEqual(read().map(w=>[w.id,w.icon]),[['10','chrome'],['20','gpt']]);
 const s=state();s.windows[0].active=false;s.windows[2].active=true;assert.deepEqual(read(s).map(w=>w.id),['30']);
});
test('monitor origins match friendly Qt names and empty monitors show no pinned apps',()=>{
 assert.equal(read(state(),'Hailstorm',1100,0,0).length,2);assert.equal(read(state(),'DISPLAY3').length,0);
 assert.deepEqual(read(state(),'DISPLAY2').map(w=>w.id),['40']);
});
test('a spotlight retains both app icons for the same workspace',()=>{const s=state();s.windows[0].spotlight=true;assert.equal(read(s).length,2);});
test('stopped paused and stale Niri state never leaks unrelated apps',()=>{
 for(const s of [{...state(),paused:true},{...state(),running:false}])assert.deepEqual(read(s),[]);
 assert.deepEqual(read(state(),'DISPLAY1',6000),[]);
});
