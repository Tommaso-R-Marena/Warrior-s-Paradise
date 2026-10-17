"use strict";
const fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const assert=require("node:assert/strict");
const root=path.resolve(__dirname,"../../browser/3d"),win={};
const context={window:win,Math,Map};
for(const name of ["battlefield-ground.js","battle-atmosphere.js"]){
 vm.runInNewContext(fs.readFileSync(path.join(root,name),"utf8"),context,{filename:name});
}
const ground=win.WPBattlefieldGround;
const height=(x,z)=>.18*Math.sin(x*.051)+.17*Math.cos(z*.067)+.20*Math.sin((x+z)*.032);
function collector(){
 return {triangles:[],tri(a,b,c,col){this.triangles.push({vs:[a,b,c],col});},
  quad(a,b,c,d,col){this.tri(a,b,c,col);this.tri(a,c,d,col);},
  cone(){}};
}
function checkGround(g,min=.027,max=.081){
 for(const tri of g.triangles){
  for(const [x,y,z] of tri.vs){
   assert([x,y,z].every(Number.isFinite),"nonfinite ground geometry");
   const delta=y-height(x,z);
   assert(delta>min-1e-6&&delta<max+1e-6,"decoration is floating, submerged or not terrain sampled: "+delta);
  }
 }
}
const base=collector(),b=ground.appendBase(base,height,()=>false);
assert(b.segments>=42&&b.segments<=48,"farm road continuity");
assert.equal(b.shoulders,2*b.segments);
assert.equal(base.triangles.length,b.triangles);
assert(b.shellScars>=18&&b.shellScars<=25,"ground scars missing");
checkGround(base,.028,.044);
const high=collector(),detail=ground.appendHigh(high,height,()=>false);
assert(detail.ruts>80&&detail.puddles>=10,"wet track ruts and puddles missing");
assert(detail.triangles<ground.maxHighTriangles,"triangle limit");
assert.equal(high.triangles.length,detail.triangles);
assert(high.triangles.some(q=>q.col[1]<0),"High puddles must have signed wet material tag");
checkGround(high,.050,.063);
const old=collector(),atmosphere=win.WPBattleAtmosphere;
assert(atmosphere.blast(9,31,"artillery"));
const drawn=atmosphere.render(old,"high",9,31,height);
assert(drawn.scars===1&&drawn.triangles>0);
for(const tri of old.triangles){
 for(const [x,y,z] of tri.vs)
  assert(y-height(x,z)>.07-1e-6&&y-height(x,z)<.081,"explosion ground scar at wrong elevation");
}
ground.reset();
const vehicle={id:"armor",x:-39,z:-23,yaw:0,width:2.8,speed:2.5,alive:true};
ground.markTracks([vehicle],.10,height,"high");
for(let i=0;i<9;i++){vehicle.z+=.65;ground.markTracks([vehicle],.10,height,"high");}
assert(ground.stats().marks>=12,"treads not accumulating on traversed ground");
const treadMesh=collector(),num=ground.drawTracks(treadMesh,"high",-39,-20,height);
assert(num>10&&num<=ground.trackCap*2,"drawn marks missing or outside budget");
checkGround(treadMesh,.063,.065);
const missing=collector();assert.equal(ground.drawTracks(missing,"low",-39,-20,height),0);
vehicle.z+=500;ground.markTracks([vehicle],.10,height,"high");
assert(ground.stats().marks<ground.trackCap,"teleport wrongly stamped giant ribbon");
for(let k=0;k<150;k++)ground.markTracks([], .20,height,"high");
assert.equal(ground.stats().marks,0,"tracks must expire in bounded lifetime");
ground.reset();assert.equal(ground.stats().marks,0);
const html=fs.readFileSync(path.join(root,"index.html"),"utf8"),game=fs.readFileSync(path.join(root,"game.js"),"utf8");
assert(html.includes("battlefield-ground.js"));
for(const value of ["WPBattlefieldGround.appendBase(","WPBattlefieldGround.appendHigh(",
 "WPBattlefieldGround.markTracks(","WPBattlefieldGround?.drawTracks(","wetTagged",
 "heightAt(c.x,c.z)"])
 assert((game+"\n"+fs.readFileSync(path.join(root,"battle-atmosphere.js"),"utf8")).includes(value),
  "Missing integration "+value);
console.log("PASS GROUNDED NORMANDY",JSON.stringify({roadSections:b.segments,roadTriangles:b.triangles,
 scars:b.shellScars,highRuts:detail.ruts,wetPatches:detail.puddles,highTriangles:detail.triangles,
 drivenTrackTriangles:num,craterTriangles:drawn.triangles}));
