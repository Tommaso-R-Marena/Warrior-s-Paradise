"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=path.resolve(__dirname,"../../browser/3d");
const env={window:{},Uint8Array,Math};
vm.runInNewContext(fs.readFileSync(path.join(root,"material-atlas.js"),"utf8"),env);
const mk=env.window.WPMaterialAtlas;
const a=mk.generate(),b=mk.generate();
assert.equal(a.width,512);assert.equal(a.height,128);
assert.equal(a.tiles,4);
assert.equal(a.color.length,262144);assert.equal(a.normal.length,262144);
assert.deepEqual(Buffer.from(a.color),Buffer.from(b.color),"Texture must be reproducible");
assert.deepEqual(Buffer.from(a.normal),Buffer.from(b.normal),"Normal map must be reproducible");
assert(a.color.some((v,i)=>i%4===3&&v>190),"Roughness channel omitted");
const heads=[0,1,2,3].map(tile=>[...a.color.slice(tile*128*4,tile*128*4+3)]);
assert(new Set(heads.map(x=>x.join(","))).size===4,"Materials look identical");
const gl={TEXTURE_2D:2,RGBA:4,UNSIGNED_BYTE:1,UNPACK_ALIGNMENT:5,
 LINEAR_MIPMAP_LINEAR:6,LINEAR:7,CLAMP_TO_EDGE:8,TEXTURE_WRAP_S:9,TEXTURE_WRAP_T:10,
 TEXTURE_MAG_FILTER:11,TEXTURE_MIN_FILTER:12,TEXTURE0:13,TEXTURE1:14,
 textures:0,uploads:0,createTexture(){return{number:++this.textures}},bindTexture(){},
 pixelStorei(){},texImage2D(target,level,internal,width,height,border,format,type,data){
  assert.equal(width,512);assert.equal(height,128);
  assert.equal(data.length,262144);this.uploads++;
 },texParameteri(){},generateMipmap(){},activeTexture(){},uniform1i(){}};
const textures=mk.upload(gl);
assert.equal(gl.uploads,2);assert.equal(gl.textures,2);
assert(mk.bind(gl,{diffuse:{},normal:{}},textures));
console.log("PASS tiled PBR material atlas:",JSON.stringify({size:textures.size,tiles:a.tiles,uploads:gl.uploads}));
