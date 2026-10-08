# Warrior's Paradise — Field Edition

**A playable, lightweight browser skirmish for phones and low-power computers.**

This is deliberately a **separate implementation**, NOT the full Unreal Engine 5.8 game and not a streamed desktop build. It uses textured Canvas 2D raycasting and procedural art (still not full mesh-based 3D), with no WebGL dependency, Unreal Engine download, external JavaScript framework, paid backend, signup or account. Visuals, map, squad size and weapon fidelity are simpler than the UE project.

## Play from the repository

Easiest: open `browser/index.html` in a modern desktop browser. No server is required for the game logic.

With Python 3, from the repository root:

```bash
python tools/serve_field_edition.py
```

Then open `http://127.0.0.1:8000/`. This command serves **only** the `browser/` folder (not the whole private source tree).

To play on a phone connected to the **same trusted Wi-Fi** as the computer, run:

```bash
python tools/serve_field_edition.py --lan
```

Open `http://<COMPUTER-LAN-IP>:8000/` on the phone. If your OS firewall blocks port 8000, allow the local Python server on a **private network only**. This is local unencrypted HTTP, and does not automatically enable offline installation. For installability use the HTTPS Pages URL below. To stop, Ctrl+C.

## Browser controls

| Device | Controls |
|---|---|
| Desktop | WASD movement, **mouse horizontal/vertical look**, click/hold fire, **right-click ADS**, Shift sprint, R reload, F frag, T smoke, C crouch, V heal, Q squad-switch, X follow/hold orders, Esc pause |
| Phone/tablet | Left thumbstick moves; drag on the right to look **left/right/up/down**; FIRE, **hold ADS**, reload, frag, smoke, field dressing, switch soldier and follow/hold buttons |
| Any | Choose **Normandy Farm / Occupied Town / Forest Outpost**, **Assault / Last Stand**, Recruit / Regular / Veteran enemies, aim sensitivity and ECO / Balanced / Sharp / **Ultra** graphics. ECO remains default on touch devices |

In **Assault**, capture three sectors by occupying each flag after defeating defenders. In **Last Stand**, survive five progressively larger waves and obtain ammo, bandages and limited explosives between waves. Choose from three tile-layout battlefields. Your three-person squad uses Lee-Enfield / Sten / Bren roles and can provide finite-ammo covering fire; the medic can treat wounded allies. Enemy AI uses line-of-sight, recent-contact investigation, basic combat movement and reloading. The Field Edition is still a small single-player skirmish and **not** feature- or graphics-equivalent to World War Heroes.

## Offline install / home screen

On an HTTPS deployment (or localhost), the `manifest.webmanifest` and `sw.js` make static resources cacheable after the first successful visit. Use the browser's **Add to Home Screen** or **Install App** action if supported.

Opening `index.html` via `file://` plays locally but cannot register a service worker. LAN HTTP also does not normally qualify for service workers; use HTTPS or localhost. Refresh at least once online before expecting offline reuse. Incognito/private browsing can discard the cache.

## Free public hosting with Cloudflare Pages

A dedicated Cloudflare Pages project **warriors-paradise-field** has been created with a GitHub link to the repository. Settings:

- Production branch: `main`
- Repository: `Tommaso-R-Marena/Warrior-s-Paradise` (the repo stays private)
- Build command: none (static)
- Build output folder: `browser`
- Project address after successful deployment: `https://warriors-paradise-field.pages.dev/`
- No Worker or GPU server needed. Browser clients do all simulation/rendering locally.

If the first deployment does not appear, open Cloudflare **Workers & Pages → warriors-paradise-field → Deployments** and review the build source connection; a `main` commit touching `browser/` can trigger a Git deployment. Do not advertise the URL as live until Pages reports success and a real browser loads it. Cloudflare Pages Free has published plan quotas, and deploys a static site without GPU compute.

## Relation to native Android / iPhone UE

The native UE 5.8 project is a **different** build. Epic supports Android packaging with a matching SDK/NDK, compatible phone graphics hardware and the mobile renderer. iPhone native packages require a macOS/Xcode signing workflow. The existing UE project has not yet been built or profiled for either phone platform; it should not be treated as a ready-to-install APK/IPA.

UE **Pixel Streaming** also permits phone/browser access to the full desktop edition, but requires a rendering PC or GPU cloud server, which is not provided by static free Pages hosting. The browser edition avoids that dependency by using its own lighter game simulation.

## Hardware honesty

- The browser edition is **CPU-first**, but the OS/browser may still use an integrated GPU for canvas composition. It does not require a discrete GPU.
- ECO targets ~225 internal horizontal pixels / 28 fps; Balanced ~330 / 35 fps; Sharp ~440 / 45 fps; Ultra ~640 / 58 fps. These are **configuration ceilings, not measured device FPS**. Canvas pixel budgets respect portrait/landscape ratio; a bounded adaptive scale lowers internal resolution when rendering cannot sustain its target. Procedural stone/timber textures, perspective grass/mud/road casting, tactical smoke and soldier billboards use Canvas 2D.
- Performance varies by device. If the browser or phone cannot run JavaScript Canvas 2D, it cannot play this game. Phones that support modern browsers generally can, but have not been device-tested here.
- This version is local single-player: no account, global multiplayer, or server synchronization. A static server cannot host the full native UE physics/AI simulation.

## Testing

The dependency-free Node runtime smoke command is:

```bash
node tools/tests/browser_runtime_smoke.js
```

It simulates portrait and landscape sessions and verifies ADS/fire/reload/grenade/squad controls, vertical touch look, each battlefield, Ultra rendering and a complete five-wave victory progression. It checks drawing operations, not actual image quality. It is **not** a real browser image review or phone performance benchmark.

The native Android plan is in [`docs/MOBILE_RELEASE_PLAN.md`](../docs/MOBILE_RELEASE_PLAN.md). The [mobile parity scorecard](../docs/MOBILE_PARITY_SCORECARD.md) compares actual functionality to the commercial reference and names the remaining acceptance tests.

## What remains

Actual browser-device QA (iPhone/Android/old integrated PC), audio balance, more levels/weapon classes, collision/pathfinding polish, artwork/audio licensing and controller support. The desktop Unreal compilation is a separate acceptance gate.
