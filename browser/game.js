"use strict";
// Warrior's Paradise Field Edition: self-contained CPU raycaster, no libraries.
// This is a separate browser skirmish. It is not a compiled Unreal build.
(() => {
  const $ = (id) => document.getElementById(id);
  const canvas = $("world"), ctx = canvas.getContext("2d", { alpha: false });
  const radar = $("radar"), radarCtx = radar.getContext("2d");
  const visuals = window.WPVisuals || null;
  const dom = {
    modal: $("modal"), start: $("start-btn"), title: $("modal-title"),
    battlefield: $("battlefield"), difficulty: $("difficulty"),
    copy: $("modal-copy"), quality: $("quality"), qualityLabel: $("quality-label"),
    health: $("health"), ammo: $("ammo"), medkits: $("medkits"),
    grenades: $("grenades"), loadout: $("loadout"), aimRing: $("aim-ring"), crosshair: $("crosshair"),
    mission: $("mission"), progress: $("progress"), squad: $("squad"),
    hit: $("hitmarker"), damage: $("damage-flash"), tip: $("tip")
  };
  const coarse = matchMedia("(pointer: coarse)").matches;
  if (coarse) dom.quality.value = "eco";
  else if (navigator.hardwareConcurrency && navigator.hardwareConcurrency <= 4) dom.quality.value = "eco";
  else dom.quality.value = "balanced";

  const WEAPONS = Object.freeze({
    rifle: { name: "LEE-ENFIELD", capacity: 10, interval: .63, reload: 2.05, damage: 59, range: 16, spread: .015, sound: 145 },
    smg: { name: "STEN MK II", capacity: 32, interval: .095, reload: 1.7, damage: 18, range: 9.5, spread: .052, sound: 195 },
    lmg: { name: "BREN", capacity: 30, interval: .14, reload: 2.85, damage: 32, range: 14, spread: .036, sound: 118 }
  });
  const MAP_W = 28, MAP_H = 28;
  const tiles = new Uint8Array(MAP_W * MAP_H);
  let objectives = [
    { x: 7.5, y: 6.5, name: "FORWARD FARM", description: "CLEAR THE OUTBUILDINGS" },
    { x: 15.5, y: 14.5, name: "CROSSROADS", description: "SECURE THE ROAD JUNCTION" },
    { x: 23.2, y: 22.2, name: "RADIO POST", description: "SILENCE THE ENEMY RADIO" }
  ];
  let buildings = [
    [8, 2, 3, 2, 1], [4, 9, 3, 3, 1], [11, 8, 4, 2, 1],
    [19, 5, 3, 3, 1], [19, 13, 4, 3, 1], [8, 19, 4, 3, 1],
    [15, 21, 3, 3, 1], [3, 17, 2, 4, 2], [24, 8, 2, 4, 2],
    [10, 25, 4, 2, 2]
  ];
  const clamp = (x, a, b) => Math.max(a, Math.min(b, x));
  const dist2 = (a, b) => Math.hypot(a.x - b.x, a.y - b.y);
  const wall = (x, y) => x < 0 || y < 0 || x >= MAP_W || y >= MAP_H || tiles[(y | 0) * MAP_W + (x | 0)] !== 0;
  const tile = (x, y) => x < 0 || y < 0 || x >= MAP_W || y >= MAP_H ? 1 : tiles[(y | 0) * MAP_W + (x | 0)];
  const random = (a, b) => a + Math.random() * (b - a);
  function populateMap() {
    tiles.fill(0);
    for (let y = 0; y < MAP_H; y++)
      for (let x = 0; x < MAP_W; x++)
        if (!x || !y || x === MAP_W - 1 || y === MAP_H - 1) tiles[y * MAP_W + x] = 1;
    for (const [x, y, w, h, type] of buildings)
      for (let j = y; j < y + h; j++)
        for (let i = x; i < x + w; i++) tiles[j * MAP_W + i] = type;
    // Obstacles on the edge of paths, with broad navigable corridors.
    for (const [x, y] of [[3,6],[5,5],[12,5],[16,8],[8,13],[13,18],[21,18],[24,24],[6,23]]) {
      if (!wall(x,y)) tiles[y * MAP_W + x] = 2;
    }
  }
  populateMap();
  // Decorative billboards: rendered locally and excluded from collision logic.
  let scenery = [
    [2.5,7.6,"tree",1.9],[4.1,13.5,"tree",1.8],
    [16.1,3.0,"tree",2.1],[23.4,3.6,"tree",2.0],
    [2.6,22.5,"tree",1.9],[6.1,25.0,"tree",1.6],
    [25.1,17.6,"tree",1.7],[17.5,18.1,"tree",2.1],
    [11.8,16.8,"tree",1.8],[25.0,25.6,"tree",2.0],
    [7.7,8.1,"crate",.70],[17.8,13.5,"crate",.70],
    [21.5,21.2,"crate",.78],[11.0,6.5,"crate",.62]
  ].filter(([x,y])=>!wall(x,y))
   .map(([x,y,kind,size])=>({x,y,kind,size}));
  const MAPS = {
    farm: {
      label:"NORMANDY FARM",
      objectives:objectives.map(o=>({...o})),
      buildings:buildings.map(b=>b.slice()),
      scenery:scenery.map(o=>({...o})),
      defenders:[
        [[8.8,6.4],[6.7,7.5],[10.4,5.7],[7.1,4.6]],
        [[13.5,13],[17.2,14],[16.2,16.5],[13.4,16.7]],
        [[21.3,21.1],[24.8,20.8],[23.8,24],[20.4,23.7],[25.0,23.2]]
      ]
    },
    town: {
      label:"OCCUPIED TOWN",
      objectives:[
        {x:7.3,y:6.5,name:"CHAPEL ROAD",description:"CLEAR THE APPROACH"},
        {x:15.5,y:14.5,name:"CENTRAL SQUARE",description:"SECURE THE CROSSROADS"},
        {x:22.5,y:21.5,name:"GARRISON",description:"CAPTURE THE COMMAND POST"}
      ],
      buildings:[
        [8,2,4,3,1],[3,9,3,3,1],[11,8,3,3,1],
        [19,4,3,4,1],[19,13,4,3,1],[8,18,3,3,1],
        [15,20,4,4,1],[3,18,3,4,2],[24,8,2,4,2],
        [11,25,5,2,2],[24,23,2,3,1]
      ],
      scenery:[
        [6.6,5.2,"crate",.8],[17.1,11.7,"crate",.75],
        [21.2,19.6,"crate",.8],[2.2,13.9,"tree",1.6],
        [17.5,5.7,"tree",1.5],[6.3,23.7,"tree",1.8],
        [24.2,17.1,"tree",1.7],[13.5,18.6,"crate",.75]
      ],
      defenders:[
        [[6.6,7.2],[9.3,6.6],[10.4,7.2],[7.2,4.5]],
        [[13.5,12.6],[17.5,14.4],[16.6,17.3],[13.6,16.4]],
        [[21.1,21.2],[23.6,20.5],[22.5,24.7],[20.2,24.1],[25.5,21.6]]
      ]
    },
    forest: {
      label:"FOREST OUTPOST",
      objectives:[
        {x:7.2,y:7.2,name:"HILL APPROACH",description:"CLEAR THE WOODLINE"},
        {x:14.5,y:14.5,name:"AMMO DEPOT",description:"CAPTURE THE SUPPLY POINT"},
        {x:23.3,y:22.3,name:"FOREST RADIO",description:"CUT ENEMY COMMUNICATIONS"}
      ],
      buildings:[
        [8,3,2,2,2],[5,10,2,2,2],[12,8,3,2,1],
        [19,6,3,3,2],[19,14,3,3,2],[8,19,2,3,2],
        [15,21,2,2,2],[3,18,2,4,2],[24,9,2,3,1],[11,25,3,2,2]
      ],
      scenery:[
        [2.5,6.8,"tree",2.0],[5.6,5.2,"tree",2.2],
        [11.5,3.2,"tree",1.9],[15.5,5.6,"tree",2.1],
        [24.8,5.5,"tree",2.3],[3.5,14.2,"tree",2.2],
        [8.5,14.5,"tree",2.2],[17.3,11.3,"tree",1.8],
        [25.4,14.8,"tree",2.0],[5.5,24.8,"tree",2.2],
        [14.7,19.1,"tree",2.0],[19.4,20.6,"tree",2.2],
        [25.1,25.5,"tree",2.0],[10.5,16.8,"crate",.72],
        [18.2,18.4,"crate",.70],[21.4,24.2,"crate",.72]
      ],
      defenders:[
        [[6.6,7.4],[9.8,6.6],[10.7,7.4],[7.2,4.7]],
        [[13.4,12.7],[16.4,14.8],[16.2,17.1],[12.8,15.4]],
        [[22.1,21.5],[24.8,21.2],[23.5,24.5],[20.6,23.9],[25.2,23.2]]
      ]
    }
  };
  function setBattlefield(key) {
    const preset=MAPS[key]||MAPS.farm;
    game.battlefield=MAPS[key]?key:"farm";
    objectives=preset.objectives.map(o=>({...o}));
    buildings=preset.buildings.map(b=>b.slice());
    populateMap();
    scenery=preset.scenery.filter(o=>!wall(o.x,o.y)).map(o=>({...o}));
    game.defenderSpawns=preset.defenders.map(group=>group.map(pos=>pos.slice()));
  }
  function spawnAt(x, y, team, role) {
    return { x, y, team, role, hp: team === "enemy" ? 74 : 100, maxHp: team === "enemy" ? 74 : 100,
      angle: .6, alive: true, cooldown: random(.2, 1.2), flash: 0, hit: 0,
      weapon: role === "ASSAULT" ? "smg" : role === "MEDIC" ? "lmg" : "rifle",
      ammo: role === "ASSAULT" ? 32 : role === "MEDIC" ? 30 : 10,
      reserve: team === "enemy" ? 36 : role === "ASSAULT" ? 128 : role === "MEDIC" ? 90 : 70,
      reload: 0, ads: false, crouch: false, medkits: team === "enemy" ? 0 : 2,
      frags: team === "enemy" ? 0 : 2, smokes: team === "enemy" ? 0 : 1,
      strafedir: Math.random() < .5 ? 1 : -1,
      suppression: 0, memory: 0, target: null, pathTimer: 0, pathNode: null };
  }
  const game = {
    phase: "menu", player: null, friends: [], enemies: [], objects: [],
    objective: 0, cap: 0, kills: 0, time: 0, screenW: 288, screenH: 170,
    ord: "FOLLOW", thrown: [], particles: [],
    battlefield: "farm", difficulty: "regular", defenderSpawns: [],
    shake: 0, flash: 0, hitTime: 0, message: "", messageTimer: 0,
    quality: dom.quality.value, fps: 30, fpsTime: 0, lastFrame: 0, raf: 0
  };
  const input = { keys: Object.create(null), fire: false, joyX: 0, joyY: 0, lookX: 0 };
  let audio = null;
  function beep(freq, duration, strength = .05, type = "sawtooth") {
    try {
      if (!audio) audio = new (window.AudioContext || window.webkitAudioContext)();
      if (audio.state === "suspended") audio.resume();
      const o = audio.createOscillator(), gain = audio.createGain();
      o.type = type; o.frequency.setValueAtTime(freq, audio.currentTime);
      o.frequency.exponentialRampToValueAtTime(Math.max(45, freq * .4), audio.currentTime + duration);
      gain.gain.setValueAtTime(strength, audio.currentTime);
      gain.gain.exponentialRampToValueAtTime(.001, audio.currentTime + duration);
      o.connect(gain); gain.connect(audio.destination); o.start(); o.stop(audio.currentTime + duration);
    } catch { /* Silent mode is safe and supported. */ }
  }
  function notify(message, duration = 3) {
    game.message = message;
    game.messageTimer = duration;
  }
  function makeSoldiers() {
    const squad = [
      spawnAt(3.5,3.4,"ally","RIFLEMAN"),
      spawnAt(2.6,3.8,"ally","ASSAULT"),
      spawnAt(3.0,2.7,"ally","MEDIC")
    ];
    for (const s of squad) s.angle = .65;
    game.friends = squad;
    game.player = squad[0];
    game.enemies = [];
    for (let i = 0; i < 3; i++) spawnDefenders(i);
  }
  function spawnDefenders(stage) {
    for (const [x,y] of game.defenderSpawns[stage]||[]) {
      if (!wall(x,y)) {
        const e=spawnAt(x,y,"enemy",stage===2?"DEFENDER":"INFANTRY");
        const hpScale=game.difficulty==="veteran"?1.25:game.difficulty==="recruit"?.76:1;
        e.hp=(e.hp+stage*10)*hpScale;e.maxHp=e.hp;
        game.enemies.push(e);
      }
    }
  }
  function reset() {
    game.difficulty=dom.difficulty.value;
    setBattlefield(dom.battlefield.value);
    game.phase="playing"; game.objective=0;game.cap=0;
    game.kills=0;game.time=0;game.shake=0;game.hitTime=0;game.flash=0;
    game.objects=[];game.thrown=[];game.particles=[];game.ord="FOLLOW";
    makeSoldiers();
    notify(MAPS[game.battlefield].label+" • SECURE SECTOR ONE",4);
    dom.modal.classList.remove("shown");
    dom.aimRing.classList.remove("aimed");dom.crosshair.classList.remove("aimed");
    resizeRender();
    updateHud();
    try { if (!coarse && canvas.requestPointerLock) canvas.requestPointerLock(); } catch {}
  }
  function gameEnd(win) {
    game.phase = win ? "won" : "lost"; input.fire = false;
    dom.aimRing.classList.remove("aimed");dom.crosshair.classList.remove("aimed");
    document.exitPointerLock?.();
    dom.modal.classList.add("shown");
    dom.title.innerHTML = win ? "MISSION<br><em>COMPLETE</em>" : "SQUAD<br><em>LOST</em>";
    dom.copy.textContent = win
      ? "All three sectors secure. " + game.kills + " enemies neutralized. Your team held the line."
      : "The squad has been eliminated. Redeploy and try a different approach.";
    dom.start.innerHTML = "REDEPLOY <span>→</span>";
    beep(win ? 640 : 95, .42, .07, win ? "triangle" : "sawtooth");
  }
  function slide(s, dx, dy) {
    const r=.22;
    const ok=(x,y)=>!wall(x-r,y-r)&&!wall(x+r,y-r)&&!wall(x-r,y+r)&&!wall(x+r,y+r);
    if (ok(s.x+dx,s.y)) s.x+=dx;
    if (ok(s.x,s.y+dy)) s.y+=dy;
  }
  // Grid waypoint planner: cheap BFS with short caching per soldier.
  // Direct motion is preferred; BFS only runs when a wall blocks it.
  function moveSmart(s,to,speed,dt) {
    if(!s||!to||!s.alive)return;
    const d=dist2(s,to);if(d<.08)return;
    const dx=(to.x-s.x)/d,dy=(to.y-s.y)/d;
    const forward=trace(s.x,s.y,dx,dy,Math.min(d,3)).distance;
    if(forward>=Math.min(d,3)-.28) {
      slide(s,dx*speed*dt,dy*speed*dt);
      s.pathNode=null;
      return;
    }
    s.pathTimer=(s.pathTimer||0)-dt;
    const destX=Math.floor(to.x),destY=Math.floor(to.y);
    if(s.pathTimer<=0||!s.pathNode||s.pathGoal!==destY*MAP_W+destX) {
      s.pathTimer=.65+Math.random()*.18;
      s.pathGoal=destY*MAP_W+destX;
      const source=Math.floor(s.y)*MAP_W+Math.floor(s.x);
      const goal=s.pathGoal;
      const queue=new Int16Array(MAP_W*MAP_H),parents=new Int16Array(MAP_W*MAP_H);
      parents.fill(-1);
      let first=0,last=0;queue[last++]=source;parents[source]=source;
      while(first<last){
        const curr=queue[first++];if(curr===goal)break;
        const x=curr%MAP_W,y=(curr/MAP_W)|0;
        for(const [nx,ny] of [[x-1,y],[x+1,y],[x,y-1],[x,y+1]]){
          if(wall(nx+.5,ny+.5))continue;
          const next=ny*MAP_W+nx;
          if(parents[next]!==-1)continue;
          parents[next]=curr;queue[last++]=next;
        }
      }
      if(goal>=0&&goal<parents.length&&parents[goal]!==-1){
        let cell=goal;
        while(parents[cell]!==source&&cell!==source&&parents[cell]!==-1)
          cell=parents[cell];
        s.pathNode={x:cell%MAP_W+.5,y:Math.floor(cell/MAP_W)+.5};
      }else s.pathNode=null;
    }
    if(s.pathNode){
      const node=s.pathNode,dist=dist2(s,node);
      if(dist>.10)slide(s,(node.x-s.x)/dist*speed*dt,(node.y-s.y)/dist*speed*dt);
    }
  }
  // DDA against tile geometry: also used for gameplay line of sight.
  function trace(x,y,dx,dy,maxDist=26) {
    let cx=Math.floor(x),cy=Math.floor(y);
    const stepX=dx<0?-1:1,stepY=dy<0?-1:1;
    const deltaX=dx===0?1e30:Math.abs(1/dx),deltaY=dy===0?1e30:Math.abs(1/dy);
    let sideX=(dx<0?x-cx:cx+1-x)*deltaX,sideY=(dy<0?y-cy:cy+1-y)*deltaY;
    let side=0, travel=0;
    for(let i=0;i<52;i++){
      if(sideX<sideY){travel=sideX;sideX+=deltaX;cx+=stepX;side=0;}
      else{travel=sideY;sideY+=deltaY;cy+=stepY;side=1;}
      if(travel>maxDist) break;
      if(tile(cx,cy)) return {distance:travel,type:tile(cx,cy),side,cx,cy,
        texture:side===0?y+travel*dy:x+travel*dx};
    }
    return {distance:maxDist,type:0,side:0,cx,cy,texture:0};
  }
  const seen=(a,b)=>!wall(b.x,b.y)&&!smokeBlocks(a,b)&&trace(a.x,a.y,(b.x-a.x)/Math.max(.001,dist2(a,b)),(b.y-a.y)/Math.max(.001,dist2(a,b)),dist2(a,b)).distance>=dist2(a,b)-.15;
  function smokeBlocks(a,b) {
    const dx=b.x-a.x,dy=b.y-a.y,len2=dx*dx+dy*dy;
    if(len2<.01)return false;
    for(const cloud of game.thrown){
      if(cloud.kind!=="smoke"||cloud.fuse>0||cloud.life<=0)continue;
      const t=clamp(((cloud.x-a.x)*dx+(cloud.y-a.y)*dy)/len2,0,1);
      if(Math.hypot(a.x+t*dx-cloud.x,a.y+t*dy-cloud.y)<cloud.radius)return true;
    }
    return false;
  }
  function mouseTarget(spread=0) {
    const p=game.player;
    if(!p)return null;
    const direction=p.angle+spread;
    const dx=Math.cos(direction),dy=Math.sin(direction);
    const config=WEAPONS[p.weapon];
    const wallDist=trace(p.x,p.y,dx,dy,config.range).distance;
    let best=null,bestRange=Math.min(config.range,wallDist+.04);
    for(const e of game.enemies){
      if(!e.alive)continue;
      const vx=e.x-p.x,vy=e.y-p.y,forward=vx*dx+vy*dy,offset=Math.abs(vx*dy-vy*dx);
      const assist=coarse&&p.ads?.034:.0;
      if(forward>0&&forward<bestRange&&offset<.20+forward*(p.ads?.002:.005)+forward*assist&&seen(p,e)){
        best=e;bestRange=forward;
      }
    }
    return best;
  }
  function fire() {
    const p=game.player;
    if(!p||!p.alive||game.phase!=="playing"||p.cooldown>0||p.reload>0)return;
    if(p.ammo<=0){reload();notify("MAGAZINE EMPTY • RELOADING",1);return;}
    const w=WEAPONS[p.weapon];
    p.cooldown=w.interval;p.ammo--;p.flash=.10;
    const recoil=w.spread*(p.ads?.27:1);
    const walking=(input.keys.KeyW||input.keys.KeyS||Math.abs(input.joyY)>.12)?.85:0;
    const movementSpread=walking*.027+(p.crouch?-.003:0);
    p.angle+=random(-recoil,recoil)*.12;
    game.shake=p.ads?.035:.068;
    beep(w.sound,.07,.038,"sawtooth");
    const e=mouseTarget(random(-1,1)*Math.max(.003,recoil+movementSpread));
    if(e){
      const distance=dist2(p,e);
      const falloff=clamp(1-Math.max(0,distance-w.range*.5)/w.range*.28,.65,1);
      let damage=w.damage*falloff*random(.87,1.13);
      if(Math.random()<.09)damage*=1.45;
      e.hp-=damage;e.hit=.19;e.suppression=.85;
      game.hitTime=.17;beep(470,.045,.017,"triangle");
      if(e.hp<=0){e.alive=false;game.kills++;notify("TARGET NEUTRALIZED",.7);}
    }else{
      for(const enemy of game.enemies){
        if(enemy.alive&&dist2(p,enemy)<9&&seen(p,enemy))enemy.suppression=Math.max(enemy.suppression,.23);
      }
    }
  }
  function reload() {
    const p=game.player;if(!p||p.reload>0||game.phase!=="playing")return;
    const w=WEAPONS[p.weapon];
    if(p.ammo>=w.capacity||p.reserve<=0)return;
    p.reload=w.reload;
    notify("RELOADING • "+w.name,1.0);
    beep(420,.09,.016,"triangle");
  }
  function finishReload(s) {
    const need=Math.min(WEAPONS[s.weapon].capacity-s.ammo,s.reserve);
    s.ammo+=need;s.reserve-=need;
  }
  function setADS(active) {
    const p=game.player;
    if(!p||(!active&&game.phase!=="playing"&&game.phase!=="paused"))return;
    p.ads=!!active;
    dom.aimRing.classList[p.ads?"add":"remove"]("aimed");
    dom.crosshair.classList[p.ads?"add":"remove"]("aimed");
  }
  function toggleCrouch(){
    if(game.phase!=="playing")return;
    game.player.crouch=!game.player.crouch;
    notify(game.player.crouch?"CROUCH":"STAND",.7);
  }
  function toggleOrders() {
    if(game.phase!=="playing")return;
    game.ord=game.ord==="FOLLOW"?"HOLD":"FOLLOW";
    notify("SQUAD ORDER • "+game.ord,1.7);
  }
  function throwGrenade(kind="frag") {
    const p=game.player;if(!p||game.phase!=="playing"||!p.alive)return;
    if(kind==="smoke"?p.smokes<=0:p.frags<=0){notify("NO "+kind.toUpperCase()+" GRENADES",.9);return;}
    const dx=Math.cos(p.angle),dy=Math.sin(p.angle);
    const wallDist=trace(p.x,p.y,dx,dy,5.5).distance;
    const distance=Math.min(4.7,Math.max(.7,wallDist-.35));
    const point={x:p.x+dx*distance,y:p.y+dy*distance,kind,
      fuse:kind==="frag"?1.45:1.12,life:kind==="smoke"?10.5:.55,
      radius:kind==="smoke"?1.35:2.35};
    if(kind==="smoke")p.smokes--;else p.frags--;
    game.thrown.push(point);
    notify(kind==="smoke"?"SMOKE OUT":"FRAG OUT",.8);
    beep(kind==="smoke"?275:340,.14,.025,"triangle");
  }
  function heal() {
    const p=game.player;if(!p||!p.alive||p.hp>=p.maxHp||p.medkits<=0||game.phase!=="playing")return;
    p.medkits--;p.hp=Math.min(p.maxHp,p.hp+48);notify("FIELD DRESSING APPLIED",1.8);beep(580,.20,.05,"sine");
  }
  function switchSoldier() {
    if(game.phase!=="playing")return;
    const alive=game.friends.filter(s=>s.alive);
    if(alive.length<=1){notify("NO OTHER SQUADMATE AVAILABLE",1.4);return;}
    const idx=alive.indexOf(game.player);
    game.player=alive[(idx+1)%alive.length];
    setADS(false);
    game.player.ads=false;
    dom.aimRing.classList.remove("aimed");
    dom.crosshair.classList.remove("aimed");
    notify("NOW CONTROLLING "+game.player.role+" • "+WEAPONS[game.player.weapon].name,1.7);
    beep(700,.10,.024,"triangle");
  }
  function knockDown(s) {
    s.alive=false;s.hp=0;s.flash=0;
    if(s===game.player){
      const replacement=game.friends.find(f=>f.alive);
      if(replacement){game.player=replacement;notify("SOLDIER DOWN • SQUAD SWITCHED",2.2);}
      else gameEnd(false);
    }
  }
  function update(dt) {
    const p=game.player;
    game.time+=dt;game.hitTime=Math.max(0,game.hitTime-dt);game.shake=Math.max(0,game.shake-dt*2);
    game.flash=Math.max(0,game.flash-dt*3);game.messageTimer=Math.max(0,game.messageTimer-dt);
    for(const g of game.thrown){
      const wasFused=g.fuse>0;
      if(g.fuse>0)g.fuse-=dt;
      if(wasFused&&g.fuse<=0){
        if(g.kind==="frag"){
          beep(74,.24,.09,"sawtooth");
          game.shake=.22;
          for(const soldier of game.enemies.concat(game.friends)){
            if(!soldier.alive)continue;
            const d=dist2(g,soldier);
            if(d>=g.radius||!seen(g,soldier))continue;
            soldier.hp-=(1-d/g.radius)*110;
            soldier.hit=.25;soldier.suppression=.94;
            if(soldier.hp<=0){
              if(soldier.team==="enemy"){soldier.alive=false;game.kills++;}
              else knockDown(soldier);
            }
          }
        }
      }
      if(g.fuse<=0)g.life-=dt;
    }
    game.thrown=game.thrown.filter(g=>g.fuse>0||g.life>0);
    for(const s of game.friends.concat(game.enemies)){
      if(!s.alive)continue;
      s.flash=Math.max(0,s.flash-dt);s.hit=Math.max(0,s.hit-dt);s.suppression=Math.max(0,s.suppression-dt*.1);
      s.cooldown=Math.max(0,s.cooldown-dt);
      if(s.reload>0){s.reload-=dt;if(s.reload<=0){s.reload=0;finishReload(s);}}
    }
    if(!p||!p.alive)return;
    let forward=(input.keys.KeyW||input.keys.ArrowUp?1:0)-(input.keys.KeyS||input.keys.ArrowDown?1:0)-input.joyY;
    let strafe=(input.keys.KeyD?1:0)-(input.keys.KeyA?1:0)+input.joyX;
    const len=Math.hypot(forward,strafe);
    if(len>1){forward/=len;strafe/=len;}
    const speed=(input.keys.ShiftLeft||input.keys.ShiftRight?4.6:3.05)*
      (p.crouch?.51:1)*(p.ads?.78:1)*(p.hp<35?.84:1);
    const ca=Math.cos(p.angle),sa=Math.sin(p.angle);
    slide(p,(ca*forward-sa*strafe)*speed*dt,(sa*forward+ca*strafe)*speed*dt);
    if(input.keys.ArrowLeft)p.angle-=dt*1.65;
    if(input.keys.ArrowRight)p.angle+=dt*1.65;
    p.angle=(p.angle+Math.PI*2)%(Math.PI*2);
    if(input.fire)fire();

    // Followers share the same lightweight simulation. No network/server.
    for(const f of game.friends){
      if(!f.alive||f===p)continue;
      const index=game.friends.indexOf(f),side=index%2?1:-1;
      const wanted={x:p.x-Math.cos(p.angle)*1.15-Math.sin(p.angle)*side*.85,
        y:p.y-Math.sin(p.angle)*1.15+Math.cos(p.angle)*side*.85};
      const d=dist2(f,wanted);
      if(d>.45&&game.ord==="FOLLOW") {
        moveSmart(f,wall(wanted.x,wanted.y)?p:wanted,(d>4?3.7:2.8),dt);
      }
      let target=null,best=9.5;
      for(const e of game.enemies){
        if(!e.alive)continue;
        const dis=dist2(f,e);
        if(dis<best&&seen(f,e)){best=dis;target=e;}
      }
      if(target){f.angle=Math.atan2(target.y-f.y,target.x-f.x);if(f.cooldown<=0&&f.reload===0){
        f.cooldown=random(.65,1.0);f.flash=.09;
        if(Math.random()<.56){target.hp-=random(10,20);target.hit=.1;target.suppression=.3;
          if(target.hp<=0){target.alive=false;game.kills++;}}
      }} else f.angle=p.angle;
    }

    // Enemies search, advance, hold and fire. Suppression alters accuracy.
    for(const e of game.enemies){
      if(!e.alive)continue;
      let target=null,best=14;
      for(const f of game.friends){if(!f.alive)continue;const d=dist2(e,f);if(d<best&&seen(e,f)){best=d;target=f;}}
      if(target){
        e.target=target;e.memory=2;
        const a=Math.atan2(target.y-e.y,target.x-e.x);e.angle=a;
        if(best>4.5&&best<14){
          const speed=e.suppression>.5?0.24:.50;
          moveSmart(e,target,speed,dt);
        }
        if(e.cooldown<=0&&best<11){
          e.cooldown=random(1.05,2.1)*(e.suppression>.6?1.5:1);
          e.flash=.12;
          const accuracy=clamp(.64-best*.028-e.suppression*.35,.08,.57);
          if(Math.random()<accuracy){
            const damage=random(8,16);
            target.hp-=damage;target.hit=.16;
            if(target===p){game.flash=.2;game.shake=.22;beep(72,.12,.035,"triangle");notify("UNDER FIRE!",.65);}
            if(target.hp<=0)knockDown(target);
          }
        }
      } else {
        e.memory=Math.max(0,e.memory-dt);
        const heard=game.friends.filter(f=>f.alive&&dist2(e,f)<8)
          .sort((a,b)=>dist2(e,a)-dist2(e,b))[0];
        if(heard&&!seen(e,heard))moveSmart(e,heard,.56,dt);
      }
    }

    // Capture sectors in order; enemies contest at very short range.
    const o=objectives[game.objective];
    if(o){
      const nearby=dist2(p,o)<2.05;
      const contested=game.enemies.some(e=>e.alive&&dist2(e,o)<2.9);
      if(nearby&&!contested)game.cap=clamp(game.cap+dt/4.3,0,1);
      else game.cap=clamp(game.cap-dt*.18,0,1);
      if(game.cap>=1){
        game.objective++;game.cap=0;
        beep(720,.35,.08,"triangle");
        notify(game.objective>=objectives.length?"ALL SECTORS SECURED":"SECTOR SECURE • ADVANCE",3);
        if(game.objective>=objectives.length){gameEnd(true);return;}
      }
    }
    if(game.messageTimer<=0)game.message="";
    updateHud();
  }
  function updateHud(){
    const p=game.player;if(!p)return;
    dom.health.textContent="HP "+Math.max(0,Math.ceil(p.hp));
    dom.health.style.color=p.hp<30?"#f47b68":"";
    dom.ammo.textContent=String(p.ammo).padStart(2,"0")+" / "+String(p.reserve).padStart(2,"0")+(p.reload>0?" ↻":"");
    dom.medkits.textContent="✚ "+p.medkits;
    dom.grenades.textContent="◉ "+p.frags+" / ▤ "+p.smokes;
    dom.loadout.textContent=WEAPONS[p.weapon].name+" · "+(p.crouch?"CROUCH":"STAND")+" · "+game.ord;
    const o=objectives[game.objective];
    dom.mission.textContent=o?(game.message||"SECURE "+o.name):(game.message||"VICTORY");
    dom.progress.textContent=o?"SECTOR "+(game.objective+1)+" / 3 · "+(dist2(p,o)<2.05?(game.cap*100|0)+"% CAPTURED":o.description):"MISSION COMPLETE";
    dom.squad.textContent="SQUAD · "+game.friends.filter(f=>f.alive).length+" ALIVE · "+game.kills+" KILLS";
    dom.qualityLabel.textContent=game.quality.toUpperCase()+" · "+Math.round(game.fps)+" FPS";
    dom.hit.style.opacity=game.hitTime>0?".95":"0";
    dom.damage.style.opacity=String(game.flash*.8);
  }
  function resizeRender(){
    const width=window.innerWidth, height=window.innerHeight;
    const cap=game.quality==="sharp"?440:game.quality==="balanced"?330:225;
    const ratio=clamp(height/Math.max(1,width),.43,1.8);
    game.screenW=cap;game.screenH=clamp(Math.round(cap*ratio),100,475);
    canvas.width=game.screenW;canvas.height=game.screenH;
    ctx.imageSmoothingEnabled=false;
  }
  let zBuffer=[];
  const shade=(rgb,f)=>"rgb("+rgb.map(v=>Math.round(clamp(v*f,0,255))).join(",")+")";
  function renderWorld() {
    const p=game.player; if(!p)return;
    const w=canvas.width,h=canvas.height,horizon=Math.floor(h*.53);
    if(visuals){
      visuals.setup();
      visuals.background(ctx,w,h,p.angle,game.time);
      visuals.floor(ctx,w,h,p.x,p.y,p.angle,!!p.ads,game.quality);
    }
    else{
      const sky=ctx.createLinearGradient(0,0,0,horizon);
      sky.addColorStop(0,"#51686b");sky.addColorStop(1,"#c0ad80");
      ctx.fillStyle=sky;ctx.fillRect(0,0,w,horizon);
      const ground=ctx.createLinearGradient(0,horizon,0,h);
      ground.addColorStop(0,"#77745b");ground.addColorStop(1,"#29382c");
      ctx.fillStyle=ground;ctx.fillRect(0,horizon,w,h-horizon);
      ctx.fillStyle="#a8a27a43";ctx.fillRect(0,horizon-2,w,5);
    }
    const dirX=Math.cos(p.angle),dirY=Math.sin(p.angle),
      fov=p.ads?.44:.69,planeX=-dirY*fov,planeY=dirX*fov;
    const step=game.quality==="eco"?2:1;
    zBuffer.length=w;
    for(let x=0;x<w;x+=step){
      const cameraX=2*(x+.5)/w-1;
      const rX=dirX+planeX*cameraX,rY=dirY+planeY*cameraX;
      const hit=trace(p.x,p.y,rX,rY,26),depth=Math.max(.14,hit.distance);
      for(let xx=x;xx<x+step&&xx<w;xx++)zBuffer[xx]=depth;
      const height=Math.min(h*3,h/depth*.80),top=Math.round(horizon-height*.5);
      const base=hit.type===2?[119,98,67]:[120,120,106];
      const fog=clamp(1-depth/28,.24,1);
      const factor=(hit.side===1?.73:1)*(.62+.42*fog);
      if(visuals) {
        visuals.wall(ctx,x,top,step,height+1,hit,depth,game.quality);
        continue;
      }
      ctx.fillStyle=shade(base,factor);ctx.fillRect(x,top,step,height+1);
      // Brick/board variation without external textures.
      const textureX=Math.abs(hit.texture%1);
      if(hit.type===1){
        ctx.fillStyle=shade([60,65,59],factor*.78);
        if(Math.abs(textureX-.5)<.038)ctx.fillRect(x,top,step,height);
        const courses=height/4;
        for(let j=1;j<4;j++)ctx.fillRect(x,top+j*courses,step,Math.max(1,height*.012));
      } else if(hit.type===2){
        ctx.fillStyle=shade([56,53,44],factor);
        if(textureX<.14||textureX>.87)ctx.fillRect(x,top,step,height);
        if(height>11)ctx.fillRect(x,top+height*.46,step,Math.max(1,height*.10));
      }
      if(depth<7){
        ctx.fillStyle="rgba(10,17,13,"+(0.10*(1-depth/8))+")";
        ctx.fillRect(x,top+height*.92,step,Math.max(1,height*.08));
      }
    }
    const sprites=scenery.slice();
    for(const g of game.thrown)if(g.fuse<=0&&g.life>0)
      sprites.push({x:g.x,y:g.y,kind:g.kind,size:g.kind==="smoke"?3.1:1.1,life:g.life});
    const o=objectives[game.objective];
    if(o)sprites.push({x:o.x,y:o.y,kind:"objective",size:1.55});
    for(const e of game.enemies)if(e.alive)sprites.push({...e,kind:"enemy",size:.92});
    for(const f of game.friends)if(f.alive&&f!==p)sprites.push({...f,kind:"friend",size:.92});
    sprites.sort((a,b)=>dist2(b,p)-dist2(a,p));
    for(const s of sprites){
      const dx=s.x-p.x,dy=s.y-p.y;
      const inverse=1/(planeX*dirY-dirX*planeY);
      const transformX=inverse*(dirY*dx-dirX*dy);
      const transformY=inverse*(-planeY*dx+planeX*dy);
      if(transformY<=.17||transformY>26)continue;
      const mid=Math.round((w/2)*(1+transformX/transformY));
      const spriteH=Math.abs(h/transformY*s.size*.82);
      const spriteW=s.kind==="objective"?spriteH*.28:(s.kind==="tree"?spriteH*.72:(s.kind==="crate"?spriteH:.43*spriteH));
      const left=Math.round(mid-spriteW/2),right=Math.round(mid+spriteW/2);
      if(right<0||left>=w)continue;
      // This is a low-cost billboard approximation: central depth test
      // prevents most wall-through-sprite rendering.
      if(transformY>zBuffer[clamp(mid,0,w-1)]+.3)continue;
      const baseY=horizon+spriteH*.49,top=baseY-spriteH;
      if(s.kind==="objective"){
        ctx.fillStyle="#1c2824";ctx.fillRect(mid-1,top,2,spriteH);
        ctx.fillStyle="#d5c17d";ctx.fillRect(mid,top,Math.max(3,spriteW),spriteH*.28);
        ctx.fillStyle="#eee0b9";ctx.fillRect(mid+1,top+spriteH*.09,Math.max(2,spriteW*.40),Math.max(2,spriteH*.035));
        continue;
      }
      if(s.kind==="tree"||s.kind==="crate"){
        if(visuals)visuals.decoration(ctx,s,mid,top,spriteW,spriteH);
        continue;
      }
      if(s.kind==="smoke"||s.kind==="frag"){
        const radius=Math.max(2,spriteH*.5);
        const haze=ctx.createRadialGradient(mid,top+spriteH*.52,0,mid,top+spriteH*.52,radius);
        haze.addColorStop(0,s.kind==="smoke"?"rgba(187,195,177,.69)":"rgba(251,195,100,.9)");
        haze.addColorStop(.62,s.kind==="smoke"?"rgba(154,164,152,.47)":"rgba(247,109,55,.55)");
        haze.addColorStop(1,"rgba(126,136,126,0)");
        ctx.fillStyle=haze;ctx.fillRect(mid-radius,top,radius*2,spriteH);
        continue;
      }
      if(visuals){
        visuals.soldier(ctx,s,mid,top,spriteW,spriteH,transformY);
        if(s.kind==="enemy"&&transformY<7){
          ctx.fillStyle="#241d1b";ctx.fillRect(mid-spriteW*.3,top-4,spriteW*.6,2);
          ctx.fillStyle="#e18265";ctx.fillRect(mid-spriteW*.3,top-4,spriteW*.6*Math.max(0,s.hp/s.maxHp),2);
        }
        continue;
      }
      const hostile=s.kind==="enemy",hurt=s.hit>0;
      const body=hurt?"#eedbc4":hostile?"#575b54":"#596e55";
      const coat=hurt?"#e3d2b8":hostile?"#454d47":"#556c57";
      const skin="#bc9d7e";
      // Helmet, head, jacket, limbs and rifle: sprite silhouette.
      ctx.fillStyle=skin;ctx.fillRect(mid-spriteW*.13,top+spriteH*.14,spriteW*.26,spriteH*.17);
      ctx.fillStyle=hostile?"#394441":"#758367";
      ctx.fillRect(mid-spriteW*.25,top+spriteH*.09,spriteW*.50,spriteH*.075);
      ctx.fillStyle=coat;ctx.fillRect(mid-spriteW*.30,top+spriteH*.30,spriteW*.60,spriteH*.40);
      ctx.fillStyle=body;ctx.fillRect(mid-spriteW*.39,top+spriteH*.32,spriteW*.11,spriteH*.37);
      ctx.fillRect(mid+spriteW*.28,top+spriteH*.32,spriteW*.11,spriteH*.37);
      ctx.fillStyle="#272e2a";ctx.fillRect(mid-spriteW*.24,top+spriteH*.68,spriteW*.2,spriteH*.3);
      ctx.fillRect(mid+spriteW*.04,top+spriteH*.68,spriteW*.2,spriteH*.3);
      ctx.fillStyle="#1b2420";ctx.fillRect(mid+spriteW*.10,top+spriteH*.47,spriteW*.53,Math.max(1,spriteH*.045));
      if(s.flash>0){ctx.fillStyle="#f9d98f";ctx.beginPath();ctx.arc(mid+spriteW*.66,top+spriteH*.48,Math.max(2,spriteH*.075),0,6.29);ctx.fill();}
      if(hostile&&transformY<7){
        ctx.fillStyle="#2a1c19";ctx.fillRect(mid-spriteW*.3,top-4,spriteW*.6,2);
        ctx.fillStyle="#d57667";ctx.fillRect(mid-spriteW*.3,top-4,spriteW*.6*(s.hp/s.maxHp),2);
      }
    }
    // Weapon silhouette and muzzle flash at the foreground.
    if(visuals)visuals.weapon(ctx,w,h,p,game.time,!!p.ads);
    else{
    const sway=Math.sin(game.time*7)*((input.keys.KeyW||Math.abs(input.joyY)>.12)?.6:0);
    const wx=w*.59,wy=h*.81+sway;
    ctx.fillStyle="#28332c";ctx.fillRect(wx,wy,w*.18,h*.20);
    ctx.fillStyle="#596050";ctx.fillRect(wx+w*.07,wy-h*.06,w*.1,h*.13);
    ctx.fillStyle="#272a26";ctx.fillRect(wx+w*.085,wy-h*.08,w*.025,h*.15);
    ctx.fillStyle="#292721";ctx.fillRect(wx+w*.16,wy+h*.055,w*.16,h*.10);
    if(p.flash>0){
      ctx.fillStyle="#f5d792";ctx.beginPath();ctx.moveTo(wx+w*.11,wy-h*.09);
      ctx.lineTo(wx+w*.04,wy-h*.25);ctx.lineTo(wx+w*.14,wy-h*.2);
      ctx.lineTo(wx+w*.24,wy-h*.24);ctx.lineTo(wx+w*.18,wy-h*.1);ctx.fill();
    }
    }
    if(p.reload>0){
      ctx.fillStyle="#e8d7b3";ctx.font=Math.max(9,w*.031)+"px monospace";
      ctx.textAlign="center";ctx.fillText("RELOADING",w/2,h*.76);
    }
    renderRadar();
  }
  function renderRadar(){
    const p=game.player;if(!p)return;
    const r=radarCtx,size=136,scale=4.15;
    r.clearRect(0,0,size,size);r.fillStyle="#15221fee";r.fillRect(0,0,size,size);
    const ox=size/2-p.x*scale,oy=size/2-p.y*scale;
    for(let y=0;y<MAP_H;y++)for(let x=0;x<MAP_W;x++){
      const t=tile(x,y);if(t){r.fillStyle=t===1?"#65746a":"#8d805d";r.fillRect(ox+x*scale,oy+y*scale,scale,scale);}
    }
    const dot=(s,color,radius)=>{
      r.beginPath();r.fillStyle=color;r.arc(ox+s.x*scale,oy+s.y*scale,radius,0,Math.PI*2);r.fill();
    };
    for(const enemy of game.enemies)if(enemy.alive&&dist2(enemy,p)<11)dot(enemy,"#d77967",2);
    for(const friend of game.friends)if(friend.alive&&friend!==p)dot(friend,"#b0d0a8",2.4);
    const o=objectives[game.objective];
    if(o){r.strokeStyle="#edcc7d";r.lineWidth=2;r.beginPath();r.arc(ox+o.x*scale,oy+o.y*scale,4.4,0,Math.PI*2);r.stroke();}
    dot(p,"#ffffff",3.5);
    r.strokeStyle="#fff9df";r.lineWidth=1.5;r.beginPath();
    r.moveTo(ox+p.x*scale,oy+p.y*scale);
    r.lineTo(ox+(p.x+Math.cos(p.angle)*1.9)*scale,oy+(p.y+Math.sin(p.angle)*1.9)*scale);r.stroke();
    r.strokeStyle="#d5dec166";r.strokeRect(.5,.5,size-1,size-1);
  }
  function tick(now){
    requestAnimationFrame(tick);
    if(!game.lastFrame)game.lastFrame=now;
    const interval=1000/(game.quality==="eco"?28:game.quality==="balanced"?35:45);
    if(now-game.lastFrame<interval)return;
    const elapsed=now-game.lastFrame;game.lastFrame=now;
    const dt=Math.min(.055,elapsed/1000);
    game.fps=game.fps*.94+(1000/elapsed)*.06;
    if(game.phase==="playing")update(dt);
    if(game.player&&game.phase!=="menu")renderWorld();
  }
  function setQuality(value){
    game.quality=["eco","balanced","sharp"].includes(value)?value:"eco";
    resizeRender();updateHud();
  }
  dom.quality.addEventListener("change",()=>setQuality(dom.quality.value));
  dom.start.addEventListener("click",()=>{
    if(game.phase==="paused"){
      game.phase="playing";
      dom.modal.classList.remove("shown");
      dom.start.innerHTML="DEPLOY <span>→</span>";
      try { if(!coarse)canvas.requestPointerLock?.(); } catch {}
    }else{
      reset();
    }
  });
  window.addEventListener("resize",resizeRender,{passive:true});
  document.addEventListener("keydown",(e)=>{
    if(["Space","ArrowUp","ArrowDown","ArrowLeft","ArrowRight"].includes(e.code))e.preventDefault();
    input.keys[e.code]=true;
    if(e.repeat||game.phase!=="playing")return;
    if(e.code==="KeyR")reload();
    if(e.code==="KeyV"||e.code==="KeyH")heal();
    if(e.code==="KeyQ")switchSoldier();
    if(e.code==="KeyF")throwGrenade("frag");
    if(e.code==="KeyT")throwGrenade("smoke");
    if(e.code==="KeyX")toggleOrders();
    if(e.code==="KeyC")toggleCrouch();
    if(e.code==="Escape"){setADS(false);game.phase="paused";dom.modal.classList.add("shown");dom.title.innerHTML="FIELD<br><em>PAUSED</em>";dom.copy.textContent="Resume the battle or redeploy. Progress is kept until you restart.";dom.start.innerHTML="RESUME <span>→</span>";document.exitPointerLock?.();}
    if(e.code==="Space")fire();
  });
  document.addEventListener("keyup",(e)=>{input.keys[e.code]=false;});
  document.addEventListener("visibilitychange",()=>{
    if(document.hidden){input.fire=false;input.joyX=0;input.joyY=0;setADS(false);}
  });
  document.addEventListener("mousemove",(e)=>{
    if(document.pointerLockElement===canvas&&game.phase==="playing")game.player.angle+=e.movementX*(game.player.ads?.00145:.0028);
  });
  canvas.addEventListener("pointerdown",(e)=>{
    if(coarse||game.phase!=="playing")return;
    if(e.button===0){input.fire=true;try{canvas.requestPointerLock?.();}catch{}}
    if(e.button===2)setADS(true);
  });
  document.addEventListener("pointerup",(e)=>{if(e.button===0)input.fire=false;if(e.button===2)setADS(false);});
  canvas.addEventListener("contextmenu",(e)=>e.preventDefault());
  document.addEventListener("pointerlockchange",()=>{if(document.pointerLockElement!==canvas)input.fire=false;});
  const movePad=$("move-pad"),knob=$("move-knob"),lookZone=$("look-zone");
  const touches={move:null,look:null,lastLookX:0};
  function joy(e){
    const rect=movePad.getBoundingClientRect(),centerX=rect.left+rect.width/2,centerY=rect.top+rect.height/2;
    const dx=e.clientX-centerX,dy=e.clientY-centerY,span=rect.width*.36,length=Math.max(1,Math.hypot(dx,dy));
    const fraction=Math.min(1,length/span);input.joyX=dx/length*fraction;input.joyY=dy/length*fraction;
    const half=knob.offsetWidth/2;
    knob.style.left=(rect.width/2+input.joyX*span-half)+"px";
    knob.style.top=(rect.height/2+input.joyY*span-half)+"px";
  }
  movePad.addEventListener("pointerdown",(e)=>{if(touches.move!==null)return;touches.move=e.pointerId;movePad.setPointerCapture(e.pointerId);joy(e);});
  movePad.addEventListener("pointermove",(e)=>{if(e.pointerId===touches.move)joy(e);});
  const clearMove=(e)=>{if(e.pointerId!==touches.move)return;touches.move=null;input.joyX=0;input.joyY=0;knob.style.left="";knob.style.top="";};
  movePad.addEventListener("pointerup",clearMove);movePad.addEventListener("pointercancel",clearMove);
  lookZone.addEventListener("pointerdown",(e)=>{if(touches.look!==null)return;touches.look=e.pointerId;touches.lastLookX=e.clientX;lookZone.setPointerCapture(e.pointerId);});
  lookZone.addEventListener("pointermove",(e)=>{if(touches.look!==e.pointerId||!game.player||game.phase!=="playing")return;const delta=clamp(e.clientX-touches.lastLookX,-55,55);touches.lastLookX=e.clientX;game.player.angle+=delta*(game.player.ads?.0044:.008);});
  const clearLook=(e)=>{if(e.pointerId===touches.look)touches.look=null;};
  lookZone.addEventListener("pointerup",clearLook);lookZone.addEventListener("pointercancel",clearLook);
  function button(id,down,up){
    const el=$(id);
    el.addEventListener("pointerdown",(e)=>{e.preventDefault();el.setPointerCapture(e.pointerId);el.classList.add("pressed");down();});
    const released=(e)=>{el.classList.remove("pressed");if(up)up();};
    el.addEventListener("pointerup",released);el.addEventListener("pointercancel",released);
  }
  button("fire-btn",()=>{input.fire=true;fire();},()=>{input.fire=false;});
  button("reload-btn",reload);
  button("heal-btn",heal);
  button("switch-btn",switchSoldier);
  button("ads-btn",()=>setADS(true),()=>setADS(false));
  button("frag-btn",()=>throwGrenade("frag"));
  button("smoke-btn",()=>throwGrenade("smoke"));
  button("order-btn",toggleOrders);
  setQuality(dom.quality.value);
  requestAnimationFrame(tick);
  if("serviceWorker" in navigator&&location.protocol!=="file:"){
    window.addEventListener("load",()=>navigator.serviceWorker.register("./sw.js").catch(()=>{}));
  }
})();