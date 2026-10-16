/* High-only geometric roof detailing for the Normandy village.
 * Creates staggered individual roof courses over the existing collision-neutral
 * gabled roof mesh. Bounded geometry shares the static battlefield draw call.
 */
(function(root){"use strict";
const MAX_TRIANGLES=1280;
function append(g,buildings){
 if(!g||typeof g.quad!=="function"||!Array.isArray(buildings))return {triangles:0,tiles:0,roofs:0};
 let triangles=0,tiles=0,roofs=0;
 for(let j=0;j<buildings.length;j++){
  const [x,z,w,d]=buildings[j];
  if(![x,z,w,d].every(Number.isFinite)||w<=0||d<=0)continue;
  const half=(w+1)*.5,depth=(d+1)*.5;
  const material=j%2===0?[.465,.253,.207]:[.355,.339,.304];
  let roofTiles=0;
  for(const side of [-1,1]){
   for(let row=0;row<7;row++){
    // Each course follows the actual pitched roof equation from Geometry.roof.
    const outer=1-(row+.10)/7,inner=1-(row+.91)/7;
    const xx0=x+side*half*outer,xx1=x+side*half*inner;
    const yy0=4.3+2*(1-outer)+.029,yy1=4.3+2*(1-inner)+.029;
    for(let segment=0;segment<5;segment++){
     if(triangles+2>MAX_TRIANGLES)break;
     const t0=(segment+.055+(row%2)*.095)/5;
     const t1=(segment+.95+(row%2)*.035)/5;
     const zz0=z-depth+2*depth*Math.min(t0,.998);
     const zz1=z-depth+2*depth*Math.min(t1,.999);
     const tone=.88+((row*13+segment*7+j*5)%7)*.026;
     const color=material.map(v=>Math.min(.79,v*tone));
     g.quad([xx0,yy0,zz0],[xx0,yy0,zz1],
      [xx1,yy1,zz1],[xx1,yy1,zz0],color);
     triangles+=2;tiles++;roofTiles++;
    }
   }
  }
  if(roofTiles)roofs++;
 }
 return {triangles,tiles,roofs,maxTriangles:MAX_TRIANGLES};
}
root.WPArchitecturalDetail=Object.freeze({append,maxTriangles:MAX_TRIANGLES});
})(window);
