# Animation and cinematic workflow

Pristine can be evaluated in a cinematic scene, but it does not yet provide a production simulation cache, arbitrary-time scrubbing, Alembic export or Movie Render Queue determinism. Do not rely on the editor viewport alone as a baked fluid result.

## Prepare a shot

1. Duplicate the example map into your project's Content folder and build the shot's lighting/container geometry.
2. Use a Region for bounded large surfaces or Small Fluid for a small closed container. Keep scale one and rotation zero.
3. Bake the Region after finalizing terrain, then reset to its starting level. Reset Small Fluid after changing particle spacing or container dimensions.
4. Set Performance to Custom and disable Adaptive Visuals when a consistent visual refresh is more important than interactive cost. Select a fixed mesh resolution/rate appropriate to the shot.
5. Disable Source Follow Camera for a fixed analytic ocean patch if camera-centered recentering would be undesirable.
6. Rehearse the impact/motion sequence with explicit Blueprint events calling AddSplash or AddImpulse. Reset before each rehearsal.
7. Allow enough simulation warm-up for the initial particle block to settle before the action. Compare rendered frames, not only viewport motion.

## Controlled stepping

Region and Small Fluid expose AdvanceSimulation. For a custom offline driver, disable bSimulate so actor Tick does not also advance physics. Reset once, then call AdvanceSimulation with a consistent small timestep for each simulation interval. Check Last Simulated Seconds; increase substep capacity or reduce requested timestep if it does not cover the interval. There is no built-in catch-up queue.

Do not call both automatic Tick simulation and manual advancement for the same frame. Changing cinematic temporal sample count or engine frame timing can change a live simulation. A fixed external driver still needs validation with your render pipeline; it is not an included cache/scrubbing system. Source waves use world time and do not share the manual fluid clock.

## Validate delivery

Render a short low-cost frame sequence first. Check for material differences in your chosen renderer, flickering thin droplets, frame-to-frame discontinuity and unintended resets. Then test the actual output resolution and sampling settings. Lumen and path tracing have different water/translucency behavior; path-tracer parity has not been validated.

For a shot requiring detailed pouring, contact-angle wetting, high-resolution breaking waves or repeatable random-access playback, this release needs additional solver/cache work or a separately validated fluid pipeline. See the roadmap for planned milestones.
