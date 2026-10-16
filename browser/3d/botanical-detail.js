/* High-only botanical layer: terrain-anchored grasses, wildflowers and
 * irregular bocage foliage. No transparent overdraw or gameplay colliders.
 * All geometry is appended to the existing static world VBO.
 */
(function(root){"use strict";
const LIMIT=1750;
const GREENS=[[.22,.31,.16],[.27,.35,.18],[.34,.39,.21],[.24,.32,.20]],
 STRAW=[.43,.41,.26],FLOWER=[[.56,.48,.26],[.60,.59,.47],[.45,.47,.59]];
const smooth=(x,a,b)=>Math.max(0,Math.min(1,(x-a)/(b-a)));
function append(g,blocked,flags=[],buildings=[]){
 let state=0x19440816,triangles=0,clumps=0,blades=0,flowers=0,hedges=0;
 const random=()=>{state^=state<<13;state^=state>>>17;state^=state<<5;return(state>>>0)/4294967296;};
 const terrain=(x,z)=>root.WPWW2Terrain?.surface(x,z,flags,buildings)||0;
 function tri(a,b,c,color){if(triangles>=LIMIT)return false;g.tri(a,b,c,color);triangles++;return true;}
 // Four different plant heights per clump, with variable bending & splayed leaf normals.
 for(let i=0;i<255&&triangles<LIMIT-12;i++){
  const x=-44+random()*88,z=-44+random()*88;
  const nr=.34+random()*.40;
  if(blocked(x,z,nr+1.3)||flags.some(f=>Math.hypot(x-f.x,z-f.z)<5.8))continue;
  const y=terrain(x,z)+.025,plantH=.20+random()*.36;
  const baseHue=GREENS[Math.floor(random()*GREENS.length)];
  for(let j=0;j<5;j++){
   const angle=random()*Math.PI*2,offset=.035+random()*.26;
   const dx=Math.cos(angle),dz=Math.sin(angle);
   const rootX=x+dx*offset,rootZ=z+dz*offset;
   const h=plantH*(.60+random()*.7),tip=.14+random()*.23;
   const width=.028+random()*.04;
   const col=baseHue.map((v,k)=>Math.min(.75,v*(.88+random()*.24)));
   if(!tri([rootX-dz*width,y,rootZ+dx*width],
    [rootX+dz*width,y,rootZ-dx*width],
    [rootX+dx*tip,y+h,rootZ+dz*tip],col))break;
   blades++;
  }
  // Rare vertical wildflowers create natural color variation and spatial scale.
  if(i%13===0&&triangles<LIMIT-7){
   const h=plantH*1.3;
   tri([x-.015,y,z],[x+.015,y,z],[x,y+h,z],GREENS[0]);
   const petal=FLOWER[Math.floor(random()*FLOWER.length)];
   for(let k=0;k<4;k++){
    const a=k*Math.PI*.5;
    const dx=Math.cos(a)*.10,dz=Math.sin(a)*.10;
    tri([x,y+h,z],[x+dx,y+h+.04,z+dz],
     [x+dx*.55-dz*.35,y+h+.10,z+dz*.55+dx*.35],petal);
   }
   flowers++;
  }
  clumps++;
 }
 // Low, ragged bocage canopy silhouettes in unused field margins.
 for(let i=0;i<36&&triangles<LIMIT-13;i++){
  const edge=i%4,offset=-41+Math.floor(i/4)*10.1;
  const x=edge===0?-40:edge===1?40:offset;
  const z=edge===2?-40:edge===3?40:offset;
  if(blocked(x,z,1.5))continue;
  const y=terrain(x,z),height=.60+random()*.70,r=.53+random()*.45;
  for(let k=0;k<8;k++){
   const a=k*Math.PI/4,b=(k+1)*Math.PI/4;
   const base=[x+(r+.08)*Math.cos(a),y+.27,z+(r+.08)*Math.sin(a)];
   const next=[x+(r+.08)*Math.cos(b),y+.27,z+(r+.08)*Math.sin(b)];
   if(!tri(base,next,[x+(random()-.5)*.38,y+height,z+(random()-.5)*.38],
     GREENS[(k+i)%GREENS.length]))break;
  }
  hedges++;
 }
 return {triangles,clumps,blades,flowers,hedges,maxTriangles:LIMIT};
}
root.WPBotanicalDetail=Object.freeze({append,maxTriangles:LIMIT});
})(window);
