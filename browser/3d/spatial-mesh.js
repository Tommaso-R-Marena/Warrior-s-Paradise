/* Warrior's Paradise — CPU-side static world spatial batches.
 * Invariant: every source triangle exists in exactly one output mesh.
 * Far-spanning geometry (roads/ground) is never culled.
 * No dependency, allocation, or WebGL call during per-frame visibility tests.
 */
(function(root){"use strict";
const STRIDE=9,TRI=3*STRIDE;
function partition(data,cellSize=20){
 if(!data||data.length%TRI!==0||!(cellSize>=8&&cellSize<=64))
  throw Error("Invalid static mesh geometry");
 const cells=new Map(),global=[],triangleCount=data.length/TRI;
 const finite=(x)=>typeof x==="number"&&Number.isFinite(x);
 for(let offset=0;offset<data.length;offset+=TRI){
  let minx=Infinity,miny=Infinity,minz=Infinity,maxx=-Infinity,maxy=-Infinity,maxz=-Infinity;
  let sumx=0,sumz=0;
  for(let k=0;k<3;k++){
   const i=offset+k*STRIDE,x=data[i],y=data[i+1],z=data[i+2];
   if(!finite(x)||!finite(y)||!finite(z))throw Error("Invalid vertex");
   minx=Math.min(minx,x);miny=Math.min(miny,y);minz=Math.min(minz,z);
   maxx=Math.max(maxx,x);maxy=Math.max(maxy,y);maxz=Math.max(maxz,z);
   sumx+=x;sumz+=z;
  }
  if(maxx-minx>cellSize*1.25||maxz-minz>cellSize*1.25){
   for(let j=0;j<TRI;j++)global.push(data[offset+j]);
   continue;
  }
  const gx=Math.floor((sumx/3)/cellSize),gz=Math.floor((sumz/3)/cellSize);
  const key=gx+","+gz;
  let item=cells.get(key);
  if(!item){item={cx:(gx+.5)*cellSize,cz:(gz+.5)*cellSize,
   bounds:[Infinity,Infinity,Infinity,-Infinity,-Infinity,-Infinity],data:[]};cells.set(key,item);}
  item.bounds[0]=Math.min(item.bounds[0],minx);
  item.bounds[1]=Math.min(item.bounds[1],miny);
  item.bounds[2]=Math.min(item.bounds[2],minz);
  item.bounds[3]=Math.max(item.bounds[3],maxx);
  item.bounds[4]=Math.max(item.bounds[4],maxy);
  item.bounds[5]=Math.max(item.bounds[5],maxz);
  for(let j=0;j<TRI;j++)item.data.push(data[offset+j]);
 }
 const batches=Array.from(cells.values()).map(item=>{
  const b=item.bounds;
  const cx=(b[0]+b[3])*.5,cz=(b[2]+b[5])*.5;
  // Conservative horizontal radius that includes the entire 3D cell's
  // projection, buildings and rotated models (not merely nominal grid size).
  return {cx,cz,radius:Math.hypot((b[3]-b[0])*.5,(b[5]-b[2])*.5)+.25,
    data:new Float32Array(item.data),triangles:item.data.length/TRI};
 });
 const globalData=new Float32Array(global);
 const checkTriangles=batches.reduce((total,b)=>total+b.triangles,global.length/TRI);
 if(checkTriangles!==triangleCount)throw Error("Static partition lost triangles");
 return {batches,global:globalData,triangles:triangleCount,
  globalTriangles:global.length/TRI,batchCount:batches.length};
}
function visible(batch,x,z,yaw,halfHorizontal,far){
 if(!batch||!Number.isFinite(x+z+yaw+halfHorizontal+far))return true;
 const dx=batch.cx-x,dz=batch.cz-z,dist=Math.hypot(dx,dz),r=batch.radius;
 if(dist-r>far)return false;
 if(dist<=r+3)return true;
 const fx=-Math.sin(yaw),fz=Math.cos(yaw);
 const forward=dx*fx+dz*fz;
 if(forward < -r-2)return false;
 const side=Math.abs(dx*fz-dz*fx);
 return side <= Math.max(0,forward)*Math.tan(halfHorizontal)+r+4;
}
root.WPSpatialMesh=Object.freeze({partition,visible});
})(window);