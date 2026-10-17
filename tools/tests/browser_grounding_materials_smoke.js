"use strict";
/* Shadows follow rendered conformation; metal and wheels remain high-only.
 * CI will additionally compile their shaders in real WebGL2 Chromium.
 */
const assert=require("node:assert/strict");
const fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const dir=path.resolve(__dirname,"../../browser/3d"),root={};
for(const name of ["contact-shadows.js","ww2-vehicles.js"]){
 vm.runInNewContext(fs.readFileSync(path.join(dir,name),"utf8"),{window:root,Math,Set},{filename:name});
}
function surface(x,z){return .14*Math.sin(x*.19)+.12*Math.cos(z*.27);}
const terrainPoints=[],shadows={tri(a,b,c,color){
 for(const p of [a,b,c,color])assert(p.every(Number.isFinite),"Finite triangles only");
 assert(a.every((x,i)=>Number.isFinite(x)),"shadow finite");
 for(const p of [a,b,c])if(p.length===3)assert(Math.abs(p[1]-surface(p[0],p[2])-.04)<1e-7);
 terrainPoints.push([a,b,c,color]);
}};
const player={x:0,z:0},vehicle={x:1,z:2,yaw:.2,width:3,length:5.6,alive:false};
const soldiers=Array.from({length:15},(_,i)=>({x:(i%5)*2-5,z:5+Math.floor(i/5)*2,hp:100,facing:i*.23}));
const low=root.WPContactShadows.render(shadows,surface,player,"low",soldiers,[vehicle]);
assert.equal(low.triangles,0);
const medium=root.WPContactShadows.render(shadows,surface,player,"medium",soldiers,[vehicle]);
assert(medium.actors===1&&medium.triangles===16,"Medium casts armor shadow only");
const high=root.WPContactShadows.render(shadows,surface,player,"high",soldiers,[vehicle]);
assert(high.actors>1&&high.triangles<=210,"High shadows exceed geometry budget");
assert(high.triangles>medium.triangles);
const gray=(color)=>assert(color.every(Number.isFinite));
const fleet=root.WPArmoredFleet;
function geometry(){const m={triangles:0,tagged:0,boxes:0,wheels:0,
 box(x,y,z,w,h,d,color){gray(color);this.triangles+=12;this.boxes++;if(color[2]<0)this.tagged++;},
 tri(a,b,c,color){gray(color);this.triangles++;this.wheels++;if(color[2]<0)this.tagged++;},
 cone(x,y,z,r,h,color,faces=7){this.triangles+=faces;},
 quad(a,b,c,d,color){this.triangles+=2;}};return m;}
const mediumModel=geometry(),highModel=geometry();
const near={x:-39,z:-23,yaw:0};
fleet.draw(mediumModel,null,"medium",near,0);
fleet.draw(highModel,null,"high",near,0);
assert(highModel.wheels>=240,"High road wheel ring and hub geometry missing");
assert(highModel.triangles>mediumModel.triangles+360,"High wheels lack substantial 3D detail");
assert(mediumModel.tagged===0,"Medium/Low must retain original material palette");
assert(highModel.tagged>150,"Armor PBR metal tagging missing");
const game=fs.readFileSync(path.join(dir,"game.js"),"utf8");
const html=fs.readFileSync(path.join(dir,"index.html"),"utf8");
assert(html.includes('contact-shadows.js'));
for(const value of ["contactShadowTriangles","float metalTagged=(1.-step(0.,vColor.b))*(1.-bakedTagged)","float contactShadow=1.-step(.10,vColor.r)"])
 assert(game.includes(value),"missing renderer contract "+value);
console.log("PASS high-fidelity grounding",JSON.stringify({mediumShadows:medium.triangles,
 highShadows:high.triangles,modelExtraTriangles:highModel.triangles-mediumModel.triangles,
 highTagCount:highModel.tagged,highWheelTriangles:highModel.wheels}));
