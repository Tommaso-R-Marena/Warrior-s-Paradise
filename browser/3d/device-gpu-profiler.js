/* Hardware GPU timing: opt-in local Chrome/Firefox/Android WebGL2 disjoint queries.
 * Does not assume a GPU extension exists and does not call gl.finish().
 * All samples remain on the user's device until explicitly exported.
 */
(function(root){"use strict";
function create(gl){
 const ext=gl?.getExtension?.("EXT_disjoint_timer_query_webgl2")||null;
 const samples=[],MAX=160,pending=[];
 let active=null,frames=0,rejected=0,disjoint=0,errors=0;
 let reason=ext?"":"GPU timestamp extension unavailable";
 const time=()=>typeof performance!=="undefined"&&performance.now?performance.now():Date.now();
 const mean=arr=>arr.length?arr.reduce((a,b)=>a+b,0)/arr.length:0;
 const pct=(a,t)=>a.length?a.slice().sort((x,y)=>x-y)[Math.floor((a.length-1)*t)]:null;
 function cleanup(query){try{gl.deleteQuery(query)}catch{errors++;}}
 function poll(){
  if(!ext||!gl.getParameter||!gl.getQueryParameter)return;
  let isDisjoint=false;
  try{isDisjoint=!!gl.getParameter(ext.GPU_DISJOINT_EXT)}catch(e){errors++;reason=String(e);return;}
  if(isDisjoint){
   disjoint++;if(active){try{gl.endQuery(ext.TIME_ELAPSED_EXT)}catch{}cleanup(active);active=null;}
   while(pending.length)cleanup(pending.shift().query);
   return;
  }
  for(let i=pending.length-1;i>=0;i--){
   const rec=pending[i];let ready=false;
   try{ready=!!gl.getQueryParameter(rec.query,gl.QUERY_RESULT_AVAILABLE)}catch{errors++;}
   if(!ready)continue;
   try{
    const ns=gl.getQueryParameter(rec.query,gl.QUERY_RESULT);
    if(Number.isFinite(ns)&&ns>=0&&ns<1e10){
     samples.push({ms:Math.round(ns/1e4)/100,at:Math.round(rec.at)});
     if(samples.length>MAX)samples.shift();
    }else rejected++;
   }catch{errors++;}
   cleanup(rec.query);pending.splice(i,1);
  }
 }
 function begin(){
  frames++;if(!ext||!gl.createQuery||!gl.beginQuery)return false;
  poll();
  if(active||pending.length>=3||frames%12!==0)return false;
  try{active=gl.createQuery();gl.beginQuery(ext.TIME_ELAPSED_EXT,active);return true;}
  catch(e){errors++;reason=String(e);if(active)cleanup(active);active=null;return false;}
 }
 function end(){
  if(!active)return;
  const query=active;active=null;
  try{gl.endQuery(ext.TIME_ELAPSED_EXT);pending.push({query,at:time()});}
  catch(e){errors++;reason=String(e);cleanup(query);}
 }
 function report(){
  poll();
  const ms=samples.map(x=>x.ms);
  const p95=pct(ms,.95);
  let renderer=null,vendor=null,softwareLikely=false;
  try{
   const rex=gl.getExtension?.("WEBGL_debug_renderer_info");
   vendor=rex?gl.getParameter(rex.UNMASKED_VENDOR_WEBGL):gl.getParameter?.(gl.VENDOR)||null;
   renderer=rex?gl.getParameter(rex.UNMASKED_RENDERER_WEBGL):gl.getParameter?.(gl.RENDERER)||null;
   softwareLikely=/swiftshader|llvmpipe|software|microsoft basic/i.test(String(renderer));
  }catch{}
  return {supported:!!ext,reason:reason||null,queries:ms.length,pending:pending.length,
   meanGpuMs:ms.length?Math.round(mean(ms)*100)/100:null,p95GpuMs:p95,
   maxGpuMs:pct(ms,1),gpuDisjointEvents:disjoint,rejected,errors,
   renderer,vendor,softwareLikely,samples:ms.slice(-75)};
 }
 function reset(){samples.length=0;rejected=0;disjoint=0;errors=0;}
 function dispose(){if(active){try{gl.endQuery(ext.TIME_ELAPSED_EXT)}catch{}cleanup(active);active=null;}
  while(pending.length)cleanup(pending.shift().query);}
 return Object.freeze({begin,end,report,reset,dispose});
}
root.WPDeviceGpuProfiler=Object.freeze({create});
})(window);
