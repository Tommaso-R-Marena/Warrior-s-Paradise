/* Actual Chromium regression: terrain relief, rock solidity and swept movement.
 * Uses the same Cloudflare static build and production game simulation.
 */
import {chromium} from "playwright";
const assert=(v,m)=>{if(!v)throw Error(m);};
const base=process.env.WP_TEST_URL||"http://127.0.0.1:8765/3d/";
const browser=await chromium.launch({headless:true,
 args:["--enable-webgl","--use-gl=angle","--use-angle=swiftshader"]});
try{
 const page=await browser.newPage({viewport:{width:960,height:620}});
 const errors=[];page.on("pageerror",err=>errors.push(String(err)));
 await page.goto(base+(base.includes("?")?"&":"?")+"qa=1",
  {waitUntil:"domcontentloaded",timeout:30000});
 await page.locator("#quality").selectOption("medium");
 await page.locator("#deploy").click();
 await page.waitForFunction(()=>!!window.WP3DQA&&window.WP3DDiagnostics?.().dressingReady,
  null,{timeout:25000});
 const result=await page.evaluate(()=>{
  const qa=window.WP3DQA;
  const sample=[];
  for(let z=-40;z<42;z+=6)for(let x=-40;x<42;x+=6)
   sample.push(qa.terrainProbe(x,z));
  const heights=sample.map(p=>p.y),max=Math.max(...heights),min=Math.min(...heights);
  const obstacles=qa.collisionExamples();
  const rocks=obstacles.rocks.slice(0,15);
  let rockTest=null;
  for(const r of rocks){
   const [x,z,w,d]=r;
   if(!qa.terrainProbe(x,z,.15).blocked)continue;
   for(const direction of [-1,1]){
    const sx=x+direction*(w/2+1.1),sz=z;
    if(qa.terrainProbe(sx,sz,.44).blocked)continue;
    const [mx,mz]=qa.traverse(sx,sz,-direction*(w/2+2.0),0);
    rockTest={start:[sx,sz],rock:[x,z,w,d],end:[mx,mz],
      endedInside:Math.abs(mx-x)<w/2+.40&&Math.abs(mz-z)<d/2+.4,
      delta:Math.hypot(mx-sx,mz-sz)};
    break;
   }
   if(rockTest)break;
  }
  let steep=null;
  for(const p of sample)if(p.slope>.75&&!p.blocked){
   const end=qa.traverse(p.x,p.z,.16,0);
   if(Math.hypot(end[0]-p.x,end[1]-p.z)<.08){steep={slope:p.slope,from:[p.x,p.z],to:end};break;}
  }
  return {relief:max-min,max,min,rockCount:obstacles.rocks.length,
    procedural:obstacles.core.length,rockTest,steep,
    diagnostics:window.WP3DDiagnostics()};
 });
 assert(result.relief>.48,
  "All rendered terrain is still a flat plane: "+JSON.stringify(result).slice(0,1200));
 assert(result.rockCount>0,
  "Imported large rocks need registered physical collision shapes");
 assert(result.procedural>0,"Scattered rocks and timber still pass-through");
 assert(result.rockTest&&!result.rockTest.endedInside,
  "Actual player trajectory must stop at the rock footprint: "+JSON.stringify(result.rockTest));
 assert(result.diagnostics.physicalTerrainTriangles===722,
  "Low/Medium physics must use the shared 19x19 triangle ground");
 assert(result.diagnostics.terrainAtPlayer===
  (await page.evaluate(()=>window.WP3DQA.terrainProbe(
   window.WP3DDiagnostics().position.x,window.WP3DDiagnostics().position.z).y)),
  "Player ground position and rendered terrain must agree");
 assert(errors.length===0,"Browser runtime errors: "+JSON.stringify(errors));
 console.log("PASS ACTUAL CHROMIUM TOPOGRAPHY",JSON.stringify({
  relief:Number(result.relief.toFixed(2)),rocks:result.rockCount,
  procedural:result.procedural,rockStop:result.rockTest,steep:result.steep,
  groundTriangles:result.diagnostics.physicalTerrainTriangles
 }));
}finally{await browser.close();}
