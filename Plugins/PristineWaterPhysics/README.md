# Pristine Water Physics 0.6 (UE 5.7)

Source plugin with bundled original Unreal materials, ray-tracing-visible wave geometry, bounded shallow-water simulation, splash droplets, buoyancy interactions and an experimental small-volume SPH solver. This is a real-time approximation, not a validated engineering CFD package or a simulation of every property of water.

## Install and open

1. Close Unreal. Copy `PristineWaterPhysics` into `YourProject/Plugins/PristineWaterPhysics`.
2. Build your Development Editor target using UE 5.7 and Visual Studio C++ tools. A Blueprint-only project can add an empty C++ class first.
3. Enable the plugin, restart, and enable Show Plugin Content in the Content Browser.
4. Open `/PristineWaterPhysics/Demo/L_PristineFluidLab` for the contained-water test scene. Press Play for fluid simulation. The small fluid example sits on the side plinth; select it and press F to inspect closely.
5. In Place Actors, search `Pristine Water Region`, `Pristine Small Fluid`, or `Pristine Water Source`. They are native Blueprintable actors. You can create child Blueprints of each.

If materials are missing, use **Tools > Create Pristine Water Assets**. Existing assets are preserved. The plugin includes its material generator and texture source; no Fab assets are required.

## Landscape, boundary and water-level tools

Place **Pristine Water Region** above a Landscape or static collision mesh. Keep its rotation zero and scale one. Its position is the coordinate origin; `Water Level` is a local Z height in centimeters.

- Edit the `ClosedRegion` spline control points to outline the allowed area. Points are connected as straight polygon edges; use a simple, non-self-intersecting polygon with at least three points. More points approximate curves.
- Set `Resolution` (8–128 cells per axis). Cell size determines shoreline and terrain detail, not world size. A 3 km region at 48 cells cannot resolve centimeter ripples. Use local regions near interactions, and Water Source for distant seas/oceans.
- Select **Bake Landscape** in the actor Details panel. Downward collision traces sample the bed. Use `Terrain Actors` to restrict sampling to your Landscape and selected basin meshes, excluding overhead collision from unrelated actors. An empty list samples all blocking actors. Landscape and meshes must block `Terrain Channel` (WorldStatic by default). Missing hits become closed cells. Set Trace Above/Below to cover your terrain height range.
- Inspect `Landscape Bake Valid` and `Landscape Sample Count`, then set `Water Level` and press **Reset Water**. The water fills cells below that level. Runtime Blueprint `SetWaterLevel` performs the same reset.
- Re-bake after moving the actor, changing the terrain, or changing resolution/bounds. An incompatible existing bake closes the domain until re-baked. `Clear Landscape Bake` deliberately returns to a flat bottom controlled by `Default Depth`.
- The polygon is an impermeable simulation boundary. Water cannot leave even if terrain outside it is lower. Put visible bank/wall geometry along its edge if you want a physical-looking container.

Bake Landscape reads terrain; it does not sculpt or destructively modify your Landscape. Sculpt the basin with Unreal Landscape mode, then bake. Bed heights are serialized into the level; no terrain traces occur each simulation frame. Sub-cell walls or features can be missed by center sampling. The closed polygon is conservatively rasterized, so the visible edge may sit up to a cell inside it. Overhangs, caves and stacked water layers require a 3D solver.

Changing Water Level or Reset Water deliberately changes the stored volume and clears velocities and splashes. During simulation, volume remains conserved across cell exchanges and splash transfers. `GetWaterVolumeM3` includes airborne droplets. `SampleRegion` returns surface height, signed depth, flow velocity, and hydrostatic gauge pressure in pascals. It rejects cells without water and points below the bed; check `DepthCm >= 0` separately for submersion.

## Splash and object interaction

Call `AddSplash(WorldPosition, VelocityCmPerSecond, RadiusCm, Strength)` on a region. It adds horizontal/radial momentum and transfers a small amount of water into gravity-driven droplets. Drops reflect at the closed boundary and return their volume on landing or expiry. They are visual ballistic particles, not resolved fluid sheets or breaking-wave spray. Momentum injection is user-driven; it is not an energy-conserving coupled rigid-body solver.

For automatic entry splashes and buoyancy, add **Pristine Water Interactor** to a simulating physics actor. Assign `Region`, optionally `Physics Body`, and an approximate sphere `Radius` and `Local Center`. It applies submerged-sphere buoyancy and quadratic drag at the sample point, and triggers a splash on entry. Boats need several sample components/points and carefully chosen volumes; arbitrary hull displacement is not solved. Droplets do not exert forces on scene objects.

## Small quantities: Pristine Small Fluid

A separate CPU weakly-compressible SPH simulation handles small containers with pressure, gravity, viscosity, symmetric surface cohesion and static scene collision. A continuous marching-tetrahedra surface supports detached pieces and overhangs. The box drawn in the editor is a closed numerical container; add visible container geometry yourself.

