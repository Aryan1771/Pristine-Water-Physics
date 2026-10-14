# Walking, wading and underwater test

The nature comparison uses `APristineWaterCharacter` to test the same water regions from a human viewpoint. The character samples the region's actual simulated surface, rather than a rectangular swimming volume. Shallow water keeps ground movement; deeper water enables swept three-dimensional movement. Terrain still blocks the capsule. The water surface itself has no blocking collision, so it can be crossed in either direction.

## Set up the comparison

1. Build the updated plugin in UE 5.7. Regenerate its materials using the `PristineWaterAssets` commandlet.
2. Generate the nature map with `Tools/CreateNatureComparison.py` if it does not exist yet.
3. Run `Tools/UpgradeNatureInteractions.py` through Unreal's Python command. This sets the map's game mode, creates the test character Blueprint and updates optical settings. Save the map.
4. Open `/Game/PristineNature/L_WaterComparison`. Press **Play**, then click inside the viewport to capture the mouse.

| Control | Action |
|---|---|
| W, A, S, D | Walk or swim; looking up/down steers swimming |
| Mouse | Look |
| Space | Jump on land; ascend in water |
| Left Ctrl | Dive while swimming |
| V | Switch first/third person |
| 1–4 | Puddle, pool, pond and lake test positions |
| 5 | Underwater lake position |
| Shift+F1 | Release the mouse in Play in Editor |

Walk through the puddle to disturb the shallow solver. Enter the pond or lake to transition to swimming. Dive with Ctrl and look upward to inspect the underside. Swim up and walk onto the bank to return to normal ground movement. Movement produces bounded, rate-limited splash impulses; resting in water does not continuously inject momentum.

## Appearance controls

`VolumeDepth` is the **optical reference distance in centimetres** for `DeepColor` transmission. It is not the terrain depth. Using 4 cm here just because a puddle is 4 cm deep makes that puddle absorb light much too strongly. The comparison now uses a consistent 500 cm optical reference across all four bodies. The actual scene depth determines how much bed remains visible.

`Clarity` controls scattering, `Absorption` scales wavelength-dependent extinction, and `DeepColor` supplies transmission at the reference distance. Roughness controls the sharpness of reflections; ripple strength controls small surface-normal disturbances. A low roughness cannot create reflection detail missing from the surrounding scene or renderer.

A world view subsystem reports actual immersion depth to Unreal so its Single Layer Water and cloud passes recognise submerged cameras. The underwater camera applies exponential attenuation over view distance, capped at the surface intersection for upward rays. This is an approximate underwater post-process, not a complete volumetric light transport simulation. The surface material is two-sided. Ray-traced reflection quality still depends on the Unreal renderer and project settings.

## MetaHuman integration

Use Epic's MetaHuman Creator to assemble a character in this project. An assembled `BP_MHC_WaterTester` under `/Game/MetaHumans` is picked up by the upgrade script. Alternatively, open `BP_WaterTester` and set **CharacterVisual → Child Actor Class** to your assembled MetaHuman Blueprint. Set **Third Person** to inspect it.

MetaHuman assets, rigs, textures and animations retain Epic's licenses. They are not included in this repository's MIT license. The plugin has no mandatory MetaHuman dependency; the same character controller can carry another licensed character visual.

When the installed Creator locomotion examples are available, the upgrade script copies its idle and forward-walk clips into the project, registers their skeleton as compatible with the assembled body skeleton, and generates a simple two-second treading loop from the idle pose. **Idle Animation**, **Walk Animation** and **Swim Animation** on `BP_WaterTester` can be replaced with clips for your character. The treading loop is a test motion, not motion-captured swimming or foot placement IK. Generated animations derived from Epic's idle pose retain the source asset's licensing requirements.

## Scope

This is a local keyboard/mouse test controller. Water movement uses Unreal's swept flying movement with water-state switching, drag and gentle idle buoyancy; it does not report Unreal's built-in `IsSwimming` state. Use the exposed `bSwimming` property for animation integration. Multiplayer prediction, drowning, oxygen, inventory and a production swimming animation system are outside this test controller. Call `RefreshWaterRegions` after adding or streaming water actors at runtime.

The depth-averaged basin solver cannot reproduce overturning waves, every small droplet or arbitrary three-dimensional fluid topology. These controls provide an interactive evaluation scene, not a claim of identical rendering or physics to another game.
