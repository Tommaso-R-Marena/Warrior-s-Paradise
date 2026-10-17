"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const folder=path.resolve(__dirname,"../../browser/3d"),window={};
vm.runInNewContext(fs.readFileSync(path.join(folder,"architectural-weathering.js"),"utf8"),
 {window,Math},{filename:"architectural-weathering.js"});
const weather=window.WPArchitecturalWeathering;
const boxes=[[-24,-12,11,9],[-2,-25,9,8],[18,-12,11,11],[-22,13,10,10],
 [5,27,9,12],[28,1,10,9],[-39,15,8,10],[35,-27,10,12],[-8,38,10,11]];
function run(){
 const triangles=[],colors=[],g={
  tri(a,b,c,color){triangles.push([a,b,c]);colors.push(color);},
  quad(a,b,c,d,color){
   triangles.push([a,b,c],[a,c,d]);colors.push(color,color);
  }};
 const stats=weather.append(g,boxes);
 assert(stats.triangles===triangles.length,"real geometry count must equal diagnostic");
 assert(stats.triangles>600&&stats.triangles<=weather.maxTriangles,"budget");
 assert.equal(stats.collidersAdded,0);
 assert.equal(stats.drawCallsAdded,0);
 assert(stats.ivyLeaves>50&&stats.patches>100,"weathering layers missing");
 assert.equal(stats.buildings,boxes.length);
 for(let i=0;i<triangles.length;i++)for(const p of triangles[i]){
  assert(p.every(Number.isFinite),"nonfinite visual vertex");
  const b=boxes.find(([x,z,w,d])=>
   Math.abs(p[0]-x)<w/2+.32&&Math.abs(p[2]-z)<d/2+.40);
  assert(b,"visual detail must remain on pre-existing physical building");
  assert(p[1]>=.15&&p[1]<=4.8,"facade detaches from original house");
 }
 const tagged=colors.filter(c=>c[0]<0),leaf=colors.filter(c=>c[0]>0&&c[1]>c[0]);
 assert(tagged.length>200&&leaf.length>50,"paint and foliage material classification");
 return {stats,positions:triangles.slice(0,3),materials:colors.slice(0,4)};
}
const a=run(),b=run();
assert.deepEqual(a,b,"artist seed and geometry must reproduce on any replay");
const source=fs.readFileSync(path.join(folder,"game.js"),"utf8");
const html=fs.readFileSync(path.join(folder,"index.html"),"utf8");
assert(html.includes("architectural-weathering.js"),"weathering script not loaded");
for(const s of ["WPArchitecturalWeathering.append(","weatheringTriangles",
 "weatheringLeaves","weatheringPatches","step(0.,aColor.r)*step("])
 assert(source.includes(s),"missing High renderer contract "+s);
console.log("PASS NORMANDY WEATHERED FACADES",JSON.stringify(a.stats));
