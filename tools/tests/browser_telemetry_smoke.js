/* Offline unit test for user-visible, locally generated frame + gameplay report. */
"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const code=fs.readFileSync(path.resolve(__dirname,"../../browser/3d/telemetry.js"),"utf8");
const panel={hidden:true},pre={textContent:""};
const mock={window:{devicePixelRatio:2},performance:{now:()=>1000},
 Date,Math,navigator:{userAgent:"test",hardwareConcurrency:4},
 document:{getElementById:id=>id==="performance-panel"?panel:id==="performance-details"?pre:null}};
vm.runInNewContext(code,mock);
const p=mock.window.WP3DProbe;assert(p);
for(let i=0;i<240;i++)p.frame(i%30===0?48:16.7,3.2,.7);
for(let i=0;i<100;i++)p.movement(i*.12,i*.05,1,4.5,.04);
for(let i=0;i<10;i++){p.event("shot");if(i%2===0)p.event("hit");}
p.event("kill");p.event("capture");p.event("heal");
const r=p.report();
assert(r.performance.avgFps>40&&r.performance.avgFps<65);
assert(r.performance.onePercentLowFps<r.performance.avgFps);
assert(r.performance.p95FrameMs<=r.performance.p99FrameMs);
assert.equal(r.sampling.frames,240);
assert(r.gameplay.metersTraveled>10);
assert.equal(r.gameplay.shots,10);assert.equal(r.gameplay.hits,5);
assert.equal(r.gameplay.accuracyPercent,50);assert.equal(r.gameplay.objectivesCaptured,1);
p.toggle();assert.equal(panel.hidden,false);assert.match(pre.textContent,/FPS/);
p.toggle();assert.equal(panel.hidden,true);
p.reset();
assert.equal(p.report().sampling.frames,0);
assert.equal(p.report().gameplay.shots,0);
console.log("PASS local frame and gameplay telemetry:",JSON.stringify({fps:r.performance.avgFps,onePercentLow:r.performance.onePercentLowFps,movement:r.gameplay.metersTraveled,shotAccuracy:r.gameplay.accuracyPercent}));
