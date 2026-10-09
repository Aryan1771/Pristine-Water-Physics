[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$plugin = Join-Path $repoRoot 'Plugins/PristineWaterPhysics'
$descriptor = Get-Content (Join-Path $plugin 'PristineWaterPhysics.uplugin') -Raw | ConvertFrom-Json
if ($descriptor.VersionName -ne '0.6.0') { throw 'Update the release checks for the new version.' }
foreach ($name in @('PristineWaterPhysics','PristineWaterEditor')) {
    if ($descriptor.Modules.Name -notcontains $name) { throw "Missing module: $name" }
}
foreach ($relative in @('Content/Demo/L_PristineFluidLab.umap','Content/Materials/M_PristineFluid.uasset','Content/Materials/M_PristineCinematic.uasset','Content/Materials/M_PristineVolume.uasset','Content/Materials/M_PristineDroplet.uasset')) {
    if (!(Test-Path -LiteralPath (Join-Path $plugin $relative))) { throw "Missing asset: $relative" }
}
$broken = @()
foreach ($file in Get-ChildItem $repoRoot -Recurse -File -Filter '*.md') {
    foreach ($match in [regex]::Matches((Get-Content $file.FullName -Raw),'\]\(([^)]+)\)')) {
        $target = $match.Groups[1].Value.Trim('<','>')
        if ($target -match '^(https?://|mailto:|#)') { continue }
        $target = ($target -split '#',2)[0]
        if ($target -and !(Test-Path -LiteralPath (Join-Path $file.DirectoryName $target))) {
            $broken += "$($file.Name): $target"
        }
    }
}
if ($broken.Count) { throw "Broken documentation links:`n$($broken -join "`n")" }
$oversized = Get-ChildItem $plugin -Recurse -File | Where-Object Length -ge 90MB
if ($oversized) { throw 'An asset approaches the GitHub per-file limit; review asset storage.' }
Write-Output 'Repository checks passed: descriptor, required demo/material assets, local documentation links and file sizes.'
