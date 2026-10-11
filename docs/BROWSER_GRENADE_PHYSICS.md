# Physical browser frag flight

3D Frontline fragmentation grenades now travel from the soldier's throwing position with gravity, fuse timing, collision-aware wall rebounds, ground impacts and friction. The visual grenade follows its computed height until it detonates. Blast damage is blocked by solid cover and can harm allied fireteam members as well as enemies.

The existing low-cost game simulation and mobile controls are retained; no extra network services or renderer draw calls are required. The Node browser integration smoke asserts a hand-position launch, measurable flight, upward motion and fuse consumption. This is simplified projectile movement, **not** a full rigid-body or fragment-ray simulation. The real Chromium/SwiftShader FPS gate remains required before merging.
