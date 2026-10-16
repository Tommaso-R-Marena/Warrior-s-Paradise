"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const js=fs.readFileSync(path.resolve(__dirname,"../../browser/3d/ww2-terrain.js"),"utf8");
const scope={};vm.runInNewContext(js,{window:scope,Math,Number,Float64Array}, {timeout:15000});
const t=scope.WPWW2Terrain;
const flags=[{x:-12,z:-5},{x:11,z:11},{x:31,z:29}];
const boxes=[[-24,-12,11,9],[-2,-25,9,8],[18,-12,11,11],[-22,13,10,10],
 [5,27,9,12],[28,1,10,9],[-39,15,8,10],[35,-27,10,12],[-8,38,10,11]];
assert.equal(t.baseTriangles,722);
assert.equal(t.maxTriangles,2888);
assert.equal(t.surface(10,10,flags,boxes),-.071,"The road remains level");
assert.equal(t.surface(-36,-33,flags,boxes),-.071,"Allied spawn remains level");
function run(level){
 const emitted=[],g={tri(a,b,c,color){
  for(const p of [a,b,c,color])assert([...p].every(Number.isFinite));
  emitted.push([a,b,c]);
 }};
 const record=t.append(g,flags,boxes,level);
 assert.equal(record.triangles,emitted.length);
 assert.equal(record.triangles,level==="base"?722:2888);
 assert.equal(record.drawCallsAdded,0,"No extra GPU draw call required");
 assert(record.maxY-record.minY>.46,"Heightfield must have significant real relief");
 assert(record.shadowedCells>0,"Static buildings cast no visual terrain shading");
 return {record,emitted};
}
const low=run("base"),high=run("high");
assert(high.record.maxY-high.record.minY>.46);
const STEP=94/19;
for(const [ix,iz] of [[0,0],[5,8],[9,7],[15,10],[18,18]]){
 const x=-47+ix*STEP,z=-47+iz*STEP;
 const h=t.surface(x,z,flags,boxes);
 assert(Math.abs(h-low.emitted[(iz*19+ix)*2][0][1])<1e-8,
  "Render triangle vertex differs from physical ground elevation");
}
// Every 2x High subdivision follows the same coarse triangle surface.
for(let i=0;i<50;i++){
 const x=-44+i*1.67,z=-42+i*1.32,y=t.surface(x,z,flags,boxes);
 assert(Number.isFinite(y));
 const slope=t.slope(x,z,flags,boxes);
 assert(Number.isFinite(slope.grade)&&Math.abs(Math.hypot(...slope.normal)-1)<1e-5);
}
let steep=null;
for(let z=-44;z<=42;z+=1.2)for(let x=-44;x<=42;x+=1.2){
 if(t.slope(x,z,flags,boxes).grade>.74){steep={x,z};break;}
 if(steep)break;
}
assert(steep,"The terrain should contain genuine high-grade topological features");
assert.equal(t.traversable(steep.x,steep.z,steep.x+.12,steep.z,.44,flags,boxes),false,
 "Player must not walk straight through excessively steep ground");
assert.equal(t.traversable(10,10,10.2,10.2,.44,flags,boxes),true,
 "Country road should allow movement");
assert.equal(t.traversable(10,10,10.2,10.2,1.35,flags,boxes,"vehicle"),true,
 "Vehicle should traverse level road");
assert.equal(t.traversable(steep.x,steep.z,steep.x+.12,steep.z,1.2,flags,boxes,"vehicle"),false);
// A hillside must occlude a projectile, including targets whose hitboxes sit
// at a raised physical Y coordinate instead of the former flat world floor.
const bulletScope={};
vm.runInNewContext(fs.readFileSync(path.resolve(__dirname,"../../browser/3d/infantry-ballistics.js"),"utf8"),
 {window:bulletScope,Math,Number,Error},{timeout:15000});
const ballistic=bulletScope.WPInfantryBallistics;
const e={x:0,z:12,hp:80,groundY:0,alert:0};
const common={source:{x:0,y:1.67,z:0},weapon:"rifle",yaw:0,pitch:0,
 targets:[e]};
const straight=ballistic.trace({...common,obstacles:{groundHeight:()=>0}});
assert.equal(straight.hit,e,"Unobstructed flat-ground shot should still hit");
const hidden=ballistic.trace({...common,obstacles:{groundHeight:(x,z)=>z>5&&z<8?2.7:0}});
assert.equal(hidden.hit,null,"A terrain ridge must absorb a bullet");
assert.equal(hidden.blockedBy,"terrain");
const uphillShot=ballistic.trace({...common,pitch:.12,
 targets:[{...e,groundY:2}],obstacles:{groundHeight:()=>0}});
assert(uphillShot.hit&&uphillShot.damage>0,"Elevated target must use matching collision height");
console.log("PASS topography: shared mesh, steep-ground limits, elevated hitboxes and terrain-blocked bullets");
