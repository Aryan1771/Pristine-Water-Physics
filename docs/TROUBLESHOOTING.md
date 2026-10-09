# Troubleshooting

| Symptom | Check and corrective action |
| --- | --- |
| Plugin is missing | Confirm `<Project>/Plugins/PristineWaterPhysics/PristineWaterPhysics.uplugin`; rebuild the editor target and enable the plugin. |
| Modules are incompatible | Close Unreal and rebuild from source against the exact installed engine. Do not copy DLLs between engine patch builds. |
| Plugin content is invisible | Enable Show Plugin Content in Content Browser settings. |
| Water is untextured | Check bundled materials; run Tools > Create Pristine Water Assets if missing. Wait for shader compilation. |
| Region stays dry after bake | Check selected Terrain Actors, collision channel, trace heights, Landscape Sample Count and water level above the bed. |
| Holes or missing shoreline cells | Missing terrain hits become closed cells. Thin walls/shore detail can be below grid resolution. Check the conservative boundary rasterization. |
| Region becomes dry after editing resolution | The terrain bake is stale. Re-bake after resolution/bounds changes. |
| Water escapes the visible basin | Align the numerical spline with the basin; bake the actual collision bed and increase resolution if narrow barriers are missed. |
| Object sinks | Verify Region reference, Physics Body, collision/Simulate Physics, sphere radius, center and object mass. Buoyancy uses spherical volume. |
| Settings change on Play | Auto/preset settings apply at Begin Play. Choose Custom for manual quality values. |
| Low frame rate | Inspect solver versus mesh time, profile GPU separately, lower mesh detail/rate, then lower particles/grid resolution and explicitly reset/re-bake. |
| Water moves slowly during hitches | Check Unsimulated Seconds and Max Substeps. Requests over 0.1 seconds are capped and omitted time is not queued. |
| Small droplets disappear | Reconstruction resolution may be too coarse. Particle mass can exist without a visible isosurface. |
| Small liquid looks lumpy | Increase surface detail within budget; low particle count and approximate cohesion impose physical/visual limits. |
| No expected ray-traced reflections | Check DX12/SM6, hardware support, project ray-tracing settings, procedural mesh support, material and scene lighting. |
| Packaged game loses materials | Include map/assets in the cook; runtime-only spawning may need explicit material/texture cook directories. |
| Shot changes between renders | There is no simulation cache or MRQ determinism guarantee. Use controlled stepping and verify render timing. |

When reporting a bug, include Unreal version, OS/GPU, actor settings, a minimal reproduction and logs with personal paths or credentials removed. Mention whether it occurs in the editor, Play, standalone or a packaged game.