- `Particle Radius` and `Particles Per Axis` control resolution and liquid mass (up to 1,000 particles). `Reset Fluid` creates a centered block that falls under gravity.
- `Rest Density`: kg/m³; default 998.2. `Kinematic Viscosity`: m²/s; default approximately 1.004e-6. `Surface Tension`: N/m; default 0.0728. Cohesion is an approximation, without contact-angle/wetting physics.
- `Sound Speed` is an artificial solver parameter, not physical water acoustics. Raising it reduces compressibility but requires smaller timesteps. This is not incompressible FLIP.
- `AddImpulse` accepts world-space cm/s. All six box faces contain particles. Static collision sweeps use WorldStatic; dynamic two-way object coupling is not implemented.
- `Surface Resolution` controls the reconstructed mesh (12–56); default 32. Small droplets thinner than a mesh cell may disappear visually while remaining in the simulation. The rendered isosurface volume is approximate and differs from conserved particle mass.
- Keep the actor horizontal and at unit scale. Reinitialize after changing mass, particle spacing or container dimensions. LiquidMaterial can be replaced with your own compatible transparent material.

Both solvers use stability-limited substeps with a maximum substep count. Large frame hitches are capped at 0.1 seconds, and simulation slows if Max Substeps is exhausted. Inspect `Last Simulated Seconds` and `Performance > Unsimulated Seconds`; the latter reports time omitted in the last update. No fixed frame-rate performance claim, multiplayer determinism or replication is provided.

## PC performance presets and sliders (0.5)

Every water actor exposes **Performance** in Details. Select **Auto**, **Low**, **Balanced**, **High** or **Custom**, then click **Apply Performance Settings**. Begin Play applies the selected preset automatically. At runtime, use Blueprint `SetPerformancePreset`. Existing water optics and motion controls remain available.

Auto chooses an initial tier from CPU core count, physical RAM and ray-tracing capability. `GetWaterHardware` also reports the GPU name. This is a conservative starting heuristic, not a measured GPU rating. Adaptive mode subsequently measures this actor's CPU simulation and mesh work. It does not benchmark the whole game or guarantee a frame rate.

| Slider | Meaning |
| --- | --- |
| Target FPS, 15–144 | Desired frame budget used by the controller; does not set the game's FPS cap. |
| Budget Ms, 0.1–30 | Advisory CPU allowance per water actor per target frame. Divide your water budget among actors. |
| Max Surface Hz, 1–60 | Maximum geometry refresh rate. Physics continues independently. |
| Min Surface Hz, 1–30 | Lowest adaptive geometry rate; never higher than the effective maximum. |
| Adaptive Visuals | Reduces refresh rate when measured work exceeds the budget; slowly recovers when affordable. |

Choose **Custom** before adjusting the numeric performance controls, then click Apply. With Adaptive Visuals off, geometry uses the requested maximum, capped by Target FPS and actual simulation updates. On Small Fluid, the existing Surface Updates Per Second is an additional cap. The controller exposes Effective Surface Hz, Average Solver Ms, Average Mesh Ms and Solver Over Budget for tuning. A single mesh update cannot be interrupted to enforce Budget Ms; GPU rendering/ray-tracing cost is outside this CPU controller.

Low/Balanced/High start at 12/24/40 visual updates per second and 2/3/5 ms budgets. Small Fluid uses surface resolutions 20/28/40; Source uses Low/Medium/High mesh tiers. Region grid resolution and Small Fluid particle count are preserved across performance presets, so quality changes do not silently delete, refill or change the mass of simulated water. Source's analytic ripple feature remains High-tier only. Lowering visual refresh can make fast splashes visibly step.

If Solver Over Budget remains true, reduce Region Resolution or Small Fluid Particles Per Axis, then explicitly re-bake/reset. Those resolution changes reinitialize water. Lower surface resolution or refresh rate first when mesh cost dominates. Ray tracing still requires project/renderer settings and compatible hardware. A project-wide GPU scalability system is separate from these controls.

The region now computes normals in linear time and batches splash instance changes at visual refresh. Small Fluid reuses simulation buffers, caches interacting particle pairs, and reconstructs the scalar field only within each particle's compact support.

## Temperature and physical-property controls (0.5)

On Region or Small Fluid, set **Temperature C** and click **Apply Water Temperature**, or call Blueprint `SetWaterTemperature`. The supported range is 0.01–80 °C for fresh liquid water near one bar. Temperature is a prescribed uniform input, not an evolving heat field.

