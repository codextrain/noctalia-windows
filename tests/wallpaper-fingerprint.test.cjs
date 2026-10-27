const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const logic=vm.createContext({});vm.runInContext(fs.readFileSync(path.join(__dirname,'../qml/Commons/WallpaperFingerprint.js'),'utf8'),logic);
test('rewriting the same image path with equal length changes the signature',()=>{
 const a=Uint8Array.from({length:1024},(_,i)=>i%256), b=a.slice();b[500]^=255;
 assert.notEqual(logic.fingerprint(a),logic.fingerprint(b));assert.equal(logic.fingerprint(a),logic.fingerprint(a));
});
test('empty reads are ignored and a changed file must be stable twice',()=>{
 let state=logic.observe({candidate:'',applied:''},'image|A');assert.equal(state.changed,false);
 state=logic.observe(state,'image|B');assert.equal(state.changed,false);assert.equal(state.applied,'image|A');
 state=logic.observe(state,'');assert.equal(state.applied,'image|A');
 state=logic.observe(state,'image|B');assert.equal(state.changed,true);assert.equal(state.applied,'image|B');
 assert.equal(logic.observe(state,'image|B').changed,false);
});
test('rapid changes do not commit transient files',()=>{
 let state=logic.observe({candidate:'',applied:''},'A');state=logic.observe(state,'B');state=logic.observe(state,'C');assert.equal(state.changed,false);state=logic.observe(state,'C');assert.equal(state.changed,true);assert.equal(state.applied,'C');
});
