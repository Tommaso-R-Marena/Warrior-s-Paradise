"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),
vm=require("node:vm"),crypto=require("node:crypto");
const root=path.resolve(__dirname,"../../browser/3d");
const manifest=JSON.parse(fs.readFileSync(path.join(root,"assets/kenney/CHARACTER_MANIFEST.json"),"utf8"));
assert.equal(manifest.license,"CC0-1.0");
const encoded=fs.readFileSync(path.join(root,"assets/kenney/character_medium.glb.b64"),"utf8").trim();
const buffer=Buffer.from(encoded,"base64");
assert.equal(buffer.toString("base64"),encoded,"Noncanonical GLB source");
assert.equal(buffer.length,manifest.source_bytes);
assert.equal(crypto.createHash("sha256").update(buffer).digest("hex"),manifest.upstream_sha256);
assert.equal(crypto.createHash("sha1").update("blob "+buffer.length+"\0").update(buffer).digest("hex"),
 manifest.upstream_git_blob_sha1);
const scope={window:{},TextDecoder,Uint8Array,DataView,ArrayBuffer,Float32Array,Math};
vm.runInNewContext(fs.readFileSync(path.join(root,"skinning.js"),"utf8"),scope,
 {filename:"skinning.js",timeout:3000});
const sk=scope.window.WPSkinning;
const ab=buffer.buffer.slice(buffer.byteOffset,buffer.byteOffset+buffer.length);
const model=sk.parse(ab);
assert.equal(model.joints.length,58);
assert.equal(model.triangleCount,1604);
assert.equal(model.animations.length,0,"Unexpected animations; revisit curated model assumptions");
const character={x:2,z:5,moveSpeed:3,phase:0,facing:.7,crouch:false};
function geometry(t,mode){
 const triangles=[],actor={...character,moveSpeed:mode==="walk"?3:0,crouch:mode==="crouch"};
 const result=sk.append({tri(a,b,c,col){triangles.push(...a,...b,...c,...col)}},model,actor,"ally",t);
 assert.equal(result.triangles,1604);
 assert(triangles.every(Number.isFinite),"Bad skinned vertex coordinates");
 return triangles;
}
const walk0=geometry(.3,"walk"),walk1=geometry(.7,"walk"),crouch=geometry(.7,"crouch");
assert(walk0.some((v,i)=>Math.abs(v-walk1[i])>.01),"Gait should deform actual skinned geometry");
assert(walk1.some((v,i)=>Math.abs(v-crouch[i])>.01),"Crouch pose should deform vertices");
const bad=Buffer.from(buffer);bad.writeUInt32LE(0,0);
assert.throws(()=>sk.parse(bad.buffer.slice(bad.byteOffset,bad.byteOffset+bad.length)),
 /bad GLB header/);
const original=sk.pose(model,0,"aim");
assert.equal(original.length,58);
console.log("PASS genuine CC0 58-bone glTF skinning:",JSON.stringify({
 bytes:buffer.length,joints:model.joints.length,triangles:model.triangleCount,
 animations:model.animations.length,poses:["walk","crouch","aim"],corruptionRejected:true}));
