# GENESIS – startet das Spiel (Standalone), führt Konsolenbefehle aus und lässt die Engine einen Screenshot inkl. HUD erzeugen.
# Dient der visuellen Prüfung (z. B. Developer HUD) ohne manuelle Bedienung. Ein geöffneter Editor stört nicht.
# Aufruf:
#   powershell -ExecutionPolicy Bypass -File Tools\Build\Capture-GameScreenshot.ps1 -ExecCmds "showdebug Genesis" -Output Docs\Media\hud.png

param(
    [string]$ExecCmds = "showdebug Genesis",
    [string]$Output = "Genesis\Saved\Screenshots\genesis_capture.png",
    [int]$TimeoutSeconds = 180,
    # >0: Screenshot erst nach dieser Spielzeit (Einschwingen von Übergängen), via genesis.Debug.After
    [float]$ShotDelaySeconds = 0,
    # Optional: Map statt der Standard-Map (z. B. /Game/Genesis/Conception/Maps/L_GEN_OviductAmpulla)
    [string]$Map = "",
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Project = Join-Path $RepoRoot "Genesis\Genesis.uproject"
$Editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$ShotDir = Join-Path $RepoRoot "Genesis\Saved\Screenshots"
$OutputPath = if ([IO.Path]::IsPathRooted($Output)) { $Output } else { Join-Path $RepoRoot $Output }
New-Item -ItemType Directory -Force -Path (Split-Path $OutputPath) | Out-Null

# "shot showui" als letzter Befehl: Die Engine speichert das nächste Bild inklusive HUD
$Commands = if ($ShotDelaySeconds -gt 0) { "$ExecCmds,genesis.Debug.After $ShotDelaySeconds shot showui" } else { "$ExecCmds,shot showui" }
$Started = Get-Date
$Arguments = @("`"$Project`"") + $(if ($Map) { @($Map) } else { @() }) + @("-game", "-windowed", "-ResX=1600", "-ResY=900", "-nosplash", "-ExecCmds=`"$Commands`"")
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -PassThru

$Shot = $null
$Deadline = $Started.AddSeconds($TimeoutSeconds)
while ((Get-Date) -lt $Deadline -and -not $Shot) {
    Start-Sleep -Seconds 2
    if ($Process.HasExited) { throw "Spiel wurde vorzeitig beendet (Exit $($Process.ExitCode))." }
    $Shot = Get-ChildItem $ShotDir -Recurse -Filter "ScreenShot*.png" -ErrorAction SilentlyContinue |
        Where-Object { $_.LastWriteTime -gt $Started } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
}

# Datei fertig schreiben lassen, dann Spiel beenden
Start-Sleep -Seconds 2
Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue

if (-not $Shot) { throw "Kein Screenshot innerhalb von $TimeoutSeconds s erzeugt." }
Copy-Item $Shot.FullName $OutputPath -Force
Remove-Item $Shot.FullName -Force
Write-Host "Screenshot: $OutputPath"
