/* Warrior's Paradise — spatial, multi-layered battlefield audio.
 * Entirely original synthesized sounds. No audio downloads, CDN or analytics.
 * All audio is initiated by a direct user gesture and can be muted instantly.
 */
(function(root){"use strict";
let context=null,master=null,ambience=null,active=false,muted=false;
const buffers=new Map();
let lastFootstep=0,ambientStarted=false,shotCount=0;
const state=()=>({available:!!(root.AudioContext||root.webkitAudioContext),active,
 muted,ambience:ambientStarted,shotCount});
function noise(duration,kind="broad"){
 if(!context)return null;
 const key=kind+":"+duration;
 if(buffers.has(key))return buffers.get(key);
 const b=context.createBuffer(1,Math.ceil(context.sampleRate*duration),context.sampleRate);
 const p=b.getChannelData(0);let brown=0;
 for(let i=0;i<p.length;i++){
  const white=Math.random()*2-1;
  brown=(brown+.03*white)/1.03;
  const x=i/p.length;
  p[i]=kind==="brown"?brown*3.5:kind==="impulse"?white*Math.exp(-15*x):white;
 }
 buffers.set(key,b);return b;
}
function boot(){
 if(muted)return false;
 if(!context){
  const C=root.AudioContext||root.webkitAudioContext;
  if(!C)return false;
  try{
   context=new C();master=context.createGain();master.gain.value=.65;
   master.connect(context.destination);
  }catch{return false;}
 }
 if(context.state==="suspended")context.resume().catch(()=>{});
 active=true;
 if(!ambientStarted)wind();
 return true;
}
function wind(){
 if(!context||ambientStarted)return;
 try{
  ambientStarted=true;
  const source=context.createBufferSource();
  source.buffer=noise(3.1,"brown");source.loop=true;
  const filter=context.createBiquadFilter();filter.type="lowpass";filter.frequency.value=550;
  ambience=context.createGain();ambience.gain.value=.047;
  source.connect(filter);filter.connect(ambience);ambience.connect(master);
  source.start();
 }catch{ambientStarted=false;}
}
function place(node,x=0,z=0,ear={x:0,z:0,yaw:0}){
 const d=Math.hypot(x-ear.x,z-ear.z);
 const attenuation=1/(1+.036*d+.005*d*d);
 const gain=context.createGain();
 gain.gain.value=Math.min(1,attenuation);
 node.connect(gain);
 if(context.createStereoPanner){
  const pan=context.createStereoPanner();
  const dx=x-ear.x,dz=z-ear.z;
  pan.pan.value=Math.max(-.92,Math.min(.92,(dx*Math.cos(ear.yaw)+dz*Math.sin(ear.y))/Math.max(4,d)));
  gain.connect(pan);pan.connect(master);
 }else gain.connect(master);
 return gain;
}
function burst({freq=280,duration=.17,loud=.2,kind="impulse",x=0,z=0,ear,delay=0}){
 if(!active||muted||!context)return;
 const now=context.currentTime+delay;
 const buffer=noise(Math.max(.06,duration),"impulse");
 const source=context.createBufferSource();source.buffer=buffer;
 const filter=context.createBiquadFilter();filter.type="lowpass";filter.frequency.value=freq;
 const envelope=context.createGain();envelope.gain.setValueAtTime(.001,now);
 envelope.gain.linearRampToValueAtTime(loud,now+.003);
 envelope.gain.exponentialRampToValueAtTime(.001,now+duration);
 source.connect(filter);filter.connect(envelope);
 place(envelope,x,z,ear);
 source.start(now);source.stop(now+Math.min(duration,buffer.duration));
}
function shot(weapon,x=0,z=0,ear){
 if(!boot())return;
 shotCount++;
 const spec=weapon==="smg"?{f:950,d:.12,l:.18}:weapon==="lmg"?{f:690,d:.19,l:.22}:{f:610,d:.31,l:.29};
 burst({freq:spec.f,duration:spec.d,loud:spec.l,x,z,ear});
 burst({freq:95,duration:.14,loud:spec.l*.65,x,z,ear});
}
function explosion(x=0,z=0,ear){
 if(!boot())return;
 burst({freq:160,duration:.9,loud:.32,x,z,ear});
 burst({freq:70,duration:.65,loud:.32,x,z,ear,delay:.05});
 burst({freq:1600,duration:.28,loud:.13,x,z,ear,delay:.12});
}
function footsteps(time,speed,crouch=false){
 if(!active||muted||speed<1.1||time-lastFootstep<(crouch?.51:.33))return;
 lastFootstep=time;
 burst({freq:crouch?210:380,duration:.065,loud:crouch?.025:.055});
}
function distant(now){
 if(!active||muted)return;
 // Deterministic event cadence (no simulation side effects): one distant report every 8 seconds.
 if(Math.floor(now/8)!==Math.floor((now-.045)/8)){
  burst({freq:180,duration:.3,loud:.07,x:22,z:33,ear:{x:0,z:0,yaw:0}});
 }
}
function setMuted(next){
 muted=!!next;
 if(master)master.gain.value=muted?0:.65;
 return muted;
}
root.WPBattleSound=Object.freeze({boot,shot,explosion,footsteps,distant,
 setMuted,state});
})(window);