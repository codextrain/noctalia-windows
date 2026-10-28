const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const logic=vm.createContext({});vm.runInContext(fs.readFileSync(path.join(__dirname,'../qml/Commons/LocationPolicy.js'),'utf8'),logic);
test('manual city atomically overrides automatic IP location and preserves other preferences',()=>{
 const before={auto_locate:true,address:'Joinville',country:'BR'};
 const after=JSON.parse(JSON.stringify(logic.manual(before,' São Bento do Sul, SC ')));
 assert.deepEqual(after,{auto_locate:false,address:'São Bento do Sul, SC',country:'BR'});
 assert.equal(before.auto_locate,true);assert.equal(before.address,'Joinville');
});
test('empty city never overwrites an existing selection',()=>{
 for(const value of ['', '  ',null,undefined])assert.equal(logic.manual({address:'São Bento do Sul'},value),null);
});
