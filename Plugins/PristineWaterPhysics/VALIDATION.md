# Validation — Pristine Water Physics 0.6

Validated on Windows 11, UE 5.7.4, Intel i7-14650HX and RTX 4060 Laptop GPU, 9 October 2026. Development Editor and Game targets compile. Twelve automation tests pass. The 0.5 validation and historical CPU measurements are retained in [VALIDATION-0.5.md](VALIDATION-0.5.md); those timings are not benchmarks of the new solver.

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
