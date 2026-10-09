# Validation — Pristine Water Physics 0.5

Validated 9 October 2026 on Windows 11, UE 5.7.4 (CL 51494982), Intel Core i7-14650HX, NVIDIA RTX 4060 Laptop GPU and MSVC 14.51.

## Build and regression checks

HostProjectEditor and HostProject Win64 Development builds succeed. The runtime and editor modules compile through Unreal Header Tool. Existing engine material-expression deprecation, include-order and non-preferred compiler warnings remain. No cooked/shipping game launch has been certified.

Eight automation tests pass, zero fail. The eight test names and their coverage are listed below. Run the repository test script to generate a fresh machine-local report.

- ConservationAndDryBeds: uneven-bed equilibrium, wetting, positive depth, volume conservation and impermeable walls.
- RegionAndSplashes: splash volume transfer/return, exterior queries, water-level reset and procedural geometry.
- SmallVolume: particle/mass preservation, finite state, closed-box containment and reconstructed skin.
- TerrainBake: selected collision geometry, expected volume, below-bed queries and stale-bake protection.
- SourceAndMaterial: material reuse, clamps, mesh budgets, buoyancy, analytic ripple queries and restoring flat geometry.
- FreshWater: reference density/viscosity/surface tension, density maximum near 4 C, temperature bounds and decreasing hot-water viscosity.
- Adaptation: synthetic CPU timing lowers visual refresh, reports an unmet solver budget and honors manual/custom settings. This is a controller regression, not a hardware FPS guarantee.
- ConservationAndPressure: temperature changes preserve liquid mass; quality changes preserve mass/count/region volume; hydrostatic pressure matches rho g h; flat region mesh normals point upward; modified SPH remains finite and contained.

## CPU microbenchmark

Same machine, Development Editor, NullRHI (rendering disabled), five warm-up calls followed by 60 AdvanceSimulation calls at 1/60 second. Region uses 48 cells per axis and 60 surface updates/second. Small Fluid uses 216 particles, surface resolution 32, a 20 Hz mesh cap, no scene collision and zero tangential wall damping. Adaptive visuals are disabled for comparison. Startup and initial mesh construction are excluded; Python call overhead is included. Scripts are under Tools/BenchmarkBefore.py and Tools/BenchmarkAfter.py.

| Workload | 0.4 mean ms/call | 0.5 mean ms/call | Reduction |
| --- | ---: | ---: | ---: |
| Region, 48 x 48 | 40.164 | 1.178 | 97.1% |
| Small Fluid, 216 particles | 33.493 | 26.979 | 19.5% |

These are one-run microbenchmarks, not gameplay frame times or cross-PC predictions. Mesh construction occurs intermittently for Small Fluid, so its individual updates can spike above the mean. The measured solver cost alone can exceed a 3 ms budget; adaptive visuals cannot eliminate solver cost. Performance varies with particle distribution, collisions, surface resolution, object count, background load and GPU rendering.

The optimizations remove generic region tangent reconstruction, move splash instance updates out of physics substeps, retain scratch buffers, reuse SPH neighbor pairs, and scatter reconstruction kernels only to nearby grid samples. A linked-list neighbor representation was rejected after it regressed the benchmark; the final implementation retains contiguous bucket arrays.

## Rendering checks and limits

The 0.4 material generation and DX12/SM6 ray-tracing checks remain applicable; materials are unchanged. The 0.5 updated meshes were additionally captured under DX12/SM6 with hardware ray tracing, Lumen hardware tracing and reflection screen traces disabled. The pool/splash and small-fluid images are integration checks, not photorealism certification. Thin particles can remain visually unresolved and the SPH skin can look lumpy.

No full GPU timing/scalability study, many-actor stress test, numerical convergence study, multiplayer test, path-tracer parity or platform matrix is included. CPU adaptive quality is per actor; it does not measure full-frame GPU cost or guarantee Target FPS. Auto uses a coarse hardware heuristic, not a GPU benchmark.

This remains an experimental approximation. Large-scale fluid uses local-inertial shallow water without momentum advection; the ocean source is analytic; small fluid uses artificial compressibility and approximate cohesion. Temperature is a prescribed uniform property input, not a thermodynamic solver. Heat transfer, phase changes, evaporation, capillary wetting, bubbles, acoustic propagation, spectral caustics and exact two-way fluid/rigid coupling remain unsupported. Conserved particle mass does not guarantee exact visual isosurface volume.

The GitHub repository is source-first and excludes editor DLLs. Rebuild against your own Unreal installation. The measurements above are from the local validation build.
