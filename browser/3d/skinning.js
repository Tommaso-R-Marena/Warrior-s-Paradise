/* Warrior's Paradise — controlled glTF 2.0 skinned-mesh runtime.
 * Built for tiny, provenance-audited CC0 characters. No CDN or native module.
 * CPU skinning produces triangles in the existing WebGL2 geometry pipeline.
 * Supports joint hierarchy, inverse bind matrices, 4 weighted influences,
 * quaternion poses, and LINEAR/STEP glTF animation translation/rotation tracks.
 */
(function(root){"use strict";
const MAX=3*1024*1024, MAX_JOINTS=96, MAX_VERTICES=16000;
const fail=m=>{throw Error("WP rig: "+m)};
const check=(b,m)=>{if(!b)fail(m)};
const identity=()=>[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1];
function mul(a,b){const o=new Array(16).fill(0);for(let c=0;c<4;c++)for(let r=0;r<4;r++)
 for(let k=0;k<4;k++)o[c*4+r]+=a[k*4+r]*b[c*4+k];return o;}
function at(m,x,y,z){return [m[0]*x+m[4]*y+m[8]*z+m[12],
 m[1]*x+m[5]*y+m[9]*z+m[13],m[2]*x+m[6]*y+m[10]*z+m[14]];}
function quatMul(a,b){return[a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1],
 a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0],
 a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3],
 a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2]];}
