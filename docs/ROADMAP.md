# Roadmap toward production use

This roadmap is a statement of development priorities, not a list of shipped features or promised dates.

1. **Release qualification:** cooked-game smoke tests, supported-platform matrix, many-actor CPU/GPU benchmarks, memory budgets and convergence comparisons.
2. **Cinematic reproducibility:** fixed simulation clock, cache recording/playback, scrubbing and tested Movie Render Queue integration.
3. **Fluid accuracy:** momentum advection for large regions, improved incompressibility for small fluid, validated boundaries and wetting/contact angles.
4. **Scale:** GPU fluid backend, spatially adaptive resolution and coordinated multi-actor budgets.
5. **Interactions:** conservative fluid/rigid-body momentum exchange, better hull displacement and explicit transfer between water domains.
6. **Rendering:** validated renderer paths, underwater optics, foam/bubbles and caustics workflows.
7. **Thermodynamics:** heat transport and phase-change modeling only with appropriate validation and performance scope.

Calling this the best or physically complete water engine would require independent comparisons and substantially more validation. Current 0.5 functionality and its limits are documented in the README and validation report.
