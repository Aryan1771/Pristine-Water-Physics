[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EngineRoot,
    [Parameter(Mandatory)][string]$Project,
    [Parameter(Mandatory)][string]$EditorTarget,
    [string]$ReportDirectory = (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestResults')
)
$ErrorActionPreference = 'Stop'
$projectPath = (Resolve-Path -LiteralPath $Project).Path
$enginePath = (Resolve-Path -LiteralPath $EngineRoot).Path
$build = Join-Path $enginePath 'Engine/Build/BatchFiles/Build.bat'
$editor = Join-Path $enginePath 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$installedPlugin = Join-Path (Split-Path $projectPath -Parent) 'Plugins/PristineWaterPhysics/PristineWaterPhysics.uplugin'
foreach ($file in @($build,$editor,$installedPlugin)) {
    if (!(Test-Path -LiteralPath $file)) { throw "Required file missing: $file. Install this checkout's plugin in the test project first." }
}
& $build $EditorTarget Win64 Development "-Project=$projectPath" -WaitMutex -NoHotReload -NoUBA -MaxParallelActions=2
if ($LASTEXITCODE -ne 0) { throw "Editor build failed: $LASTEXITCODE" }
# Each run gets a fresh folder so stale successful reports cannot mask a failed run.
$runReport = Join-Path ([IO.Path]::GetFullPath($ReportDirectory)) (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
New-Item -ItemType Directory -Path $runReport -Force | Out-Null
$testLog = Join-Path $runReport 'Automation.log'
& $editor $projectPath -unattended -NullRHI -nosound '-ExecCmds=Automation RunTests PristineWater' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$runReport" "-abslog=$testLog"
if ($LASTEXITCODE -ne 0) { throw "Unreal automation process failed: $LASTEXITCODE. Inspect $testLog" }
$report = Get-Content (Join-Path $runReport 'index.json') -Raw | ConvertFrom-Json
$passed = [int]$report.succeeded + [int]$report.succeededWithWarnings
if ($report.failed -gt 0 -or $report.notRun -gt 0 -or $passed -lt 12 -or @($report.tests | Where-Object { $_.state -ne 'Success' }).Count -gt 0) {
    throw "Automation incomplete/failed: passed=$passed, failed=$($report.failed), notRun=$($report.notRun). Inspect $runReport"
}
if ($report.succeededWithWarnings -gt 0) {
    Write-Warning "$($report.succeededWithWarnings) successful tests logged warnings; inspect $testLog."
}
Write-Output "Passed $passed PristineWater tests. Report: $runReport"
