# Combined Arms

Warrior's Paradise treats vehicles, emplacements and aircraft as participants in the same infantry battle rather than separate minigames.

## Infantry squad

The infantry layer remains the core of the slice:

- six-man Allied squad including the player;
- Rifleman / Assault / Medic / Engineer roles;
- Follow / Attack / Hold orders;
- manual soldier switching;
- downed / bleedout / revive / replacement lifecycle;
- suppression, cover, smoke, hearing and last-known-position AI;
- nearby squadmates can embark a transport automatically and return to autonomous infantry behavior after disembark.

## Tanks

`AWPArmoredVehicle` now supports both autonomous and player-crewed operation.

Simulation includes:

- directional front / side / rear armor response;
- AP and HE shell types;
- automatic AI shell selection by target type;
- engine subsystem;
- tracks subsystem;
- turret subsystem;
- ammo-rack subsystem;
- mobility degradation;
- immobilization from engine/track destruction;
- slower turret/cannon performance after turret damage;
- catastrophic ammo-rack damage;
- cannon and coaxial MG;
- smoke-aware target acquisition;
- prioritization of hostile armor, light vehicles and anti-tank guns.

### Player tank controls

| Input | Tank action |
|---|---|
| W / S | Drive forward / reverse |
| A / D | Turn hull |
| Mouse X / Y | Traverse turret / elevate gun |
| Left Mouse | Fire cannon |
| Right Mouse | Hold coaxial MG fire |
| R | Toggle AP / HE |
| E | Exit tank |

The HUD reports hull, engine, tracks, turret, ammo rack and selected shell.

## Transport and light vehicles

`AWPLightVehicle` supports:

- transport truck;
- armed armored-car mode;
- player possession;
- squad embark/disembark;
- emergency disembark on destruction;
- vehicle health/destruction;
- engine/weapon presentation profiles;
- autonomous armored-car combat when not player-controlled;
- smoke-aware target acquisition.

Entering a transport suspends mounted squad AI and disables collision. On exit, the soldiers are restored to the battlefield and resume their normal AI behavior.

## Gun emplacements

`AWPGunEmplacement` supports:

- heavy machine gun;
- anti-tank gun;
- player operation;
- autonomous unoccupied operation;
- mechanical yaw/pitch traverse limits;
- smoke-aware sight;
- physical projectiles;
- AP/HE anti-tank shells;
- damage/destruction and emergency operator ejection.

AT guns prioritize hostile tanks/light vehicles. HMGs primarily engage infantry.

## Air support

`AWPAirSupportAircraft` supports three mission types:

- Flyby;
- Strafe;
- Bombing.

Aircraft traverse the battlefield as real actors. Strafe missions fire physical ballistic projectiles. Bombing missions release physical `AWPAirBomb` actors.

Air bombs affect:

- infantry damage;
- infantry suppression;
- armored vehicles;
- light vehicles;
- emplacements;
- destructible cover.

The Allied player has limited air-support charges. Target areas containing hostile vehicles bias the support request toward bombing; otherwise the director selects a strafe pass.

### Infantry support control

| Input | Support action |
|---|---|
| T | Designate artillery |
| Y | Designate air support |

## Battle choreography

The runtime battle currently stages:

- Allied transport at the opening position;
- usable Allied HMG and AT-gun support positions;
- Axis defensive HMG/AT emplacements;
- Allied and Axis armored-car escalation;
- Allied strafe support during the village push;
- Allied bombing support around the inner-village armor phase;
- Allied tank support;
- Axis tank counterattack;
- Axis bombing during the final counterattack;
- recurring non-combat Allied/Axis aircraft flybys.

The intent is to create the perception of a wider battle without requiring fully player-flyable aircraft yet.

## Presentation

The battle presentation set can independently provide:

- tank visual profiles;
- transport / armored-car visual profiles;
- HMG / AT-gun visual profiles;
- Allied / Axis aircraft visual profiles;
- engine, gun, cannon, flyby and destruction audio;
- muzzle / explosion / destruction Niagara.

The free-content generator searches installed assets and creates these Data Assets automatically.

## Validation boundary

The combined-arms code has passed static declaration/implementation and source-structure checks. It has **not yet passed a real Unreal Engine 5.8 compile or PIE playtest**. That remains the authoritative gate for controls, collision, camera feel, pivots, imported asset scale and runtime tuning.
