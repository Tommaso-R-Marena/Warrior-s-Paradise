/* Hash-verify all real CC0 512px diffuse/normal/roughness JPEGs and test atlas. */
"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),
 crypto=require("node:crypto"),vm=require("node:vm");
const root=path.resolve(__dirname,"../../browser/3d");
const manifest=JSON.parse(fs.readFileSync(path.join(root,"assets/materials/MANIFEST.json"),"utf8"));
assert.equal(manifest.license,"CC0-1.0");
assert.deepEqual(manifest.texture_resolution,[512,512]);
const tiles={},signatures=new Map();
let bytesTotal=0;
for(const spec of manifest.assets){
 for(const kind of ["photo","normal","roughness"]){
  const record=spec.maps[kind],asset=record.asset,label=spec.name+"_"+kind;
  const encoded=fs.readFileSync(path.join(root,"assets/materials",asset),"utf8").trim();
  assert.equal(encoded.length%4,0,label+": invalid Base64 length");
  const bytes=Buffer.from(encoded,"base64");
  assert.equal(bytes.toString("base64"),encoded,label+": noncanonical Base64");
  assert.equal(bytes.length,record.optimized_bytes,label+": incorrect byte length");
  assert.equal(crypto.createHash("sha256").update(bytes).digest("hex"),record.optimized_sha256,
   label+": optimized source SHA-256 has changed");
  assert(bytes.length>1200&&bytes.length<250000);
  assert.equal(bytes[0],255);assert.equal(bytes[1],216);
  assert.equal(bytes[bytes.length-2],255);assert.equal(bytes[bytes.length-1],217);
  bytesTotal+=bytes.length;tiles[label]=encoded;
  signatures.set(crypto.createHash("sha256").update(bytes).digest("hex"),label);
 }
}
assert.equal(Object.keys(tiles).length,9);
const gl={TEXTURE_2D:1,UNPACK_ALIGNMENT:2,RGBA:3,UNSIGNED_BYTE:4,LINEAR:5,LINEAR_MIPMAP_LINEAR:6,
 CLAMP_TO_EDGE:7,TEXTURE_WRAP_S:8,TEXTURE_WRAP_T:9,TEXTURE_MAG_FILTER:10,TEXTURE_MIN_FILTER:11,
 TEXTURE0:12,TEXTURE1:13,uploads:[],createTexture(){return{}},bindTexture(){},pixelStorei(){},
 texImage2D(target,level,fmt,w,h,pad,fmt2,type,data){
  this.uploads.push({w,h,bytes:data.length,firstAlpha:data[3],sample:Array.from(data.slice(0,4))});
 },texParameteri(){},generateMipmap(){},activeTexture(){},uniform1i(){}};
let lastKind="photo";
const ctx2d={clearRect(){},drawImage(bitmap){lastKind=bitmap.kind},
 getImageData(){
  const data=new Uint8ClampedArray(2048*512*4);
  const values=lastKind==="photo"?[80,103,59,255]:lastKind==="normal"?[128,128,246,255]:[88,88,88,255];
  for(let i=0;i<data.length;i+=4)for(let k=0;k<4;k++)data[i+k]=values[k];
  return {data};
 }};
const canvas={width:0,height:0,getContext(){return ctx2d}};
const mock={window:{},Math,Uint8Array,Blob,document:{createElement:()=>canvas},
 atob:s=>Buffer.from(s,"base64").toString("binary"),
 fetch:async url=>{
  const name=url.match(/\/(grass|mud|brick)_(photo|normal|roughness)\.jpg\.b64$/);
  return {ok:!!name,status:name?200:404,text:async()=>tiles[name[1]+"_"+name[2]]};
 },
 createImageBitmap:async blob=>{
  const data=Buffer.from(await blob.arrayBuffer());
  const label=signatures.get(crypto.createHash("sha256").update(data).digest("hex"));
  assert(label,"Unknown source bitmap");
  return {width:512,height:512,kind:label.split("_")[1],close(){}};
 }};
vm.runInNewContext(fs.readFileSync(path.join(root,"material-atlas.js"),"utf8"),mock);
const art=mock.window.WPMaterialAtlas;
(async()=>{
 const tex=art.upload(gl);
 assert.equal(await art.enhance(gl,tex),true);
 assert.deepEqual(Array.from(tex.size),[2048,512]);
 assert.equal(tex.realNormalRoughness,true);
 assert.equal(gl.uploads.length,4);
 assert.equal(gl.uploads[2].bytes,2048*512*4);
 assert.equal(gl.uploads[2].firstAlpha,88,"Artist-authored roughness not packed to alpha");
 assert.equal(gl.uploads[3].sample[2],246,"Genuine OpenGL normal blue channel not packed");
 console.log("PASS real CC0 terrain photo bundle:",JSON.stringify({
  maps:9,bytes:bytesTotal,atlases:2,textureSize:tex.size,provenanceHashesVerified:true}));
})().catch(e=>{console.error(e);process.exitCode=1});
