/* Warrior's Paradise — procedural WWII infantry/viewmodel presentation.
 * No external asset dependencies. Animates hierarchical-looking body segments
 * from simulation state; not a glTF skeleton or photogrammetric character.
 * All authored mesh geometry is original and belongs to Warrior's Paradise.
 */
(function(root){"use strict";
const TEAM={
 ally:{coat:[.29,.32,.22],trousers:[.27,.28,.21],webbing:[.45,.42,.30],helmet:[.26,.30,.23]},
 axis:{coat:[.32,.33,.28],trousers:[.28,.29,.26],webbing:[.31,.28,.22],helmet:[.26,.28,.25]}
};
const SKIN=[.70,.56,.43],STEEL=[.19,.22,.21],OAK=[.36,.26,.18],
 BOOT=[.17,.16,.13],DARK=[.10,.115,.105],CANVAS=[.40,.38,.29],METAL=[.27,.30,.29];
function soldier(g,actor,team,t,distMoved=0){
 const colors=TEAM[team]||TEAM.axis;
 const {x,z}=actor;
 const target=actor.facing===undefined?0:actor.facing;
 const yaw=target;
 const s=Math.sin(yaw),c=Math.cos(yaw);
 const part=(px,py,pz,w,h,d,col,rotation=0)=>{
  const X=x+px*c+pz*s,Z=z-px*s+pz*c;
  g.box(X,py,Z,w,h,d,col,yaw+rotation);
 };
 const stride=Math.min(1,Math.max(0,distMoved*1.8));
 const cadence=t*9+(actor.phase||0);
 const gait=Math.sin(cadence)*.27*stride;
 const crouched=!!actor.crouch||(actor.suppression||0)>1.0;
 const duck=crouched?.42:0;
 const hitFlinch=Math.min(1,(actor.reaction||0)*2.4);
 const damaged=(actor.hit||0)>.05,coat=damaged?[.39,.30,.25]:colors.coat;
 const bob=Math.sin(cadence*2)*.027*stride-hitFlinch*.11;
 // Layered trousers, boots, calf gaiters, soldier tunic and belt.
 for(const side of [-1,1]){
  const fore=side*gait;
  part(side*.18,.52-duck*.52+Math.max(0,fore)*.07,fore*.42,
   .27,.82,.32,colors.trousers,fore*.4);
  part(side*.18,.19-duck*.25,fore*.6+.08,.29,.35,.37,BOOT);
  part(side*.18,.78-duck*.50,0,.28,.17,.32,colors.webbing);
 }
 part(0,1.35+bob-duck,0,.67,1.03,.39,coat);
 part(0,.93-duck,0,.72,.15,.44,colors.webbing);
 part(0,1.59-duck,.21,.62,.08,.08,colors.webbing);
 // Ammunition pouches, canteen, soldier backpack and shovel.
 for(const side of [-1,1]){
  part(side*.26,.96-duck,.28,.18,.26,.14,CANVAS);
  part(side*.21,1.26-duck,-.34,.19,.36,.17,colors.webbing);
 }
 part(0,1.34-duck,-.34,.55,.65,.20,CANVAS);
 part(-.28,1.13-duck,-.48,.13,.66,.11,STEEL);
 part(.31,1.02-duck,-.34,.17,.30,.17,CANVAS);
 // Neck, collar and head.
 part(0,1.90-duck,0,.19,.21,.19,SKIN);
 part(0,2.13-duck,.055,.42,.43,.42,SKIN);
 part(0,2.30-duck,.065,.46,.10,.46,BOOT);
 part(0,2.41-duck,0,.65,.22,.58,colors.helmet);
 part(0,2.32-duck,.20,.60,.085,.30,colors.helmet);
 part(0,2.08-duck,.285,.32,.10,.035,[.43,.34,.27]);
 part(-.12,2.18-duck,.277,.043,.034,.025,DARK);
 part(.12,2.18-duck,.277,.043,.034,.025,DARK);
 // Bent, animated arms in firing posture. Hands overlap weapon stock.
 const firing=actor.alert>0||team==="ally";
 for(const side of [-1,1]){
  const armY=firing?1.62:1.51;
  part(side*.49,armY+bob-duck,.11+(-gait*.15),.24,.48,.29,coat,side*.13);
  part(side*.40,1.43+bob-duck,.39,.23,.43,.23,coat,side*.14);
  part(side*.38,1.37+bob-duck,.59,.19,.18,.20,SKIN);
 }
 // Held long arm with barrel and wood furniture oriented in world space.
 part(.04,1.44+bob-duck,.55,.16,.15,.71,OAK);
 part(.04,1.47+bob-duck,.96,.10,.11,.56,STEEL);
 part(.04,1.51+bob-duck,1.24,.08,.08,.25,DARK);
 part(.04,1.28+bob-duck,.52,.12,.25,.20,STEEL);
 return {pose:crouched?"crouch":stride>.08?"walk":"aim"};
}
function fieldEquipment(g,actor,team){
 // Overlay period-inspired kit on the real deforming character, never another full body.
 const colors=TEAM[team]||TEAM.axis;
 const yaw=actor.facing||0,c=Math.cos(yaw),s=Math.sin(yaw);
 const part=(px,py,pz,w,h,d,col)=>{
  g.box(actor.x+px*c+pz*s,py,actor.z-px*s+pz*c,w,h,d,col,yaw);
 };
 const duck=actor.crouch?.12:0;
 part(0,2.15-duck,.02,.64,.19,.60,colors.helmet);
 part(0,2.22-duck,.16,.69,.09,.41,colors.helmet);
 part(0,1.28-duck,-.25,.55,.59,.18,CANVAS);
 for(const side of [-1,1]){
  part(side*.25,1.05-duck,.24,.19,.28,.16,colors.webbing);
  part(side*.28,1.40-duck,-.28,.15,.23,.16,colors.webbing);
 }
 part(0,1.52-duck,.22,.66,.08,.08,colors.webbing);
 part(-.30,1.15-duck,-.37,.10,.61,.11,STEEL);
 part(.33,1.06-duck,-.26,.18,.25,.16,CANVAS);
 part(.05,1.39-duck,.65,.16,.13,.85,OAK);
 part(.05,1.42-duck,1.05,.10,.10,.48,STEEL);
}
function weapon(g,player,game){
 const F=[-Math.sin(player.yaw),Math.cos(player.yaw)],R=[-Math.cos(player.yaw),-Math.sin(player.yaw)];
 const recoil=Math.max((game.flash||0)>0?Math.min(.11,game.flash*.6):0,game.recoilVisual||0);
 const bob=Math.sin(player.stepPhase||0)*(player.stepStrength||0)*
  (player.aim?.004:.014);
 const reloadDuration=player.weapon==="rifle"?1.9:player.weapon==="smg"?1.6:2.7;
 const reloadPhase=player.reload>0?1-Math.min(1,player.reload/reloadDuration):0;
 const reloadDip=player.reload>0?Math.sin(Math.PI*reloadPhase)*.22:0;
 const center={x:player.x+F[0]*(.91-recoil)+R[0]*((player.aim?.036:.29)+reloadDip*.24),
  z:player.z+F[1]*(.91-recoil)+R[1]*((player.aim?.036:.29)+reloadDip*.24),
  y:(player.crouch?1.02:1.67)-(player.aim?.22:.40)+player.pitch*.15+bob-reloadDip};
 const X=n=>center.x+F[0]*n,Z=n=>center.z+F[1]*n;
 const yaw=-player.yaw,model=player.weapon;
 const box=(l,y,w,h,d,col)=>g.box(X(l),center.y+y,Z(l),w,h,d,col,yaw);
 const grip=(side,dy)=>g.box(center.x+R[0]*side,center.y+dy,
  center.z+R[1]*side,.17,.20,.28,SKIN,yaw);
 if(model==="rifle"){
  // Lee-Enfield No.4-inspired bolt rifle silhouette: wood forestock, receiver, bolt.
  box(-.39,-.07,.22,.18,.51,OAK);
  box(.02,0,.17,.15,.68,OAK);
  box(-.01,.075,.21,.15,.36,STEEL);
  box(.46,.02,.11,.10,.78,STEEL);
  box(.48,-.048,.16,.08,.35,OAK);
  box(.79,.10,.06,.12,.065,DARK);
  box(-.15,.135,.09,.08,.10,DARK);
  box(-.13,-.17,.16,.24,.24,STEEL);
  box(-.13,.15,.15,.05,.18,[.47,.46,.43]); // bolt action
  box(-.21,.11,.085,.09,.11,STEEL);
 }else if(model==="smg"){
  // Sten Mk II: tubular receiver, perforated jacket, sideways magazine.
  box(-.32,-.04,.16,.13,.36,STEEL);
  box(.11,.035,.17,.17,.52,STEEL);
  box(.46,.035,.12,.13,.66,STEEL);
  box(.78,.035,.10,.15,.15,DARK);
  box(-.38,-.12,.06,.08,.62,METAL); // skeleton stock
  box(-.60,-.13,.28,.23,.055,STEEL);
  g.box(center.x+R[0]*.24,center.y+.032,
   center.z+R[1]*.24,.47,.16,.15,STEEL,yaw); // side-fed magazine
  box(-.16,-.18,.13,.28,.16,DARK);
  box(.13,.12,.07,.13,.055,DARK);
 }else{
  // Bren Mk II: chunky receiver, top-mounted magazine and bipod.
  box(-.43,-.08,.28,.18,.60,OAK);
  box(.04,.03,.24,.23,.61,STEEL);
  box(.53,.025,.16,.15,.97,STEEL);
  box(.72,-.04,.21,.07,.36,OAK);
  box(.05,.34,.23,.36,.22,STEEL);
  box(.12,.52,.23,.045,.24,DARK);
  box(-.02,-.19,.15,.30,.22,DARK);
  box(.88,-.21,.05,.41,.045,STEEL);
  box(.96,-.21,.05,.41,.045,STEEL);
  box(.74,.16,.09,.10,.065,DARK);
 }
 grip(.20,-.18);grip(.35,-.28);
 g.box(center.x+R[0]*.37,center.y-.45,center.z+R[1]*.37,.29,.39,.31,colorsSleeve(player),yaw);
 if(game.flash>0)g.cone(X(1.04),center.y+.025,Z(1.04),
  .18+Math.sin(game.elapsed*92)*.05,.48,[.95,.74,.22],9);
}
function colorsSleeve(){return [.30,.33,.26];}
root.WPBattleArt=Object.freeze({soldier,weapon,fieldEquipment});
})(window);