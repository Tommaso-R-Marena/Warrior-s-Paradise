/* Directional shadow map for the High graphics path.
 * Renders the existing static world into one 512px depth texture on changes,
 * not once per frame. Low and Medium never allocate this framebuffer.
 * World physics remains authoritative; shadows are presentation only.
 */
(function(root){"use strict";
const SIZE=512;
const SUN=[.53,.88,-.36];
const VS=`#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
uniform mat4 uLightVP;
void main(){gl_Position=uLightVP*vec4(aPos,1.);}`;
const FS=`#version 300 es
precision mediump float;
void main(){}`;
function dot(a,b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
function norm(v){const d=Math.hypot(...v)||1;return v.map(x=>x/d);}
function cross(a,b){return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]];}
function matrix(){
 const eye=SUN.map((s,i)=>s*125+(i===1?2:0)),forward=norm(eye.map((e,i)=>[0,1,0][i]-e));
 const right=norm(cross(forward,[0,1,0])),up=cross(right,forward);
 const view=[right[0],up[0],-forward[0],0,
  right[1],up[1],-forward[1],0,right[2],up[2],-forward[2],0,
  -dot(right,eye),-dot(up,eye),dot(forward,eye),1];
 const l=-88,r=88,b=-88,t=88,n=.1,f=280;
 const projection=[2/(r-l),0,0,0,0,2/(t-b),0,0,0,0,-2/(f-n),0,
  -(r+l)/(r-l),-(t+b)/(t-b),-(f+n)/(f-n),1];
 const product=new Float32Array(16);
 for(let c=0;c<4;c++)for(let row=0;row<4;row++)
  for(let k=0;k<4;k++)product[c*4+row]+=projection[k*4+row]*view[c*4+k];
 return product;
}
function create(gl){
 if(!gl||!gl.createFramebuffer||!gl.createTexture||!gl.drawBuffers||
    !gl.texImage2D||!gl.framebufferTexture2D||!gl.checkFramebufferStatus)
  return null;
 let texture=null,fbo=null,program=null,dirty=true,draws=0,failures=0;
 try{
  const compile=(type,src)=>{
   const s=gl.createShader(type);gl.shaderSource(s,src);gl.compileShader(s);
   if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))throw Error(gl.getShaderInfoLog(s));
   return s;
  };
  program=gl.createProgram();
  gl.attachShader(program,compile(gl.VERTEX_SHADER,VS));
  gl.attachShader(program,compile(gl.FRAGMENT_SHADER,FS));
  gl.linkProgram(program);
  if(!gl.getProgramParameter(program,gl.LINK_STATUS))throw Error(gl.getProgramInfoLog(program));
  texture=gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D,texture);
  gl.texImage2D(gl.TEXTURE_2D,0,gl.DEPTH_COMPONENT24,SIZE,SIZE,0,gl.DEPTH_COMPONENT,gl.UNSIGNED_INT,null);
  for(const coord of [gl.TEXTURE_WRAP_S,gl.TEXTURE_WRAP_T])
   gl.texParameteri(gl.TEXTURE_2D,coord,gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.NEAREST);
  fbo=gl.createFramebuffer();
  gl.bindFramebuffer(gl.FRAMEBUFFER,fbo);
  gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.DEPTH_ATTACHMENT,gl.TEXTURE_2D,texture,0);
  gl.drawBuffers([gl.NONE]);gl.readBuffer(gl.NONE);
  if(gl.checkFramebufferStatus(gl.FRAMEBUFFER)!==gl.FRAMEBUFFER_COMPLETE)
   throw Error("Shadow framebuffer incomplete");
 }catch(error){
  if(fbo)gl.deleteFramebuffer?.(fbo);
  if(texture)gl.deleteTexture?.(texture);
  if(program)gl.deleteProgram?.(program);
  return null;
 }finally{
  gl.bindFramebuffer(gl.FRAMEBUFFER,null);
  gl.bindTexture(gl.TEXTURE_2D,null);
 }
 const vp=matrix(),lightLoc=gl.getUniformLocation(program,"uLightVP");
 return Object.freeze({
  invalidate(){dirty=true;},
  renderStatic(mesh,screenWidth,screenHeight){
   if(!dirty)return true;
   if(!mesh?.vao||!Number.isFinite(mesh.count)||mesh.count<=0)return false;
   // Restore all affected GL state after the depth-only pass.
   gl.bindFramebuffer(gl.FRAMEBUFFER,fbo);
   try{
    gl.viewport(0,0,SIZE,SIZE);gl.clear(gl.DEPTH_BUFFER_BIT);
    gl.enable(gl.DEPTH_TEST);
    gl.useProgram(program);gl.uniformMatrix4fv(lightLoc,false,vp);
    gl.bindVertexArray(mesh.vao);
    gl.drawArrays(gl.TRIANGLES,0,mesh.count);
    dirty=false;draws++;return true;
   }catch(error){failures++;dirty=true;return false;}
   finally{
    gl.bindFramebuffer(gl.FRAMEBUFFER,null);
    gl.viewport(0,0,screenWidth,screenHeight);
   }
  },
  bind(locations,enabled=true){
   gl.activeTexture(gl.TEXTURE2);gl.bindTexture(gl.TEXTURE_2D,texture);
   gl.uniform1i(locations.map,2);
   gl.uniformMatrix4fv(locations.light,false,vp);
   gl.uniform2f(locations.texel,1/SIZE,1/SIZE);
   gl.uniform1f(locations.enabled,!dirty&&enabled?1:0);
   gl.activeTexture(gl.TEXTURE0);
   return !dirty&&enabled;
  },
  stats(){return {ready:true,resolution:SIZE,staticPasses:draws,dirty,failures};}
 });
}
root.WPDirectionalShadows=Object.freeze({create,matrix,resolution:SIZE});
})(window);
