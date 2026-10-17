/* High-only opaque atmospheric sky. This is a single fullscreen WebGL2 triangle:
 * no transparency, textures, network loads, extra framebuffers, or depth writes.
 * It remains optional; the game falls back to its inexpensive clear color.
 */
(function(root){"use strict";
const VERT=`#version 300 es
precision highp float;
out vec2 vUv;
void main(){
 vec2 p=vec2(float((gl_VertexID&1)*4-1),float((gl_VertexID&2)*2-1));
 vUv=p*.5+.5;gl_Position=vec4(p,1.,1.);
}`;
const FRAG=`#version 300 es
precision highp float;
in vec2 vUv;
uniform vec3 uForward,uRight,uUp;
uniform vec2 uScale;
out vec4 outColor;
// Fully analytic daylight/stratocumulus model: no sky texture or alpha
// overdraw. Three bounded harmonics approximate large, middle and broken
// cloud scales in directional sky space, then warm sunlit cloud edges.
void main(){
 vec2 q=vUv*2.-1.;
 vec3 ray=normalize(uForward+uRight*q.x*uScale.x+uUp*q.y*uScale.y);
 vec3 sun=normalize(vec3(.53,.88,-.36));
 float elevation=clamp(ray.y,0.,1.);
 float towardSun=max(0.,dot(ray,sun));
 float nearHorizon=pow(1.-elevation,2.4);
 vec3 zenith=vec3(.217,.390,.602);
 vec3 lower=vec3(.675,.751,.751);
 // Rayleigh blue skylight attenuates close to the physical horizon,
 // while near-solar Mie forward scatter creates a warm atmospheric veil.
 vec3 sky=mix(lower,zenith,pow(elevation,.70));
 float mie=pow(towardSun,16.)*(.11+.24*nearHorizon);
 sky+=vec3(.89,.69,.42)*mie;
 if(ray.y>.035){
  vec2 s=ray.xz/max(.16,ray.y+.18);
  // Broad cumulus decks plus mid-altitude broken clouds. Analytical
  // harmonics are intentionally cheaper than multi-step volumetric rays.
  float broad=sin(s.x*1.44+sin(s.y*.53)*.76)+cos(s.y*1.79-s.x*.41);
  float middle=sin(s.x*3.33+s.y*.91)*cos(s.y*2.63-s.x*.63);
  float wisps=sin(s.y*.67-s.x*.15);
  float coverage=broad*.31+middle*.22+wisps*.27;
  float cloud=smoothstep(.13,.74,coverage);
  cloud*=smoothstep(.065,.18,ray.y)*(1.-smoothstep(.71,.92,ray.y));
  float thick=smoothstep(.43,.88,coverage);
  vec3 underside=vec3(.595,.634,.635);
  vec3 bright=vec3(.862,.864,.838);
  vec3 cloudColor=mix(bright,underside,thick*.56);
  cloudColor+=vec3(.072,.052,.025)*pow(towardSun,12.)*(1.-thick);
  sky=mix(sky,cloudColor,cloud*.68);
 }
 float sunCore=smoothstep(.99942,.99984,towardSun);
 float aureole=pow(towardSun,128.);
 sky+=vec3(.22,.156,.089)*aureole+vec3(.73,.59,.39)*sunCore;
 float aerosol=1.-smoothstep(-.16,.11,ray.y);
 sky=mix(sky,vec3(.666,.723,.702),aerosol*.24);
 outColor=vec4(clamp(sky,0.,1.),1.);
}`;
function compile(gl,type,source){
 const s=gl.createShader(type);
 gl.shaderSource(s,source);gl.compileShader(s);
 if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))throw Error("Sky shader: "+gl.getShaderInfoLog(s));
 return s;
}
function create(gl){
 try{
  const prog=gl.createProgram(),vs=compile(gl,gl.VERTEX_SHADER,VERT),fs=compile(gl,gl.FRAGMENT_SHADER,FRAG);
  gl.attachShader(prog,vs);gl.attachShader(prog,fs);gl.linkProgram(prog);
  if(!gl.getProgramParameter(prog,gl.LINK_STATUS))throw Error("Sky link: "+gl.getProgramInfoLog(prog));
  const vao=gl.createVertexArray();
  const loc=Object.fromEntries(["uForward","uRight","uUp","uScale"].map(k=>[k,gl.getUniformLocation(prog,k)]));
  let draws=0;
  return Object.freeze({
   render(forward,right,up,scaleX,scaleY){
    if(!Number.isFinite(scaleX+scaleY)||scaleX<=0||scaleY<=0)return false;
    gl.disable(gl.DEPTH_TEST);gl.depthMask(false);
    try{
     gl.useProgram(prog);gl.bindVertexArray(vao);
     gl.uniform3f(loc.uForward,...forward);
     gl.uniform3f(loc.uRight,...right);
     gl.uniform3f(loc.uUp,...up);
     gl.uniform2f(loc.uScale,scaleX,scaleY);
     gl.drawArrays(gl.TRIANGLES,0,3);draws++;
     return true;
    }finally{
     gl.depthMask(true);gl.enable(gl.DEPTH_TEST);
    }
   },
   stats(){return {ready:true,draws,mode:"opaque-fullscreen-triangle"};}
  });
 }catch(e){
  return Object.freeze({render(){return false;},stats(){return {ready:false,draws:0,error:String(e)};}});
 }
}
root.WPAtmosphericSky=Object.freeze({create});
})(window);
