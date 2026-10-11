# Battlefield visual and tactile enhancements

This feature adds ground discoloration after artillery and fragmentation explosions and short-lived broken-up dust plumes. All shapes share the already existing dynamic WebGL draw; no transparency or third-party runtime library is introduced.

There are at most seven active blast records. Low graphics draws only four matte scar wedges per blast. Medium shows two short dust cones per nearby blast and High four, while both use eight scar wedges; distant effects are culled and all blast records expire in 4.2–7 seconds. These are intentionally simple opaque geometric approximations, **not photoreal volumetric smoke or AI-obscuring cover**.

Also included are movement-linked camera and weapon bob, weapon-lowering during reload, distance-scaled artillery/grenade concussion, and gentle High-quality-only vegetation sway. The latter uses a High-only vertex program; Low and Medium retain their existing shaders. All simulation and assets remain client-side and Cloudflare Pages-compatible.

Automated smoke coverage checks bounded effects, expiry, distant culling, artillery and grenade spawning. Real Chromium FPS tests and physical-device validation remain the hard performance gates. Do not claim AAA realism or sustained 60 FPS.
