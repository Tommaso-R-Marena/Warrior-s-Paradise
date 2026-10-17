/* Bounded battlefield aftereffects. Geometry joins the existing dynamic mesh:
 * zero added GPU draw calls, no extra shaders or alpha blending.
 */
(function(root){"use strict";
const MAX=7,clouds=[];
const COLOR={
 scar:[.20,.175,.145],dust:[.48,.435,.35],
 ash:[.36,.35,.325],smoke:[.34,.355,.34]
};
let spawned=0,retired=0;
function blast(x,z,kind="frag"){
 if(!Number.isFinite(x)||!Number.isFinite(z))return false;
 const artillery=kind==="artillery";
 if(clouds.length===MAX){clouds.shift();retired++;}
 const seed=Math.abs(Math.sin(x*12.9898+z*78.233)*43758.5453)%1;
 clouds.push({x,z,kind:artillery?"artillery":"frag",age:0,
  life:artillery?7:4.2,radius:artillery?2.6:1.15,seed});
 spawned++;return true;
}
function update(dt){
 if(!Number.isFinite(dt)||dt<=0)return;
 const step=Math.min(.2,dt);
 for(const c of clouds)c.age+=step;
 for(let i=clouds.length-1;i>=0;i--)if(clouds[i].age>=clouds[i].life){
  clouds.splice(i,1);retired++;
 }
}
function render(g,quality,eyeX=0,eyeZ=0,heightAt=()=>0){
 if(!g||typeof g.tri!=="function")return {scars:0,plumes:0,triangles:0};
 let scars=0,plumes=0,triangles=0;
 const low=quality==="low",high=quality==="high";
 for(const c of clouds){
  const distance=Math.hypot(c.x-eyeX,c.z-eyeZ);
  if(distance>58)continue;
  const count=low?4:8;
  // The discoloration lingers for a few seconds but not forever.
  for(let i=0;i<count;i++){
   const a=i*2*Math.PI/count,b=(i+1)*2*Math.PI/count;
   const variation=1+.10*Math.sin(c.seed*80+i*4.7);
   const ax=c.x+Math.cos(a)*c.radius*variation,az=c.z+Math.sin(a)*c.radius*variation;
   const bx=c.x+Math.cos(b)*c.radius,bz=c.z+Math.sin(b)*c.radius;
   g.tri([c.x,heightAt(c.x,c.z)+.078,c.z],
    [ax,heightAt(ax,az)+.079,az],[bx,heightAt(bx,bz)+.079,bz],COLOR.scar);
   triangles++;
  }
  scars++;
  if(low||distance>36||c.age>c.life*.62)continue;
  // Broken-up opaque dust, not a fictional solid smoke/AI-vision barrier.
  const visibility=1-c.age/(c.life*.62),pieces=high?4:2;
  for(let i=0;i<pieces;i++){
   const theta=c.seed*6.283+i*2.399+c.age*.17;
   const outward=.22+i*.18+c.age*.14;
   const px=c.x+Math.cos(theta)*outward,pz=c.z+Math.sin(theta)*outward;
   const height=(.33+i*.19+c.age*.27)*(c.kind==="artillery"?1.3:1);
   const width=(.16+i*.06+.11*c.age)*Math.max(.18,visibility);
   const facets=high?6:4;
   g.cone(px,heightAt(px,pz)+.05+height*.28,pz,width,height,
    i%3===0?COLOR.ash:i%3===1?COLOR.dust:COLOR.smoke,facets);
   plumes++;triangles+=facets;
  }
 }
 return {scars,plumes,triangles};
}
function reset(){clouds.length=0;spawned=0;retired=0;}
function stats(){return {active:clouds.length,spawned,retired,capacity:MAX};}
root.WPBattleAtmosphere=Object.freeze({blast,update,render,reset,stats});
})(window);
