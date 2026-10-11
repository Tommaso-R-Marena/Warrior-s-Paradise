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
   window.WP3DQA.combatScenario({x,z,weapon:d.weapon,pitch:d.pitch,
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
 const head=await doShot({range:12,pitch:.041});
 assert(head.shot.hit&&head.shot.zone==="head"&&head.shot.damage>near.shot.damage,
  "A real elevated aim must produce a lethal headshot, not a generic body hit");
 const tooFar=await doShot({range:70});
 assert(!tooFar.shot.hit&&tooFar.after.hp===tooFar.before.hp,
  "Rifle must do exactly zero health damage beyond gameplay max range");
 const sten=await doShot({weapon:"smg",range:35});
 assert(!sten.shot.hit&&sten.after.hp===80,"Sten must be range limited");
 const covered=await doShot({obstacle:true});
 assert(!covered.shot.hit&&covered.after.hp===80&&covered.shot.blockedBy==="masonry",
  "A farm house between shooter and opponent must block the bullet");
 assert(!errors.length,"Chromium console errors: "+JSON.stringify(errors));
 console.log("PASS ACTUAL CHROMIUM FIRE",JSON.stringify({
  surpriseDamage:surprise.shot.damage,headshot:head.shot.damage,
  nearDamage:near.shot.damage,farDamage:far.shot.damage,
  longRange:tooFar.shot.reason,stenOutOfRange:sten.shot.reason,cover:covered.shot.blockedBy}));
}finally{await browser.close();}
