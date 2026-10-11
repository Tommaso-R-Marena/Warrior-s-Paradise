/* Obstacle-aware infantry routing for the free WebGL2 battlefield.
 * Planning runs only after a soldier gets stuck; no per-frame A* work.
 * Does not use live opponent coordinates or break collision rules.
 */
(()=>{
"use strict";
const STEP=1.5, HALF=45, N=61, MAX_VISITS=1050;
const directions=[
 [1,0,1],[-1,0,1],[0,1,1],[0,-1,1],
 [1,1,Math.SQRT2],[-1,1,Math.SQRT2],[1,-1,Math.SQRT2],[-1,-1,Math.SQRT2]
];
const counters={plans:0,successful:0,failed:0,expanded:0};
const clamp=n=>Math.max(0,Math.min(N-1,n));
const grid=n=>clamp(Math.round((n+HALF)/STEP));
const coord=n=>n*STEP-HALF;
const key=(x,z)=>z*N+x;
const dist=(ax,az,bx,bz)=>Math.hypot(ax-bx,az-bz);
const estimate=(x,z,gx,gz)=>{
 const dx=Math.abs(x-gx),dz=Math.abs(z-gz);
 return dx+dz+(Math.SQRT2-2)*Math.min(dx,dz);
};
function plan(sx,sz,gx,gz,isBlocked){
 counters.plans++;
 if(![sx,sz,gx,gz].every(Number.isFinite)){counters.failed++;return [];}
 const walk=new Int8Array(N*N);
 const can=(x,z)=>{
  if(x<0||x>=N||z<0||z>=N)return false;
  const k=key(x,z);
  if(!walk[k])walk[k]=isBlocked(coord(x),coord(z),.49)?-1:1;
  return walk[k]===1;
 };
 let ix=grid(sx),iz=grid(sz),tx=grid(gx),tz=grid(gz);
 // A goal may lie near a wall: select the closest reachable cell candidate.
 if(!can(tx,tz)){
  let best=Infinity,nearest=null;
  for(let radius=1;radius<=4;radius++){
   for(let dx=-radius;dx<=radius;dx++)for(let dz=-radius;dz<=radius;dz++){
    if(Math.max(Math.abs(dx),Math.abs(dz))!==radius)continue;
    const x=tx+dx,z=tz+dz;
    if(can(x,z)){
     const score=dist(coord(x),coord(z),gx,gz);
     if(score<best){best=score;nearest=[x,z];}
    }
   }
   if(nearest)break;
  }
  if(!nearest){counters.failed++;return [];}
  [tx,tz]=nearest;
 }
 const first=key(ix,iz),last=key(tx,tz);
 const cost=new Float64Array(N*N),parent=new Int32Array(N*N);
 cost.fill(Infinity);parent.fill(-1);cost[first]=0;
 const closed=new Uint8Array(N*N),heap=[];
 const push=(id,score)=>{
  const node={id,score};let j=heap.length;heap.push(node);
  while(j>0){const p=(j-1)>>1;if(heap[p].score<=score)break;
   heap[j]=heap[p];j=p;}heap[j]=node;
 };
 const pop=()=>{
  const result=heap[0],end=heap.pop();
  if(heap.length){
   let j=0;while(true){let c=j*2+1;if(c>=heap.length)break;
    if(c+1<heap.length&&heap[c+1].score<heap[c].score)c++;
    if(heap[c].score>=end.score)break;heap[j]=heap[c];j=c;}
   heap[j]=end;
  }
  return result;
 };
 push(first,estimate(ix,iz,tx,tz));let visited=0;
 while(heap.length&&visited<MAX_VISITS){
  const id=pop().id;if(closed[id])continue;
  closed[id]=1;visited++;
  if(id===last){
   const route=[];let p=id;
   while(p!==first&&p>=0&&route.length<N*N){
    route.push({x:coord(p%N),z:coord(Math.floor(p/N))});p=parent[p];
   }
   route.reverse();
   // The actor occupies its real position, not necessarily its grid center.
   counters.successful++;counters.expanded+=visited;return route;
  }
  const x=id%N,z=Math.floor(id/N);
  for(const [dx,dz,weight] of directions){
   const nx=x+dx,nz=z+dz;if(!can(nx,nz))continue;
   // Diagonal wall-clipping is never permitted.
   if(dx&&dz&&(!can(x+dx,z)||!can(x,z+dz)))continue;
   const next=key(nx,nz);if(closed[next])continue;
   const value=cost[id]+weight;
   if(value>=cost[next])continue;
   cost[next]=value;parent[next]=id;
   push(next,value+estimate(nx,nz,tx,tz));
  }
 }
 counters.failed++;counters.expanded+=visited;return [];
}
function advance(actor,gx,gz,speed,dt,isBlocked,tryMove){
 if(!(dt>0&&speed>0))return;
 const state=actor._route||(actor._route={points:[],index:0,goalX:gx,goalZ:gz,stuck:0,retry:0,age:0});
 state.age+=dt;state.retry=Math.max(0,state.retry-dt);
 if(dist(gx,gz,state.goalX,state.goalZ)>3.5||state.age>8){
  state.points=[];state.index=0;state.stuck=0;state.age=0;
 }
 state.goalX=gx;state.goalZ=gz;
 let px=gx,pz=gz,following=state.index<state.points.length;
 if(following){
  let point=state.points[state.index];
  while(point&&dist(actor.x,actor.z,point.x,point.z)<.95){
   state.index++;point=state.points[state.index];
  }
  if(point){px=point.x;pz=point.z;}
  else {state.points=[];state.index=0;following=false;}
 }
 const d=dist(actor.x,actor.z,px,pz);
 if(d<.06)return;
 const oldX=actor.x,oldZ=actor.z;
 const scale=Math.min(speed*dt,d)/d;
 [actor.x,actor.z]=tryMove(actor.x,actor.z,(px-actor.x)*scale,(pz-actor.z)*scale);
 const gain=d-dist(actor.x,actor.z,px,pz);
 if(gain<Math.min(speed*dt,d)*.22)state.stuck+=dt;
 else state.stuck=Math.max(0,state.stuck-dt*.8);
 if(state.stuck>.48&&state.retry<=0){
  // Never perform unbounded replanning on every simulation tick.
  const route=plan(actor.x,actor.z,gx,gz,isBlocked);
  state.points=route;state.index=0;state.age=0;state.stuck=0;state.retry=route.length?.8:2.5;
 }
 // Keep the original movement and collision fallback when no path exists.
}
const reset=()=>{for(const k of Object.keys(counters))counters[k]=0;};
window.WPTacticalNav=Object.freeze({advance,plan,reset,stats:()=>({...counters})});
})();