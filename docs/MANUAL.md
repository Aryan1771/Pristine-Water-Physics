# Controls and integration manual

See the [detailed plugin reference](../Plugins/PristineWaterPhysics/README.md) for the numerical models and [legacy Source controls](../Plugins/PristineWaterPhysics/LEGACY-SOURCES.md) for optics and wave controls.

## Units and transforms

| Quantity | Unit |
| --- | --- |
| Actor position, water level, depth, extent, particle radius | Centimeters |
| Impulse API velocity / sample velocity | Centimeters per second |
| GetWaterVolumeM3 | Cubic meters, including region splash droplets |
| GetLiquidMassKg | Kilograms of SPH particle mass |
| Rest Density | kg/m3 |
| Kinematic Viscosity | m2/s |
| Surface Tension | N/m |
| HydrostaticPressurePa | Pascals, gauge pressure from submerged depth |
| Temperature C | Celsius, 0.01–80 for the freshwater reference model |
| Performance Budget Ms | Milliseconds of CPU work per actor per target frame |

The Water component's older Volume.Density control uses density relative to 1000 kg/m3: `1` means `1000 kg/m3`. Small Fluid Rest Density uses kg/m3 directly. Keep simulation actor scale one and rotation zero. Transform changes are not an inertial moving-container simulation.

## Main Blueprint functions

| Actor/library | Function | Behavior |
| --- | --- | --- |
| Region | BakeLandscape | Collision-samples bed, stores bake and resets water. Editor authoring operation. |
| Region | ResetWater / SetWaterLevel | Refills at selected height and clears motion and droplets. Changes total volume. |
| Region | ClearLandscapeBake | Returns to the flat-bed configuration; inspect/reset accordingly. |
| Region | AddSplash | Adds momentum and transfers some volume to ballistic droplets. |
| Region | SampleRegion | Surface height, signed depth, velocity and hydrostatic pressure. |
| Region | GetWaterVolumeM3 | Sum of grid and airborne volume. |
| Small Fluid | ResetFluid | Recreates particles; changes initial mass according to density and spacing. |
| Small Fluid | AddImpulse | Adds bounded world-space velocity to existing particles. |
| Small Fluid | SetFluidProperties | Sets custom density, viscosity, surface tension and wall damping, retaining mass. |
| Small Fluid | GetParticleCount / GetLiquidMassKg / IsFluidStateValid | Diagnostics. |
| Region / Small Fluid | SetWaterTemperature | Applies uniform temperature-dependent properties, preserving existing mass. |
| Region / Small Fluid | AdvanceSimulation | Explicitly advances up to the permitted timestep/substep budget. |
| All three water actors | SetPerformancePreset / ApplyPerformanceSettings | Applies hardware/manual visual quality settings. |
| Source | SetBodyType / SetQuality / RebuildSurface | Analytic body preset and surface mesh controls. |
| Source | SampleWater / GetSphereBuoyancyForce | Analytic surface query and spherical buoyancy estimate. |
| Source | AddRipple | Four timed ripples in High quality; geometric with CPU wave geometry enabled. |
| Water Performance Library | GetWaterHardware | CPU core count, RAM, GPU name, RT support and initial recommended tier. |
| Water Properties | FreshWaterAtTemperature | Density, viscosity and surface tension reference values. |

`SampleRegion.bWet` indicates a water-bearing cell and rejects below-bed points; test `DepthCm >= 0` as well to determine whether a point is submerged. `SampleWater.bInside` is a body-boundary/depth-range check; use signed Depth for submersion.

## Selecting performance

Start with Auto, then inspect Average Solver Ms and Average Mesh Ms in Play mode. Presets are applied at Begin Play; select Custom when preserving manual values. Region resolution and particle counts are unchanged by presets. Auto is a CPU/RAM/RT-capability heuristic rather than a measured GPU score.

| Tier | Max visual Hz | CPU budget ms | Small Fluid surface resolution | Source quality |
| --- | ---: | ---: | ---: | --- |
| Low | 12 | 2 | 20 | Low |
| Balanced | 24 | 3 | 28 | Medium |
| High | 40 | 5 | 40 | High |
| Custom | User selected | User selected | Retained | Retained |

Adaptive Visuals reduces mesh update frequency and recovers gradually. Target FPS informs its arithmetic; it does not cap the engine FPS. Min Surface Hz is bounded by the maximum. Small Fluid has an additional Surface Updates Per Second cap. Costs are per actor: ten actors do not share one budget automatically. GPU cost, ray-tracing acceleration structures, scene shading and unrelated systems are outside this controller.

If solver time is high, reduce particle/grid resolution explicitly and reset/re-bake. If mesh time is high, reduce surface resolution or refresh rate. A mesh update cannot be interrupted mid-build. Profiling is necessary even when the average meets the budget.

## Physics and temperature

Region grid volume and SPH mass are separate models; there is no automatic transfer between actors. Region thermal expansion scales depth/discharge and airborne volume to preserve mass. Small Fluid updates rest density, viscosity and cohesion strength while preserving particle masses. Heating is a uniform property adjustment, not heat transport. Region bottom drag is empirical; it does not resolve molecular viscosity.

Max Substeps limits work. A frame request is capped at 0.1 seconds, and very small stability timesteps can exhaust substeps. Last Simulated Seconds reports actual advancement; Performance.UnsimulatedSeconds reports omitted time for that call. This time is not automatically recovered. Numerical sound speed is artificial, not the acoustic speed of real water.

## Ownership and networking

Actors own independent simulation state. Assign Region references explicitly to Interactors. No per-frame discovery is needed. Network state replication, cross-machine determinism, save-game particle caches and multiplayer reconciliation are not provided. Dedicated servers skip ongoing visual mesh updates but may still execute the fluid solver.

## Material overrides

Use materials compatible with the selected surface and Unreal renderer. Avoid adding shader displacement on top of CPU wave geometry. Region uses Single Layer Water; small liquid and droplets use translucent materials with approximate IOR 1.333. There is no spectral dispersion, exact underwater total internal reflection or physically resolved caustic transport.


Version 0.6 adds ApplyCinematicPreset on Region and Source, and ApplyCinematicLook / SetRoughness on the Water component. See [Maximum quality](MAXIMUM_QUALITY.md). The new region solver stores cell-centered horizontal momentum; C++ users accessing old Grid.QX/QY arrays must migrate to Grid.MX/MY or Grid.Velocity.
