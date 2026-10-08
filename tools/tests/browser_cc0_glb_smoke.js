/* Offline regression for our checked-in CC0 GLB pack (no network).
 * This tests actual binary geometry, not just filenames or text markers.
 */
"use strict";
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const vm=require("node:vm");
const root=path.resolve(__dirname,"../../browser/3d");
const source=fs.readFileSync(path.join(root,"asset-loader.js"),"utf8");
const env={window:{},TextDecoder,Uint8Array,ArrayBuffer,DataView,
  atob:str=>Buffer.from(str,"base64").toString("binary")};
vm.runInNewContext(source,env,{filename:"asset-loader.js",timeout:1000});
const loader=env.window.WPAssetKit;
assert(loader&&typeof loader.parseGLB==="function");
const specs=[
 ["tree-oak",196],["tree-pinedefaulta",230],["rock-largea",80],
 ["plant-bush",32],["fence-simple",64]
];
let totalBytes=0,triangles=0;
for(const [name,expected] of specs){
 const file=path.join(root,"assets/kenney",name+".glb.b64");
 const base64=fs.readFileSync(file,"utf8").trim();
 const buf=Buffer.from(base64,"base64");
 assert.equal(buf.toString("base64"),base64,"Noncanonical base64: "+name);
 assert(buf.length<25000,"Exceeded mobile asset budget: "+name);
 totalBytes+=buf.length;
 const ab=buf.buffer.slice(buf.byteOffset,buf.byteOffset+buf.byteLength);
 const model=loader.parseGLB(ab);
 assert.equal(model.count,expected,"Unexpected model topology: "+name);
 assert(model.max[1]>model.min[1]&&model.count>20,"Broken bounds: "+name);
 const geo={data:[],tri(a,b,c,col){this.data.push([a,b,c,col]);}};
 assert.equal(loader.appendInstances(geo,model,[[4,0,6,5,0],[6,0,7,4,.5]],[[.2,.3,.2],[.35,.28,.22]]),model.count*2);
 assert.equal(geo.data.length,model.count*2);
 assert(geo.data.every(t=>t.slice(0,3).every(v=>v.every(Number.isFinite))));
 triangles+=model.count;
}
assert(totalBytes<100000);
const mutated=Buffer.from(fs.readFileSync(path.join(root,"assets/kenney","tree-oak.glb.b64"),"utf8").trim(),"base64");
mutated.writeUInt32LE(0,0);
assert.throws(()=>loader.parseGLB(mutated.buffer.slice(mutated.byteOffset,mutated.byteOffset+mutated.byteLength)),/not a binary glTF/);
console.log("PASS CC0 GLB source and geometry:",JSON.stringify({assets:specs.length,bytes:totalBytes,trianglesPerSet:triangles,corruptionRejected:true}));
