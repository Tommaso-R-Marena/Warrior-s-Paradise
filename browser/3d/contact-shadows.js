/* Lightweight grounded shadows for dynamic armor and infantry.
 * Uses terrain-following opaque geometry in the EXISTING live vertex buffer.
 * Only Medium/High render it; no extra shader, transparency, pass or network IO.
 * The sun direction matches scene's sun (.53,.88,-.36).
 */
(function(root){"use strict";
const DARK=[.032,.039,.048],SOFT=[.060,.067,.077];
const MAX=210;
function render(g,ground,player,quality,soldiers=[],vehicles=[]){
 if(!g||typeof g.tri!=="function"||typeof ground!=="function"||
  quality==="low")return {triangles:0,actors:0,budget:MAX};
 let triangles=0,actors=0;
 const sectors=quality==="high"?12:8;
 function patch(actor,rx,rz,height,strength=1){
  if(triangles+sectors*2>MAX)return;
  if(Math.hypot(actor.x-player.x,actor.z-player.z)>34)return;
  const yaw=actor.yaw||actor.facing||0,s=Math.sin(yaw),c=Math.cos(yaw);
  // Light travel from high right sun creates a compact directional contact offset.
  const offset=(height/.88)*.50;
  const x=actor.x-.53*offset,z=actor.z+.36*offset;
  const vertex=(a,rad,scale=1)=>{
   const u=Math.cos(a)*rx*rad*scale,v=Math.sin(a)*rz*rad*scale;
   const X=x+u*c-v*s,Z=z+u*s+v*c;
   const y=ground(X,Z)+.04;return [X,y,Z];
  };
  const center=[x,ground(x,z)+.04,z];
  for(let i=0;i<sectors&&triangles+2<=MAX;i++){
   const a=i*2*Math.PI/sectors,b=(i+1)*2*Math.PI/sectors;
   const p=vertex(a,.61),q=vertex(b,.61);
   g.tri(center,p,q,DARK);triangles++;
   g.tri(p,vertex(a,1.0),vertex(b,1.0),SOFT);triangles++;
  }
  actors++;
 }
 if(quality==="high"){
  for(const a of soldiers){
   if((a.hp??100)<=0)continue;
   patch(a,.35,.24,1.10);
  }
 }
 for(const v of vehicles){
  // Wrecks still cast contact shadows: the hull remains on the battlefield.
  patch(v,v.width*.55,v.length*.55,1.4);
 }
 return {triangles,actors,budget:MAX};
}
root.WPContactShadows=Object.freeze({render,maxTriangles:MAX,colors:{dark:DARK,soft:SOFT}});
})(window);
