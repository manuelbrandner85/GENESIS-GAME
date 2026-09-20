# GENESIS – baut eine spielbare Fassung (Windows) und legt sie unter Build\Windows ab.
#
# Warum Development und nicht Shipping: In dieser Fassung soll getestet werden. Development behält
# die Konsole (Taste ^), die Entwicklerbefehle und das Developer HUD – genau das, womit sich ein
# Fehler beschreiben lässt. Eine Shipping-Fassung kommt, wenn es etwas zu veröffentlichen gibt.
#
# Aufruf:
#   powershell -ExecutionPolicy Bypass -File Tools\Build\Package-Game.ps1
#   powershell -ExecutionPolicy Bypass -File Tools\Build\Package-Game.ps1 -Clean     (voller Cook)
#
# Der erste Lauf dauert lange (alle Shader), jeder weitere ist dank iterativem Cook deutlich kürzer.

param(
    # Alles neu kochen statt nur die Änderungen
    [switch]$Clean,
    [string]$Configuration = "Development",
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Project = Join-Path $RepoRoot "Genesis\Genesis.uproject"
$Archive = Join-Path $RepoRoot "Build"
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"

if (-not (Test-Path $RunUAT)) { throw "RunUAT nicht gefunden: $RunUAT" }
if (-not (Test-Path $Project)) { throw "Projekt nicht gefunden: $Project" }

# Der Editor hält die Binaries; ohne ihn zu schließen scheitert der Build am Schreibschutz
Get-Process UnrealEditor, UnrealEditor-Cmd -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2

New-Item -ItemType Directory -Force -Path $Archive | Out-Null

# Nur die Karten des Vertical Slice: Alles andere kostet Kochzeit ohne Nutzen
$Maps = "/Game/Genesis/Conception/Maps/L_GEN_OviductAmpulla+/Game/Genesis/Birth/Maps/L_GEN_Birth+/Game/Genesis/Maps/L_DevSandbox"

$Arguments = @(
    "BuildCookRun",
    "-project=`"$Project`"",
    "-noP4",
    "-platform=Win64",
    "-clientconfig=$Configuration",
    "-cook",
    "-map=$Maps",
    "-build",
    "-stage",
    "-pak",
    "-archive",
    "-archivedirectory=`"$Archive`"",
    "-nodebuginfo",
    "-utf8output"
)
if (-not $Clean) { $Arguments += "-iterativecooking" }

Write-Host "GENESIS Paket: $Configuration Win64 -> $Archive"
$Started = Get-Date
& $RunUAT @Arguments
if ($LASTEXITCODE -ne 0) { throw "Paketbau fehlgeschlagen (Exit $LASTEXITCODE)." }

$Exe = Get-ChildItem (Join-Path $Archive "Windows") -Filter "Genesis.exe" -Recurse -ErrorAction SilentlyContinue |
    Select-Object -First 1
$Minutes = [math]::Round(((Get-Date) - $Started).TotalMinutes, 1)

if ($Exe) {
    $SizeMb = [math]::Round(((Get-ChildItem (Split-Path $Exe.FullName) -Recurse | Measure-Object Length -Sum).Sum / 1MB), 0)
    Write-Host "PAKET FERTIG nach $Minutes min: $($Exe.FullName) (Ordner $SizeMb MB)"
} else {
    throw "Paketbau lief durch, aber Genesis.exe wurde nicht gefunden."
}
