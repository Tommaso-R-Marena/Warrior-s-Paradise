/* Warrior's Paradise — real locally-hosted period-inspired vehicle meshes,
 * bounded drivable physics and enemy armor. No server, external runtime scripts,
 * multiplayer, or claim that every source is historically exact.
 */
(function(root){"use strict";
const defs=[
 {id:"jeep",name:"Willys MB 4×4",asset:"willys_mb",team:"ally",x:-41,z:-31,yaw:-.72,
  width:1.7,length:3.35,height:1.8,max:12,hp:120},
 {id:"armor",name:"M4 Sherman (75 mm)",asset:"m4_sherman_shape",team:"ally",x:-40,z:-23,yaw:-1.04,
  width:3.0,length:5.6,height:2.5,max:6.1,hp:320},
 {id:"enemy",name:"Panzer IV (H)",asset:"panzer_iv_shape",team:"axis",x:34,z:16,yaw:-1.9,
  width:3.0,length:5.6,height:2.5,max:0,hp:300}
];
const records=[],models={},status={ready:false,loaded:0,errors:{},trianglesLast:0};
let occupied=null,fired=0,hit=0,collisionRejected=0,collisionSamples=0;
const shells=[],SHELL_SPEED=105,MAX_RANGE=58;
const clamp=(n,a,b)=>Math.max(a,Math.min(b,n));
const angleDelta=(a,b)=>Math.atan2(Math.sin(a-b),Math.cos(a-b));
const distance=(a,b)=>Math.hypot(a.x-b.x,a.z-b.z);
function reset(){
 records.length=0;for(const d of defs)records.push({...d,speed:0,reload:0,shotFlash:0,alive:true,
  groundY:0,turretYaw:d.yaw,ammo:{ap:16,he:8},shellType:"ap",lastImpact:null});
 occupied=null;fired=0;hit=0;collisionRejected=0;collisionSamples=0;status.trianglesLast=0;shells.length=0;
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
function mount(player,id="armor"){
 const v=records.find(a=>a.id===id&&a.team==="ally"&&a.alive);
 if(!v||occupied)return false;
 occupied=v.id;player.x=v.x;player.z=v.z;player.yaw=v.turretYaw;
 return true;
}
function selectShell(type){
 const v=current();
 if(!v||v.id!=="armor")return false;
 v.shellType=type==="he"||type==="ap"?type:(v.shellType==="ap"?"he":"ap");
 return v.shellType;
}
function current(){return records.find(r=>r.id===occupied)||null}
function nearest(player){
 let best=null,d=Infinity;
 for(const v of records)if(v.alive&&v.team==="ally"){
  const away=distance(v,player);if(away<d){best=v;d=away;}
 }
 return best?{id:best.id,name:best.name,distance:d}:null;
}
function near(player){
 const next=nearest(player);
 return next&&next.distance<4.6?records.find(v=>v.id===next.id):null;
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
 return mount(player,v.id);
}
// Projected SAT axes avoid treating every long hull as a giant circular wall.
function overlappingHull(a,x,z,yaw,b){
 const axes=[
  [Math.cos(yaw),Math.sin(yaw)],[-Math.sin(yaw),Math.cos(yaw)],
  [Math.cos(b.yaw),Math.sin(b.yaw)],[-Math.sin(b.yaw),Math.cos(b.yaw)]
 ];
 const diffX=b.x-x,diffZ=b.z-z;
 for(const [ux,uz] of axes){
  const projected=Math.abs(diffX*ux+diffZ*uz);
  const ar=Math.abs(ux*Math.cos(yaw)+uz*Math.sin(yaw))*a.width*.5+
   Math.abs(-ux*Math.sin(yaw)+uz*Math.cos(yaw))*a.length*.5;
  const br=Math.abs(ux*Math.cos(b.yaw)+uz*Math.sin(b.yaw))*b.width*.5+
   Math.abs(-ux*Math.sin(b.yaw)+uz*Math.cos(b.yaw))*b.length*.5;
  if(projected>ar+br+.14)return false;
 }
 return true;
}
function footprintClear(v,x,z,yaw,blocked){
 const c=Math.cos(yaw),s=Math.sin(yaw),right=[c,s],forward=[-s,c];
 const edgeWidth=v.width*.46,edgeLength=v.length*.46;
 const divisions=Math.max(2,Math.ceil(v.length/.82));
 const radius=.14;
 for(let i=0;i<=divisions;i++){
  const along=-edgeLength+2*edgeLength*i/divisions;
  for(const across of [-edgeWidth,0,edgeWidth]){
   const px=x+right[0]*across+forward[0]*along;
   const pz=z+right[1]*across+forward[1]*along;
   collisionSamples++;
   if(blocked(px,pz,radius))return false;
  }
 }
 return !records.some(other=>other!==v&&other.alive&&overlappingHull(v,x,z,yaw,other));
}
function move(dt,input,player,blocked,traversable){
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
 // Continuous hull sweep: check physical width AND length while both moving
 // and steering. Prevents tunnelling through thin fences or meter-scale rocks.
 // Tracked vehicles pivot in place; wheel-steered jeeps must be rolling.
 const turning= v.id==="armor"?(.22+.78*Math.min(1,Math.abs(v.speed)/2)):
  Math.min(1,Math.abs(v.speed)/2)*Math.sign(v.speed||1);
 const yawDelta=turn*dt*(v.id==="jeep"?.70:.42)*turning;
 const travel=v.speed*dt,steps=Math.max(1,Math.ceil(Math.max(Math.abs(travel)/.18,
  Math.abs(yawDelta)*v.length/.13)));
 const startX=v.x,startZ=v.z,startYaw=v.yaw;
 for(let i=0;i<steps;i++){
  const nextYaw=startYaw+yawDelta*(i+1)/steps;
  const nx=v.x-Math.sin(nextYaw)*travel/steps;
  const nz=v.z+Math.cos(nextYaw)*travel/steps;
  if(!footprintClear(v,nx,nz,nextYaw,blocked)||
     (traversable&&!traversable(v.x,v.z,nx,nz,Math.max(v.width*.48,.65)))){
   collisionRejected++;v.speed=0;break;
  }
  v.x=nx;v.z=nz;v.yaw=nextYaw;
 }
 player.x=v.x;player.z=v.z;
 if(v.id==="armor")v.turretYaw=player.yaw;
 return true;
}
// Shell trace uses a small 3D projectile arc, checked along the entire path.
// It resolves the earliest building, heightfield, rock, infantry or armor impact.
function traceShell(v,player,enemies,world){
 const heading=Number.isFinite(player.yaw)?player.yaw:v.turretYaw;
 const pitch=clamp(Number.isFinite(player.pitch)?player.pitch:0,-.36,.36);
 const dx=-Math.sin(heading),dz=Math.cos(heading);
 const x0=v.x+dx*2.55,z0=v.z+dz*2.55,y0=(v.groundY||0)+2.32;
 const invCos=1/Math.max(.25,Math.cos(pitch)**2);
 const enemyArmor=records.find(a=>a.id==="enemy"&&a.alive);
 const obstacles=world?.obstacles||[];
 const terrain=world?.heightAt||(()=>-100);
 const max=MAX_RANGE;
 for(let t=.35;t<=max;t+=.38){
  const x=x0+dx*t,z=z0+dz*t;
  const y=y0+Math.tan(pitch)*t-4.905*t*t*invCos/(SHELL_SPEED*SHELL_SPEED);
  if(world){
   if(Math.abs(x)>46.5||Math.abs(z)>46.5)return {x,z,y,type:"boundary",distance:t};
   if(y<=terrain(x,z)+.065)return {x,z,y,type:"terrain",distance:t};
   for(const b of obstacles){
    const [bx,bz,w,d,h=1.1,base=terrain(bx,bz)]=b;
    if(Math.abs(x-bx)<=w*.5&&Math.abs(z-bz)<=d*.5&&y>=base&&y<=base+h)
     return {x,z,y,type:"cover",distance:t};
   }
  }
  if(enemyArmor&&Math.abs(x-enemyArmor.x)<enemyArmor.width*.48&&
      Math.abs(z-enemyArmor.z)<enemyArmor.length*.47&&
      y>(enemyArmor.groundY||0)+.25&&y<(enemyArmor.groundY||0)+2.95)
    return {x,z,y,type:"armor",target:enemyArmor,distance:t};
  for(const e of enemies){
   if(e.hp<=0)continue;
   const ground=Number.isFinite(e.groundY)?e.groundY:world?.heightAt?.(e.x,e.z)||0;
   if(Math.hypot(x-e.x,z-e.z)<.50&&y>=ground+.12&&y<=ground+2.55)
    return {x,z,y,type:"infantry",target:e,distance:t};
  }
 }
 return {x:x0+dx*max,z:z0+dz*max,
  y:y0+Math.tan(pitch)*max-4.905*max*max*invCos/(SHELL_SPEED*SHELL_SPEED),
  type:"miss",distance:max};
}
function fire(player,enemies,visible,onHit,onTracer,onBlast,world){
 const v=current();
 if(!v||v.id!=="armor"||!v.alive||v.reload>0)return false;
 const type=v.shellType||"ap";
 if(v.ammo[type]<=0)return false;
 v.ammo[type]--;v.reload=type==="he"?4.1:3.35;
 v.shotFlash=.23;v.turretYaw=player.yaw;fired++;
 const direction={dx:-Math.sin(player.yaw),dz:Math.cos(player.yaw)};
 let impact;
 if(world)impact=traceShell(v,player,enemies,world);
 else{
  // Compatibility for isolated legacy unit tests; gameplay always supplies world.
  let victim=null,near=42;
  for(const e of enemies){
   if(e.hp<=0)continue;
   const rx=e.x-v.x,rz=e.z-v.z,t=rx*direction.dx+rz*direction.dz;
   if(t<3||t>near||Math.abs(rx*direction.dz-rz*direction.dx)>1.3||
      !visible(v.x,v.z,e.x,e.z))continue;
   victim=e;near=t;
  }
  impact=victim?{x:victim.x,z:victim.z,y:1.4,type:"infantry",target:victim}:
   {x:v.x+direction.dx*38,z:v.z+direction.dz*38,y:1.4,type:"miss"};
 }
 // AP focuses on a direct impact. HE has a radius, reduced at range.
 const injured=new Set();
 const wound=(enemy,damage)=>{
  if(enemy.hp<=0||damage<=0)return;
  enemy.hp-=damage;enemy.hit=.55;enemy.suppression=2;
  if(!injured.has(enemy)){injured.add(enemy);hit++;onHit?.(enemy);}
 };
 if(impact.type==="armor"&&impact.target){
  const armor=impact.target;
  const incoming=player.yaw;
  const alignment=Math.cos(angleDelta(incoming,armor.yaw));
  const damage=type==="ap"?(alignment>.48?170:alignment<-.48?92:126):39;
  armor.hp=Math.max(0,armor.hp-damage);
  if(armor.hp===0){armor.alive=false;armor.speed=0;}
  hit++;
 }
 if(impact.type==="infantry"&&impact.target)
  wound(impact.target,type==="ap"?165:110);
 if(type==="he"&&impact.type!=="miss"){
  for(const e of enemies){
   if(e.hp<=0||injured.has(e))continue;
   const dist=Math.hypot(e.x-impact.x,e.z-impact.z);
   if(dist>4.4)continue;
   const impactHeight=world?.heightAt?.(impact.x,impact.z)||0;
   const targetHeight=Number.isFinite(e.groundY)?e.groundY:0;
   if(Math.abs(targetHeight-impactHeight)>3.8)continue;
   if(world&&visible&&!visible(impact.x,impact.z,e.x,e.z))continue;
   wound(e,Math.round(105*(1-dist/5)));
  }
 }
 v.lastImpact={type:impact.type,x:impact.x,z:impact.z,y:impact.y,
  shell:type,armorHp:records.find(a=>a.id==="enemy")?.hp||0};
 const muzzle={x:v.x+direction.dx*2.55,z:v.z+direction.dz*2.55,y:(v.groundY||0)+2.32};
 shells.push({a:muzzle,b:{x:impact.x,z:impact.z,y:impact.y},
  age:0,life:.48,type});
 if(shells.length>12)shells.shift();
 onTracer?.(muzzle.x,muzzle.z,impact.x,impact.z,"friendly",muzzle.y,impact.y);
 if(impact.type!=="miss")onBlast?.(impact.x,impact.z,type==="he"?"artillery":"frag");
 return true;
}
function tick(dt,player,hasLOS,onPlayerHit,onBlast,world){
 for(const v of records){
  v.reload=Math.max(0,v.reload-dt);v.shotFlash=Math.max(0,v.shotFlash-dt);
 }
 for(let i=shells.length-1;i>=0;i--){
  shells[i].age+=dt;
  if(shells[i].age>=shells[i].life)shells.splice(i,1);
 }
 const enemy=records.find(v=>v.id==="enemy");
 if(!enemy?.alive||!player||player.hp<=0)return;
 const d=distance(enemy,player);
 // Axis armor acquires and moves toward the player, but must respect physical
 // hull clearance and traversable slopes exactly like player armor does.
 enemy.turretYaw=Math.atan2(enemy.x-player.x,player.z-enemy.z);
 if(world?.blocked&&d>16&&d<57){
  const desired=enemy.turretYaw;
  const steer=clamp(angleDelta(desired,enemy.yaw),-.38*dt,.38*dt);
  const nyaw=enemy.yaw+steer;
  const speed=1.75,travel=Math.abs(angleDelta(desired,nyaw))>.30?0:Math.min(speed*dt,.16);
  const nx=enemy.x-Math.sin(nyaw)*travel,nz=enemy.z+Math.cos(nyaw)*travel;
  if(footprintClear(enemy,nx,nz,nyaw,world.blocked)&&
     (!world.traversable||world.traversable(enemy.x,enemy.z,nx,nz,enemy.width*.48))){
   enemy.x=nx;enemy.z=nz;enemy.yaw=nyaw;enemy.speed=speed;
  }else{
   enemy.speed=0;collisionRejected++;
   // Bounded avoidance steering with the same hull acceptance checks.
   for(const dir of [-1,1]){
    const turn=enemy.yaw+dir*.28*dt;
    if(footprintClear(enemy,enemy.x,enemy.z,turn,world.blocked)){
     enemy.yaw=turn;break;
    }
   }
  }
  if(world.heightAt)enemy.groundY=world.heightAt(enemy.x,enemy.z);
 }else enemy.speed=0;
 if(enemy.reload>0||d>27||d<7||!hasLOS(enemy.x,enemy.z,player.x,player.z))return;
 enemy.reload=5.1;enemy.shotFlash=.22;
 const damage=isDriving()&&current()?.id==="armor"?9:12;
 if(isDriving()&&current()){
  const target=current();target.hp-=damage;
  if(target.hp<=0){target.alive=false;occupied=null;player.hp-=35;}
 }else player.hp-=damage;
 onBlast?.(player.x+1.5,player.z-1,"frag");
 onPlayerHit?.(damage);
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
  // A bounded high-detail pass: suspension, track grousers, stowage, hatch
  // bosses, vision blocks, engine deck, tow hooks and hull weathering.
  for(const side of [-1,1]){
   for(let j=0;j<11;j++){
    const z=-2.40+j*.47;
    box(side*1.43,.82,z,.49,.12,.15,j%2?accent:metal);
    if(j%2===0)box(side*1.70,.24,z,.09,.15,.28,track);
   }
   for(let j=0;j<5;j++)box(side*1.37,.47,-1.80+j*.89,.08,.45,.56,metal);
   box(side*1.11,1.78,-1.50,.09,.15,1.08,metal);
   box(side*.88,1.51,1.67,.56,.06,.08,metal);
   box(side*1.0,1.18,-2.05,.19,.16,.26,accent);
   box(side*.66,1.72,-1.68,.33,.24,.43,accent);
  }
  for(let j=0;j<5;j++){
   box(-.66+j*.33,1.82,-1.44,.065,.05,.75,track);
   box(-.71+j*.35,1.54,1.33,.07,.04,.49,metal);
  }
  // Historical silhouette cues; not yet CAD-verified equipment dimensions.
  gunBox(.54,2.68,-.75,.10,.18,.20,track);
  gunBox(-.52,2.64,-.68,.11,.15,.31,accent);
  gunBox(0,2.86,-.84,.68,.09,.54,hull);
  gunBox(.58,2.64,.05,.18,.12,.28,metal);
  for(const side of [-1,1]){
   gunBox(side*.69,2.44,.20,.15,.13,.10,track);
   gunBox(side*.70,2.48,-.94,.18,.11,.14,accent);
  }
  box(0,1.32,-2.27,1.50,.13,.15,metal);
  box(-.47,1.69,-2.17,.18,.28,.19,track);
  box(.48,1.69,-2.17,.18,.28,.19,track);
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
   triangles+=drawArmor(g,v,quality,
    v.id==="armor"&&occupied==="armor"?player.yaw:v.turretYaw);
  }else{
   // Functional always-present silhouette for phone and absent imported art.
   const c=v.team==="axis"?[.34,.37,.30]:[.30,.36,.27];
   g.box(v.x,1.0+(v.groundY||0),v.z,v.width,1.0,v.length,c,-v.yaw);
   g.box(v.x,1.65+(v.groundY||0),v.z-.45,v.width*.77,.60,v.length*.52,c,-v.yaw);
   if(v.id!=="jeep"){
    const tyaw=v.id==="armor"&&occupied==="armor"?player.yaw:v.turretYaw;
    const hx=-Math.sin(tyaw),hz=Math.cos(tyaw);
    g.box(v.x,2.13+(v.groundY||0),v.z,.95,.45,1.40,[.25,.30,.24],-tyaw);
    g.box(v.x+hx*1.65,2.16+(v.groundY||0),v.z+hz*1.65,
     .19,.17,2.65,[.15,.17,.15],-tyaw);
   }else{
    g.box(v.x,1.72+(v.groundY||0),v.z-.28,v.width*.93,.08,.12,[.19,.19,.17],v.yaw);
   }
   triangles+=72;
  }
  if(v.id==="jeep"&&quality==="high"&&dist<36){
   // High-specific jeep accessories sit on both imported and fallback bodies.
   const yaw=-v.yaw,cs=Math.cos(yaw),sn=Math.sin(yaw),ground=v.groundY||0;
   const accessory=(x,y,z,w,h,d,col)=>{
    g.box(v.x+cs*x+sn*z,ground+y,v.z-sn*x+cs*z,w,h,d,col,yaw);
    triangles+=12;
   };
   const frame=[.18,.20,.17],canvas=[.34,.33,.26],rubber=[.12,.13,.12];
   // Windshield surround, spare tyre, headlights, folded canvas and grille.
   for(const side of [-1,1]){
    accessory(side*.69,1.62,.39,.065,.73,.055,frame);
    accessory(side*.55,1.03,1.55,.21,.19,.19,[.58,.56,.40]);
    accessory(side*.64,.54,-.96,.38,.54,.18,rubber);
   }
   accessory(0,1.99,.39,1.43,.065,.06,frame);
   accessory(0,1.47,-1.25,1.30,.18,.34,canvas);
   for(let j=0;j<7;j++)accessory(-.45+j*.15,.92,1.59,.046,.25,.045,frame);
   accessory(0,1.37,-1.73,.52,.52,.11,rubber);
   accessory(.01,1.45,-1.80,.19,.19,.13,frame);
  }
  if(v.shotFlash>0){
   const yaw=v.turretYaw||v.yaw;
   const mx=v.x-Math.sin(yaw)*3.15,mz=v.z+Math.cos(yaw)*3.15;
   g.cone(mx,2.32+(v.groundY||0),mz,.23,.67,[.98,.71,.27],7);
   triangles+=7;
  }
  // Engine exhaust and displaced dust respond to actual hull movement.
  if(Math.abs(v.speed)>.4&&quality!=="low"&&dist<30){
   const phase=Math.sin(time*17+v.x*.7);
   const yaw=v.yaw;
   const x=v.x+Math.sin(yaw)*(v.length*.44),z=v.z-Math.cos(yaw)*(v.length*.44);
   g.cone(x,(v.groundY||0)+.11,z,.19+Math.abs(phase)*.08,.35,
    [.43,.41,.35],5);triangles+=5;
  }
 }
 // Bounded shell streaks show which direction the vehicle actually fired.
 for(const s of shells){
  if(Math.hypot(s.a.x-player.x,s.a.z-player.z)>65)continue;
  const t=clamp(s.age/s.life,0,1),head=.14+.86*t;
  const p=[s.a.x+(s.b.x-s.a.x)*head,
   s.a.y+(s.b.y-s.a.y)*head,s.a.z+(s.b.z-s.a.z)*head];
  const tail=[s.a.x+(s.b.x-s.a.x)*Math.max(0,head-.09),
   s.a.y+(s.b.y-s.a.y)*Math.max(0,head-.09),
   s.a.z+(s.b.z-s.a.z)*Math.max(0,head-.09)];
  g.quad(tail,p,[p[0],p[1]+.065,p[2]],
   [tail[0],tail[1]+.065,tail[2]],s.type==="he"?[.99,.58,.23]:[.96,.86,.52]);
  triangles+=2;
 }
 status.trianglesLast=triangles;
 return triangles;
}
function setGround(height){
 for(const v of records)v.groundY=height(v.x,v.z);
}
function info(){return {ready:status.ready,loaded:status.loaded,errors:{...status.errors},
 driving:occupied,vehicle:current()?.name||"",enemyAlive:records.find(v=>v.id==="enemy")?.alive,
 vehicles:records.map(v=>({id:v.id,name:v.name,x:v.x,z:v.z,yaw:v.yaw,turretYaw:v.turretYaw,
 hp:v.hp,speed:v.speed,alive:v.alive,reload:v.reload,ammo:{...v.ammo},shellType:v.shellType,
 lastImpact:v.lastImpact})),
 shots:fired,hits:hit,activeShells:shells.length,collisionRejected,collisionSamples,
 renderedTriangles:status.trianglesLast};}
root.WPArmoredFleet=Object.freeze({reset,load,toggle,mount,selectShell,nearest,isDriving,current,near,move,fire,tick,draw,info,setGround,footprintClear,overlappingHull,traceShell});
})(window);
