# Step-by-step tutorials

Install the plugin using [SETUP.md](SETUP.md). Unreal uses centimeters for actor positions. Keep simulated water actors horizontal with scale `(1, 1, 1)`.

## 1. Create a contained pool or landscape lake

1. Create a new level with lighting, a floor and a basin. A Landscape or static mesh with working collision can supply the bed.
2. Search Place Actors for **Pristine Water Region** and add it above the basin.
3. Select its **ClosedRegion** spline. Move its control points to enclose the desired water area. Edges are straight and the polygon must not cross itself. Add more points to approximate a curved shore.
4. Set **Resolution** to 32 for a first test. Increase it only when the shoreline needs more detail. A large region with few cells cannot resolve small waves or narrow walls.
5. Add the intended Landscape/basin actors to **Terrain Actors**. This excludes roofs and unrelated collision from the bake. Set Trace Above and Trace Below to cover the basin.
6. Click **Bake Landscape**. Check that Landscape Bake Valid is true and Landscape Sample Count is greater than zero. The terrain must block the selected Terrain Channel.
7. Set **Water Level** to the required local Z height, then click **Reset Water**. Level 0 is the actor's origin height; it is not necessarily world Z zero.
8. Select Performance > Auto and click **Apply Performance Settings**. Press Play.
9. Add visible walls/banks along the region boundary. The numerical boundary contains water even when there is no visible wall.
10. Save the level. Re-bake after changing terrain, region placement, bounds or resolution.

Expected result: a wet surface over bed cells below the selected level, contained inside the polygon. Missing terrain collision creates closed cells. A conservative grid boundary can sit up to a cell inside the spline.

Reset Water and SetWaterLevel refill the domain and clear motion/splashes. They are authoring/reset operations, not a pump or a volume-preserving tide animation.

## 2. Add a buoyant object and entry splash

1. Create an Actor Blueprint with a Static Mesh component as its root. Give the mesh simple collision and enable Simulate Physics.
2. Add a **Pristine Water Interactor** component.
3. Place the Blueprint above your pool. On its Interactor instance, assign **Region** to the pool actor. Assign Physics Body if the intended simulating component is not automatically selected.
4. Set **Radius** to approximate the submerged object in centimeters. Set Local Center to the location of that sphere relative to the object.
5. Choose a plausible object mass. The submerged sphere must displace enough water to support that mass; an undersized sphere will sink.
6. Press Play. The Interactor applies buoyancy and drag, and triggers a splash as the sphere enters the region.
7. For a larger object, use several non-overlapping sample spheres whose combined volume approximates the hull. Avoid assigning a full hull volume to every component.

For an explicit gameplay splash, call Region's **Add Splash** from a Blueprint event. Start with World Position at the surface, Velocity `(0,0,-400)` cm/s, Radius `100` cm and Strength `1`. This injects momentum and transfers existing liquid into droplets. It does not compute exact impact energy or a coupled rigid-body reaction.

## 3. Simulate a small quantity

1. Add **Pristine Small Fluid** from Place Actors.
2. Start with Particle Radius `1` cm, Particles Per Axis `4` (64 particles), and Half Extent `(12,12,18)` cm.
3. Click **Reset Fluid**. This creates a centered block of particles inside the closed numerical box.
4. Add visible container geometry if needed. The editor box itself is not a rendered glass container.
5. Set Performance to Low or Balanced and click Apply Performance Settings.
6. Press Play. Gravity draws the liquid toward the box floor. Enable Scene Collision only when static scene geometry should also affect it.
7. Trigger **Add Impulse** with `(35,0,60)` cm/s to test motion.
8. Increase Surface Resolution for a smoother visual skin. Increase Particles Per Axis only after measuring solver cost; particle count is the cube of this value.
9. Set Temperature C to `40` and click Apply Water Temperature, or call SetWaterTemperature at runtime. Existing particle mass is retained.

Expected result: a coarse continuous liquid surface with possible detached drops. The six numerical box faces remain closed. Moving the actor is not a physically simulated moving vessel. This solver is not currently suitable for unconstrained high-resolution pouring, exact wetting or production fluid-cache export.

## 4. Add background ocean water

1. Place **Pristine Water Source**.
2. Select Body Type Ocean and click Apply Body Preset.
3. Enable Follow Camera for a local-player-centered patch. Disable it for a fixed cinematic camera or a stationary patch.
4. Enable Ray Tracing Wave Geometry when using geometry-aware ray intersections.
5. Set Performance Auto for an initial tier, or Custom to retain manually selected Quality.
6. Adjust wave amplitude, frequency, speed, flow direction, color and roughness through the Water settings. Use High quality for AddRipple support.

This actor supplies an analytic visual ocean and buoyancy queries. It does not conserve ocean volume, resolve breaking waves or automatically exchange water with a Region.

## 5. Connect gameplay sliders

1. Create a Widget Blueprint with sliders and a reference to your water actor, supplied by your game/level Blueprint.
2. On Temperature slider OnValueChanged, map 0–1 to 0.01–80 and call SetWaterTemperature on Region or Small Fluid.
3. For a quality selector, call SetPerformancePreset with Low, Balanced, High or Custom.
4. For numeric performance sliders, set the actor's Performance struct to Custom, set Budget Ms / Target FPS / Min Surface Hz / Max Surface Hz, then call ApplyPerformanceSettings. Apply on release or debounce changes rather than rebuilding on every mouse movement.
5. On Small Fluid, Surface Updates Per Second is an additional cap; set it to at least your desired maximum. Read Effective Surface Hz and Solver Over Budget to display actual controller state.
6. Avoid wiring resolution or water-level resets to continuously firing sliders: those operations deliberately reinitialize water and may require a new landscape bake.

The plugin exposes Details controls and Blueprint functions. It does not ship a ready-made in-game settings widget.
