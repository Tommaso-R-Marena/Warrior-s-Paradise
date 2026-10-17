/* Warrior's Paradise: bounded offline UV/PBR + secondary-UV lightmap renderer.
 * Zero server-side GPU, paid services or remote texture dependencies.
 */
(function(root){"use strict";
const MAX_BYTES=5*1024*1024,MAX_VERTS=180000;
const LICENSES=new Set(["CC0-1.0","CC-BY-3.0","CC-BY-4.0","CC-BY-SA-3.0","CC-BY-SA-4.0","original"]);
function req(x,msg){if(!x)throw Error("WP PBR: "+msg);}
const safeId=s=>typeof s==="string"&&/^[a-z0-9][a-z0-9_-]{0,63}$/.test(s);
function fnv(bytes){let h=0x811c9dc5;for(let i=0;i<bytes.length;i++)h=Math.imul((h^bytes[i])>>>0,0x01000193)>>>0;return h.toString(16).padStart(8,"0");}
function bytes64(s,limit=MAX_BYTES){
 req(typeof s==="string"&&s.length>0&&s.length<=Math.ceil(limit*4/3)+4&&s.length%4===0&&/^[A-Za-z0-9+/]*={0,2}$/.test(s),"unsafe base64");
 const data=Uint8Array.from(atob(s),c=>c.charCodeAt(0));
 req(data.length>0&&data.length<=limit,"asset bytes exceed cap");return data;
}
function safePath(s){
 req(typeof s==="string"&&/^\.\/assets\/(?:materials|pbr)\/[a-zA-Z0-9_-]+\.(?:jpg\.b64|png\.b64|wptex\.json)$/.test(s),"unsafe texture path");return s;
}
function unpack(pack,tier){
 req(pack&&pack.schema==="wp-pbr-mesh-v2"&&safeId(pack.id),"mesh version or identity");
 req(pack.source&&LICENSES.has(pack.source.license)&&typeof pack.source.author==="string"&&
  pack.source.author.trim().length>0&&pack.source.author.length<160,"license and attribution required");
 const min=pack.bounds?.min,max=pack.bounds?.max;
 req(Array.isArray(min)&&Array.isArray(max)&&min.length===3&&max.length===3&&
  min.every((x,i)=>Number.isFinite(x)&&Number.isFinite(max[i])&&x<=max[i]&&Math.abs(x)<250&&Math.abs(max[i])<250),"bounds");
 const lod=pack.lods?.[tier];
 req(lod&&Number.isInteger(lod.vertices)&&lod.vertices>=3&&lod.vertices<=MAX_VERTS&&lod.vertices%3===0,"triangle count");
 const data=bytes64(lod.data);
 req(data.length===lod.vertices*18&&fnv(data)===lod.fnv32,"checksum/stride");
 const dv=new DataView(data.buffer,data.byteOffset,data.byteLength);
 const result=new Float32Array(lod.vertices*10);
 for(let i=0;i<lod.vertices;i++){
  const off=i*18,j=i*10;
  for(let a=0;a<3;a++)result[j+a]=min[a]+(max[a]-min[a])*dv.getUint16(off+a*2,true)/65535;
  const n=[dv.getInt8(off+6),dv.getInt8(off+7),dv.getInt8(off+8)],len=Math.hypot(...n);
  req(len>70&&len<220&&data[off+9]===0,"invalid normals");
  for(let a=0;a<3;a++)result[j+3+a]=n[a]/len;
  for(let a=0;a<4;a++)result[j+6+a]=dv.getUint16(off+10+a*2,true)/65535;
 }
 return result;
}
function manifestValid(doc){
 req(doc&&doc.schema==="wp-pbr-scene-v2"&&Array.isArray(doc.groups)&&doc.groups.length>0&&doc.groups.length<=6,"scene schema");
 const names=new Set;
 for(const group of doc.groups){
  req(safeId(group.mesh)&&!names.has(group.mesh),"duplicate/unsafe mesh id");names.add(group.mesh);
  req(group.textures&&typeof group.textures==="object","missing textures");
  for(const channel of ["albedo","normal","roughness","lightmap"])safePath(group.textures[channel]);
  if(group.textures.metallic)safePath(group.textures.metallic);
  req(Array.isArray(group.instances)&&group.instances.length>0&&group.instances.length<=5,"instance cap");
  for(const v of group.instances){
   req(v&&["x","y","z","yaw","scale"].every(k=>Number.isFinite(v[k]))&&
    v.scale>.05&&v.scale<20&&Math.abs(v.x)<47&&Math.abs(v.z)<47&&Math.abs(v.y)<25&&Math.abs(v.yaw)<7&&
    v.collider==="existing-building","unsafe transform or nonphysical cover");
  }
 }return doc;
}
function place(data,instances){
 const n=data.length/10,out=new Float32Array(data.length*instances.length);
 req(out.length<=MAX_VERTS*10*4,"placed vertex cap");
 for(let k=0;k<instances.length;k++){
  const {x,y,z,yaw,scale}=instances[k],c=Math.cos(yaw),s=Math.sin(yaw);
  for(let i=0;i<n;i++){
   const a=i*10,b=(k*n+i)*10,X=data[a]*scale,Z=data[a+2]*scale;
   out[b]=x+c*X+s*Z;out[b+1]=y+data[a+1]*scale;out[b+2]=z-s*X+c*Z;
   out[b+3]=c*data[a+3]+s*data[a+5];out[b+4]=data[a+4];out[b+5]=-s*data[a+3]+c*data[a+5];
   out.set(data.subarray(a+6,a+10),b+6);
  }
 }return out;
}
async function loadScene(fetcher=fetch){
 const get=async url=>{const r=await fetcher(url,{cache:"force-cache"});req(r.ok,"missing "+url);return r.json();};
 const doc=manifestValid(await get("./assets/pbr/scene.json"));
 const groups=await Promise.all(doc.groups.map(async group=>{
  const pack=await get("./assets/pbr/"+group.mesh+".wppbr.json");
  req(pack.id===group.mesh,"asset id mismatch");
  return {...group,source:pack.source,bounds:pack.bounds,
   medium:place(unpack(pack,"medium"),group.instances),
   high:place(unpack(pack,"high"),group.instances)};
 }));
 const sum=t=>groups.reduce((n,g)=>n+g[t].length/30,0);
 req(sum("high")<=22000&&sum("medium")<=10000,"scene triangle budget");
 return{groups,mediumTriangles:sum("medium"),highTriangles:sum("high")};
}
const VS=[
"#version 300 es","precision highp float;",
"layout(location=0) in vec3 p;layout(location=1) in vec3 n;",
"layout(location=2) in vec2 uv;layout(location=3) in vec2 uvLight;",
"uniform mat4 uVP;out vec3 vP;out vec3 vN;out vec2 vUv;out vec2 vLight;",
"void main(){vP=p;vN=n;vUv=uv;vLight=uvLight;gl_Position=uVP*vec4(p,1.);}"
].join("\n");
const FS=[
"#version 300 es","precision highp float;",
"in vec3 vP;in vec3 vN;in vec2 vUv;in vec2 vLight;",
"uniform sampler2D uAlbedo,uNormal,uRough,uMetal,uLight;",
"uniform vec3 uEye;uniform float uMetalAvailable;out vec4 outColor;",
"vec3 worldNormal(vec3 n){",
" vec3 dp1=dFdx(vP),dp2=dFdy(vP);",
" vec2 duv1=dFdx(vUv),duv2=dFdy(vUv);",
" float det=duv1.x*duv2.y-duv1.y*duv2.x;",
" if(abs(det)<.000001)return n;",
" vec3 t=normalize((dp1*duv2.y-dp2*duv1.y)*sign(det));",
" vec3 b=normalize(cross(n,t));",
" vec3 map=normalize(texture(uNormal,vUv).xyz*2.-1.);",
" return normalize(mat3(t,b,n)*map);",
"}",
"void main(){",
" vec3 N=worldNormal(normalize(vN));",
" vec3 A=texture(uAlbedo,vUv).rgb;",
" float rough=clamp(texture(uRough,vUv).r,.09,1.);",
" float metal=uMetalAvailable>.5?clamp(texture(uMetal,vUv).r,0.,1.):0.;",
" vec3 irradiance=clamp(texture(uLight,clamp(vLight,vec2(.005),vec2(.995))).rgb,vec3(.015),vec3(1.2));",
" vec3 L=normalize(vec3(.53,.88,-.36));vec3 V=normalize(uEye-vP);",
" vec3 H=normalize(V+L);float nl=max(0.,dot(N,L)),nv=max(.001,dot(N,V));",
" float nh=max(0.,dot(N,H)),vh=max(0.,dot(V,H));",
" float a=rough*rough,a2=a*a,d=(nh*nh*(a2-1.)+1.);",
" float D=a2/max(.001,3.14159265*d*d);",
" float k=(rough+1.)*(rough+1.)/8.;",
" float G=nl/(nl*(1.-k)+k)*nv/(nv*(1.-k)+k);",
" vec3 F0=mix(vec3(.04),A,metal),F=F0+(1.-F0)*pow(1.-vh,5.);",
" vec3 spec=F*D*G/max(.015,4.*nl*nv);",
" vec3 color=A*(1.-metal)*(1.-F)*irradiance+spec*nl*.42;",
" float haze=clamp((distance(uEye,vP)-22.)/88.,0.,.8);",
" color=mix(color,vec3(.60,.67,.65),haze);",
" color=(color*(2.51*color+.03))/(color*(2.43*color+.59)+.14);",
" outColor=vec4(pow(max(color,vec3(0.)),vec3(1./2.2)),1.);",
"}"
].join("\n");
function shader(gl,kind,src){
 const s=gl.createShader(kind);gl.shaderSource(s,src);gl.compileShader(s);
 if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))throw Error("PBR shader: "+gl.getShaderInfoLog(s));
 return s;
}
function compile(gl){
 const p=gl.createProgram(),vs=shader(gl,gl.VERTEX_SHADER,VS),fs=shader(gl,gl.FRAGMENT_SHADER,FS);
 gl.attachShader(p,vs);gl.attachShader(p,fs);gl.linkProgram(p);
 if(!gl.getProgramParameter(p,gl.LINK_STATUS))throw Error("PBR link: "+gl.getProgramInfoLog(p));
 gl.deleteShader(vs);gl.deleteShader(fs);return p;
}
async function readTexture(fetcher,path){
 safePath(path);
 const r=await fetcher(path,{cache:"force-cache"});req(r.ok,"missing texture "+path);
 if(path.endsWith(".wptex.json")){
  const p=await r.json();
  req(p.schema==="wp-linear-texture-v1"&&Number.isInteger(p.width)&&Number.isInteger(p.height)&&
   p.width>=4&&p.height>=4&&p.width<=2048&&p.height<=2048,"lightmap dimensions");
  const data=bytes64(p.rgba,2048*2048*4);
  req(data.length===p.width*p.height*4&&fnv(data)===p.fnv32,"lightmap checksum");
  return{data,width:p.width,height:p.height};
 }
 req(typeof createImageBitmap==="function","image bitmap unsupported");
 const raw=bytes64((await r.text()).replace(/\s/g,""));
 req(raw[0]===0xff&&raw[1]===0xd8||raw[0]===0x89&&raw[1]===0x50,"unsupported picture");
 const bmp=await createImageBitmap(new Blob([raw],{type:path.endsWith(".jpg.b64")?"image/jpeg":"image/png"}),{imageOrientation:"flipY"});
 req(bmp.width>=4&&bmp.width<=2048&&bmp.height>=4&&bmp.height<=2048,"texture size");
 return{bitmap:bmp,width:bmp.width,height:bmp.height};
}
function gpuTexture(gl,obj,kind){
 const t=gl.createTexture();req(t,"texture allocation");
 gl.bindTexture(gl.TEXTURE_2D,t);gl.pixelStorei(gl.UNPACK_ALIGNMENT,1);
 gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,kind==="lightmap"?gl.CLAMP_TO_EDGE:gl.REPEAT);
 gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,kind==="lightmap"?gl.CLAMP_TO_EDGE:gl.REPEAT);
 gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR_MIPMAP_LINEAR);
 gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);
 if(obj.data)gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA8,obj.width,obj.height,0,gl.RGBA,gl.UNSIGNED_BYTE,obj.data);
 else gl.texImage2D(gl.TEXTURE_2D,0,kind==="albedo"?gl.SRGB8_ALPHA8:gl.RGBA,
  gl.RGBA,gl.UNSIGNED_BYTE,obj.bitmap);
 gl.generateMipmap(gl.TEXTURE_2D);obj.bitmap?.close?.();return t;
}
function gpuMesh(gl,geometry){
 const v=gl.createVertexArray(),b=gl.createBuffer();
 gl.bindVertexArray(v);gl.bindBuffer(gl.ARRAY_BUFFER,b);gl.bufferData(gl.ARRAY_BUFFER,geometry,gl.STATIC_DRAW);
 for(const [index,count,offset] of [[0,3,0],[1,3,12],[2,2,24],[3,2,32]]){
  gl.enableVertexAttribArray(index);gl.vertexAttribPointer(index,count,gl.FLOAT,false,40,offset);
 }
 gl.bindVertexArray(null);return{v,b,count:geometry.length/10};
}
async function prepare(gl,fetcher=fetch){
 let p,groups=[],textures=[];
 try{
  const scene=await loadScene(fetcher);
  p=compile(gl);
  const loc=Object.fromEntries(["uVP","uEye","uAlbedo","uNormal","uRough","uMetal","uLight","uMetalAvailable"]
   .map(name=>[name,gl.getUniformLocation(p,name)]));
  const cache=new Map;
  for(const g of scene.groups){
   const maps={};
   for(const channel of ["albedo","normal","roughness","metallic","lightmap"]){
    if(!g.textures[channel])continue;
    const uri=g.textures[channel];
    if(!cache.has(uri)){
     const obj=await readTexture(fetcher,uri);
     const tx=gpuTexture(gl,obj,channel);textures.push(tx);cache.set(uri,tx);
    }maps[channel]=cache.get(uri);
   }
   groups.push({...g,maps,meshes:{medium:gpuMesh(gl,g.medium),high:gpuMesh(gl,g.high)}});
  }
  let frames=0,rendered=0,lastTier="none";
  const cleanup=()=>{for(const g of groups)for(const m of Object.values(g.meshes)){
   gl.deleteVertexArray(m.v);gl.deleteBuffer(m.b);
  }textures.forEach(tx=>gl.deleteTexture(tx));gl.deleteProgram(p);};
  return Object.freeze({
   render(vp,eye,tier){
    if(tier!=="high")return 0;
    gl.useProgram(p);gl.uniformMatrix4fv(loc.uVP,false,vp);gl.uniform3fv(loc.uEye,eye);
    let total=0;
    for(const g of groups){
     const channels=["albedo","normal","roughness","metallic","lightmap"];
     const uniforms=["uAlbedo","uNormal","uRough","uMetal","uLight"];
     for(let i=0;i<channels.length;i++){
      gl.activeTexture(gl.TEXTURE4+i);gl.bindTexture(gl.TEXTURE_2D,g.maps[channels[i]]||null);
      gl.uniform1i(loc[uniforms[i]],4+i);
     }
     gl.uniform1f(loc.uMetalAvailable,g.maps.metallic?1:0);
     const m=g.meshes[tier];gl.bindVertexArray(m.v);gl.drawArrays(gl.TRIANGLES,0,m.count);total+=m.count/3;
    }gl.bindVertexArray(null);frames++;rendered+=total;lastTier=tier;return total;
   },
   stats(){return{ready:true,frames,rendered,lastTier,groups:groups.length,
    highTriangles:scene.highTriangles,mediumTriangles:scene.mediumTriangles,
    sources:groups.map(g=>({id:g.mesh,...g.source}))};},
   dispose:cleanup
  });
 }catch(error){
  for(const g of groups)for(const m of Object.values(g.meshes)){gl.deleteVertexArray(m.v);gl.deleteBuffer(m.b);}
  for(const t of textures)gl.deleteTexture(t);
  if(p)gl.deleteProgram(p);throw error;
 }
}
root.WPPBRLightmaps=Object.freeze({prepare,unpack,manifestValid,loadScene,place,readTexture,fnv,vertexBytes:18});
})(window);
