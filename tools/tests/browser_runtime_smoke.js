"use strict";
/* Browser Field Edition integration test (dependency-free Node).
 * Runs actual procedural renderer/game scripts in a minimal browser mock.
 * This is not a substitute for visual/mobile hardware QA.
 */
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const vm=require("node:vm");
const ROOT=path.resolve(__dirname,"../../browser");
const visuals=fs.readFileSync(path.join(ROOT,"visuals.js"),"utf8");
const game=fs.readFileSync(path.join(ROOT,"game.js"),"utf8");
const html=fs.readFileSync(path.join(ROOT,"index.html"),"utf8");
const ids=new Set([...html.matchAll(/id="([\w-]+)"/g)].map(x=>x[1]));
for(const [,id] of game.matchAll(/\$\("([\w-]+)"\)/g)){
 assert(ids.has(id),"missing button/control in HTML: "+id);
}
function testDevice(width,height) {
 const elements={},listeners={},frames=[];
 let draws=0;
 const ctx={};
 for(const name of ["fillRect","clearRect","beginPath","arc","fill","stroke","moveTo","lineTo","strokeRect","fillText","drawImage","ellipse","closePath","save","restore","translate","scale","arcTo"]){
  ctx[name]=()=>{if(name==="drawImage")draws++};
 }
 ctx.createLinearGradient=()=>({addColorStop(){}});
 ctx.createRadialGradient=ctx.createLinearGradient;
 function el(id) {
  return elements[id]??={
   id,width:136,height:136,style:{},value:"eco",innerHTML:"",textContent:"",offsetWidth:57,
   classList:{add(){},remove(){}},handlers:{},
   getContext(){return ctx},setPointerCapture(){},requestPointerLock(){},
   getBoundingClientRect(){return {left:8,top:height-170,width:125,height:125}},
   addEventListener(event,fn){(this.handlers[event]??=[]).push(fn)},
   fire(event,props={}){for(const fn of this.handlers[event]??[])
    fn({button:0,pointerId:1,clientX:115,clientY:115,preventDefault(){},...props})}
  };
 }
 const document={
  getElementById:el,
  createElement:()=>el("canvas-"+Object.keys(elements).length),
  addEventListener(event,fn){(listeners[event]??=[]).push(fn)},
  exitPointerLock(){},pointerLockElement:null
 };
 const window={innerWidth:width,innerHeight:height,addEventListener(){}};
 const ctx0={window,document,navigator:{hardwareConcurrency:4},
  matchMedia:()=>({matches:true}),location:{protocol:"file:"},
  requestAnimationFrame:fn=>frames.push(fn),
  console,Math,Uint8Array,Int16Array,
  setTimeout(){}};
 vm.createContext(ctx0);
 vm.runInContext(visuals,ctx0,{filename:"visuals.js"});
 vm.runInContext(game,ctx0,{filename:"game.js"});
 const frame=i=>{const fn=frames.shift();assert(fn,"no animation frame");fn((i+1)*36)};
 el("start-btn").fire("click");
 for(let i=0;i<120;i++)frame(i);
 const start=el("ammo").textContent;
 el("ads-btn").fire("pointerdown");
 el("fire-btn").fire("pointerdown");
 for(let i=120;i<159;i++)frame(i);
 el("fire-btn").fire("pointerup");el("ads-btn").fire("pointerup");
 const fired=el("ammo").textContent;
 assert.notEqual(fired,start,"firing did not consume ammunition");
 el("reload-btn").fire("pointerdown");
 for(let i=159;i<230;i++)frame(i);
 const reloaded=el("ammo").textContent;
 assert.notEqual(fired,reloaded,"reload did not complete");
 el("frag-btn").fire("pointerdown");
 el("smoke-btn").fire("pointerdown");
 for(let i=230;i<310;i++)frame(i);
 assert.match(el("grenades").textContent,/◉ 1 \/ ▤ 0/);
 el("order-btn").fire("pointerdown");
 el("switch-btn").fire("pointerdown");
 for(let i=310;i<445;i++)frame(i);
 assert.match(el("loadout").textContent,/STEN MK II.*HOLD/);
 assert(draws>50,"texture/sprite renderer did not run");
 assert(el("world").width>0&&el("world").height>0);
 console.log("PASS",width+"x"+height,"445 simulated frames","Canvas drawImage calls:",draws);
}
testDevice(390,844);
testDevice(844,390);
