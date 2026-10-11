"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const script=fs.readFileSync(path.resolve(__dirname,"../../browser/3d/ww2-terrain.js"),"utf8");
const scope={};vm.runInNewContext(script,{window:scope,Math,Number});
const terrain=scope.WPWW2Terrain;
const flags=[{x:-12,z:-5},{x:11,z:11},{x:31,z:29}];
const boxes=[[-24,-12,11,9],[-2,-25,9,8],[18,-12,11,11],[-22,13,10,10],[5,27,9,12],[28,1,10,9]];
function create(){
 let emitted=0;
 const g={tri(...points){
   emitted++;
   for(const p of points.slice(0,3))assert(p.every(Number.isFinite));
 }};
 const a=terrain.append(g,flags,boxes);
 assert.equal(a.triangles,emitted);
 assert.equal(a.triangles,terrain.maxTriangles);
 assert(a.shadowedCells>0,"Sunlight occlusion never applied");
 assert(a.minY>-.11&&a.maxY<.25,"Gameplay terrain elevation too large");
 assert.equal(a.drawCallsAdded,0);
 return {triangles:a.triangles,shadowedCells:a.shadowedCells,minY:a.minY,maxY:a.maxY};
}
assert.equal(terrain.surface(10,10,flags,boxes),-.071,"Farm track cannot undulate");
assert.deepEqual(create(),create(),"Terrain changes across reproducible loads");
console.log("PASS terrain: bounded mesh, level road, deterministic baked sunlight occlusion");
