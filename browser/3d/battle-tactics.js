/* Deterministic bounded squad-level combat behavior — browser/phone safe.
 * Suppressed combatants look for actual LOS-blocking terrain, use a peek cycle,
 * recover morale and relocate if the hiding place becomes a dead end.
 * This does not claim full squad command/ballistic armor simulation.
 */
(function(root){"use strict";
const clamp=(x,a,b)=>Math.max(a,Math.min(b,x));
const distance=(a,b,x,z)=>Math.hypot(a-x,b-z);
const stats={threats:0,coverFound:0,retreats:0,peeks:0};
function chooseCover(actor,threat,obstacles,blocked,lineHit){
 const covers=[...(obstacles?.covers||[]),...(obstacles?.buildings||[])];
 if(!Number.isFinite(threat?.x)||!Number.isFinite(threat?.z))return null;
 let winner=null,best=Infinity,inspected=0;
 for(const c of covers){
  if(++inspected>48)break; // hard per-event computational cap
  if(!Array.isArray(c)||c.length<4)continue;
  const [x,z,w,d]=c,from=distance(x,z,threat.x,threat.z);
  if(from<1.2||from>45||distance(x,z,actor.x,actor.z)>14)continue;
  const ux=(x-threat.x)/from,uz=(z-threat.z)/from;
  // Position wholly behind the obstacle from the shooter's direction.
  const shield=Math.max(w,d)*.51+.85;
  const px=x+ux*shield,pz=z+uz*shield;
  if(blocked(px,pz,.42)||lineHit(threat.x,threat.z,px,pz))continue;
  const dist=distance(actor.x,actor.z,px,pz);
  const score=dist+.07*distance(x,z,threat.x,threat.z);
  if(score<best){best=score;winner={x:px,z:pz,objectX:x,objectZ:z,age:0,
   peekX:px,peekZ:pz};}
 }
 if(winner){
  // Pick one reachable firing notch to either side of the obstacle.
  // A true firing notch must clear the near-edge silhouette from the shooter.
  const span=Math.max(2.7,Math.min(5,Math.hypot(winner.x-winner.objectX,winner.z-winner.objectZ)*1.35+1.6));
  const vx=(winner.objectZ-threat.z),vz=-(winner.objectX-threat.x);
  const vl=Math.hypot(vx,vz)||1;
  for(const side of [actor.strafe||1,-(actor.strafe||1)]){
   const nx=winner.x+vx/vl*span*side,nz=winner.z+vz/vl*span*side;
   if(!blocked(nx,nz,.42)&&lineHit(threat.x,threat.z,nx,nz)){
    winner.peekX=nx;winner.peekZ=nz;break;
   }
  }
 }
 return winner;
}
function onThreat(actor,threat,severity,obstacles,blocked,lineHit){
 if(!actor||actor.hp<=0)return null;
 stats.threats++;
 actor.morale=clamp((actor.morale??1)-clamp(severity,0,3)*.16,.08,1);
 actor.reaction=Math.max(actor.reaction||0,Math.min(.55,.18+severity*.11));
 actor.suppression=Math.max(actor.suppression||0,.55+severity*.52);
 if(!Number.isFinite(threat?.x)||!Number.isFinite(threat?.z))return null;
 actor.lastX=threat.x;actor.lastZ=threat.z;
 actor.alert=Math.max(actor.alert||0,1.7);
 if((actor.coverRetry||0)>0&&actor.cover)return actor.cover;
 const next=chooseCover(actor,threat,obstacles,blocked,lineHit);
 if(next){actor.cover=next;actor.coverRetry=3.3;stats.coverFound++;}
 else actor.coverRetry=.8;
 return next;
}
function step(actor,dt,threat,blocked,lineHit){
 const d=clamp(dt,0,.1);
 actor.morale=clamp((actor.morale??1)+d*.033,0,1);
 actor.reaction=Math.max(0,(actor.reaction||0)-d);
 actor.coverRetry=Math.max(0,(actor.coverRetry||0)-d);
 if(!actor.cover||actor.hp<=0)return null;
 const c=actor.cover;c.age+=d;
 if(c.age>8.5||!threat||!Number.isFinite(threat.x)){
  actor.cover=null;return null;
 }
 const atCover=distance(actor.x,actor.z,c.x,c.z)<.85;
 // Leave the position after a fixed interval instead of indefinitely camping.
 if(!atCover){
  stats.retreats++;
  return {x:c.x,z:c.z,speed:actor.morale<.45?1.65:1.38,
   crouch:actor.suppression>1.2,holdFire:true,mode:"seek-cover"};
 }
 const cycle=(c.age%3.6),peeking=cycle>2.5&&cycle<3.25&&
  distance(c.peekX,c.peekZ,c.x,c.z)>.3;
 if(peeking){
  stats.peeks++;
  return {x:c.peekX,z:c.peekZ,speed:.85,crouch:false,holdFire:false,mode:"peek"};
 }
 // If the player flanks and can see the soldier from behind their cover,
 // abandon its now-obsolete protection and resume a normal fight.
 if(lineHit(threat.x,threat.z,actor.x,actor.z)&&c.age>2.0){
  actor.cover=null;return null;
 }
 return {x:c.x,z:c.z,speed:.15,crouch:true,holdFire:true,mode:"hunker"};
}
function reset(){for(const k of Object.keys(stats))stats[k]=0;}
root.WPBattleTactics=Object.freeze({chooseCover,onThreat,step,stats:()=>({...stats}),reset});
})(window);
