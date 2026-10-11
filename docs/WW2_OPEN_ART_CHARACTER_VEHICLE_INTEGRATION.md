# Historical WWII artwork and vehicles — self-hosted browser slice

Source verification: OGA creator/license pages were reviewed individually. The converted binary GLBs are checked into `browser/3d/assets/oga` as Base64 for static Cloudflare Pages transport. `WW2_MANIFEST.json` hashes both the original creator archives and exported files; Python CI checks the SHA-256s and required glTF runtime extensions. Source packages are downloaded at build time using a capped size and verified archive entry rules.

| Model | Publisher | License | Gameplay |
| --- | --- | --- | --- |
| German WWII uniformed soldier | nisu, OpenGameArt | CC0 | Axis rig, High graphics path |
| Allied/US paratrooper | knik1985, OpenGameArt | CC BY 3.0 | Optional rig; displayed only if a valid skin can be exported, retained original creator credit |
| GAZ-67 Soviet 4×4 (1943–53) | artie31, OpenGameArt | CC0 | Drive and dismount with E / mobile VEHICLE |
| Low-poly tank study | hubahuba, OpenGameArt | CC0 | Drivable friendly armored platform and visible enemy armor; it is **not** a historically verified Panzer IV or Sherman model |

Rigging and animation: Axis GLB is genuinely skinned; fallback bone gait is synthesized by the deterministic project runtime if an original clip is absent. US/paratrooper, if successfully imported, offers its creator-supplied animation clips chosen by state. Else the existing Kenney skeletal rig/fallback is used. These are **not** professional motion-captured animations, complete cloth simulation, verified weapon holding poses, or uniform-grade history documentation.

Vehicle simulation: lightweight acceleration/steering and terrain/building collision, boarding/dismounting with spatial checks, fixed reload/cannon timing, enemy LOS and range gates. The GAZ-67 in Normandy is an illustrative Soviet model **not a historically accurate American/British issue vehicle for the scenario**. The tank is a stylized study, not a documentary-accurate M4 Sherman. Armored combat is demonstrative, not advanced ballistics/damage-simulation fidelity.

Performance: each vehicle is reduced offline to about 1700 mesh triangles, imported dynamically only if nearby and budget/graphics tier permit; phone quality uses the lightweight fallback. AI, static world and skin processing remain entirely local. This is an actual 3D browser vertical slice, not a UE5 native build and not photorealistic.

Artist pages:
- https://opengameart.org/content/rigged-lowpoly-ww2-soldier
- https://opengameart.org/content/character-us-soldier
- https://opengameart.org/content/lowpoly-soviet-jeep
- https://opengameart.org/content/tank1

The next historically authentic stage should prioritize licensed M4 Sherman, British Universal Carrier, German Panzer IV, allied riflemen uniforms, first-person Lee-Enfield/Bren/Sten animation libraries, and real terrain/lighting/lightmaps, accompanied by formal human review of historical sourcing.
