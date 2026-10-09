# Third-party notices and references

## Unreal Engine

Unreal Engine is a separate dependency, not included in this repository. Unreal Engine is a trademark of Epic Games. Engine headers, tools and built-in assets are governed by Epic's agreements. The demo references `/Engine/BasicShapes` assets supplied by the user's Unreal installation. The MIT license does not grant rights to Epic's technology or those assets.

## Freshwater property references

`Plugins/PristineWaterPhysics/Tools/ReferenceWater.tsv` contains tabulated physical reference values obtained from the NIST Chemistry WebBook fluid database. Density and dynamic viscosity are sampled near 1 bar at approximately two-degree intervals. The plugin uses interpolation; it is not a full implementation of NIST's thermophysical models.

- Database: https://webbook.nist.gov/chemistry/fluid/
- Query: water (C7732185), isobar at 1 bar, temperature 0–80 C, increment 2 C, density kg/m3 and viscosity microPa s. The returned liquid table begins at 0.01 C and ends at 80.01 C.
- Surface tension: IAPWS, Revised Release on Surface Tension of Ordinary Water Substance, June 2014: https://iapws.org/documents/release/Surf-H2O.download

The reference organizations do not endorse or validate this plugin. Their documents, names and database services are not relicensed under the repository's MIT license. The property formulas and data usage are documented for reproducibility.

## Original content

The water materials, procedural texture generation and demo authoring code were developed for this plugin. No marketplace content or Unreal Engine source distribution is included. Generated binary Unreal packages are included with their authoring code in the editor module and Tools directory.

## Conservative shallow-water scheme

The region solver implements first-order HLL fluxes with hydrostatic reconstruction and per-face shared draining limits. Method reference: Audusse, Bouchut, Bristeau, Klein and Perthame (2004), A Fast and Stable Well-Balanced Scheme with Hydrostatic Reconstruction for Shallow Water Flows, https://doi.org/10.1137/S1064827503431090. The implementation is original; the cited work is not bundled or relicensed.
