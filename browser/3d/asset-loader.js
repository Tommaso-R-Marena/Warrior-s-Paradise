/* Warrior's Paradise — minimal local GLB mesh reader (no CDN).
 * Import boundary for curated static glTF 2.0 models. Unsupported meshes fail
 * closed and the procedural battlefield remains playable.
 * No skinning, animation, compressed mesh extensions or PBR textures yet.
 */
(function(root){
"use strict";
const MAX_BYTES=4*1024*1024, MAX_TRIANGLES=60000;
function assert(condition,message){if(!condition)throw Error("WP model: "+message);}
const ID=[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1];
function multiply(a,b){
 const m=new Array(16).fill(0);
 for(let c=0;c<4;c++)for(let r=0;r<4;r++)for(let k=0;k<4;k++)
  m[c*4+r]+=a[k*4+r]*b[c*4+k];
 return m;
}
function nodeMatrix(n){
 if(n.matrix){assert(n.matrix.length===16,"invalid matrix");return n.matrix;}
 const t=n.translation||[0,0,0],s=n.scale||[1,1,1];
 const [x,y,z,w]=n.rotation||[0,0,0,1];
 const xx=x*x,yy=y*y,zz=z*z,xy=x*y,xz=x*z,yz=y*z,wx=w*x,wy=w*y,wz=w*z;
 return [(1-2*(yy+zz))*s[0],(2*(xy+wz))*s[0],(2*(xz-wy))*s[0],0,
  (2*(xy-wz))*s[1],(1-2*(xx+zz))*s[1],(2*(yz+wx))*s[1],0,
  (2*(xz+wy))*s[2],(2*(yz-wx))*s[2],(1-2*(xx+yy))*s[2],0,
  t[0],t[1],t[2],1];
}
function transform(v,m){
 return [m[0]*v[0]+m[4]*v[1]+m[8]*v[2]+m[12],
  m[1]*v[0]+m[5]*v[1]+m[9]*v[2]+m[13],
  m[2]*v[0]+m[6]*v[1]+m[10]*v[2]+m[14]];
}
function parseGLB(buffer){
 assert(buffer instanceof ArrayBuffer,"expected ArrayBuffer");
 assert(buffer.byteLength>=28&&buffer.byteLength<=MAX_BYTES,"invalid model size");
 const dv=new DataView(buffer);
 assert(dv.getUint32(0,true)===0x46546c67,"not a binary glTF");
 assert(dv.getUint32(4,true)===2&&dv.getUint32(8,true)===buffer.byteLength,"invalid GLB header");
 let off=12,doc=null,bin=null;
 while(off+8<=buffer.byteLength){
  const len=dv.getUint32(off,true),type=dv.getUint32(off+4,true);off+=8;
  assert(len%4===0&&off+len<=buffer.byteLength,"bad chunk length");
  if(type===0x4e4f534a){
   assert(!doc,"duplicate JSON chunk");
   doc=JSON.parse(new TextDecoder().decode(new Uint8Array(buffer,off,len)));
  }else if(type===0x004e4942){assert(!bin,"duplicate binary chunk");bin=new DataView(buffer,off,len);}
  off+=len;
 }
 assert(off===buffer.byteLength&&doc&&bin&&doc.asset?.version==="2.0","incomplete glTF");
 assert(!doc.extensionsRequired?.some(x=>x!=="KHR_materials_unlit"),"unsupported required extension");
 const allowed=(a)=>a&&a.bufferView!==undefined&&!a.sparse;
 function accessor(id){
  const a=doc.accessors?.[id];assert(allowed(a),"unsupported accessor");
  const bv=doc.bufferViews?.[a.bufferView];
  assert(bv&&(!bv.buffer||bv.buffer===0),"unsupported buffer view");
  const n={SCALAR:1,VEC2:2,VEC3:3,VEC4:4}[a.type],sz={5120:1,5121:1,5122:2,5123:2,5125:4,5126:4}[a.componentType];
  assert(n&&sz&&Number.isInteger(a.count)&&a.count>=0&&a.count<=300000,"invalid accessor");
  const stride=bv.byteStride||n*sz;
  assert(stride>=n*sz&&stride<=252,"bad stride");
  const start=(bv.byteOffset||0)+(a.byteOffset||0);
  assert(start>=0&&start+(Math.max(a.count-1,0))*stride+n*sz<=bin.byteLength,"accessor out of bounds");
  const out=new Array(a.count*n);
  for(let i=0;i<a.count;i++)for(let k=0;k<n;k++){
   const p=start+i*stride+k*sz;let v;
   switch(a.componentType){
    case 5120:v=bin.getInt8(p);break;case 5121:v=bin.getUint8(p);break;
    case 5122:v=bin.getInt16(p,true);break;case 5123:v=bin.getUint16(p,true);break;
    case 5125:v=bin.getUint32(p,true);break;case 5126:v=bin.getFloat32(p,true);break;
   }
   assert(Number.isFinite(v),"nonfinite vertex");
   if(a.normalized&&a.componentType!==5126){
    const max={5120:127,5121:255,5122:32767,5123:65535,5125:4294967295}[a.componentType];
    v=Math.max(-1,v/max);
   }
   out[i*n+k]=v;
  }
  return {out,n,count:a.count};
 }
 const triangles=[],min=[Infinity,Infinity,Infinity],max=[-Infinity,-Infinity,-Infinity];
 function visit(id,parent,depth){
  assert(depth<=12,"too deep node hierarchy");
  const node=doc.nodes?.[id];assert(node,"invalid scene node");
  const mat=multiply(parent,nodeMatrix(node));
  if(node.mesh!==undefined){
   const mesh=doc.meshes?.[node.mesh];assert(mesh,"missing mesh");
   for(const p of mesh.primitives||[]){
    assert(p.mode===undefined||p.mode===4,"nontriangular primitive");
    assert(p.attributes?.POSITION!==undefined,"missing POSITION");
    const verts=accessor(p.attributes.POSITION);assert(verts.n===3,"positions must be vec3");
    const indices=p.indices===undefined?{out:Array.from({length:verts.count},(_,i)=>i),count:verts.count,n:1}:accessor(p.indices);
    assert(indices.n===1&&indices.count%3===0,"invalid triangle indices");
    const color=doc.materials?.[p.material||0]?.pbrMetallicRoughness?.baseColorFactor||[.55,.57,.49,1];
    const colors=p.attributes.COLOR_0!==undefined?accessor(p.attributes.COLOR_0):null;
    for(let i=0;i<indices.count;i+=3){
     assert(triangles.length<MAX_TRIANGLES,"too many triangles");
     const vs=[],cs=[];
     for(let k=0;k<3;k++){
      const vi=indices.out[i+k];assert(Number.isInteger(vi)&&vi>=0&&vi<verts.count,"bad vertex index");
      const v=transform(verts.out.slice(vi*3,vi*3+3),mat);vs.push(v);
      for(let axis=0;axis<3;axis++){min[axis]=Math.min(min[axis],v[axis]);max[axis]=Math.max(max[axis],v[axis]);}
      if(colors){assert(colors.n>=3&&vi<colors.count,"bad vertex color");cs.push(colors.out.slice(vi*colors.n,vi*colors.n+3));}
     }
     const rgb=[0,1,2].map(c=>Math.min(1,Math.max(0,color[c]*(colors?(cs[0][c]+cs[1][c]+cs[2][c])/3:1))));
     triangles.push({vs,color:rgb,material:p.material||0});
    }
   }
  }
  for(const child of node.children||[])visit(child,mat,depth+1);
 }
 const nodes=doc.scenes?.[doc.scene||0]?.nodes||[];
 assert(nodes.length>0,"missing default scene");
 for(const id of nodes)visit(id,ID,0);
 assert(triangles.length>0,"no mesh triangles");
 return {triangles,min,max,count:triangles.length};
}
async function loadModels(specs){
 const entries=await Promise.all(specs.map(async spec=>{
  assert(/^[a-z0-9-]+$/.test(spec.name),"unsafe asset name");
  const url="./assets/kenney/"+spec.name+".glb.b64";
  const response=await fetch(url,{cache:"force-cache"});
  if(!response.ok)throw Error("asset HTTP "+response.status+": "+spec.name);
  const encoded=(await response.text()).replace(/\s/g,"");
  assert(encoded.length>0&&encoded.length<MAX_BYTES*1.4,"base64 size");
  assert(/^[A-Za-z0-9+/]*={0,2}$/.test(encoded),"invalid base64 text");
  const raw=atob(encoded),bytes=Uint8Array.from(raw,c=>c.charCodeAt(0));
  const model=parseGLB(bytes.buffer);
  return [spec.name,model];
 }));
 return Object.fromEntries(entries);
}
function appendInstances(geom,model,instances,palette){
 assert(model&&Array.isArray(model.triangles),"model not loaded");
 for(const instance of instances){
  const [x,y,z,scale,yaw=0]=instance;
  const cs=Math.cos(yaw),sn=Math.sin(yaw);
  for(const tri of model.triangles){
   const vs=tri.vs.map(v=>{
    const vx=v[0]*scale,vz=v[2]*scale;
    return [x+vx*cs+vz*sn,y+(v[1]-model.min[1])*scale,z-vx*sn+vz*cs];
   });
   geom.tri(vs[0],vs[1],vs[2],palette?.[tri.material]||tri.color);
  }
 }
 return instances.length*model.count;
}
root.WPAssetKit=Object.freeze({parseGLB,loadModels,appendInstances});
})(window);
