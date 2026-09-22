# GENESIS Trailer - rendert eine Level Sequence mit der Movie Render Queue (Kommandozeile, echtes GPU-Rendering).
# Aufruf:
#   powershell -ExecutionPolicy Bypass -File Tools\Trailer\Render-Sequence.ps1 -Map /Game/Genesis/Trailer/Maps/L_SEQ001_Origin `
#       -Sequence /Game/Genesis/Trailer/Sequences/Sub/SEQ_001_Origin -Preset MRQ_Test_1080p
# Ergebnis: Genesis\Saved\MovieRenders\<Sequenz>\<Preset>\*.png|exr

param(
    [Parameter(Mandatory = $true)][string]$Map,
    [Parameter(Mandatory = $true)][string]$Sequence,
    [string]$Preset = "MRQ_Test_1080p",
    [int]$TimeoutMinutes = 180,
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Project = Join-Path $RepoRoot "Genesis\Genesis.uproject"
$Editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$SeqName = $Sequence.Split("/")[-1]
$OutDir = Join-Path $RepoRoot "Genesis\Saved\MovieRenders\$SeqName\$Preset"
$LogFile = Join-Path $RepoRoot "Genesis\Saved\Logs\Render_${SeqName}_$Preset.log"

$Builders = Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match "^(UnrealBuildTool|link|cl)$" }
if ($Builders) { throw "Ein C++-Build laeuft gerade. Spaeter erneut starten." }
if (Test-Path $OutDir) { Remove-Item -Recurse -Force $OutDir }

$Arguments = @(
    "`"$Project`"", $Map, "-game",
    "-LevelSequence=`"$Sequence.$SeqName`"",
    "-MoviePipelineConfig=`"/Game/Genesis/Trailer/Render/$Preset.$Preset`"",
    "-windowed", "-ResX=1280", "-ResY=720", "-NoLoadingScreen", "-NoScreenMessages", "-nosound",
    "-abslog=`"$LogFile`""
)
$Started = Get-Date
Write-Host "MRQ: $SeqName mit $Preset" -ForegroundColor Cyan
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -PassThru
if (-not $Process.WaitForExit($TimeoutMinutes * 60 * 1000)) {
    Stop-Process -Id $Process.Id -Force
    throw "Render-Timeout nach $TimeoutMinutes Minuten."
}
$Frames = @(Get-ChildItem $OutDir -File -ErrorAction SilentlyContinue)
$Minutes = ((Get-Date) - $Started).TotalMinutes
Select-String -Path $LogFile -Pattern "LogMovieRenderPipeline.*(Error|Warning)|Fatal error" -ErrorAction SilentlyContinue | Select-Object -First 15 | ForEach-Object { Write-Host $_.Line -ForegroundColor Yellow }
Write-Host ("Frames: {0}  Dauer: {1:N1} min  Exit: {2}  Ausgabe: {3}" -f $Frames.Count, $Minutes, $Process.ExitCode, $OutDir)
if ($Frames.Count -eq 0) { Write-Host "Keine Frames gerendert. Log: $LogFile" -ForegroundColor Red; exit 1 }
exit 0
