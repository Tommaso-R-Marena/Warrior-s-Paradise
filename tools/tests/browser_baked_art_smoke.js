"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const dir=path.resolve(__dirname,"../../browser/3d");
const ctx={window:{},atob:s=>Buffer.from(s,"base64").toString("binary"),Math,
 Uint8Array,Float32Array,DataView,Set};
vm.runInNewContext(fs.readFileSync(path.join(dir,"baked-art.js"),"utf8"),ctx,
 {filename:"baked-art.js"});
const lib=ctx.window.WPBakedArt;
const pack=JSON.parse(fs.readFileSync(path.join(dir,"assets/baked/normandy-stonework.wpmesh.json"),"utf8"));
const manifest=JSON.parse(fs.readFileSync(path.join(dir,"assets/baked/scene.json"),"utf8"));
lib.manifestValid(manifest);
const med=lib.decode(pack,"medium"),high=lib.decode(pack,"high");
assert(high.triangles>med.triangles*3&&high.bytes>med.bytes);
assert(high.data.every(Number.isFinite));
assert(high.data[5]>.8,"stonework face must have physically oriented front normals");
let failure=0;
function rejects(block){
 try{block();}catch(error){failure++;return;}
 throw Error("Rejected unsafe pack was silently accepted");
}
const clone=JSON.parse(JSON.stringify(pack));
clone.lods.high.fnv32="00000000";
rejects(()=>lib.decode(clone,"high"));
clone.lods.high.fnv32=pack.lods.high.fnv32;
clone.lods.high.vertices+=3;
rejects(()=>lib.decode(clone,"high"));
clone.lods.high.vertices=pack.lods.high.vertices;
clone.source.license="unlicensed";
rejects(()=>lib.decode(clone,"high"));
clone.source.license=pack.source.license;
clone.bounds.max[0]=Infinity;
rejects(()=>lib.decode(clone,"high"));
const bad=JSON.parse(JSON.stringify(manifest));
bad.groups[0].instances[0].collider="new";
rejects(()=>lib.manifestValid(bad));
bad.groups[0].instances[0].collider="existing-building";
bad.groups[0].mesh="../../malicious";
rejects(()=>lib.manifestValid(bad));
const fetcher=async url=>{
 const relative=url.replace(/^\.\//,"");
 return{ok:true,async json(){return JSON.parse(fs.readFileSync(path.join(dir,relative),"utf8"));}};
};
(async()=>{
 const scene=await lib.load(fetcher);
 assert.equal(scene.highTriangles,high.triangles*3);
 assert.equal(scene.mediumTriangles,med.triangles*3);
 assert.equal(scene.instances,3);
 assert(scene.sources[0].license==="original");
 const base=new Array(180).fill(.35),low=3,medium=9;
 const result=lib.spliceTier(base,low,medium,scene);
 assert(result.mediumCount>medium);
 assert(result.highCount>base.length/9);
 assert(result.data.slice(0,low*9).every((x,i)=>x===base[i]),
  "Low scene prefix changed when importing baked LODs");
 assert(result.data[medium*9]<0,"baked art marker must survive transformations");
 assert(Number.isFinite(result.data.at(-1)));
 const shader=fs.readFileSync(path.join(dir,"game.js"),"utf8");
 const html=fs.readFileSync(path.join(dir,"index.html"),"utf8");
 for(const s of ["bakedTagged","activateBakedArt","bakedHighTriangles",
  "Unanchored cosmetic mesh","float grass=(1.-bakedTagged)"])
  assert(shader.includes(s),"missing renderer contract "+s);
 assert(html.includes('baked-art.js'));
 console.log("PASS BAKED STATIC ART",JSON.stringify({
  originals:[med.triangles,high.triangles],placed:[scene.mediumTriangles,scene.highTriangles],
  samples:scene.instances,compressedBytes:med.bytes+high.bytes,rejects:failure}));
})().catch(e=>{console.error(e);process.exitCode=1});
