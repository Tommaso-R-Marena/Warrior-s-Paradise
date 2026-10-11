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
const infantry=fs.readFileSync(path.join(root,"battle-art.js"),"utf8");
const skinning=fs.readFileSync(path.join(root,"skinning.js"),"utf8");
const materials=fs.readFileSync(path.join(root,"material-atlas.js"),"utf8");
const spatialAudio=fs.readFileSync(path.join(root,"battle-audio.js"),"utf8");
const decoration=fs.readFileSync(path.join(root,"normandy-dressing.js"),"utf8");
const vehicles=fs.readFileSync(path.join(root,"ww2-vehicles.js"),"utf8");
const ww2Environment=fs.readFileSync(path.join(root,"ww2-environment.js"),"utf8");
const terrainScript=fs.readFileSync(path.join(root,"ww2-terrain.js"),"utf8");
const kenney=["tree-oak","tree-pinedefaulta","rock-largea","plant-bush","fence-simple"];
const quaternius=["barn","chicken_coop","windmill","well"];
const normandy=["log-stacklarge","fence-gate","log-large","rock-largeb","stump-old","grass-large"];
const character="character_medium";
const historical=["axis_ww2","gaz67_jeep","panzer_study"];
const models=Object.fromEntries([
 ...kenney.concat(normandy).map(name=>["kenney/"+name,fs.readFileSync(path.join(root,"assets/kenney",name+".glb.b64"),"utf8")]),
 ["kenney/"+character,fs.readFileSync(path.join(root,"assets/kenney",character+".glb.b64"),"utf8")],
 ...quaternius.map(name=>["quaternius/"+name,fs.readFileSync(path.join(root,"assets/quaternius",name+".glb.b64"),"utf8")]),
 ...historical.map(name=>["oga/"+name,fs.readFileSync(path.join(root,"assets/oga",name+".glb.b64"),"utf8")])
]);

