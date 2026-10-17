/* Physically registered but render-only battle damage for High WebGL2.
 * Infantry impacts use the actual ballistics hit endpoint; only recorded
 * masonry hits place wall-scars. Explosions leave terrain-sampled scorch
 * marks and deterministic short-lived ballistic stone/dirt fragments.
 * Strict bounded CPU triangles, no new draw calls or collision surfaces.
 */
(function(root){"use strict";
const MAX_MARKS=42,MAX_BLASTS=9,MAX_DEBRIS=42;
const marks=[],blasts=[],debris=[];
let registered=0,exploded=0,rejected=0,dropped=0,renderedTriangles=0;
const C={hole:[.105,.102,.092],char:[.156,.136,.113],
 edge:[.365,.339,.289],chip:[.54,.508,.436],sparks:[.77,.605,.35],
 darkSoil:[.195,.175,.146],dust:[.39,.353,.306]};
const finite=(v)=>Number.isFinite(v);
const signed=c=>[-Math.abs(c[0]),c[1],c[2]];
function checkPoint(x,y,z){
 return [x,y,z].every(finite)&&Math.abs(x)<48&&Math.abs(z)<48&&y>-2&&y<16;
}
function addBounded(arr,max,obj){
 if(arr.length>=max){arr.shift();dropped++;}
 arr.push(obj);
}
function nearestWall(x,z,y,buildings){
 let best=null;
 for(const [bx,bz,w,d] of buildings||[]){
  if(![bx,bz,w,d].every(finite)||w<2||d<2)continue;
  // Impact point must be inside or within a thin shell of the actual
  // collidable building's floorplan. Never paint empty air or invent cover.
  if(Math.abs(x-bx)>w*.5+.28||Math.abs(z-bz)>d*.5+.28)continue;
  const faces=[
   {axis:"x",normal:-1,face:bx-w*.5,dist:Math.abs(x-(bx-w*.5))},
   {axis:"x",normal:1,face:bx+w*.5,dist:Math.abs(x-(bx+w*.5))},
   {axis:"z",normal:-1,face:bz-d*.5,dist:Math.abs(z-(bz-d*.5))},
   {axis:"z",normal:1,face:bz+d*.5,dist:Math.abs(z-(bz+d*.5))}
  ];
  for(const face of faces){
   if(face.axis==="x"&&Math.abs(z-bz)>d*.5-.06)continue;
   if(face.axis==="z"&&Math.abs(x-bx)>w*.5-.06)continue;
   if(!best||face.dist<best.dist)best={...face,x,z,y};
  }
 }
 return best&&best.dist<=.36?best:null;
}
function bullet(shot,buildings,heightAt=()=>0){
 const e=shot?.endpoint;
 if(!e||shot.hit||!shot.blockedBy||!checkPoint(e.x,e.y,e.z)){
  rejected++;return false;
 }
 const kind=shot.blockedBy;
 let mark;
 if(kind==="masonry"){
  const wall=nearestWall(e.x,e.z,e.y,buildings);
  if(!wall){rejected++;return false;}
  const y=e.y;
  if(y<.18||y>5.8){rejected++;return false;}
  mark={kind:"wall",x:e.x,y,z:e.z,axis:wall.axis,
   normal:wall.normal,face:wall.face};
 }else if(kind==="terrain"){
  const ground=heightAt(e.x,e.z);
  if(!finite(ground)||Math.abs(e.y-ground)>1.5){rejected++;return false;}
  mark={kind:"ground",x:e.x,z:e.z,y:ground};
 }else{rejected++;return false;}
 const seed=registered*17+Math.abs(Math.round(e.x*17+e.z*23));
 addBounded(marks,MAX_MARKS,{...mark,seed,age:0,life:52});
 registered++;return true;
}
function explosion(x,z,kind="frag",heightAt=()=>0){
 const y=heightAt(x,z);
 if(!checkPoint(x,y,z)){rejected++;return false;}
 const artillery=kind==="artillery"||kind==="he";
 const radius=artillery?2.6:1.22;
 const seed=(Math.imul((Math.round(x*100)+889)|0,1664525)+
  Math.imul((Math.round(z*100)+712)|0,1013904223))>>>0;
 addBounded(blasts,MAX_BLASTS,{x,z,y,kind,life:36,age:0,radius,seed});
 const count=artillery?11:6;
 for(let i=0;i<count;i++){
  const angle=(i+((seed%31)/31))*Math.PI*2/count;
  const speed=(artillery?2.8:1.8)+((seed>>>((i%4)*7))%11)*.13;
  addBounded(debris,MAX_DEBRIS,{
   x,z,y:y+.09,
   vx:Math.cos(angle)*speed,vz:Math.sin(angle)*speed,
   vy:(artillery?3.1:2.1)+.13*(i%5),
   phase:i*.53,life:1.7+((i+seed)%7)*.12,age:0,
   size:.045+.012*(i%4),kind:i%5===0?"spark":"stone"
  });
 }
 exploded++;return true;
}
function update(dt){
 if(!finite(dt)||dt<=0)return;
 const step=Math.min(dt,.2);
 for(const group of [marks,blasts,debris]){
  for(let i=group.length-1;i>=0;i--){
   group[i].age+=step;
   if(group[i].age>=group[i].life)group.splice(i,1);
  }
 }
}
function wallPoint(mark,u,v,offset){
 if(mark.axis==="z")return [mark.x+u,mark.y+v,mark.face+mark.normal*offset];
 return [mark.face+mark.normal*offset,mark.y+v,mark.z+u];
}
function wallScar(g,m){
 const spread=.095+(m.seed%5)*.013,center=wallPoint(m,0,0,.065);
 let n=0;
 for(let j=0;j<6;j++){
  const a=j*Math.PI/3,b=(j+1)*Math.PI/3;
  const point=t=>wallPoint(m,Math.cos(t)*spread,Math.sin(t)*spread,.062);
  const inner=t=>wallPoint(m,Math.cos(t)*spread*.39,Math.sin(t)*spread*.39,.069);
  g.tri(center,inner(a),inner(b),signed(C.hole));n++;
  g.quad(inner(a),point(a),point(b),inner(b),
   signed(j%2?C.char:C.edge));n+=2;
 }
 // Two pale broken chips adjacent to the darker, recessed-looking mark.
 g.tri(wallPoint(m,-spread*.9,-spread*.30,.074),
  wallPoint(m,-spread*.55,-spread*.76,.074),
  wallPoint(m,-spread*.25,-spread*.50,.074),signed(C.chip));n++;
 return n;
}
function groundScar(g,x,z,r,heightAt,color){
 const center=[x,heightAt(x,z)+.090,z];
 let n=0;
 for(let j=0;j<8;j++){
  const a=j*Math.PI/4,b=(j+1)*Math.PI/4;
  const edge=t=>{const px=x+Math.cos(t)*r,pz=z+Math.sin(t)*r;
   return [px,heightAt(px,pz)+.091,pz];};
  g.tri(center,edge(b),edge(a),signed(j%3?color:C.edge));n++;
 }
 return n;
}
function render(g,tier,eyeX,eyeZ,heightAt=()=>0){
 renderedTriangles=0;
 if(tier!=="high"||!g||typeof g.tri!=="function"||typeof g.quad!=="function")return 0;
 const near=(x,z)=>Math.hypot(eyeX-x,eyeZ-z)<38;
 for(const m of marks){
  if(!near(m.x,m.z))continue;
  if(m.kind==="wall")renderedTriangles+=wallScar(g,m);
  else renderedTriangles+=groundScar(g,m.x,m.z,.16,heightAt,C.hole);
 }
 for(const b of blasts)if(near(b.x,b.z))
  renderedTriangles+=groundScar(g,b.x,b.z,b.radius,heightAt,C.darkSoil);
 for(const d of debris){
  if(!near(d.x,d.z)||d.age>d.life)continue;
  const t=d.age,px=d.x+d.vx*t,pz=d.z+d.vz*t;
  const py=d.y+d.vy*t-4.905*t*t;
  const base=heightAt(px,pz)+.06;
  if(py<base||!checkPoint(px,py,pz))continue;
  const width=d.size*(1-t/d.life*.45);
  const a=[px-width,py,pz],b=[px+width,py+.018,pz],
   c=[px,py+width*1.8,pz+width*.4];
  g.tri(a,b,c,signed(d.kind==="spark"?C.sparks:C.chip));
  renderedTriangles++;
 }
 return renderedTriangles;
}
function reset(){
 marks.length=0;blasts.length=0;debris.length=0;
 registered=0;exploded=0;rejected=0;dropped=0;renderedTriangles=0;
}
function stats(){return{marks:marks.length,blasts:blasts.length,debris:debris.length,
 bulletMarks:registered,explosions:exploded,rejected,dropped,renderedTriangles,
 maxMarks:MAX_MARKS,maxBlasts:MAX_BLASTS,maxDebris:MAX_DEBRIS,
 maxTriangles:MAX_MARKS*20+(MAX_BLASTS*8)+MAX_DEBRIS,drawCallsAdded:0,
 collidersAdded:0};}
root.WPCombatSurfaceEffects=Object.freeze({
 bullet,explosion,update,render,reset,stats,nearestWall});
})(window);
