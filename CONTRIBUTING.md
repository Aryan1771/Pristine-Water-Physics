# Contributing

Use UE 5.7 and a project with the plugin installed. Keep runtime code in PristineWaterPhysics and editor authoring/tests in PristineWaterEditor. Do not add dependencies on the editor module to runtime builds.

Before submitting changes:

1. Run `scripts/Test-Repository.ps1` for repository/asset/link checks.
2. Run `scripts/Test-Unreal.ps1` against a local test project containing the edited plugin. This builds Development Editor and executes the eight PristineWater tests with NullRHI.
3. Build the Development game target when runtime dependencies or code change. Run a cooked smoke test before claiming packaged-game support.
4. Add meaningful conservation, bounds or numerical-state regressions for solver changes. Avoid timing thresholds in automated tests; use reproducible benchmarks separately.
5. For rendering changes, capture and inspect the actual supported renderer. NullRHI tests cannot certify materials or visuals.
6. Document changed controls, units, reset behavior, validation evidence and limitations.

Do not commit generated Binaries, Intermediate, Saved, shader caches, project-specific logs or personal paths. Original Unreal assets are tracked as binary files; their authoring code is in the editor module and Tools. Keep source and generated assets consistent. Do not redistribute Unreal Engine source or third-party assets without the appropriate rights.

The current automated tests target Editor context. GitHub-hosted runners do not have a licensed Unreal installation configured here, so no passing Unreal CI badge is claimed. The local scripts are repeatable entry points for a future self-hosted build agent.

Changes should preserve mass/volume unless an explicitly documented reset or source/sink operation is requested. Report benchmark hardware, configuration, particle/grid counts, update rates and rendering mode. Keep deterministic controller tests separate from noisy performance measurements.
