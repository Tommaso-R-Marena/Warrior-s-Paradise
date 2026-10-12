/* Fixed-cost, artist-authored Normandy battlefield detail pass.
 * Adds silhouettes of hedgerows, cut telegraph poles, road rut lines,
 * shattered barricades and mortar-scarred masonry to the HIGH static buffer.
 * Does not change gameplay cover, line of sight, or Low/Medium vertex budgets.
 */
(function(root){"use strict";
const MAX=1400;
const OAK=[.31,.245,.18],BARK=[.27,.25,.18],STONE=[.41,.39,.34],
 DIRT=[.28,.25,.21],LEAF=[.23,.31,.19],WIRE=[.18,.21,.20];
function append(g,blocked,flags,buildings=[]){
 let props=0,triangles=0,posts=0,hedges=0,rubble=0;
 const colliders=[],height=(x,z)=>root.WPWW2Terrain?.surface(x,z,flags,buildings)||0;
 let state=198444;const rand=()=>{state^=state<<13;state^=state>>>17;state^=state<<5;return(state>>>0)/4294967296;};
 const clear=(x,z,r=1.4)=>x>-43&&x<43&&z>-43&&z<43&&!blocked(x,z,r)&&
  !flags.some(f=>Math.hypot(x-f.x,z-f.z)<6+r);
 function box(...v){if(triangles+12>MAX)return false;v[1]+=height(v[0],v[2]);g.box(...v);triangles+=12;return true;}
 function cone(...v){const facets=v[6]||5;if(triangles+facets>MAX)return false;v[1]+=height(v[0],v[2]);g.cone(...v);triangles+=facets;return true;}
 // Scarred bocage/hedgerows break up the open field silhouette.
 for(let i=0;i<48;i++){
  const edge=i%4,along=-40+Math.floor(i/4)*7.3+(rand()-.5)*1.5;
  const x=edge===0?-39:edge===1?39:along;
  const z=edge===2?-39:edge===3?39:along;
  if(!clear(x,z,.7))continue;
  const h=.65+rand()*.8;
  if(cone(x,.1,z,1.1+rand()*.6,h,LEAF,6)){hedges++;props++;colliders.push([x,z,1.15,1.15]);}
 }
 // WWII rural telephone/power poles with visible crossbeams.
 for(let i=0;i<15;i++){
  const x=-43+i*6.1,z=x-4.3;
  if(!clear(x,z,1.2))continue;
  if(!box(x,2.3,z,.18,4.6,.20,BARK))continue;
  box(x,4.23,z,1.36,.11,.14,OAK);
  for(const dx of [-.54,0,.54])box(x+dx,4.30,z,.09,.15,.10,WIRE);
  posts++;props++;colliders.push([x,z,.3,.3]);
 }
 // Scattered masonry, shattered wooden farm equipment and shell impacts.
 for(let i=0;i<32;i++){
  const x=-37+rand()*74,z=-37+rand()*74;
  if(!clear(x,z,1.1))continue;
  const r=.45+rand()*.5;
  if(i%3===0){
   for(let k=0;k<5;k++){
    const a=k*Math.PI*2/5,b=(k+1)*Math.PI*2/5;
    if(triangles+1>MAX)break;
    g.tri([x,-.025,z],[x+Math.cos(a)*r,.022,z+Math.sin(a)*r],
     [x+Math.cos(b)*r,.022,z+Math.sin(b)*r],DIRT);triangles++;
   }
  }else if(i%3===1){
   box(x,.13,z,1.4,.24,.18,OAK,rand()*2);
   box(x+.45,.34,z,.08,.45,.65,BARK,rand()*2);
  }else{
   for(let j=0;j<3;j++)box(x+(j-1)*.43,.2+j*.04,z+(rand()-.5)*.4,.44,.24,.36,STONE,rand()*3);
  }
  rubble++;props++;if(i%3!==0)colliders.push([x,z,i%3===1?1.4:1.7,.85]);
 }
 return {triangles,props,posts,hedges,rubble,colliders,maxTriangles:MAX};
}
root.WPWW2Environment=Object.freeze({append,maxTriangles:MAX});
})(window);
