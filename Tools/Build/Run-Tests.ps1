# GENESIS – Automation-Tests headless ausführen
# Aufruf:  powershell -ExecutionPolicy Bypass -File Tools\Build\Run-Tests.ps1 [-Filter Genesis]
# Ergebnis: Genesis\Saved\Automation\Reports\index.json + Zusammenfassung in der Konsole. Exit-Code 0 = alle Tests grün.

param(
    [string]$Filter = "Genesis",
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Project = Join-Path $RepoRoot "Genesis\Genesis.uproject"
$Editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$ReportDir = Join-Path $RepoRoot "Genesis\Saved\Automation\Reports"
$LogFile = Join-Path $RepoRoot "Genesis\Saved\Logs\GenesisTests.log"

if (Test-Path $ReportDir) { Remove-Item -Recurse -Force $ReportDir }
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null

$Arguments = @(
    "`"$Project`"",
    "-ExecCmds=`"Automation RunTests $Filter;Quit`"",
    "-TestExit=`"Automation Test Queue Empty`"",
    "-ReportExportPath=`"$ReportDir`"",
    "-unattended", "-nopause", "-nosplash", "-NullRHI", "-nosound",
    "-abslog=`"$LogFile`""
)

Write-Host "GENESIS Tests: Filter '$Filter'" -ForegroundColor Cyan
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -Wait -PassThru -NoNewWindow

$Index = Join-Path $ReportDir "index.json"
if (-not (Test-Path $Index)) {
    Write-Host "Kein Testbericht erzeugt (Exit $($Process.ExitCode)). Log: $LogFile" -ForegroundColor Red
    exit 2
}

# Der Bericht kann ein BOM enthalten
$Report = Get-Content $Index -Raw -Encoding UTF8 | ConvertFrom-Json
$Failed = @($Report.tests | Where-Object { $_.state -ne "Success" })
$WithWarnings = @($Report.tests | Where-Object { $_.warnings -gt 0 })

foreach ($Test in $Report.tests) {
    $Color = if ($Test.state -eq "Success") { "Green" } else { "Red" }
    Write-Host ("[{0}] {1}" -f $Test.state, $Test.fullTestPath) -ForegroundColor $Color
    if ($Test.state -ne "Success") {
        foreach ($Entry in $Test.entries) {
            if ($Entry.event.type -eq "Error") { Write-Host ("    " + $Entry.event.message) -ForegroundColor Red }
        }
    }
}

Write-Host ("Tests: {0}  Erfolgreich: {1}  davon mit Warnungen: {2}  Fehlgeschlagen: {3}" -f $Report.tests.Count, ($Report.tests.Count - $Failed.Count), $WithWarnings.Count, $Failed.Count)
if ($Failed.Count -gt 0 -or $Report.tests.Count -eq 0) { exit 1 }
exit 0
