/* Fast, deterministic CPU contract: High graphics modules remain optional,
 * bounded and render-state safe. Real shader compilation runs in Playwright.
 */
"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const root=path.resolve(__dirname,"../../browser/3d");
const win={};
for(const file of ["battle-sky.js","architectural-detail.js"]){
 vm.runInNewContext(fs.readFileSync(path.join(root,file),"utf8"),{window:win,Math},{filename:file});
}
const gl={
 VERTEX_SHADER:1,FRAGMENT_SHADER:2,COMPILE_STATUS:3,LINK_STATUS:4,
 DEPTH_TEST:5,TRIANGLES:6,shaderSources:[],calls:[],
 createShader(){return {};},createProgram(){return {};},createVertexArray(){return {};},
 shaderSource(_,source){this.shaderSources.push(source);},
 compileShader(){},getShaderParameter(){return true;},getShaderInfoLog(){return "";},
 attachShader(){},linkProgram(){},getProgramParameter(){return true;},getProgramInfoLog(){return "";},
 getUniformLocation(_,name){return name;},useProgram(){},bindVertexArray(){},
 disable(v){this.calls.push(["disable",v]);},enable(v){this.calls.push(["enable",v]);},
 depthMask(v){this.calls.push(["depth",v]);},uniform3f(){},uniform2f(){},
 drawArrays(type,start,count){this.calls.push(["draw",type,start,count]);}
};
const sky=win.WPAtmosphericSky.create(gl);
assert.equal(sky.stats().ready,true);
assert(sky.render([0,0,1],[-1,0,0],[0,1,0],.8,.6));
assert.equal(sky.stats().draws,1);
assert.deepEqual(gl.calls.slice(-2),[["depth",true],["enable",gl.DEPTH_TEST]]);
assert(gl.shaderSources.join("\n").includes("vec3 sun="));
assert.equal(sky.render([0,0,1],[-1,0,0],[0,1,0],Infinity,.6),false);
assert.equal(sky.stats().draws,1);
const roofs=[[-24,-12,11,9],[-2,-25,9,8],[18,-12,11,11],
 [-22,13,10,10],[5,27,9,12],[28,1,10,9],[-39,15,8,10],
 [35,-27,10,12],[-8,38,10,11]];
function roofsRun(){
 const faces=[];
 const g={quad(a,b,c,d,color){
  for(const v of [a,b,c,d,color])assert(v.every(Number.isFinite));
  assert(a[1]>4.3&&a[1]<6.5,"Tile not on the real pitched roof");
  assert(c[1]>4.3&&c[1]<6.5,"Tile not on the real pitched roof");
  faces.push([a,b,c,d,color]);
 }};
 const stats=win.WPArchitecturalDetail.append(g,roofs);
 assert.equal(stats.triangles,faces.length*2);
 assert.equal(stats.roofs,9);
 assert(stats.triangles>900&&stats.triangles<=win.WPArchitecturalDetail.maxTriangles);
 return stats;
}
assert.deepEqual(roofsRun(),roofsRun());
const html=fs.readFileSync(path.join(root,"index.html"),"utf8");
const game=fs.readFileSync(path.join(root,"game.js"),"utf8");
for(const name of ["battle-sky.js","architectural-detail.js"])
 assert(html.includes(name),"New static module omitted from browser entry");
assert(game.includes("skyRenderer.render("));
assert(game.includes("WPArchitecturalDetail.append("));
assert(game.includes("if(game.quality===\"high\""),"Sky must remain high-tier only");
console.log("PASS high-tier graphics:",sky.stats(),roofsRun());
