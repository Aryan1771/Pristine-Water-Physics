# Unreal setup and delivery notes

## Requirements

Tested: Windows 11, UE 5.7.4, Visual Studio C++ tools, RTX 4060 Laptop GPU. Other operating systems, engine versions and GPU families are unverified. Install the C++ toolchain recommended for your Unreal release and a Windows SDK. Ray tracing requires compatible hardware; simulation itself does not require it.

## Install in a project

1. Save your project and close Unreal Editor.
2. Copy the repository's `Plugins/PristineWaterPhysics` folder into your project's `Plugins` folder. The `.uplugin` must be immediately inside `Plugins/PristineWaterPhysics`.
3. If the project has no C++ target, create an empty C++ class using Unreal first, close the editor, then install the plugin.
4. Generate project files from the `.uproject` context menu or your engine's project-generation tool.
5. Open the generated solution. Choose Development Editor and Win64, then build your project's editor target.
6. Open the project. Enable Pristine Water Physics under Edit > Plugins and restart if required. Its Procedural Mesh Component dependency is declared by the plugin.
7. Enable Show Plugin Content in the Content Browser and locate the PristineWaterPhysics folder.
8. Open the bundled fluid lab. If working on the example, duplicate its map into `/Game` so plugin updates do not overwrite your scene.

There are no portable prebuilt DLLs in this repository. A DLL from a different Unreal build can fail to load even when its major/minor version matches. Build from source on the target installation.

## Materials and demos

Bundled materials and the demo are real `.uasset`/`.umap` files. If missing, select Tools > Create Pristine Water Assets in the editor. The generator preserves existing assets. Demo authoring scripts are `Tools/CreateFluidDemo.py` and `Tools/CreateDemo.py` inside the plugin; enable Unreal's Python Editor Script Plugin and Editor Scripting Utilities to run them. Run scripts through Tools > Execute Python Script and save your work first. These scripts switch the editor's active level and create missing demo assets.

The material generation commandlet is `PristineWaterAssets`. Example:

```powershell
& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $Project -run=PristineWaterAssets -AllowCommandletRendering -unattended
```

`$EngineRoot` and `$Project` must be set to your engine directory and `.uproject` file. Material authoring is editor-only; it is not needed at runtime when assets are cooked.

## Ray tracing

1. In Project Settings, choose DirectX 12 as Windows Default RHI and enable the SM6 shader format.
2. Enable Support Hardware Ray Tracing and Support Compute Skin Cache. Restart and allow shader compilation to finish.
3. Choose Lumen for dynamic global illumination and reflections; enable Use Hardware Ray Tracing When Available.
4. Keep `r.RayTracing.Geometry.ProceduralMeshes=1` for the procedural surfaces. Use suitable environment lighting and scene geometry for reflections.
5. On Water Source, enable Ray Tracing Wave Geometry. Medium/High tiers use the geometry-aware material; Low is intended as a cheaper rendering tier.
6. Inspect the result on the target hardware. Transparent small-fluid materials and Single Layer Water have different renderer behavior; path-tracer equivalence and caustics are not certified.

`Tools/RayTracingSettings.ini` provides settings to review and merge into your project. Do not overwrite your project's configuration blindly. The plugin does not automatically enable project-wide ray tracing.

## Cook and package

1. Include your water-containing maps in the project's packaging map list.
2. Keep material/texture soft references intact. If you spawn actors only at runtime without referenced maps, add `/PristineWaterPhysics/Materials` and `/PristineWaterPhysics/Textures` to Additional Asset Directories to Cook.
3. Build the game target and perform a Development cook/package for your platform.
4. Launch the packaged executable outside the editor. Verify materials, procedural meshes, terrain bake data, collisions and Blueprint interactions.
5. Profile representative scenes, including the largest expected actor/particle count. Test GPU as well as CPU frame time.
6. Repeat for Shipping and every supported platform before distributing a game.

Only editor automation, local rendering and Development target compilation have been validated so far. A successful C++ build is not evidence that a cooked product is ready to ship.

## Updating from 0.4

Back up project assets, replace the plugin source/content, rebuild and inspect water actors. Version 0.5 applies performance presets at Begin Play. Use Performance > Custom and Apply Performance Settings if you need to retain your selected surface resolution/rate or Source quality. Region grid resolution and particle counts are never automatically changed by presets. Existing terrain bakes must be regenerated when terrain or grid bounds change.
