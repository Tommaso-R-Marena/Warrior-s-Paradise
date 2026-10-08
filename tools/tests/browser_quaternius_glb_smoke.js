"use strict";
const assert=require("node:assert/strict");
const fs=require("node:fs"),path=require("node:path"),vm=require("node:vm"),crypto=require("node:crypto");
const root=path.resolve(__dirname,"../../browser/3d");
const manifest=JSON.parse(fs.readFileSync(path.join(root,"assets/quaternius/MANIFEST.json"),"utf8"));
assert.equal(manifest.license,"CC0-1.0");
assert.equal(manifest.assets.length,4);
const env={window:{},TextDecoder,atob:s=>Buffer.from(s,"base64").toString("binary"),fetch};
vm.runInNewContext(fs.readFileSync(path.join(root,"asset-loader.js"),"utf8"),env);
const kit=env.window.WPAssetKit;
let size=0,triangles=0;
for(const item of manifest.assets){
 const file=path.join(root,"assets/quaternius",item.name+".glb.b64");
 const encoded=fs.readFileSync(file,"utf8").trim();
 const buf=Buffer.from(encoded,"base64");
 assert.equal(buf.toString("base64"),encoded,"Noncanonical GLB base64: "+item.name);
 assert.equal(buf.length,item.bytes);
 assert.equal(crypto.createHash("sha256").update(buf).digest("hex"),
  item.upstream_sha256,"Source integrity mismatch for "+item.name);
 assert.equal(crypto.createHash("sha1").update("blob "+buf.length+"\0").update(buf).digest("hex"),
  item.git_sha1,"Upstream Git blob mismatch for "+item.name);
 const ab=buf.buffer.slice(buf.byteOffset,buf.byteOffset+buf.length);
 const mesh=kit.parseGLB(ab);
 assert(mesh.count>300&&mesh.count<5000,"Unexpected GLB complexity "+item.name);
 assert(mesh.min.every(Number.isFinite)&&mesh.max.every(Number.is));
 const geo={n:0,tri(){this.n++}};
 assert.equal(kit.appendInstances(geo,mesh,[[1,0,1,1,0]]),mesh.count);
 assert.equal(geo.n,mesh.count);
 size+=buf.length;triangles+=mesh.count;
}
assert(size<600000);
console.log("PASS Quaternius farm artwork:",JSON.stringify({assets:4,bytes:size,triangles}));
