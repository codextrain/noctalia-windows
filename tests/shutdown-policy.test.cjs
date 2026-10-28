const test=require('node:test');
const assert=require('node:assert/strict');
const p=require('../qml/Commons/ShutdownPolicy.js');
test('shutdown deadline validates minutes and caps at 24 hours',()=>{
 for(const n of [0,-1,1.5,NaN,Infinity,1441])assert.equal(p.deadline(n,1000),0);
 assert.equal(p.deadline(60,1000),3601000);assert.equal(p.deadline(1440,0),86400000);
});
test('countdown handles elapsed deadlines and formats hours',()=>{
 assert.equal(p.remaining(1000,1001),0);assert.equal(p.remaining(61000,1001),60);
 assert.equal(p.format(3661),'01:01:01');assert.equal(p.format(0),'00:00:00');
});