async function scenario(shouldLoad,farms=true){
 const raf=[],els={},stats={draws:0,uploads:0,textures:0},events={};
 const gl={VERTEX_SHADER:1,FRAGMENT_SHADER:2,COMPILE_STATUS:3,LINK_STATUS:4,
  DEPTH_TEST:5,ARRAY_BUFFER:6,STATIC_DRAW:7,DYNAMIC_DRAW:8,FLOAT:9,
  TRIANGLES:10,COLOR_BUFFER_BIT:16,DEPTH_BUFFER_BIT:32,
  TEXTURE_2D:40,TEXTURE0:41,TEXTURE1:42,RGBA:43,UNSIGNED_BYTE:44,
  UNPACK_ALIGNMENT:45,TEXTURE_WRAP_S:46,TEXTURE_WRAP_T:47,
  TEXTURE_MIN_FILTER:48,TEXTURE_MAG_FILTER:49,
  CLAMP_TO_EDGE:50,LINEAR:51,LINEAR_MIPMAP_LINEAR:52};
 for(const n of ["createShader","createProgram","createVertexArray","createBuffer","getUniformLocation"])
  gl[n]=()=>({});
 for(const n of ["shaderSource","compileShader","attachShader","linkProgram","useProgram",
  "enable","clearColor","bindVertexArray","bindBuffer","enableVertexAttribArray",
  "vertexAttribPointer","viewport","clear","uniformMatrix4fv","uniform3fv",
  "bindTexture","pixelStorei","texParameteri","generateMipmap",
  "activeTexture","uniform1i","uniform1f"])
  gl[n]=()=>{};
 gl.bufferData=()=>{stats.uploads++};
 gl.createTexture=()=>{stats.textures++;return {index:stats.textures}};
 gl.texImage2D=(...args)=>{
  assert.equal(args[3],512,"Unexpected material width");
  assert.equal(args[4],128,"Unexpected material height");
 };
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
  const match=/^\.\/assets\/(kenney|quaternius|oga)\/([a-z0-9_-]+)\.glb\.b64$/.exec(url);
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
 vm.runInContext(skinning,sandbox,{filename:"skinning.js",timeout:15000});
 vm.runInContext(materials,sandbox,{filename:"material-atlas.js",timeout:15000});
 vm.runInContext(infantry,sandbox,{filename:"battle-art.js",timeout:15000});
 vm.runInContext(spatialAudio,sandbox,{filename:"battle-audio.js",timeout:15000});
 vm.runInContext(decoration,sandbox,{filename:"normandy-dressing.js",timeout:15000});
 vm.runInContext(vehicles,sandbox,{filename:"ww2-vehicles.js",timeout:15000});
 vm.runInContext(ww2Environment,sandbox,{filename:"ww2-environment.js",timeout:15000});
 vm.runInContext(terrainScript,sandbox,{filename:"ww2-terrain.js",timeout:15000});
 assert(win.WPBattleArt&&win.WPBattleSound&&win.WPMaterialAtlas,
  "Full presentation modules failed to initialize");
 vm.runInContext(gameplay,sandbox,{filename:"game.js",timeout:15000});
 // Microtask drain for asynchronous fetch, text decoding and static mesh upload.
 for(let i=0;i<240;i++){
  await Promise.resolve();
  const state=win.WP3DDiagnostics();
  if(shouldLoad?(state.assetsLoaded&&state.rigLoaded&&state.dressingReady&&state.armor?.ready):state.assetError)break;
 }
 const before=win.WP3DDiagnostics();
 assert.equal(before.texturesLoaded,true,"Material textures failed to upload");
 assert.equal(stats.textures,2,"Expected albedo/roughness and bump textures");
 assert.equal(before.assetsLoaded,shouldLoad,JSON.stringify(before));
 assert.equal(sources.length,shouldLoad?kenney.length+quaternius.length+normandy.length+5:kenney.length+5);
 assert(sources.every(url=>/^\.\/assets\/(kenney|quaternius|oga)\//.test(url)));
 if(shouldLoad){
  assert(before.rigLoaded,"Licensed character rig failed to load");
  assert.equal(before.ww2Rigs.axis,true,"Actual rigged German soldier not parsed: "+JSON.stringify(before.ww2Rigs));
  assert.equal(before.armor.loaded,2,"Actual CC0 jeep and tank models not parsed: "+JSON.stringify(before.armor.errors));
  assert(before.assetInstances>=157&&before.assetTriangles>15000);
  assert(before.geometryTriangles>24000);
  assert.equal(before.buildingsLoaded,farms?4:0);
  if(farms){
   assert(before.assetInstances>161,"Missing extra CC0 set dressing");
   assert.equal(before.dressingReady,true);
   assert(before.dressingInstances>0,"Extra CC0 models not incorporated: "+before.dressingError);
   assert(before.dressingTriangles>0&&before.dressingTriangles<=4800,
    "Dressing exceeds hard triangle budget");
   assert(before.dressingCover>0,"Tactical barriers need world collision");
  assert(before.terrainTriangles>1500&&before.terrainTriangles<=2888,
    "Landscape ground mesh missing or unbounded");
  assert(before.ww2EnvironmentTriangles>0&&before.ww2EnvironmentTriangles<=1400,
   "Actual wartime details are missing or exceed static triangle budget");
   assert.match(el("assetstatus").textContent,/WORLD OBJECTS/);
  }else{
   assert.match(before.farmError,/404/);
   assert(before.dressingReady&&before.dressingInstances>0,
    "Independent CC0 dressing should survive missing farm assets");
  }
 }else{
  assert(before.assetError.includes("404"));
  assert.equal(before.armor.loaded,0,"Unavailable OGA files must remain visual fallbacks");
  assert.equal(before.assetInstances,0);
  assert.equal(before.rigLoaded,false);
  assert(before.geometryTriangles>5000);
 }
 el("deploy").fire("click");
 assert.equal(win.WP3DDiagnostics().alliesAlive,2);
 if(shouldLoad){
  const medium=win.WP3DDiagnostics();
  assert.equal(medium.dressingDrawn,true,"Desktop default must show the full CC0 scenery");
  assert(medium.renderedStaticTriangles<medium.geometryTriangles,
   "Medium must use bounded CC0 LOD, not the full High mesh");
  assert(medium.renderedStaticTriangles>medium.geometryTriangles-medium.dressingTriangles,
   "Medium must still render actual additional licensed models");
  assert(medium.mediumDressingTriangles<=1800,"Medium triangle budget exceeded");
 }
 for(let i=0;i<50;i++){
  assert(raf.length>0,"Frame loop stopped");
  raf.shift()((i+1)*34);
 }
 assert(stats.draws>=100,"No geometry drawn");
 if(shouldLoad)assert(win.WP3DDiagnostics().skinActors>0,"Skinning not present in actual gameplay draw");
 if(shouldLoad){
  el("quality").value="low";
  el("deploy").fire("click");
  const low=win.WP3DDiagnostics();
  assert.equal(low.dressingDrawn,false,"Low tier must skip the extra asset geometry");
  assert(low.renderedStaticTriangles<low.geometryTriangles,
   "Low tier should render fewer static triangles than Medium/High");
 }
 assert(stats.uploads>=50,"Rendering stopped");
 return {loaded:before.assetsLoaded,triangles:before.geometryTriangles,
  props:before.assetInstances,draws:stats.draws};
}
(async()=>{
 const loaded=await scenario(true),farmFallback=await scenario(true,false),
  fallback=await scenario(false);
 console.log("PASS CC0 integrated WebGL2 scenes:",JSON.stringify({loaded,farmFallback,fallback}));
})().catch(error=>{console.error(error);process.exitCode=1});
