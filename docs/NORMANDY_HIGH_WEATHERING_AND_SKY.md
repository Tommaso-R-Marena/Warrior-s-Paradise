# High-quality Normandy visual-weathering release

This release improves the **free, locally rendered Cloudflare WebGL2 game** without a GPU-backed server. All improvements are procedural/original; none should be described as photogrammetry, AAA soldier/vehicle art, or physically simulated volumetric lighting.

## Added scenery

- Nine existing collidable farmhouses gain High-only irregular masonry loss, uneven patch colors, drainage streaks beneath windows, thin downpipes and coarse mortar, plus climbing ivy.
- Wall detail geometry attaches to the original building coordinates/footprints, not a separate player collision model. Thus there is **no new invisible bulletproof cover**, no discrepancy between Low/Medium collision and High rendering.
- Only a bounded number of non-alpha triangles are added to the existing static WebGL2 buffer/draw call (2,350 max). Test asserts all authoring points lie within the building's physically collidable footprint plus ≤0.40m facade trim.
- Negative-red wall pigments bypass the existing terrain/brick photographic tile atlas; the original High foliage wind shader now excludes those tagged, rigid wall polygons. Leaf triangles remain subtly wind responsive.
- The High atmospheric-sky shader adds a directionally consistent sun aureole, Rayleigh-like blue sky gradient, warm forward-scattering veil and bounded layered stratocumulus. The sky still renders in one opaque fullscreen triangle with no remote textures, depth-write corruption or additional framebuffers.

## Tests and limitations

- `node tools/tests/browser_architectural_weathering_smoke.js`: deterministic geometry, finite points, physical footprint alignment, ivy counts, material tags, triangle budget and no new colliders or draw calls.
- `node tools/tests/browser_graphics_fidelity_smoke.js`: one-pass shader creation and GL state restoration.
- `node tools/tests/browser_playwright_weathering.mjs`: actual Chromium/SwiftShader screenshot, High shader compilation, diagnostics and WebGL error check.
- The existing real-browser tank, ballistic, terrain and Low/Medium paired FPS gates also run. High software GPU FPS is **not** a reliable GPU-accelerated gaming FPS measurement.
- The weathering is stylistically motivated, not a reconstruction of specific Normandy buildings, and the clouds are an artistic analytic approximation rather than raymarched volumetric fluid simulation.

The real next leap to *Enlisted*-like realism depends on higher-quality licensed character and vehicle assets, animation systems, measured actual GPU performance and broader world art production, which this patch does not claim to solve.
