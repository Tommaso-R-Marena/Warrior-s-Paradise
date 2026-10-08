"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),
 crypto=require("node:crypto"),vm=require("node:vm");
const root=path.resolve(__dirname,"../../browser/3d");
const manifest=JSON.parse(fs.readFileSync(path.join(root,"assets/audio/MANIFEST.json"),"utf8"));
assert.equal(manifest.license,"CC0-1.0");
const sample=fs.readFileSync(path.join(root,"assets/audio/cc0-gunshot.wav.b64"),"utf8").trim();
const bytes=Buffer.from(sample,"base64");
assert.equal(bytes.length,manifest.audio_bytes);
assert.equal(bytes.toString("ascii",0,4),"RIFF");
assert.equal(bytes.toString("ascii",8,12),"WAVE");
assert.equal(crypto.createHash("sha1").update("blob "+bytes.length+"\0").update(bytes).digest("hex"),
 manifest.source_blob_sha1);
const source=fs.readFileSync(path.join(root,"battle-audio.js"),"utf8");
function node(){return{gain:parameter(),frequency:parameter(),pan:parameter(),playbackRate:parameter(),
 connect(){},start(){},stop(){}}}
function parameter(){return{value:0,setValueAtTime(){},linearRampToValueAtTime(){},exponentialRampToValueAtTime(){}}}
class AC {
 constructor(){this.currentTime=0;this.sampleRate=22050;this.state="running";this.destination=node();}
 createGain(){return node()}createStereoPanner(){return node()}createBiquadFilter(){return node()}
 createBufferSource(){return node()}
 createBuffer(_,n){return{duration:n/this.sampleRate,getChannelData:()=>new Float32Array(n)}}
 decodeAudioData(buffer){assert.equal(buffer.byteLength,bytes.length);return Promise.resolve({duration:.87})}
 resume(){return Promise.resolve()}
}
async function run(ok){
 const win={AudioContext:AC};
 const ctx={window:win,Math,Uint8Array,atob:s=>Buffer.from(s,"base64").toString("binary"),
 fetch:async path=>{
  assert.equal(path,"./assets/audio/cc0-gunshot.wav.b64");
  return{ok,status:ok?200:404,text:async()=>sample};
 }};
 vm.runInNewContext(source,ctx,{filename:"battle-audio.js"});
 const sound=win.WPBattleSound;
 assert(sound.boot());
 for(let i=0;i<20;i++)await Promise.resolve();
 assert.equal(sound.state().recordedReady,ok);
 sound.shot("rifle",0,1,{x:0,z:0,yaw:0});
 assert.equal(sound.state().shotCount,1);
 return sound.state();
}
(async()=>{
 const success=await run(true),fallback=await run(false);
 assert(!fallback.recordedReady);
 console.log("PASS CC0 recorded battlefield audio:",JSON.stringify({bytes:bytes.length,success,fallback}));
})().catch(error=>{console.error(error);process.exitCode=1});
