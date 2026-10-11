"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const script=fs.readFileSync(path.resolve(__dirname,"../../browser/3d/ww2-environment.js"),"utf8");
const root={};vm.runInNewContext(script,{window:root,Math});
const env=root.WPWW2Environment;
assert.equal(env.maxTriangles,1400);
function run(){
 let emitted=0;
 const g={tri(){emitted++;},box(){emitted+=12;},cone(x,y,z,r,h,col,faces=7){assert(Number.isFinite(x+y+z+r+h));emitted+=faces;}};
 const flags=[{x:-12,z:-5},{x:11,z:11},{x:31,z:29}];
 const stats=env.append(g,()=>false,flags);
 assert.equal(stats.triangles,emitted,"Reported triangle budget must reflect real mesh triangles");
 assert(stats.triangles>100&&stats.triangles<=1400,"Static WWII environment geometry cap violated");
 assert(stats.hedges>0&&stats.posts>0&&stats.rubble>0,"World lacks integrated hedgerows, poles or debris");
 return stats;
}
assert.deepEqual(run(),run(),"High-tier war damage must be deterministic and reproducible");
console.log("PASS WW2 environment: deterministic, bounded hedgerows, telegraph poles and rubble");
