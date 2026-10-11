# WWII visual fidelity and real-device GPU acceptance

## Source assets and attribution

The source packages are acquired by `tools/assets/vendor_ww2_open_art.py`, transformed by pinned system Blender, and audited in `browser/3d/assets/oga/WW2_MANIFEST.json`. Each manifest item includes the source webpage, original archive SHA-256, derived GLB SHA-256, runtime path, license, creator and animation count.

- **US Allied soldier:** source character by **knik1985**, [Character: US soldier](https://opengameart.org/content/character-us-soldier), **CC BY 3.0**. This project transforms the original geometry to a 29-bone glTF-compatible skin and authors four deterministic *replacement* clips on that skin. Do not describe these as motion-captured or source-authored.
- **German WWII soldier:** **nisu**, [Rigged Lowpoly WW2 Soldier](https://opengameart.org/content/rigged-lowpoly-ww2-soldier), CC0. A valid source skin is required; movement falls back to runtime bone poses if the supplied model lacks compatible clips.
- **Willys MB Jeep:** **Aligned Games**, [Willy's Jeep Asset](https://opengameart.org/content/willys-jeep-asset), **CC BY-SA 3.0** (alternative GPL v3 offered by creator). The derived simplified Willys mesh is adapted and remains **CC BY-SA 3.0**. This project removes the original high-resolution textures when exporting the browser LOD to limit bandwidth; this is a disclosed modification. Original attribution and the source are linked from the in-game menu. The surrounding game code is not itself relicensed.
- **M4 Sherman and Panzer IV:** bespoke game-built silhouettes distinguish the Allied rounded-turret Sherman from the German box-turret Panzer IV and include bogies/track runs. These are historically informed representations, **not dimensionally verified restoration drawings**. Generic tank fallback (hubahuba CC0) remains in the local asset collection.

The source uses no material that requires an API key, paid account, CDN runtime access, or proprietary Epic content.

## 30-second real-device graphics benchmark

The **F3** PERFORMANCE LAB panel works in Chrome, Firefox and compatible mobile browsers. With WebGL2 enabled and the player deployed:

1. Choose LOW, BALANCED or HIGH on the deploy screen, then play normally for at least 15 seconds to warm shaders and finish loading models/textures.
2. Press **F3** (or enable the panel using an attached keyboard on mobile), choose **CAPTURE 30S ON THIS DEVICE**, and move/turn/shoot and interact with vehicles consistently across runs.
3. After 30 seconds, click **EXPORT JSON RESULTS**. Only this explicit action downloads the report; no usage, hardware information, or telemetry is transmitted to our servers.
4. Repeat three times per quality tier, record whether device thermal throttling or background processes were present, and compare the median **1% low FPS**, **p95 frame time**, average render CPU time, **GPU mean/p95 time**, scaled internal resolution and device renderer.
5. If the report says `gpu.supported=false`, the browser did not expose `EXT_disjoint_timer_query_webgl2`. GPU time is **unknown**, not zero. Never claim that the GPU is fast or slow from CPU frame time alone. Disjoint/invalid measurements are discarded.

The GPU timer is sampled no more than once every 12 frames and holds at most three pending queries; it avoids `gl.finish()` and does not block the render loop. The query extension may be hidden by privacy settings. Device-specific GPU/renderer data appears only in the locally exported JSON.

### Acceptance targets, not measured results
- Low-phone gameplay should target sustained **30 FPS** on an ordinary midrange Android handset, with p95 frame time < 50 ms and no persistent motion blocking.
- Medium desktop should target **45+ FPS** on an integrated GPU, with p95 < 33 ms and no unexpected contextual shader failure.
- High may be unavailable on weaker devices; adapt scale rather than introducing costly additional rendering passes.
- These targets require measurements on **physical devices**. GitHub Actions SwiftShader is primarily a regression detector and does not certify real hardware.

## Terrain and lighting

The High-only render tier overlays a finite, static tessellated rural terrain mesh using deterministic broad/fine noise, slight height variation, and directionally baked building shadows. Roads, building footprints and capture sites remain at their original gameplay collision elevation. The High shader combines warm direct sunlight with cooler skylight; Low and Medium retain their existing tested shaders, collision, and static geometry prefixes. This is not dynamic per-object shadow casting, GPU lightmapping, global illumination, or a complete heightfield collision engine.

## Known limits and next physical test

Animations exported from older Rigify sources are adapted and are not full professionally authored locomotion cycles. Vehicle suspension, wheels, turret/hull rotation, ballistics, and historical dimensions are approximate. The native Unreal Engine 5.8 target still needs an installed engine and cooked-platform test. Mobile touch gameplay and GPU performance require testing on the actual handset rather than assuming a Windows software Chromium result generalizes.
