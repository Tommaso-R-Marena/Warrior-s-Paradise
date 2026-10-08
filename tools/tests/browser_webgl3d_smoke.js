/* Offline Node smoke test for the genuine WebGL2 browser edition.
 * Verifies initialization, GPU command submission, combat and fallback
 * with mocked browser/WebGL objects. Not visual QA or a hardware benchmark.
 */
"use strict";
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const vm=require("node:vm");
const root=path.resolve(__dirname,"../../browser/3d");
const js=fs.readFileSync(path.join(root,"game.js"),"utf8");
const html=fs.readFileSync(path.join(root,"index.html"),"utf8");
const css=fs.readFileSync(path.join(root,"style.css"),"utf8");
const ids=new Set([...html.matchAll(/id="([\w-]+)"/g)].map(m=>m[1]));
for(const [,id] of js.matchAll(/\$\("([\w-]+)"\)/g))
 assert(ids.has(id),"Missing WebGL2 UI control: "+id);
assert(css.includes("@media(pointer:coarse)"));
assert(!/https?:\/\/(?:cdn|unpkg|esm\.sh)/.test(html+js));

function run(width,height,webgl=true){
 const elements={},callbacks={},raf=[],stats={draws:0,vertices:0,uploads:0};
 const gl={
  VERTEX_SHADER:1,FRAGMENT_SHADER:2,COMPILE_STATUS:3,LINK_STATUS:4,
  DEPTH_TEST:5,ARRAY_BUFFER:6,STATIC_DRAW:7,DYNAMIC_DRAW:8,FLOAT:9,
  TRIANGLES:10,COLOR_BUFFER_BIT:16,DEPTH_BUFFER_BIT:32
 };
 for(const name of ["createShader","createProgram","createVertexArray","createBuffer","getUniformLocation"])gl[name]=()=>({});
 for(const name of ["shaderSource","compileShader","attachShader","linkProgram","useProgram",
  "enable","clearColor","bindVertexArray","bindBuffer","enableVertexAttribArray",
  "vertexAttribPointer","viewport","clear","uniformMatrix4fv","uniform3fv"])gl[name]=()=>{};
 gl.bufferData=()=>stats.uploads++;
 gl.getShaderParameter=()=>true;gl.getProgramParameter=()=>true;
 gl.getShaderInfoLog=()=>"";gl.getProgramInfoLog=()=>"";
 gl.drawArrays=(_,__,count)=>{stats.draws++;stats.vertices+=count;};

 function el(id){
  return elements[id]??={
   id,width:0,height:0,clientWidth:width,clientHeight:height,disabled:false,
   hidden:false,innerHTML:"",textContent:"",value:id==="quality"?"low":"rifle",
   style:{},classList:{add(){},remove(){},toggle(){}},events:{},
   addEventListener(type,fn){(this.events[type]??=[]).push(fn)},
   send(type,params={}){for(const fn of this.events[type]||[])
     fn({button:0,pointerId:1,clientX:45,clientY:40,pointerType:"touch",
       preventDefault(){},...params});},
   setPointerCapture(){},getBoundingClientRect(){
     return{left:0,top:0,width:140,height:140};},
   getContext(){return webgl?gl:null},requestPointerLock(){}
  };
 }
 const doc={
  getElementById:el,pointerLockElement:null,hidden:false,
  addEventListener(type,fn){(callbacks[type]??=[]).push(fn)},
  exitPointerLock(){}
 };
 const win={devicePixelRatio:width<height?2:1,addEventListener(type,fn){
  (callbacks["win:"+type]??=[]).push(fn);
 }};
 const sandbox={window:win,document:doc,
  navigator:{userAgent:width<height?"Android Chrome":"Desktop Chrome"},
  requestAnimationFrame(fn){raf.push(fn)},console,Math,Float32Array,Error};
 vm.createContext(sandbox);
 vm.runInContext(js,sandbox,{filename:"browser/3d/game.js",timeout:15000});
 if(!webgl){
  assert.equal(el("fallback").hidden,false);
  assert.equal(el("deploy").disabled,true);
  return {fallback:true};
 }
 assert.equal(typeof win.WP3DDiagnostics,"function");
 let info=win.WP3DDiagnostics();
 assert.equal(info.renderer,"WebGL2");
 assert(info.geometryTriangles>2000,"World lacks 3D geometry");
 assert(info.renderSize[0]>150&&info.renderSize[1]>100);
 el("deploy").send("click");
 info=win.WP3DDiagnostics();
 assert.equal(info.sector,0);
 assert.equal(info.playerHp,100);
 const frame=(i)=>{
  const fn=raf.shift();
  assert(fn,"requestAnimationFrame must schedule next frame");
  fn((i+1)*34);
 };
 for(let i=0;i<80;i++)frame(i);
 const original=el("mag").textContent;
 el("fire").send("pointerdown");
 for(let i=80;i<110;i++)frame(i);
 el("fire").send("pointerup");
 const after=el("mag").textContent;
 assert.notEqual(original,after,"firing must consume ammo");
 el("reload").send("pointerdown");
 for(let i=110;i<185;i++)frame(i);
 assert.notEqual(el("mag").textContent,after,"reload never completed");
 el("grenade").send("pointerdown");
 el("swap").send("pointerdown");
 for(let i=185;i<250;i++)frame(i);
 assert.match(el("frags").textContent,/× 1/);
 assert.equal(win.WP3DDiagnostics().weapon,"smg");
 assert(stats.draws>200,"No sustained GPU draw calls");
 assert(stats.uploads>200,"Dynamic infantry geometry not updated");
 return {frames:250,worldTriangles:info.geometryTriangles,draws:stats.draws,
  vertices:stats.vertices,resolution:info.renderSize};
}
const result={
 portrait:run(390,844,true),
 landscape:run(844,390,true),
 noWebGL:run(390,844,false)
};
console.log("PASS WebGL2 mobile combat smoke:",JSON.stringify(result));
