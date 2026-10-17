# UV PBR and secondary-UV baked irradiance for free Cloudflare hosting

This is the companion to docs/OFFLINE_BAKED_CLOUDFLARE_ART.md. It ships with a separate UV-textured WebGL2 material renderer, authored image maps, two geometry LODs, per-asset source metadata, texture integrity checks, and a Blender Cycles offline exporter. There is no remote rendering GPU at playtime.

## Implemented

* UV0: base color/albedo (decoded as sRGB), OpenGL tangent-space normal maps, separate roughness and optional metallic textures. The shader uses Fresnel-Schlick, GGX highlights, ambient irradiance and tone mapping.
* UV1: independent non-overlapping texture coordinates for precomputed static RGB illumination. Maps are treated as linear data and do not overwrite PBR albedo colors.
* Static Medium/High packs: quantized 18-byte vertices with positions, normals, UV0 and UV1. License/author, checksum, local-path, triangle and texture-size checks fail closed. Low and Balanced never initialize this expensive PBR renderer; the High preset loads and renders it. Medium LOD output is maintained and tested but reserved for future optimization.
* Existing gameplay collision remains authoritative: placement rectangles must fit existing buildings. This pipeline cannot create invisible bulletproof cover.
* Starter scene: original Normandy brick wall facade (2350 High vs 2 Medium triangles per instance) on three buildings. It uses existing ambientCG CC0 brick color, real authored normal and roughness photographs, plus an original procedurally precomputed 64-pixel-square **example** linear irradiance map. The example is NOT physically ray-traced, not photogrammetry, and is not a historically certified asset. Final Blender exports generate actual baked direct/indirect lightmaps.
* CI checks packs, normal/UV reconstruction, source licensing, shader integration, real Chromium/SwiftShader and unchanged combat and FPS guards.

## How to use temporary GPU workstation

1. Obtain a high-quality legally redistributable WW2 model with original textures. In Blender ensure ONE opaque material per exported part and clean, authored UV0. Split complex multi-material models into separate parts. The exporter is a static asset tool, not a skeleton or vehicle-turret animation pipeline.
2. Use Blender 4.2+ and Cycles; for example:

    blender --background --python tools/assets/blender_export_pbr_lightmaps.py -- --input /assets/farmhouse.glb --id farmhouse-hero --author "Real artist" --license CC0-1.0 --texture-size 512 --lightmap-size 256 --medium-ratio .42 --cycles-samples 32 --output browser/3d/assets/pbr

3. The exporter preserves original UV0, creates packed independent UV1, bakes material albedo, tangent normals, roughness and the actual metallic input; it also bakes direct and indirect static irradiance using Cycles, packs Medium/High geometry, and writes local PNG/base64 PBR images and a linear RGBA lightmap.
4. Register generated asset and paths in browser/3d/assets/pbr/scene.json. Anchor each placement over known collidable geometry; physical collision and AI LOS for any NEW structures must be implemented separately.
5. Run: python tools/assets/compile_pbr_lightmaps.py --verify browser/3d/assets/pbr/farmhouse-hero.wppbr.json; then python -m unittest discover -s tools/tests -p test_pbr_lightmaps.py; then node tools/tests/browser_pbr_lightmaps_smoke.js. Let the full GitHub Actions Chromium suite run.
6. Merge only after visual inspection, attribution review, benchmark passes and production Cloudflare verification. The workstation can then be shut off; clients render locally.

## Candidate sources, not bundled

* WWII Panzergrenadier by britdawgmasterfunk, approximately 27,000 triangles and 2K PBR maps. BlendSwap describes a CC0 rigged character: https://blendswap.com/blend/31342 . Its Sketchfab listing displays CC Attribution: check the exact host file and terms and retain author credit accordingly.
* MetaWorldOS WWII Tank Pack with Sherman and Panzer IV PBR GLB models: https://metaworldos.itch.io/wwii-tank-pack-glb-models . Its listing states CC0 but the full archive is roughly 809 MB. Inspect and selectively convert actual files, not the full archive into the Cloudflare website.
* ambientCG Bricks066, Grass004 and Ground037: CC0 materials already vendored in the browser project. Larger original normal/roughness maps may be baked into unique atlas parts.

No high-poly Panzergrenadier or tank pack has been downloaded and integrated by this change. The existing CC0 soldier and WWII art is predominantly low-poly. Achieving Enlisted-quality actors requires rig and animation transfer, terrain and foliage, material coherence, new collision, and real GPU profiling.

## Limits and safe handling

The current importer caps the PBR scene at 6 material groups, at most 5 placements each, <=22,000 High triangles and <=10,000 Medium triangles, individual packed mesh stream <=5 MiB, 2048px-per-side image maps, and strictly relative image paths. These are deliberate resource limits, not an architectural ceiling. Mobile/Low and Balanced never initialize the extra renderer.

Baked lightmaps are static 8-bit linear RGB irradiance (LDR), not dynamic full global illumination or real-time reflections; they do not recalculate when a wall is destroyed. The shader supports opaque geometry, not transparent windows or animated/skeletal PBR materials. Multiple material parts require independent atlas groups. The exporter successfully passed a genuine headless Blender/Cycles CI run on a simple original, UV-mapped GLB, producing all five maps and quantized mesh LODs. That test does **not** establish successful conversion of complex externally licensed multi-material/animated WWII assets. Correct tangent-space normal orientation, atlas pack seams, world illumination and per-device FPS require real Blender/GPU inspection.

A GPU computer is only needed to **author and bake** graphics. It cannot precompute every interactive frame for all future players. The static browser deployment remains free within Cloudflare Pages quotas while each viewer uses their own device's graphics processing.
