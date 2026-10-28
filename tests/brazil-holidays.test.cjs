const test=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const logic=vm.createContext({});vm.runInContext(fs.readFileSync(path.join(__dirname,'../qml/Commons/BrazilHolidays.js'),'utf8'),logic);
test('2026 October and November holidays match the Brazilian calendar',()=>{
 assert.deepEqual(Array.from(logic.monthDates(2026,9,false),h=>h.key),['2026-10-12']);
 assert.deepEqual(Array.from(logic.monthDates(2026,10,false),h=>h.key),['2026-11-02','2026-11-15','2026-11-20']);
 assert.equal(logic.monthDates(2026,9,true).find(h=>h.day===28).kind,'optional');
});
test('movable dates change with Easter across years without UTC conversion',()=>{
 assert.equal(logic.key(logic.easter(2025)),'2025-04-20');
 assert.equal(logic.key(logic.easter(2026)),'2026-04-05');
 assert.equal(logic.key(logic.easter(2027)),'2027-03-28');
 const dates=logic.yearDates(2026);
 assert.equal(dates.find(h=>h.name==='Paixão de Cristo').key,'2026-04-03');
 assert.equal(dates.find(h=>h.name==='Corpus Christi').key,'2026-06-04');
 assert.equal(dates.find(h=>h.name==='Carnaval (terça-feira)').key,'2026-02-17');
});
test('optional dates remain distinct and are hidden when disabled',()=>{
 const dates=logic.yearDates(2026);
 assert.equal(dates.length,19);assert.equal(dates.filter(h=>h.kind==='optional').length,9);
 assert.equal(logic.monthDates(2026,1,false).length,0);
 assert.equal(logic.monthDates(2026,1,true).length,3);
 assert.equal(dates.find(h=>h.key==='2026-12-24').detail.includes('13h'),true);
 assert.equal(logic.yearDates(2023).some(h=>h.key==='2023-11-20'),false);
});
