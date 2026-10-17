# WebGL2 WWII infantry: authored motion transitions and UV-attached cloth colors

This extends Warrior's Paradise **free Cloudflare Pages** 3D frontend. It uses the already checked-in historical Allied and Axis glTF source rigs, preserves all existing gameplay/combat physics, does not require a GPU streaming server, and leaves Low and Balanced display quality scalable.

## Implemented real movements

The Allied glTF includes imported Idle, Aim, Walk and Run keyframed actions. The skinner now smoothly blends source-joint translation, quaternion rotation and scale into the new target animation across 240 milliseconds (smoothstep easing) when a moving actor starts walking, runs, slows down, stops, aims, or crouches. Existing procedural fallback is used for actions for which no historical clip exists. Bone blending occurs **before** parent/world joint transforms, avoiding matrix-linear interpolations that distort skinning.

The crouch posture offset eases instead of instantly dropping. The older LOD argument in the skinning renderer was ineffective: it grouped source triangles but drew all of them. This release repairs true 2×/3× index decimation. Near soldiers within 11m retain full geometry, 11–18m use half the original triangles, and beyond 18m (within the 28m High actor range) use about one-third; animation joints and UV colors remain supported. Actor animation state lives in a rendering-only WeakMap and never changes hit registration, navigation, gun damage, enemy perception or player collision. The Axis historical skinned source has a UV0 mesh and skin, but **no authored animation tracks**, so it continues to use named-bone procedural motion. Motion mode remains based on simulated walk/run speed and crouch state. We have NOT integrated bespoke prone, vault, ladder-climb, swim, vehicle enter/exit, melee or grenade throw animations in this patch; those require dedicated collision-aware game actions, properly rigged meshes and source clips.

Distant fallback infantry now distinguish running, marching, crouch-walking and aiming through different gait cadence, leg stride, knee lift, hit flinch and equipment motion. Added high-only uniform details increase silhouette fidelity but are omitted from Low and Balanced tiers.

## Implemented material preservation

Original glTF `TEXCOORD_0` UVs are now imported with vertex positions, skin weights and morphing bones. Both real historical source meshes have UV0 channels, so each skinned vertex gets **body-attached** textile color variation derived from its original `(u,v)` coordinate. The lightweight procedural weave and broad dyed-cloth variation is precomputed once per source mesh/team as an array (no repeated trig in the live skinning loop). If present, the original glTF material's `baseColorFactor` is preserved as part of the vertex albedo mix. Character material diagnostics expose `skinUVFabricTriangles` and `skinBlendedActors`.

**Important limitation:** These particular checked-in Allied/Axis files do not include embedded PBR images. The new cloth detail is an original UV-based *vertex-color approximation*, NOT sampled photographic albedo textures, full-resolution UV image maps, tangent-space normal mapping or roughness/metallic PBR on animated characters. The earlier High static-environment pipeline already supports albedo/normal/roughness/metallic and independent lightmaps, but bringing that quality to *skeletal* meshes remains a separate renderer development milestone. This patch must not be described as photorealistic or Enlisted quality.

## Verification

- `node tools/tests/browser_skinning_smoke.js` verifies the genuine Kenney 58-joint CC0 skin.
- `node tools/tests/browser_allied_rig_animation_smoke.js` checks source Allied rig/keyframe deformation.
- `node tools/tests/browser_uv_cloth_motion_smoke.js` tests real Allied/Axis glTF UV0 and mesh/material streams, action switches aim→walk→run→crouch, bone matrix interpolation, finite triangles, precomputed cloth cache and High-only fallback silhouette detail.
- `node tools/tests/browser_playwright_uv_infantry.mjs` starts an actual High Chromium/WebGL2 battle with a live Axis defender and asserts a UV-cloth-skinned model is drawn without changing its health, alongside actual shader errors and screenshot artifact.
- Existing full browser workflow checks imported assets, AI, tank/infantry ballistics, topography, smoke, sky, lightmaps, Low/Medium paired frame-time benchmark and static hosting.

Software-rendered Chromium FPS is only a regression signal. A real GPU device and an actual phone remain necessary to establish interactive FPS and final visual realism.
