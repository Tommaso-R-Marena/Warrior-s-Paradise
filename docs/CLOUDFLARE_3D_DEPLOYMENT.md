# Warrior's Paradise — Cloudflare 3D deployment decision

Updated October 8, 2026. The target is **a browser-playable, native-rendered 3D WWII skirmish** on devices with WebGL2, alongside the existing ultra-light software renderer. It is not yet an Enlisted/World War Heroes-quality full game.

## What is already deployed

The Cloudflare Pages project `warriors-paradise-field` uses:
- GitHub repository `Tommaso-R-Marena/Warrior-s-Paradise`
- Production branch `main`
- No build command; output directory `browser`

The new 3D game consists only of `browser/3d/index.html`, `style.css`, and `game.js`. No framework/CDN, paid compute, npm install or remote graphics instance is required.

Public entry points:
- **True 3D WebGL2 Preview:** https://warriors-paradise-field.pages.dev/3d/
- **2.5D Field Edition (low-end fallback):** https://warriors-paradise-field.pages.dev/

Cloudflare deploy status verifies upload, **not** real hardware rendering. Test the public page in a graphics-capable browser before advertising it as fully tested. The 3D code has a fallback link to Field Edition if WebGL2 is unavailable.

## What "runs completely on Cloudflare" means — and what it doesn't

| Component | Cloudflare alone? | Reality |
|---|---|---|
| Serve 3D game HTML/JS | **Yes** | Pages/Workers static assets deliver globally |
| Serve model and texture downloads | **Yes** | Pages for smaller assets, R2 for larger assets if needed |
| Run 3D geometry, gameplay and AI on player phone/laptop | **Yes, via the browser** | Game executes locally; Cloudflare serves files only |
| Use a computer without a discrete GPU | **Often** | Integrated graphics with WebGL2 can run simplified, optimized scenes |
| Run on a device with *no available graphics acceleration or WebGL2* | **Fallback only** | Software 2.5D edition has less demanding hardware |
| Run Unreal Engine 5.8 at desktop cinematic fidelity *on Cloudflare Pages* | **No** | Requires a separate GPU-equipped runtime |
| Stream full Unreal frames to low-power browsers | **Hybrid only** | Unreal Pixel Streaming on GPU server + Cloudflare frontend |
| Free unlimited high-end GPU streaming to thousands | **No** | Compute, encoding, TURN, bandwidth and concurrent sessions are material costs |

Cloudflare Workers, Pages, R2, Durable Objects and optional WebSockets can provide backend services, but a static Pages deployment does not include an Unreal-capable GPU host. Do not imply Cloudflare's AI inference offerings supply a general-purpose realtime UE renderer.

## Runtime features in 3D Preview

The 3D client uses **native WebGL2** rather than Canvas raycasting:
- a compiled GLSL vertex/fragment shading pipeline
- true perspective camera, per-face lighting, fog and depth-buffered 3D meshes
- procedural terrain, roads, farm buildings, roof geometry, wooded areas and supplies
- basic animated infantry representation, collision, cover occlusion and hitscan
- three infantry weapons (Lee-Enfield, Sten, Bren), reloading, ADS, grenades, healing
- capture three sectors by defeating defenders and occupying objectives
- desktop mouse/pointer-lock and WASD, mobile left-stick/right-look and combat buttons
- Low / Balanced / High resolutions, adaptive internal rendering scale
- no sign-in or game server; single-player local game simulation

**Visual expectations:** this is original simple low-poly 3D geometry. Full Enlisted-quality character animation, vehicle gameplay, authored assets, sound design, complex shading, multiplayer and networking remain open projects. Only the native Unreal source project targets those assets and gameplay systems.

## Verification

The dependency-free Node test is:

```bash
node tools/tests/browser_webgl3d_smoke.js
python -m unittest tools.tests.test_browser_webgl3d -v
```

The Node suite drives a mocked WebGL2 interface for 250 frames in portrait and landscape, verifying draw calls, weapon firing/reloads and control actions; and checks the no-WebGL fallback. It is **not a visual quality or GPU performance benchmark**. Tests are also included in `tools/validate_project.py` and the repository CI workflow.

Real acceptance requires Chrome Android, Safari iOS and a Windows laptop with integrated graphics. Test WebGL context creation, resizing, touch layout, frame pacing, audio activation and full mission completion on devices.

## How we can reach much higher fidelity

**Track A: locally rendered high-end WebGL/WebGPU client.** Export licensed optimized glTF/GLB meshes and animations from Blender/Unreal, use texture compression (KTX2/Basis), GPU instancing, streaming LODs, baked lighting, navigation meshes and spatial audio. Keep asset bundles within mobile memory budgets and profile 30/60 fps. Cloudflare can distribute that content nearly free at small scale, subject to plan limits.

**Track B: Unreal Pixel Streaming.** Compile/cook the actual UE game on Windows/Linux with the matching UE5.8 Pixel Streaming infrastructure. Run each concurrent game session on a GPU host (own computer, university/hardware donation, or commercial GPU VM). Put the landing page, account, session routing and cached public assets on Cloudflare, but connect browsers to WebRTC signaling/media on the GPU host or TURN. Costs and scaling must be measured; no assumption of free unlimited servers. The official UE5.8 infrastructure is https://github.com/EpicGamesExt/PixelStreamingInfrastructure (UE5.8 branch).

Do **not** connect random user devices to an exposed local desktop without access control, signaling authentication and bandwidth/rate limits.

## Next milestones

1. Prove actual `/3d/` loading and full playthrough on physical devices; fix severe regressions.
2. Replace static infantry stand-ins with genuine skeleton animations, then weapon/vehicle assets.
3. Improve combat hit detection, AI navigation, cover placement and game modes with automated deterministic tests.
4. Add art-streaming compression and continuous frame-time budgets.
5. Package and verify native UE5.8 on a real GPU-equipped builder in parallel.
6. Decide whether Pixel Streaming is affordable/useful for a limited high-fidelity demo after estimating concurrent users and GPU cost.

No marketing claims of Enlisted-quality fidelity until independently verified visuals, animation, controls, content and measured frame rate match the benchmark.
