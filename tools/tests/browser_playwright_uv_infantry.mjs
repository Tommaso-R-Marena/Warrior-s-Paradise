/* Checks actual asset-backed skinned characters are rendered on the
 * WebGL2 battlefield with sourced UV fabric. Node tests separately assert
 * smooth TRS blending across exact animated glTF keyframes.
 */
import {chromium} from "playwright";
import fs from "node:fs/promises";
const browser=await chromium.launch({headless:true,args:[
 "--enable-webgl","--use-gl=angle","--use-angle=swiftshader"]});
const url=(process.env.WP_TEST_URL||"http://127.0.0.1:8765/3d/")+"?qa=1";
const check=(q,msg)=>{if(!q)throw Error(msg)};
try{
 const page=await browser.newPage({viewport:{width:1100,height:680}});
 const errors=[];page.on("pageerror",e=>errors.push(String(e)));
 await page.goto(url,{waitUntil:"domcontentloaded",timeout:30000});
 await page.locator("#quality").selectOption("high");
 await page.locator("#deploy").click();
 await page.waitForFunction(()=>window.WP3DDiagnostics?.().playing,null,{timeout:12000});
 await page.evaluate(()=>window.WP3DQA.combatScenario({x:0,z:0,yaw:0,pitch:0,
  aim:true,enemies:[{x:0,z:9,hp:80,alert:0,facing:Math.PI}]}));
 await page.waitForFunction(()=>{
  const d=window.WP3DDiagnostics?.();
  return d?.rigLoaded&&(d.ww2Rigs?.axis||d.ww2Rigs?.ally)
   &&d.skinUVFabricTriangles>0&&d.skinActors>0;
 },null,{timeout:95000});
 const d=await page.evaluate(()=>window.WP3DDiagnostics());
 check(d.skinActors>=1&&d.skinTriangles>100,"Rig animation disappeared");
 check(d.skinUVFabricTriangles>100,"Actual asset UV0 was not preserved");
 check(d.defenders?.length===1&&d.defenders[0].hp===80,
  "Textures or animation erroneously affected gameplay health");
 check(d.renderer==="WebGL2"&&d.playing,"Game no longer running");
 const glError=await page.evaluate(()=>document.getElementById("viewport").getContext("webgl2").getError());
 check(glError===0,"WebGL error with UV-animated infantry: "+glError);
 check(!errors.length,"Browser errors: "+errors.join("; "));
 await fs.mkdir("artifacts/benchmarks",{recursive:true});
 await page.screenshot({path:"artifacts/benchmarks/high-uv-uniform-skeletal-animation.png",
  timeout:90000,animations:"disabled"});
 console.log("PASS REAL GLTF UV UNIFORM AND SKELETAL WEBGL",JSON.stringify({
  skinActors:d.skinActors,triangles:d.skinTriangles,uvTriangles:d.skinUVFabricTriangles,
  historical:d.ww2Rigs,glError}));
}finally{await browser.close();}
