"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const folder=path.resolve(__dirname,"../../browser/3d"),root={};
vm.runInNewContext(fs.readFileSync(path.join(folder,"farmhouse-craft.js"),"utf8"),
 {window:root,Math,Number},{filename:"farmhouse-craft.js"});
const kit=root.WPFarmhouseCraft;
const boxes=[[-24,-12,11,9],[-2,-25,9,8],[18,-12,11,11],
 [-22,13,10,10],[5,27,9,12],[28,1,10,9],[-39,15,8,10],[35,-27,10,12],[-8,38,10,11]];
function run(){
 const triangles=[],materials=[];
 const geom={
  tri(a,b,c,col){triangles.push([a,b,c]);materials.push(col);},
  quad(a,b,c,d,col){this.tri(a,b,c,col);this.tri(a,c,d,col);},
  box(x,y,z,w,h,d,col){
   // Geometry.box emits 12 triangles; validate finite bounds per face.
   const p=[[x-w/2,y-h/2,z-d/2],[x+w/2,y-h/2,z-d/2],
   [x+w/2,y+h/2,z+d/2],[x-w/2,y+h/2,z+d/2]];
   for(let k=0;k<12;k++)this.tri(p[0],p[(k%3)+1],p[2],col);
  }
 };
 const result=kit.append(geom,boxes);
 assert.equal(result.triangles,triangles.length);
 assert.equal(result.buildings,9);
 assert.equal(result.windows,36);
 assert.equal(result.shutters,72);
 assert.equal(result.roofPots,18);
 assert(result.hinges>=120&&result.lintels===72);
 assert(result.triangles>=1800&&result.triangles<=kit.maxTriangles);
 assert.equal(result.drawCallsAdded,0);
 assert.equal(result.collidersAdded,0);
 for(let k=0;k<triangles.length;k++){
  assert(materials[k][0]<0,"Painted furniture must bypass terrain atlas");
  assert(materials[k].every(Number.isFinite));
  for(const v of triangles[k]){
   assert(v.every(Number.isFinite),"NaN building detail");
   assert(boxes.some(([x,z,w,d])=>Math.abs(v[0]-x)<w/2+.38&&
    Math.abs(v[2]-z)<d/2+.43),"window/shutter detached from real collision footprint");
   assert(v[1]>=0&&v[1]<7.25);
  }
 }
 return {result,samples:triangles.slice(0,3),colors:materials.slice(0,3)};
}
const a=run(),b=run();
assert.deepEqual(a,b,"original craft should be exactly repeatable");
const game=fs.readFileSync(path.join(folder,"game.js"),"utf8");
const html=fs.readFileSync(path.join(folder,"index.html"),"utf8");
assert(html.includes('farmhouse-craft.js'));
assert(game.includes("WPFarmhouseCraft.append("));
assert(game.includes("farmhouseCraft:"));
console.log("PASS FARMHOUSE AUTHORED DETAILS",JSON.stringify(a.result));
