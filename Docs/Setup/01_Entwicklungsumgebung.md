# Setup: Entwicklungsumgebung

## Benötigte Software

| Software | Version | Hinweis |
|---|---|---|
| Unreal Engine | 5.8.2 | Epic Games Launcher |
| Visual Studio Community | 2026 (18.x) | Komponenten aus `Genesis/.vsconfig` (MSVC 14.50 LTSC, Windows 11 SDK 22621, Unreal-Integration) |
| Git + Git LFS | aktuell | `git lfs install` einmalig |
| GitHub CLI | aktuell | `gh auth login` einmalig |
| Blender | 5.2 | MCP-Addon für Automatisierung |

## Visual Studio installieren (automatisierbar)

```bash
winget install --exact --id Microsoft.VisualStudio.Community --source winget --override "--passive --wait --norestart --config C:\Pfad\zu\Genesis\.vsconfig"
```

Visual Studio ist bereits installiert? Dann den **Visual Studio Installer** öffnen → **Weitere** → **Konfiguration importieren** → `Genesis/.vsconfig` wählen → **Überprüfen**.

## Repository klonen

```bash
git clone https://github.com/manuelbrandner85/GENESIS-GAME.git
```

## Bauen

```bash
powershell -ExecutionPolicy Bypass -File Tools\Build\Build-Editor.ps1
```

Danach `Genesis/Genesis.uproject` per Doppelklick öffnen.

## Tests ausführen

```bash
powershell -ExecutionPolicy Bypass -File Tools\Build\Run-Tests.ps1
```

Im Editor alternativ: **Tools → Session Frontend → Automation** → Filter `Genesis` → **Start Tests**.

## Developer HUD

Im laufenden Spiel (PIE) die Konsole mit `^` öffnen:
- `showdebug Genesis` blendet alle Seiten ein und aus.
- `genesis.Debug.Page Save` zeigt nur eine Seite.

## Entwicklerbefehle (nicht in Shipping)

| Befehl | Wirkung |
|---|---|
| `genesis.Clock.SkipDays 365` | Zeitsprung um N Tage |
| `genesis.Clock.TimeScale 3600` | Weltsekunden pro Echtzeitsekunde |
| `genesis.Save.All [Profil]` | Soul, World und Life speichern |
| `genesis.Save.LoadAll [Profil]` | Soul, World und Life laden |
| `genesis.Soul.Create [Seed]` | Spielerseele anlegen und erste Inkarnation beginnen |
| `genesis.Soul.SimulateLife` | Beispiel-Lebensabschluss und nächste Inkarnation |
| `genesis.Genetics.SimulateFamily [Jahre]` | Zwei Gründer, Mutter N Jahre Stress, Kind zeugen |
| `genesis.Memory.SimulateLieChain` | Beispiel-Kausalkette über Generationen mit Erinnerungen |

## Automatisierte Sichtprüfung

`Tools\Build\Capture-GameScreenshot.ps1` startet das Spiel, führt Konsolenbefehle aus und speichert ein Bildschirmfoto. In der Engine geht das auch über `shot showui` als letzten Befehl in `-ExecCmds`.

## Entwickler-Map neu erzeugen

```bash
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "Genesis\Genesis.uproject" -run=pythonscript -script="Tools/Unreal/CreateDevSandbox.py" -unattended -nullrhi
```
