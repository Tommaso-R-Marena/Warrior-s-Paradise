/* Conservative player-view visibility for *rendering only*.
 * Never cull gameplay simulation, collision, LOS, AI or damage.
 * High-only: near actors within 10m always visible, distant actors
 * well beyond the 73deg camera plus margin may skip CPU skinning.
 */
(function(root){"use strict";
function visible(actor,player,quality){
 if(quality!=="high")return true;
 if(!actor||!player)return true;
 const x=actor.x-player.x,z=actor.z-player.z;
 if(!Number.isFinite(x)||!Number.isFinite(z)||!Number.isFinite(player.yaw))return true;
 const d2=x*x+z*z;
 if(d2<=100)return true;
 if(d2>90*90)return false;
 // At >10m, a 120° half-plane guard exceeds the widest horizontal FOV
 // (73° vertical at 16:9 = ~104° horizontal). Generous angular padding
 // protects peripheral infantry, third-person tank and animation motion.
 const cosine=(-Math.sin(player.yaw)*x+Math.cos(player.yaw)*z)/Math.sqrt(d2);
 return cosine>-.50;
}
function sortVisible(actors,player,quality){
 return actors.filter(a=>visible(a,player,quality))
  .sort((a,b)=>(a.x-player.x)**2+(a.z-player.z)**2-
                ((b.x-player.x)**2+(b.z-player.z)**2));
}
root.WPActorVisibility=Object.freeze({visible,sortVisible});
})(window);
