# GENESIS Trailer - PLACEHOLDER Voice-over.
# Unreal Engine enthaelt keine deutsche Stimme (TextToSpeech-Plugin = Flite, nur Englisch).
# Deshalb Platzhalter ueber die Windows-OneCore-Stimmen (Stefan = Erzaehler, Katja = Kind).
# Die finale Stimme muss eine eigene/genehmigte Aufnahme sein (siehe Docs/Trailer/00_Trailer_Produktion.md).
# Aufruf: powershell -ExecutionPolicy Bypass -File Tools\Trailer\Audio\Generate-TempVO.ps1

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$ShotList = Join-Path $RepoRoot "Docs\Trailer\ShotList.json"
$OutDir = Join-Path $RepoRoot "Genesis\Saved\TrailerAudio\VO"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

[void][Windows.Media.SpeechSynthesis.SpeechSynthesizer, Windows.Media.SpeechSynthesis, ContentType = WindowsRuntime]
[void][Windows.Storage.Streams.DataReader, Windows.Storage.Streams, ContentType = WindowsRuntime]
Add-Type -AssemblyName System.Runtime.WindowsRuntime

$AsTask = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
    $_.Name -eq "AsTask" -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1' })[0]
function Wait-Async($Operation, [Type]$ResultType) {
    $Task = $AsTask.MakeGenericMethod($ResultType).Invoke($null, @($Operation))
    $Task.Wait() | Out-Null
    return $Task.Result
}

$Synth = New-Object Windows.Media.SpeechSynthesis.SpeechSynthesizer
$Voices = [Windows.Media.SpeechSynthesis.SpeechSynthesizer]::AllVoices
$Narrator = $Voices | Where-Object { $_.DisplayName -match "Stefan" } | Select-Object -First 1
$Child = $Voices | Where-Object { $_.DisplayName -match "Katja" } | Select-Object -First 1
if (-not $Narrator -or -not $Child) { throw "Deutsche OneCore-Stimmen Stefan/Katja nicht gefunden." }

$Data = Get-Content $ShotList -Raw -Encoding UTF8 | ConvertFrom-Json
foreach ($Line in $Data.voiceover) {
    $Text = [Security.SecurityElement]::Escape($Line.text.Trim())
    # Auslassungspunkte -> gesprochene Pause
    $Text = $Text.Replace([string][char]0x2026, '<break time="350ms"/>').Trim()
    if ($Line.speaker -eq "child") {
        $Synth.Voice = $Child
        $Prosody = '<prosody rate="-12%" pitch="+35%" volume="soft">'
    } elseif ($Line.speaker -eq "mother" -or $Line.speaker -eq "daughter") {
        $Synth.Voice = $Child
        $Prosody = '<prosody rate="-20%" pitch="-4%" volume="soft">'
    } else {
        $Synth.Voice = $Narrator
        $Prosody = '<prosody rate="-15%" pitch="-10%">'
    }
    $Ssml = '<speak version="1.0" xmlns="http://www.w3.org/2001/10/synthesis" xml:lang="de-DE">' + $Prosody + $Text + '</prosody></speak>'
    $Stream = Wait-Async ($Synth.SynthesizeSsmlToStreamAsync($Ssml)) ([Windows.Media.SpeechSynthesis.SpeechSynthesisStream])
    $Size = [uint32]$Stream.Size
    $Reader = New-Object Windows.Storage.Streams.DataReader($Stream.GetInputStreamAt(0))
    [void](Wait-Async ($Reader.LoadAsync($Size)) ([uint32]))
    $Bytes = New-Object byte[] $Size
    $Reader.ReadBytes($Bytes)
    $Path = Join-Path $OutDir ($Line.id + ".wav")
    [IO.File]::WriteAllBytes($Path, $Bytes)
    Write-Host ("{0}  {1} Bytes  {2}" -f $Line.id, $Size, $Synth.Voice.DisplayName)
}
Write-Host "VO-Platzhalter: $OutDir"
