/* Verify real bundled photographic CC0 JPEGs decode in the atlas pipeline. */
"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=path.resolve(__dirname,"../../browser/3d");
const manifest=JSON.parse(fs.readFileSync(path.join(root,"assets/materials/MANIFEST.json"),"utf8"));
assert.equal(manifest.license,"CC0-1.0");
const tiles={};
for(const spec of manifest.assets){
 const source=fs.readFileSync(path.join(root,"assets/materials",spec.asset),"utf8").trim();
 assert.equal(source.length%4,0);
 const bytes=Buffer.from(source,"base64");
 assert.equal(bytes.toString("base64"),source);
 assert(bytes.length>1200&&bytes.length<90000);
 assert(bytes[0]===255&&bytes[1]===216&&bytes[bytes.length-2]===255&&bytes[bytes.length-1]===217);
 tiles[spec.name]=source;
}
const gl={TEXTURE_2D:1,UNPACK_ALIGNMENT:2,RGBA:3,UNSIGNED_BYTE:4,LINEAR:5,LINEAR_MIPMAP_LINEAR:6,
 CLAMP_TO_EDGE:7,TEXTURE_WRAP_S:8,TEXTURE_WRAP_T:9,TEXTURE_MAG_FILTER:10,TEXTURE_MIN_FILTER:11,
 TEXTURE0:12,TEXTURE1:13,uploads:[],createTexture(){return{}},bindTexture(){},pixelStorei(){},
 texImage2D(target,level,fmt,w,h,pad,fmt2,type,data){this.uploads.push({w,h,bytes:data.length})},
 texParameteri(){},generateMipmap(){},activeTexture(){},uniform1i(){}};
const ctx2d={drawImage(){},getImageData(){const data=new Uint8ClampedArray(1024*256*4);
 for(let i=0;i<data.length;i+=4){data[i]=60;data[i+1]=70;data[i+2]=45;data[i+3]=255;}
 return {data};}};
const canvas={width:0,height:0,getContext(){return ctx2d}};
const mock={window:{},Math,Uint8Array,Blob,document:{createElement:()=>canvas},
 atob:s=>Buffer.from(s,"base64").toString("binary"),
 fetch:async url=>{const name=url.match(/\/(grass|mud)_photo\.jpg\.b64$/)?.[1];
 return{ok:!!name,status:name?200:404,text:async()=>tiles[name]};},
 createImageBitmap:async blob=>{const bytes=new Uint8Array(await blob.arrayBuffer());
 assert.equal(bytes[0],255);return{width:256,height:256,close(){}}}};
vm.runInNewContext(fs.readFileSync(path.join(root,"material-atlas.js"),"utf8"),mock);
const art=mock.window.WPMaterialAtlas;
(async()=>{
 const tex=art.upload(gl);
 assert.equal(Object.keys(tiles).length,3);
 const ok=await art.enhance(gl,tex);
 assert.equal(ok,true);assert.deepEqual(Array.from(tex.size),[1024,256]);
 assert.equal(gl.uploads.length,4);assert.equal(gl.uploads[2].bytes,1024*256*4);
 console.log("PASS real CC0 terrain photo bundle:",JSON.stringify({tiles:3,uploaded:gl.uploads.length,size:tex.size}));
})().catch(e=>{console.error(e);process.exitCode=1});
