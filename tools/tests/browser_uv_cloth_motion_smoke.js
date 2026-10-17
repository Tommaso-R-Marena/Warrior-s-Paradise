"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),
 vm=require("node:vm"),path=require("node:path");
const root=path.resolve(__dirname,"../../browser/3d"),window={};
const code=fs.readFileSync(path.join(root,"skinning.js"),"utf8");
const battleArt=fs.readFileSync(path.join(root,"battle-art.js"),"utf8");
vm.runInNewContext(code+"\n"+battleArt,{
 window,TextDecoder,Uint8Array,ArrayBuffer,DataView,Float32Array,Math,WeakMap,Set,Map
},{filename:"skin-and-art.js",timeout:20000});
const sk=window.WPSkinning;
function realModel(name){
 const b=Buffer.from(fs.readFileSync(path.join(root,"assets/oga/"+name+".glb.b64"),"ascii"),"base64");
 const ab=b.buffer.slice(b.byteOffset,b.byteOffset+b.byteLength);
 const m=sk.parse(ab);m.isHistorical=true;return m;
}
function triangles(model,actor,team,t){
 let n=0,ok=true,checksum=0,colors=0;
 const g={tri(a,b,c,color){
  n++;
  for(const v of [a,b,c,color])if(!v.every(Number.isFinite))ok=false;
  checksum+=a[0]+b[1]*.27+c[2]*.13;
  colors+=color[0]*.14+color[1]*.23+color[2]*.30;
 }};
 const result=sk.append(g,model,actor,team,t);
 assert(ok,"nonfinite skin vertex");
 assert.equal(n,result.triangles,"rendered skinned triangles disagree");
 return {...result,checksum,colors};
}
const ally=realModel("allied_ww2"),axis=realModel("axis_ww2");
assert(ally.mesh.some(m=>m.uv),"Allied artist UV coordinates discarded");
assert(axis.mesh.some(m=>m.uv),"Axis artist UV coordinates discarded");
assert(ally.animations.some(a=>/walk/i.test(a.name)));
assert(ally.animations.some(a=>/run/i.test(a.name)));
assert(ally.animations.some(a=>/aim/i.test(a.name)));
assert.notEqual(sk.clothAlbedo([.2,.3],"ally"),sk.clothAlbedo([.8,.9],"ally"));
const actor={x:3,z:9,groundY:.20,facing:.4,moveSpeed:0,phase:0,crouch:false,hp:90};
const idle=triangles(ally,actor,"ally",.1);
assert(!idle.blended&&idle.uvTexturedTriangles>0,"authored UV texture not used");
actor.moveSpeed=2.5;
const walk0=triangles(ally,actor,"ally",.2);
assert(walk0.blended&&walk0.mode==="walk");
const walk1=triangles(ally,actor,"ally",.32);
assert(walk1.blended,"intermediate 240ms transition missing");
const walkEnd=triangles(ally,actor,"ally",.46);
assert(!walkEnd.blended,"locomotion blend must end after 240ms");
actor.moveSpeed=4.2;
const run0=triangles(ally,actor,"ally",.50);
assert(run0.blended&&run0.mode==="run");
const run1=triangles(ally,actor,"ally",.80);
assert(!run1.blended);
assert(Math.abs(run1.checksum-walkEnd.checksum)>.00001,"run and walk should deform mesh");
actor.crouch=true;actor.moveSpeed=.8;
const crouchStart=triangles(ally,actor,"ally",.85);
const crouchEnd=triangles(ally,actor,"ally",1.2);
assert(crouchStart.blended&&crouchEnd.mode==="crouch"&&!crouchEnd.blended);
assert(Math.abs(crouchEnd.checksum-run1.checksum)>.0001,"crouch silhouette must change");
const axisActor={x:4,z:10,groundY:0,facing:-.5,moveSpeed:2,crouch:false};
const axisWalk=triangles(axis,axisActor,"axis",.3);
axisActor.moveSpeed=0;
const axisAim=triangles(axis,axisActor,"axis",.34);
assert(axisAim.blended,"unanimated Axis glTF fallback action transition missing");
assert(axisWalk.uvTexturedTriangles>0,"Axis cloth UV not attached");
const cached=ally.mesh.filter(m=>m.uv);
assert(cached.every(m=>m.uvClothCache?.ally?.length===m.positions.length/3),
 "UV weave must be cached per mesh vertex, not recomputed for each frame");
const pose0=sk.pose(ally,.25,"walk",ally.animations.findIndex(a=>/walk/i.test(a.name)));
const pose1=sk.pose(ally,.25,"aim",ally.animations.findIndex(a=>/aim/i.test(a.name)));
const middle=sk.pose(ally,.25,"aim",ally.animations.findIndex(a=>/aim/i.test(a.name)),
 {fromMode:"walk",fromClip:ally.animations.findIndex(a=>/walk/i.test(a.name)),alpha:.5});
let middleValid=0;
for(let i=0;i<middle.length;i++)for(let j=0;j<16;j++){
 assert(Number.isFinite(middle[i][j]),"blended joint nonfinite");
 if(Math.abs(pose0[i][j]-pose1[i][j])>.02)middleValid++;
}
assert(middleValid>4,"author rig poses not meaningfully different");
const art=window.WPBattleArt;
function fallback(speed,crouch,tier){
 const f={x:0,z:0,groundY:0,facing:0,moveSpeed:speed,crouch};
 let boxes=0,finite=true,checksum=0;
 const g={box(x,y,z,w,h,d,c,yaw){
  boxes++;if(![x,y,z,w,h,d,yaw,...c].every(Number.isFinite))finite=false;
  checksum+=x*.7+y+z*.5;
 }};
 const state=art.soldier(g,f,"ally",1.27,speed,tier);
 assert(finite,"fallback soldier contains invalid moving geometry");
 return {...state,boxes,checksum};
}
const standing=fallback(0,false,"high"),walking=fallback(2,false,"high"),
 running=fallback(4.3,false,"high"),crouched=fallback(.9,true,"high");
assert.deepEqual([standing.pose,walking.pose,running.pose,crouched.pose],
 ["aim","walk","run","crouch"]);
assert(running.cadence>walking.cadence&&walking.cadence>crouched.cadence);
assert(standing.boxes>fallback(0,false,"low").boxes,"High outfit geometry not tier gated");
const source=fs.readFileSync(path.join(root,"game.js"),"utf8");
for(const token of ["skinUVFabricTriangles","skinBlendedActors","rig.uvFabricTriangles",
 "rig.blendedActors","art.soldier(m,actor,team,game.elapsed,actor.moveSpeed||0,game.quality)"])
 assert(source.includes(token),"runtime integration missing: "+token);
console.log("PASS REAL UV CLOTH AND INFANTRY TRANSITIONS",JSON.stringify({
 ally:{joints:ally.joints.length,triangles:idle.triangles,
 uvTriangles:idle.uvTexturedTriangles,authoredClips:ally.animations.length},
 axis:{joints:axis.joints.length,triangles:axisWalk.triangles,
 uvTriangles:axisWalk.uvTexturedTriangles},
 walkBlend:true,runBlend:true,crouchBlend:true,validJointDifferences:middleValid,
 fallback:[standing.pose,walking.pose,running.pose,crouched.pose],
 extraHighEquipment:standing.boxes-fallback(0,false,"low").boxes}));
