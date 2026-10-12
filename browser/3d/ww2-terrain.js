/* One authoritative triangulated Normandy heightfield for rendering and movement.
 * LOW/MEDIUM sample a 19×19-grid triangulation; HIGH subdivides precisely
 * those same triangles, so visual geometry and ground collision coincide.
 * No physics middleware, atlas download or additional render pass.
 */
(function(root){"use strict";
const HALF=47,BASE=19,HIGH=38,BASE_Y=-.071;
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
const smooth=t=>{t=clamp(t,0,1);return t*t*(3-2*t);};
function hash(i,j){
 let n=Math.imul(i,374761393)^Math.imul(j,668265263)^189797;
 n=Math.imul(n^(n>>>13),1274126177);return((n^(n>>>16))>>>0)/4294967295;
}
function noise(x,z){
 const i=Math.floor(x),j=Math.floor(z),a=smooth(x-i),b=smooth(z-j);
 return (1-b)*((1-a)*hash(i,j)+a*hash(i+1,j))+
  b*((1-a)*hash(i,j+1)+a*hash(i+1,j+1));
}
const farms=[[-37,30,10,10],[-35,-5,5,5],[41,40,9,9],[13,-38,5,5]];
const vehicles=[[-41,-31,5,5],[-40,-23,7,8],[-36,-33,7,7]];
function flatten(x,z,flags,buildings){
 const road=1-smooth((Math.abs(x-z)-2.1)/8.4);
 let masked=road;
 for(const [cx,cz,w,d] of [...buildings,...farms,...vehicles]){
  const bx=Math.max(0,Math.abs(x-cx)-w*.5),bz=Math.max(0,Math.abs(z-cz)-d*.5);
  masked=Math.max(masked,1-smooth((Math.hypot(bx,bz)-.8)/5.2));
 }
 for(const f of flags)masked=Math.max(masked,1-smooth((Math.hypot(x-f.x,z-f.z)-5.0)/8.0));
 return clamp(masked,0,1);
}
function raw(x,z,flags,buildings){
 const broad=noise(x*.046+8,z*.045-6),local=noise(x*.10+4,z*.10-2);
 const waves=Math.sin(x*.083+z*.022)*Math.cos(z*.076-x*.019);
 const ridge=3.05*Math.exp(-Math.pow((x-19)/4.1,2)-Math.pow((z+34)/11,2));
 const escarpment=4.5*Math.exp(-Math.pow((x+22)/5.6,2)-Math.pow((z-29)/11,2));
 const landscape=1.72*(broad-.5)+.65*waves+.42*(local-.5)+ridge+escarpment;
 return BASE_Y+(1-flatten(x,z,flags,buildings))*landscape;
}
let cachedFlags=null,cachedBuildings=null,cachedSamples=null;
function node(ix,iz,flags,buildings){
 if(cachedFlags!==flags||cachedBuildings!==buildings||!cachedSamples){
  const nodes=new Float64Array((BASE+1)*(BASE+1)),step=2*HALF/BASE;
  for(let z=0;z<=BASE;z++)for(let x=0;x<=BASE;x++)
   nodes[z*(BASE+1)+x]=raw(-HALF+x*step,-HALF+z*step,flags,buildings);
  cachedFlags=flags;cachedBuildings=buildings;cachedSamples=nodes;
 }
 return cachedSamples[iz*(BASE+1)+ix];
}
function surface(x,z,flags=[],buildings=[]){
 if(!Number.isFinite(x)||!Number.isFinite(z))throw Error("Invalid heightfield coordinates");
 const scale=BASE/(2*HALF);
 const u=clamp((x+HALF)*scale,0,BASE),v=clamp((z+HALF)*scale,0,BASE);
 const ix=Math.min(BASE-1,Math.floor(u)),iz=Math.min(BASE-1,Math.floor(v));
 const a=u-ix,b=v-iz;
 const h00=node(ix,iz,flags,buildings),h01=node(ix,iz+1,flags,buildings),
  h10=node(ix+1,iz,flags,buildings),h11=node(ix+1,iz+1,flags,buildings);
 // Exactly match the two triangle diagonals emitted by append().
 return a<=b?h00+(h01-h00)*b+(h11-h01)*a:
  h00+(h10-h00)*a+(h11-h10)*b;
}
function slope(x,z,flags=[],buildings=[]){
 const d=.37,dx=(surface(x+d,z,flags,buildings)-surface(x-d,z,flags,buildings))/(2*d);
 const dz=(surface(x,z+d,flags,buildings)-surface(x,z-d,flags,buildings))/(2*d);
 return {dx,dz,grade:Math.hypot(dx,dz),normal:[-dx,1,-dz].map(t=>t/Math.hypot(dx,1,dz))};
}
function traversable(x,z,nx,nz,r,flags,buildings,kind="infantry"){
 const delta=Math.hypot(nx-x,nz-z);
 if(!Number.isFinite(delta)||delta>5)return false;
 const gradeLimit=kind==="vehicle"?.38:.73;
 const riseLimit=kind==="vehicle"?.19:.45;
 const steps=Math.max(2,Math.ceil(delta/.24));
 let previous=surface(x,z,flags,buildings);
 for(let i=1;i<=steps;i++){
  const u=i/steps,px=x+(nx-x)*u,pz=z+(nz-z)*u;
  const next=surface(px,pz,flags,buildings);
  if(Math.abs(next-previous)>riseLimit||slope(px,pz,flags,buildings).grade>gradeLimit)return false;
  previous=next;
 }
 // Weighted vehicle contact corners discourage tilting through steep ditches.
 if(kind==="vehicle"){
  const center=surface(nx,nz,flags,buildings);
  for(const sx of [-r,r])for(const sz of [-r,r]){
   if(Math.abs(surface(nx+sx,nz+sz,flags,buildings)-center)>r*.52)return false;
  }
 }
 return true;
}
function shadow(x,z,buildings){
 let occlusion=0;
 for(const [cx,cz,w,d] of buildings){
  const sx=x-cx,sz=z-cz,behind=-.60*sx+.80*sz,across=Math.abs(.80*sx+.60*sz);
  const extent=Math.max(w,d)*.5;
  if(behind>extent&&behind<extent+5&&across<extent+2)
   occlusion=Math.max(occlusion,(1-smooth((behind-extent)/5))*
    (1-smooth((across-extent)/2))*.27);
 }
 return occlusion;
}
function append(g,flags,buildings,level="high"){
 const high=level==="high",cells=high?HIGH:BASE,size=2*HALF/cells;
 let triangles=0,shadowed=0,minY=Infinity,maxY=-Infinity;
 for(let iz=0;iz<cells;iz++)for(let ix=0;ix<cells;ix++){
  const x=-HALF+ix*size,z=-HALF+iz*size,cx=x+size/2,cz=z+size/2;
  const moist=noise(cx*.19+10,cz*.19-1),shade=(.95-.25*shadow(cx,cz,buildings))+(moist-.5)*.12;
  if(shadow(cx,cz,buildings)>.025)shadowed++;
  const base=moist>.71?[.33,.32,.26]:moist<.27?[.25,.34,.22]:[.29,.38,.255];
  const color=base.map(v=>clamp(v*shade,.08,.78));
  // High mesh is displaced +0.008 m to avoid coplanar depth fighting with
  // the always-visible 19-cell coarse ground; that is below collision tolerance.
  const offset=high?.008:0;
  const h00=surface(x,z,flags,buildings)+offset,h01=surface(x,z+size,flags,buildings)+offset,
   h11=surface(x+size,z+size,flags,buildings)+offset,h10=surface(x+size,z,flags,buildings)+offset;
  minY=Math.min(minY,h00,h01,h10,h11);maxY=Math.max(maxY,h00,h01,h10,h11);
  g.tri([x,h00,z],[x,h01,z+size],[x+size,h11,z+size],color);
  g.tri([x,h00,z],[x+size,h11,z+size],[x+size,h10,z],color);
  triangles+=2;
 }
 return {triangles,minY,maxY,shadowedCells:shadowed,drawCallsAdded:0,
  physicalTriangles:2*BASE*BASE,renderCells:cells};
}
root.WPWW2Terrain=Object.freeze({surface,slope,traversable,append,shadow,
 maxTriangles:HIGH*HIGH*2,baseTriangles:BASE*BASE*2,extent:HALF});
})(window);
