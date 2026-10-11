/* Warrior's Paradise — dependency-free WebGL2 3D Frontline.
 * Native browser-rendered geometry; no external JS, GPU server, login or API.
 * Terrain/props are intentionally simple placeholders, not Unreal parity.
 */
(()=>{
"use strict";
const $=id=>document.getElementById(id), canvas=$("viewport");
const gl=canvas.getContext("webgl2",{antialias:false,alpha:true,powerPreference:"high-performance"});
const fallback=$("fallback"), menu=$("menu");
if(!gl){
  fallback.hidden=false;$("deploy").disabled=true;
  $("menu-copy").textContent="3D rendering is unavailable on this device. Use Field Edition instead.";
  return;
}
const VS=[
 "#version 300 es","precision highp float;",
 "layout(location=0) in vec3 aPos;layout(location=1) in vec3 aNormal;",
 "layout(location=2) in vec3 aColor;",
 "uniform mat4 uVP;out vec3 vColor;out vec3 vPos;out vec3 vNorm;",
 "void main(){vColor=aColor;vPos=aPos;vNorm=aNormal;gl_Position=uVP*vec4(aPos,1.);}"
].join("\n");
const FS=[
 "#version 300 es","precision highp float;",
 "in vec3 vColor;in vec3 vPos;in vec3 vNorm;",
 "uniform vec3 uEye;uniform float uDetailLevel;uniform float uUseAtlas;",
 "uniform sampler2D uDiffuse;uniform sampler2D uBump;out vec4 outColor;",
 "float hash21(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}",
 "float valnoise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);",
 "return mix(mix(hash21(i),hash21(i+vec2(1.,0.)),f.x),",
 "mix(hash21(i+vec2(0.,1.)),hash21(i+vec2(1.,1.)),f.x),f.y);}",
 "void main(){",
 "vec3 n=normalize(vNorm);",
 "float sun=max(dot(n,normalize(vec3(.53,.88,-.36))),0.);",
 "float exposure=.38+.69*sun;",
 "float distanceToEye=distance(vPos,uEye);",
 "float nearby=1.-smoothstep(25.,85.,distanceToEye);",
 "vec2 surface=abs(n.y)>.61?vPos.xz:(abs(n.x)>abs(n.z)?vPos.zy:vPos.xy);",
 "float large=.5,fine=.5;",
 "if(uDetailLevel>.02&&uUseAtlas<.5){large=valnoise(surface*1.3);fine=valnoise(surface*13.);}",
 "float mottled=(large-.5)*.23+(fine-.5)*.13;",
 "float grass=step(vColor.r+.035,vColor.g)*step(vColor.b,vColor.g+.08);",
 "float earth=1.-grass;",
 "vec3 rgb=vColor*exposure;",
 "rgb*=1.+mottled*nearby*uDetailLevel;",
 "float grassFibers=.5;",
 "if(uDetailLevel>.02&&uUseAtlas<.5)grassFibers=hash21(floor(surface*19.));",
 "rgb*=1.+grass*nearby*(grassFibers-.5)*.16*uDetailLevel;",
 "float vertical=1.-step(.65,abs(n.y));",
 "float bricky=vertical*step(.075,vColor.r-vColor.g)*(1.-grass);",
 "vec2 brickCoord=vec2(surface.x+floor(surface.y*1.4)*.31,surface.y)*vec2(2.7,3.3);",
 "vec2 joint=fract(brickCoord);",
 "float mortar=1.-step(.075,min(min(joint.x,1.-joint.x),min(joint.y,1.-joint.y)));",
 "rgb*=1.-.23*bricky*mortar*nearby;",
 "float rough=.5;",
 "if(uDetailLevel>.02&&uUseAtlas<.5)rough=earth*vertical*hash21(floor(surface*23.));",
 "rgb*=1.+.07*(rough-.5)*nearby*uDetailLevel;",
 "if(uUseAtlas>.5&&uDetailLevel>.02){",
 "float terrain=step(.65,n.y)*(1.-smoothstep(.25,.9,vPos.y));",
 "float masonry=vertical*step(.10,vColor.r-vColor.g)*step(.9,vPos.y);",
 "float tile=terrain>.5?(grass>.5?0.:1.):(masonry>.5?3.:2.);",
 "float intensity=max(terrain,masonry)*uDetailLevel;",
 "vec2 uv=(fract(surface*.26)*.968+.016+vec2(tile,0.))/vec2(4.,1.);",
 "vec4 pbr=texture(uDiffuse,uv);vec3 bump=texture(uBump,uv).rgb;",
 "float roughness=clamp(pbr.a,.3,1.);",
 "float bumpSun=max(0.,dot(normalize(vec3((bump.r-.5)*1.25,.96,(bump.g-.5)*1.25)),normalize(vec3(.53,.88,-.36))));",
 "vec3 textured=pbr.rgb*(.39+.7*bumpSun);",
 "rgb=mix(rgb,textured,intensity*.78);",
 "vec3 eyeDir=normalize(uEye-vPos);",
 "vec3 halfDir=normalize(eyeDir+normalize(vec3(.53,.88,-.36)));",
 "float highlight=pow(max(0.,dot(n,halfDir)),mix(92.,8.,roughness));",
 "rgb+=vec3(.07)*highlight*(1.-roughness)*intensity;",
 "}",
 "float fog=1.-exp(-pow(max(0.,distanceToEye-18.)/78.,1.45));",
 "rgb=mix(rgb,vec3(.58,.66,.64),min(.81,fog));",
 "rgb=(rgb*(2.51*rgb+.03))/(rgb*(2.43*rgb+.59)+.14);",
 "outColor=vec4(pow(max(rgb,0.),vec3(.93)),1.);",
 "}"
].join("\n");
function shader(type,source){
 const o=gl.createShader(type);gl.shaderSource(o,source);gl.compileShader(o);
 if(!gl.getShaderParameter(o,gl.COMPILE_STATUS))throw Error(gl.getShaderInfoLog(o));
 return o;
}
let program;
try{
 program=gl.createProgram();gl.attachShader(program,shader(gl.VERTEX_SHADER,VS));
 gl.attachShader(program,shader(gl.FRAGMENT_SHADER,FS));gl.linkProgram(program);
 if(!gl.getProgramParameter(program,gl.LINK_STATUS))throw Error(gl.getProgramInfoLog(program));
}catch(e){
 fallback.hidden=false;fallback.textContent="3D shader setup failed. Try the lightweight Field Edition. "+String(e);
 return;
}
gl.useProgram(program);
const vpLocation=gl.getUniformLocation(program,"uVP");
const eyeLocation=gl.getUniformLocation(program,"uEye");
const detailLocation=gl.getUniformLocation(program,"uDetailLevel");
const useAtlasLocation=gl.getUniformLocation(program,"uUseAtlas");
const diffuseLocation=gl.getUniformLocation(program,"uDiffuse");
const bumpLocation=gl.getUniformLocation(program,"uBump");
let materialTextures=null;
const atlasKit=window.WPMaterialAtlas;
try{if(atlasKit)materialTextures=atlasKit.upload(gl);}catch(error){
 materialTextures=null;
}
const photographicMaterials={loaded:false,error:""};
if(materialTextures&&atlasKit?.enhance){
 // High/balanced: attempt self-hosted CC0 photographic albedo materials.
 // Keep original procedurally generated textures when unavailable or on low.
 atlasKit.enhance(gl,materialTextures).then(ok=>photographicMaterials.loaded=!!ok)
  .catch(error=>{photographicMaterials.error=String(error);});
}
gl.enable(gl.DEPTH_TEST);
gl.clearColor(0,0,0,0);
function mesh(dynamic=false){
 const vao=gl.createVertexArray(),vbo=gl.createBuffer();
 gl.bindVertexArray(vao);gl.bindBuffer(gl.ARRAY_BUFFER,vbo);
 for(let n=0;n<3;n++){
  gl.enableVertexAttribArray(n);gl.vertexAttribPointer(n,3,gl.FLOAT,false,36,n*12);
 }
 gl.bindVertexArray(null);
 return {vao,vbo,count:0,dynamic};
}
function upload(target,data){
 gl.bindBuffer(gl.ARRAY_BUFFER,target.vbo);
 gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(data),
  target.dynamic?gl.DYNAMIC_DRAW:gl.STATIC_DRAW);
 target.count=data.length/9;
}
function draw(target){
 if(!target.count)return;
 gl.bindVertexArray(target.vao);
 gl.drawArrays(gl.TRIANGLES,0,target.count);
}
class Geometry{
 constructor(){this.data=[];}
 tri(a,b,c,color){
  const ab=a.map((v,i)=>b[i]-v),ac=a.map((v,i)=>c[i]-v);
  let n=[ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0]];
  const m=Math.hypot(...n)||1;n=n.map(v=>v/m);
  for(const v of [a,b,c])this.data.push(...v,...n,...color);
 }
 quad(a,b,c,d,col){this.tri(a,b,c,col);this.tri(a,c,d,col);}
 box(cx,cy,cz,w,h,d,col,yaw=0){
  const cs=Math.cos(yaw),sn=Math.sin(yaw);
  const local=(x,y,z)=>[cx+x*cs+z*sn,cy+y,cz-x*sn+z*cs];
  const v=[local(-w/2,-h/2,-d/2),local(w/2,-h/2,-d/2),
    local(w/2,-h/2,d/2),local(-w/2,-h/2,d/2),
    local(-w/2,h/2,-d/2),local(w/2,h/2,-d/2),
    local(w/2,h/2,d/2),local(-w/2,h/2,d/2)];
  for(const [a,b,c,d] of [[3,2,1,0],[4,5,6,7],[0,1,5,4],
   [1,2,6,5],[2,3,7,6],[3,0,4,7]])this.quad(v[a],v[b],v[c],v[d],col);
 }
 cone(cx,cy,cz,r,h,col,facets=7){
  for(let i=0;i<facets;i++){
   const a=i*Math.PI*2/facets,b=(i+1)*Math.PI*2/facets;
   this.tri([cx+Math.cos(a)*r,cy,cz+Math.sin(a)*r],
    [cx+Math.cos(b)*r,cy,cz+Math.sin(b)*r],[cx,cy+h,cz],col);
  }
 }
 roof(cx,y,cz,w,d,height,col){
  // Normandy gable and two pitched planes; avoids fake pyramid rooftops.
  this.quad([cx-w/2,y,cz-d/2],[cx-w/2,y,cz+d/2],
   [cx,y+height,cz+d/2],[cx,y+height,cz-d/2],col);
  this.quad([cx,y+height,cz-d/2],[cx,y+height,cz+d/2],
   [cx+w/2,y,cz+d/2],[cx+w/2,y,cz-d/2],col);
  this.tri([cx-w/2,y,cz-d/2],[cx+w/2,y,cz-d/2],
   [cx,y+height,cz-d/2],COLORS.plaster);
  this.tri([cx+w/2,y,cz+d/2],[cx-w/2,y,cz+d/2],
   [cx,y+height,cz+d/2],COLORS.plaster);
  for(let n=0;n<6;n++){
   const z=cz-d/2+.2+n*(d-.4)/5;
   this.box(cx-w*.25,y+height*.5,z,.07,.055,.08,COLORS.wood);
   this.box(cx+w*.25,y+height*.5,z,.07,.055,.08,COLORS.wood);
  }
 }
}
const COLORS={
 grass:[.32,.39,.26],grass2:[.27,.34,.23],ground:[.34,.36,.28],
 mud:[.33,.29,.22],road:[.48,.45,.37],stone:[.52,.49,.43],
 plaster:[.69,.66,.56],brick:[.48,.34,.28],timber:[.31,.24,.17],
 roof:[.47,.25,.20],roof2:[.38,.34,.30],wood:[.43,.31,.20],
 sand:[.57,.51,.38],leaf:[.24,.35,.23],leaf2:[.31,.41,.26],
 trunk:[.30,.23,.15],metal:[.28,.33,.32],dark:[.12,.14,.14],
 blue:[.27,.34,.38],skin:[.71,.57,.44],helmet:[.22,.27,.21],
 gold:[.86,.71,.33],red:[.65,.20,.15]
};
let seed=4026;function rnd(){seed^=seed<<13;seed^=seed>>>17;seed^=seed<<5;return (seed>>>0)/4294967296;}
const boxes=[
 [-24,-12,11,9],[-2,-25,9,8],[18,-12,11,11],[-22,13,10,10],
 [5,27,9,12],[28,1,10,9],[-39,15,8,10],
 [35,-27,10,12],[-8,38,10,11]
];
const flags=[{x:-12,z:-5,title:"FARM APPROACH"},{x:11,z:11,title:"CROSSROADS"},{x:31,z:29,title:"RADIO POST"}];
const treeTrunks=[];
const artStructures=[];
const covers=flags.flatMap(f=>[
 [f.x-6,f.z-7,4,.85],[f.x+6,f.z+6,4,.85],
 [f.x-6,f.z+5,.85,4],[f.x+5,f.z-6,.85,4]
]);
const within=(x,z,b,pad=0)=>Math.abs(x-b[0])<(b[2]/2+pad)&&Math.abs(z-b[1])<(b[3]/2+pad);
function blocked(x,z,r=.42){
 if(x<-47+r||x>47-r||z<-47+r||z>47-r)return true;
 return boxes.some(b=>within(x,z,b,r))||covers.some(b=>within(x,z,b,r))||artStructures.some(b=>within(x,z,b,r))||treeTrunks.some(t=>Math.hypot(x-t.x,z-t.z)<t.radius+r);
}
function openLOS(ax,az,bx,bz){
 const d=Math.hypot(bx-ax,bz-az),segments=Math.ceil(d/.55);
 for(let i=1;i<segments;i++){
  const t=i/segments;if(boxes.some(b=>within(ax+(bx-ax)*t,az+(bz-az)*t,b,.12))||
      covers.some(b=>within(ax+(bx-ax)*t,az+(bz-az)*t,b,.05))||
      artStructures.some(b=>within(ax+(bx-ax)*t,az+(bz-az)*t,b,.05))||
      treeTrunks.some(tr=>Math.hypot(ax+(bx-ax)*t-tr.x,az+(bz-az)*t-tr.z)<tr.radius))return false;
 }
 return true;
}
function buildWorld(){
 const m=new Geometry();
 m.quad([-49,-.1,-49],[-49,-.1,49],[49,-.1,49],[49,-.1,-49],COLORS.grass);
 for(let i=0;i<340;i++){
  const x=-46+rnd()*92,z=-46+rnd()*92,r=.35+rnd()*1.65;
  const col=rnd()>.45?COLORS.grass2:COLORS.ground;
  m.quad([x,.001,z],[x-r,.001,z+r*.45],[x+.18,.001,z+r],[x+r,.001,z],col);
 }
 // Country road with diagonal segments; no dynamic tessellation required.
 for(let i=-8;i<8;i++){
  const a=i*6;
  m.quad([a-2.2,.015,a-1],[a-2.2,.015,a+5],[a+2.2,.015,a+5],
    [a+2.2,.015,a-1],COLORS.road);
  m.box(a+1.7,.025,a+2,.17,.02,2.8,COLORS.mud);
 }
 for(const [j,b] of boxes.entries()){
  const [x,z,w,d]=b;
  const wall=j%3===0?COLORS.brick:j%3===1?COLORS.stone:COLORS.plaster;
  m.box(x,2.15,z,w,4.3,d,wall);
  m.roof(x,4.3,z,w+1,d+1,2.0,j%2?COLORS.roof:COLORS.roof2);
  for(let k=-1;k<=1;k+=2){
   m.box(x+k*w*.24,2.8,z+d/2+.05,1.1,1.15,.13,COLORS.dark);
   m.box(x+k*w*.24,2.8,z-d/2-.05,1.1,1.15,.13,COLORS.dark);
   m.box(x+k*w*.24,2.8,z+d/2+.13,.10,1.25,.12,COLORS.wood);
  }
  m.box(x,1.12,z+d/2+.09,1.45,2.23,.12,COLORS.timber);
  m.box(x,1.12,z-d/2-.09,1.45,2.23,.12,COLORS.timber);
  m.box(x,.2,z,w+.09,.4,d+.09,COLORS.stone);
  for(const side of [-1,1]){
   const front=z+side*(d/2+.14);
   m.box(x,2.18,front,w*.95,.13,.11,COLORS.wood);
   m.box(x,3.78,front,w*.95,.11,.12,COLORS.timber);
   for(const k of [-1,1]){
    const wx=x+k*w*.24;
    m.box(wx,2.20,front,.10,1.35,.12,COLORS.timber);
    m.box(wx,3.40,front,1.28,.10,.13,COLORS.wood);
    m.box(wx+k*.66,2.8,front,.33,1.26,.13,COLORS.wood);
    m.box(wx,2.8,front,.10,1.15,.14,COLORS.wood);
   }
   m.box(x,2.3,front,1.70,.15,.15,COLORS.stone);
   m.box(x+.45,1.1,front+side*.08,.10,.11,.12,COLORS.gold);
  }
  for(const k of [-1,1]){
   m.box(x+k*(w/2-.10),2.1,z,.15,4.1,.15,COLORS.wood);
   m.box(x,1.90,z+k*(d/2+.08),w*.95,.11,.13,COLORS.wood);
  }
  m.box(x+w*.31,5.65,z-d*.15,.68,2.05,.72,COLORS.brick);
  m.box(x+w*.31,6.72,z-d*.15,.87,.20,.90,COLORS.stone);
 }
 for(let i=0;i<28;i++){
  const x=-45+rnd()*90,z=-45+rnd()*90;
  if(blocked(x,z,2.2)||flags.some(f=>Math.hypot(x-f.x,z-f.z)<6))continue;
  const h=3.4+rnd()*2.3;
  m.box(x,h*.25,z,.35,h*.5,.35,COLORS.trunk);
  m.cone(x,h*.38,z,.9+rnd()*.5,h*.51,i%3?COLORS.leaf:COLORS.leaf2,7);
  m.cone(x,h*.57,z,.75,h*.4,COLORS.leaf2,7);
 }
 for(let i=0;i<85;i++){
  const x=-43+rnd()*86,z=-43+rnd()*86;
  if(blocked(x,z,1.7)||flags.some(f=>Math.hypot(x-f.x,z-f.z)<3))continue;
  const type=i%5;
  if(type===0){
   for(let j=0;j<4;j++)m.box(x+(j-1.5)*.95,.33,z,1.05,.65,.70,COLORS.sand);
  }else if(type===1){
   m.box(x,.45,z,.8,.9,.8,COLORS.wood);
   m.box(x,.45,z+.43,.9,.12,.12,COLORS.timber);
  }else{
   m.box(x,.18,z,.3+rnd()*.8,.35,.7,COLORS.stone);
  }
 }
 for(const [x,z,w,d] of covers){
  const alongX=w>d;
  for(let i=0;i<4;i++){
   const offset=(i-1.5)*(alongX?w:d)/4;
   for(let layer=0;layer<2;layer++)m.box(
    x+(alongX?offset:0)+(layer?.1:0),.24+layer*.42,z+(alongX?0:offset),
    alongX?w/4+.1:w,.40,alongX?d:d/4+.1,layer?COLORS.sand:COLORS.mud);
  }
 }
 // Rough fencing and scattered shell-scarred earth.
 for(let i=0;i<18;i++){
  const x=-43+i*4,z=-43+(i%3)*1.5;
  if(blocked(x,z,1.2))continue;
  m.box(x,.72,z,.12,1.44,.12,COLORS.timber);
  m.box(x,1.03,z+1.2,.09,.09,2.5,COLORS.wood);
 }
 for(let i=0;i<35;i++){
  const x=-42+rnd()*84,z=-41+rnd()*82,r=.45+rnd()*.8;
  if(blocked(x,z,1.8))continue;
  for(let n=0;n<10;n++){
   const t=n*Math.PI/5,nt=(n+1)*Math.PI/5;
   m.tri([x,-.04,z],[x+Math.cos(t)*r,.006,z+Math.sin(t)*r],
    [x+Math.cos(nt)*r,.006,z+Math.sin(nt)*r],COLORS.mud);
  }
 }
 for(const f of flags){
  m.box(f.x,2.65,f.z,.10,5.3,.10,COLORS.metal);
  m.quad([f.x+.05,4.9,f.z],[f.x+.05,4.9,f.z+1.5],
   [f.x+.05,3.9,f.z+1.5],[f.x+.05,3.9,f.z],COLORS.gold);
  for(let n=0;n<12;n++){
   let a=n*Math.PI/6,b=(n+1)*Math.PI/6,r=2.7;
   m.tri([f.x,.045,f.z],[f.x+Math.cos(a)*r,.045,f.z+Math.sin(a)*r],
    [f.x+Math.cos(b)*r,.045,f.z+Math.sin(b)*r],n%2?COLORS.gold:COLORS.sand);
  }
 }
 return m.data;
}
const staticMesh=mesh(false),liveMesh=mesh(true);
const fallbackWorld=buildWorld();
upload(staticMesh,fallbackWorld);
const scenery={loaded:false,instances:0,triangles:0,error:""};
const rig={model:null,ready:false,error:"",skinnedLastFrame:0,skinnedTriangles:0};
function loadSkinnedCharacters(){
 const runtime=window.WPSkinning;
 if(!runtime)return;
 runtime.load("character_medium").then(model=>{
  rig.model=model;rig.ready=true;
  if($("rigstatus"))$("rigstatus").textContent="58-JOINT CC0 CHARACTER RIG READY";
 }).catch(error=>{
  rig.error=String(error);
  if($("rigstatus"))$("rigstatus").textContent="FALLBACK SOLDIERS READY";
 });
}

