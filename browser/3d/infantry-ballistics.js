/* Warrior's Paradise — deterministic single-player infantry ballistics.
 * "Effective maximum range" is a deliberate MAP-SCALE gameplay cap, NOT
 * the real-world physical maximum flight range of a .303 or 9mm bullet.
 * No enemy alertness test occurs in a hit calculation: surprise attacks work.
 */
(function(root){"use strict";
const PROFILES=Object.freeze({
 rifle:Object.freeze({name:"Lee-Enfield No.4",damage:62,fullDamage:14,maxRange:65,
  edgeFactor:.30,muzzleVelocity:744,headMultiplier:2.25,limbMultiplier:.64}),
 smg:Object.freeze({name:"Sten Mk II",damage:29,fullDamage:8,maxRange:31,
  edgeFactor:.27,muzzleVelocity:365,headMultiplier:2.0,limbMultiplier:.67}),
 lmg:Object.freeze({name:"Bren Mk I",damage:42,fullDamage:13,maxRange:56,
  edgeFactor:.32,muzzleVelocity:744,headMultiplier:2.15,limbMultiplier:.7})
});
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
function damageAt(weapon,meters,zone="torso"){
 const w=PROFILES[weapon];if(!w||!Number.isFinite(meters)||meters<0||meters>w.maxRange)return 0;
 const f=meters<=w.fullDamage?1:1-(1-w.edgeFactor)*
  clamp((meters-w.fullDamage)/(w.maxRange-w.fullDamage),0,1);
 const anatomical=zone==="head"?w.headMultiplier:
  zone==="leg"||zone==="arm"?w.limbMultiplier:1;
 return Math.round(w.damage*f*anatomical*10)/10;
}
function projectileY(y,pitch,range,velocity){
 // Approximate bullet path in local vertical plane; horizontal distance is
 // range, not projectile path length. 9.81 m/s² acts throughout flight.
 return y+Math.tan(pitch)*range-4.905*range*range/
  Math.max(1,velocity*velocity*Math.cos(pitch)*Math.cos(pitch));
}
function cylinderInterval(ox,oz,dx,dz,x,z,r,maxRange){
 const rx=ox-x,rz=oz-z,b=2*(rx*dx+rz*dz),c=rx*rx+rz*rz-r*r;
 const disc=b*b-4*c;if(disc<0)return null;
 const rootD=Math.sqrt(disc),near=(-b-rootD)/2,far=(-b+rootD)/2;
 if(far<.15||near>maxRange)return null;
 return [Math.max(.15,near),Math.min(maxRange,far)];
}
function hitEnemy(eye,dx,dz,pitch,velocity,e,maxRange){
 const crouch=!!e.crouch||e.suppression>1.2,drop=crouch?.42:0,ground=e.groundY||0;
 const zones=[
  {name:"head",dx:0,r:.285,low:1.91-drop+ground,high:2.44-drop+ground},
  {name:"torso",dx:0,r:.42,low:.94-drop+ground,high:1.91-drop+ground},
  {name:"arm",dx:-.49,r:.21,low:1.03-drop+ground,high:1.77-drop+ground},
  {name:"arm",dx:.49,r:.21,low:1.03-drop+ground,high:1.77-drop+ground},
  {name:"leg",dx:-.20,r:.19,low:.12-drop*.40+ground,high:.97-drop*.55+ground},
  {name:"leg",dx:.20,r:.19,low:.12-drop*.40+ground,high:.97-drop*.55+ground}
 ];
 let best=null;
 for(const z of zones){
  const interval=cylinderInterval(eye.x,eye.z,dx,dz,e.x+z.dx,e.z,z.r,maxRange);
  if(!interval)continue;
  // Intersect small arcing bullet trajectory with upright region, including
  // characters facing away. Take closest true 3D sample (not center distance).
  const span=interval[1]-interval[0],steps=Math.max(1,Math.ceil(span/.10));
  for(let i=0;i<=steps;i++){
   const t=interval[0]+span*i/steps;
   const y=projectileY(eye.y,pitch,t,velocity);
   if(y>=z.low&&y<=z.high&&(!best||t<best.distance)){
    best={target:e,zone:z.name,distance:t,y};
    break;
   }
  }
 }
 return best;
}
function firstCover(eye,dx,dz,pitch,velocity,obstacles,maxRange){
 let best=null;
 const terrain=typeof obstacles?.terrain==="function"?obstacles.terrain:null;
 function record(t,type){
  if(t<.2||t>maxRange||(best&&t>=best.distance))return;
  best={distance:t,type,y:projectileY(eye.y,pitch,t,velocity)};
 }
 const bxs=[...(obstacles?.buildings||[]).map(b=>[b[0],b[1],b[2],b[3],5.8,"masonry"]),
  ...(obstacles?.covers||[]).map(b=>[b[0],b[1],b[2],b[3],.97,"sandbags"]),
  ...(obstacles?.structures||[]).map(b=>[b[0],b[1],b[2],b[3],2.6,"farm obstacle"])];
 for(const [cx,cz,w,d,height,type] of bxs){
  let tMin=.2,tMax=maxRange;
  for(const [pos,dir,c,half] of [[eye.x,dx,cx,w/2],[eye.z,dz,cz,d/2]]){
   if(Math.abs(dir)<1e-8){if(pos<c-half||pos>c+half){tMin=Infinity;break;}continue;}
   const t1=(c-half-pos)/dir,t2=(c+half-pos)/dir;
   tMin=Math.max(tMin,Math.min(t1,t2));tMax=Math.min(tMax,Math.max(t1,t2));
  }
  if(tMin>tMax||tMin>maxRange)continue;
  // If the trajectory passes over the wall, don't pretend the obstacle blocks.
  const a=projectileY(eye.y,pitch,tMin,velocity);
  const b=projectileY(eye.y,pitch,tMax,velocity);
  const localGround=terrain?terrain(cx,cz):0;
  if((a>=localGround+.03&&a<=localGround+height)||
     (b>=localGround+.03&&b<=localGround+height))record(tMin,type);
 }
 for(const tr of obstacles?.trunks||[]){
  const area=cylinderInterval(eye.x,eye.z,dx,dz,tr.x,tr.z,tr.radius||.45,maxRange);
  if(!area)continue;
  const y=projectileY(eye.y,pitch,area[0],velocity);
  const base=terrain?terrain(tr.x,tr.z):0;
  if(y>=base&&y<=base+5.5)record(area[0],"tree");
 }
 // The SAME topological surface used to draw the ground and walk on it must
 // stop shots. Terrain sample checks are only made when firing, not per frame.
 if(terrain){
  const limit=Math.min(maxRange,best?.distance??maxRange);
  for(let t=.28;t<=limit+.001;t+=.28){
   const px=eye.x+dx*t,pz=eye.z+dz*t;
   if(projectileY(eye.y,pitch,t,velocity)<terrain(px,pz)+.035){
    record(t,"terrain");break;
   }
  }
 }else if(pitch<-.006){
  // Backward compatible level-ground fallback for standalone API users.
  for(let t=.5;t<=Math.min(maxRange,best?.distance??maxRange);t+=.5){
   if(projectileY(eye.y,pitch,t,velocity)<0){record(t,"ground");break;}
  }
 }
 return best;
}
function trace({source,weapon,yaw,pitch=0,targets=[],obstacles=null,
 spreadYaw=0,spreadPitch=0}){
 const w=PROFILES[weapon];if(!w)throw Error("Unknown infantry weapon "+weapon);
 if(!Number.isFinite(yaw)||!Number.isFinite(pitch)||!source||
   ![source.x,source.y,source.z].every(Number.isFinite))throw Error("Invalid firing origin");
 const angle=yaw+spreadYaw,tilt=clamp(pitch+spreadPitch,-.85,.85);
 const dx=-Math.sin(angle),dz=Math.cos(angle);
 let hit=null;
 for(const e of targets){
  if(!e||e.hp<=0)continue;
  const candidate=hitEnemy(source,dx,dz,tilt,w.muzzleVelocity,e,w.maxRange);
  if(candidate&&(!hit||candidate.distance<hit.distance))hit=candidate;
 }
 const cover=firstCover(source,dx,dz,tilt,w.muzzleVelocity,obstacles,w.maxRange);
 if(cover&&(!hit||cover.distance<hit.distance-.04))hit=null;
 const distance=hit?hit.distance:cover?.distance??w.maxRange;
 const endpoint={x:source.x+dx*distance,
  y:projectileY(source.y,tilt,distance,w.muzzleVelocity),
  z:source.z+dz*distance};
 const nearMisses=[];
 if(!hit){
  for(const e of targets){
   if(!e||e.hp<=0)continue;
   const rx=e.x-source.x,rz=e.z-source.z,t=rx*dx+rz*dz;
   if(t<.5||t>=distance||t>w.maxRange)continue;
   const lateral=Math.abs(rx*dz-rz*dx);
   const vertical=Math.abs(projectileY(source.y,tilt,t,w.muzzleVelocity)-(e.groundY||0)-1.45);
   if(lateral<1.25&&vertical<1.2)nearMisses.push(e);
  }
 }
 return {weapon,hit:hit?.target||null,zone:hit?.zone||null,
  damage:hit?damageAt(weapon,hit.distance,hit.zone):0,
  distance,endpoint,blockedBy:!hit&&cover?cover.type:null,
  reason:hit?"hit":cover?"cover":"range-or-miss",nearMisses};
}
function canSee(observer,target,hasLOS,maxDistance=29){
 if(!observer||!target||target.hp<=0)return false;
 const d=Math.hypot(target.x-observer.x,target.z-observer.z);
 const crouch=!!target.crouch,range=maxDistance*(crouch?.66:1);
 if(d>range)return false;
 // Direction-sensitive awareness: undetected targets directly behind are
 // invisible until they approach closely. Damage recognition occurs separately.
 if(d>5.2){
  const facing=Number.isFinite(observer.facing)?observer.facing:0;
  const fx=Math.sin(facing),fz=Math.cos(facing);
  const dot=((target.x-observer.x)*fx+(target.z-observer.z)*fz)/Math.max(.001,d);
  if(dot<-.05)return false; // ~93 degree half-angle cone.
 }
 return !!hasLOS(observer.x,observer.z,target.x,target.z);
}
root.WPInfantryBallistics=Object.freeze({profiles:PROFILES,damageAt,trace,canSee,projectileY});
})(window);
