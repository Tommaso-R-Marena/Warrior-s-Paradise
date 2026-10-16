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
void main(){
 vec2 q=vUv*2.-1.;
 vec3 ray=normalize(uForward+uRight*q.x*uScale.x+uUp*q.y*uScale.y);
 float altitude=clamp(ray.y,0.,1.);
 vec3 horizon=vec3(.69,.755,.745);
 vec3 zenith=vec3(.235,.42,.625);
 float gradient=pow(altitude,.69);
 vec3 sky=mix(horizon,zenith,gradient);
 // A bounded, soft stratocumulus layer; no texture fetches or translucent overdraw.
 if(ray.y>.045){
  vec2 uv=ray.xz/max(.18,ray.y+.18);
  float a=sin(uv.x*1.8+uv.y*.38);
  float b=sin(uv.y*2.35-uv.x*.62);
  float cloud=smoothstep(.64,1.05,.64*a+.36*b);
  cloud*=smoothstep(.06,.21,ray.y)*(1.-smoothstep(.71,.92,ray.y));
  sky=mix(sky,vec3(.80,.81,.79),cloud*.39);
 }
 vec3 sun=normalize(vec3(.53,.88,-.36));
 float facing=max(0.,dot(ray,sun));
 float glow=pow(facing,40.);
 float core=smoothstep(.99925,.99986,facing);
 sky+=vec3(.22,.145,.065)*glow+vec3(.72,.55,.32)*core;
 float mist=1.-smoothstep(-.12,.13,ray.y);
 sky=mix(sky,vec3(.68,.73,.70),mist*.24);
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