function loadLicensedScenery(){
 const kit=window.WPAssetKit;
 if(!kit||typeof fetch!=="function"){scenery.error="GLB loader or fetch unavailable";return;}
 const models=["tree-oak","tree-pinedefaulta","rock-largea","plant-bush","fence-simple"];
 kit.loadModels(models.map(name=>({name}))).then(assets=>{
  // This is an optional visual layer; never replace the validated battle geometry.
  let state=944628,rand=()=>{state^=state<<13;state^=state>>>17;state^=state<<5;return (state>>>0)/4294967296;};
  const world=new Geometry();
  world.data=fallbackWorld.slice();
  let copies=0,added=0;
  const specs=[
   {name:"tree-oak",count:31,scale:[4.5,6.3],palette:[[.21,.34,.18],[.37,.31,.25]]},
   {name:"tree-pinedefaulta",count:20,scale:[4.5,6.2],palette:[[.19,.30,.20],[.34,.29,.24],[.28,.37,.25]]},
   {name:"plant-bush",count:60,scale:[3.2,5.6],palette:[[.25,.36,.20],[.32,.40,.23]]},
   {name:"rock-largea",count:34,scale:[2.7,4.8],palette:[[.41,.42,.39],[.48,.47,.42]]},
   {name:"fence-simple",count:12,scale:[3.0,3.6],palette:[[.39,.31,.23],[.49,.38,.27]]}
  ];
  for(const spec of specs){
   const positions=[];
   let tries=0;
   while(positions.length<spec.count&&tries++<spec.count*32){
    const x=-44+rand()*88,z=-44+rand()*88;
    if(blocked(x,z,2.2)||Math.hypot(x+36,z+33)<8||
      flags.some(f=>Math.hypot(x-f.x,z-f.z)<7))continue;
    const scale=spec.scale[0]+rand()*(spec.scale[1]-spec.scale[0]);
    positions.push([x,0,z,scale,rand()*Math.PI*2]);
    if(spec.name.startsWith("tree-")){
     treeTrunks.push({x,z,radius:.46});
    }
   }
   added+=kit.appendInstances(world,assets[spec.name],positions,spec.palette);
   copies+=positions.length;
  }
  upload(staticMesh,world.data);
  scenery.loaded=true;scenery.instances=copies;scenery.triangles=added;
  // Independent CC0 farm import: if a model is missing, keep the Kenney scene.
  const farm=["barn","chicken_coop","windmill","well"];
  kit.loadModels(farm.map(name=>({name,url:"./assets/quaternius/"+name+".glb.b64"}))).then(buildings=>{
   const extras=new Geometry();
   extras.data=world.data.slice();
   const sites=[
    {name:"barn",x:-37,z:30,scale:.96},
    {name:"chicken_coop",x:-35,z:-5,scale:1.25},
    {name:"windmill",x:41,z:40,scale:.84},
    {name:"well",x:13,z:-38,scale:1.2}
   ];
   const colliders=[];
   let ntri=0;
   for(const site of sites){
    const model=buildings[site.name];
    ntri+=kit.appendInstances(extras,model,[[site.x,0,site.z,site.scale,0]]);
    const min=model.min,max=model.max;
    colliders.push([
     site.x+(max[0]+min[0])*site.scale*.5,
     site.z+(max[2]+min[2])*site.scale*.5,
     (max[0]-min[0])*site.scale,(max[2]-min[2])*site.scale
    ]);
   }
   upload(staticMesh,extras.data);
   artStructures.push(...colliders);
   scenery.instances+=sites.length;scenery.triangles+=ntri;
   scenery.buildings=sites.length;
   if($("assetstatus"))$("assetstatus").textContent=
     "KENNEY + QUATERNIUS CC0 · "+scenery.instances+" IMPORTED MODELS";
  }).catch(err=>{
   scenery.farmError=String(err);
   if($("assetstatus"))$("assetstatus").textContent=
     "KENNEY CC0 SCENERY READY · FARM PACK OPTIONAL";
  });
  
  if($("assetstatus"))$("assetstatus").textContent="KENNEY CC0 SCENERY READY · "+copies+" PLACED ASSETS";
 }).catch(err=>{
  scenery.error=String(err);
  if($("assetstatus"))$("assetstatus").textContent="LIGHTWEIGHT SCENERY ACTIVE · OPTIONAL ART UNAVAILABLE";
  // If an import fails, keep the code-built baseline without partial collision.
  treeTrunks.length=0;
 });
}

