/* Extra Kenney Nature Kit CC0 set-dressing for Normandy.
 * All assets are vendored and validated in CI; no external runtime fetch.
 * Builds one bounded static world buffer and returns collision AABBs.
 */
(function(root){"use strict";
const specs=[
 {name:"log-stacklarge",count:10,scale:[1.8,2.5],cover:true,nearObjective:true},
 {name:"fence-gate",count:5,scale:[2.9,3.5],cover:true,nearObjective:true},
 {name:"log-large",count:9,scale:[2.0,3.0],cover:true,nearObjective:true},
 {name:"rock-largeb",count:15,scale:[2.0,3.1],cover:true,nearObjective:false},
 {name:"stump-old",count:17,scale:[2.8,3.8],cover:false,nearObjective:false},
 {name:"grass-large",count:30,scale:[3.0,4.2],cover:false,nearObjective:false}
];
const LIMIT_TRIANGLES=4800,LIMIT_INSTANCES=86,MEDIUM_TRIANGLES=1800;
function footprint(model,x,z,scale,yaw){
 const cx=(model.min[0]+model.max[0])*.5*scale;
 const cz=(model.min[2]+model.max[2])*.5*scale;
 const width=(model.max[0]-model.min[0])*scale;
 const depth=(model.max[2]-model.min[2])*scale;
 const cos=Math.cos(yaw),sin=Math.sin(yaw);
 return [x+cx*cos+cz*sin,z-cx*sin+cz*cos,
  Math.abs(width*cos)+Math.abs(depth*sin),
  Math.abs(width*sin)+Math.abs(depth*cos)];
}
const overlap=(a,b,pad=.7)=>Math.abs(a[0]-b[0])<(a[2]+b[2])/2+pad &&
 Math.abs(a[1]-b[1])<(a[3]+b[3])/2+pad;
function plan(models,blocked,flags){
 let seed=98273511;
 const rand=()=>{seed^=seed<<13;seed^=seed>>>17;seed^=seed<<5;return (seed>>>0)/4294967296;};
 const placements=[],collision=[],counts={},max=LIMIT_TRIANGLES;
 let triangles=0,attempts=0;
 for(const spec of specs){
  const model=models[spec.name];
  if(!model||!Number.isFinite(model.count)||model.count<=0)throw Error("Bad asset "+spec.name);
  let count=0;
  for(let i=0;i<spec.count && placements.length<LIMIT_INSTANCES &&
   triangles+model.count<=max;i++){
   let placed=false;
   for(let t=0;t<60;t++){
    attempts++;
    const near=spec.nearObjective&&flags.length;
    const f=near?flags[Math.floor(rand()*flags.length)]:null;
    const theta=rand()*Math.PI*2;
    const radius=8.5+rand()*12;
    const x=near?f.x+Math.cos(theta)*radius:-43+rand()*86;
    const z=near?f.z+Math.sin(theta)*radius:-43+rand()*86;
    const scale=spec.scale[0]+rand()*(spec.scale[1]-spec.scale[0]);
    const yaw=rand()*Math.PI*2;
    const box=footprint(model,x,z,scale,yaw);
    if(box.some(v=>!Number.isFinite(v)))continue;
    const extent=Math.max(box[2],box[3])*.5;
    if(box[0]<-44+extent||box[0]>44-extent ||
     box[1]<-44+extent||box[1]>44-extent)continue;
    if(blocked(box[0],box[1],Math.max(.6,extent+.40)))continue;
    if(flags.some(flag=>Math.hypot(box[0]-flag.x,box[1]-flag.z)<7+extent))continue;
    if(collision.some(b=>overlap(box,b)) ||
     (spec.cover&&placements.some(item=>overlap(box,item.box,.45))))continue;
    const item={name:spec.name,instance:[x,0,z,scale,yaw],box};
    placements.push(item);
    if(spec.cover)collision.push(box);
    count++;triangles+=model.count;placed=true;break;
   }
   if(!placed)break;
  }
  counts[spec.name]=count;
 }
 return {placements,colliders:collision,counts,triangles,attempts};
}
async function load(kit,Geometry,base,blocked,flags){
 const models=await kit.loadModels(specs.map(s=>({name:s.name})));
 const batch=plan(models,blocked,flags);
 const geo=new Geometry();
 geo.data=base.slice();
 let actual=0,mediumTriangles=0,mediumDone=false;
 const mediumColliders=[];
 for(const item of batch.placements){
  const count=models[item.name].count;
  if(!mediumDone){
   if(mediumTriangles+count<=MEDIUM_TRIANGLES){
    mediumTriangles+=count;
    if(specs.find(s=>s.name===item.name)?.cover)mediumColliders.push(item.box);
   }
   else mediumDone=true;
  }
  actual+=kit.appendInstances(geo,models[item.name],[item.instance]);
 }
 if(actual!==batch.triangles)throw Error("Set dressing budget accounting mismatch");
 return {...batch,mediumTriangles,mediumColliders,data:geo.data,sourceModels:specs.length};
}
root.WPNormandyDressing=Object.freeze({load,plan,footprint,
 specs:specs.map(s=>({...s})),limits:Object.freeze({
 triangles:LIMIT_TRIANGLES,mediumTriangles:MEDIUM_TRIANGLES,instances:LIMIT_INSTANCES})});
})(window);
