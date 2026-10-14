# Changelog

## Unreleased

- Added an optional alpine lake extension inspired by a user-supplied reference, with original terrain, shore rocks, meadow vegetation, snow patches and a comparison camera.

- Added a walking/wading/swimming test character, region-aware immersion, puddle/wake impulses, first/third-person camera and optional MetaHuman visual/animation slots.
- Added two-sided water surfaces, renderer immersion reporting and a depth-based underwater attenuation material that fades back to an identity effect. Corrected the comparison scene to use optical reference distance independently of basin depth.
- Added a character immersion/exit and nonblocking surface regression, plus a reusable scene upgrade script and controls guide.

- Added a reproducible single-map woodland comparison with puddle, pool, pond and lake regions, terrain collision, original vegetation, a trail, signs and five cameras.
- Added editor mesh-authoring helpers and guides for water scale, settings, performance and useful free Unreal assets.
- Corrected the test runner to count Unreal's successful-with-warnings results as completed successes while still surfacing warnings and rejecting failed/incomplete tests.

## 0.6.0

- Replaced local-inertial region flow with conservative HLL momentum transport and hydrostatic terrain reconstruction.
- Corrected droplet return momentum to be weighted by incoming liquid volume.
- Added an analytical dam-break refinement test, momentum transport test and volume/momentum exchange test.
- Added M_PristineCinematic and Apply Cinematic Preset with low-roughness dielectric reflections and restrained calm-water appearance.
- Added roughness controls, persistent shore-foam controls, maximum-quality renderer guidance and a reproducible reflection-demo script.
- Twelve passing regressions and Development Editor/Game builds; refreshed ray-traced visual checks.

## 0.5.0

- Auto/Low/Balanced/High/Custom performance presets across water actors.
- Adaptive per-actor CPU visual update budgets and visible diagnostics.
- Freshwater density/viscosity/surface tension versus prescribed temperature, with mass preservation.
- Hydrostatic pressure queries and small-fluid wall damping.
- Faster region normal generation, batched splash instance updates, reusable solver buffers, cached SPH pairs and compact-support surface-field construction.
- Eight regression tests, local Editor/Game builds and DX12 ray-tracing captures.
- Source-first GitHub packaging, Unreal tutorials, control manual, cinematic notes and build/test scripts.
- Original plugin code/assets licensed under MIT with repository-owner authorization, replacing the repository's initial GPL license.

## 0.4.0

- Terrain-bounded shallow water, spline boundaries, landscape baking and water-level tools.
- Splash volume transfer, sphere buoyancy/drag and experimental small-volume SPH.
- Ray-visible procedural water surfaces and original demo/material assets.
