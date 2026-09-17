# GENESIS – Editor-Build über UnrealBuildTool
# Aufruf:  powershell -ExecutionPolicy Bypass -File Tools\Build\Build-Editor.ps1 [-Configuration Development] [-Clean]

param(
    [string]$Configuration = "Development",
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Project = Join-Path $RepoRoot "Genesis\Genesis.uproject"
$BuildBat = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"

if (-not (Test-Path $BuildBat)) { throw "Build.bat nicht gefunden: $BuildBat" }

$Arguments = @("GenesisEditor", "Win64", $Configuration, "-Project=`"$Project`"", "-WaitMutex", "-NoHotReloadFromIDE")
if ($Clean) { $Arguments += "-Clean" }

Write-Host "GENESIS Build: GenesisEditor Win64 $Configuration" -ForegroundColor Cyan
& $BuildBat @Arguments
$ExitCode = $LASTEXITCODE

if ($ExitCode -ne 0) {
    Write-Host "BUILD FEHLGESCHLAGEN (Exit $ExitCode)" -ForegroundColor Red
} else {
    Write-Host "BUILD ERFOLGREICH" -ForegroundColor Green
}
exit $ExitCode
