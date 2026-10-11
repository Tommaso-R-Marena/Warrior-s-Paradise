"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=path.resolve(__dirname,"../../browser/3d"),window={};
const skinCode=fs.readFileSync(path.join(root,"skinning.js"),"utf8");
const content=fs.readFileSync(path.join(root,"assets/oga/allied_ww2.glb.b64"),"ascii");
const sandbox={window,fetch:async url=>{
 assert.equal(url,"./assets/oga/allied_ww2.glb.b64");
 return{ok:true,text:async()=>content};
},TextDecoder,Uint8Array,ArrayBuffer,DataView,Float32Array,Math,
atob:str=>Buffer.from(str,"base64").toString("binary")};
vm.createContext(sandbox);
vm.runInContext(skinCode,sandbox,{timeout:18000});
(async()=>{
 const model=await window.WPSkinning.load("allied_ww2");
 assert.equal(model.isHistorical,true);
 assert(model.mesh.length>0&&model.triangleCount>=100,"No actual skinned Allied geometry");
 assert(model.joints.length>15&&model.joints.length<=96,"Runtime bone budget violated");
 const walk=model.animations.findIndex(a=>/allied_walk/i.test(a.name));
 const run=model.animations.findIndex(a=>/allied_run/i.test(a.name));
 const aim=model.animations.findIndex(a=>/allied_aim/i.test(a.name));
 assert(walk>=0&&run>=0&&aim>=0,"Missing authored GLB animation actions");
 const poseAt=(t,idx)=>window.WPSkinning.pose(model,t,"walk",idx);
 const early=poseAt(.05,walk),late=poseAt(.25,walk);
 assert.equal(early.length,model.joints.length);
 let changing=0,movement=0;
 for(let j=0;j<early.length;j++)for(let k=0;k<16;k++){
  const delta=Math.abs(early[j][k]-late[j][k]);
  if(delta>1e-5)changing++;movement+=delta;
 }
 assert(changing>=3&&movement>.005,"Allied bone transforms are static through walking clip");
 const actor={x:0,z:0,facing:0,moveSpeed:2.5,crouch:false};
 let triangles=0,valid=true;
 const g={tri(a,b,c,col){triangles++;for(const p of [a,b,c,col])
  if(!p.every(Number.isFinite))valid=false;}};
 const res=window.WPSkinning.append(g,model,actor,"ally",.27);
 assert(res.triangles===triangles&&triangles>100&&valid,
  "Animated Allied mesh must emit finite geometry in the actual game renderer");
 console.log("PASS real Allied rig:",model.joints.length,"joints",
  model.animations.length,"GLB animations",changing,"joint matrix component changes",
  triangles,"animated mesh triangles");
})().catch(err=>{console.error(err);process.exitCode=1;});