const WEAPONS={
 rifle:{name:"LEE-ENFIELD",capacity:10,reserve:70,damage:55,delay:.6,reload:1.9,spread:.015},
 smg:{name:"STEN",capacity:32,reserve:128,damage:19,delay:.10,reload:1.6,spread:.052},
 lmg:{name:"BREN",capacity:30,reserve:100,damage:32,delay:.16,reload:2.7,spread:.038}
};
const player={x:-36,z:-33,yaw:-.77,pitch:0,hp:100,maxHp:100,ammo:10,reserve:70,
 weapon:"rifle",cd:0,reload:0,aim:false,crouch:false,frags:2,med:2,hurt:0};
const game={playing:false,finished:false,sector:0,capture:0,kills:0,elapsed:0,
 enemies:[],allies:[],grenades:[],tracers:[],strikes:[],support:1,order:"follow",
 flash:0,quality:"medium",difficulty:"regular",fps:45,scaler:1,adapt:0};
const input={keys:{},fire:false,joyX:0,joyY:0,movePointer:null,lookPointer:null,
 lookX:0,lookY:0};
function spawnEnemies(){
 game.enemies=[];
 for(let n=0;n<flags.length;n++){
  const f=flags[n];
  const positions=[[3,0],[-2.3,4.4],[4.5,-3.4],[-4.4,-2.7],[0,6.2]];
  let count=0;
  for(const [dx,dz] of positions){
   const x=f.x+dx,z=f.z+dz;
   if(blocked(x,z,.8))continue;
   game.enemies.push({x,z,hp:80,maxHp:80,sector:n,cd:.5+rnd()*1.7,
     facing:rnd()*Math.PI*2,moveSpeed:0,
    alert:0,lastX:x,lastZ:z,strafe:rnd()>.5?1:-1,phase:rnd()*6,hit:0});
   if(++count>=3+n)break;
  }
 }
}
function spawnAllies(){
 game.allies=[{x:-37.5,z:-35,hp:85,cd:1.1,ammo:16,reload:0,phase:0,moveSpeed:0,facing:-.78,alert:1},
             {x:-33.5,z:-35,hp:85,cd:1.5,ammo:16,reload:0,phase:2.1,moveSpeed:0,facing:-.78,alert:1}];
}
function addTracer(ax,az,bx,bz,team){
 game.tracers.push({ax,az,bx,bz,life:.12,team});
 if(game.tracers.length>30)game.tracers.shift();
}
function orderSquad(){
 if(!game.playing)return;
 game.order=game.order==="follow"?"hold":"follow";
 msg("FIRETEAM: "+(game.order==="follow"?"FOLLOW AND COVER":"HOLD POSITION"),1.6);
}
function callArtillery(){
 if(!game.playing||game.support<1)return;
 game.support--;
 const f=flags[game.sector];
 const tx=player.x-Math.sin(player.yaw)*18,tz=player.z+Math.cos(player.yaw)*18;
 const x=f&&Math.hypot(tx-f.x,tz-f.z)<8?f.x:Math.max(-44,Math.min(44,tx));
 const z=f&&Math.hypot(tx-f.x,tz-f.z)<8?f.z:Math.max(-44,Math.min(44,tz));
 game.strikes.push({x,z,life:2.1,detonated:false});
 msg("ARTILLERY INBOUND · KEEP CLEAR",2);
}
function reset(){
 const type=$("weapon").value in WEAPONS?$("weapon").value:"rifle";
 const weapon=WEAPONS[type];
 Object.assign(player,{x:-36,z:-33,yaw:-.78,pitch:0,hp:100,maxHp:100,
   ammo:weapon.capacity,reserve:weapon.reserve,weapon:type,cd:0,reload:0,aim:false,
  frags:2,med:2,hurt:0,crouch:false});
 Object.assign(game,{playing:true,finished:false,sector:0,capture:0,kills:0,elapsed:0,
  grenades:[],tracers:[],strikes:[],support:1,order:"follow",flash:0,
   difficulty:$("difficulty").value,quality:$("quality").value,scaler:1,adapt:0});
 spawnEnemies();spawnAllies();
 game.loadouts=Object.fromEntries(Object.entries(WEAPONS).map(([name,w])=>[
  name,{ammo:w.capacity,reserve:w.reserve}
 ]));
 if(window.WPBattleSound)window.WPBattleSound.boot();
 if(window.WP3DProbe)window.WP3DProbe.reset();
 $("menu").hidden=true;input.fire=false;
 const fine=typeof window.matchMedia==="function"&&window.matchMedia("(pointer: fine)").matches;
 if(fine&&canvas.requestPointerLock)try{canvas.requestPointerLock();}catch{}
 const hint=$("movement-hint");if(hint){hint.hidden=false;hint.textContent="WASD / ARROWS TO MOVE · MOUSE TO LOOK · SHIFT SPRINT · F3 STATS";}
 
 msg("CAPTURE THE FORWARD FARM",2);hud();resize();
}
function msg(s,time=1.4){$("status").textContent=s;game.notice=time;}
function hud(){
 $("hp").textContent="HP "+Math.ceil(Math.max(0,player.hp));
 $("mag").textContent=player.reload>0?"RELOADING":player.ammo+" / "+player.reserve;
 $("frags").textContent="FRAG × "+player.frags;
 const flag=flags[game.sector];
 $("mission").textContent=flag?"SECURE "+flag.title:"MISSION COMPLETE";
 $("objective").textContent=flag?"SECTOR "+(game.sector+1)+" / 3 · "+Math.floor(game.capture*100)+"% SECURED":"ALL SECTORS CAPTURED";
 $("info").textContent=WEAPONS[player.weapon].name+" · "+(player.crouch?"CROUCHED":player.aim?"ADS":"HIP FIRE")+" · "+game.kills+" KILLS";
 $("squadstate").textContent="FIRETEAM "+game.allies.filter(a=>a.hp>0).length+"/2 · "+game.order.toUpperCase();
 $("supportstate").textContent="ARTILLERY × "+game.support;
 $("capture-fill").style.width=Math.floor(game.capture*100)+"%";
 if(flag){
  const distance=Math.round(Math.hypot(player.x-flag.x,player.z-flag.z));
  const bearing=-Math.atan2(flag.x-player.x,flag.z-player.z)-player.yaw;
  const angle=Math.atan2(Math.sin(bearing),Math.cos(bearing));
  const pointer=Math.abs(angle)<.4?"↑":Math.abs(angle)>2.7?"↓":angle>0?"↗":"↖";
  $("nav").textContent=pointer+" "+distance+"m TO "+flag.title;
 }else $("nav").textContent="ALL OBJECTIVES SECURED";
 $("fps").textContent="3D · "+game.quality.toUpperCase()+" · "+Math.round(game.fps)+" FPS";
 $("hurt").style.opacity=player.hurt>.02?String(Math.min(.42,player.hurt*.22)):"0";
 $("crosshair").classList.toggle("aiming",player.aim);
}
function end(victory){
 game.playing=false;game.finished=true;input.fire=false;player.aim=false;
 $("menu").hidden=false;
 $("menu-title").innerHTML=victory?"OPERATION<br><em>COMPLETE</em>":"SQUAD<br><em>ELIMINATED</em>";
 $("menu-copy").textContent=victory?
 "All three objectives secured in "+Math.floor(game.elapsed/60)+"m "+Math.floor(game.elapsed%60)+"s. "+game.kills+" defenders neutralized. Try a higher difficulty.":
 "Your soldier was eliminated. Re-deploy with another weapon or use cover and field dressings.";
 $("deploy").textContent="REDEPLOY →";
}
function move(x,z,dx,dz){
 if(!blocked(x+dx,z,.44))x+=dx;
 if(!blocked(x,z+dz,.44))z+=dz;
 return [x,z];
}
function lineHit(ax,az,bx,bz){
 return openLOS(ax,az,bx,bz);
}
function shoot(){
 if(!game.playing||player.reload>0||player.cd>0)return;
 const w=WEAPONS[player.weapon];
 if(player.ammo<=0){reload();return;}
 player.cd=w.delay;player.ammo--;game.flash=.13;
 if(window.WP3DProbe)window.WP3DProbe.event("shot");
 const dx=-Math.sin(player.yaw),dz=Math.cos(player.yaw);
 const accuracy=player.aim?.32:1;
 const moving=Math.hypot(input.joyX,input.joyY)>.1;
 const shift=(rnd()-.5)*(w.spread*accuracy+(moving?.012:0));
 const shootYaw=player.yaw+shift;
 const sx=-Math.sin(shootYaw),sz=Math.cos(shootYaw);
 let target=null,nearest=70;
 for(const e of game.enemies){
  if(e.hp<=0)continue;
  const vx=e.x-player.x,vz=e.z-player.z,t=vx*sx+vz*sz;
  const off=Math.abs(vx*sz-vz*sx);
  if(t>0&&t<nearest&&off<.42+Math.abs(t)*.003 &&
   Math.abs(player.pitch)<.055+.45/Math.max(.6,t) &&
   lineHit(player.x,player.z,e.x,e.z)){
   target=e;nearest=t;
  }
 }
 player.yaw+=(rnd()-.5)*w.spread*.32;
 player.pitch=Math.max(-.7,Math.min(.7,player.pitch+w.spread*(player.aim?.2:.6)));
 const range=target?Math.hypot(target.x-player.x,target.z-player.z):38;
 addTracer(player.x,player.z,player.x+sx*range,player.z+sz*range,"friendly");
 if(target){
  target.hp-=w.damage*(.9+rnd()*.2);target.hit=.25;
  if(window.WP3DProbe)window.WP3DProbe.event("hit");
  target.suppression=1.7;
  $("hitmarker").classList.add("visible");
  game.hitmarker=.12;
  if(target.hp<=0){game.kills++;if(window.WP3DProbe)window.WP3DProbe.event("kill");msg("TARGET DOWN",.85);}
 }
 if(window.WPBattleSound)window.WPBattleSound.shot(player.weapon,player.x,player.z,player);
 else beep(player.weapon==="smg"?165:player.weapon==="lmg"?95:120,.045);
}
let audioCtx=null;
const reportBuffers=new Map();
function beep(freq,dur){
 try{
  const Constructor=window.AudioContext||window.webkitAudioContext;
  if(!Constructor)return;
  audioCtx=audioCtx||new Constructor();
  if(audioCtx.state==="suspended")audioCtx.resume();
  const now=audioCtx.currentTime;
  const key=freq+"-"+dur;
  let buffer=reportBuffers.get(key);
  if(!buffer){
   const seconds=Math.max(.085,dur*3),rate=audioCtx.sampleRate;
   buffer=audioCtx.createBuffer(1,Math.ceil(seconds*rate),rate);
   const data=buffer.getChannelData(0);
   for(let i=0;i<data.length;i++){
    const decay=Math.exp(-6*i/data.length);
    data[i]=(Math.random()*2-1)*decay;
   }
   reportBuffers.set(key,buffer);
  }
  const report=audioCtx.createBufferSource(),filter=audioCtx.createBiquadFilter();
  report.buffer=buffer;
  filter.type="lowpass";filter.frequency.value=freq<90?460:freq<130?1300:1700;
  const noiseGain=audioCtx.createGain();
  noiseGain.gain.setValueAtTime(freq<90?.25:.15,now);
  noiseGain.gain.exponentialRampToValueAtTime(.001,now+buffer.duration);
  report.connect(filter);filter.connect(noiseGain);noiseGain.connect(audioCtx.destination);
  const thump=audioCtx.createOscillator(),bass=audioCtx.createGain();
  thump.type="triangle";thump.frequency.setValueAtTime(freq,now);
  thump.frequency.exponentialRampToValueAtTime(Math.max(25,freq*.42),now+dur);
  bass.gain.setValueAtTime(.15,now);
  bass.gain.exponentialRampToValueAtTime(.001,now+dur);
  thump.connect(bass);bass.connect(audioCtx.destination);
  report.start(now);thump.start(now);thump.stop(now+dur);
 }catch{}
}
function reload(){
 if(!game.playing||player.reload>0)return;
 const w=WEAPONS[player.weapon];
 if(player.ammo>=w.capacity||player.reserve<=0)return;
 player.reload=w.reload;msg("RELOADING "+w.name,1);
}
function setAim(v){player.aim=game.playing&&!!v;}
function grenade(){
 if(!game.playing||player.frags<=0)return;
 player.frags--;
 const reach=9.5,dx=-Math.sin(player.yaw),dz=Math.cos(player.yaw);
 const x=player.x+dx*reach,z=player.z+dz*reach;
 game.grenades.push({x,z,life:1.25});msg("FRAG OUT!",1.1);
}
function heal(){
 if(!game.playing||!player.med||player.hp>=100)return;
 player.med--;player.hp=Math.min(100,player.hp+36);msg("FIELD DRESSING APPLIED",1.1);
}
function swap(){
 if(!game.playing)return;
 const names=Object.keys(WEAPONS),idx=names.indexOf(player.weapon);
 game.loadouts[player.weapon]={ammo:player.ammo,reserve:player.reserve};
 player.weapon=names[(idx+1)%names.length];
 const w=WEAPONS[player.weapon],store=game.loadouts[player.weapon];
 player.ammo=store.ammo;player.reserve=store.reserve;
 player.reload=0;msg("EQUIPPED "+w.name,1.3);
}
function updateCombinedArms(dt){
 for(const t of game.tracers)t.life-=dt;
 game.tracers=game.tracers.filter(t=>t.life>0);
 for(const strike of game.strikes){
  strike.life-=dt;
  if(strike.life<=0&&!strike.detonated){
   strike.detonated=true;
   for(const e of game.enemies){
    if(e.hp<=0)continue;
    const d=Math.hypot(e.x-strike.x,e.z-strike.z);
    if(d<8&&lineHit(strike.x,strike.z,e.x,e.z)){
     e.hp-=135*(1-d/11);e.hit=.4;e.suppression=2.2;
     if(e.hp<=0)game.kills++;
    }
   }
   const d=Math.hypot(player.x-strike.x,player.z-strike.z);
   if(d<6){player.hp-=48*(1-d/6);player.hurt=1;}
   for(const ally of game.allies){
    const da=Math.hypot(ally.x-strike.x,ally.z-strike.z);
    if(da<6)ally.hp-=44*(1-da/6);
   }
   if(window.WPBattleSound)window.WPBattleSound.explosion(strike.x,strike.z,player);
 else beep(50,.32);
 msg("ARTILLERY IMPACT",.9);
   if(player.hp<=0){end(false);return;}
  }
 }
 game.strikes=game.strikes.filter(t=>t.life>-.42);
 for(let i=0;i<game.allies.length;i++){
  const a=game.allies[i];if(a.hp<=0)continue;
  a.cd-=dt;
  if(a.reload>0){a.reload-=dt;if(a.reload<=0)a.ammo=16;}
  const side=i?1:-1,behind=3+i*.8;
  const fx=player.x+Math.sin(player.yaw)*behind+Math.cos(player.yaw)*side*2.1;
  const fz=player.z-Math.cos(player.yaw)*behind+Math.sin(player.yaw)*side*2.1;
  const to=Math.hypot(fx-a.x,fz-a.z),oldX=a.x,oldZ=a.z;
  if(game.order==="follow"&&to>2.1){
   [a.x,a.z]=move(a.x,a.z,(fx-a.x)/to*Math.min(4.7,to)*dt,
    (fz-a.z)/to*Math.min(4.7,to)*dt);
  }
  a.moveSpeed=Math.hypot(a.x-oldX,a.z-oldZ)/Math.max(dt,.001);
  if(a.moveSpeed>.05)a.facing=Math.atan2(player.x-a.x,player.z-a.z);
  let victim=null,near=26;
  for(const enemy of game.enemies){
   const dist=Math.hypot(enemy.x-a.x,enemy.z-a.z);
   if(enemy.hp>0&&dist<near&&lineHit(a.x,a.z,enemy.x,enemy.z)){
    near=dist;victim=enemy;
   }
  }
  if(victim)a.facing=Math.atan2(victim.x-a.x,victim.z-a.z);
  if(victim&&a.cd<=0&&a.reload<=0){
   if(a.ammo<=0){a.reload=1.9;continue;}
   a.ammo--;a.cd=.42+rnd()*.48;
   addTracer(a.x,a.z,victim.x,victim.z,"friendly");
  if(window.WPBattleSound)window.WPBattleSound.shot("rifle",a.x,a.z,player);
   if(rnd()<(.49-near*.006)){
    victim.hp-=19+rnd()*10;victim.hit=.18;victim.suppression=1.2;
    if(victim.hp<=0){game.kills++;msg("FIRETEAM: TARGET DOWN",.9);}
   }
  }
 }
}
function simulate(dt){
 if(!game.playing)return;
 game.elapsed+=dt;
 player.cd=Math.max(0,player.cd-dt);player.hurt=Math.max(0,player.hurt-dt*2);
 game.flash=Math.max(0,game.flash-dt);
 if(game.notice>0){game.notice-=dt;if(game.notice<=0)$("status").textContent="ADVANCE · CLEAR · CAPTURE";}
 if(game.hitmarker>0){game.hitmarker-=dt;if(game.hitmarker<=0)$("hitmarker").classList.remove("visible");}
 if(player.reload>0){
  player.reload-=dt;
  if(player.reload<=0){const w=WEAPONS[player.weapon],qty=Math.min(w.capacity-player.ammo,player.reserve);
   player.ammo+=qty;player.reserve-=qty;}
 }
 let forward=(input.keys.KeyW||input.keys.ArrowUp?1:0)-(input.keys.KeyS||input.keys.ArrowDown?1:0)-input.joyY;
 let strafe=(input.keys.KeyD||input.keys.ArrowRight?1:0)-(input.keys.KeyA||input.keys.ArrowLeft?1:0)+input.joyX;
 const mag=Math.hypot(forward,strafe);
 if(mag>1){forward/=mag;strafe/=mag;}
 const pace=(input.keys.ShiftLeft&&!player.crouch?6.6:4.5)*(player.aim?.72:1)*(player.crouch?.57:1);
 const x=-Math.sin(player.yaw)*forward-Math.cos(player.yaw)*strafe;
 const z=Math.cos(player.yaw)*forward-Math.sin(player.yaw)*strafe;
 const oldX=player.x,oldZ=player.z;
 [player.x,player.z]=move(player.x,player.z,x*pace*dt,z*pace*dt);
 if(window.WP3DProbe)window.WP3DProbe.movement(player.x,player.z,mag,pace,dt);
 if(window.WPBattleSound){
  const speed=Math.hypot(player.x-oldX,player.z-oldZ)/Math.max(.001,dt);
  window.WPBattleSound.footsteps(game.elapsed,speed,player.crouch);
  window.WPBattleSound.distant(game.elapsed);
 }
 if(input.fire)shoot();
 for(const g of game.grenades){
  if(g.life<=0)continue;
  g.life-=dt;
  if(g.life<=0&&!g.detonated){
   g.detonated=true;
   for(const e of game.enemies){
    if(e.hp<=0)continue;
    const d=Math.hypot(g.x-e.x,g.z-e.z);
    if(d<5&&lineHit(g.x,g.z,e.x,e.z)){
     e.hp-=110*(1-d/5);e.hit=.4;
     if(e.hp<=0)game.kills++;
    }
   }
   if(Math.hypot(g.x-player.x,g.z-player.z)<4){
    player.hp-=Math.max(0,38*(1-Math.hypot(g.x-player.x,g.z-player.z)/4));
    player.hurt=1;
   }
   if(window.WPBattleSound)window.WPBattleSound.explosion(g.x,g.z,player);
 else beep(62,.25);
 msg("EXPLOSION",.7);
  }
 }
 game.grenades=game.grenades.filter(g=>g.life>-.3);
 updateCombinedArms(dt);
 if(!game.playing)return;
 for(const e of game.enemies){
  if(e.hp<=0)continue;
  e.cd-=dt;e.hit=Math.max(0,e.hit-dt);
  e.suppression=Math.max(0,(e.suppression||0)-dt);
  let victim=null,d=26;
  for(const candidate of [player,...game.allies]){
   if(candidate.hp<=0)continue;
   const range=Math.hypot(candidate.x-e.x,candidate.z-e.z);
   if(range<d&&lineHit(e.x,e.z,candidate.x,candidate.z)){victim=candidate;d=range;}
  }
  const visible=!!victim;
  if(visible){e.alert=2.1;e.lastX=victim.x;e.lastZ=victim.z;}
  else e.alert=Math.max(0,e.alert-dt);
  const tx=visible?victim.x:e.lastX,tz=visible?victim.z:e.lastZ;
  const targetD=Math.hypot(tx-e.x,tz-e.z),oldX=e.x,oldZ=e.z;
  if(e.alert>0&&targetD>7){
   const speed=e.suppression>.2?.38:visible?1.24:.84;
   [e.x,e.z]=move(e.x,e.z,(tx-e.x)/targetD*speed*dt,(tz-e.z)/targetD*speed*dt);
  }
  e.moveSpeed=Math.hypot(e.x-oldX,e.z-oldZ)/Math.max(dt,.001);
  if(e.alert>0)e.facing=Math.atan2(tx-e.x,tz-e.z);
  if(visible&&d<21&&e.cd<=0){
   e.cd=(e.suppression>.2?2.2:1.3)+rnd()*.95;
   const difficulty=game.difficulty==="veteran"?1.18:game.difficulty==="recruit"?.55:1;
   const hit=Math.max(.05,(.51-d*.013)*(victim.crouch?.52:1)*difficulty*
     (e.suppression>.2?.4:1));
   addTracer(e.x,e.z,victim.x,victim.z,"hostile");
   if(window.WPBattleSound)window.WPBattleSound.shot("rifle",e.x,e.z,player);
   if(rnd()<hit){
    const damage=(6+rnd()*7)*(game.difficulty==="veteran"?1.2:game.difficulty==="recruit"?.7:1);
    victim.hp-=damage;
    if(victim===player){
     player.hurt=.9;msg("UNDER FIRE · FIND COVER",.8);
     if(player.hp<=0){end(false);return;}
    }else if(victim.hp<=0)msg("FIRETEAM CASUALTY",1.8);
   }
  }
 }
 const flag=flags[game.sector];
 if(flag){
  const defenders=game.enemies.filter(e=>e.sector===game.sector&&e.hp>0);
  const distance=Math.hypot(player.x-flag.x,player.z-flag.z);
  if(distance<4&&!defenders.length){
   game.capture=Math.min(1,game.capture+dt/2.5);
   if(game.capture>=1){
    game.sector++;game.capture=0;
   if(window.WP3DProbe)window.WP3DProbe.event("capture");
    player.hp=Math.min(100,player.hp+17);player.reserve+=24;
    player.frags=Math.min(3,player.frags+1);game.support=Math.min(2,game.support+1);
    if(game.sector===flags.length){end(true);return;}
    msg("SECTOR SECURED · RESUPPLIED AND ARTILLERY READY",2.5);
   }
  }else if(distance>=4)game.capture=Math.max(0,game.capture-dt*.16);
 }
 hud();
}
function dynamicGeometry(){
 const m=new Geometry();
 const art=window.WPBattleArt,skinner=window.WPSkinning;
 let skinnedActors=0,skinnedTriangles=0;
 // Save low-end phones: limit both shading work and CPU-skinned scene actors.
 const limit=game.quality==="high"?4:game.quality==="medium"?2:0;
 const maxActors=game.fps<24?Math.min(limit,1):game.fps<35?Math.min(limit,2):limit;
 const viewDist=game.quality==="low"?14:game.quality==="medium"?23:28;
 const candidates=[
  ...game.allies.filter(a=>a.hp>0),
  ...game.enemies.filter(e=>e.hp>0)
 ].sort((a,b)=>Math.hypot(a.x-player.x,a.z-player.z)-Math.hypot(b.x-player.x,b.z-player.z));
 const selected=new Set(candidates.slice(0,maxActors));
 const present=(actor,team)=>{
  const distance=Math.hypot(actor.x-player.x,actor.z-player.z);
  if(rig.ready&&skinner&&distance<viewDist&&selected.has(actor)){
   const drawn=skinner.append(m,rig.model,actor,team,game.elapsed);
   if(art?.fieldEquipment)art.fieldEquipment(m,actor,team);
   skinnedActors++;skinnedTriangles+=drawn.triangles;
  }else if(art)art.soldier(m,actor,team,game.elapsed,actor.moveSpeed||0);
  else{
   const color=team==="ally"?COLORS.blue:COLORS.dark;
   m.box(actor.x,1.4,actor.z,.7,1.5,.5,color);
   m.box(actor.x,2.2,actor.z,.5,.48,.5,COLORS.helmet);
  }
 };
 for(const e of game.enemies)if(e.hp>0)present(e,"axis");
 for(const a of game.allies)if(a.hp>0)present(a,"ally");
 rig.skinnedLastFrame=skinnedActors;
 rig.skinnedTriangles=skinnedTriangles;
 for(const t of game.tracers){
  const dx=t.bx-t.ax,dz=t.bz-t.az,len=Math.hypot(dx,dz);
  if(len<.1)continue;
  const x=(t.ax+t.bx)/2,z=(t.az+t.bz)/2;
  m.box(x,1.46,z,.025,.023,Math.min(len,12),
   t.team==="hostile"?COLORS.red:COLORS.gold,Math.atan2(dx,dz));
 }
 for(const strike of game.strikes){
  if(!strike.detonated){
   m.box(strike.x,.35,strike.z,.08,.7,.08,COLORS.red);
  }else{
   const radius=Math.max(.1,3.2*(1+strike.life));
   m.cone(strike.x,0,strike.z,radius,4.5,COLORS.gold,12);
   m.cone(strike.x,0,strike.z,radius*1.3,2.5,COLORS.mud,10);
  }
 }
 for(const g of game.grenades){
  if(g.life<=0){
   m.cone(g.x,.03,g.z,1.4,3,COLORS.red,12);
  }else m.box(g.x,.22,g.z,.25,.4,.25,COLORS.metal);
 }
 // Model-specific Lee-Enfield, Sten and Bren view models (or compatibility gun).
 if(art)art.weapon(m,player,game);
 else{
  const dx=-Math.sin(player.yaw),dz=Math.cos(player.yaw),yaw=-player.yaw;
  const py=(player.crouch?1.02:1.67)-.4;
  m.box(player.x+dx*.85,py,player.z+dz*.85,.2,.17,.85,COLORS.metal,yaw);
 }
 return m.data;
}
function perspective(fov,aspect,near,far){
 const f=1/Math.tan(fov/2),nf=1/(near-far);
 return new Float32Array([f/aspect,0,0,0,0,f,0,0,0,0,(far+near)*nf,-1,0,0,2*far*near*nf,0]);
}
function lookAt(eye,target){
 let z=[eye[0]-target[0],eye[1]-target[1],eye[2]-target[2]];
 let n=Math.hypot(...z)||1;z=z.map(v=>v/n);
 let x=[z[2],0,-z[0]];n=Math.hypot(...x)||1;x=x.map(v=>v/n);
 const y=[z[1]*x[2]-z[2]*x[1],z[2]*x[0]-z[0]*x[2],z[0]*x[1]-z[1]*x[0]];
 return new Float32Array([x[0],y[0],z[0],0,x[1],y[1],z[1],0,
  x[2],y[2],z[2],0,-(x[0]*eye[0]+x[1]*eye[1]+x[2]*eye[2]),
 -(y[0]*eye[0]+y[1]*eye[1]+y[2]*eye[2]),
 -(z[0]*eye[0]+z[1]*eye[1]+z[2]*eye[2]),1]);
}
function mul(a,b){
 const out=new Float32Array(16);
 for(let c=0;c<4;c++)for(let r=0;r<4;r++)for(let k=0;k<4;k++)
  out[c*4+r]+=a[k*4+r]*b[c*4+k];
 return out;
}
let lastWidth=0,lastHeight=0;
function resize(){
 const ratio=window.devicePixelRatio||1;
 const quality={low:.78,medium:1.08,high:1.55}[game.quality]||.78;
 const scalar=Math.min(1.85,ratio*quality*game.scaler);
 const maxPixels={low:340000,medium:800000,high:1650000}[game.quality]||340000;
 const w=Math.max(160,Math.floor(canvas.clientWidth*scalar));
 const h=Math.max(100,Math.floor(canvas.clientHeight*scalar));
 const limiter=Math.min(1,Math.sqrt(maxPixels/(w*h)));
 canvas.width=Math.floor(w*limiter);canvas.height=Math.floor(h*limiter);
 lastWidth=canvas.width;lastHeight=canvas.height;
 gl.viewport(0,0,lastWidth,lastHeight);
}
function render(){
 if(!lastWidth||!lastHeight)return;
 gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);
 const eye=[player.x,player.crouch?1.02:1.67,player.z],forward=[
  -Math.sin(player.yaw)*Math.cos(player.pitch),Math.sin(player.pitch),
  Math.cos(player.yaw)*Math.cos(player.pitch)];
 const proj=perspective((player.aim?42:73)*Math.PI/180,lastWidth/lastHeight,.065,120);
 const view=lookAt(eye,[eye[0]+forward[0],eye[1]+forward[1],eye[2]+forward[2]]);
 gl.uniformMatrix4fv(vpLocation,false,mul(proj,view));
 gl.uniform3fv(eyeLocation,new Float32Array(eye));
 // Uniform detail tier keeps shader variants consistent; mobile low skips noise.
 if(gl.uniform1f){
  gl.uniform1f(detailLocation,game.quality==="low"?0:game.quality==="medium"?.58:1);
  gl.uniform1f(useAtlasLocation,materialTextures?1:0);
 }
 if(materialTextures&&atlasKit)atlasKit.bind(gl,{diffuse:diffuseLocation,normal:bumpLocation},materialTextures);
 draw(staticMesh);
 upload(liveMesh,dynamicGeometry());
 draw(liveMesh);
}
let previous=0,adaptTime=0;
const nowClock=()=>typeof performance!=="undefined"&&performance.now?performance.now():Date.now();

