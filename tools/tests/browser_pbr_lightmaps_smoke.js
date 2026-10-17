"use strict";
const assert=require("node:assert/strict");
const fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const dir=path.resolve(__dirname,"../../browser/3d");
const app={window:{},atob:s=>Buffer.from(s,"base64").toString("binary"),
 Uint8Array,Float32Array,DataView,Math,Set,Number,Blob};
vm.runInNewContext(fs.readFileSync(path.join(dir,"pbr-lightmaps.js"),"utf8"),app);
const api=app.window.WPPBRLightmaps;
const scene=JSON.parse(fs.readFileSync(path.join(dir,"assets/pbr/scene.json"),"utf8"));
const pack=JSON.parse(fs.readFileSync(path.join(dir,"assets/pbr/normandy-brick-face.wppbr.json"),"utf8"));
const lm=JSON.parse(fs.readFileSync(path.join(dir,"assets/pbr/normandy-brick-irradiance.wptex.json"),"utf8"));
assert.equal(api.vertexBytes,18);
api.manifestValid(scene);
const medium=api.unpack(pack,"medium"),high=api.unpack(pack,"high");
assert(medium.length>0&&high.length>medium.length*100);
assert(medium.every(Number.isFinite)&&high.every(Number.isFinite));
assert.equal(medium.length/30,2);
assert.equal(high.length/30,2350);
const placement=api.place(medium,scene.groups[0].instances);
assert.equal(placement.length,medium.length*3);
assert(placement.every(Number.isFinite));
assert.equal(lm.schema,"wp-linear-texture-v1");
const data=Buffer.from(lm.rgba,"base64");
assert.equal(api.fnv(data),lm.fnv32);
assert.equal(data.length,64*64*4);
assert(scene.groups[0].textures.albedo.endsWith("brick_photo.jpg.b64"));
let rejections=0;
const reject=fn=>{assert.throws(fn);rejections++};
function copy(x){return JSON.parse(JSON.stringify(x))}
const corrupted=copy(pack);corrupted.lods.high.fnv32="deadbeef";
reject(()=>api.unpack(corrupted,"high"));
corrupted.lods.high.fnv32=pack.lods.high.fnv32;
corrupted.lods.high.vertices+=3;
reject(()=>api.unpack(corrupted,"high"));
corrupted.lods.high.vertices=pack.lods.high.vertices;
corrupted.source.license="none";
reject(()=>api.unpack(corrupted,"high"));
const invalid=copy(scene);invalid.groups[0].textures.albedo="https://evil.example/asset";
reject(()=>api.manifestValid(invalid));
invalid.groups[0].textures.albedo=scene.groups[0].textures.albedo;
invalid.groups[0].instances[0].collider="none";
reject(()=>api.manifestValid(invalid));
invalid.groups[0].instances[0].collider="existing-building";
invalid.groups[0].mesh="../evil";
reject(()=>api.manifestValid(invalid));
const fetcher=async url=>({
 ok:true,
 async json(){
  return JSON.parse(fs.readFileSync(path.join(dir,url.replace("./","")),"utf8"));
 }
});
(async()=>{
 const loaded=await api.loadScene(fetcher);
 assert.equal(loaded.highTriangles,7050);
 assert.equal(loaded.mediumTriangles,6);
 assert(loaded.groups[0].source.license==="original");
 const game=fs.readFileSync(path.join(dir,"game.js"),"utf8");
 const html=fs.readFileSync(path.join(dir,"index.html"),"utf8");
 for(const keyword of ["loadPhysicalPBRArt","pbrRenderer.render(","uvPBR:","UV/PBR art creates invisible"])
  assert(game.includes(keyword),"missing game hook "+keyword);
 assert(html.includes("pbr-lightmaps.js"));
 console.log("PASS UV PBR LIGHTMAP PACK",JSON.stringify({
  mediumTriangles:loaded.mediumTriangles,highTriangles:loaded.highTriangles,
  mapPixels:lm.width*lm.height,rejections}));
})().catch(e=>{console.error(e);process.exitCode=1});
