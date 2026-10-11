/* Deterministic pathfinding, collision, blocked-goal and CPU bounds. */
"use strict";
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const vm=require("node:vm");
const source=fs.readFileSync(path.resolve(__dirname,"../../browser/3d/tactical-navigation.js"),"utf8");
const sandbox={window:{},Math,Number,Int8Array,Float64Array,Int32Array,Uint8Array};
vm.createContext(sandbox);vm.runInContext(source,sandbox);
const nav=sandbox.window.WPTacticalNav;
assert(nav,"Navigation API failed to initialize");
const wall=(x,z,r=0)=>Math.abs(x)<1.8+r&&Math.abs(z)<6+r;
const clearance=(x,z,r=0)=>wall(x,z,r)||Math.abs(x)>44||Math.abs(z)>44;
const collision=(x,z,dx,dz)=>{
 if(!clearance(x+dx,z,.42))x+=dx;
 if(!clearance(x,z+dz,.42))z+=dz;
 return [x,z];
};
const direct={x:-9,z:11};
for(let i=0;i<240;i++)nav.advance(direct,9,11,3,1/60,clearance,collision);
assert(direct.x>2.5,"Direct unobstructed paths must remain responsive");
assert.equal(nav.stats().plans,0,"No A* planning is needed on clear ground");
const soldier={x:-9,z:0};
for(let i=0;i<1100;i++)nav.advance(soldier,9,0,3,1/60,clearance,collision);
assert(Math.hypot(soldier.x-9,soldier.z)<1.8,
 "AI should traverse around a solid 12-metre wall rather than stick to it: "+JSON.stringify(soldier));
assert(nav.stats().successful>0,"A* must engage for obstructed paths");
assert(nav.stats().plans<18,"Bot must cache paths rather than replan every frame");
const route=nav.plan(-9,-1,9,1,clearance);
assert(route.length>0,"Unobstructed map must yield a route");
for(const p of route)assert(!clearance(p.x,p.z,.45),"A* route crosses an obstacle");
const trapped=(x,z,r=0)=>Math.abs(x)>44||Math.abs(z)>44||
 (Math.abs(x)>2&&Math.abs(x)<5&&Math.abs(z)<5)||
 (Math.abs(z)>2&&Math.abs(z)<5&&Math.abs(x)<5);
assert.equal(nav.plan(0,0,12,0,trapped).length,0,
 "An enclosed soldier must not tunnel through collision geometry");
const counts=nav.stats();
assert(counts.expanded<25000,"A* exceeded bounded expansion budget");
console.log("PASS tactical navigation:",JSON.stringify({soldier,counts}));