"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const js=fs.readFileSync(path.resolve(__dirname,"../../browser/3d/device-gpu-profiler.js"),"utf8");
const root={};vm.runInNewContext(js,{window:root,Date,Math});
assert.equal(root.WPDeviceGpuProfiler.create({getExtension:()=>null}).report().supported,false);
const ext={TIME_ELAPSED_EXT:301,GPU_DISJOINT_EXT:302};
let created=0,deleted=0,active=0,disjoint=false;
const gl={
 QUERY_RESULT_AVAILABLE:400,QUERY_RESULT:401,VENDOR:402,RENDERER:403,
 getExtension:n=>n==="EXT_disjoint_timer_query_webgl2"?ext:null,
 getParameter:n=>n===302?disjoint:n===402?"Test Vendor":"Hardware Renderer",
 createQuery:()=>({id:++created}),beginQuery:()=>{active++},
 endQuery:()=>{active--},
 getQueryParameter:(q,n)=>n===400?true:8500000,
 deleteQuery:()=>{deleted++}
};
const timer=root.WPDeviceGpuProfiler.create(gl);
for(let i=0;i<48;i++){timer.begin();timer.end();}
const measured=timer.report();
assert.equal(active,0);
assert(measured.supported&&measured.queries>=3&&measured.meanGpuMs===8.5);
assert.equal(deleted,created);
disjoint=true;for(let i=0;i<12;i++){timer.begin();timer.end();}
timer.report();
assert(timer.report().gpuDisjointEvents>=1,"Disjoint samples should not be retained");
timer.dispose();assert.equal(active,0);
console.log("PASS actual GPU timing instrumentation: extension optional, query lifetime and disjoint");
