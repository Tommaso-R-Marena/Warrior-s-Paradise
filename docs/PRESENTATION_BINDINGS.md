# Production Presentation Bindings

This file defines where production animation, VFX and audio should attach. Presentation code should **not** duplicate gameplay decisions.

## Weapon

`UWPWeaponComponent` exposes:

- `OnShotFired` — recoil/camera/AnimBP fire reaction;
- `OnProjectileSpawned` — muzzle flash, muzzle smoke, shell-ejection coordination;
- `OnReloadStarted` — reload montage/audio;
- `OnReloadCompleted` — montage/state completion;
- `OnReloadCancelled` — interrupt montage/audio cleanly;
- `OnDryFire` — trigger click/empty feedback;
- `OnWeaponStateChanged` — UI/AnimBP state refresh.

## Bullet impacts

`AWPBallisticProjectile::OnImpact` provides impact location, normal and actor.

Production binding should resolve physical material/surface type and spawn the correct decal, particles, sound and debris. Do not move damage calculation into the VFX layer.

## Soldier health / wounds

`UWPHealthComponent` exposes:

- `OnDamageReceived` — damage magnitude + source;
- `OnHealthChanged`;
- `OnDowned`;
- `OnRevived`;
- `OnDeath`.

Use these for hit reactions, blood/dirt presentation, voice lines, downed animation and death animation/ragdoll transitions.

## Suppression

`UWPSuppressionComponent::OnSuppressionChanged` exposes normalized suppression.

The C++ player already applies a restrained FOV/roll response. Production audio can add near-miss crack, ringing or mix ducking without changing suppression values.

## Soldier actions

`AWPSoldierCharacter` exposes:

- `OnMeleeAttack`;
- `OnBandageStarted`;
- `OnBandageCompleted`.

Production AnimBPs should bind montages/hand IK/props here.

## Explosives and smoke

- `AWPFragGrenade::OnExploded`;
- `AWPSmokeCloud` is the shared visual/gameplay smoke volume;
- artillery exposes `OnImpact`.

Replace temporary light/debris presentation with Niagara/audio while keeping radius/damage/suppression in C++.

## Vehicles

`AWPArmoredVehicle` exposes:

- `OnCannonFired`;
- `OnMachineGunFired`;
- `OnVehicleDamaged`;
- `OnVehicleDestroyed`.

`AWPTankShell::OnImpact` exposes shell impact location.

Production vehicle Blueprints should attach meshes, tracks/wheels, turret animation, exhaust, suspension presentation and audio without reimplementing armor health or target rules.

## Recommended production subclasses

Create Blueprint subclasses such as:

- `BP_WP_AlliedRifleman`;
- `BP_WP_AxisRifleman`;
- `BP_WP_AlliedTank`;
- `BP_WP_AxisTank`;
- production projectile/effect subclasses where necessary.

Then set the existing class references in game/director/data assets to those subclasses. Core C++ should remain the source of truth.
