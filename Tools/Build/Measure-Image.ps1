# GENESIS – misst die Belichtung eines Screenshots: Leuchtdichte-Perzentile und Anteil ausgefressener/abgesoffener Pixel.
# Ohne Messung ist "zu hell" oder "zu dunkel" nur ein Eindruck – mit Messung eine Zahl.
# Aufruf:
#   powershell -ExecutionPolicy Bypass -File Tools\Build\Measure-Image.ps1 -Path ArtSource\Generated\Conception\UE_oocyte_01.png

param(
    [Parameter(Mandatory = $true)][string]$Path,
    # Bildkanten beschneiden (Anteil je Seite), z. B. um HUD-Ränder auszublenden
    [double]$Crop = 0.0
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$Full = if ([IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..\..")) $Path }
$Bitmap = [System.Drawing.Bitmap]::FromFile($Full)
try {
    $X0 = [int]($Bitmap.Width * $Crop)
    $Y0 = [int]($Bitmap.Height * $Crop)
    $X1 = $Bitmap.Width - $X0
    $Y1 = $Bitmap.Height - $Y0

    # Jeden vierten Pixel abtasten: für Perzentile genau genug, aber schnell
    $Values = New-Object 'System.Collections.Generic.List[double]'
    $Clipped = 0
    $Black = 0
    for ($y = $Y0; $y -lt $Y1; $y += 4) {
        for ($x = $X0; $x -lt $X1; $x += 4) {
            $Pixel = $Bitmap.GetPixel($x, $y)
            # Rec. 709 auf den sRGB-Werten: das ist die Helligkeit, die das Auge im fertigen Bild sieht
            $Luma = (0.2126 * $Pixel.R + 0.7152 * $Pixel.G + 0.0722 * $Pixel.B) / 255.0
            $Values.Add($Luma)
            if ($Pixel.R -ge 250 -and $Pixel.G -ge 250 -and $Pixel.B -ge 250) { $Clipped++ }
            if ($Luma -le 0.02) { $Black++ }
        }
    }

    $Sorted = $Values.ToArray()
    [Array]::Sort($Sorted)
    function Percentile([double[]]$Data, [double]$P) { $Data[[int][Math]::Floor(($Data.Length - 1) * $P)] }

    $Count = $Sorted.Length
    [PSCustomObject]@{
        Datei      = Split-Path $Full -Leaf
        Pixel      = $Count
        Mittel     = [Math]::Round(($Sorted | Measure-Object -Average).Average, 4)
        P01        = [Math]::Round((Percentile $Sorted 0.01), 4)
        P10        = [Math]::Round((Percentile $Sorted 0.10), 4)
        Median     = [Math]::Round((Percentile $Sorted 0.50), 4)
        P90        = [Math]::Round((Percentile $Sorted 0.90), 4)
        P99        = [Math]::Round((Percentile $Sorted 0.99), 4)
        WeissProz  = [Math]::Round(100.0 * $Clipped / $Count, 2)
        SchwarzProz = [Math]::Round(100.0 * $Black / $Count, 2)
    } | Format-List
}
finally {
    $Bitmap.Dispose()
}
