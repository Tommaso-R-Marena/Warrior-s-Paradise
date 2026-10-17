"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const dir=path.resolve(__dirname,"../../browser/3d"),win={};
vm.runInNewContext(fs.readFileSync(path.join(dir,"actor-visibility.js"),"utf8"),
 {window:win,Math},{filename:"actor-visibility.js"});
const vis=win.WPActorVisibility;
const player={x:0,z:0,yaw:0},actor=(x,z)=>({x,z,hp:100,moveSpeed:2,team:"axis"});
assert(vis.visible(actor(0,4),player,"high"));
assert(vis.visible(actor(0,-4),player,"high"),"nearest target must render in every direction");
assert(vis.visible(actor(0,40),player,"high"),"front target must render");
assert(!vis.visible(actor(0,-40),player,"high"),"far behind target should be culled");
assert(vis.visible(actor(0,-40),player,"medium"),"Balanced must be untouched");
assert(vis.visible(actor(0,-40),player,"low"),"Low must be untouched");
assert(vis.visible(actor(0,12),{...player,yaw:Math.PI*.42},"high"),
 "Peripheral actors should have generous angular margin");
assert(vis.visible(actor(NaN,1),player,"high"),"invalid positions fail visible");
assert(!vis.visible(actor(0,91),player,"high"),"far out-of-world actor is culled");
const actors=[actor(0,-40),actor(0,36),actor(0,6),actor(0,-6)];
const before=JSON.stringify(actors);
const selected=vis.sortVisible(actors,player,"high");
assert.equal(selected.length,3);
assert.equal(selected[0],actors[2],"skinning prioritizes close visible target");
assert.equal(JSON.stringify(actors),before,"render filter must not touch physics actor state");
const game=fs.readFileSync(path.join(dir,"game.js"),"utf8");
for(const term of ["WPActorVisibility","sortVisible(","visibility.visible(","culledActors","visibleActors"])
 assert(game.includes(term),"runtime source missing "+term);
const html=fs.readFileSync(path.join(dir,"index.html"),"utf8");
assert(html.includes('actor-visibility.js'));
console.log("PASS CONSERVATIVE HIGH ONLY ACTOR CULLING",JSON.stringify({
 nearest:selected[0].z,visible:selected.length,culled:actors.length-selected.length,
 lowUnchanged:true,mediumUnchanged:true}));
