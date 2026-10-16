/* A/B comparison in *one* Chromium process/runner.
 * Same viewport, pinned SwiftShader, same real game, no synthetic FPS.
 * Reference build is main served from a separately checked-out worktree.
 */
import {chromium} from "playwright";
import fs from "node:fs/promises";
const locations={base:"http://127.0.0.1:8766/3d/",candidate:"http://127.0.0.1:8765/3d/"};
const browser=await chromium.launch({headless:true,args:[
 "--enable-webgl","--use-gl=angle","--use-angle=swiftshader"
]});
const outputs=[];
async function sample(which,quality,repeat){
 const viewport={width:quality==="low"?854:1000,height:quality==="low"?480:562};
 const ctx=await browser.newContext({viewport,deviceScaleFactor:1});
 const page=await ctx.newPage(),errors=[];
 page.on("pageerror",err=>errors.push(String(err)));
 try{
  await page.goto(locations[which],{waitUntil:"domcontentloaded",timeout:30000});
  await page.waitForFunction(()=>!!window.WP3DDiagnostics&&!!window.WP3DProbe,{timeout:20000});
  await page.locator("#quality").selectOption(quality);
  await page.locator("#deploy").click();
  await page.waitForFunction(()=>window.WP3DDiagnostics().assetsLoaded,{timeout:30000});
  if(which==="candidate"){
   await page.waitForFunction(()=>window.WP3DDiagnostics().dressingReady,{timeout:20000});
   const d=await page.evaluate(()=>window.WP3DDiagnostics());
   if(d.dressingInstances<1||d.dressingTriangles>4800)
    throw Error("CC0 dressing not ready for paired FPS test: "+d.dressingError);
  }
  await page.waitForTimeout(850);
  await page.evaluate(()=>window.WP3DProbe.reset());
  const start=Date.now();
  await page.waitForFunction(()=>window.WP3DProbe.report().sampling.frames>=24,
   null,{timeout:45000}).catch(()=>{});
  const report=await page.evaluate(()=>window.WP3DProbe.report());
  const result={which,quality,repeat,frames:report.sampling.frames,
   fps:report.performance.avgFps,p95:report.performance.p95FrameMs,
   renderCpuMs:report.performance.avgRenderCpuMs,elapsedMs:Date.now()-start,
   errors};
  if(errors.length)throw Error(which+" "+quality+" browser crash: "+errors.join("; "));
  if(!Number.isFinite(result.fps)||result.frames<12)
   throw Error(which+" "+quality+" failed to render at least 3 actual frames: "+JSON.stringify(result));
  outputs.push(result);console.log("AB_FRAME_RESULT",JSON.stringify(result));
 }finally{await ctx.close()}
}
let failed=null;
try{
 // ABBA order limits systematic compile/warmup/test-order bias.
 for(const quality of ["low","medium"]){
  for(const which of ["base","candidate","candidate","base"])
   await sample(which,quality,outputs.length);
 }
}catch(err){failed=String(err.stack||err)}
finally{await browser.close()}
const comparisons={};
for(const quality of ["low","medium"]){
 const group=outputs.filter(x=>x.quality===quality);
 const mean=which=>{const rows=group.filter(x=>x.which===which);
  return rows.length===2?rows.reduce((sum,x)=>sum+x.fps,0)/2:null;};
 const reference=mean("base"),candidate=mean("candidate");
 comparisons[quality]={referenceFps:reference,candidateFps:candidate,
  ratio:reference&&candidate?candidate/reference:null};
}
await fs.mkdir("artifacts/benchmarks",{recursive:true});
await fs.writeFile("artifacts/benchmarks/paired-comparison.json",
 JSON.stringify({outputs,comparisons,failed},null,2));
console.log("AB_COMPARISON",JSON.stringify(comparisons));
if(failed){console.error(failed);process.exitCode=1;}
else for(const [tier,c] of Object.entries(comparisons)){
 // Severe reproducible regression threshold, not a performance target.
 if(!c.ratio||c.ratio<.6){
  console.error("AB_PERFORMANCE_REGRESSION "+tier+" "+JSON.stringify(c));
  process.exitCode=1;
 }
}
