"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const base=path.resolve(__dirname,"../../browser/3d");
const window={};
vm.runInNewContext(fs.readFileSync(path.join(base,"combat-surface-effects.js"),"utf8"),
 {window,Math,Number,Array},{filename:"combat-surface-effects.js"});
const effects=window.WPCombatSurfaceEffects;
const terrain=(x,z)=>.18*Math.sin(x*.07)+.23*Math.cos(z*.08);
const buildings=[[0,0,10,8],[-25,-12,10,9]];
function scene(){
 const triangles=[],colors=[];
 return {
  triangles,colors,
  tri(a,b,c,col){triangles.push([a,b,c]);colors.push(col);},
  quad(a,b,c,d,col){this.tri(a,b,c,col);this.tri(a,c,d,col);}
 };
}
let q={endpoint:{x:5,y:2.32,z:.5},blockedBy:"masonry",hit:null};
assert(effects.bullet(q,buildings,terrain),"masonry face must accept a verified bullet");
assert(!effects.bullet({...q,hit:{hp:100}},buildings,terrain),"infantry body hit must not leave false hole");
assert(!effects.bullet({...q,endpoint:{x:25,y:2.32,z:23}},buildings,terrain),"wall marks must be collider-aligned");
assert(!effects.bullet({...q,blockedBy:"none"},buildings,terrain),"misses cannot create holes");
const ground={endpoint:{x:13,y:terrain(13,9)+.06,z:9},blockedBy:"terrain",hit:null};
assert(effects.bullet(ground,buildings,terrain),"terrain intersection creates dirt pockmark");
assert(effects.explosion(12,9,"artillery",terrain));
assert(effects.explosion(-8,21,"frag",terrain));
let data=scene(),triangles=effects.render(data,"high",13,9,terrain);
assert(triangles===data.triangles.length&&triangles>60,"actual scar triangle count");
assert(data.triangles.some(t=>t.some(p=>Math.abs(p[0]-5.065)<.03)),
 "masonry impact must be offset near wall collision face");
assert(data.triangles.every(t=>t.every(v=>v.every(Number.isFinite))),"nonfinite debris vertices");
assert(data.colors.every(c=>c.every(Number.isFinite)),"nonfinite weather colors");
assert(effects.stats().debris===17,"physical fragments must be bounded");
assert.equal(effects.render(scene(),"medium",12,9,terrain),0);
assert.equal(effects.render(scene(),"low",12,9,terrain),0);
for(let i=0;i<70;i++){
 const x=4.999+(i%2)*.002;
 effects.bullet({...q,endpoint:{x,y:1+(i%6)*.4,z:Math.sin(i)*1.3}},buildings,terrain);
}
assert.equal(effects.stats().marks,42,"projectile scarring must use a strict ring budget");
for(let i=0;i<20;i++)effects.explosion(Math.sin(i)*18,Math.cos(i)*18,"he",terrain);
const count=effects.stats();
assert(count.blasts<=count.maxBlasts&&count.debris<=count.maxDebris,"bounded HE wreckage ring");
effects.update(.23);
data=scene();
triangles=effects.render(data,"high",0,0,terrain);
assert.equal(triangles,data.triangles.length);
assert(triangles<=count.maxTriangles,"High damage scene must obey GPU triangle max");
assert(data.triangles.every(t=>t.every(v=>v.every(Number.isFinite))),"dynamic GPU stream nonfinite");
for(let k=0;k<300;k++)effects.update(.20);
assert.equal(effects.stats().marks,0,"wall chips must eventually expire");
assert.equal(effects.stats().blasts,0,"scorch patches must eventually expire");
assert.equal(effects.stats().debris,0,"debris must fall and expire");
effects.reset();
assert.equal(effects.stats().bulletMarks,0,"reset must remove historical damage");
const game=fs.readFileSync(path.join(base,"game.js"),"utf8");
const html=fs.readFileSync(path.join(base,"index.html"),"utf8");
for(const name of ["WPCombatSurfaceEffects?.bullet(shot","WPCombatSurfaceEffects?.explosion(",
 "WPCombatSurfaceEffects?.update(dt)","WPCombatSurfaceEffects?.render(m,game.quality",
 "combatSurface:"])assert(game.includes(name),"simulation integration missing "+name);
assert(html.includes('combat-surface-effects.js'),"surface library not hosted");
console.log("PASS REAL COLLISION-GROUNDED COMBAT MARKS",JSON.stringify({
 registered:count.bulletMarks,explosions:count.explosions,marks:count.marks,
 debris:count.debris,triangles,cap:count.maxTriangles,rejected:count.rejected}));
