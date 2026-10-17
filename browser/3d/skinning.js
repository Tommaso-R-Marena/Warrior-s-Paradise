/* Warrior's Paradise — controlled glTF 2.0 skinned-mesh runtime.
 * Built for tiny, provenance-audited CC0 characters. No CDN or native module.
 * CPU skinning produces triangles in the existing WebGL2 geometry pipeline.
 * Supports joint hierarchy, inverse bind matrices, 4 weighted influences,
 * quaternion poses, and LINEAR/STEP glTF animation translation/rotation tracks.
 */
(function(root){"use strict";
const MAX=4*1024*1024, MAX_JOINTS=96, MAX_VERTICES=16000;
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
   // Imported historical meshes already contain an authored TEXCOORD_0.
   // Retain it through CPU skinning, rather than discarding fabric UVs.
   const uv=a.TEXCOORD_0!==undefined?accessor(a.TEXCOORD_0):null;
   check(!uv||(uv.n===2&&uv.count===positions.count),"invalid UV0 stream");
   const indices=p.indices===undefined?Array.from({length:positions.count},(_,i)=>i):Array.from(accessor(p.indices).out);
   check(positions.n===3&&positions.count<=MAX_VERTICES&&joints.n===4&&weights.n===4
    &&joints.count===positions.count&&weights.count===positions.count,"skin streams");
   check(indices.length%3===0&&indices.length<MAX_VERTICES*12,"triangle index count");
   for(const idx of indices)check(Number.isInteger(idx)&&idx>=0&&idx<positions.count,"out-of-range index");
   renderables.push({node:ni,positions:positions.out,joints:joints.out,weights:weights.out,
    colors:colors?.out,colorStride:colors?.n||0,uv:uv?.out||null,
    indices,material:p.material||0});
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
 const heights=renderables.flatMap(m=>Array.from(m.positions).filter((_,i)=>i%3===1));
 const standingHeight=heights.length?Math.max(...heights)-Math.min(...heights):0;
 return {nodes,roots,parent,skins:skin,joints:skin.joints,standingHeight,
  materials:doc.materials||[],
  bindMatrices:Array.from({length:jointCount},(_,i)=>Array.from(ibm.out.slice(16*i,16*i+16))),
  mesh:renderables,animations,triangleCount:renderables.reduce((n,p)=>n+p.indices.length/3,0),names};
}
// Shortest-arc quaternion SLERP eliminates linear-quaternion foot and shoulder pops.
function slerpQuaternion(a,b,t){
 let cosine=a.reduce((sum,x,i)=>sum+x*b[i],0),end=b;
 if(cosine<0){cosine=-cosine;end=b.map(x=>-x);}
 cosine=Math.max(-1,Math.min(1,cosine));
 if(cosine>.9995){
  const q=a.map((x,i)=>x+(end[i]-x)*t),len=Math.hypot(...q)||1;
  return q.map(v=>v/len);
 }
 const theta=Math.acos(cosine),sinTheta=Math.sin(theta);
 const u=Math.sin((1-t)*theta)/sinTheta,v=Math.sin(t*theta)/sinTheta;
 return a.map((x,i)=>x*u+end[i]*v);
}
function sample(track,seconds){
 const ts=track.times,vals=track.values,n=track.n;
 if(!ts.length)return null;
 let i=0;
 while(i+1<ts.length&&ts[i+1]<=seconds)i++;
 const j=Math.min(i+1,ts.length-1);
 const t=track.interpolation==="STEP"||i===j?0:Math.max(0,Math.min(1,(seconds-ts[i])/Math.max(1e-6,ts[j]-ts[i])));
 if(track.path==="rotation"){
  const a=Array.from(vals.slice(i*n,i*n+n)),b=Array.from(vals.slice(j*n,j*n+n));
  return slerpQuaternion(a,b,t);
 }
 return Array.from({length:n},(_,k)=>vals[i*n+k]*(1-t)+vals[j*n+k]*t);
}
function trackOverrides(model,time,mode,animIndex){
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
   // The real Allied asset uses Blender's thigh.L/shin.L naming, not
   // Mixamo's LeftUpLeg/LeftLeg. Without aliases its crouch was only a
   // whole-body drop instead of moving knees, hips and weapon-bearing arms.
   ["thigh.L",[1,0,0],Math.sin(stride)*.45*move+crouch*.71],
   ["thigh.R",[1,0,0],-Math.sin(stride)*.45*move+crouch*.71],
   ["shin.L",[1,0,0],Math.max(0,-Math.sin(stride))*.38*move-crouch*.65],
   ["shin.R",[1,0,0],Math.max(0,Math.sin(stride))*.38*move-crouch*.65],
   ["upper_arm.L",[1,0,0],Math.sin(stride)*.15*move-.13-crouch*.18],
   ["upper_arm.R",[1,0,0],-Math.sin(stride)*.15*move-.13-crouch*.18],
   ["spine.001",[1,0,0],-.19*crouch],
  ];
  for(const [name,axis,angle] of animations){
   const ni=model.names.get(name);if(ni===undefined)continue;
   override.set(ni,{rotation:quatMul(model.nodes[ni].rotation||[0,0,0,1],axisQ(axis,angle))});
  }
 }
 return override;
}
function pose(model,time=0,mode="walk",animIndex=-1,transition=null){
 const local=new Array(model.nodes.length),world=new Array(model.nodes.length);
 const override=trackOverrides(model,time,mode,animIndex);
 // Blend TRS *before* traversing the bone hierarchy. Interpolating skinned
 // positions/matrices afterwards causes foot pops and shrinking joints.
 if(transition&&transition.alpha<1){
  const before=trackOverrides(model,time,transition.fromMode,transition.fromClip);
  const a=Math.max(0,Math.min(1,transition.alpha));
  for(const id of new Set([...before.keys(),...override.keys()])){
   const node=model.nodes[id],from=before.get(id)||{},to=override.get(id)||{};
   const blendVec=(key,defaultValue)=>{
    const v0=from[key]||node[key]||defaultValue;
    const v1=to[key]||node[key]||defaultValue;
    return v0.map((v,k)=>v+(v1[k]-v)*a);
   };
   override.set(id,{
    translation:blendVec("translation",[0,0,0]),
    rotation:slerpQuaternion(from.rotation||node.rotation||[0,0,0,1],
       to.rotation||node.rotation||[0,0,0,1],a),
    scale:blendVec("scale",[1,1,1])
   });
  }
 }
 // Animation-layer motion: subtle breathing on idle/aim and torso follow-through.
 // Composes with historical authored clips instead of replacing locomotion.
 const breathing=Math.sin(time*2.15)*.012;
 const lean=mode==="run"?.023:mode==="walk"?.009:0;
 for(const name of ["Spine","Spine1","Chest","mixamorig:Spine","mixamorig:Spine1"]){
  const id=model.names.get(name);if(id===undefined)continue;
  const node=model.nodes[id],ov=override.get(id)||{};
  const base=ov.rotation||node.rotation||[0,0,0,1];
  ov.rotation=quatMul(base,axisQ([1,0,0],breathing+lean*Math.sin(time*7.2)));
  override.set(id,ov);
  break;
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
// This state is purely visual: no AI, collision, health or hit registration
// depends on frame timing or whether an actor happened to be rendered.
const states=new WeakMap();
const MOTION_BLEND_SECONDS=.24;
function transitionFor(actor,model,mode,clip,now){
 let state=states.get(actor);
 if(!state||state.model!==model){
  state={model,mode,clip,start:now,transition:null,stance:mode==="crouch"?1:0,last:now};
  states.set(actor,state);
 }
 if(mode!==state.mode||clip!==state.clip){
  state.transition={fromMode:state.mode,fromClip:state.clip,started:now};
  state.mode=mode;state.clip=clip;
 }
 const delta=Math.max(0,Math.min(.12,now-state.last));
 state.last=now;
 const target=mode==="crouch"?1:0;
 state.stance+=Math.sign(target-state.stance)*Math.min(Math.abs(target-state.stance),delta/.24);
 const t=state.transition;
 let transition=null;
 if(t){
  const u=Math.max(0,Math.min(1,(now-t.started)/MOTION_BLEND_SECONDS));
  // Smoothstep acceleration/deceleration avoids popping at action boundaries.
  if(u>=1)state.transition=null;
  else transition={fromMode:t.fromMode,fromClip:t.fromClip,alpha:u*u*(3-2*u)};
 }
 return {transition,crouch:state.stance};
}
// UV0-anchored fabric weave and dyed field-uniform mottling. This is an
// offline-like procedural albedo approximation baked to vertex colors,
// NOT sampled photographic baseColor/normal/roughness PBR textures.
// It follows the clothing as bones deform, unlike world-space noise.
function clothAlbedo(uv,team){
 if(!uv||!uv.every(Number.isFinite))return 1;
 const [u,v]=uv,thread=Math.sin(u*6.2831853*45)*Math.sin(v*6.2831853*57);
 const patches=Math.sin(u*17+Math.sin(v*13))*Math.cos(v*22-u*5);
 return Math.max(.76,Math.min(1.18,
  (team==="axis"?.91:.96)+thread*.055+patches*.065));
}
function append(g,model,actor,team="axis",time=0,lod=1){
 check(model&&model.mesh&&actor,"missing model or actor");
 const velocity=actor.moveSpeed||0;
 const mode=actor.crouch?"crouch":velocity>3.5?"run":velocity>.22?"walk":"aim";
 const palette=team==="ally"?[.34,.39,.29]:[.32,.34,.29],
 scale=model.isHistorical&&model.standingHeight>0?Math.min(2.5,2.12/model.standingHeight):.56,
 angle=actor.facing||0,cos=Math.cos(angle),sin=Math.sin(angle);
 const action=mode==="run"?/run|sprint/i:mode==="walk"?/walk|patrol|run/i:
  mode==="crouch"?/crawl|crouch/i:/idle|aim|stand/i;
 const clip=model.isHistorical?model.animations.findIndex(a=>action.test(a.name)):-1;
 const motion=transitionFor(actor,model,mode,clip,time);
 const bones=pose(model,time+(actor.phase||0),mode,clip,motion.transition);
 const tint=team==="ally"?.90:.77;
 let added=0;
 for(const mesh of model.mesh){
  const n=mesh.positions.length/3,transformed=new Array(n),colors=new Array(n);
  // Trigonometric textile detail is computed once per source UV mesh/team,
  // never during repeated CPU skinning frames on low-power hardware.
  if(mesh.uv&&!mesh.uvClothCache)mesh.uvClothCache={};
  if(mesh.uv&&!mesh.uvClothCache[team]){
   const a=new Float32Array(n);
   for(let i=0;i<n;i++)a[i]=clothAlbedo([mesh.uv[i*2],mesh.uv[i*2+1]],team);
   mesh.uvClothCache[team]=a;
  }
  const clothFactor=mesh.uv?mesh.uvClothCache[team]:null;
  const baseTint=model.materials?.[mesh.material]?.pbrMetallicRoughness?.baseColorFactor;
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
    (actor.groundY||0)+.055+pos[1]*scale-motion.crouch*.12,
    actor.z+(-pos[0]*sin+pos[2]*cos)*scale];
   const vertex=mesh.colors&&mesh.colorStride>=3?
    Array.from(mesh.colors.slice(i*mesh.colorStride,i*mesh.colorStride+3)):palette;
   const cloth=clothFactor?clothFactor[i]:1;
   colors[i]=[0,1,2].map(k=>Math.min(1,Math.max(.018,
     (vertex[k]*tint*.6+palette[k]*.4)*
     (baseTint&&Number.isFinite(baseTint[k])?(.75+.25*baseTint[k]):1)*cloth)));
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
 return {triangles:added,mode,jointCount:model.joints.length,
  uvTexturedTriangles:model.mesh.reduce((n,m)=>n+(m.uv?m.indices.length/3:0),0),
  blended:!!motion.transition};
}
async function load(name="character_medium"){
 check(/^[a-z0-9_-]+$/.test(name),"invalid name");
 const folder=/^(axis_ww2|allied_ww2)$/.test(name)?"oga":"kenney";
 const res=await fetch("./assets/"+folder+"/"+name+".glb.b64",{cache:"force-cache"});
 check(res.ok,"skin asset not available: "+res.status);
 const txt=(await res.text()).replace(/\s/g,"");
 check(txt.length<MAX*1.35&&/^[a-zA-Z0-9+/]*={0,2}$/.test(txt),"skin asset too large or invalid");
 const bytes=Uint8Array.from(atob(txt),c=>c.charCodeAt(0));
 const result=parse(bytes.buffer);result.isHistorical=folder==="oga";return result;
}
root.WPSkinning=Object.freeze({
 load,parse,pose,append,slerpQuaternion,clothAlbedo,transitionFor,MOTION_BLEND_SECONDS});
})(window);