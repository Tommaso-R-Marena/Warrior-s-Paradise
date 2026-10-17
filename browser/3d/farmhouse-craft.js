/* High-only authored-style detail for existing Normandy farmhouse architecture.
 * Historically inspired timber shutters, multi-pane glazing, stone lintels,
 * rain drip ledges, roof tile ridge repairs, and chimney pots. Original
 * procedural geometry only, no extra collision, server, textures or draw passes.
 */
(function(root){"use strict";
const CAP=2400;
const FRAME=[.29,.258,.203],SILL=[.46,.448,.404],
 GLASS=[.177,.229,.238],GLOSS=[.25,.298,.303],
 WOOD=[[.205,.257,.201],[.25,.273,.224],[.306,.248,.185]],
 IRON=[.20,.205,.198],ASH=[.19,.175,.158],CLAY=[.43,.263,.215];
const tagged=c=>[-Math.max(.01,c[0]),c[1],c[2]];
function append(g,buildings){
 if(!g||typeof g.quad!=="function"||typeof g.box!=="function"||
  !Array.isArray(buildings))return {triangles:0,windows:0,shutters:0,roofPots:0};
 let triangles=0,windows=0,shutters=0,hinges=0,lintels=0,roofPots=0,buildingsDecorated=0;
 function face(x0,y0,x1,y1,z,c,side){
  if(triangles+2>CAP)return false;
  const a=[x0,y0,z],b=[x1,y0,z],cc=[x1,y1,z],d=[x0,y1,z];
  if(side<0)g.quad(d,cc,b,a,tagged(c));
  else g.quad(a,b,cc,d,tagged(c));
  triangles+=2;return true;
 }
 for(let idx=0;idx<buildings.length;idx++){
  if(triangles>CAP-100)break;
  const b=buildings[idx];
  if(!Array.isArray(b)||!b.slice(0,4).every(Number.isFinite))continue;
  const [x,z,w,d]=b;if(w<6||w>25||d<5||d>25)continue;
  buildingsDecorated++;
  for(const side of [-1,1]){
   const wall=z+side*(d/2+.245);
   for(const position of [-1,1]){
    const wx=x+position*w*.24;
    const blue=WOOD[(idx+(side>0?1:0)+position+3)%WOOD.length];
    // Original window is 1.1 m wide at 2.8 m height; glass sits over
    // its existing inset rather than hiding an unrelated region of wall.
    const depth=wall+side*.025;
    const cx=wx,cy=2.79;
    for(let pane=0;pane<4;pane++){
     const dx=(pane%2-.5)*.49,dy=(Math.floor(pane/2)-.5)*.53;
     const tint=(pane===0||pane===3)?GLOSS:GLASS;
     face(cx+dx-.20,cy+dy-.23,cx+dx+.20,cy+dy+.23,depth,tint,side);
    }
    // Central glazing cross-members and dark wood window box framing.
    face(cx-.035,cy-.62,cx+.035,cy+.62,depth+side*.018,FRAME,side);
    face(cx-.55,cy-.035,cx+.55,cy+.035,depth+side*.018,FRAME,side);
    face(cx-.59,cy-.65,cx-.52,cy+.65,depth+side*.021,FRAME,side);
    face(cx+.52,cy-.65,cx+.59,cy+.65,depth+side*.021,FRAME,side);
    face(cx-.58,cy+.57,cx+.58,cy+.67,depth+side*.021,FRAME,side);
    // Stone lintel and drip-bearing sill; thin planes do not introduce
    // collision that would disagree with Low graphics or bullets.
    face(cx-.66,3.47,cx+.66,3.55,depth+side*.027,SILL,side);
    face(cx-.69,2.13,cx+.69,2.22,depth+side*.035,SILL,side);lintels+=2;
    for(const flap of [-1,1]){
     const sx=cx+flap*.795,zsh=wall+side*.055;
     const shade=blue.map(v=>v*(.94+.04*((idx+flap+2)%3)));
     // Three distinct longitudinal wooden shutter planks plus two battens
     // and forged-iron hinge irons mimic practical rural shutters.
     for(let plank=0;plank<3;plank++){
      const plx=sx+(plank-1)*.14;
      face(plx-.065,2.18,plx+.065,3.39,zsh,shade,side);
     }
     for(const yy of [2.36,3.22]){
      face(sx-.235,yy-.034,sx+.235,yy+.034,zsh+side*.016,FRAME,side);
      face(sx-.20,yy-.017,sx-.10,yy+.017,zsh+side*.026,IRON,side);
      hinges++;
     }
     shutters++;
    }
    windows++;
   }
   // Low-profile floor threshold follows the existing non-interactive door.
   const door=wall+side*.075;
   face(x-.79,.08,x+.79,.22,door,SILL,side);
   face(x-.72,2.28,x+.72,2.36,door,FRAME,side);
  }
  // Ceramic chimney pots are small, visible silhouettes seated above the
  // existing solid chimney's top plane (the base game chimney reaches ~6.82).
  const bx=x+w*.31,bz=z-d*.15;
  for(const offset of [-.18,.18]){
   if(triangles+12>CAP)break;
   g.box(bx+offset,7.00,bz,.14,.30,.18,tagged(CLAY));triangles+=12;roofPots++;
   // Soot-dark cap visible at non-grazing camera angles.
   face(bx+offset-.11,7.15,bx+offset+.11,7.19,bz+d*.008,ASH,1);
  }
 }
 return {triangles,windows,shutters,hinges,lintels,roofPots,
  buildings:buildingsDecorated,collidersAdded:0,drawCallsAdded:0,maxTriangles:CAP};
}
root.WPFarmhouseCraft=Object.freeze({append,maxTriangles:CAP});
})(window);
