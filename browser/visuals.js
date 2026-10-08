"use strict";
// Procedural, dependency-free canvas art direction for Field Edition.
// Generated once on-device: no remote textures, licenses, WebGL requirement.
window.WPVisuals = (() => {
  const textures = {};
  const sprites = {};
  const rnd = (n) => {
    n = Math.imul(n ^ (n >>> 16), 0x7feb352d);
    n = Math.imul(n ^ (n >>> 15), 0x846ca68b);
    return ((n ^ (n >>> 16)) >>> 0) / 4294967296;
  };
  function surface(w,h) {
    const canvas=document.createElement("canvas");
    canvas.width=w;canvas.height=h;
    return [canvas,canvas.getContext("2d",{alpha:true})];
  }
  function buildTexture(kind) {
    const [c,g]=surface(96,96);
    const base=kind===2?[107,91,65]:kind===3?[96,108,91]:[113,108,94];
    g.fillStyle="rgb("+base.join(",")+")";g.fillRect(0,0,96,96);
    for(let i=0;i<2800;i++) {
      const x=(rnd(i*37+kind*91)*96)|0,y=(rnd(i*83+kind*43)*96)|0;
      const v=(rnd(i*11+kind*61)*45)|0;
      const light=i%2===0;
      g.fillStyle=light?"rgba(225,212,176,"+(v/850)+")":"rgba(24,30,24,"+(v/450)+")";
      g.fillRect(x,y,1+rnd(i*33)*4,1+rnd(i*77)*3);
    }
    if(kind===1) {
      // Old stone/mortar: irregular courses and weathered blocks.
      for(let row=0;row<6;row++) {
        const y=row*16;
        g.strokeStyle="#41443d";g.lineWidth=2;g.beginPath();g.moveTo(0,y);g.lineTo(96,y);g.stroke();
        for(let col=0;col<4;col++) {
          const x=col*28+(row%2?14:0);
          g.strokeStyle="rgba(31,36,30,.55)";g.beginPath();g.moveTo(x,y);g.lineTo(x+rnd(row*7+col)*2,y+16);g.stroke();
        }
        g.strokeStyle="rgba(213,206,178,.26)";g.beginPath();g.moveTo(0,y+2);g.lineTo(96,y+2);g.stroke();
      }
      for(let k=0;k<6;k++) {
        const x=(rnd(k*19+51)*84)|0,y=(rnd(k*13+37)*79)|0;
        g.strokeStyle="rgba(44,49,42,.44)";g.lineWidth=1.5;
        g.beginPath();g.moveTo(x,y);g.lineTo(x+3,y+6);g.lineTo(x-1,y+10);g.stroke();
      }
    }else if(kind===2) {
      // Rough timber barricade, wooden sleepers and nails.
      for(let x=0;x<96;x+=19) {
        g.fillStyle="#433d31";g.fillRect(x,0,3,96);
        g.fillStyle="rgba(210,190,145,.20)";g.fillRect(x+4,0,1,96);
        for(let y=8;y<96;y+=30) {
          g.fillStyle="#33342c";g.fillRect(x+10,y,2,2);
          g.strokeStyle="rgba(56,47,34,.30)";g.beginPath();g.moveTo(x+5,y+7);g.lineTo(x+15,y+25);g.stroke();
        }
      }
    }else{
      // Olive painted metal and wartime reinforcing strips.
      for(let y=0;y<96;y+=24) {
        g.fillStyle="#505f4e";g.fillRect(0,y,96,3);
        g.fillStyle="rgba(224,219,171,.16)";g.fillRect(0,y+3,96,1);
      }
      for(let x=0;x<96;x+=30) {
        g.fillStyle="rgba(35,48,38,.7)";g.fillRect(x,0,3,96);
        for(let y=8;y<96;y+=24){g.fillStyle="#a39e6b";g.fillRect(x+1,y,2,2);}
      }
    }
    return c;
  }
  function drawSoldier(team,variant=0) {
    const [c,g]=surface(68,138),enemy=team==="enemy";
    // Ground shadow.
    g.fillStyle="rgba(10,18,15,.30)";
    g.beginPath();g.ellipse(34,132,24,4,0,0,Math.PI*2);g.fill();
    const trousers=enemy?"#343d38":"#4b5846",jacket=enemy?"#626761":"#718163";
    const shadow=enemy?"#323936":"#3e5143",metal=enemy?"#474d48":"#576455";
    // Boots and trousers.
    g.fillStyle="#242a25";g.fillRect(16,118,14,13);g.fillRect(39,119,14,12);
    g.fillStyle=trousers;g.beginPath();g.moveTo(18,87);g.lineTo(49,87);
    g.lineTo(52,121);g.lineTo(39,121);g.lineTo(34,98);g.lineTo(29,121);g.lineTo(16,121);g.fill();
    g.fillStyle=shadow;g.fillRect(18,98,9,22);g.fillRect(43,97,7,20);
    // Pack silhouette and legs arm shadow.
    g.fillStyle="#343e32";g.fillRect(10,43,12,42);g.fillRect(48,46,11,39);
    g.fillStyle=jacket;g.beginPath();g.moveTo(18,46);g.lineTo(46,46);
    g.lineTo(54,88);g.lineTo(14,88);g.fill();
    g.fillStyle="rgba(23,29,24,.25)";g.fillRect(14,82,40,6);
    // Straps and ammunition pouches.
    g.strokeStyle="#c2ad78";g.lineWidth=4;
    g.beginPath();g.moveTo(22,48);g.lineTo(44,80);g.moveTo(44,48);g.lineTo(24,80);g.stroke();
    g.fillStyle="#464631";for(let x=19;x<48;x+=11)g.fillRect(x,79,9,8);
    // Face, neck and helmet.
    g.fillStyle="#aa866a";g.fillRect(27,36,12,13);
    g.fillStyle="#c19d7c";g.beginPath();g.ellipse(34,30,11,13,0,0,Math.PI*2);g.fill();
    g.fillStyle="#262c29";g.fillRect(37,28,3,2);g.fillRect(29,28,3,2);
    g.fillStyle=metal;g.beginPath();g.ellipse(34,21,21,10,0,Math.PI,Math.PI*2);g.fill();
    g.fillRect(12,19,44,5);g.fillStyle="rgba(12,23,16,.35)";g.fillRect(14,23,40,3);
    // Arms hold weapon diagonally; baked silhouette.
    g.strokeStyle=jacket;g.lineWidth=12;g.lineCap="round";
    g.beginPath();g.moveTo(19,54);g.lineTo(11,77);g.lineTo(34,79);g.moveTo(48,55);g.lineTo(55,72);g.lineTo(37,78);g.stroke();
    g.strokeStyle="#b58d69";g.lineWidth=6;g.beginPath();g.moveTo(31,79);g.lineTo(37,79);g.stroke();
    g.strokeStyle="#2c312b";g.lineWidth=7;g.beginPath();g.moveTo(20,74);g.lineTo(58,70);g.stroke();
    g.strokeStyle="#847050";g.lineWidth=5;g.beginPath();g.moveTo(20,76);g.lineTo(42,74);g.stroke();
    g.fillStyle="#1f2622";g.fillRect(54,65,13,4);
    if(variant===1) {
      // Subtle alternate camouflage.
      g.fillStyle="rgba(65,84,54,.30)";
      for(let i=0;i<35;i++)g.fillRect(16+rnd(i*11)*34,50+rnd(i*7)*34,3,4);
    }
    return c;
  }
  function setup() {
    if(textures[1])return;
    for(let i=1;i<=3;i++)textures[i]=buildTexture(i);
    sprites.friend=drawSoldier("friend");
    sprites.enemy=drawSoldier("enemy");
    sprites.enemyVeteran=drawSoldier("enemy",1);
    const [tree,g]=surface(112,160);
    g.fillStyle="rgba(5,12,8,.30)";g.beginPath();g.ellipse(55,153,40,6,0,0,6.29);g.fill();
    g.fillStyle="#4c3d2b";g.fillRect(48,95,14,55);
    for(let i=0;i<19;i++) {
      const x=20+rnd(i*17)*72,y=12+rnd(i*39)*98,rad=20+rnd(i*53)*17;
      g.fillStyle=["#314536","#3c5238","#51613f","#273d35"][i%4];
      g.beginPath();g.ellipse(x,y,rad*.82,rad,0,0,6.29);g.fill();
    }
    sprites.tree=tree;
    const [crate,box]=surface(96,94);
    box.fillStyle="#2b2921";box.fillRect(12,16,73,72);
    box.fillStyle="#816c4c";box.fillRect(17,13,66,68);
    box.fillStyle="#a1885a";box.fillRect(20,16,60,5);
    box.fillStyle="#5e513a";box.fillRect(18,41,64,7);box.fillRect(18,68,64,6);
    box.strokeStyle="#453b2c";box.lineWidth=7;box.beginPath();box.moveTo(20,18);box.lineTo(81,76);box.moveTo(80,18);box.lineTo(20,76);box.stroke();
    sprites.crate=crate;
  }
  function background(ctx,w,h,angle,time,pitch=0) {
    const horizon=Math.floor(h*(.53-pitch*.56));
    const sky=ctx.createLinearGradient(0,0,0,horizon);
    sky.addColorStop(0,"#536c79");sky.addColorStop(.59,"#99a6a0");
    sky.addColorStop(1,"#e0bea0");
    ctx.fillStyle=sky;ctx.fillRect(0,0,w,horizon);
    const sunX=w*(.73-angle/(2*Math.PI)*.38);
    const sunY=h*.16;
    const glow=ctx.createRadialGradient(sunX,sunY,0,sunX,sunY,h*.4);
    glow.addColorStop(0,"rgba(255,226,163,.23)");
    glow.addColorStop(1,"rgba(255,226,163,0)");
    ctx.fillStyle=glow;ctx.fillRect(0,0,w,horizon);
    ctx.fillStyle="#e5d6af";ctx.beginPath();ctx.arc(sunX,sunY,Math.max(2,h*.035),0,Math.PI*2);ctx.fill();
    // Parallax ridge silhouettes.
    for(let layer=0;layer<3;layer++) {
      const base=horizon-h*(.09-layer*.033),amp=h*(.025-layer*.006);
      const offset=angle*(17+layer*9);
      ctx.beginPath();ctx.moveTo(0,horizon);
      for(let x=-12;x<=w+12;x+=7){
        const ridge=base-Math.sin((x+offset)*.041+layer*1.8)*amp-
          Math.sin((x+offset)*.017+layer*.7)*amp*.7;
        ctx.lineTo(x,ridge);
      }
      ctx.lineTo(w,horizon);ctx.closePath();
      ctx.fillStyle=["#607467","#657261","#606852"][layer];ctx.fill();
    }
    ctx.fillStyle="#676c57";ctx.fillRect(0,horizon,w,h-horizon);
    const earth=ctx.createLinearGradient(0,horizon,0,h);
    earth.addColorStop(0,"#8f8b70");earth.addColorStop(.55,"#6c7158");earth.addColorStop(1,"#394a36");
    ctx.fillStyle=earth;ctx.fillRect(0,horizon,w,h-horizon);
    // Cheap perspective road/grass streaks: precomputed math, no per-pixel buffer.
    for(let row=1;row<28;row++){
      const y=horizon+Math.pow(row/28,1.55)*(h-horizon);
      const light=(row%3===0);
      ctx.fillStyle=light?"rgba(198,185,132,.10)":"rgba(27,44,24,.10)";
      const shift=Math.sin(angle*3+row*.32)*w*.14;
      for(let col=0;col<9;col++){
        const x=((col*53+row*29+shift)% (w+90))-45;
        ctx.fillRect(x,y,2+row*.22,Math.max(1,row*.04));
      }
    }
    const fog=ctx.createLinearGradient(0,horizon-h*.08,0,horizon+h*.18);
    fog.addColorStop(0,"rgba(205,189,159,0)");
    fog.addColorStop(.5,"rgba(205,189,159,.22)");
    fog.addColorStop(1,"rgba(205,189,159,0)");
    ctx.fillStyle=fog;ctx.fillRect(0,horizon-h*.08,w,h*.27);
  }
  // Ground plane casting: perspective-correct world coordinates at each
  // coarse screen cell. Banded/fogged terrain supplies depth absent from
  // a flat-color raycaster, at a capped CPU cost for entry-level phones.
  function floor(ctx,w,h,px,py,angle,ads,quality,pitch=0) {
    const horizon=Math.floor(h*(.53-pitch*.56));
    const cell=quality==="eco"?12:quality==="balanced"?8:5;
    const rowStep=quality==="eco"?8:quality==="balanced"?6:4;
    const dx=Math.cos(angle),dy=Math.sin(angle);
    const fov=ads?.44:.69,planeX=-dy*fov,planeY=dx*fov;
    const leftX=dx-planeX,leftY=dy-planeY;
    const rightX=dx+planeX,rightY=dy+planeY;
    const colors={
      grass:["#586446","#626b48","#4d5d41","#6a7551","#464e38"],
      road:["#72644e","#82765d","#6c604d","#8d8062"],
      mud:["#514b3c","#5e543e","#736248"],
      track:["#463e33","#534634"]
    };
    for(let sy=horizon+3;sy<h;sy+=rowStep){
      const rowDistance=(h*.47)/Math.max(2,sy-horizon);
      // Subdivide in perspective; the approximate world footprint is
      // shared by all pixels of each cell, avoiding an expensive per-pixel pass.
      const stepX=rowDistance*(rightX-leftX)*cell/w;
      const stepY=rowDistance*(rightY-leftY)*cell/w;
      let wx=px+rowDistance*leftX,wy=py+rowDistance*leftY;
      for(let sx=0;sx<w;sx+=cell,wx+=stepX,wy+=stepY){
        const ix=Math.floor(wx*3),iy=Math.floor(wy*3);
        const noise=rnd(Math.imul(ix,73856093)^Math.imul(iy,19349663));
        const diagonal=Math.abs(wx-wy-.45);
        const mainRoad=diagonal<1.18&&wx>3.5&&wx<25.8&&wy>3.2&&wy<25.8;
        const secondaryRoad=(Math.abs(wy-14.1)<.69&&wx>9&&wx<21)||
          (Math.abs(wx-7.1)<.58&&wy>3&&wy<11);
        const road=mainRoad||secondaryRoad;
        const wheelTrack=road&&
          ((diagonal>.42&&diagonal<.55)||
           (Math.abs(wy-14.1)>.28&&Math.abs(wy-14.1)<.33&&wx>9&&wx<21));
        const damp=noise<.035;
        let palette=road?colors.road:colors.grass;
        if(wheelTrack)palette=colors.track;
        else if(damp||(!road&&noise<.12))palette=colors.mud;
        const idx=Math.floor(noise*palette.length)%palette.length;
        ctx.fillStyle=palette[idx];
        ctx.fillRect(sx,sy,cell+1,rowStep+1);
      }
      // Air perspective increases with distance toward the horizon.
      const fog=Math.min(.48,Math.max(0,(rowDistance-4)*.025));
      if(fog>.03){
        ctx.fillStyle="rgba(184,170,142,"+fog.toFixed(3)+")";
        ctx.fillRect(0,sy,w,rowStep);
      }
    }
  }
  function wall(ctx,x,top,width,height,hit,depth,quality) {
    const tex=textures[hit.type]||textures[1];
    if(!tex)return false;
    const column=Math.min(95,Math.max(0,Math.floor(Math.abs(hit.texture-Math.floor(hit.texture))*96)));
    ctx.drawImage(tex,column,0,1,96,x,top,width,Math.max(1,height));
    // Face shading, depth haze and lighter warm air at distance.
    const darkness=(hit.side===1?.16:.04)+Math.min(.34,depth*.012);
    ctx.fillStyle="rgba(18,25,20,"+darkness.toFixed(3)+")";
    ctx.fillRect(x,top,width,height);
    if(depth>5){
      ctx.fillStyle="rgba(190,178,149,"+Math.min(.44,(depth-5)*.027).toFixed(3)+")";
      ctx.fillRect(x,top,width,height);
    }
    if(quality!=="eco"&&height>12){
      ctx.fillStyle="rgba(13,19,17,.17)";
      ctx.fillRect(x,top+height*.95,width,Math.max(1,height*.05));
    }
    return true;
  }
  function soldier(ctx,s,mid,top,w,h,depth) {
    setup();
    const sprite=s.kind==="friend"?sprites.friend:(s.role==="DEFENDER"?sprites.enemyVeteran:sprites.enemy);
    const tint=depth>10?Math.min(.48,(depth-10)*.04):0;
    ctx.drawImage(sprite,mid-w*.62,top,w*1.24,h);
    if(tint){ctx.fillStyle="rgba(158,158,132,"+tint+")";ctx.fillRect(mid-w*.4,top+h*.13,w*.8,h*.75);}
    if(s.hit>0){ctx.fillStyle="rgba(255,233,206,.6)";ctx.fillRect(mid-w*.48,top+h*.32,w*.96,h*.51);}
    if(s.flash>0){
      ctx.fillStyle="#ffe19b";ctx.beginPath();ctx.arc(mid+w*.55,top+h*.52,Math.max(2,h*.05),0,Math.PI*2);ctx.fill();
    }
  }
  function decoration(ctx,s,mid,top,w,h) {
    setup();ctx.drawImage(sprites[s.kind]||sprites.crate,mid-w/2,top,w,h);
  }
  function weapon(ctx,w,h,p,t,ads) {
    const j=p.flash>0?Math.random()*w*.007:0;
    const bob=Math.sin(t*9)*h*.012;
    const baseX=w*(ads?.50:.62)+j,baseY=h*(ads?.71:.87)+bob;
    ctx.save();ctx.translate(baseX,baseY);
    const scale=w/330;ctx.scale(scale,scale);
    // Wood stock, black receiver, angled hand guard, barrel and front sight.
    ctx.fillStyle="#54432b";ctx.beginPath();ctx.moveTo(-18,15);ctx.lineTo(33,40);
    ctx.lineTo(100,52);ctx.lineTo(108,89);ctx.lineTo(3,89);ctx.closePath();ctx.fill();
    ctx.fillStyle="#262d2c";ctx.fillRect(-12,-4,91,23);
    ctx.fillStyle="#44504a";ctx.fillRect(0,0,90,8);
    ctx.fillStyle="#292e2d";ctx.fillRect(55,-18,16,19);
    ctx.fillStyle="#666b62";ctx.fillRect(58,-24,4,10);
    ctx.fillStyle="#1c2121";ctx.fillRect(66,14,19,42);
    ctx.fillStyle="#9f7a55";ctx.beginPath();ctx.ellipse(24,55,23,15,-.3,0,6.29);ctx.fill();
    ctx.strokeStyle="#161d1b";ctx.lineWidth=3;ctx.beginPath();ctx.moveTo(-10,-5);ctx.lineTo(79,-5);ctx.stroke();
    if(p.flash>0){
      ctx.fillStyle="#ffe2a0";ctx.beginPath();ctx.moveTo(63,-23);ctx.lineTo(40,-67);
      ctx.lineTo(66,-50);ctx.lineTo(93,-68);ctx.lineTo(79,-23);ctx.closePath();ctx.fill();
      ctx.fillStyle="#fff9df";ctx.beginPath();ctx.arc(63,-28,8,0,6.29);ctx.fill();
    }
    ctx.restore();
  }
  return {setup,background,floor,wall,soldier,decoration,weapon};
})();