- Small Fluid updates rest density, kinematic viscosity and surface tension while retaining existing particle masses. `SetFluidProperties` also supports custom density, viscosity, tension and wall damping without changing mass. Subsequent `Reset Fluid` deliberately recomputes initial mass from density and particle spacing.
- Region updates density and applies uniform volume expansion/contraction to conserve mass, including airborne droplets. Hydrostatic pressure follows rho × 9.81 × submerged depth. Its shallow-water solver still uses empirical bottom drag, not resolved molecular viscosity or capillary forces.
- Small Fluid's **Wall Friction** slider (0–100 per second) damps tangential contact velocity. It is a numerical wall-slip approximation, not a contact-angle or adhesion model.

Density and dynamic viscosity interpolate the bundled `Tools/ReferenceWater.tsv` at two-degree intervals from [NIST's fluid database](https://webbook.nist.gov/chemistry/fluid/). The table is for water at 1 bar, 0.01–80.01 °C; the exposed range stops at 80 °C. Surface tension uses the [IAPWS 2014 liquid-vapor correlation](https://iapws.org/documents/release/Surf-H2O.download). The density maximum near 4 °C is retained. These property fits do not make the numerical solver exact: no freezing, boiling, evaporation, salinity, heat transfer, pressure-dependent equation of state, wetting, bubbles or acoustic propagation is implemented.

## Rendering and ray tracing

The new region surface uses **M_PristineFluid**, a Single Layer Water material with mesh normals, absorption/scattering, small normal waves and flow foam. Actual simulation displacement is uploaded to procedural mesh geometry so rasterization and ray intersections see the same surface. `M_PristineVolume` and `M_PristineDroplet` use forward-lit translucency with IOR 1.333 and dielectric reflectance. These materials approximate small-water optics; they do not implement spectral dispersion, multiple scattering, caustics or a full underwater TIR solution.

Water Source defaults to **Ray Tracing Wave Geometry**, with its update rate controlled by Performance settings. Medium/High use the geometry-aware material. This remains an analytic two-wave ocean surface, not a volume-conserving ocean solver. Disabling the option restores inexpensive shader waves, whose displaced surface is not guaranteed to match ray intersections. Custom material overrides must use geometry normals and avoid adding duplicate displacement. High-quality `AddRipple` also displaces CPU geometry and updates the height query when this option is enabled; disabling it retains the original shading-only ripples. The legacy controls and presets are described in LEGACY-SOURCES.md; its 0.3 tick/geometry limitations are superseded here.

For the same setup as the validation host, use Project Settings:

- Windows Default RHI: DirectX 12; D3D12 Shader Format: SM6.
- Support Hardware Ray Tracing and Support Compute Skin Cache: enabled; restart and compile shaders.
- Dynamic Global Illumination and Reflections: Lumen; Use Hardware Ray Tracing when available: enabled.
- `r.RayTracing.Geometry.ProceduralMeshes=1` (engine default). Use appropriate sky/scene geometry for reflections.

See `Tools/RayTracingSettings.ini` for settings to merge into your project, not an automatic global override. Ray-traced translucency/path tracing have different renderer limitations from Single Layer Water and Lumen. Merely enabling ray tracing does not guarantee physically exact water or acceptable frame time. Verify on target hardware.

[Epic: Hardware Ray Tracing](https://dev.epicgames.com/documentation/en-us/unreal-engine/hardware-ray-tracing-in-unreal-engine?application_version=5.7) · [Epic: Single Layer Water](https://dev.epicgames.com/documentation/en-us/unreal-engine/single-layer-water-shading-model-in-unreal-engine?application_version=5.7) · [Epic: Fluid Simulation Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/fluid-simulation-in-unreal-engine---overview?application_version=5.7)

## Validation and packaging

See VALIDATION.md for checks actually performed. Run Session Frontend > Automation > PristineWater for source tests and the fluid regressions. The Tools scripts can generate the demo and materials in another host project. Editor-only generation/tests are excluded from packaged games.

Retain the material soft references and include your water-containing maps in the cook. For runtime-only spawning without referenced maps/assets, explicitly add `/PristineWaterPhysics/Materials` and `/PristineWaterPhysics/Textures` to Additional Asset Directories to Cook. Test a packaged build on each supported target; editor tests alone do not certify shipping builds.

This release is experimental. It does not simulate full 3D large-scale Navier–Stokes flow, breaking waves, heat transport or phase changes, evaporation, bubbles, wetting/contact angles, or arbitrary-fluid rheology. Large water uses a conservative shallow-water approximation with momentum transport; small water uses a bounded CPU SPH approximation. Use a dedicated GPU FLIP/Niagara solution for high-resolution pouring and production-scale volumetric splashes.


## Cinematic reflections and solver upgrade (0.6)

Apply Cinematic Preset on Region or Source for the new dielectric Single Layer Water material, low roughness, restrained micro-ripples and a 60 Hz custom visual profile. Configure hardware Lumen ray-hit lighting in the project for the maximum-quality path. The Region now uses HLL momentum transport with hydrostatic reconstruction; incoming splash momentum is volume-weighted. See the repository maximum-quality guide and validation report for measured checks and limits.
