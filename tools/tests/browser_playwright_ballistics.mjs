/* Browser-level combat integration: shoot actual enemy game objects, not stubs. */
import {chromium} from "playwright";
const base=process.env.WP_TEST_URL||"http://127.0.0.1:8765/3d/";
const browser=await chromium.launch({headless:true,args:["--enable-webgl","--use-gl=angle","--use-angle=swiftshader"]});
const assert=(ok,message)=>{if(!ok)throw Error(message)};
let page;
try{
 page=await browser.newPage({viewport:{width:1000,height:670}});
 const errors=[];page.on("pageerror",err=>errors.push(String(err)));
 await page.goto(base+(base.includes("?")?"&":"?")+"qa=1",{waitUntil:"domcontentloaded",timeout:30000});
 await page.locator("#quality").selectOption("low");
 await page.locator("#deploy").click();
 await page.waitForFunction(()=>!!window.WP3DQA&&!!window.WP3DDiagnostics&&window.WP3DDiagnostics().playing,null,{timeout:15000});
 const doShot=async({weapon="rifle",range=12,pitch=0,obstacle=false})=>page.evaluate(async d=>{
   const x=d.obstacle?-24:0,z=d.obstacle?-25:0;
   const enemyZ=d.obstacle?-4:d.range,enemyX=x;
   // Aimed headshot follows physical elevation, rather than using a
   // misleading constant aiming pitch from the former flat world.
   const pitch=d.pitch==="head"?Math.atan2(
    (window.WP3DQA.terrainProbe(enemyX,enemyZ).y+2.31)-
    (window.WP3DQA.terrainProbe(x,z).y+1.67),enemyZ-z):d.pitch;
   window.WP3DQA.combatScenario({x,z,weapon:d.weapon,pitch,
    aim:true,yaw:0,clearDecoration:true,
    enemies:[{x:enemyX,z:enemyZ,hp:80,alert:0,facing:3.14159}]});
   const before=window.WP3DDiagnostics().defenders[0];
   const shot=window.WP3DQA.shootOnce();
   const after=window.WP3DDiagnostics();
   return {shot,before,after:after.defenders[0],stealthHits:after.stealthHits,
    headshots:after.headshots};
 },{weapon,range,pitch,obstacle});
 const surprise=await doShot({range:12});
 assert(surprise.shot?.hit,"A fully unaware enemy at 12 meters did not take damage: "+JSON.stringify(surprise));
 assert(surprise.before.alert===0&&surprise.after.hp<surprise.before.hp,
  "No surprise damage or impossible awareness-based hit gate");
 assert(surprise.stealthHits>0,"Surprise hit must be recorded");
 const near=await doShot({range:9});
 const far=await doShot({range:36});
 assert(near.shot.hit&&far.shot.hit,"Rifle must hit unobstructed enemies at both ranges");
 assert(near.shot.damage>far.shot.damage,
  "A longer rifle shot must do strictly less damage than the capped close-range maximum");
 const head=await doShot({range:12,pitch:"head"});
 assert(head.shot.hit&&head.shot.zone==="head"&&head.shot.damage>near.shot.damage,
  "A real elevated aim must produce a lethal headshot, not a generic body hit: "+JSON.stringify(head));
 const tooFar=await doShot({range:70});
 assert(!tooFar.shot.hit&&tooFar.after.hp===tooFar.before.hp,
  "Rifle must do exactly zero health damage beyond gameplay max range");
 const sten=await doShot({weapon:"smg",range:35});
 assert(!sten.shot.hit&&sten.after.hp===80,"Sten must be range limited");
 const covered=await doShot({obstacle:true});
 assert(!covered.shot.hit&&covered.after.hp===80&&covered.shot.blockedBy==="masonry",
  "A farm house between shooter and opponent must block the bullet");
 // Exercise the actual game loop: downed infantry persist, weapon kick
 // visibly increases accuracy bloom, then recovers without permanent drift.
 const firing=await page.evaluate(()=>{
  window.WP3DQA.combatScenario({x:0,z:0,yaw:0,pitch:0,aim:true,weapon:"rifle",
   clearDecoration:true,enemies:[{x:0,z:12,hp:55,alert:0,facing:Math.PI}]});
  const shot=window.WP3DQA.shootOnce();
  const immediate=window.WP3DDiagnostics();
  window.WP3DQA.step(1);
  const fallen=window.WP3DDiagnostics();
  window.WP3DQA.step(120);
  const recovered=window.WP3DDiagnostics();
  return {shot,immediateBloom:immediate.recoilBloom,
   immediateReturn:immediate.recoilReturn,fallenBodies:fallen.corpses,
   renderBodies:fallen.corpsesDrawn,finishedBloom:recovered.recoilBloom,
   finishedReturn:recovered.recoilReturn};
 });
 assert(firing.shot.hit&&firing.fallenBodies>=1,
  "Shot casualties should remain in the actual rendered battlefield");
 assert(firing.immediateBloom>0&&firing.immediateReturn>0,
  "Weapon recoil and inaccuracy bloom should be measurable after the shot");
 assert(firing.finishedBloom<firing.immediateBloom*.05&&
   firing.finishedReturn<firing.immediateReturn*.05,
  "Weapon kick and bloom must recover after not firing");
 const terrainCheck=await page.evaluate(async()=>{
   // Actual deployed physics, not a static stand-in: all procedural rocks,
   // imported Kenney boulders and base terrain share contact-space coordinates.
   await new Promise(resolve=>{
    if(window.WP3DDiagnostics().assetsLoaded){resolve();return;}
    const t=setInterval(()=>{if(window.WP3DDiagnostics().assetsLoaded){clearInterval(t);resolve();}},30);
    setTimeout(()=>{clearInterval(t);resolve();},4500);
   });
   const qa=window.WP3DQA,d=window.WP3DDiagnostics(),all=qa.collisionExamples();
   const core=all.core[0],boulder=all.rocks[0];
   function blocked(box){return !!box&&qa.terrainProbe(box[0],box[1],.20).blocked;}
   const from=core?[core[0]-core[2]/2-1.2,core[1]]:null;
   const walked=from?qa.traverse(from[0],from[1],core[2]/2+2,0):null;
   return {actualGround:d.physicalTerrainTriangles,elevation:d.terrainAtPlayer,
    coreCount:all.core.length,boulders:all.rocks.length,
    proceduralBlocked:blocked(core),importedBlocked:blocked(boulder),
    sweptFrom:from,sweptTo:walked,rockCenter:core,
    graphics:d.collisionQuality,activeDressing:d.activeDressingColliders};
 });
 assert(terrainCheck.actualGround===722,"Game does not use authoritative physical mesh");
 assert(terrainCheck.coreCount>=15&&terrainCheck.proceduralBlocked,
  "Baseline rocks and battlefield obstacles must be solid");
 assert(terrainCheck.boulders>0&&terrainCheck.importedBlocked,
  "At least one imported Kenney boulder must use the actual GLB collision bounds");
 assert(terrainCheck.activeDressing===0&&terrainCheck.graphics==="low",
  "Low graphics cannot have invisible High-detail obstacle collisions");
 assert(!errors.length,"Chromium console errors: "+JSON.stringify(errors));
 console.log("PASS ACTUAL CHROMIUM FIRE",JSON.stringify({
  surpriseDamage:surprise.shot.damage,headshot:head.shot.damage,
  nearDamage:near.shot.damage,farDamage:far.shot.damage,
  longRange:tooFar.shot.reason,stenOutOfRange:sten.shot.reason,cover:covered.shot.blockedBy}));
}finally{await browser.close();}
