# Architecture

Warrior's Paradise uses a C++-first Unreal Engine architecture. Core combat rules remain reviewable in source; Blueprints and data assets are reserved for presentation, authored encounters, and art-facing configuration.

## Current runtime

### AWPSoldierCharacter

Owns first-person locomotion, camera, movement state, health, weapon input, sprinting, crouching, and death handling.

### UWPWeaponComponent

Owns magazine state, reserve ammunition, cadence, spread, traces, damage, and reload timing.

The component is presentation-independent. Replacing the temporary view weapon with a production skeletal weapon does not change ballistic rules.

### UWPHealthComponent

Owns health state and exposes health/death delegates.

### AWPEnemySoldier

Provides the first executable opponent:

- range management;
- target facing;
- line-of-sight checks;
- advance/retreat behavior;
- controlled firing pulses;
- battlefield-director death reporting.

This is a bootstrap opponent. Production AI will move decision-making into squad and tactical layers.

### AWPBattlefieldDirector

Owns the code-generated fallback battlefield, combat waves, score, and wave progression.

Keeping a code-only fallback means the combat kernel can be validated even if production map assets are absent.

### AWPCombatGameMode / AWPCombatHUD

Establish the gameplay classes and keep the first playable loop independent of UMG assets.

## Production layers

The mature project separates six concerns:

1. **Simulation** — health, wounds, damage, ballistics, inventory, teams, objectives.
2. **Presentation** — skeletal meshes, animation, audio, VFX, camera, UI.
3. **Decision AI** — perception, threat assessment, suppression, tactical intent.
4. **Locomotion AI** — paths, cover, formations, local avoidance, vaulting.
5. **Battle orchestration** — tickets, objectives, reinforcements, artillery, vehicles.
6. **Content data** — weapons, factions, roles, encounters, tuning.

## Multiplayer posture

The vertical slice is single-player first. Simulation classes should avoid assumptions that prevent later authority/replication work, but networking will not be allowed to slow down proof of the core battlefield experience.

Once the single-player slice is stable, multiplayer work should begin from explicit authority boundaries rather than retrofitting arbitrary replicated properties.
