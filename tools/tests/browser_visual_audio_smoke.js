"use strict";
const assert=require("node:assert/strict");
const fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=path.resolve(__dirname,"../../browser/3d");
const art=fs.readFileSync(path.join(root,"battle-art.js"),"utf8");
const audio=fs.readFileSync(path.join(root,"battle-audio.js"),"utf8");
const win={};
vm.runInNewContext(art,{window:win,Math});
assert.equal(typeof win.WPBattleArt.soldier,"function");
const geom=()=>({boxes:[],cones:[],box(...a){this.boxes.push(a)},cone(...a){this.cones.push(a)}});
const poses=[];
for(const team of ["ally","axis"]){
 for(const t of [0,.4]){
  const g=geom();
  const state=win.WPBattleArt.soldier(g,{x:2,z:4,phase:1,alert:1,
   facing:.8,suppression:0,hit:0},team,t,2);
  assert(g.boxes.length>27,"Anatomy, equipment or clothing was omitted");
  assert.equal(state.pose,"walk");
  poses.push(g.boxes[0][2]);
 }
}
assert.notEqual(poses[0],poses[1],"Gait animation did not change over time");
const viewCounts={};
for(const weapon of ["rifle","smg","lmg"]){
 const g=geom();
 win.WPBattleArt.weapon(g,{x:0,z:0,yaw:.2,pitch:0,aim:false,crouch:false,weapon},
  {elapsed:1,flash:.1});
 assert(g.boxes.length>9);
 assert(g.cones.length===1);
 viewCounts[weapon]=g.boxes.length;
}
assert.notEqual(viewCounts.rifle,viewCounts.smg,"All weapons appear identical");
const param=()=>({value:0,setValueAtTime(){},linearRampToValueAtTime(){},
 exponentialRampToValueAtTime(){}});
const pan=new Proxy({value:0},{set(target,key,value){
 if(key==="value")assert(Number.isFinite(value),"Non-finite WebAudio stereo pan");
 target[key]=value;return true;
}});
const node=()=>({gain:param(),frequency:param(),pan,connect(){},start(){},stop(){}});
class AudioCtx{
 constructor(){this.sampleRate=22050;this.currentTime=0;this.state="running";this.destination=node();}
 createGain(){return node();}
 createBiquadFilter(){return node();}
 createBufferSource(){return node();}
 createStereoPanner(){return node();}
 createBuffer(channels,frames){return{duration:frames/this.sampleRate,getChannelData:()=>new Float32Array(frames)};}
 resume(){return Promise.resolve();}
}
win.AudioContext=AudioCtx;
vm.runInNewContext(audio,{window:win,Math});
const snd=win.WPBattleSound;
assert.equal(snd.state().active,false);
assert(snd.boot(),"Audio initiation should work in user gesture");
snd.shot("rifle",5,6,{x:0,z:0,yaw:0});
snd.shot("smg",2,3,{x:0,z:0,yaw:0});
snd.explosion(10,12,{x:0,z:0,yaw:0});
snd.footsteps(.4,4,false);
snd.distant(8.01);
snd.shot("rifle",NaN,Infinity,{x:NaN,z:3,yaw:undefined});
assert.equal(snd.state().shotCount,3);
assert(snd.setMuted(true));
snd.shot("lmg");
assert.equal(snd.state().shotCount,3,"Muted audio must suppress new reports");
console.log("PASS visual and spatial audio systems:",JSON.stringify({poses:poses.length,viewCounts,sound:snd.state()}));
