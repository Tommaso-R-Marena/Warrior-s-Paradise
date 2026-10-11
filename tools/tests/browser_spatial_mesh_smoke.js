"use strict";
/* One-time conservation and conservative visibility checks; not GPU benchmarking. */
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const root=path.resolve(__dirname,"../../browser/3d");
const ctx={window:{},Float32Array,Math,Map,Array,Number,Error};
vm.runInNewContext(fs.readFileSync(path.join(root,"spatial-mesh.js"),"utf8"),ctx);
const spatial=ctx.window.WPSpatialMesh;
const data=[];
function triangle(vertices){
 for(const pos of vertices)data.push(...pos,0,1,0,.3,.4,.2);
}
triangle([[-45,0,-45],[45,0,-45],[45,0,45]]); // always-on terrain
triangle([[-45,0,-45],[45,0,45],[-45,0,45]]);
for(let z=-40;z<=40;z+=8)for(let x=-40;x<=40;x+=8){
 triangle([[x,0,z],[x+1,0,z],[x+.5,2,z+.5]]);
}
const p=spatial.partition(data,20);
assert.equal(p.triangles,data.length/27);
assert.equal(p.triangles,2+p.batches.reduce((n,b)=>n+b.triangles,0));
assert.equal(p.globalTriangles,2);
assert(p.batchCount>=16&&p.batchCount<=30);
const all=p.batches.map(x=>spatial.visible(x,0,0,0,.65,90));
assert(all.some(Boolean)&&all.some(x=>!x),"Looking north must cull geometry behind player");
assert(p.batches.every(b=>spatial.visible(b,b.cx,b.cz,0,.65,90)),"A cell at eye must remain visible");
assert(p.batches.every(b=>spatial.visible(b,0,0,0,Math.PI/2,200)||b.cz < -b.radius-2),
 "180-degree field should include all possible forward cells");
const malformed=data.slice();malformed[1]=NaN;
assert.throws(()=>spatial.partition(malformed,20),/Invalid vertex/);
const missing=data.slice(0,-1);
assert.throws(()=>spatial.partition(missing,20),/Invalid static mesh/);
console.log("PASS spatial world conservation and frustum pruning:",JSON.stringify({
 triangles:p.triangles,batches:p.batchCount,globalTriangles:p.globalTriangles,
 visibleNorth:all.filter(Boolean).length,culledNorth:all.filter(v=>!v).length}));
