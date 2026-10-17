# High Normandy farmhouse craftsmanship and actor-render optimization

This update improves the browser/WebGL2 version of Warrior's Paradise. Cloudflare Pages still hosts **only static assets**. Every player's device renders the world locally, and Low/Balanced tiers preserve their existing visual pipelines.

## High-only farmhouse detail

The nine original collidable farmhouse bodies receive original, manually parameterized appearance detail aligned to the existing openings:

* Four-pane tinted window inserts, overlaid wood glazing mullions and exterior window frames.
* Distinct timber plank shutters with hinges and cross battens (72 shutters).
* Thin stone lintels and drip ledges, front/rear doorway threshold trim and 18 chimney pots.
* The earlier weathered plaster, mortar, ivy, photographic ambientCG texture atlas and dedicated PBR UV/lightmap system continue unchanged.

The fittings add at most 2,400 geometry triangles, **no additional draw pass** and **no collision bodies**. Signed-red weathering and colored wood trim are classified as static non-wind geometry by the existing High shader, preventing imported terrain photo maps from painting window glass. The added glazing is opaque colored procedural geometry: it is not transparent raytraced glass or historically surveyed architecture. Models are original, not unlicensed photogrammetry.

## High render-only view culling

The previous renderer could generate animation geometry for all on-foot infantry, including those far behind the camera. The new `WPActorVisibility` module conservatively skips drawing actors more than 10m away whose horizontal bearing lies over 120 degrees from camera forward, and actors over 90m away. Actors inside 10m **always render**, even directly behind the player. A generous frustum margin accommodates wide desktop screens and player yaw.

This culling does **not** remove soldiers from gameplay. It does not change enemy AI, tactical decisions, health, shooting, line of sight, hit registration, or collision. The skinning budget now prioritizes actors who could actually enter the visible frame. Low and Medium keep their previous model rendering behavior. Relevant `WP3DDiagnostics()` counters show `visibleActors`, `culledActors`, and `farmhouseCraft`.

## Tests

* `node tools/tests/browser_farmhouse_craft_smoke.js` validates geometry counts, authoring determinism, material tags, existing building footprint and finite coordinates.
* `node tools/tests/browser_actor_visibility_smoke.js` validates conservative directional culling, fail-visible nonfinite inputs, visible close threats, and unchanged Low/Balanced.
* `node tools/tests/browser_playwright_farmhouse.mjs` checks the rendered scene, screenshots, original combat-play state and actual WebGL2 errors.
* Existing Chromium tests independently cover infantry ballistics, Sherman/Panzer controls, terrain physics, UV/PBR lightmaps, sky, Low/Medium paired rendering and field art.

The graphical model remains **stylized**; it does not equal Enlisted's licensed WW2 characters, skeletal animation, destructible level architecture or AAA materials. These changes are deliberately low-infrastructure upgrades with bounded geometry and no recurring server GPU costs. Software-rendered CI FPS should not be interpreted as gaming GPU or smartphone performance.
