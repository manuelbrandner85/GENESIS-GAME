# GENESIS Trailer - fuehrt ein Unreal-Python-Skript headless im Projekt aus (UnrealEditor-Cmd -run=pythonscript).
# Prueft vorher, ob gerade ein C++-Build laeuft (sonst LNK1104 in der anderen Session durch DLL-Locks).
# Aufruf:
#   powershell -ExecutionPolicy Bypass -File Tools\Trailer\Run-UnrealPython.ps1 -Script Tools\Trailer\Unreal\ExportLibraryAudio.py [-EnablePlugins Harmonix] [-ScriptArgs "..."]

param(
    [Parameter(Mandatory = $true)][string]$Script,
    [string]$EnablePlugins = "",
    [string]$ScriptArgs = "",
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Project = Join-Path $RepoRoot "Genesis\Genesis.uproject"
$Editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$ScriptPath = if ([IO.Path]::IsPathRooted($Script)) { $Script } else { Join-Path $RepoRoot $Script }
$LogName = [IO.Path]::GetFileNameWithoutExtension($ScriptPath)
$LogFile = Join-Path $RepoRoot "Genesis\Saved\Logs\Trailer_$LogName.log"

$Builders = Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match "^(UnrealBuildTool|link|cl)$" }
if ($Builders) { throw "Ein C++-Build laeuft gerade ($($Builders.ProcessName -join ', ')). Spaeter erneut starten." }

$Cmd = "`"$ScriptPath`""
if ($ScriptArgs) { $Cmd = "`"$ScriptPath $ScriptArgs`"" }
$Arguments = @("`"$Project`"", "-run=pythonscript", "-script=$Cmd", "-unattended", "-nopause", "-nosplash", "-NoShaderCompile", "-abslog=`"$LogFile`"")
if ($EnablePlugins) { $Arguments += "-EnablePlugins=$EnablePlugins" }

Write-Host "Unreal Python: $ScriptPath" -ForegroundColor Cyan
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -Wait -PassThru -NoNewWindow -RedirectStandardOutput "$LogFile.stdout.txt"
$Errors = Select-String -Path $LogFile -Pattern "LogPython: Error|Traceback|GENESIS_TRAILER_FAIL" -ErrorAction SilentlyContinue
$Ok = Select-String -Path $LogFile -Pattern "GENESIS_TRAILER_OK" -ErrorAction SilentlyContinue
Select-String -Path $LogFile -Pattern "GENESIS_TRAILER" -ErrorAction SilentlyContinue | ForEach-Object { Write-Host $_.Line }
if ($Errors) { $Errors | Select-Object -First 30 | ForEach-Object { Write-Host $_.Line -ForegroundColor Red } }
if (-not $Ok -or $Errors) { Write-Host "FEHLGESCHLAGEN (Exit $($Process.ExitCode)). Log: $LogFile" -ForegroundColor Red; exit 1 }
Write-Host "OK. Log: $LogFile" -ForegroundColor Green
exit 0
