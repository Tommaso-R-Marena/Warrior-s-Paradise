# One-Command Free Content Workflow

The complete redistributable/free-resource setup can be run locally on Windows:

~~~powershell
.\tools\setup_free_content.ps1
~~~

or:

~~~bat
tools\setup_free_content.bat
~~~

## Default flow

The command:

1. downloads the curated Poly Haven CC0 set at 2K;
2. downloads and safely extracts approved direct CC0 sources;
3. writes source URLs, hashes and license provenance to `ExternalAssets/asset-lock.json`;
4. runs `tools/validate_project.py`;
5. finds Unreal Engine 5.8;
6. compiles `WarriorParadiseEditor`;
7. runs the external-asset importer through `UnrealEditor-Cmd.exe`;
8. builds CC0 PBR materials;
9. discovers installed free characters, weapons, VFX, audio, tanks and environment assets;
10. generates Warrior's Paradise loadout/presentation Data Assets;
11. creates the battle-wide `DA_WP_BattlePresentation_Free` preset;
12. reruns validation and asset-coverage reporting.

The runtime director automatically looks for:

~~~text
/Game/WarriorParadise/Profiles/DA_WP_BattlePresentation_Free
~~~

If it exists, new soldiers, tanks and runtime village surfaces use it. If it does not, the primitive fallback remains functional.

## Resolution

~~~powershell
.\tools\setup_free_content.ps1 -Resolution 4k
~~~

2K is the default because it is a good vertical-slice balance between fidelity, disk use and import time.

## Re-run after adding provider assets

The script intentionally does not scrape/authenticate into Epic/Fab/Sonniss.

Acquire those through their supported interfaces, then rerun:

~~~powershell
.\tools\setup_free_content.ps1 -SkipDownload -SkipBuild
~~~

The profile generator searches installed project content for MetaHuman/WWII soldiers, Motion Matching AnimBPs, period weapons, Sherman/Panzer/Panther meshes, rural buildings, muzzle/impact/tracer/explosion/smoke Niagara, bullet-hole/decal materials, soldier and weapon audio, vehicle audio, battlefield ambience, generated Poly Haven materials, scenery props and an overcast HDRI. It creates loadout, weapon, soldier, smoke, explosion, vehicle and environment profiles and connects them through the battle-wide preset.

Only skeleton-compatible AnimBPs are assigned automatically. Otherwise the script reports that UE5 IK retargeting is needed.

## Offline / selective use

Skip downloads:

~~~powershell
.\tools\setup_free_content.ps1 -SkipDownload
~~~

Skip the C++ build after a previously clean compile:

~~~powershell
.\tools\setup_free_content.ps1 -SkipBuild
~~~

Download only direct CC0 content:

~~~powershell
.\tools\setup_free_content.ps1 -SkipPolyHaven
~~~

Download only Poly Haven:

~~~powershell
.\tools\setup_free_content.ps1 -SkipDirectCC0
~~~

## Important limitation

This automates everything that can be automated legally and reproducibly without logging into provider storefronts. Free Fab/Epic/Sonniss content still requires acquisition through those providers. Once those files are installed locally, no gameplay-code changes are required to use them.
