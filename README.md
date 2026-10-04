# Warrior's Paradise

**Warrior's Paradise** is an Unreal Engine 5.8 WWII first-person combat project aimed at an **Enlisted-quality vertical slice**: one highly polished battle first, then expansion only after combat, AI, visuals, audio, and performance meet the target.

## Current milestone

The repository contains an executable, asset-independent combat foundation:

- first-person WASD movement, mouse look, jump, crouch, and sprint;
- physical rifle projectiles with gravity, configurable muzzle velocity, travel range, aim-dependent dispersion, magazines, reserve ammunition, and reload timing;
- right-mouse aim-down-sights with FOV/weapon interpolation and procedural recoil response;
- reusable health/death component;
- explicit Allied/Axis combat factions with autonomous friendly and hostile infantry;
- AI target selection, range management, line-of-sight checks, burst fire, and suppression-sensitive behavior;
- five friendly squadmates who fight alongside the player and become controllable reserves after player death;
- projectile near-miss suppression with decay and HUD feedback;
- three sequential capture objectives, reinforcement escalation, final counterattack, and battle-victory state;
- runtime-generated battlefield blockout with buildings, cover, boundaries, directional light, skylight, and atmospheric fog;
- wave spawning, scoring, and escalation;
- code HUD for health, ammunition, wave, hostiles, score, crosshair, and death state;
- Unreal project/build/configuration scaffolding;
- GitHub Actions repository validation.

This is the **code-side vertical-slice foundation**, not yet the production visual target. Production terrain, character/weapon assets, animation, audio, VFX, destruction, deeper squad tactics, and vehicles still require Unreal Editor content work and licensed/original assets.

## Engine

Target: **Unreal Engine 5.8**. Use the newest 5.8 hotfix available in the Epic Games Launcher.

## Run

1. Install Unreal Engine 5.8 and Visual Studio 2022 with **Game development with C++**.
2. Clone this repository and check out the desired branch.
3. Generate Visual Studio project files for `WarriorParadise.uproject` if Unreal asks.
4. Build the `WarriorParadiseEditor` target.
5. Open `WarriorParadise.uproject`.
6. Press **Play**.

The first bootstrap deliberately uses Unreal's built-in Entry map. `AWPBattlefieldDirector` creates a battlefield blockout at runtime, allowing the combat loop to execute without a committed binary map.

## Controls

| Input | Action |
|---|---|
| W / A / S / D | Move |
| Mouse | Look |
| Right Mouse | Aim down sights |
| Left Shift | Sprint |
| Left Control | Crouch |
| Space | Jump |
| Left Mouse | Fire |
| R | Reload |

## Production plan

- [Vertical slice specification](docs/VERTICAL_SLICE.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Asset pipeline](docs/ASSET_PIPELINE.md)
- [Performance target](docs/PERFORMANCE_TARGET.md)

## Validation

```bash
python tools/validate_project.py
```

Run the structural contract locally with Python before opening Unreal. A full Unreal C++ compile still requires an Unreal Engine 5.8 installation or an appropriately provisioned build runner. The repository also contains a manually dispatchable GitHub workflow; automatic hosted jobs are intentionally disabled while the connected account's hosted runner is failing before job steps begin.
