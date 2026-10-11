# Obstacle-aware browser infantry (October 2026)

The Cloudflare WebGL2 battle uses deterministic, dependency-free grid A* **only when a combatant actually stalls** against cover, a building or an imported collision prop. A soldier on unobstructed ground keeps the original direct movement. The same non-penetration `blocked` and `move` callbacks enforce collision for every step, including diagonals. Routes are cached per soldier; moving targets invalidate stale plans, and repeated failures are throttled.

The planner is limited to a 61 × 61 grid, 1.5 m cells and 1,050 expansions per plan. It never performs A* every animation frame, never reads a hidden enemy's live location, and does not add a server, network dependency or extra WebGL draw calls. Following squad members route to their desired formation position; alerted defenders route toward the **visible target or retained last-known position** already established by the AI simulation.

Diagnostics expose `navigation.{plans,successful,failed,expanded}`. Node regression tests cover unobstructed movement, routing around a solid wall, no obstacle penetration, no wall tunnelling from enclosed origins and bounded planner work. Browser Chromium CI remains the performance gate. This is simple ground-plane routing, **not** a production navigation mesh, sophisticated fireteam cover selection or a proof of 60 FPS on physical hardware.