function frame(now){
 const wallStart=nowClock();
 if(!previous)previous=now;
 const rawMs=Math.max(0,Math.min(1000,now-previous));
 previous=now;
 // Use real elapsed time for movement. A capped single 45ms simulation step
 // previously made the soldier crawl when the GPU/CPU fell below ~22 FPS.
 const dt=Math.min(.20,rawMs/1000);
 if(game.playing&&dt>0){
  const ticks=Math.max(1,Math.ceil(dt/.034)),step=dt/ticks;
  for(let i=0;i<ticks&&game.playing;i++)simulate(step);
 }
 const cpuAfterSimulation=nowClock();
 if(game.playing)render(); // The deployment dialog needs no 3D frame rendering.
 const renderMs=nowClock()-cpuAfterSimulation;
 if(renderMs>175&&game.scaler>.55){
  game.scaler=Math.max(.55,game.scaler-.16);
  resize();
 }
 if(rawMs>0){
  const fps=1000/rawMs;
  game.fps=game.fps*.90+fps*.10;
  adaptTime+=rawMs/1000;
  if(adaptTime>3){
   adaptTime=0;
   const target=game.quality==="low"?28:game.quality==="medium"?42:57;
   const next=game.fps<target*.77?Math.max(.55,game.scaler-.10):
    game.fps>target*.97?Math.min(1,game.scaler+.05):game.scaler;
   if(Math.abs(next-game.scaler)>.015){game.scaler=next;resize();}
  }
  if(window.WP3DProbe)window.WP3DProbe.frame(rawMs,renderMs,nowClock()-wallStart);
 }
 requestAnimationFrame(frame);
}
window.addEventListener("resize",resize);
document.addEventListener("keydown",e=>{
 input.keys[e.code]=true;
 if(game.playing&&["Space","ArrowUp","ArrowDown","ArrowLeft","ArrowRight","KeyW","KeyA","KeyS","KeyD"].includes(e.code))e.preventDefault();
 if(e.code==="F3"){e.preventDefault();if(window.WP3DProbe)window.WP3DProbe.toggle();return;}
 if(e.repeat)return;
 if(e.code==="KeyR")reload();
 if(e.code==="KeyV")heal();
 if(e.code==="KeyG")grenade();
 if(e.code==="KeyQ")swap();
 if(e.code==="KeyX")orderSquad();
 if(e.code==="KeyT")callArtillery();
 if(e.code==="KeyC")player.crouch=!player.crouch;
 if(e.code==="Escape"){input.fire=false;setAim(false);if(document.pointerLockElement)document.exitPointerLock();}
});
document.addEventListener("keyup",e=>{input.keys[e.code]=false;});
document.addEventListener("mousemove",e=>{
 if(document.pointerLockElement!==canvas||!game.playing)return;
 player.yaw+=e.movementX*.0024*(player.aim?.54:1);
 player.pitch=Math.min(.68,Math.max(-.68,player.pitch-e.movementY*.0021*(player.aim?.60:1)));
});
canvas.addEventListener("contextmenu",e=>e.preventDefault());
canvas.addEventListener("pointerdown",e=>{
 if(!game.playing||e.pointerType==="touch")return;
 if(e.button===2){setAim(true);return;}
 if(e.button===0){
  input.fire=true;
  if(canvas.requestPointerLock)try{canvas.requestPointerLock();}catch{}
 }
});
document.addEventListener("pointerup",e=>{
 if(e.button===0&&e.pointerType!=="touch")input.fire=false;
 if(e.button===2)setAim(false);
});
window.addEventListener("blur",()=>{input.keys={};input.fire=false;setAim(false);input.joyX=0;input.joyY=0;});
document.addEventListener("visibilitychange",()=>{
 if(document.hidden){input.fire=false;setAim(false);input.joyX=0;input.joyY=0;}
});
const stick=$("left-stick"),nub=$("nub"),look=$("right-look");
function updateStick(e){
 const r=stick.getBoundingClientRect(),x=r.left+r.width/2,y=r.top+r.height/2;
 const dx=e.clientX-x,dy=e.clientY-y,mag=Math.hypot(dx,dy)||1;
 const radius=Math.min(r.width,r.height)*.36,weight=Math.min(1,mag/radius);
 input.joyX=dx/mag*weight;input.joyY=dy/mag*weight;
 nub.style.transform="translate("+(input.joyX*radius)+"px,"+(input.joyY*radius)+"px)";
}
stick.addEventListener("pointerdown",e=>{
 if(input.movePointer!==null)return;
 input.movePointer=e.pointerId;stick.setPointerCapture(e.pointerId);updateStick(e);
});
stick.addEventListener("pointermove",e=>{if(e.pointerId===input.movePointer)updateStick(e);});
function releaseStick(e){
 if(e.pointerId!==input.movePointer)return;
 input.movePointer=null;input.joyX=0;input.joyY=0;nub.style.transform="";
}
stick.addEventListener("pointerup",releaseStick);stick.addEventListener("pointercancel",releaseStick);
look.addEventListener("pointerdown",e=>{
 if(input.lookPointer!==null)return;
 input.lookPointer=e.pointerId;input.lookX=e.clientX;input.lookY=e.clientY;
 look.setPointerCapture(e.pointerId);
});
look.addEventListener("pointermove",e=>{
 if(e.pointerId!==input.lookPointer||!game.playing)return;
 const dx=Math.max(-70,Math.min(70,e.clientX-input.lookX));
 const dy=Math.max(-70,Math.min(70,e.clientY-input.lookY));
 input.lookX=e.clientX;input.lookY=e.clientY;
 const scale=player.aim?.005:.009;
 player.yaw+=dx*scale;
 player.pitch=Math.min(.68,Math.max(-.68,player.pitch-dy*scale*.82));
});
function releaseLook(e){if(e.pointerId===input.lookPointer)input.lookPointer=null;}
look.addEventListener("pointerup",releaseLook);look.addEventListener("pointercancel",releaseLook);
function button(id,on,off){
 const el=$(id);el.addEventListener("pointerdown",e=>{
  if(!game.playing)return;
  e.preventDefault();el.setPointerCapture(e.pointerId);on();
 });
 if(off){
  el.addEventListener("pointerup",off);el.addEventListener("pointercancel",off);
 }
}
button("fire",()=>{input.fire=true;},()=>{input.fire=false;});
button("aim",()=>setAim(true),()=>setAim(false));
button("reload",reload);button("grenade",grenade);button("heal",heal);button("swap",swap);
button("orders",orderSquad);button("support",callArtillery);
button("crouch",()=>{player.crouch=!player.crouch;});
$("performance-download").addEventListener("click",()=>window.WP3DProbe?.exportReport());
$("performance-close").addEventListener("click",()=>window.WP3DProbe?.toggle());
// Register the root offline cache from the 3D subdirectory too. The
// existing PWA service worker will cache these assets after first use.
if("serviceWorker" in navigator && location.protocol!=="file:"){
  navigator.serviceWorker.register("../sw.js").catch(()=>{});
}
$("deploy").addEventListener("click",reset);
$("sound-toggle").addEventListener("click",()=>{
 const sound=window.WPBattleSound;
 if(!sound)return;
 const next=!sound.state().muted;
 sound.setMuted(next);
 $("sound-toggle").textContent=next?"SOUND: OFF":"SOUND: ON";
 $("sound-toggle").setAttribute("aria-pressed",String(next));
});
$("quality").value=/Mobi|Android|iPhone|iPad/i.test(navigator.userAgent||"")?"low":"medium";
game.quality=$("quality").value;
spawnEnemies();resize();hud();loadLicensedScenery();loadSkinnedCharacters();
// Lightweight read-only testability hooks (not used for normal gameplay).
window.WP3DDiagnostics=()=>({
 sector:game.sector,alive:game.enemies.filter(e=>e.hp>0).length,
 playerHp:player.hp,weapon:player.weapon,renderSize:[canvas.width,canvas.height],
 renderer:"WebGL2",geometryTriangles:staticMesh.count/3,
  assetInstances:scenery.instances,assetTriangles:scenery.triangles,assetsLoaded:scenery.loaded,
  assetError:scenery.error,texturesLoaded:!!materialTextures,photoMaterialsLoaded:photographicMaterials.loaded,
  photoMaterialsError:photographicMaterials.error,rigLoaded:rig.ready,rigError:rig.error,
  skinActors:rig.skinnedLastFrame,skinTriangles:rig.skinnedTriangles,buildingsLoaded:scenery.buildings||0,
  farmError:scenery.farmError||"",
  alliesAlive:game.allies.filter(a=>a.hp>0).length,
  difficulty:game.difficulty,squadOrder:game.order,support:game.support,
  activeTracers:game.tracers.length,
  position:{x:player.x,z:player.z,yaw:player.yaw,pitch:player.pitch},
  playing:game.playing,elapsedSeconds:game.elapsed,framesPerSecond:game.fps,
  performance:window.WP3DProbe?window.WP3DProbe.report().performance:null,
  input:{keys:Object.keys(input.keys).filter(k=>input.keys[k]),joyX:input.joyX,joyY:input.joyY}
});
// Isolated headless test entrypoint. Only available when explicitly requested.
// Executes the *same* player, collision, ballistics and AI simulation without
// depending on a GPU's frame scheduler. Never exposed by ordinary play URLs.
if(typeof location!=="undefined"&&typeof location.search==="string"&&
   (location.search.startsWith("?qa=1")||location.search.includes("&qa=1"))){
 window.WP3DQA=Object.freeze({
  step(ticks=60){
   if(!Number.isInteger(ticks)||ticks<1||ticks>600)throw Error("QA ticks must be 1..600");
   for(let i=0;i<ticks&&game.playing;i++)simulate(1/60);
   return {x:player.x,z:player.z,elapsed:game.elapsed,playing:game.playing,
     input:{...input.keys},shots:window.WP3DProbe?.report().gameplay.shots||0};
  }
 });
}
requestAnimationFrame(frame);
})();