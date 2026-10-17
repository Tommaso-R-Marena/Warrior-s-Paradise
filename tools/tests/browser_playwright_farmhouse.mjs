/* Real Chromium validation of window glass, shutters and render-only culling.
 * Budget and material identity are also checked by deterministic Node tests.
 */
import {chromium} from "playwright";
import fs from "node:fs/promises";
const browser=await chromium.launch({headless:true,args:[
 "--enable-webgl","--use-gl=angle","--use-angle=swiftshader"]});
const url=(process.env.WP_TEST_URL||"http://127.0.0.1:8765/3d/")+"?qa=1";
const check=(ok,msg)=>{if(!ok)throw Error(msg)};
try{
 const page=await browser.newPage({viewport:{width:1100,height:690}});
 const errors=[];page.on("pageerror",e=>errors.push(String(e)));
 await page.goto(url,{waitUntil:"domcontentloaded",timeout:30000});
 await page.locator("#quality").selectOption("high");
 await page.locator("#deploy").click();
 await page.waitForFunction(()=>{
  const d=window.WP3DDiagnostics?.();
  return d?.dressingReady&&d?.farmhouseCraft?.windows>0;
 },null,{timeout:85000});
 await page.evaluate(()=>window.WP3DQA.step(20));
 const d=await page.evaluate(()=>window.WP3DDiagnostics());
 check(d.farmhouseCraft.windows===36&&d.farmhouseCraft.shutters===72,
  "Historical four-pane windows and shutters missing");
 check(d.farmhouseCraft.roofPots===18,"Chimney pots not rendered");
 check(d.farmhouseCraft.triangles>1800&&d.farmhouseCraft.triangles<=2400,
  "No bounded High geometry");
 check(d.farmhouseCraft.drawCallsAdded===0&&d.farmhouseCraft.collidersAdded===0,
  "High decorations must not invent physical cover or draw passes");
 check(d.renderedStaticTriangles===d.geometryTriangles,"High world draw does not include farmhouse craft");
 check(d.atmosphericSky?.ready&&d.highShaderReady&&d.playing,"Original High graphics/gameplay broken");
 const computed=await page.evaluate(()=>{
  const actor={x:0,z:-35},player={x:0,z:0,yaw:0};
  return {rearHigh:window.WPActorVisibility.visible(actor,player,"high"),
   rearBalanced:window.WPActorVisibility.visible(actor,player,"medium")};
 });
 check(!computed.rearHigh&&computed.rearBalanced,"High actor culling must not affect Balanced rendering");
 check(!errors.length,"Runtime page errors: "+errors.join(";"));
 const glError=await page.evaluate(()=>document.querySelector("#viewport").getContext("webgl2").getError());
 check(glError===0,"GL errors: "+glError);
 await fs.mkdir("artifacts/benchmarks",{recursive:true});
 await page.screenshot({path:"artifacts/benchmarks/high-farmhouse-windows-visibility.png"});
 console.log("PASS REAL FARMHOUSE GLASS & CULLING",JSON.stringify({
  craft:d.farmhouseCraft,culled:d.culledActors,visible:d.visibleActors,glError,computed}));
}finally{await browser.close();}
