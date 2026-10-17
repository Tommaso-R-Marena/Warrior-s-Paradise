/* High-only Normandy masonry weathering, drainage and wall-climbing ivy.
 * All vertices are placed on the existing nine solid farmhouse facades.
 * Does not create collision, AI cover or separate WebGL2 draw calls.
 * A negative-red pigment tag explicitly bypasses the world atlas in High.
 * This is original procedural texture/geometry, not authentic photogrammetry.
 */
(function(root){"use strict";
const MAX=2350;
const ROCK=[[.40,.378,.331],[.50,.469,.414],[.31,.306,.281]],
      DAMP=[[.215,.235,.210],[.267,.259,.231],[.330,.306,.259]],
      IVY=[[.195,.293,.147],[.249,.335,.174],[.276,.352,.192]];
function pigment(c){return [-Math.max(.025,c[0]),c[1],c[2]];}
function rand32(seed){let s=seed|0;return()=>{
 s^=s<<13;s^=s>>>17;s^=s<<5;return(s>>>0)/4294967296;};}
function append(g,buildings){
 if(!g||typeof g.tri!=="function"||typeof g.quad!=="function"||
  !Array.isArray(buildings))return {triangles:0,buildings:0,patches:0,ivyLeaves:0};
 let triangles=0,patches=0,ivyLeaves=0,downspouts=0,mortar=0,houses=0;
 const tri=(a,b,c,color)=>{
  if(triangles>=MAX)return false;
  g.tri(a,b,c,color);triangles++;return true;
 };
 const quad=(a,b,c,d,color)=>{
  if(triangles+2>MAX)return false;
  g.quad(a,b,c,d,color);triangles+=2;return true;
 };
 const normals=[];
 for(let j=0;j<buildings.length;j++){
  const entry=buildings[j];
  if(!Array.isArray(entry)||entry.length<4||!entry.slice(0,4).every(Number.isFinite))continue;
  const [cx,cz,w,d]=entry;
  if(w<3||d<3||w>30||d>30)continue;
  houses++;
  const rng=rand32(0x194406+j*70791);
  for(const side of [-1,1]){
   const front=cz+side*(d/2+.222);
   // Quoin repair fills are aligned with both sides of *existing* wall surfaces.
   // Avoid window and door silhouettes; they are deliberately not fake cover.
   for(let k=0;k<10;k++){
    const isLeft=k%2===0;
    const localX=(isLeft?-1:1)*(w*.49-(.09+rng()*.35));
    const yy=.35+rng()*3.7,half=.13+rng()*.24;
    const x=cx+localX,z=front+side*(.003+k*.00008);
    const crack=ROCK[(j+k)%ROCK.length];
    const color=pigment(crack.map(c=>c*(.80+.2*rng())));
    if(tri([x-half,yy-.13,z],[x+half,yy-.08,z],[x+(rng()-.5)*half,yy+.30,z],color))patches++;
   }
   // Shallow soot and drainage staining beneath sills, grounded in real geometry.
   for(const windowX of [-w*.24,w*.24]){
    for(let k=0;k<4;k++){
     const shift=(rng()-.5)*.27;
     const x0=cx+windowX+shift;
     const y0=1.47-rng()*.36-.14*k,y1=1.83-rng()*.09;
     const z=front+side*.007;
     const shade=pigment(DAMP[(k+j)%DAMP.length]);
     if(quad([x0-.055,y0,z],[x0+.050,y0+.075,z],
      [x0+.031,y1,z],[x0-.020,y1-.15,z],shade))patches++;
    }
   }
   // Tapered vines grow along the corners; they do not emerge across windows.
   // Unlike wall pigment, positive green remains a wind-animated leaf material.
   if(j%2===0){
    for(const direction of [-1,1]){
     const x=cx+direction*(w*.44),z=front+side*.046;
     let prev=[x,.23,z];
     for(let k=0;k<10;k++){
      const yy=.30+k*.36;
      const sway=(Math.sin(k*1.66+j*2.1))*.15;
      const px=x+sway,pz=z+side*Math.sin(k*.7)*.033;
      const point=[px,yy,pz];
      if(quad([prev[0]-.018,prev[1],prev[2]],[prev[0]+.019,prev[1],prev[2]],
       [px+.016,yy,pz],[px-.017,yy,pz],pigment(DAMP[0]))mortar++;
      prev=point;
      // Leaf silhouettes stand off the masonry just enough to prevent flicker.
      for(const flip of [-1,1]){
       const spread=flip*(.11+.075*rng());
       const tip=[px+spread,yy+.15+rng()*.12,pz+side*(.055+.08*rng())];
       const leaf=IVY[(k+j+(flip===1?1:0))%IVY.length];
       if(tri([px,yy,pz+side*.039],
        [px+spread*.43,yy-.065,pz+side*.045],tip,leaf))ivyLeaves++;
      }
     }
    }
   }
   // Metal rainwater drains, gutters and masonry drips are decorative and thin.
   const pipeX=cx+w*.43;
   for(let k=0;k<4;k++){
    const z=front+side*.085,yy=.28+k*.94;
    if(quad([pipeX-.048,yy,z],[pipeX+.048,yy,z],
     [pipeX+.048,yy+.95,z],[pipeX-.048,yy+.95,z],pigment(DAMP[2])))downspouts++;
   }
   // Short, irregular wall horizontal grouting courses with no Z fighting.
   for(let k=0;k<3;k++){
    const yy=.28+k*.54,z=front+side*.012;
    for(let s=0;s<5;s++){
     const x0=cx-w*.43+s*w*.172;
     const tone=pigment(ROCK[(s+j)%ROCK.length].map(c=>c*.77));
     if(quad([x0,yy,z],[x0+w*.10,yy,z],
      [x0+w*.10,yy+.023,z],[x0,yy+.023,z],tone))mortar++;
    }
   }
  }
 }
 return {triangles,patches,ivyLeaves,downspouts,mortar,buildings:houses,
  maxTriangles:MAX,drawCallsAdded:0,collidersAdded:0};
}
root.WPArchitecturalWeathering=Object.freeze({append,maxTriangles:MAX});
})(window);
