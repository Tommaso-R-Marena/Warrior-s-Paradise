# Physically grounded High battlefield impact effects

This update to the free, Cloudflare Pages-hosted WebGL2 game adds **render-only bullet damage and ground impact effects**. No player needs a cloud GPU: the browser constructs a strictly bounded dynamic effect mesh using the existing single battlefield draw call.

## Implemented

- **Authoritative bullet collision registration.** Actual `WPInfantryBallistics.trace` reports the intersection with a collidable farmhouse or heightfield. Only a report of `blockedBy: masonry` may create a wall chip. Misses, living infantry hits, open air, or coordinates outside real building footprints are rejected. Building marks project onto the nearest genuinely collided AABB wall face with a thin offset to prevent z-fighting.
- **Small visible wall damage:** dark central pockmark, irregular chipped outer ring and small bright stone fragments. This is a visual decal, not a real crater cut out of wall collision mesh. It remains up to 52 simulated seconds.
- **Artillery / Sherman / grenade consequences:** a correctly terrain-sampled eight-facet darkened earth scar, finite-size hot fragments or stone chips, and ballistic fragment trajectories under the same approximate gravitational acceleration already used for ammunition simulation. This complements the existing bounded smoke and cannon streak effects.
- **No new physics obstacles:** scars and particles cannot stop bullets, players or tanks. Bullet damage itself is still resolved by real ballistic collision, not by the new visual layer.
- **High only** with strict caps: 42 bullet marks, 9 active explosion scars, 42 fragments, ≤954 maximum visual triangles with the existing dynamic VBO. Low and Balanced immediately return zero geometry.
- All changed surface materials are tagged to bypass the unrelated grass/terrain photographic atlas. State expires, is reset with a new battle, and remains deterministic without a randomness dependency.
- Diagnostics: `WP3DDiagnostics().combatSurface` reports active marks, explosions, debris, rejected invalid points, total registered hits, caps, and actual triangle count.

## Validation

Run `node tools/tests/browser_combat_surface_smoke.js` to check real masonry plane alignment, rejecting non-hits, visual buffer budgets, finite vertices, Low/Medium no-op behavior, expiry and reset.

The CI step `node tools/tests/browser_playwright_surface_impacts.mjs` enters an actual High match, uses `WP3DQA.shootOnce()` to shoot **real farmhouse collision**, verifies that target health remains unchanged, then triggers the game's actual artillery control and simulation, and checks the generated wall and earth effects, GPU errors and screenshots. The remainder of the existing real-browser tests verifies the armor, infantry, world geometry, licensed UV assets and paired Low/Medium FPS relative to main.

These are gameplay approximations and opaque procedural geometry, not actual brittle fracture physics, bullet-penetration simulation, particles with GPU instancing, or photogrammetry. The underlying world remains a stylized browser game; genuine high-fidelity historical infantry and vehicle asset conversion remains necessary for Enlisted-level appearance.
