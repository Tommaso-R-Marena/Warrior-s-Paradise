/* Proof-of-bytes boundary for offline-baked static art.
 * No remote URLs, GPU baking, decoder libraries or paid infrastructure.
 * 12 bytes / vertex: int16 position x/y/z, signed normal xyz, RGB baked AO.
 * Exactly 3 vertices per triangle; game geometry remains 9 float32/vertex.
 */
(function(root){"use strict";
const MAX_FILES=12,MAX_VERTICES=180000,MAX_BYTES=4*1024*1024;
const LICENSES=new Set(["CC0-1.0","CC-BY-4.0","CC-BY-SA-4.0","original"]);
function check(x,msg){if(!x)throw Error("BakedArt: "+msg);}
const finite=(v)=>typeof v==="number"&&Number.isFinite(v);
function safeName(s){return typeof s==="string"&&/^[a-z0-9][a-z0-9_-]{0,63}$/.test(s);}
function fnv(data){
 let h=0x811c9dc5;for(let i=0;i<data.length;i++){h^=data[i];h=Math.imul(h,0x01000193)>>>0;}
 return h.toString(16).padStart(8,"0");
}
function bytes(encoded){
 check(typeof encoded==="string"&&encoded.length>=16&&encoded.length<MAX_BYTES*1.4,"invalid packed base64 size");
 check(/^[A-Za-z0-9+/]+={0,2}$/.test(encoded)&&encoded.length%4===0,"invalid packed base64");
 const raw=atob(encoded);
 check(raw.length<=MAX_BYTES,"packed geometry exceeds budget");
 return Uint8Array.from(raw,c=>c.charCodeAt(0));
}
function decode(mesh,quality){
 check(mesh&&mesh.schema==="wp-baked-mesh-v1"&&safeName(mesh.id),"unrecognized pack");
 check(mesh.source&&LICENSES.has(mesh.source.license)&&
  typeof mesh.source.author==="string"&&mesh.source.author.length>0&&mesh.source.author.length<150,
  "missing permitted source license and attribution");
 for(const key of ["min","max"])check(Array.isArray(mesh.bounds?.[key])&&mesh.bounds[key].length===3&&mesh.bounds[key].every(finite),"invalid bounds");
 const lo=mesh.bounds.min,hi=mesh.bounds.max;
 check(lo.every((v,i)=>v<=hi[i]&&Math.abs(v)<250&&Math.abs(hi[i])<250),"world-space bounds invalid");
 const entry=mesh.lods?.[quality];
 check(entry&&Number.isInteger(entry.vertices)&&entry.vertices>0&&entry.vertices<=MAX_VERTICES&&
  entry.vertices%3===0,"invalid vertex count");
 const data=bytes(entry.data);
 check(data.length===entry.vertices*12,"incorrect packed vertex bytes");
 check(entry.fnv32===fnv(data),"checksum mismatch");
 const view=new DataView(data.buffer,data.byteOffset,data.byteLength);
 const output=new Float32Array(entry.vertices*9);
 const range=hi.map((x,i)=>x-lo[i]);
 for(let i=0;i<entry.vertices;i++){
  const at=i*12;
  for(let ax=0;ax<3;ax++){
   const q=view.getUint16(at+2*ax,true);
   output[i*9+ax]=lo[ax]+q/65535*range[ax];
  }
  const nx=view.getInt8(at+6)/127,ny=view.getInt8(at+7)/127,nz=view.getInt8(at+8)/127;
  const len=Math.hypot(nx,ny,nz);
  check(len>.2&&len<1.8,"invalid vertex normal");
  output[i*9+3]=nx/len;output[i*9+4]=ny/len;output[i*9+5]=nz/len;
  for(let c=0;c<3;c++)output[i*9+6+c]=data[at+9+c]/255;
 }
 return {data:output,triangles:entry.vertices/3,bytes:data.length,id:mesh.id,source:mesh.source};
}
function transform(base,instances){
 const out=new Float32Array(base.data.length*instances.length),vertices=base.data.length/9;
 check(out.length<=MAX_VERTICES*9*3,"too many placed vertices");
 for(let k=0;k<instances.length;k++){
  const q=instances[k],cs=Math.cos(q.yaw),sn=Math.sin(q.yaw);
  for(let i=0;i<vertices;i++){
   const s=i*9,d=(k*vertices+i)*9,x=base.data[s]*q.scale,z=base.data[s+2]*q.scale;
   out[d]=q.x+x*cs+z*sn;
   out[d+1]=q.y+base.data[s+1]*q.scale;
   out[d+2]=q.z-x*sn+z*cs;
   const nx=base.data[s+3],nz=base.data[s+5];
   out[d+3]=nx*cs+nz*sn;out[d+4]=base.data[s+4];out[d+5]=-nx*sn+nz*cs;
   out.set(base.data.subarray(s+6,s+9),d+6);
   // Negative red is an explicit baked-vertex-color marker, not a metal tag.
   // Both WebGL shaders remove this sign before lighting and atlas routing.
   out[d+6]=-Math.abs(out[d+6]);
  }
 }
 return out;
}
function manifestValid(doc){
 check(doc&&doc.schema==="wp-baked-scene-v1"&&Array.isArray(doc.groups)&&
  doc.groups.length>0&&doc.groups.length<=MAX_FILES,"invalid scene manifest");
 const names=new Set;
 for(const group of doc.groups){
  check(safeName(group.mesh)&&!names.has(group.mesh),"unsafe or duplicate mesh");
  names.add(group.mesh);
  check(Array.isArray(group.instances)&&group.instances.length>0&&group.instances.length<=12,"invalid placements");
  for(const q of group.instances){
   check(["x","y","z","yaw","scale"].every(k=>finite(q[k]))&&q.scale>.01&&q.scale<30&&
    Math.abs(q.x)<47&&Math.abs(q.z)<47&&Math.abs(q.y)<25&&Math.abs(q.yaw)<7,
    "invalid instance transform");
   // Every decorative asset must lie on top of a pre-existing physical
   // building: no cover can be invented by a visual-only imported mesh.
   check(q.collider==="existing-building","static art requires physical anchor");
  }
 }
 return doc;
}
async function load(fetcher=fetch){
 const response=await fetcher("./assets/baked/scene.json",{cache:"force-cache"});
 check(response.ok,"manifest HTTP "+response.status);
 const doc=manifestValid(await response.json());
 const groups=await Promise.all(doc.groups.map(async group=>{
  const res=await fetcher("./assets/baked/"+group.mesh+".wpmesh.json",{cache:"force-cache"});
  check(res.ok,"mesh HTTP "+res.status);
  const mesh=await res.json();
  check(mesh.id===group.mesh,"mesh identity mismatch");
  const medium=transform(decode(mesh,"medium"),group.instances);
  const high=transform(decode(mesh,"high"),group.instances);
  return {medium,high,instances:group.instances.length,bounds:mesh.bounds,placements:group.instances,
   mediumTriangles:medium.length/27,highTriangles:high.length/27,
   source:mesh.source,mesh:group.mesh};
 }));
 const sum=(key)=>groups.reduce((s,g)=>s+g[key],0);
 check(sum("highTriangles")<35000&&sum("mediumTriangles")<15000,"scene triangle budget");
 return {groups,highTriangles:sum("highTriangles"),mediumTriangles:sum("mediumTriangles"),
  instances:sum("instances"),sources:groups.map(g=>({id:g.mesh,...g.source}))};
}
// Preserve Low prefix, insert Medium after its complete prefix, append High;
// all tier meshes remain in a SINGLE static draw call.
function spliceTier(base,lowCount,mediumCount,baked){
 check(Array.isArray(base)||base instanceof Float32Array,"bad base mesh");
 check(Number.isInteger(lowCount)&&Number.isInteger(mediumCount)&&
  lowCount>=0&&mediumCount>=lowCount&&mediumCount*9<=base.length,"bad tier boundaries");
 const medium=baked.groups.flatMap(g=>Array.from(g.medium));
 const high=baked.groups.flatMap(g=>Array.from(g.high));
 const split=mediumCount*9;
 return {data:[...base.slice(0,split),...medium,...base.slice(split),...high],
  lowCount,mediumCount:mediumCount+medium.length/9,
  highCount:base.length/9+(medium.length+high.length)/9};
}
root.WPBakedArt=Object.freeze({load,decode,transform,spliceTier,manifestValid,fnv,maxSceneHighTriangles:35000});
})(window);
