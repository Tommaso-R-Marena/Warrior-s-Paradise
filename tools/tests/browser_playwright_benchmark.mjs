/* Real Chromium movement, combat, and frame-time measurement; saved as CI artifacts. */
import {chromium} from "playwright";
import fs from "node:fs/promises";
import path from "node:path";
const url=(process.env.WP_TEST_URL||"http://127.0.0.1:8765/3d/")+"?qa=1";
const dest=path.resolve("artifacts/benchmarks");await fs.mkdir(dest,{recursive:true});
const browser=await chromium.launch({headless:true,args:["--enable-webgl","--use-gl=angle","--use-angle=swiftshader"]});
const results=[];let failure="";
const assert=(condition,message)=>{if(!condition)throw Error(message)};
const dist=(a,b)=>Math.hypot(a.x-b.x,a.z-b.z);
async function run(name,viewport,quality,mobile){
 const ctx=await browser.newContext({viewport,deviceScaleFactor:1,isMobile:mobile,hasTouch:mobile});
 const page=await ctx.newPage(),errors=[];page.on("pageerror",e=>errors.push(String(e)));
 try{
  await page.goto(url,{waitUntil:"domcontentloaded",timeout:30000});
  await page.waitForFunction(()=>!!window.WP3DDiagnostics&&!!window.WP3DQA,{timeout:20000});
  await page.locator("#quality").selectOption(quality);
  await page.locator("#difficulty").selectOption("recruit");
  await page.locator("#deploy").click();
  await page.waitForFunction(()=>window.WP3DDiagnostics().assetsLoaded&&window.WP3DDiagnostics().rigLoaded,{timeout:20000});
  await page.waitForFunction(()=>window.WP3DDiagnostics().dressingReady,{timeout:20000});
  const dressing=await page.evaluate(()=>window.WP3DDiagnostics());
  assert(dressing.dressingInstances>0&&dressing.dressingTriangles<=4800,
   name+": additional CC0 scene assets missing or too costly: "+dressing.dressingError);
  await page.waitForTimeout(500);
  if(quality!=="low"){
   await page.waitForFunction(()=>{
    const d=window.WP3DDiagnostics();
    return d.photoMaterialsLoaded||!!d.photoMaterialsError;
   },null,{timeout:30000});
  }
  const initial=await page.evaluate(()=>window.WP3DDiagnostics());
  if(quality!=="low")assert(initial.photoMaterialsLoaded,
   name+": photographic maps did not load: "+initial.photoMaterialsError);
  if(quality==="medium")assert(initial.materialMode==="medium",
   "Medium must use cost-bounded photographic albedo atlas");
  if(quality==="high"){
   assert(initial.highShaderReady,"High dedicated normal shader did not compile");
   assert(initial.realPBRMaps&&initial.materialMode==="detailed",
    name+": artist-authored normals/roughness missing: "+initial.photoMaterialsError);
  }
  assert(initial.texturesLoaded&&initial.renderer==="WebGL2",name+": graphics pipeline unavailable");
  await page.evaluate(()=>window.WP3DProbe.reset());
  const start=await page.evaluate(()=>window.WP3DDiagnostics().position);
  // User input is genuine keyboard/touch. The same in-game simulation is
  // stepped directly to test physics when SwiftShader starves requestAnimationFrame.
  const step=ticks=>page.evaluate(t=>window.WP3DQA.step(t),ticks);
  if(mobile){
   const b=await page.locator("#left-stick").boundingBox();
   assert(b&&b.width>55,"Mobile joystick hidden");
   const x=b.x+b.width/2,y=b.y+b.height/2;
   await page.mouse.move(x,y);await page.mouse.down();await page.mouse.move(x,y-b.height*.32,{steps:5});
   await step(125);await page.mouse.up();
  }else{
   await page.keyboard.down("w");await step(125);await page.keyboard.up("w");
  }
  const diagnosticAfter=await page.evaluate(()=>window.WP3DDiagnostics());
  const after=diagnosticAfter.position;
  console.log("MOVEMENT_DIAGNOSTIC",name,JSON.stringify({
   from:start,to:after,playing:diagnosticAfter.playing,
   gameTime:diagnosticAfter.elapsedSeconds,input:diagnosticAfter.input,
   fps:diagnosticAfter.framesPerSecond,perf:diagnosticAfter.performance}));
  if(dist(start,after)<=1.1)await page.screenshot({path:path.join(dest,name+"-stuck.png")});
  assert(dist(start,after)>1.1,name+": movement stuck ("+dist(start,after)+" m)");
  if(!mobile){
   const before=after;
   await page.keyboard.down("ArrowDown");await step(35);await page.keyboard.up("ArrowDown");
   const last=await page.evaluate(()=>window.WP3DDiagnostics().position);
   assert(dist(before,last)>.20,"Arrow key movement failed");
  }
  await page.keyboard.press("c");await page.keyboard.press("x");await page.keyboard.press("t");
  const gunBefore=await page.locator("#mag").innerText();
  if(mobile){
   const b=await page.locator("#fire").boundingBox();
   assert(b,"Mobile fire button not displayed");
   await page.mouse.move(b.x+b.width/2,b.y+b.height/2);
  }else await page.mouse.move(viewport.width/2,viewport.height/2);
  await page.mouse.down();await step(35);await page.mouse.up();
  // Independently observe browser-driven rAF frame pacing under actual CI GPU.
  // With software WebGL this may be extremely slow. Report it, never invent FPS.
  await page.waitForTimeout(1500);
  const final=await page.evaluate(()=>window.WP3DDiagnostics());
  const report=await page.evaluate(()=>window.WP3DProbe.report());
  // A zero count is itself a serious rendering/driver failure, not a
  // gameplay-physics failure; persist it separately for performance triage.
  assert(report.gameplay.metersTraveled>1.0,"Measured player movement absent");
  assert(final.squadOrder==="hold"&&final.support===0,"Tactical controls failed");
  assert(report.gameplay.shots>0||gunBefore!==(await page.locator("#mag").innerText()),"Firing failed");
  assert(Number.isFinite(report.performance.p95FrameMs),"Invalid frame data");
  assert(!errors.length,"Uncaught browser exceptions: "+errors.join("; "));
  await page.screenshot({path:path.join(dest,name+".png"),fullPage:true});
  const outcome={name,quality,viewport,metersMoved:dist(start,after),initial,final,report,
   renderedFrameCount:report.sampling.frames,
   benchmarkStatus:report.sampling.frames<4?"GPU rendering severely throttled":"Observed browser frames",
   note:"Software-GPU GitHub hosted Chromium performance, not real phone or GPU measurements"};
  results.push(outcome);
  await fs.writeFile(path.join(dest,name+".json"),JSON.stringify(outcome,null,2));
  console.log("PASS PHYSICS",name,"movement",outcome.metersMoved.toFixed(2)+"m",
   "mean",report.performance.avgFps,"FPS","p95",report.performance.p95FrameMs+"ms");
 }finally{await ctx.close();}
}
try{
 await run("desktop-low",{width:854,height:480},"low",false);
 await run("mobile-low",{width:390,height:844},"low",true);
 await run("desktop-balanced",{width:1280,height:720},"medium",false);
 await run("desktop-high",{width:960,height:540},"high",false);
}catch(e){failure=String(e.stack||e);console.error(failure);}
finally{await browser.close();}
const md=["# Warrior’s Paradise actual browser benchmark","","Automated Chromium/SwiftShader results are hardware-specific; do not interpret as gaming PC or phone FPS.","",
 "| Mode | Movement (m) | Avg FPS | 1% low FPS | p95 frame ms | p95 render CPU ms |",
 "|---|---:|---:|---:|---:|---:|",
 ...results.map(r=>{const p=r.report.performance;return "| "+[r.name,r.metersMoved.toFixed(2),p.avgFps,p.onePercentLowFps,p.p95FrameMs,p.p95RenderCpuMs].join(" | ")+" |"})];
if(failure)md.push("","FAILED: "+failure.replaceAll("\n"," "));
await fs.writeFile(path.join(dest,"summary.md"),md.join("\n")+"\n");
await fs.writeFile(path.join(dest,"summary.json"),JSON.stringify({pass:!failure,results,failure},null,2));
if(failure)process.exitCode=1;
