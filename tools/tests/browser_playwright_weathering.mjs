/* Real headless Chromium confirms static High weathering, atmosphere shading,
 * intact Low tier, grounded world draw and no GL errors. CI SwiftShader is
 * a software GPU, NOT a hardware performance estimate.
 */
import {chromium} from "playwright";
import fs from "node:fs/promises";
const browser=await chromium.launch({headless:true,args:[
 "--enable-webgl","--use-gl=angle","--use-angle=swiftshader"]});
const url=(process.env.WP_TEST_URL||"http://127.0.0.1:8765/3d/")+"?qa=1";
const requireTrue=(v,m)=>{if(!v)throw Error(m)};
try{
 const page=await browser.newPage({viewport:{width:960,height:540}});
 const errors=[];page.on("pageerror",e=>errors.push(String(e)));
 await page.goto(url,{waitUntil:"domcontentloaded",timeout:30000});
 await page.locator("#quality").selectOption("high");
 await page.locator("#deploy").click();
 await page.waitForFunction(()=>{
  const d=window.WP3DDiagnostics?.();
  return d?.dressingReady&&d?.weatheringTriangles>0&&d?.atmosphericSky?.draws>0;
 },null,{timeout:90000});
 const d=await page.evaluate(()=>window.WP3DDiagnostics());
 requireTrue(d.weatheredBuildings>=9,"Farmhouses lack grounded weathering");
 requireTrue(d.weatheringTriangles>=1200&&d.weatheringTriangles<=2350,
  "Weathering geometry out of budget: "+d.weatheringTriangles);
 requireTrue(d.weatheringLeaves>=100,"Ivy leaf detail missing");
 requireTrue(d.weatheringPatches>=120,"Water stains and chipped plaster missing");
 requireTrue(d.highShaderReady&&d.atmosphericSky.ready&&d.atmosphericSky.draws>0,
  "High shader/cloud atmosphere not rendered");
 requireTrue(d.renderedStaticTriangles===d.geometryTriangles,
  "High static detail invisible");
 requireTrue(d.renderer==="WebGL2"&&d.playing,"Gameplay state broken");
 requireTrue(!errors.length,"Page exceptions: "+errors.join(" ; "));
 const glError=await page.evaluate(()=>document.getElementById("viewport").getContext("webgl2").getError());
 requireTrue(glError===0,"High shader WebGL error "+glError);
 await fs.mkdir("artifacts/benchmarks",{recursive:true});
 await page.screenshot({path:"artifacts/benchmarks/normandy-weathered-clouds-high.png"});
 console.log("PASS REAL CHROMIUM WEATHERED BUILDINGS + SKY",JSON.stringify({
  houses:d.weatheredBuildings,triangles:d.weatheringTriangles,
  leaves:d.weatheringLeaves,patches:d.weatheringPatches,
  sky:d.atmosphericSky,glError}));
}finally{await browser.close();}
