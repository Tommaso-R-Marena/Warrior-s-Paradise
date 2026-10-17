/* Terrain-conformal, historically inspired field wear:
 * continuous cart road, shoulders, wagon ruts, finite wet depressions,
 * shell-darkened earth and bounded, moving tank tread imprints.
 * All effects use existing static/dynamic scene buffers. No textures, shader
 * passes, alpha blending, new collision or paid services are required.
 */
(function(root){"use strict";
const HALF=44,STEP=2.0,INV=Math.SQRT1_2;
const TRACK_CAP=90,tracks=[],positions=new Map;
let recorded=0,dropped=0;
const C={
 road:[.39,.362,.298],edge:[.325,.315,.256],rut:[.268,.249,.208],
 pebble:[.49,.455,.385],mud:[.20,.18,.155],damp:[.265,-.310,.336],
 damp2:[.214,-.270,.296],scorch:[.21,.196,.163],bank:[.35,.33,.265]
};
const finite=(x)=>Number.isFinite(x);
function heightpoint(height,x,z,offset=.035){
 return [x,height(x,z)+offset,z];
}
function strip(g,height,t0,t1,left,right,color,raise=.033){
 const point=(t,n)=>{
  const x=t+n*INV,z=t-n*INV;
  return heightpoint(height,x,z,raise);
 };
 const a=point(t0,left),b=point(t1,left),c=point(t1,right),d=point(t0,right);
 g.quad(a,b,c,d,color);
 return[a,b,c,d];
}
function road(g,height){
 let segments=0,triangles=0,shoulders=0;
 for(let t=-HALF;t<HALF-.001;t+=STEP){
  const end=Math.min(HALF,t+STEP);
  strip(g,height,t,end,-2.05,2.05,C.road,.043);triangles+=2;
  strip(g,height,t,end,-2.42,-2.05,C.edge,.036);
  strip(g,height,t,end,2.05,2.42,C.edge,.036);
  triangles+=4;shoulders+=2;segments++;
 }
 return {segments,triangles,shoulders};
}
function scar(g,height,x,z,r,facets=8){
 const core=heightpoint(height,x,z,.032);
 let triangles=0;
 for(let k=0;k<facets;k++){
  const a=k*Math.PI*2/facets,b=(k+1)*Math.PI*2/facets;
  const pa=heightpoint(height,x+Math.cos(a)*r,z+Math.sin(a)*r,.029);
  const pb=heightpoint(height,x+Math.cos(b)*r,z+Math.sin(b)*r,.029);
  g.tri(core,pa,pb,k%3===0?C.scorch:C.mud);triangles++;
 }return triangles;
}
function appendBase(g,height,blocked=()=>false){
 if(!g||typeof g.tri!=="function"||typeof g.quad!=="function"||typeof height!=="function")
  throw Error("Surface needs a triangle mesh and authoritative terrain");
 const path=road(g,height);let shellScars=0,triangles=path.triangles;
 let seed=0x19440606;
 const next=()=>{seed^=seed<<13;seed^=seed>>>17;seed^=seed<<5;return(seed>>>0)/4294967296;};
 for(let k=0;k<25;k++){
  const x=-41+next()*82,z=-41+next()*82,r=.45+next()*.72;
  if(Math.abs(x-z)<5||blocked(x,z,1.7))continue;
  triangles+=scar(g,height,x,z,r,8);shellScars++;
 }
 return {...path,shellScars,triangles};
}
function appendHigh(g,height,blocked=()=>false){
 if(!g||typeof g.tri!=="function"||typeof g.quad!=="function")throw Error("High terrain mesh required");
 let triangles=0,ruts=0,puddles=0,gravel=0;
 for(let t=-HALF;t<HALF-.001;t+=STEP){
  const end=Math.min(HALF,t+STEP);
  for(const n of [-1.20,1.20]){
   // Narrow ribbons mark the accumulated rut depth visually; the true
   // heightfield remains collision-authoritative at both quality levels.
   strip(g,height,t,end,n-.145,n+.145,C.rut,.051);
   ruts++;triangles+=2;
  }
  // Pale, shallow raised grit between the wheel tracks is also terrain-bound.
  if(Math.abs(t)%6<2.1){
   strip(g,height,t+.3,Math.min(end,t+1.3),-.18,.18,C.pebble,.050);
   gravel++;triangles+=2;
  }
 }
 // Small wet areas inside the wagon tracks, not a fictitious lake.
 // Signed green channel tells HIGH-only shader to use shallow-water glints.
 for(let k=0;k<13;k++){
  const t=-35+k*5.6,n=(k%2?1:-1)*1.20;
  const cx=t+n*INV,cz=t-n*INV,rx=.22,rz=.74;
  if(blocked(cx,cz,.22))continue;
  const mid=heightpoint(height,cx,cz,.058);
  for(let i=0;i<8;i++){
   const a=i*Math.PI/4,b=(i+1)*Math.PI/4;
   const e=heightpoint(height,cx+Math.cos(a)*rx+Math.sin(a)*rz*INV,
    cz+Math.cos(a)*rx-Math.sin(a)*rz*INV,.059);
   const f=heightpoint(height,cx+Math.cos(b)*rx+Math.sin(b)*rz*INV,
    cz+Math.cos(b)*rx-Math.sin(b)*rz*INV,.059);
   g.tri(mid,e,f,i%3===0?C.damp:C.damp2);triangles++;
  }puddles++;
 }
 return {ruts,puddles,gravel,triangles,maxTriangles:800};
}
function markTracks(vehicles,dt,height,quality){
 if(!finite(dt)||dt<=0||typeof height!=="function")return;
 // Lifetimes run on simulation dt, so screenshots and tests remain deterministic.
 for(let i=tracks.length-1;i>=0;i--){
  tracks[i].life-=Math.min(dt,.2);
  if(tracks[i].life<=0){tracks.splice(i,1);dropped++;}
 }
 if(quality!=="high")return;
 for(const v of vehicles||[]){
  if(!v||!v.alive||!finite(v.x)||!finite(v.z)||!finite(v.yaw)||
   !finite(v.width)||!finite(v.speed)||Math.abs(v.speed)<.55)continue;
  const last=positions.get(v.id);
  positions.set(v.id,{x:v.x,z:v.z,yaw:v.yaw});
  if(!last)continue;
  const travel=Math.hypot(v.x-last.x,v.z-last.z);
  // Refuse teleports, excessive stamps and stationary jitter.
  if(travel<.33||travel>3.8)continue;
  const segments=Math.min(5,Math.ceil(travel/.70));
  for(let k=0;k<segments;k++){
   const a=(k+.02)/segments,b=(k+.96)/segments;
   let x0=last.x+(v.x-last.x)*a,z0=last.z+(v.z-last.z)*a;
   let x1=last.x+(v.x-last.x)*b,z1=last.z+(v.z-last.z)*b;
   const tx=x1-x0,tz=z1-z0,inv=1/Math.max(.01,Math.hypot(tx,tz));
   const sideX=tz*inv,sideZ=-tx*inv;
   for(const side of [-1,1]){
    const offset=side*v.width*.43;
    const rect={x0:x0+sideX*offset,z0:z0+sideZ*offset,
     x1:x1+sideX*offset,z1:z1+sideZ*offset,
     acrossX:sideX,acrossZ:sideZ,life:24};
    // Do not accidentally leave the terrain extent.
    if([rect.x0,rect.z0,rect.x1,rect.z1].some(q=>Math.abs(q)>46))continue;
    if(tracks.length>=TRACK_CAP){tracks.shift();dropped++;}
    tracks.push(rect);recorded++;
   }
  }
 }
}
function drawTracks(g,quality,px,pz,height){
 if(quality!=="high"||!g||typeof g.quad!=="function")return 0;
 let triangles=0;
 for(const t of tracks){
  const cx=(t.x0+t.x1)*.5,cz=(t.z0+t.z1)*.5;
  if(Math.hypot(px-cx,pz-cz)>33)continue;
  const w=.16,sx=t.acrossX*w,sz=t.acrossZ*w;
  const vert=(x,z)=>heightpoint(height,x,z,.064);
  g.quad(vert(t.x0-sx,t.z0-sz),vert(t.x1-sx,t.z1-sz),
   vert(t.x1+sx,t.z1+sz),vert(t.x0+sx,t.z0+sz),
   t.life<4?C.edge:C.mud);
  triangles+=2;
 }
 return triangles;
}
function reset(){tracks.length=0;positions.clear();recorded=0;dropped=0;}
function stats(){return {marks:tracks.length,recorded,dropped,capacity:TRACK_CAP};}
root.WPBattlefieldGround=Object.freeze({appendBase,appendHigh,scar,markTracks,drawTracks,
 reset,stats,maxHighTriangles:800,trackCap:TRACK_CAP,roadHalf:HALF});
})(window);
