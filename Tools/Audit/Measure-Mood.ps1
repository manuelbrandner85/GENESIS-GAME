# GENESIS – Farbstimmung eines Bildes gegen das Cover messen (Doc 00b, Referenz-Matching; Doc 31).
# Mittlere Leuchtdichte (Rec. 709 auf sRGB), Anteil warmer und kalter Pixel, Tiefen, Spitzlichter, mittlere Sättigung.
#   warm: Farbton 0–60° oder 330–360°, Sättigung ≥ 0,15, Helligkeit ≥ 0,08
#   kalt: Farbton 170–260°, Sättigung ≥ 0,10, Helligkeit ≥ 0,08
#   Tiefen: Leuchtdichte ≤ 0,02 · Spitzlichter: Leuchtdichte ≥ 0,85
# Aufruf: powershell -ExecutionPolicy Bypass -File Tools\Audit\Measure-Mood.ps1 -Path a.png,b.png

param([Parameter(Mandatory = $true)][string[]]$Path)

Add-Type -AssemblyName System.Drawing
foreach ($File in ($Path | ForEach-Object { $_ -split "," })) {
    $Bitmap = [System.Drawing.Bitmap]::FromFile((Resolve-Path $File))
    try {
        $Sum = 0.0; $Warm = 0; $Cold = 0; $Deep = 0; $High = 0; $Sat = 0.0; $Count = 0
        for ($y = 0; $y -lt $Bitmap.Height; $y += 3) {
            for ($x = 0; $x -lt $Bitmap.Width; $x += 3) {
                $P = $Bitmap.GetPixel($x, $y)
                $Luma = (0.2126 * $P.R + 0.7152 * $P.G + 0.0722 * $P.B) / 255.0
                $Hue = $P.GetHue(); $S = $P.GetSaturation(); $V = [Math]::Max($P.R, [Math]::Max($P.G, $P.B)) / 255.0
                $Sum += $Luma; $Sat += $S; $Count++
                if ($Luma -le 0.02) { $Deep++ }
                if ($Luma -ge 0.85) { $High++ }
                if ($V -ge 0.08) {
                    if (($Hue -le 60 -or $Hue -ge 330) -and $S -ge 0.15) { $Warm++ }
                    elseif ($Hue -ge 170 -and $Hue -le 260 -and $S -ge 0.10) { $Cold++ }
                }
            }
        }
        [PSCustomObject]@{
            Bild = Split-Path $File -Leaf
            Leuchtdichte = [Math]::Round($Sum / $Count, 3)
            Warm = "{0:N0} %" -f (100.0 * $Warm / $Count)
            Kalt = "{0:N0} %" -f (100.0 * $Cold / $Count)
            Tiefen = "{0:N1} %" -f (100.0 * $Deep / $Count)
            Spitzlichter = "{0:N1} %" -f (100.0 * $High / $Count)
            Saettigung = [Math]::Round($Sat / $Count, 2)
        }
    }
    finally { $Bitmap.Dispose() }
}
