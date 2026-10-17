/* Real WebGL2/SwiftShader road and Sherman physical trail acceptance.
 * Tests grounded static High geometry and actual input-driven tread stamping,
 * with a Low boot to ensure expensive dynamic marks stay disabled.
 */
import {chromium} from "playwright";
import fs from "node:fs/promises";
const browser=await chromium.launch({headless:true,args:[
 "--enable-webgl","--use-gl=angle","--use-angle=swiftshader"]});
const url=(process.env.WP_TEST_URL||"http://127.0.0.1:8765/3d/")+"?qa=1";
const assert=(q,msg)=>{if(!q)throw Error(msg);};
try{
 const page=await browser.newPage({viewport:{width:1024,height:690}});
 const errors=[];page.on("pageerror",e=>errors.push(String(e)));
 await page.goto(url,{waitUntil:"domcontentloaded",timeout:30000});
 await page.locator("#quality").selectOption("high");
 await page.locator("#deployment").selectOption("sherman");
 await page.locator("#deploy").click();
 await page.waitForFunction(()=>{
  const d=window.WP3DDiagnostics?.();
  return d?.dressingReady&&d?.highSurface&&d?.armor?.ready;
 },null,{timeout:70000});
 const before=await page.evaluate(()=>window.WP3DDiagnostics());
 assert(before.groundedRoad?.segments>=42,"road is still disconnected slabs");
 assert(before.groundedRoad.triangles<540,"low-priority static road too expensive");
 assert(before.highSurface.ruts>80&&before.highSurface.puddles>=10,
  "High wet cart ruts not in uploaded static VBO");
 assert(before.highSurface.triangles<800,"High added geometry is not capped");
 assert(before.armor.driving==="armor","Sherman was not mounted");
 await page.keyboard.down("w");
 await page.evaluate(()=>window.WP3DQA.step(240));
 await page.keyboard.up("w");
 const after=await page.evaluate(()=>window.WP3DDiagnostics());
 assert(after.trackImprints.marks>0&&after.trackImprints.marks<=90,
  "Sherman motion did not create a bounded physical track: "+JSON.stringify(after.trackImprints));
 assert(after.trackImprints.renderedTriangles>0,
  "Physical tread marks are not visible near the tank");
 assert(after.armor.vehicles.some(v=>v.id==="armor"&&v.speed>=0),
  "Sherman motion state corrupted");
 const glError=await page.evaluate(()=>document.getElementById("viewport").getContext("webgl2").getError());
 assert(glError===0,"GL error after dynamic marks: "+glError);
 assert(!errors.length,"Game page errors: "+errors.join(" ; "));
 await fs.mkdir("artifacts/benchmarks",{recursive:true});
 await page.screenshot({path:"artifacts/benchmarks/grounded-road-tank-tracks-high.png"});
 console.log("PASS CHROMIUM GROUNDED ROAD AND TRACKS",JSON.stringify({
  road:before.groundedRoad,high:before.highSurface,tracks:after.trackImprints,glError}));
}finally{await browser.close();}
