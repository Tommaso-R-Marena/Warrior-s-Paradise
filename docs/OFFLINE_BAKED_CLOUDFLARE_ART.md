# Permanent zero-GPU-server art pipeline

Warrior's Paradise has **two different products**: a Windows UE5.8 source project
and the WebGL2 Cloudflare Pages frontend. Unreal Windows game executables cannot
be uploaded to Cloudflare Pages as playable HTML5 games. This pipeline applies to
the **static WebGL2 battlefield**. No GPU-equipped build computer is required
after authoring/exporting the art and committing the generated output.

## What is shipping

- `browser/3d/assets/baked/scene.json`: explicit curated placements, each
  required to overlap an already physically collidable farmhouse.
- `normandy-stonework.wpmesh.json`: *original illustrative farmhouse facade*
  static geometry with 108-triangle medium / 1,060-triangle high LOD per instance,
  three instances, baked per-vertex shading; not photogrammetry.
- `baked-art.js`: no external CDN or NPM runtime. Decodes two quantized triangle
  streams, validates source/license/geometry checksum and geometry budgets,
  applies local transforms, and rejects malformed or unanchored files.
- `game.js`: atomically adds Medium vertices **after** the original medium
  scene prefix; appends High vertices **after** High content. Low and original
  gameplay collision remain unchanged. Existing static WebGL2 draw call and
  shaders serve all three quality settings.
- The shader uses a negative *red* material marker only for imported baked
  vertex colors, distinct from the pre-existing signed *blue* painted-steel
  marker. It prevents terrain photo textures from overwriting baked shading;
  neither sign is displayed as negative RGB. The Low shader is unchanged for
  actual Low scene geometry.
- `tools/assets/compile_baked_art.py`: input validation, quantization, LOD
  compiler, local byte integrity checks and ownership metadata.
- `tools/assets/blender_bake_browser_art.py`: local Blender exporter with
  bounded deterministic ray-based **vertex ambient occlusion**, not full
  path-traced lightmaps; optional geometric decimation for Medium.
- CI runs Python pack tests, exact original sample regeneration, Node
  fail-closed decoder tests, Chromium WebGL2 shader/render smoke tests and the
  unchanged gameplay, terrain, tank, and software FPS acceptance gates.

## Offline build on a temporary GPU-equipped workstation

On the workstation, clone the repository and install Blender, Git and Python 3.
Blender works with a CPU too, but a fast machine speeds up authoring.
Before importing third-party 3D assets, read their license and confirm you can
redistribute a **modified** model and required attribution. Do not assume that
noncommercial use makes unlicensed art legal.

1. Choose a visually coherent, locally stored GLB, glTF, OBJ or FBX model,
   ideally with Blender-compatible mesh material base colors. Position it near
   its origin at useful real-world meter scale. Apply transforms as needed.
2. Bake and optimize the mesh from the repository root:

   ```bash
   blender --background --python tools/assets/blender_bake_browser_art.py -- \
     --input /absolute/path/to/farmhouse-detail.glb \
     --id farmhouse-detail \
     --author "Original artist and source" --license CC0-1.0 \
     --medium-ratio 0.38 --ao-rays 12 --ao-distance 2.0 \
     --output browser/3d/assets/baked/farmhouse-detail.wpmesh.json
   ```

3. Inspect the output. It has explicit source/author/license plus a SHA-256
   of the **input file**. The source metadata must reflect the real author,
   not the computer operator. Add a `groups` entry in
   `browser/3d/assets/baked/scene.json` with an approved mesh identifier
   and one or more `{x,y,z,yaw,scale,collider:"existing-building"}` placement
   objects. Every placement must fit over **existing building collision** in
   `game.js`. Decorative art cannot create fake hard cover; new free-standing
   buildings need separately modeled authoritative collision/AI LOS first.
4. Confirm byte integrity and successful gameplay rendering:

   ```bash
   python3 tools/assets/compile_baked_art.py --verify browser/3d/assets/baked/farmhouse-detail.wpmesh.json
   python3 -m unittest discover -s tools/tests -p test_baked_art_pipeline.py
   node tools/tests/browser_baked_art_smoke.js
   python3 -m http.server 8765 --directory browser
   ```

   Open `http://localhost:8765/3d/`, choose High, and check
   `WP3DDiagnostics().bakedArt` in developer tools. The CI browser workflow
   also asserts that shader compilation, asset decoding and gameplay work.

5. Commit the generated mesh JSON, scene manifest and source/credit documents.
   Cloudflare Pages automatically deploys `browser/` on `main`. The
   authoring computer can be turned off. **Each player's GPU still renders
   the model in real time**, at a quality tier suited to that hardware.

## Format and limits

`wp-baked-mesh-v1` stores local bounding-box min/max, then two independent
triangle streams. Each vertex uses 12 bytes: three uint16 local positions,
three int8 vertex normals and three uint8 linear color × baked ambient
occlusion. This is compact but not lossless: normals and RGB are quantized, and
real-world position precision depends on model size. Source code includes a
round-trip validator. FNV-1a catches accidental corruption; it is **not**
a cryptographic authenticity check and does not replace trusted Git history,
authorship review or TLS.

The browser enforces a 4 MiB packed stream limit and 180,000 vertices per
individual LOD, 12 groups, at most 12 placements per group, and an aggregate
maximum of 35,000 High / 15,000 Medium triangles after instancing. These
budgets protect no-GPU-server rendering and may be tightened based on measured
real hardware. Do not feed untrusted models into the repo or its Blender
importer; curate licenses and scan files before conversion.

**Known limitations**: the browser shader does not preserve imported UVs,
full PBR metal/roughness, normal map textures, skeletal animation or baked
lightmap atlases *for these new packs*. The exporter reads material base
colors; connected photographic texture graphs are **not** baked to diffuse
texture maps in this version. This is genuine *ambient-occlusion vertex-color
baking*, not baked global illumination, a complete photogrammetry pipeline or
a substitute for authentic high-detail infantry and armor models. The
existing three CC0 grass/mud/brick normal/roughness atlases work separately.

For a real Enlisted-like upgrade, the follow-on is high-resolution licensed
soldier/tank art with UV texture baking, animation rigs, authored environments,
more efficient camera-distance LOD, compressed texture streaming, actual
integrated-GPU and mobile frame captures, and separate Unreal build validation.
The Unreal project itself still needs imported .uassets/.umaps, a Windows
compile, and a real playtest. None of this work changes that.

## Troubleshooting

- **Baked art missing**: open devtools,
  `WP3DDiagnostics().bakedArt`. A checksum or placement error fails closed and
  leaves the previous battlefield playable.
- **Blender import fails**: check file type and re-export a simple, uncompressed
  triangulated GLB. Keep vertex count/geometry scale within the compiler caps.
- **Slow mobile performance**: Low never draws this scene. Keep Medium
  decimated. Test with WebGL2 real devices rather than treating CI SwiftShader
  software GPU results as real gaming GPU frame rates.
- **Facades float or overlap windows**: adjust the authored model, local origin
  and instance placement *before* baking. Visible solids must correspond to
  existing gameplay collision; the manifest alone is not collision.
- **External model requires CC-BY/SA**: retain full attribution and comply with
  share-alike requirements where applicable. The sample uses original art.
