/* Exercise the real CC0 GLB loader together with the actual WebGL2 game.
 * No browser, GPU or network required.  Both success and fallback are tested.
 */
"use strict";
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const vm=require("node:vm");
const root=path.resolve(__dirname,"../../browser/3d");
const loader=fs.readFileSync(path.join(root,"asset-loader.js"),"utf8");
const gameplay=fs.readFileSync(path.join(root,"game.js"),"utf8");
const kenney=["tree-oak","tree-pinedefaulta","rock-largea","plant-bush","fence-simple"];
const quaternius=["barn","chicken_coop","windmill","well"];
const models=Object.fromEntries([
 ...kenney.map(name=>["kenney/"+name,fs.readFileSync(path.join(root,"assets/kenney",name+".glb.b64"),"utf8")]),
 ...quaternius.map(name=>["quaternius/"+name,fs.readFileSync(path.join(root,"assets/quaternius",name+".glb.b64"),"utf8")])
]);

async function scenario(shouldLoad,farms=true){
 const raf=[],els={},stats={draws:0,uploads:0},events={};
 const gl={VERTEX_SHADER:1,FRAGMENT_SHADER:2,COMPILE_STATUS:3,LINK_STATUS:4,
  DEPTH_TEST:5,ARRAY_BUFFER:6,STATIC_DRAW:7,DYNAMIC_DRAW:8,FLOAT:9,
  TRIANGLES:10,COLOR_BUFFER_BIT:16,DEPTH_BUFFER_BIT:32};
 for(const n of ["createShader","createProgram","createVertexArray","createBuffer","getUniformLocation"])
  gl[n]=()=>({});
 for(const n of ["shaderSource","compileShader","attachShader","linkProgram","useProgram",
  "enable","clearColor","bindVertexArray","bindBuffer","enableVertexAttribArray",
  "vertexAttribPointer","viewport","clear","uniformMatrix4fv","uniform3fv"])
  gl[n]=()=>{};
 gl.bufferData=()=>{stats.uploads++};
 gl.drawArrays=()=>{stats.draws++};
 gl.getShaderParameter=gl.getProgramParameter=()=>true;
 const el=id=>els[id]??={
  id,clientWidth:900,clientHeight:600,width:900,height:600,hidden:false,disabled:false,
  textContent:"",innerHTML:"",style:{},value:id==="quality"?"low":id==="difficulty"?"regular":"rifle",
  classList:{add(){},remove(){},toggle(){}},events:{},
  addEventListener(name,fn){(this.events[name]??=[]).push(fn)},
  fire(name){for(const fn of this.events[name]||[])fn({button:0,pointerId:1,
   preventDefault(){},setPointerCapture(){}})},
  getContext(){return gl},setPointerCapture(){},requestPointerLock(){},
  getBoundingClientRect(){return {left:0,top:0,width:140,height:140}}
 };
 const win={devicePixelRatio:1,addEventListener(){}};
 const doc={getElementById:el,hidden:false,pointerLockElement:null,
  addEventListener(name,fn){(events[name]??=[]).push(fn)},exitPointerLock(){}};
 const sources=[];
 const fetchMock=async url=>{
  sources.push(url);
  const match=/^\.\/assets\/(kenney|quaternius)\/([a-z0-9_-]+)\.glb\.b64$/.exec(url);
  if(!shouldLoad||!match||(match[1]==="quaternius"&&!farms)||
    !models[match[1]+"/"+match[2]])return {ok:false,status:404};
  return {ok:true,text:async()=>models[match[1]+"/"+match[2]]};
 };
 const sandbox={window:win,document:doc,navigator:{userAgent:"Desktop Chrome"},
  location:{protocol:"https:"},requestAnimationFrame:fn=>raf.push(fn),fetch:fetchMock,
  TextDecoder,atob:text=>Buffer.from(text,"base64").toString("binary"),
  Uint8Array,ArrayBuffer,DataView,Float32Array,Math,Error};
 vm.createContext(sandbox);
 vm.runInContext(loader,sandbox,{filename:"asset-loader.js",timeout:15000});
 vm.runInContext(gameplay,sandbox,{filename:"game.js",timeout:15000});
 // Microtask drain for asynchronous fetch, text decoding and static mesh upload.
 for(let i=0;i<120;i++){
  await Promise.resolve();
  const state=win.WP3DDiagnostics();
  if(shouldLoad?(state.assetsLoaded&&(state.buildingsLoaded===4||state.farmError)):state.assetError)break;
 }
 const before=win.WP3DDiagnostics();
 assert.equal(before.assetsLoaded,shouldLoad,JSON.stringify(before));
 assert.equal(sources.length,shouldLoad?kenney.length+quaternius.length:kenney.length);
 assert(sources.every(url=>/^\.\/assets\/(kenney|quaternius)\//.test(url)));
 if(shouldLoad){
  assert(before.assetInstances>=157&&before.assetTriangles>15000);
  assert(before.geometryTriangles>24000);
  assert.equal(before.buildingsLoaded,farms?4:0);
  if(farms){
   assert.equal(before.assetInstances,161);
   assert.match(el("assetstatus").textContent,/IMPORTED MODELS/);
  }else assert.match(before.farmError,/404/);
 }else{
  assert(before.assetError.includes("404"));
  assert.equal(before.assetInstances,0);
  assert(before.geometryTriangles>5000);
 }
 el("deploy").fire("click");
 assert.equal(win.WP3DDiagnostics().alliesAlive,2);
 for(let i=0;i<50;i++){
  assert(raf.length>0,"Frame loop stopped");
  raf.shift()((i+1)*34);
 }
 assert(stats.draws>=100,"No geometry drawn");
 assert(stats.uploads>=50,"Rendering stopped");
 return {loaded:before.assetsLoaded,triangles:before.geometryTriangles,
  props:before.assetInstances,draws:stats.draws};
}
(async()=>{
 const loaded=await scenario(true),farmFallback=await scenario(true,false),
  fallback=await scenario(false);
 console.log("PASS CC0 integrated WebGL2 scenes:",JSON.stringify({loaded,farmFallback,fallback}));
})().catch(error=>{console.error(error);process.exitCode=1});
