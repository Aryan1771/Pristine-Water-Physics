# Free Unreal starter kit

Checked 10 October 2026. Start with the first five downloads; add the others when your scene needs them. No collection is essential for every game, and adding every plugin to a project increases maintenance and packaging work. Select a UE 5.7-compatible version for this water project.

## First downloads

| Asset / official listing | Why keep it | Priority |
| --- | --- | --- |
| [Content Examples — Epic](https://www.fab.com/listings/4d251261-d98c-48e2-baee-8f4e47c67091) | Practical reference scenes for materials, Blueprints, physics, animation and audio. | 1: learn the engine |
| [Stack O Bot — Epic](https://www.fab.com/listings/b4dfff49-0e7d-4c4b-a6c5-8a0315831c9c) | A playable project with explained Blueprint gameplay and environment workflows. | 2: first complete game |
| [European Black Alder — Quixel](https://www.fab.com/listings/9de7ce19-5813-42d2-a5f0-5e6447006f72) | Trees, saplings and seedlings with wind and seasonal controls. Best first vegetation upgrade for the water scene. | 3: natural environments |
| [Game Animation Sample — Epic](https://www.fab.com/listings/880e319a-a59e-4ed2-b268-b32dac7fa016) | Character locomotion reference and animation assets. | 4: characters |
| [Niagara Examples Pack — Epic](https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600) | Fire, smoke, impacts and other effects, with scalability examples. | 5: reusable VFX |

These are free listings. Content Examples and several Epic samples are UE-only content; their assets do not become MIT-licensed because you use them alongside this plugin.

## Build out the library by scene type

| Download | Use it for | Practical note |
| --- | --- | --- |
| [Open World Demo Collection](https://www.fab.com/listings/3262ab8f-f64a-4124-8efd-82cb19df6249) | Rocks, ground, bushes, grass and woodland reference assets. | Older assets: test shaders, texture budgets and LODs after migration. |
| [Electric Dreams Environment](https://www.fab.com/listings/d79688f5-29be-4fb2-a650-2d4a813f5306) | Dense jungle and PCG learning. | A heavyweight learning sample; migrate only what you need. |
| [City Sample](https://www.fab.com/listings/4898e707-7855-404b-af0e-a505ee690e68) | Buildings, streets, vehicles and crowds. | Its listing links smaller Buildings, Vehicles and Crowd packs. Prefer those when you only need models. |
| [Automotive Materials](https://www.fab.com/listings/5dd132fe-ee32-4e8c-9cd3-7496547dfb29) | Paint, metal, leather, wood, textiles and carbon fibre for vehicles, props and interiors. | Materials, not a car model collection. |
| [Lyra Starter Game](https://www.fab.com/listings/93faede1-4434-47c0-85f1-bf27c0820ad0) | Larger gameplay architecture, multiplayer and scalable character systems. | Study after a smaller project; do not copy every subsystem into your first game. |
| [Cropout Sample Project](https://www.fab.com/listings/bd733d81-7c29-44fe-b53f-65b14d06a9e2) | Stylized strategy-game and interaction reference. | Useful alternative to realistic art. |
| [Project Titan](https://www.fab.com/listings/c05aac82-4c1a-4e42-96b3-be668dc40fca) | Broad open-world art and world-building examples. | Large art-jam project; Epic explicitly says it is not solely a best-practices sample. |
| [Misc Props — UB Free Pack](https://www.fab.com/listings/e5bb46ab-edf1-4530-8797-703f4c98b4a6) | Everyday scene dressing. | Personal tier was free; Professional tier was paid when checked. |
| [18 Free Spells Niagara](https://www.fab.com/listings/967b7892-d46c-4d75-8121-33418a6899ce) | Fantasy spell prototypes. | Personal tier was free; Professional tier was paid when checked. |

For a free audio foundation outside Fab, [Sonniss GameAudioGDC](https://gdc.sonniss.com/) provides sound-effect bundles for media production. Download individual bundles as needed and keep their license with the source files.

Two useful free libraries outside Fab are [Poly Haven](https://polyhaven.com/) for HDRIs, textures and models, and [ambientCG](https://ambientcg.com/) for ground, stone, brick, wood, metal, fabric, snow and other PBR surfaces. Both publish their asset libraries under CC0. They fill gaps across indoor, urban, historical, winter and outdoor scenes without requiring another engine plugin. Begin with 2K textures; reserve higher resolutions for close-up subjects.

## Plugins already included with UE 5.7

These were verified in the installed engine's plugin descriptors. Enable through **Edit > Plugins** only when needed; no separate Fab purchase is necessary.

| Plugin | Use |
| --- | --- |
| Procedural Content Generation Framework (PCG) | Rule-based vegetation and environment placement. |
| Modeling Tools Editor Mode | Blockout, mesh editing and simple prop creation. |
| Geometry Script | Geometry operations exposed to Blueprint/editor tools. |
| Control Rig | Rigging, posing and animation editing. |
| Niagara | Particle effects and visual simulations. |
| Enhanced Input | Keyboard, controller and action mapping. |
| Movie Render Queue (Movie Render Pipeline) | Cinematic image-sequence rendering. |
| Chaos Vehicles | Vehicle simulation when your game needs driving. |
| Water | Epic's landscape water workflow; optional comparison tool, separate from Pristine's solver. |

For this project, keep **Pristine Water Physics**, **Procedural Mesh Component** and **Python Editor Script Plugin** enabled. The generated scene does not require the other optional plugins. The generator runs only in the editor; its mesh actors are saved in the level.

## Claim now, download later

1. Open the official listing and check **Included formats: Unreal Engine** and your engine version. UEFN-only referenced assets are not ordinary Unreal project downloads.
2. Check the exact selected license price. “From Free” does not mean every tier is free. Personal eligibility depends on revenue/funding, as explained by Fab's selector.
3. Use **Add to My Library** to claim. Stop there if you do not want a download.
4. Later, open Epic Games Launcher > Unreal Engine > Library > Fab Library. Asset packs use **Add to Project**, samples use **Create Project**, and code plugins use **Install to Engine** where supported.
5. Keep experimental downloads in a separate test project. Use Unreal's **Migrate** command for selected assets so dependencies accompany them.

The rotating [Limited-Time Free](https://www.fab.com/limited-time-free) offer showed Bridge Environment, AdaptivePathfindingSystem and Stylized Harvest Farm until **20 October 2026, 9:59 AM ET**. This offer is time-limited, not a promise of permanent free availability.
