/* Actual WebGL2 acceptance of independent UV albedo/normal/roughness/metal/UV1
 * lightmaps on the static battlefield. No fake/mock WebGL substitutes.
 */
import {chromium} from "playwright";
import fs from "node:fs/promises";
const browser=await chromium.launch({headless:true,args:["--enable-webgl","--use-gl=angle","--use-angle=swiftshader"]});
const url=(process.env.WP_TEST_URL||"http://127.0.0.1:8765/3d/")+"?qa=1";
function assert(v,msg){if(!v)throw Error(msg)}
try{
 const page=await browser.newPage({viewport:{width:1200,height:720}});
 const errors=[],failed=[];
 page.on("pageerror",e=>errors.push(String(e)));
 page.on("requestfailed",r=>{if(r.url().includes("/assets/pbr/"))failed.push(r.url())});
 await page.goto(url,{waitUntil:"domcontentloaded",timeout:30000});
 await page.locator("#quality").selectOption("high");
 await page.locator("#deploy").click();
 await page.waitForFunction(()=>window.WP3DDiagnostics?.().uvPBR.complete,null,{timeout:95000});
 await page.waitForFunction(()=>window.WP3DDiagnostics().uvPBR.frames>0,null,{timeout:30000});
 const d=await page.evaluate(()=>window.WP3DDiagnostics());
 assert(d.uvPBR.ready,"UV/PBR runtime did not load: "+d.uvPBR.error);
 assert(d.uvPBR.groups===1,"Incorrect material draws");
 assert(d.uvPBR.highTriangles===7050&&d.uvPBR.mediumTriangles===6,"Wrong LODs: "+JSON.stringify(d.uvPBR));
 assert(d.uvPBR.lastTier==="high"&&d.uvPBR.renderedTriangles===7050,"PBR scene not rasterized");
 assert(d.renderer==="WebGL2"&&d.playing&&d.highShaderReady,"Gameplay/high shader broken");
 assert(d.uvPBR.sources?.[0]?.license==="original","Missing asset provenance");
 const errorCode=await page.evaluate(()=>document.getElementById("viewport").getContext("webgl2").getError());
 assert(errorCode===0,"GL error "+errorCode);
 assert(!failed.length,"Missing local textures: "+failed.join(";"));
 assert(!errors.length,"WebGL runtime errors: "+errors.join(";"));
 await fs.mkdir("artifacts/benchmarks",{recursive:true});
 await page.screenshot({path:"artifacts/benchmarks/uv-pbr-lightmaps-high.png"});
 console.log("PASS REAL UV PBR LIGHTMAP",JSON.stringify({pbr:d.uvPBR,glError:errorCode}));
 await page.close();
}finally{await browser.close();}
