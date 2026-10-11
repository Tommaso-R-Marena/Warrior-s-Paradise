"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=path.resolve(__dirname,"../../browser/3d"),win={};
vm.runInNewContext(fs.readFileSync(path.join(root,"battle-tactics.js"),"utf8"),{window:win,Math,Number});
vm.runInNewContext(fs.readFileSync(path.join(root,"weapon-handling.js"),"utf8"),{window:win,Math});
const tactics=win.WPBattleTactics,weapon=win.WPWeaponHandling;
assert(tactics&&weapon);
const box=[0,7,3,3];
const blocked=(x,z,r=0)=>Math.abs(x-box[0])<box[2]/2+r&&Math.abs(z-box[1])<box[3]/2+r;
const los=(ax,az,bx,bz)=>{
 for(let i=1;i<42;i++){const t=i/42;
  if(blocked(ax+(bx-ax)*t,az+(bz-az)*t,.01))return false;}
 return true;
};
const guard={x:0,z:11,hp:80,alert:0,suppression:0,morale:1,strafe:1};
const attacker={x:0,z:0};
const wallCover={covers:[],buildings:[box]};
const spot=tactics.chooseCover(guard,attacker,wallCover,blocked,los);
assert(spot&&spot.z>8.5,"Tactics must find an actual sheltered position behind the obstacle");
assert.equal(los(attacker.x,attacker.z,spot.x,spot.z),false);
const assigned=tactics.onThreat(guard,attacker,1.8,wallCover,blocked,los);
assert(assigned&&guard.cover&&guard.reaction>0&&guard.morale<1,
 "Being hit must affect the AI and assign safe cover");
let choice=tactics.step(guard,1/60,attacker,blocked,los);
assert(choice&&choice.mode==="seek-cover"&&choice.holdFire);
guard.x=spot.x;guard.z=spot.z;
choice=tactics.step(guard,.11,attacker,blocked,los);
assert(choice&&choice.mode==="hunker"&&choice.crouch&&choice.holdFire);
let peek=false;
for(let i=0;i<66;i++){
 choice=tactics.step(guard,.06,attacker,blocked,los);
 if(choice?.mode==="peek"){peek=true;break;}
}
assert(peek,"Suppressed guard must periodically consider exposing himself to retaliate");
for(let i=0;i<160;i++)tactics.step(guard,.05,attacker,blocked,los);
assert.equal(guard.cover,null,"AI cover state must expire and permit later repositioning");
const noCover={x:0,z:7,hp:80,morale:1};
assert.equal(tactics.onThreat(noCover,attacker,1,{buildings:[],covers:[]},()=>false,()=>true),null);
assert(noCover.morale<1,"Uncovered soldier must still have realistic morale and flinch");
assert.equal(tactics.chooseCover(guard,attacker,{buildings:[[0,7,3,3]]},()=>true,los),null);
assert(tactics.stats().coverFound>=1&&tactics.stats().peeks>=1);
tactics.reset();assert.equal(tactics.stats().coverFound,0);
for(const type of ["rifle","smg","lmg"]){
 const player={yaw:0,pitch:0,aim:true,crouch:true,recoilReturn:0,recoilBloom:0};
 weapon.fire(player,type,.5);
 assert(player.pitch>0&&player.recoilBloom>0&&player.recoilReturn>0);
 assert(weapon.spreadMultiplier(player)>1);
 const kick=player.pitch;
 for(let i=0;i<150;i++)weapon.tick(player,1/60);
 assert(player.pitch<kick*.02&&player.recoilReturn<.0005,
  "Weapon "+type+" must smoothly recover from a shot instead of indefinitely drifting");
 assert(player.recoilBloom===0,"Recoil inaccuracy should reset after a pause");
 weapon.fire(player,type,0);
 weapon.fire(player,type,0);
 assert(player.recoilBloom>weapon.profiles[type].bloom,
  "Sustained automatic fire should progressively reduce accuracy");
}
const desktop=win.WPWeaponHandling.spreadMultiplier({recoilBloom:0,concussion:0});
assert.equal(desktop,1);
console.log("PASS believable firefights: geometric shelter, hunker/peek/relocate, cover cap, morale and recoil recovery");
