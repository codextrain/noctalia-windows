const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const logic=vm.createContext({});vm.runInContext(fs.readFileSync(path.join(__dirname,'../qml/Commons/GpuCatState.js'),'utf8'),logic);
test('valid idle is distinct from unavailable and stale samples',()=>{
 const data={running:true,available:true,updated_at_ms:1000,gpu_percent:0};
 assert.equal(logic.read(JSON.stringify(data),1100).percent,0);
 for(const value of [{...data,available:false},{...data,gpu_percent:null},{...data,gpu_percent:101},{...data,updated_at_ms:0}])assert.equal(logic.read(JSON.stringify(value),6001),null);
});
test('cat sleeps at idle and runs faster at high load',()=>{
 const idle=logic.next({available:false,running:false},0);assert.equal(idle.running,false);
 const low=logic.next({available:false,running:false},20);const high=logic.next({available:false,running:false},90);
 assert.equal(low.running,true);assert.equal(high.running,true);assert.ok(high.interval<low.interval);
});
test('hysteresis prevents alternating around the wake threshold',()=>{
 let state=logic.next({available:false,running:false},13);assert.equal(state.running,true);
 state=logic.next(state,10);assert.equal(state.running,true);
 for(let i=0;i<10;i++)state=logic.next(state,0);
 assert.equal(state.running,false);
});