function axisQ(axis,angle){const a=Math.sin(angle*.5);return[axis[0]*a,axis[1]*a,axis[2]*a,Math.cos(angle*.5)]}
function matrix(node,t,r,s){
 if(node.matrix&&!t&&!r&&!s)return node.matrix;
 const T=t||node.translation||[0,0,0],S=s||node.scale||[1,1,1],
 R=r||node.rotation||[0,0,0,1];const [x,y,z,w]=R;
 return [(1-2*(y*y+z*z))*S[0],(2*(x*y+z*w))*S[0],(2*(x*z-y*w))*S[0],0,
 (2*(x*y-z*w))*S[1],(1-2*(x*x+z*z))*S[1],(2*(y*z+x*w))*S[1],0,
 (2*(x*z+y*w))*S[2],(2*(y*z-x*w))*S[2],(1-2*(x*x+y*y))*S[2],0,T[0],T[1],T[2],1];
}
function parse(buffer){
 check(buffer instanceof ArrayBuffer&&buffer.byteLength>=28&&buffer.byteLength<=MAX,"invalid GLB input");
 const dv=new DataView(buffer);
 check(dv.getUint32(0,true)===0x46546c67&&dv.getUint32(4,true)===2
  &&dv.getUint32(8,true)===buffer.byteLength,"bad GLB header");
 let pos=12,doc=null,bin=null;
 while(pos+8<=buffer.byteLength){
  const len=dv.getUint32(pos,true),type=dv.getUint32(pos+4,true);pos+=8;
  check(len%4===0&&pos+len<=buffer.byteLength,"bad chunk");
  if(type===0x4e4f534a){check(!doc,"duplicate JSON");doc=JSON.parse(new TextDecoder().decode(new Uint8Array(buffer,pos,len)))}
  if(type===0x004e4942){check(!bin,"duplicate BIN");bin=new DataView(buffer,pos,len)}
  pos+=len;
 }
 check(pos===buffer.byteLength&&doc&&bin&&doc.asset?.version==="2.0","GLB incomplete");
 check(!doc.extensionsRequired?.length,"extensions requiring external decoders unsupported");
 function accessor(id){
  const a=doc.accessors?.[id],bv=doc.bufferViews?.[a?.bufferView];
  check(a&&bv&&(!bv.buffer||bv.buffer===0)&&!a.sparse,"invalid accessor");
  const n={SCALAR:1,VEC2:2,VEC3:3,VEC4:4,MAT4:16}[a.type],
  sz={5120:1,5121:1,5122:2,5123:2,5125:4,5126:4}[a.componentType];
  check(n&&sz&&Number.isInteger(a.count)&&a.count>=0&&a.count<=MAX_VERTICES*16,"accessor shape");
  const stride=bv.byteStride||n*sz,start=(bv.byteOffset||0)+(a.byteOffset||0);
  check(stride>=n*sz&&stride<512&&start>=0&&
   start+(Math.max(0,a.count-1))*stride+n*sz<=bin.byteLength,"accessor bounds");
  const out=new Float32Array(a.count*n);
  for(let i=0;i<a.count;i++)for(let k=0;k<n;k++){
   const o=start+i*stride+k*sz;let v=0;
   switch(a.componentType){
    case 5120:v=bin.getInt8(o);break;case 5121:v=bin.getUint8(o);break;
    case 5122:v=bin.getInt16(o,true);break;case 5123:v=bin.getUint16(o,true);break;
    case 5125:v=bin.getUint32(o,true);break;case 5126:v=bin.getFloat32(o,true);break;
   }
   check(Number.isFinite(v),"nonfinite accessor");
   if(a.normalized&&a.componentType!==5126)v=Math.max(-1,v/({5120:127,5121:255,5122:32767,5123:65535,5125:4294967295}[a.componentType]));
   out[i*n+k]=v;
  }
  return {out,n,count:a.count};
 }
 const skins=doc.skins;check(skins?.length===1,"one skin expected");
 const skin=skins[0],jointCount=skin.joints.length;
 check(jointCount>0&&jointCount<=MAX_JOINTS,"invalid joint count");
 const ibm=accessor(skin.inverseBindMatrices);
 check(ibm.count===jointCount&&ibm.n===16,"inverse binds missing");
 const nodes=doc.nodes||[],roots=doc.scenes?.[doc.scene||0]?.nodes||[];
 check(roots.length>0&&nodes.length<256,"scene missing");
 const names=new Map(nodes.map((n,i)=>[n.name,i]));
 const parent=new Array(nodes.length).fill(-1);
 nodes.forEach((n,i)=>(n.children||[]).forEach(child=>{
  check(Number.isInteger(child)&&nodes[child]&&parent[child]===-1,"invalid hierarchy");parent[child]=i;
 }));
 const renderables=[];
 nodes.forEach((node,ni)=>{
  if(node.mesh===undefined||node.skin===undefined)return;
  check(node.skin===0,"different skin not supported");
  const primitives=doc.meshes?.[node.mesh]?.primitives||[];
  for(const p of primitives){
   check(p.mode===undefined||p.mode===4,"triangle meshes only");
   const a=p.attributes;
   check(a.POSITION!==undefined&&a.JOINTS_0!==undefined&&a.WEIGHTS_0!==undefined,"missing skin data");
   const positions=accessor(a.POSITION),joints=accessor(a.JOINTS_0),weights=accessor(a.WEIGHTS_0);
   const colors=a.COLOR_0!==undefined?accessor(a.COLOR_0):null;
   const indices=p.indices===undefined?Array.from({length:positions.count},(_,i)=>i):Array.from(accessor(p.indices).out);
   check(positions.n===3&&positions.count<=MAX_VERTICES&&joints.n===4&&weights.n===4
    &&joints.count===positions.count&&weights.count===positions.count,"skin streams");
   check(indices.length%3===0&&indices.length<MAX_VERTICES*12,"triangle index count");
   for(const idx of indices)check(Number.isInteger(idx)&&idx>=0&&idx<positions.count,"out-of-range index");
   renderables.push({node:ni,positions:positions.out,joints:joints.out,weights:weights.out,
    colors:colors?.out,colorStride:colors?.n||0,indices,material:p.material||0});
  }
 });
 check(renderables.length>0,"no skinned mesh");
 const animations=(doc.animations||[]).map(anim=>{
  const channels=[];
  for(const channel of anim.channels||[]){
   const spl=anim.samplers[channel.sampler],path=channel.target.path;
   check(path==="translation"||path==="rotation"||path==="scale","unhandled animation channel");
   check(spl.interpolation===undefined||spl.interpolation==="LINEAR"||spl.interpolation==="STEP","unsupported interpolation");
   const times=accessor(spl.input),values=accessor(spl.output);
   check(times.n===1&&values.count===times.count,"invalid animation tracks");
   channels.push({node:channel.target.node,path,times:times.out,values:values.out,
    n:values.n,interpolation:spl.interpolation||"LINEAR"});
  }
  return {name:anim.name||"Animation",channels,duration:Math.max(0,...channels.map(c=>c.times[c.times.length-1]||0))};
 });
 return {nodes,roots,parent,skins:skin,joints:skin.joints,
  bindMatrices:Array.from({length:jointCount},(_,i)=>Array.from(ibm.out.slice(16*i,16*i+16))),
  mesh:renderables,animations,triangleCount:renderables.reduce((n,p)=>n+p.indices.length/3,0),names};
}
function sample(track,seconds){
 const ts=track.times,vals=track.values,n=track.n;
 if(!ts.length)return null;
 let i=0;
 while(i+1<ts.length&&ts[i+1]<=seconds)i++;
 const j=Math.min(i+1,ts.length-1);
 const t=track.interpolation==="STEP"||i===j?0:Math.max(0,Math.min(1,(seconds-ts[i])/Math.max(1e-6,ts[j]-ts[i])));
 let v=Array.from({length:n},(_,k)=>vals[i*n+k]*(1-t)+vals[j*n+k]*t);
 if(track.path==="rotation"){const magnitude=Math.hypot(...v)||1;v=v.map(x=>x/magnitude)}
 return v;
}
function pose(model,time=0,mode="walk",animIndex=-1){
 const local=new Array(model.nodes.length),world=new Array(model.nodes.length);
 const override=new Map();
 if(animIndex>=0&&model.animations[animIndex]){
  const anim=model.animations[animIndex],t=anim.duration?time%anim.duration:0;
  for(const ch of anim.channels){
   const src=override.get(ch.node)||{};src[ch.path]=sample(ch,t);override.set(ch.node,src);
  }
 }else{
  const move=mode==="walk"||mode==="run"?(mode==="run"?1:.63):0;
  const crouch=mode==="crouch"?.35:0;
  const stride=time*(mode==="run"?13:8.3);
  const animations=[
   ["LeftUpLeg",[1,0,0],Math.sin(stride)*.45*move+crouch],
   ["RightUpLeg",[1,0,0],-Math.sin(stride)*.45*move+crouch],
   ["LeftLeg",[1,0,0],Math.max(0,-Math.sin(stride))*.38*move],
   ["RightLeg",[1,0,0],Math.max(0,Math.sin(stride))*.38*move],
   ["LeftShoulder",[1,0,0],Math.sin(stride)*.15*move-.13],
   ["RightShoulder",[1,0,0],-Math.sin(stride)*.15*move-.13],
   ["Spine",[1,0,0],-.10*crouch],
  ];
  for(const [name,axis,angle] of animations){
   const ni=model.names.get(name);if(ni===undefined)continue;
   override.set(ni,{rotation:quatMul(model.nodes[ni].rotation||[0,0,0,1],axisQ(axis,angle))});
  }
 }
 const seen=new Set();
 function visit(id,mat,depth){
  check(depth<80&&!seen.has(id),"bad/looping joint hierarchy");seen.add(id);
  const n=model.nodes[id],ov=override.get(id)||{};
  local[id]=matrix(n,ov.translation,ov.rotation,ov.scale);
  world[id]=mul(mat,local[id]);
  for(const child of n.children||[])visit(child,world[id],depth+1);
 }
 for(const id of model.roots)visit(id,identity(),0);
 const matrices=model.joints.map((node,i)=>mul(world[node],model.bindMatrices[i]));
 return matrices;
}
function append(g,model,actor,team="axis",time=0,lod=1){
 check(model&&model.mesh&&actor,"missing model or actor");
 const velocity=actor.moveSpeed||0;
 const mode=actor.crouch?"crouch":velocity>3.5?"run":velocity>.22?"walk":"aim";
 const palette=team==="ally"?[.34,.39,.29]:[.32,.34,.29],
 scale=.56,angle=actor.facing||0,cos=Math.cos(angle),sin=Math.sin(angle);
 const bones=pose(model,time+(actor.phase||0),mode);
 const tint=team==="ally"?.90:.77;
 let added=0;
 for(const mesh of model.mesh){
  const n=mesh.positions.length/3,transformed=new Array(n),colors=new Array(n);
  for(let i=0;i<n;i++){
   const x=mesh.positions[i*3],y=mesh.positions[i*3+1],z=mesh.positions[i*3+2];
   let px=0,py=0,pz=0,sum=0;
   for(let k=0;k<4;k++){
    const weight=mesh.weights[i*4+k],joint=mesh.joints[i*4+k];
    if(weight<=0)continue;
    check(Number.isInteger(joint)&&bones[joint],"invalid vertex joint");
    const v=at(bones[joint],x,y,z);
    px+=v[0]*weight;py+=v[1]*weight;pz+=v[2]*weight;sum+=weight;
   }
   if(sum<=.000001){px=x;py=y;pz=z;sum=1}
   const pos=[px/sum,py/sum,pz/sum];
   transformed[i]=[actor.x+(pos[0]*cos+pos[2]*sin)*scale,
    .055+pos[1]*scale+(actor.crouch?-.12:0),
    actor.z+(-pos[0]*sin+pos[2]*cos)*scale];
   const vertex=mesh.colors&&mesh.colorStride>=3?
    Array.from(mesh.colors.slice(i*mesh.colorStride,i*mesh.colorStride+3)):palette;
   colors[i]=[0,1,2].map(k=>Math.min(1,Math.max(0,vertex[k]*tint*.6+palette[k]*.4)));
  }
  for(let j=0;j<mesh.indices.length;j+=3*lod){
   const indices=mesh.indices;
   for(let t=j;t<Math.min(j+3*lod,indices.length);t+=3){
    const [a,b,c]=[indices[t],indices[t+1],indices[t+2]];
    const col=[0,1,2].map(k=>(colors[a][k]+colors[b][k]+colors[c][k])/3);
    g.tri(transformed[a],transformed[b],transformed[c],col);added++;
   }
  }
 }
 return {triangles:added,mode,jointCount:model.joints.length};
}
async function load(name="character_medium"){
 check(/^[a-z0-9_-]+$/.test(name),"invalid name");
 const res=await fetch("./assets/kenney/"+name+".glb.b64",{cache:"force-cache"});
 check(res.ok,"skin asset not available: "+res.status);
 const txt=(await res.text()).replace(/\s/g,"");
 check(txt.length<MAX*1.35&&/^[a-zA-Z0-9+/]*={0,2}$/.test(txt),"skin asset too large or invalid");
 const bytes=Uint8Array.from(atob(txt),c=>c.charCodeAt(0));
 return parse(bytes.buffer);
}
root.WPSkinning=Object.freeze({load,parse,pose,append});
})(window);