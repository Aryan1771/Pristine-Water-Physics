# Validation — Pristine Water Physics 0.6

Validated on Windows 11, UE 5.7.4, Intel i7-14650HX and RTX 4060 Laptop GPU, 14 October 2026. Development Editor and Game targets compile. Thirteen automation tests pass (13 successes, zero warnings, zero failures in the final run). The 0.5 validation and historical CPU measurements are retained in [VALIDATION-0.5.md](VALIDATION-0.5.md); those timings are not benchmarks of the new solver.

## Physical changes and evidence

Region now evolves conserved depth and both horizontal momentum components with first-order HLL finite-volume fluxes. Hydrostatic reconstruction supplies terrain pressure balance. Closed walls reflect normal momentum, and shared donor flux limits protect nonnegative depth without allowing independent face exchanges to create water.

The existing conservation, lake-at-rest, dry-bed wetting, barrier, splash, terrain-bake, thermal expansion and SPH containment tests still pass. Additional tests:

- **RitterDamBreak:** comparison with the analytical one-dimensional dry-bed dam-break depth profile at one second in a 40 m domain. Depth L1 integral error falls from 0.359252251 m2 at 64 cells to 0.255414246 m2 at 128 cells. The fine-grid domain-average absolute depth error is approximately 0.00639 m against the 1 m upstream reference. This is a limited refinement check, not a general convergence certification.
- **MomentumTransport:** a transverse momentum pulse carried by a 1 m/s flow travels 0.500000000 m in 0.5 seconds in the tested interior region. The former local-inertial solver omitted this transport.
- **VolumeWeightedMomentum:** liquid removal carries proportional momentum and returning volume adds momentum proportional to that volume. This fixes the previous droplet landing behavior that applied a full-cell velocity impulse.
- **CinematicPreset:** the new material loads, the calm surface has low roughness and correct dielectric scale, the preset preserves Region volume, shore foam stays disabled after reset, and Source retains geometric waves with a 60 Hz custom visual rate.

## Rendering

The cinematic material was generated through the editor commandlet with PRISTINE_ASSETS_RESULT: SUCCESS and no material compile errors. It uses dielectric F0 and delegates angular Fresnel behavior to Unreal's BRDF. It avoids the old extra view-angle boost and diffuse water tint. It retains Single Layer Water absorption/scattering and controllable foam.

The maximum-quality demo uses hardware Lumen reflections with ray-hit lighting, full-resolution reflection settings and reflection screen traces disabled for verification. Actual captures are provided in the repository documentation. These show the tested renderer integration; they do not certify spectral transport, caustics or path-tracer parity.

## Scope

This release targets convincing real-time water, particularly high-quality reflective large surfaces. Large regions remain depth-averaged: they do not model overturning waves, resolved spray sheets or full 3D turbulence. First-order HLL is numerically diffusive. Fine terrain/shore features require sufficient resolution. The SPH solver remains experimental and is unchanged in this pass.

No multi-platform qualification, cooked Shipping launch, multiplayer replication, full-scene GPU performance guarantee or simulation-cache/Movie Render Queue determinism is claimed. Maximum-quality mode can exceed real-time budgets on some scenes and GPUs; tune it with actual CPU/GPU profiling.

## Walking, swimming and reference scene update — 14–15 October 2026

- The final HostProjectEditor and HostProject Win64 Development targets built successfully on UE 5.7.4. `Test-Unreal.ps1` passed all 13 tests, including the new CharacterTransitions regression. This checks deep entry, underwater state, shallow exit, gravity restoration, nonblocking water, absence of surface droplets from deep divers, a splash while walking through a 4 cm puddle, the underwater material reference, renderer immersion queries above water, below water and below the bed, and an identity default for blending out the underwater effect.
- The final material commandlet completed with zero errors and zero warnings. Surface back faces do not apply absorption to the air behind the interface; underwater camera attenuation handles the immersed view path. A world view extension supplies immersion depth to Unreal, correcting the black-sky result when viewing upward through the lake.
- An assembled low-quality MetaHuman is saved in the private host project. Actual DX12 Play in Editor captures confirmed the attached character, swimming/underwater state in the lake, walking on shore and a visible droplet instance during puddle movement. This does not certify production animation quality or multiplayer behavior.
- The alpine extension was generated and rendered in the same map. It adds original procedural geometry and references the engine's bundled cloud material. It retains the existing lake solver and bathymetry. The lake photograph supplies art direction; the scene is not a scan or an exact landscape reconstruction.
- The host project enables ray tracing and hardware Lumen. No new whole-scene FPS benchmark is claimed; cloud rendering, the MetaHuman and the terrain add GPU costs beyond the solver. MetaHuman source assets are not distributed in this repository.
The final DX12 five-station Play run on 15 October completed with `PRISTINE_PLAY_VALIDATION_SUCCESS`. The lake case reported swimming and underwater state; the shore case reported neither; the 4 cm puddle case recorded a splash instance without swimming; all five cases retained the assembled MetaHuman. Ray tracing and hardware Lumen both reported enabled. The subsequent terrain-only correction buries the cropped mountain boundary into the retained ground, removing exposed back faces beside the smaller comparison stations.
