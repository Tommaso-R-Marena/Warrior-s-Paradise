# Production Presentation Bindings

Warrior's Paradise keeps simulation authoritative in C++ and routes art/audio through Data Assets and presentation components. Free or production assets can therefore replace fallbacks without reimplementing damage, ammunition, AI, capture, armor or reinforcement rules.

## Battle-wide selection

`UWPBattlePresentationSet` is the top-level presentation configuration. It selects:

- Allied/Axis × Rifleman/Assault/Medic/Engineer loadouts;
- Allied/Axis vehicle visual profiles;
- environment visual profile;
- frag and artillery explosion profiles;
- tactical-smoke presentation.

The free-content generator creates the optional conventional asset:

`/Game/WarriorParadise/Profiles/DA_WP_BattlePresentation_Free`.

## Infantry loadouts and visuals

`UWPInfantryLoadoutProfile` combines:

- `UWPWeaponDefinition`;
- `UWPInfantryVisualProfile`;
- `UWPWeaponPresentationProfile`;
- `UWPSoldierPresentationProfile`;
- grenade/smoke/medkit counts.

`UWPInfantryVisualProfile` supports third-person skeletal body + AnimBP, first-person arms, static or skeletal weapon meshes, attachment sockets/transforms and a muzzle anchor.

## Weapon presentation

`UWPWeaponPresentationComponent` subscribes to `UWPWeaponComponent` and projectile events. It can drive:

- muzzle-flash Niagara;
- spatialized fire/reload/dry-fire audio;
- attached tracer Niagara;
- generic or dirt/wood/stone/metal/flesh impact Niagara;
- matching surface-specific impact sounds;
- persistent surface-specific bullet decals.

The fallback audio attenuation enables 3D positioning and occlusion even when no authored attenuation asset is supplied.

## Soldier presentation

`UWPSoldierPresentationComponent` subscribes to health/action events and can drive:

- hit and death Niagara;
- pain/downed/revive/death/melee/bandage sounds;
- hit/downed/revive/death/melee/bandage montages.

The component owns a runtime spatial-audio attenuation/occlusion fallback.

## Explosives and smoke

`UWPExplosionPresentationProfile` supplies Niagara/audio for frag and artillery impacts while damage and suppression remain in the combat actors.

`UWPSmokePresentationProfile` supplies:

- optional smoke Niagara layered on the gameplay Local Fog Volume;
- deployment audio;
- attached hiss-loop audio.

Smoke line-of-sight blocking remains gameplay-owned by `AWPSmokeCloud`.

## Vehicles

`UWPVehicleVisualProfile` supplies:

- hull/turret/barrel meshes and transforms;
- cannon and coax muzzle Niagara/audio;
- tank-shell impact Niagara/audio;
- destruction Niagara/audio.

`AWPArmoredVehicle` still owns targeting, movement, cooldowns, armor and health.

## Environment

`UWPEnvironmentVisualProfile` supplies:

- dirt/plaster/brick/stone/wood/metal/roof/concrete materials;
- optional rural building visual mesh;
- tree/crate/barrel scenery;
- optional sky cubemap/cloud presentation;
- optional battlefield/wind/distant-battle audio.

The director retains simple collision shells underneath donated building visuals. This deliberately decouples gameplay collision/navigation from third-party mesh quality.

## Animation rule

Imported/migrated animation content is assigned only when skeleton-compatible. The generator will not silently attach a mismatched Game Animation Sample AnimBP; retargeting should be performed in UE5 first.

## Core rule

Presentation assets may subscribe to or consume gameplay state. They must not duplicate or override core combat decisions.
