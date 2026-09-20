@echo off
REM GENESIS: Der Kreislauf des Lebens – startet die zuletzt gebaute spielbare Fassung.
REM
REM Der Schalter -genesisplay lässt den Durchlauf von selbst beginnen: Befruchtung, erste Woche,
REM Schwangerschaft im Zeitraffer, Geburt, erste Stunde. Zu tun gibt es in der ersten Stunde etwas:
REM Leertaste = rufen, E = suchen (sobald das Kind auf der Haut liegt), Maus = hinsehen.
REM
REM Konsole mit der Taste ^ (darüber laufen alle genesis.*-Befehle, z. B. genesis.Debug.Page Slice).

setlocal
set "SPIEL=%~dp0Build\Windows\Genesis.exe"
if not exist "%SPIEL%" (
    echo Es gibt noch keine gebaute Fassung.
    echo Bauen mit:  powershell -ExecutionPolicy Bypass -File "%~dp0Tools\Build\Package-Game.ps1"
    pause
    exit /b 1
)
start "" "%SPIEL%" -genesisplay -windowed -ResX=1600 -ResY=900
endlocal
