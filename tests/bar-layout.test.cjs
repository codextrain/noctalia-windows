const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const logic = vm.createContext({});
vm.runInContext(fs.readFileSync(path.join(__dirname, '../qml/Commons/BarLayout.js'), 'utf8'), logic);
const plain = value => JSON.parse(JSON.stringify(value));
test('null sections recover defaults, explicit empty sections stay empty', () => {
  assert.deepEqual(plain(logic.read({start:null, center:null, end:null})), plain(logic.defaults));
  assert.deepEqual(plain(logic.read({start:[], center:[], end:[]})), {start:[], center:[], end:[]});
});
test('move from palette into an empty center and remove into palette', () => {
  const empty={start:[], center:[], end:[]};
  const next=plain(logic.move(empty,'clock','center',0));
  assert.deepEqual(next,{start:[], center:['clock'], end:[]});
  assert.deepEqual(plain(logic.move(next,'clock','',0)),empty);
  assert.deepEqual(empty,{start:[], center:[], end:[]});
});
test('cross-section moves preserve the other widgets without duplicates', () => {
  assert.deepEqual(plain(logic.move({start:['launcher','clock'],center:['volume'],end:['network']},'clock','center',0)),
    {start:['launcher'],center:['clock','volume'],end:['network']});
});
test('same-section reorder adjusts insertion after removing the source', () => {
  const cfg={start:['launcher','clock','volume'],center:[],end:[]};
  assert.deepEqual(plain(logic.move(cfg,'launcher','start',3)).start,['clock','volume','launcher']);
  assert.deepEqual(plain(logic.move(cfg,'volume','start',0)).start,['volume','launcher','clock']);
  assert.deepEqual(plain(logic.move(cfg,'clock','start',2)).start,cfg.start);
});
test('compatibility JSON survives a disk roundtrip and takes precedence', () => {
  const original={start:null,center:null,end:null};
  const next=logic.move(original,'weather','center',0);
  const persisted=JSON.parse(JSON.stringify({...original, widget_layout_json:JSON.stringify(next)}));
  assert.deepEqual(plain(logic.read(persisted)),plain(next));
  assert.deepEqual(plain(logic.read({...original,widget_layout_json:'broken'})),plain(logic.defaults));
});
test('reject unknown destinations and remove duplicates from old config', () => {
  assert.equal(logic.move({},'clock','invalid',0),null);
  const recovered=plain(logic.read({start:['clock'],center:['clock'],end:[]}));
  assert.deepEqual(recovered,{start:['clock'],center:[],end:[]});
});
