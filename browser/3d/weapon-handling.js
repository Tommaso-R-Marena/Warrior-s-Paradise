/* Independent rifle/SMG/LMG recoil impulses with finite return-to-aim.
 * No per-frame allocations, sensor dependence, audio fetches or networking.
 */
(function(root){"use strict";
const weapons=Object.freeze({
 rifle:{kick:.029,side:.007,bloom:.014,returnSpeed:7.0,settle:1.4},
 smg:{kick:.011,side:.010,bloom:.009,returnSpeed:10.5,settle:1.1},
 lmg:{kick:.018,side:.010,bloom:.012,returnSpeed:8.0,settle:.85}
});
const clamp=(x,a,b)=>Math.min(b,Math.max(a,x));
function fire(p,name,roll=0){
 const w=weapons[name]||weapons.rifle;
 const kick=w.kick*(p.aim?.78:1)*(p.crouch?.82:1);
 p.pitch=clamp(p.pitch+kick,-.75,.75);
 p.yaw+=clamp(roll,-1,1)*w.side;
 p.recoilReturn=(p.recoilReturn||0)+kick;
 p.recoilBloom=clamp((p.recoilBloom||0)+w.bloom,0,.12);
 p.recoilVisual=clamp((p.recoilVisual||0)+kick*3.8,0,.19);
 p.lastRecoil=name;
}
function tick(p,dt){
 const w=weapons[p.lastRecoil]||weapons.rifle,d=clamp(dt,0,.06);
 const owed=p.recoilReturn||0;
 const recovered=Math.min(owed,owed*(1-Math.exp(-w.returnSpeed*d)));
 // Recover part, not all, of recoil each frame. No permanent vertical drift.
 p.pitch=clamp(p.pitch-recovered,-.75,.75);
 p.recoilReturn=Math.max(0,owed-recovered);
 p.recoilBloom=Math.max(0,(p.recoilBloom||0)-w.settle*d);
 p.recoilVisual=Math.max(0,(p.recoilVisual||0)-d*.75);
}
function spreadMultiplier(p){
 return 1+Math.min(2.1,(p.recoilBloom||0)*17)+
  (p.concussion||0)*.45+(p.suppression||0)*.28;
}
root.WPWeaponHandling=Object.freeze({fire,tick,spreadMultiplier,profiles:weapons});
})(window);
