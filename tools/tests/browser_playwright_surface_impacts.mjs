/* Real WebGL2 ballistics-to-wall and artillery-to-ground visual contract.
 * No stubs, no extra draw call; verifies actual QA-shot collision reason and
 * physics-triggered impact effects, GPU state, and rendered triangles.
 */
import {chromium} from "playwright";
import fs from "node:fs/promises";
const browser=await chromium.launch({headless:true,args:[
 "--enable-webgl","--use-gl=angle","--use-angle=swiftshader"]});
const url=(process.env.WP_TEST_URL||"http://127.0.0.1:8765/3d/")+"?qa=1";
const assert=(x,m)=>{if(!x)throw Error(m)};
try{
 const page=await browser.newPage({viewport:{width:1050,height:670}});
 const errors=[];page.on("pageerror",e=>errors.push(String(e)));
 await page.goto(url,{waitUntil:"domcontentloaded",timeout:30000});
 await page.locator("#quality").selectOption("high");
 await page.locator("#deploy").click();
 await page.waitForFunction(()=>window.WP3DDiagnostics?.().playing,null,{timeout:20000});
 const bullet=await page.evaluate(()=>{
  window.WP3DQA.combatScenario({x:-24,z:-25,yaw:0,pitch:0,aim:true,weapon:"rifle",
   clearDecoration:true,enemies:[{x:-24,z:-4,hp:80,alert:0,facing:Math.PI}]});
  const before=window.WP3DDiagnostics().defenders[0].hp;
  const shot=window.WP3DQA.shootOnce();
  const after=window.WP3DDiagnostics();
  return {shot,damageBefore:before,damageAfter:after.defenders[0].hp,
   marks:after.combatSurface?.marks};
 });
 assert(!bullet.shot.hit&&bullet.shot.blockedBy==="masonry",
  "Real rifle shot must hit actual farmhouse cover: "+JSON.stringify(bullet));
 assert(bullet.damageBefore===bullet.damageAfter,"wall may not let shots through");
 assert(bullet.marks>=1,"ballistics did not create physical wall damage");
 await page.waitForFunction(()=>window.WP3DDiagnostics().combatSurface.renderedTriangles>0,
  null,{timeout:30000});
 const impact=await page.evaluate(()=>window.WP3DDiagnostics().combatSurface);
 assert(impact.marks>=1&&impact.renderedTriangles>0&&
  impact.renderedTriangles<=impact.maxTriangles,"Wall impact not rendered or over-budget");
 // Move only the QA enemy away from the artillery target; the test is
 // verifying that cosmetic effects cannot apply damage of their own.
 await page.evaluate(()=>window.WP3DQA.combatScenario({
  x:-24,z:-25,yaw:0,pitch:0,aim:true,weapon:"rifle",
  enemies:[{x:35,z:35,hp:100,alert:0,facing:Math.PI}]
 }));
 // The real desktop artillery shortcut calls the exact same
 // callArtillery() handler as the touch-only pointerdown button.
 await page.keyboard.press("t");
 const blast=await page.evaluate(()=>{
  window.WP3DQA.step(147);
  return window.WP3DDiagnostics();
 });
 assert(blast.combatSurface.explosions>=1,
  "Real artillery sequence failed to create registered ground damage");
 assert(blast.combatSurface.blasts>0&&blast.combatSurface.debris>0,
  "Ballistic dirt and stone fragments not retained after impact");
 await page.waitForFunction(()=>{
  const d=window.WP3DDiagnostics().combatSurface;
  return d.renderedTriangles>25&&d.explosions>=1;
 },null,{timeout:30000});
 const final=await page.evaluate(()=>window.WP3DDiagnostics());
 assert(final.playing&&final.defenders[0].hp>0,
  "Visual effects accidentally changed shooting/AI health");
 assert(final.combatSurface.drawCallsAdded===0&&final.combatSurface.collidersAdded===0,
  "Impact visuals must not create bullets/collision surfaces");
 const glError=await page.evaluate(()=>document.querySelector("#viewport").getContext("webgl2").getError());
 assert(glError===0,"GL errors after high-impact geometry "+glError);
 assert(!errors.length,"Browser exceptions: "+errors.join("; "));
 await fs.mkdir("artifacts/benchmarks",{recursive:true});
 await page.screenshot({path:"artifacts/benchmarks/grounded-ballistic-debris-high.png",
  timeout:90000,animations:"disabled"});
 console.log("PASS REAL CHROMIUM COLLISION-BOUND IMPACTS",JSON.stringify({
  blockedBy:bullet.shot.blockedBy,marks:final.combatSurface.marks,
  activeDebris:final.combatSurface.debris,blastScars:final.combatSurface.blasts,
  renderedTriangles:final.combatSurface.renderedTriangles,glError}));
}finally{await browser.close();}
