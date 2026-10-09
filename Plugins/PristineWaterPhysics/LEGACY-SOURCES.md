# Pristine Water Physics 0.3

UE 5.7 source plugin with placeable water sources, original material assets, scalable surface geometry and Blueprint controls. Visual quality depends on lighting, shore geometry, reflections and the project's renderer. No universal frame-rate claim is made.

## Quick start

1. Close Unreal. Copy this folder into `YourProject/Plugins/PristineWaterPhysics`.
2. Build your project's Development Editor target for Win64. Source builds require Visual Studio C++ game development tools and a Windows SDK. A Blueprint-only project can add an empty C++ class first.
3. Enable Pristine Water Physics under Edit > Plugins. Its Procedural Mesh Component dependency is enabled automatically.
4. Enable **Show Plugin Content** in the Content Browser.
5. Drag `Blueprints/BP_PristinePuddle`, `BP_PristinePond`, `BP_PristineLake`, `BP_PristineSea` or `BP_PristineOcean` into your level.
6. Select the actor. Set Quality, Half Extent, Roughness, Normal Tiling and Shore Foam. Expand the **Water Settings** component for optics, colors, absorption, clarity and waves.
7. Keep water actors horizontal and at unit scale; size them using Half Extent. Put the surface at the desired elevation. Place terrain/meshes underneath; the plugin does not excavate a lake bed.

If assets are missing, use **Tools > Create Pristine Water Assets**. The builder preserves existing assets. Back up and rename a generated asset before regenerating it; the tool does not overwrite your edits.

The original PristineWaterComponent still works on your own mesh/material. The new Water Source actor supplies geometry and bundled materials automatically.

## Sources and quality

| Source | Default full dimensions | Behavior |
|---|---|---|
| Puddle | 3 x 2 m | Elliptical patch, almost flat, no default shore foam |
| Pond | 30 x 20 m | Small bounded surface |
| Lake | 200 x 140 m | Larger bounded surface |
| Sea | 2 x 2 km | Fixed rectangular surface |
| Ocean | 20 x 20 km | Finite horizon patch, follows local player zero during play |

Half Extent is in centimeters. Rectangle and ellipse footprints are supported. A lake can extend under surrounding terrain to form a natural visible shoreline. Custom retains your settings; changing to a named body preset resets its water settings.

| Quality | Vertices / triangles per source | Material |
|---|---|---|
| Low | 1,089 / 2,048 | Opaque Default Lit; one normal layer; no underwater visibility or depth shoreline foam |
| Medium | 4,225 / 8,192 | Single Layer Water; two normals, absorption, scattering, depth shoreline foam |
| High | 16,641 / 32,768 | Medium plus four local interaction ripple slots |

The ocean uses a continuous nonuniform grid concentrated near the camera. Geometry count is independent of ocean area. It recenters at 5 Hz in 1 m increments without rebuilding geometry. It is not an infinite ocean or tiled quadtree. Large distant triangles can undersample displacement; increase quality or reduce extent when visible.

Waves animate on the GPU. Bounded actors have no ongoing tick. Water geometry has collision, overlap generation, shadow casting and distance-field contribution disabled. Shader displacement receives padded bounds. Original textures are 512 square, seamless, mipmapped and streamable. Material instances are reused on repeated settings changes.

## Colors and liquids

Water Settings exposes shallow/deep colors, IOR, reflectivity, Fresnel exponent/strength, artistic TIR boost, refraction-normal strength, absorption, clarity, foam, depth, density, waves, ripples and flow direction.

Use SetLiquidType for Water, Ocean, Oil or Custom. Oil is an artistic optical/motion preset, not a viscosity simulation. Use SetLiquidColors for arbitrary tints. Material Override accepts an alternate compatible material. Component settings overwrite matching instance parameters on Apply; set your desired colors on the component too.

Density is in g/cm3 on the component and kg/m3 in queries. It affects buoyancy queries, not water tint. FresnelPower retains its original parameter name but correctly means exponent. FresnelStrength is separate. Artistic TIR is a grazing-angle adjustment, not an underwater total-internal-reflection simulation.

