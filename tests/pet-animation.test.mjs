import test from 'node:test';
import assert from 'node:assert/strict';
import {createAnimation,triggerAnimation,selectAnimation,animationFrame} from '../web/pet-animation.js';
test('food and play have finite, restartable animations',()=>{
 const a=createAnimation();triggerAnimation(a,'eat',100);
 assert.equal(selectAnimation(a,false,false,100),'eat');
 assert.equal(animationFrame(a,'eat',320),1);
 assert.equal(animationFrame(a,'eat',980),0);
 assert.equal(selectAnimation(a,false,false,3620),'idle');
 triggerAnimation(a,'play',4000);assert.equal(selectAnimation(a,false,false,4100),'play');
 triggerAnimation(a,'eat',4200);assert.equal(animationFrame(a,'eat',4200),0);
});
test('sleep takes precedence and active games remain animated',()=>{
 const a=createAnimation();triggerAnimation(a,'eat',0);
 assert.equal(selectAnimation(a,true,true,20),'sleep');
 assert.equal(selectAnimation(a,false,true,100000),'play');
 assert.equal(selectAnimation(a,false,false,100000),'idle');
 assert.equal(animationFrame(a,'sleep',2800),0);
 assert.throws(()=>triggerAnimation(a,'wifi',0));
});
