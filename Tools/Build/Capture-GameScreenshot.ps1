# GENESIS – startet das Spiel (Standalone), führt Konsolenbefehle aus und speichert einen Screenshot des Fensters.
# Dient der visuellen Prüfung (z. B. Developer HUD) ohne manuelle Bedienung.
# Aufruf:
#   powershell -ExecutionPolicy Bypass -File Tools\Build\Capture-GameScreenshot.ps1 -ExecCmds "showdebug Genesis" -Output Docs\Media\hud.png

param(
    [string]$ExecCmds = "showdebug Genesis",
    [string]$Output = "Genesis\Saved\Screenshots\genesis_capture.png",
    [int]$WarmupSeconds = 25,
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8"
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class GenesisWin32 {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
"@

# Physische Pixel statt skalierter Koordinaten (Windows-Anzeigeskalierung)
[GenesisWin32]::SetProcessDPIAware() | Out-Null

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Project = Join-Path $RepoRoot "Genesis\Genesis.uproject"
$Editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$OutputPath = if ([IO.Path]::IsPathRooted($Output)) { $Output } else { Join-Path $RepoRoot $Output }
New-Item -ItemType Directory -Force -Path (Split-Path $OutputPath) | Out-Null

$Arguments = @("`"$Project`"", "-game", "-windowed", "-ResX=1600", "-ResY=900", "-nosplash", "-ExecCmds=`"$ExecCmds`"")
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -PassThru

# Auf das Spielfenster warten
$Deadline = (Get-Date).AddSeconds(180)
while ((Get-Date) -lt $Deadline) {
    $Process.Refresh()
    if ($Process.HasExited) { throw "Spiel wurde vorzeitig beendet (Exit $($Process.ExitCode))." }
    if ($Process.MainWindowHandle -ne [IntPtr]::Zero -and $Process.MainWindowTitle) { break }
    Start-Sleep -Milliseconds 500
}

# Shader-Kompilierung und erste Frames abwarten
Start-Sleep -Seconds $WarmupSeconds
$Process.Refresh()
[GenesisWin32]::SetForegroundWindow($Process.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 800

$Rect = New-Object GenesisWin32+RECT
[GenesisWin32]::GetWindowRect($Process.MainWindowHandle, [ref]$Rect) | Out-Null
$Width = $Rect.Right - $Rect.Left
$Height = $Rect.Bottom - $Rect.Top
$Bitmap = New-Object System.Drawing.Bitmap $Width, $Height
$Graphics = [System.Drawing.Graphics]::FromImage($Bitmap)
$Graphics.CopyFromScreen($Rect.Left, $Rect.Top, 0, 0, $Bitmap.Size)
$Bitmap.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
$Graphics.Dispose(); $Bitmap.Dispose()

Stop-Process -Id $Process.Id -Force
Write-Host "Screenshot: $OutputPath ($Width x $Height)"
