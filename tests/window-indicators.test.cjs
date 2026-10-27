const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const logic=vm.createContext({});
vm.runInContext(fs.readFileSync(path.join(__dirname,'../qml/Commons/WindowIndicators.js'),'utf8'),logic);
function status(){return {running:true,updated_at_ms:1000,windows:Array.from({length:4},(_,i)=>({id:String(i+1),monitor:'\\\\.\\DISPLAY1',number:i+1,active:i===2})).concat([{id:'9',monitor:'\\\\.\\DISPLAY2',number:1,active:true}])};}
test('four windows produce four numbered indicators and highlight the active window',()=>{
 const entries=JSON.parse(JSON.stringify(logic.read(JSON.stringify(status()),'DISPLAY1',1100)));
 assert.deepEqual(entries.map(w=>w.number),[1,2,3,4]);assert.deepEqual(entries.filter(w=>w.active).map(w=>w.number),[3]);
});

test('workspace indicators combine pairs and focus their last active member',()=>{
 const data=status();data.windows=data.windows.slice(0,4);
 data.windows.forEach((w,i)=>w.workspace=Math.floor(i/2)+1);
 const entries=JSON.parse(JSON.stringify(logic.read(JSON.stringify(data),'DISPLAY1',1100)));
 assert.deepEqual(entries,[{id:'1',number:1,active:false,count:2},{id:'3',number:2,active:true,count:2}]);
 data.windows.splice(2,1);
 const after=JSON.parse(JSON.stringify(logic.read(JSON.stringify(data),'DISPLAY1',1100)));
 assert.equal(after[1].count,1);assert.equal(after[1].id,'4');
});
test('monitor filtering accepts Qt and Win32 names',()=>{assert.equal(logic.read(JSON.stringify(status()),'\\\\.\\DISPLAY2',1100).length,1);assert.equal(logic.read(JSON.stringify(status()),'DISPLAY3',1100).length,0);});

test('returning to a spotlight group focuses its selected member and retains both windows',()=>{
 const data=status();data.windows=data.windows.slice(0,4);data.windows.forEach((w,i)=>{w.workspace=Math.floor(i/2)+1;w.active=false;});
 data.windows[1].spotlight=true;
 const entries=JSON.parse(JSON.stringify(logic.read(JSON.stringify(data),'DISPLAY1',1100)));
 assert.equal(entries.length,2);assert.equal(entries[0].count,2);assert.equal(entries[0].id,'2');assert.equal(entries[0].spotlight,true);
});

test('an empty second monitor stays in Niri mode with exactly one numbered empty workspace',()=>{
 const data=status();data.windows=data.windows.slice(0,4);
 const view=JSON.parse(JSON.stringify(logic.view(JSON.stringify(data),'DISPLAY2',1100,-1920,188)));
 assert.deepEqual(view,{niri:true,entries:[{id:'',number:1,active:true,count:0,empty:true}]});
 assert.equal(logic.view(JSON.stringify(data),'DISPLAY1',1100).entries.length,4);
 data.windows=[];assert.equal(logic.view(JSON.stringify(data),'DISPLAY1',1100).entries[0].number,1);
 data.paused=true;assert.equal(logic.view(JSON.stringify(data),'DISPLAY2',1100).niri,false);
 assert.equal(logic.view('invalid','DISPLAY2',1100).niri,false);
 assert.equal(logic.view(JSON.stringify(status()),'DISPLAY2',5000).niri,false);
});
test('friendly Qt monitor names can match by monitor origin',()=>{
 const data=status(); data.windows[0].monitor_x=0; data.windows[0].monitor_y=0;
 assert.equal(logic.read(JSON.stringify(data),'Hailstorm',1100,0,0).length,1);
 assert.equal(logic.read(JSON.stringify(data),'Other',1100,-1920,188).length,0);
});
test('stopped, paused, stale and malformed state never leave fake dots',()=>{
 for(const data of [{...status(),running:false},{...status(),paused:true},{...status(),updated_at_ms:0}])assert.equal(logic.read(JSON.stringify(data),'DISPLAY1',4001).length,0);
 assert.equal(logic.read('invalid','DISPLAY1',1100).length,0);
});
