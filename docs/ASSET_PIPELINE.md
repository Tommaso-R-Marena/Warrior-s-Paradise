# Asset Pipeline

The final visual target cannot be reached with primitive geometry. Production art is a first-class dependency, but it must not contaminate gameplay architecture.

## Source categories

Use assets only when their license permits the intended distribution.

Preferred sources:

- Epic/Fab assets licensed for Unreal projects;
- Megascans content where the current license permits the project use;
- original commissioned or self-created assets;
- clearly licensed third-party packs.

Do not commit source assets whose license forbids redistribution.

## Repository policy

Git tracks:

- project source;
- configuration;
- small project-owned assets;
- documentation;
- import manifests;
- generated metadata needed for reproducibility.

Large production assets should move to Git LFS once the art pipeline begins.

## Naming

Suggested prefixes:

- SK_ — skeletal mesh
- SM_ — static mesh
- M_ — material
- MI_ — material instance
- T_ — texture
- ABP_ — animation blueprint
- A_ — animation sequence
- S_ — sound
- NS_ — Niagara system
- WBP_ — widget blueprint
- DA_ — data asset

## First production art pass

1. one hero rifle with complete first-person rig and reload animation;
2. one production infantry character;
3. modular village architecture;
4. terrain/foliage kit;
5. impact decals and surface materials;
6. muzzle/impact/smoke VFX;
7. battlefield ambience and weapon audio.

Only after this pipeline is stable should weapon and faction breadth expand.
