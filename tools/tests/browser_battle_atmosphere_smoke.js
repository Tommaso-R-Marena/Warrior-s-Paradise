/* Standalone deterministic visual-effect limits and cleanup. */
"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const code=fs.readFileSync(path.resolve(__dirname,"../../browser/3d/battle-atmosphere.js"),"utf8");
const win={};vm.runInNewContext(code,{window:win,Math,Number});
const fx=win.WPBattleAtmosphere;
assert(fx&&fx.stats().active===0);
assert.equal(fx.blast(Infinity,0),false);
let cones=0;
const geom={tri(){},cone(x,y,z,r,h){assert(Number.isFinite(x+y+z+r+h));cones++;}};
fx.blast(2,4,"artillery");fx.blast(10,-2,"frag");
assert.equal(fx.stats().active,2);
const low=fx.render(geom,"low",0,0);
assert.equal(cones,0,"Low quality must not draw smoke");
assert.equal(low.scars,2);assert(low.triangles<=8);
const medium=fx.render(geom,"medium",0,0);
assert(medium.plumes>0&&medium.triangles<=32,"Medium smoke budget exceeded");
const high=fx.render(geom,"high",0,0);
assert(high.plumes>medium.plumes&&high.triangles<=64,"High smoke budget exceeded");
for(let i=0;i<20;i++)fx.blast(i,1,"frag");
assert.equal(fx.stats().active,7,"Effects must have a hard capacity");
assert.equal(fx.render(geom,"high",200,200).triangles,0,
 "Faraway plumes must not incur geometry cost");
for(let i=0;i<40;i++)fx.update(.2);
assert.equal(fx.stats().active,0,"Effects must eventually be released");
assert(fx.stats().retired>=22);
fx.reset();assert.equal(fx.stats().spawned,0);
console.log("PASS battlefield atmosphere:",JSON.stringify({low,medium,high,stats:fx.stats()}));