## Blueprint API

- SetBodyType: select and apply a source preset.
- SetQuality: change mesh resolution and material tier.
- RebuildSurface: rebuild after programmatically changing extent or shape.
- ApplyAppearance: update source appearance after programmatically editing its properties.
- Component setters apply controls immediately. After directly editing a settings struct at runtime, call ApplyWaterSettings.
- AddRipple(WorldPosition, Strength): High only; four four-second slots, oldest replaced. Shading-only ripples do not alter CPU water height.
- SampleWater: two-wave surface elevation, normal, signed depth and density. bInside tests footprint and bottom; underwater means bInside AND Depth >= 0. GameplayDepth defines the bottom without inspecting terrain.
- GetSphereBuoyancyForce: upward force in Unreal force units for a submerged sphere approximation. Apply to a simulating component at a chosen point each physics update. Add drag/damping and tune for mass; boats and swimming are not automatically simulated.

SampleWater shares macro-wave equations with the material for horizontal unit-scale actors. Mesh interpolation introduces approximation between vertices. Overlapping water, networking, split-screen oceans, world-origin rebasing and tilted/scaled bodies need project-specific integration.

## Material contract

Scalars used: RefractiveIndex, Reflectivity, FresnelPower, FresnelStrength, TotalInternalReflection, RefractionStrength, VolumeDepth, Absorption, Clarity, FoamAmount, WaveAmplitude, WaveFrequency, WaveSpeed, RippleStrength, FlowDirectionX, FlowDirectionY, Roughness, NormalTiling, ShoreFoam, ShoreWidth.

Vectors: ShallowColor, DeepColor, Interaction0 through Interaction3 (XY position, start time, strength). Textures: NormalTextureA, NormalTextureB, FoamMask.

The component also sends Density, LiquidType and vector FlowDirection for older custom graphs; bundled shading does not consume these. A parameter must be connected in a material graph to affect rendering.

Assets are generated by PristineWaterAssetsCommandlet.cpp using deterministic periodic texture functions and material expression graphs. No Fab packs or third-party textures are bundled. The editable generator is included. Assets in Content are real Unreal packages.

## Performance workflow

Start at Medium, 1080p. Use Low on older/integrated GPUs and High for nearby interactions. Provide sky lighting and scene/reflection content; a material alone cannot provide realistic surroundings.

Use stat unit, stat gpu, profilegpu and Unreal Insights in a packaged game. Compare the same camera with water visible/hidden after shader compilation and streaming settle. Measure multiple source counts on your minimum supported hardware. Screen coverage, water/reflection passes and resolution may cost more than vertex count. Many tiny sources also add draw calls.

The plugin does not change project-wide Lumen, ray tracing, TSR, shadow or reflection settings. Planar reflections and ray tracing are not required. Editor frame rates are not shipping benchmarks. Cook and test every tier players can select.

## Development

Runtime module: PristineWaterPhysics. Editor-only asset builder/tests: PristineWaterEditor, excluded from packaged games.

Build assets after compiling the editor target:

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Path\YourProject.uproject" -run=PristineWaterAssets -AllowCommandletRendering -unattended
```

Session Frontend > Automation > PristineWater.SourceAndMaterial tests geometry budgets, material reuse, clamping, bounds and buoyancy units. See VALIDATION.md for checks actually performed on this version.

For engine-wide installation, place a BuildPlugin package under UE_5.7/Engine/Plugins/Marketplace/PristineWaterPhysics. Prefer project-local installation during development; do not install duplicate copies at both scopes.

## Current boundaries

No terrain sculpting, spline river tool, underwater post-process, volumetric caustics, breaking-wave simulation, fluid filling/spreading, automatic gameplay replication or guaranteed cross-platform support. Shapes are rectangle/ellipse; arbitrary visible shorelines come from intersecting terrain. A source creates a water surface, not a faucet or mass-conserving fluid emitter.

[Epic Single Layer Water documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/single-layer-water-shading-model-in-unreal-engine?application_version=5.7)
