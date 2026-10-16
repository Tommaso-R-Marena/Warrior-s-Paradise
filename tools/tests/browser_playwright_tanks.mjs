/* The shipped WebGL2 game, real inputs and active combat/physics in Chromium. */
import {chromium} from "playwright";
const browser=await chromium.launch({headless:true,args:[
 "--enable-webgl","--use-gl=angle","--use-angle=swiftshader"]});
const url=(process.env.WP_TEST_URL||"http://127.0.0.1:8765/3d/")+"?qa=1";
const assert=(v,msg)=>{if(!v)throw Error(msg);};
try{
 const page=await browser.newPage({viewport:{width:1024,height:680}});
 const errors=[];page.on("pageerror",e=>errors.push(String(e)));
 await page.goto(url,{waitUntil:"domcontentloaded",timeout:30000});
 await page.locator("#quality").selectOption("low");
 await page.locator("#deployment").selectOption("sherman");
 await page.locator("#deploy").click();
 await page.waitForFunction(()=>window.WP3DDiagnostics?.().playing&&!!window.WP3DQA,null,{timeout:15000});
 const initial=await page.evaluate(()=>window.WP3DDiagnostics());
 assert(initial.armor.driving==="armor","SHERMAN deployment did not put the player in the tank");
 assert(initial.armor.vehicles.find(v=>v.id==="armor").ammo.ap===16,"Tank ammunition not initialized");
 const x=initial.position.x,z=initial.position.z;
 await page.keyboard.down("w");
 await page.evaluate(()=>window.WP3DQA.step(210));
 await page.keyboard.up("w");
 const afterMove=await page.evaluate(()=>window.WP3DDiagnostics());
 const traveled=Math.hypot(afterMove.position.x-x,afterMove.position.z-z);
 assert(traveled>1.0,"Actual deployed Sherman cannot drive through its starting area: "+JSON.stringify({
  from:[x,z],to:afterMove.position,armor:afterMove.armor}));
 const crew=afterMove.armor.vehicles.find(v=>v.id==="armor");
 assert(crew.speed>=0,"Tank not driving forward");
 // Toggle shell type using the actual keyboard binding, not a module shortcut.
 await page.keyboard.press("r");
 const he=await page.evaluate(()=>window.WP3DDiagnostics());
 assert(he.armor.vehicles.find(v=>v.id==="armor").shellType==="he",
  "R did not select HE shells from the tank");
 await page.keyboard.press("r");
 const ap=await page.evaluate(()=>window.WP3DDiagnostics());
 assert(ap.armor.vehicles.find(v=>v.id==="armor").shellType==="ap",
  "R did not restore AP shells");
 // Fire through the game's real click/fire path, with the physics world present.
 await page.mouse.move(512,340);await page.mouse.down();
 await page.evaluate(()=>window.WP3DQA.step(2));
 await page.mouse.up();
 const fired=await page.evaluate(()=>window.WP3DDiagnostics());
 const gun=fired.armor.vehicles.find(v=>v.id==="armor");
 assert(fired.armor.shots>=1&&gun.ammo.ap===15,
  "Visible player cannon did not fire or expend one AP shell: "+JSON.stringify(fired.armor));
 assert(gun.reload>0&&gun.lastImpact&&gun.lastImpact.type,
  "Cannon lacks live reload and physical world impact");
 // The shell streak lasts <0.5 s of simulation time; Chromium's asynchronous
 // driver scheduling may retire it before this snapshot. Its immediate
 // existence is asserted synchronously in browser_tank_combat_smoke.js.
 await page.evaluate(()=>window.WP3DQA.step(230));
 const reloaded=await page.evaluate(()=>window.WP3DDiagnostics());
 assert(reloaded.armor.vehicles.find(v=>v.id==="armor").reload===0,
  "Cannon did not complete its reload");
 // Dismount with E and reenter when the site is safe.
 await page.keyboard.press("e");
 const exited=await page.evaluate(()=>window.WP3DDiagnostics());
 assert(!exited.armor.driving,"E did not exit the Sherman");
 await page.keyboard.press("e");
 const reentered=await page.evaluate(()=>window.WP3DDiagnostics());
 assert(reentered.armor.driving==="armor","E did not reenter the parked Sherman");
 assert(!errors.length,"Uncaught browser errors: "+errors.join("; "));
 console.log("PASS REAL TANK BROWSER",JSON.stringify({
  distanceDriven:Number(traveled.toFixed(2)),shellsFired:fired.armor.shots,
  impact:gun.lastImpact,ammo:gun.ammo,exitAndReentry:true}));
}finally{await browser.close();}
