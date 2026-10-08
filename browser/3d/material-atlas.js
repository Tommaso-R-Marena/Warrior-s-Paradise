/* Warrior's Paradise — deterministic, self-hosted material texture atlas.
 * RGBA diffuse (alpha = roughness), RG encoded bump normals. Four tiles:
 * pasture, churned mud, limestone, worn brick. No external CDN/CPU server.
 * Generated originals serve as a real texture/normal/roughness pipeline
 * and remain visual fallbacks for licensed photographic PBR maps.
 */
(function(root){"use strict";
const SIZE=128,COUNT=4;
function hash(x,y,seed){let n=(x*374761393+y*668265263+seed*1442695041)|0;
 n=Math.imul(n^(n>>>13),1274126177);return ((n^(n>>>16))>>>0)/4294967295;}
function noise(x,y,seed,size){
 let px=Math.floor(x),py=Math.floor(y),fx=x-px,fy=y-py;
 fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy);
 const h=(ix,iy)=>hash(((ix%size)+size)%size,((iy%size)+size)%size,seed);
 return (1-fy)*((1-fx)*h(px,py)+fx*h(px+1,py))+
  fy*((1-fx)*h(px,py+1)+fx*h(px+1,py+1));
}
function height(x,y,kind){
 const nx=((x%SIZE)+SIZE)%SIZE,ny=((y%SIZE)+SIZE)%SIZE;
 const f=(sz,seed)=>noise(nx*sz/SIZE,ny*sz/SIZE,seed,sz);
 const broad=f(4,kind+2),mid=f(16,kind+29),fine=f(64,kind+101);
 return .54*broad+.31*mid+.15*fine;
}
function generate(){
 const width=SIZE*COUNT,heightPx=SIZE;
 const color=new Uint8Array(width*heightPx*4),
  normal=new Uint8Array(width*heightPx*4);
 const palette=[
  [[53,85,47],[94,109,55]],   // Normandy pasture
  [[75,61,47],[110,92,67]],  // rutted earth
  [[115,113,102],[168,160,136]],// pale farm limestone
  [[106,65,52],[156,98,73]]   // weathered brick
 ];
 for(let kind=0;kind<COUNT;kind++)for(let y=0;y<SIZE;y++)for(let x=0;x<SIZE;x++){
  const h=height(x,y,kind),detail=hash(x,y,kind+729),b=palette[kind];
  const lx=h*.74+detail*.26;
  const isBrick=kind===3,offset=((y/13|0)%2)*8;
  const joint=isBrick&&(((x+offset)%32<1)||y%13<1);
  const rock=kind===2&&hash(x/11|0,y/8|0,kind)> .85;
  const fleck=kind===0&&detail>.92;
  const grain=kind===1&&detail>.81;
  const i=(y*width+kind*SIZE+x)*4;
  for(let k=0;k<3;k++){
   const value=b[0][k]*(1-lx)+b[1][k]*lx;
   const accent=joint?.62:rock?.84:fleck?1.31:grain?1.17:1;
   color[i+k]=Math.max(0,Math.min(255,Math.round(value*accent)));
  }
  color[i+3]=Math.round(205+(h-.5)*40+(kind===1?20:0)); // roughness
  const dx=height(x+1,y,kind)-height(x-1,y,kind);
  const dy=height(x,y+1,kind)-height(x,y-1,kind);
  normal[i]=Math.round(128-Math.max(-.5,Math.min(.5,dx*3.7))*127);
  normal[i+1]=Math.round(128-Math.max(-.5,Math.min(.5,dy*3.7))*127);
  normal[i+2]=255;
  normal[i+3]=255;
 }
 return {width,height:heightPx,tiles:COUNT,color,normal};
}
function upload(gl){
 if(!gl?.createTexture||!gl.texImage2D)return null;
 const atlas=generate();
 function texture(data){
  const id=gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D,id);
  gl.pixelStorei(gl.UNPACK_ALIGNMENT,1);
  gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,atlas.width,atlas.height,0,gl.RGBA,gl.UNSIGNED_BYTE,data);
  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR_MIPMAP_LINEAR);
  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);
  gl.generateMipmap(gl.TEXTURE_2D);
  return id;
 }
 return {diffuse:texture(atlas.color),normal:texture(atlas.normal),size:[atlas.width,atlas.height],tiles:atlas.tiles};
}
function bind(gl,uniforms,textures){
 if(!textures||!gl?.activeTexture)return false;
 gl.activeTexture(gl.TEXTURE0);gl.bindTexture(gl.TEXTURE_2D,textures.diffuse);
 gl.activeTexture(gl.TEXTURE1);gl.bindTexture(gl.TEXTURE_2D,textures.normal);
 gl.uniform1i(uniforms.diffuse,0);gl.uniform1i(uniforms.normal,1);return true;
}
root.WPMaterialAtlas=Object.freeze({generate,upload,bind});
})(window);