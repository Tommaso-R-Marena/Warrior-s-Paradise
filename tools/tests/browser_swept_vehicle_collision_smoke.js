"use strict";
// Swept oriented vehicle collider: reproduces rock/fence tunnelling that
// center-only next-position collision checks used to permit.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const js=fs.readFileSync(path.resolve(__dirname,"../../browser/3d/ww2-vehicles.js"),"utf8");
const root={};vm.runInNewContext(js,{window:root,Math,Number,Error});
const fleet=root.WPArmoredFleet;
assert(fleet?.footprintClear&&fleet?.overlappingHull);
fleet.reset();
const player={x:-41,z:-31,yaw:0};
assert(fleet.toggle(player,()=>false),"Should enter Willys near deployment");
const jeep=fleet.current();assert.equal(jeep.id,"jeep");
const fromX=jeep.x,fromZ=jeep.z,angle=jeep.yaw;
const dx=-Math.sin(angle),dz=Math.cos(angle);
const rock={x:fromX+dx*2.75,z:fromZ+dz*2.75};
const blocked=(x,z,r)=>Math.abs(x-rock.x)<.17+r&&Math.abs(z-rock.z)<.19+r;
assert.equal(blocked(fromX,fromZ,.75),false,"Rock is away from jeep's center");
assert.equal(fleet.footprintClear(jeep,fromX+dx*1.15,fromZ+dz*1.15,angle,blocked),false,
 "Whole vehicle must collide when nose reaches rock, although its center is clear");
const input={keys:{KeyW:true},joyY:0,joyX:0};
fleet.move(.55,input,player,blocked);
const traveled=Math.hypot(jeep.x-fromX,jeep.z-fromZ);
assert(traveled<2.75&&fleet.info().collisionRejected>0,"Jeep must halt before a thin solid obstacle");
for(let i=0;i<6;i++)fleet.move(.55,input,player,blocked);
assert(Math.hypot(jeep.x-fromX,jeep.z-fromZ)<2.75,
 "Repeated accelerated movement must never tunnel through the boulder");
assert(fleet.info().collisionSamples>10,"The collision must sample the actual physical hull");
assert(fleet.overlappingHull({width:2,length:4},0,0,0,
 {x:0,z:3.0,yaw:0,width:2,length:4}),"Overlapping vehicles must collide");
assert(!fleet.overlappingHull({width:2,length:4},0,0,0,
 {x:5,z:0,yaw:0,width:2,length:4}),"Separated vehicles should not collide");
const tank={width:3,length:5.6};
const edge=(x,z,r)=>Math.abs(x-1.3)<r+.10&&Math.abs(z-2.45)<r+.10;
assert.equal(edge(0,0,1.35),false,"Center-only vehicle probe misses the forward corner");
assert.equal(fleet.footprintClear(tank,0,0,0,edge),false,
 "Long Sherman corner should collide with an obstacle during steering");
console.log("PASS swept vehicle physics: Willys nose, repeated high-speed collision, Sherman corners and oriented hull overlap");
