import test from 'node:test';
import assert from 'node:assert/strict';
import {DAY,createPet,tick,act} from '../web/app.js';
test('daily sleep cap and real month',()=>{
 const p=createPet(0);act(p,'sleep');tick(p,DAY);assert.equal(p.growth,1);assert.equal(p.stage,0);
 tick(p,30*DAY);assert.equal(p.stage,1);tick(p,180*DAY);assert.equal(p.stage,4);
});
test('8h sleep produces 0.75 growth day',()=>{
 const p=createPet(0);act(p,'sleep');tick(p,28800);act(p,'wake');tick(p,DAY);assert.equal(p.growth,.75);
});
test('no sleep, backwards clock, and deterministic tick frequency',()=>{
 const p=createPet(0);tick(p,60*DAY);assert.equal(p.stage,1);const old=structuredClone(p);tick(p,0);assert.deepEqual(p,old);
 const a=createPet(0),b=structuredClone(a);act(a,'sleep');act(b,'sleep');
 for(let i=1;i<=172800;i++)tick(a,i);tick(b,172800);
 assert.ok(Math.abs(a.growth-b.growth)<1e-8);assert.equal(a.hunger,b.hunger);
});
test('unknown commands never pretend to connect a device',()=>{
 const p=createPet(0);assert.throws(()=>act(p,'wifi'));assert.throws(()=>act(p,'friends'));
});
test('saturation and poop bounds',()=>{
 const p=createPet(0);p.hunger=100;assert.throws(()=>act(p,'feed'));
 for(let i=0;i<100;i++){p.hunger=0;act(p,'feed');}assert.equal(p.poop,3);act(p,'clean');assert.equal(p.poop,0);
});
