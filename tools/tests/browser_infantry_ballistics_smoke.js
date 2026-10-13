"use strict";
const fs=require("node:fs"),path=require("node:path"),vm=require("node:vm"),assert=require("node:assert/strict");
const source=fs.readFileSync(path.resolve(__dirname,"../../browser/3d/infantry-ballistics.js"),"utf8");
const root={};vm.runInNewContext(source,{window:root,Math,Number,Error}, {timeout:10000});
const b=root.WPInfantryBallistics;
assert(b&&b.profiles.rifle&&b.profiles.smg&&b.profiles.lmg);
for(const name of ["rifle","smg","lmg"]){
 const w=b.profiles[name];
 assert(w.maxRange>w.fullDamage&&w.damage>0);
 assert.equal(b.damageAt(name,0),w.damage);
 assert.equal(b.damageAt(name,w.fullDamage),w.damage);
 assert.equal(b.damageAt(name,-1),0);
 assert.equal(b.damageAt(name,w.maxRange+.01),0,"No full-power infinite distance damage");
 for(let d=w.fullDamage+1;d<w.maxRange;d++)assert(b.damageAt(name,d)<=b.damageAt(name,d-1)+.0001);
 assert(b.damageAt(name,w.maxRange)<w.damage);
 assert(b.damageAt(name,5,"head")>b.damageAt(name,5,"torso"));
 assert(b.damageAt(name,5,"leg")<b.damageAt(name,5,"torso"));
}
const sourcePoint={x:0,y:1.67,z:0},enemy=z=>({x:0,z,hp:80,alert:0,facing:3.14});
const fire=(weapon,targets,extra={})=>b.trace({
 source:sourcePoint,weapon,yaw:0,pitch:0,targets,...extra
});
const surprised=enemy(12);
const body=fire("rifle",[surprised]);
assert.equal(body.hit,surprised,"Unaware enemy directly in front must take damage");
assert.equal(body.zone,"torso");
assert(body.damage>=60&&body.damage<=62);
assert.equal(surprised.alert,0,"Ballistic solver must not depend on or mutate AI alertness");
const head=fire("rifle",[surprised],{pitch:.041});
assert.equal(head.hit,surprised);
assert.equal(head.zone,"head","Vertical head aim should hit head");
assert(head.damage>body.damage);
const near=fire("rifle",[enemy(10)]).damage;
const mid=fire("rifle",[enemy(36)]).damage;
const far=fire("rifle",[enemy(62)]).damage;
assert(near>mid&&mid>far&&far>0,"Range damage is monotonically decreasing");
assert.equal(fire("rifle",[enemy(70)]).hit,null,"Too distant rifle hit should fail");
assert.equal(fire("smg",[enemy(32)]).hit,null,"Sten must not damage past cutoff");
assert(fire("lmg",[enemy(48)]).hit,"Bren should hit inside its effective game range");
const covered=fire("rifle",[enemy(12)],{obstacles:{
 buildings:[[0,6,3,3]],covers:[],structures:[],trunks:[]}});
assert.equal(covered.hit,null,"Masonry must absorb bullets");
assert.equal(covered.blockedBy,"masonry");
const sandbag=fire("rifle",[enemy(12)],{obstacles:{
 buildings:[],covers:[[0,6,3,2]],structures:[],trunks:[]}});
assert(sandbag.hit,"Chest-high bullet should pass above low sandbags");
const crouched=fire("rifle",[{...enemy(12),crouch:true}],{pitch:.041});
assert.equal(crouched.hit,null,"A crouched soldier should lower visible head/body");
const closest=enemy(9),behind=enemy(20);
assert.equal(fire("rifle",[behind,closest]).hit,closest,"Hit should stop at first soldier");
const missed=fire("rifle",[{...enemy(12),x:.92}]);
assert(!missed.hit&&missed.nearMisses.length===1,"Near misses suppress, not damage");
assert.equal(b.canSee({x:0,z:0,facing:0},{x:0,z:12,hp:80},()=>true),true);
assert.equal(b.canSee({x:0,z:0,facing:0},{x:0,z:-12,hp:80},()=>true),false,
 "Enemy cannot instantly acquire a quiet rear-sector target");
assert.equal(b.canSee({x:0,z:0,facing:0},{x:0,z:4,hp:80},()=>false),false,
 "NPC still needs actual line of sight");
// Boulders now stop projectiles at their actual top elevation. A standing
// soldier can fire over a waist-high rock but not through a tall boulder.
const highRock=fire("rifle",[enemy(12)],{obstacles:{
 rocks:[[0,6,2,2,1.85,0]],groundHeight:()=>0}});
assert.equal(highRock.hit,null);
assert.equal(highRock.blockedBy,"rock","Rock must be solid ballistic cover");
const lowRock=fire("rifle",[enemy(12)],{obstacles:{
 rocks:[[0,6,2,2,.6,0]],groundHeight:()=>0}});
assert(lowRock.hit,"A low stone should not become an invisible full-height wall");
const elevatedBoulder=fire("rifle",[enemy(12)],{obstacles:{
 rocks:[[0,6,2,2,1.4,.6]],groundHeight:()=>0}});
assert.equal(elevatedBoulder.blockedBy,"rock","A rock on a hill must retain its physical elevation");
const hill=fire("rifle",[enemy(12)],{obstacles:{
 groundHeight:(x,z)=>z>6&&z<9?2.5:0}});
assert.equal(hill.hit,null,"Elevated ground must intercept direct fire");
assert.equal(hill.blockedBy,"terrain");
console.log("PASS ballistic realism: surprise hits, range, tangible rock cover and topographic hill obstruction");
