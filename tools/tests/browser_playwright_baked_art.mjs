/* Real Chromium acceptance for permanent, serverless baked-art quality tiers. */
import {chromium} from "playwright";
import fs from "node:fs/promises";
const browser=await chromium.launch({headless:true,
 args:["--enable-webgl","--use-gl=angle","--use-angle=swiftshader"]});
const url=(process.env.WP_TEST_URL||"http://127.0.0.1:8765/3d/")+"?qa=1";
const assert=(condition,message)=>{if(!condition)throw Error(message)};
try{
 const page=await browser.newPage({viewport:{width:1080,height:720}});
 const errors=[];page.on("pageerror",e=>errors.push(String(e)));
 const failed=[];page.on("requestfailed",req=>{if(req.url().includes("/assets/baked/"))failed.push(req.url())});
 await page.goto(url,{waitUntil:"domcontentloaded",timeout:30000});
 await page.locator("#quality").selectOption("high");
 await page.locator("#deploy").click();
 await page.waitForFunction(()=>window.WP3DDiagnostics?.().bakedArt.complete,null,{timeout:80000});
 const d=await page.evaluate(()=>window.WP3DDiagnostics());
 assert(d.bakedArt.ready,"Verified baked art failed to activate: "+d.bakedArt.error);
 assert(d.bakedArt.instances===3,"Production static facade placements incorrect");
 assert(d.bakedArt.highTriangles===3180&&d.bakedArt.mediumTriangles===324,
  "Baked LOD budget wrong: "+JSON.stringify(d.bakedArt));
 assert(d.renderedStaticTriangles===d.geometryTriangles,"High must render full baked art");
 assert(d.highShaderReady,"Signed baked material variant did not compile on WebGL2");
 assert(d.renderer==="WebGL2"&&d.playing,"Baking changed the gameplay engine");
 assert(d.bakedArt.sources.length===1&&d.bakedArt.sources[0].license==="original",
  "Shipped scene missing source/rights details");
 assert(!failed.length,"Failed local baked mesh files: "+failed.join(", "));
 assert(!errors.length,"Chromium runtime errors: "+errors.join(" ; "));
 const glErrors=await page.evaluate(()=>{
  const gl=document.getElementById("viewport").getContext("webgl2");
  return gl.getError();
 });
 assert(glErrors===0,"GPU shader/mesh upload produced GL error: "+glErrors);
 await fs.mkdir("artifacts/benchmarks",{recursive:true});
 await page.screenshot({path:"artifacts/benchmarks/baked-art-high.png"});
 console.log("PASS BAKED ART REAL CHROMIUM",JSON.stringify({
  baked:d.bakedArt,staticTriangles:d.renderedStaticTriangles,glErrors}));
 await page.close();
}finally{await browser.close();}
