/* Warrior's Paradise — real locally-hosted period-inspired vehicle meshes,
 * bounded drivable physics and enemy armor. No server, external runtime scripts,
 * multiplayer, or claim that every source is historically exact.
 */
(function(root){"use strict";
const defs=[
 {id:"jeep",name:"Willys MB 4×4",asset:"willys_mb",team:"ally",x:-41,z:-31,yaw:1.0,
  width:1.7,length:3.35,height:1.8,max:12,hp:120},
 {id:"armor",name:"M4 Sherman (75 mm)",asset:"m4_sherman_shape",team:"ally",x:-40,z:-23,yaw:.5,
  width:3.0,length:5.6,height:2.5,max:6.1,hp:320},
 {id:"enemy",name:"Panzer IV (H)",asset:"panzer_iv_shape",team:"axis",x:34,z:16,yaw:-1.9,
  width:3.0,length:5.6,height:2.5,max:0,hp:300}
];
const records=[],models={},status={ready:false,loaded:0,errors:{},trianglesLast:0};
let occupied=null,fired=0,hit=0;
const distance=(a,b)=>Math.hypot(a.x-b.x,a.z-b.z);
function reset(){
 records.length=0;for(const d of defs)records.push({...d,speed:0,reload:0,shotFlash:0,alive:true,groundY:0});
 occupied=null;fired=0;hit=0;status.trianglesLast=0;
}
reset();
async function load(kit){
 if(!kit){status.ready=true;return;}
 const candidates=["willys_mb","gaz67_jeep","panzer_study"];
 await Promise.all(candidates.map(async name=>{
  try{
   const m=await kit.loadModels([{name,url:"./assets/oga/"+name+".glb.b64"}]);
   if(m[name]&&m[name].count<=6000){models[name]=m[name];status.loaded++;}
   else throw Error("excessive vehicle mesh complexity");
  }catch(e){status.errors[name]=String(e)}
 }));
 status.ready=true;
}
function isDriving(){return occupied!==null}
function current(){return records.find(r=>r.id===occupied)||null}
function near(player){
 let best=null,d=4.3;
 for(const v of records)if(v.alive&&v.team==="ally"){
  const away=distance(v,player);if(away<d){best=v;d=away;}
 }return best;
}
function toggle(player,blocked){
 if(occupied){
  const v=current();if(!v){occupied=null;return false;}
  const cos=Math.cos(v.yaw),sin=Math.sin(v.yaw);
  for(const side of [-1,1]){
   const ex=v.x+cos*(v.width*.5+1)*side,ez=v.z-sin*(v.width*.5+1)*side;
   if(!blocked(ex,ez,.43)){
    player.x=ex;player.z=ez;occupied=null;return true;
   }
  }
  return false; // No exit allowed through a wall.
 }
 const v=near(player);
 if(!v)return false;
 occupied=v.id;player.x=v.x;player.z=v.z;player.yaw=v.yaw;
 return true;
}
function move(dt,input,player,blocked){
 const v=current();if(!v||!v.alive)return false;
 const forward=((input.keys.KeyW||input.keys.ArrowUp)?1:0)-
  ((input.keys.KeyS||input.keys.ArrowDown)?1:0)-(input.joyY||0);
 const turn=((input.keys.KeyD||input.keys.ArrowRight)?1:0)-
  ((input.keys.KeyA||input.keys.ArrowLeft)?1:0)+(input.joyX||0);
 const accel=v.id==="jeep"?7.0:2.9;
 const target=Math.max(-v.max*.38,Math.min(v.max,forward*v.max));
 const change=Math.min(1,dt*(Math.abs(forward)>.1?accel/Math.max(1,v.max):1.6));
 v.speed+=(target-v.speed)*change;
 if(Math.abs(v.speed)<.035)v.speed=0;
 // Steering radius increases with speed; stationary vehicles cannot spin freely.
 v.yaw+=turn*dt*(v.id==="jeep"?.70:.32)*Math.min(1,Math.abs(v.speed)/2)*
  Math.sign(v.speed||1);
 const nx=v.x-Math.sin(v.yaw)*v.speed*dt,nz=v.z+Math.cos(v.yaw)*v.speed*dt;
 const footprint=Math.max(v.width*.48,.65);
 const other=records.some(o=>o!==v&&o.alive&&Math.hypot(o.x-nx,o.z-nz)<
  Math.max(v.width,o.width)*.72);
 if(!other&&!blocked(nx,nz,footprint)){v.x=nx;v.z=nz;}
 else v.speed=0;
 player.x=v.x;player.z=v.z;
 // Keep independent aiming direction for cannon turret/mouse camera.
 return true;
}
function fire(player,enemies,visible,onHit,onTracer,onBlast){
 const v=current();
 if(!v||v.id!=="armor"||!v.alive||v.reload>0)return false;
 v.reload=3.4;v.shotFlash=.16;fired++;
 let victim=null,near=42;
 const dx=-Math.sin(player.yaw),dz=Math.cos(player.yaw);
 for(const e of enemies){
  if(e.hp<=0)continue;
  const rx=e.x-v.x,rz=e.z-v.z,t=rx*dx+rz*dz;
  if(t<3||t>near||Math.abs(rx*dz-rz*dx)>1.3||!visible(v.x,v.z,e.x,e.z))continue;
  victim=e;near=t;
 }
 const impact=victim?{x:victim.x,z:victim.z}:{
  x:Math.max(-43,Math.min(43,v.x+dx*38)),
  z:Math.max(-43,Math.min(43,v.z+dz*38))};
 if(onTracer)onTracer(v.x,v.z,impact.x,impact.z,"friendly");
 if(onBlast)onBlast(impact.x,impact.z,"artillery");
 if(victim){victim.hp-=145;victim.hit=.5;victim.suppression=2;hit++;if(onHit)onHit(victim);}
 return true;
}
function tick(dt,player,hasLOS,onPlayerHit,onBlast){
 for(const v of records){
  v.reload=Math.max(0,v.reload-dt);v.shotFlash=Math.max(0,v.shotFlash-dt);
 }
 // Stationary scripted defensive tank: line-of-sight and distance constrained.
 const enemy=records.find(v=>v.id==="enemy");
 if(!enemy?.alive||enemy.reload>0||!player||player.hp<=0)return;
 const d=distance(enemy,player);
 if(d>23||d<7||!hasLOS(enemy.x,enemy.z,player.x,player.z))return;
 enemy.reload=5.1;
 const damage=isDriving()&&current()?.id==="armor"?5:10;
 if(isDriving()&&current()){
  const target=current();target.hp-=damage;
  if(target.hp<=0){target.alive=false;occupied=null;player.hp-=35;}
 }else player.hp-=damage;
 if(onBlast)onBlast(player.x+1.5,player.z-1,"frag");
 if(onPlayerHit)onPlayerHit(damage);
}
function drawArmor(g,v,quality,aimYaw){
 const high=quality==="high",med=quality==="medium";
 const allied=v.team==="ally";
 const hull=allied?[.31,.36,.25]:[.37,.38,.30],track=[.16,.17,.15],
  accent=allied?[.23,.27,.19]:[.31,.32,.27],metal=[.21,.23,.20];
 // Match physics: positive vehicle yaw moves toward negative world X.
 const yaw=-v.yaw,cs=Math.cos(yaw),sn=Math.sin(yaw);
 const pivotZ=v.team==="ally"?-.55:-.4;
 function gunBox(dx,y,dz,w,h,d,c){
  const cyaw=Number.isFinite(aimYaw)?-aimYaw:yaw;
  const cp=Math.cos(cyaw),sp=Math.sin(cyaw),rel=dz-pivotZ;
  const ox=v.x+sn*pivotZ,oz=v.z+cs*pivotZ;
  g.box(ox+cp*dx+sp*rel,y+(v.groundY||0),oz-sp*dx+cp*rel,w,h,d,c,cyaw);n+=12;
 }
 let n=0;
 function box(dx,y,dz,w,h,d,c=hull,angle=0){
  g.box(v.x+cs*dx+sn*dz,y+(v.groundY||0),v.z-sn*dx+cs*dz,w,h,d,c,yaw+angle);n+=12;
 }
 // Hull, mantlet, tracks and roadwheel arrangement follow distinct
 // Allied rounded Sherman versus German boxy Panzer IV silhouettes.
 box(0,.87,0,2.50,1.18,4.86,hull);
 box(0,1.50,.18,2.24,.57,3.68,hull);
 box(0,1.78,allied?-.30:.12,2.20,.32,2.65,accent);
 for(const side of [-1,1]){
  box(side*1.40,.55,0,.48,.95,5.22,track);
  box(side*1.40,.13,0,.57,.27,5.28,metal);
  if(med||high){
   const count=high?6:3;
   for(let j=0;j<count;j++){
    const z=-2.05+(j+.5)*(4.10/count);
    box(side*1.68,.52,z,.11,.55,.50,accent);
   }
  }
 }
 if(allied){
  // Cast rounded turret approximation, mantlet, M4 sloping glacis.
  gunBox(0,2.05,-.55,1.67,.55,1.92,hull);
  gunBox(0,2.29,-.50,1.35,.26,1.47,accent);
  box(0,1.31,2.20,2.1,.43,.53,accent);
  gunBox(0,2.68,-.98,.61,.27,.62,metal);
  gunBox(0,2.30,.58,.22,.13,3.02,track);
  gunBox(0,2.42,2.04,.35,.36,.38,metal);
  if(high){
   for(const side of [-1,1]){
    box(side*.75,1.81,-1.05,.15,.14,.84,accent);
    box(side*.75,1.57,2.07,.22,.21,.22,metal);
   }
  }
 }else{
  // Panzer IV flat-front turret and side skirt/Schürzen profile.
  gunBox(0,2.12,-.40,1.68,.82,1.77,hull);
  gunBox(0,2.60,-.41,1.60,.12,1.82,accent);
  gunBox(0,2.18,.66,.20,.13,2.87,track);
  gunBox(0,2.16,2.11,.34,.29,.39,metal);
  gunBox(0,2.77,-.95,.56,.25,.54,metal);
  if(med||high)for(const side of [-1,1]){
   box(side*1.61,1.08,-.05,.08,.62,3.87,hull);
   if(high)box(side*1.62,.75,0,.10,.10,3.95,track);
  }
 }
 if(high){
  // Distinct separate road-wheel and track track-shoe features.
  for(const side of [-1,1])for(let j=0;j<11;j++){
   box(side*1.43,.82,-2.40+j*.47,.49,.12,.15,accent);
  }
 }
 return n;
}
function draw(g,kit,quality,player,time){
 if(!g)return 0;
 let triangles=0,imported=0;
 for(const v of records){
  if(!v.alive)continue;
  const dist=distance(v,player);
  if(dist>70)continue;
  const y=0.02;const model=models[v.asset]||(v.id==="jeep"?models.gaz67_jeep:null);
  const rich=quality==="high"&&model&&dist<35&&
   imported<(quality==="high"?2:1)&&model.count<=2400;
  if(rich){
   const dim=Math.max(.001,model.max[0]-model.min[0]);
   const scale=v.width/dim;
   const midpointX=(model.min[0]+model.max[0])*.5*scale;
   const midpointZ=(model.min[2]+model.max[2])*.5*scale;
   const modelYaw=-v.yaw,cs=Math.cos(modelYaw),sn=Math.sin(modelYaw);
   const originX=v.x-midpointX*cs-midpointZ*sn;
   const originZ=v.z+midpointX*sn-midpointZ*cs;
   triangles+=kit.appendInstances(g,model,[[originX,y+(v.groundY||0),originZ,scale,modelYaw]]);
   imported++;
  }else if(v.id!=="jeep"&&quality!=="low"){
   triangles+=drawArmor(g,v,quality,v.id==="armor"&&occupied==="armor"?player.yaw:null);
  }else{
   // Functional always-present silhouette for phone and absent imported art.
   const c=v.team==="axis"?[.34,.37,.30]:[.30,.36,.27];
   g.box(v.x,1.0+(v.groundY||0),v.z,v.width,1.0,v.length,c,-v.yaw);
   g.box(v.x,1.65+(v.groundY||0),v.z-.45,v.width*.77,.60,v.length*.52,c,-v.yaw);
   if(v.id!=="jeep"){
    g.box(v.x,2.13+(v.groundY||0),v.z,.95,.45,1.40,[.25,.30,.24],v.yaw);
    g.box(v.x-Math.sin(v.yaw)*1.6,2.07+(v.groundY||0),v.z+Math.cos(v.yaw)*1.6,.14,.14,2.3,[.15,.17,.15],-v.yaw);
   }else{
    g.box(v.x,1.72+(v.groundY||0),v.z-.28,v.width*.93,.08,.12,[.19,.19,.17],v.yaw);
   }
   triangles+=72;
  }
  if(v.shotFlash>0)g.cone(v.x,2.5+(v.groundY||0),v.z,0.16,.55,[.96,.66,.22],5);
 }
 status.trianglesLast=triangles;
 return triangles;
}
function setGround(height){
 for(const v of records)v.groundY=height(v.x,v.z);
}
function info(){return {ready:status.ready,loaded:status.loaded,errors:{...status.errors},
 driving:occupied,vehicle:current()?.name||"",enemyAlive:records.find(v=>v.id==="enemy")?.alive,
 vehicles:records.map(v=>({id:v.id,name:v.name,x:v.x,z:v.z,hp:v.hp,speed:v.speed,alive:v.alive})),
 shots:fired,hits:hit,renderedTriangles:status.trianglesLast};}
root.WPArmoredFleet=Object.freeze({reset,load,toggle,isDriving,current,near,move,fire,tick,draw,info,setGround});
})(window);
