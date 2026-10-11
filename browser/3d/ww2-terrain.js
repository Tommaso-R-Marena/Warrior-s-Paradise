/* Normandy 1944 ground: deterministic ground shading and restrained heightfield.
 * High quality only, shares existing static vertex-buffer draw call.
 * Roads, buildings, capture sites and friendly spawns stay flat for collision
 * fidelity; no claim of general mesh collision or dynamic shadow maps.
 */
(function(root){"use strict";
const CELLS=38,HALF=47,BUDGET=CELLS*CELLS*2;
function smooth(t){t=Math.min(1,Math.max(0,t));return t*t*(3-2*t)}
function hash(i,j){
 let n=Math.imul(i,374761393)^Math.imul(j,668265263)^189797;
 n=Math.imul(n^(n>>>13),1274126177);
 return ((n^(n>>>16))>>>0)/4294967295;
}
function noise(x,z){
 const i=Math.floor(x),k=Math.floor(z),fx=smooth(x-i),fz=smooth(z-k);
 return (1-fz)*((1-fx)*hash(i,k)+fx*hash(i+1,k))+
  fz*((1-fx)*hash(i,k+1)+fx*hash(i+1,k+1));
}
function surface(x,z,flags,buildings){
 // Flatten the diagonal farm track, building foundations and capture circles
 // to prevent visual collision discontinuity without an extra physics engine.
 let flatten=1-smooth((Math.abs(x-z)-2)/7);
 for(const [cx,cz,w,d] of buildings){
  const bx=Math.max(0,Math.abs(x-cx)-w*.5),bz=Math.max(0,Math.abs(z-cz)-d*.5);
  flatten=Math.max(flatten,1-smooth((Math.hypot(bx,bz)-1)/4));
 }
 for(const f of flags)flatten=Math.max(flatten,1-smooth((Math.hypot(x-f.x,z-f.z)-5)/7));
 const broad=noise(x*.055,z*.055),micro=noise(x*.23+7,z*.23-8);
 const amplitude=.135*(broad-.4)+.036*(micro-.5);
 return -.071+(1-flatten)*Math.max(-.015,amplitude);
}
function shadow(x,z,buildings){
 let occlusion=0;
 for(const [cx,cz,w,d] of buildings){
  // Height 4.3m / morning sun elevation gives a short fixed afternoon-facing
  // umbra/penu​mbra mask. A cheap baked approximation for static geometry.
  const sx=x-cx,sz=z-cz;
  const behind=(-.60*sx+.80*sz),across=Math.abs(.80*sx+.60*sz);
  const extent=Math.max(w,d)*.5;
  if(behind>extent&&behind<extent+5&&across<extent+2){
   occlusion=Math.max(occlusion,
    (1-smooth((behind-extent)/5))*(1-smooth((across-extent)/2))*.27);
  }
 }
 return occlusion;
}
function append(g,flags,buildings){
 let triangles=0,shadowed=0,minY=Infinity,maxY=-Infinity;
 const size=HALF*2/CELLS;
 for(let iz=0;iz<CELLS;iz++)for(let ix=0;ix<CELLS;ix++){
  const x=-HALF+ix*size,z=-HALF+iz*size,centerX=x+size*.5,centerZ=z+size*.5;
  const moisture=noise(centerX*.19+10,centerZ*.19-1);
  const baked=shadow(centerX,centerZ,buildings);
  if(baked>.025)shadowed++;
  const shade=(.93-.25*baked)+(moisture-.5)*.12;
  const base=moisture>.71?[.33,.32,.26]:moisture<.27?[.25,.34,.22]:[.29,.38,.255];
  const color=base.map(v=>Math.max(.08,Math.min(.78,v*shade)));
  const h00=surface(x,z,flags,buildings),h01=surface(x,z+size,flags,buildings),
   h11=surface(x+size,z+size,flags,buildings),h10=surface(x+size,z,flags,buildings);
  if(![h00,h01,h11,h10].every(Number.isFinite))throw Error("Nonfinite terrain point");
  for(const y of [h00,h01,h11,h10]){minY=Math.min(minY,y);maxY=Math.max(maxY,y);}
  g.tri([x,h00,z],[x,h01,z+size],[x+size,h11,z+size],color);
  g.tri([x,h00,z],[x+size,h11,z+size],[x+size,h10,z],color);
  triangles+=2;
 }
 if(triangles!==BUDGET)throw Error("Terrain tessellation budget violated");
 return {triangles,minY,maxY,shadowedCells:shadowed,drawCallsAdded:0};
}
root.WPWW2Terrain=Object.freeze({append,surface,shadow,maxTriangles:BUDGET});
})(window);
