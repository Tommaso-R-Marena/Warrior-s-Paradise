/* Warrior's Paradise: opt-in local telemetry, measured frame & gameplay statistics.
   Never uploads usage data. All reports may be exported to JSON by the player. */
(function(root){"use strict";
 const maxSamples=1200, frames=[],renders=[],cpu=[],events=[];
 let activeCapture=false,captureStarted=0,lastCaptureMs=0,renderConfiguration={};
 let distance=0,blockedSeconds=0,attemptSeconds=0,shots=0,hits=0,
    objectives=0,medkits=0,restarts=0,frameCounter=0,sessionStart=Date.now(),
    posStart=null,lastPos=null,visible=false;
 const at=()=>typeof performance!=="undefined"&&performance.now?performance.now():Date.now();
 const n=(x)=>Number.isFinite(x)?Math.round(x*100)/100:0;
 const sorted=a=>a.slice().sort((a,b)=>a-b);
 const pct=(a,p)=>a.length?sorted(a)[Math.min(a.length-1,Math.floor((a.length-1)*p))]:0;
 const avg=a=>a.length?a.reduce((t,v)=>t+v,0)/a.length:0;
 function push(target,value){
  if(!Number.isFinite(value)||value<0)return;
  target.push(value);
  if(target.length>maxSamples)target.shift();
 }
 function reset(){
  frames.length=0;renders.length=0;cpu.length=0;events.length=0;
  distance=0;blockedSeconds=0;attemptSeconds=0;shots=0;hits=0;kills=0;
  objectives=0;medkits=0;frameCounter=0;restarts++;sessionStart=Date.now();
  posStart=null;lastPos=null;
 }
 function movement(x,z,requested,speed,dt){
  if(!posStart)posStart={x,z};
  if(lastPos){const step=Math.hypot(x-lastPos.x,z-lastPos.z);distance+=step;
   if(requested>.05){attemptSeconds+=dt;if(step<.007)blockedSeconds+=dt;}
  }
  lastPos={x,z};
 }
 function event(type){
  if(type==="shot")shots++;
  if(type==="hit")hits++;
  if(type==="kill")kills++;
  if(type==="capture")objectives++;
  if(type==="heal")medkits++;
  if(events.length>=80)events.shift();
  events.push({elapsedMs:Math.round(at()),type});
 }
 function frame(ms,renderMs,cpuMs){
  if(activeCapture&&Date.now()-captureStarted>=30000){
   activeCapture=false;lastCaptureMs=Date.now()-captureStarted;
  }
  push(frames,ms);push(renders,renderMs);push(cpu,cpuMs);frameCounter++;
  if(visible&&frameCounter%20===0)refresh();
 }
 function configure(settings){
  if(!settings||typeof settings!=="object")return;
  const {quality,width,height,scaler}=settings;
  if(!["low","medium","high"].includes(quality)||!Number.isFinite(width)||!Number.isFinite(height))return;
  renderConfiguration={quality,width:Math.round(width),height:Math.round(height),
   pixels:Math.round(width*height),adaptiveScale:Number.isFinite(scaler)?n(scaler):null};
 }
 function startCapture(){
  frames.length=0;renders.length=0;cpu.length=0;frameCounter=0;
  root.WPDeviceGpuCaptureController?.reset?.();
  captureStarted=Date.now();lastCaptureMs=0;activeCapture=true;refresh();
 }
 function report(){
  const avgFrame=avg(frames),p99=pct(frames,.99);
  const onePercent=frames.length?1000/Math.max(.01,avg(sorted(frames).slice(-Math.max(1,Math.ceil(frames.length*.01))))):0;
  return {version:1,source:"WarriorsParadise WebGL2",generatedAt:new Date().toISOString(),
    sampling:{frames:frames.length,uptimeSeconds:n((Date.now()-sessionStart)/1000),warmupIncluded:true},
    performance:{avgFps:n(avgFrame?1000/avgFrame:0),onePercentLowFps:n(onePercent),
      p50FrameMs:n(pct(frames,.50)),p95FrameMs:n(pct(frames,.95)),p99FrameMs:n(p99),
      maxFrameMs:n(pct(frames,1)),avgRenderCpuMs:n(avg(renders)),p95RenderCpuMs:n(pct(renders,.95)),
      avgGameLoopCpuMs:n(avg(cpu))},
    gameplay:{metersTraveled:n(distance),blockedSeconds:n(blockedSeconds),
      attemptedMovementSeconds:n(attemptSeconds),shots,hits,accuracyPercent:n(shots?100*hits/shots:0),
      kills,objectivesCaptured:objectives,medkitsUsed:medkits,restarts,
      startPosition:posStart,latestPosition:lastPos},
    gpu:root.WPDeviceGpuCaptureController?.report?.()||{supported:false,reason:"WebGL2 GPU timer unavailable"},
    renderConfiguration:{...renderConfiguration},
    capture:{active:activeCapture,requestedSeconds:30,elapsedSeconds:n(activeCapture?
      (Date.now()-captureStarted)/1000:lastCaptureMs/1000),sampledFrames:frames.length,
      deviceRun:true,autoUploaded:false},
    hardware:{userAgent:typeof navigator!=="undefined"?navigator.userAgent:"unknown",
      devicePixelRatio:root.devicePixelRatio||1,cores:typeof navigator!=="undefined"?navigator.hardwareConcurrency||null:null},
    recentEvents:events.slice(-15)};
 }
 function refresh(){
  const el=typeof document!=="undefined"?document.getElementById("performance-details"):null;
  if(!el)return;
  const r=report(),p=r.performance,g=r.gameplay;
  el.textContent=[
    "MEASURED: "+p.avgFps+" FPS average / "+p.onePercentLowFps+" 1% low",
    "Frames: p50 "+p.p50FrameMs+" ms · p95 "+p.p95FrameMs+" ms · p99 "+p.p99FrameMs+" ms",
    "Render CPU: "+p.avgRenderCpuMs+" ms avg / "+p.p95RenderCpuMs+" ms p95",
    "Samples: "+r.sampling.frames+" · Moved: "+g.metersTraveled+" m",
    "Movement blocked: "+g.blockedSeconds+" s of "+g.attemptedMovementSeconds+" s",
    "Shots "+g.shots+" · Hits "+g.hits+" · Eliminations "+g.kills+" · Captures "+g.objectivesCaptured,
    "GPU: "+(r.gpu.supported?(r.gpu.meanGpuMs??"pending")+"ms mean, "+
      (r.gpu.p95GpuMs??"pending")+"ms p95 ("+r.gpu.queries+" samples)":"GPU timer unsupported"),
    "Device capture: "+(r.capture.active?"RECORDING "+r.capture.elapsedSeconds+"s / 30s":
      r.capture.elapsedSeconds?"COMPLETE ("+r.capture.elapsedSeconds+"s)":"not yet captured"),
    "Local-only benchmark · No telemetry uploaded"
  ].join("\n");
 }
 function toggle(){
  const el=document.getElementById("performance-panel");if(!el)return;
  visible=!visible;el.hidden=!visible;refresh();
 }
 function exportReport(){
  const data=JSON.stringify(report(),null,2);
  const blob=new Blob([data],{type:"application/json"});
  const url=URL.createObjectURL(blob),a=document.createElement("a");
  a.href=url;a.download="warriors-paradise-benchmark.json";
  document.body.appendChild(a);a.click();a.remove();
  setTimeout(()=>URL.revokeObjectURL(url),1000);
 }
 root.WP3DProbe=Object.freeze({frame,movement,event,report,reset,toggle,configure,startCapture,refresh,exportReport,now:at});
})(window);
