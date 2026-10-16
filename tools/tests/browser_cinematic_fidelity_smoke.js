"use strict";
/* Deterministic High graphics acceptance without GPU. Real Chromium checks
 * shader compilation, shadow-depth framebuffer and gameplay frame pacing.
 */
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const folder=path.resolve(__dirname,"../../browser/3d"),root={};
for(const name of ["directional-shadows.js","botanical-detail.js","skinning.js","ww2-vehicles.js"]){
 vm.runInNewContext(fs.readFileSync(path.join(folder,name),"utf8"),
  {window:root,Math,Float32Array,Float64Array,Uint8Array,DataView,ArrayBuffer,Map,Set},
  {filename:name});
}
const events=[],gl={
 VERTEX_SHADER:1,FRAGMENT_SHADER:2,COMPILE_STATUS:3,LINK_STATUS:4,
 TEXTURE_2D:5,DEPTH_COMPONENT24:6,DEPTH_COMPONENT:7,UNSIGNED_INT:8,
 TEXTURE_WRAP_S:9,TEXTURE_WRAP_T:10,CLAMP_TO_EDGE:11,TEXTURE_MIN_FILTER:12,
 TEXTURE_MAG_FILTER:13,NEAREST:14,FRAMEBUFFER:15,DEPTH_ATTACHMENT:16,
 FRAMEBUFFER_COMPLETE:17,NONE:18,DEPTH_BUFFER_BIT:19,DEPTH_TEST:20,
 TRIANGLES:21,TEXTURE2:22,TEXTURE0:23,
 createShader(){return {};},shaderSource(){},compileShader(){},
 getShaderParameter(){return true;},getShaderInfoLog(){return "";},
 createProgram(){return {};},attachShader(){},linkProgram(){},
 getProgramParameter(){return true;},getProgramInfoLog(){return "";},
 createTexture(){return {};},bindTexture(){},texImage2D(){},texParameteri(){},
 createFramebuffer(){return {};},bindFramebuffer(...x){events.push(["fbo",...x]);},
 framebufferTexture2D(){},drawBuffers(){},readBuffer(){},
 checkFramebufferStatus(){return this.FRAMEBUFFER_COMPLETE;},
 getUniformLocation(_,n){return n;},useProgram(){},
 viewport(...v){events.push(["viewport",...v]);},
 clear(v){events.push(["clear",v]);},enable(){},
 uniformMatrix4fv(){},bindVertexArray(){},drawArrays(t,s,n){events.push(["draw",n]);},
 activeTexture(){},uniform1i(){},uniform1f(){},uniform2f(){}
};
const shadows=root.WPDirectionalShadows.create(gl);
assert(shadows,"Shadow-map depth framebuffer must be created");
assert.equal(root.WPDirectionalShadows.matrix().length,16);
assert(Array.from(root.WPDirectionalShadows.matrix()).every(Number.isFinite));
const mesh={vao:{},count:720};
assert(shadows.renderStatic(mesh,1024,576));
assert(shadows.renderStatic(mesh,1024,576));
assert.equal(shadows.stats().staticPasses,1,"A static scene may not be redrawn every frame");
shadows.invalidate();assert(shadows.renderStatic(mesh,1024,576));
assert.equal(shadows.stats().staticPasses,2,"Updated scene must refresh its shadows");
assert(shadows.bind({map:"a",light:"b",texel:"c",enabled:"d"}));
assert.deepEqual(events.slice(-1)[0][0],"viewport");
const flags=[{x:-12,z:-5},{x:11,z:11},{x:31,z:29}],buildings=[[-24,-12,11,9]];
function botanical(){
 const xs=[],g={tri(a,b,c,color){
  for(const vertex of [a,b,c,color])assert(vertex.every(Number.isFinite));
  xs.push([a,b,c,color]);
 }};
 const stats=root.WPBotanicalDetail.append(g,()=>false,flags,buildings);
 assert.equal(stats.triangles,xs.length);
 assert(stats.triangles>600&&stats.triangles<=root.WPBotanicalDetail.maxTriangles);
 assert(stats.clumps>50&&stats.blades>300&&stats.hedges>0);
 return stats;
}
assert.deepEqual(botanical(),botanical(),"Botanical distribution must be deterministic");
const slerp=root.WPSkinning.slerpQuaternion;
const q=[0,0,0,1],to=[0,1,0,0];
assert(Math.abs(slerp(q,to,.5).reduce((s,v)=>s+v*v,0)-1)<1e-6);
assert(Math.abs(slerp(q,to,0)[3]-1)<1e-6);
assert(Math.abs(slerp(q,to,1)[1]-1)<1e-6);
assert(Math.abs(slerp(q,q.map(x=>-x),.5)[3]-1)<1e-6);
const g=()=>({triangles:0,box(){this.triangles+=12;},cone(){this.triangles+=7;},
 tri(a,b,c,col){this.triangles++;}});
const veh={x:-40,z:-23,yaw:0};
const low=g(),high=g();
root.WPArmoredFleet.draw(low,null,"medium",veh,0);
root.WPArmoredFleet.draw(high,null,"high",veh,0);
assert(high.triangles>low.triangles+200,"High tank and jeep detail is not substantial");
const game=fs.readFileSync(path.join(folder,"game.js"),"utf8");
const html=fs.readFileSync(path.join(folder,"index.html"),"utf8");
for(const name of ["directional-shadows.js","botanical-detail.js"])
 assert(html.includes(name));
for(const marker of ["uShadowMap","directionalShadows?.renderStatic","WPBotanicalDetail.append"])
 assert(game.includes(marker),"Missing graphics integration "+marker);
console.log("PASS High shadow/vegetation/animation/vehicle contracts",JSON.stringify({shadowPasses:shadows.stats().staticPasses,botanical:botanical(),mediumTriangles:low.triangles,highTriangles:high.triangles}));
