# GENESIS – Bestandsaufnahme der Unreal-Inhalte (Doc 00b, Asset-Audit).
#
# Listet alle Assets unter /Game mit Klasse, Größe auf der Platte, wer sie referenziert und ob sie von einer Karte des
# Spielpakets (Package-Game.ps1), einem immer gekochten Ordner oder dem C++/Python-Code aus erreichbar sind.
# Schreibt JSON nach Saved/Audit/content_audit.json. Nur lesend – ändert nichts.
#
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Audit/audit_content.py" -unattended -nullrhi

import json
import os
import re

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
REPO = os.path.abspath(os.path.join(PROJECT, ".."))
OUT = os.path.join(PROJECT, "Saved", "Audit", "content_audit.json")

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(["/Game"], True)


def package_file(package):
    """Datei eines Pakets auf der Platte (.uasset oder .umap)."""
    rel = package[len("/Game/"):] if package.startswith("/Game/") else None
    if rel is None:
        return None
    for ext in (".uasset", ".umap"):
        path = os.path.join(PROJECT, "Content", rel.replace("/", os.sep) + ext)
        if os.path.exists(path):
            return path
    return None


def dependency_options():
    options = unreal.AssetRegistryDependencyOptions()
    options.include_hard_package_references = True
    options.include_soft_package_references = True
    options.include_searchable_names = False
    options.include_soft_management_references = False
    options.include_hard_management_references = False
    return options


OPTIONS = dependency_options()

assets = registry.get_assets_by_path("/Game", recursive=True)
packages = {}
for data in assets:
    package = str(data.package_name)
    entry = packages.setdefault(package, {"classes": [], "names": []})
    entry["classes"].append(str(data.asset_class_path.asset_name))
    entry["names"].append(str(data.asset_name))

# Wurzeln: Karten des Spielpakets, immer gekochte Ordner, Pfade im Code und in den Werkzeugskripten
roots = set()
package_script = os.path.join(REPO, "Tools", "Build", "Package-Game.ps1")
if os.path.exists(package_script):
    text = open(package_script, encoding="utf-8-sig").read()
    for match in re.findall(r"/Game/[A-Za-z0-9_/]+", text):
        roots.add(match)
config = os.path.join(PROJECT, "Config", "DefaultGame.ini")
always_cook = []
if os.path.exists(config):
    for line in open(config, encoding="utf-8-sig"):
        m = re.search(r'DirectoriesToAlwaysCook=\(Path="([^"]+)"\)', line)
        if m:
            always_cook.append(m.group(1))
for directory in always_cook:
    for package in packages:
        if package.startswith(directory.rstrip("/") + "/"):
            roots.add(package)

code_refs = {}
for base in (os.path.join(PROJECT, "Source"), os.path.join(PROJECT, "Plugins"), os.path.join(PROJECT, "Config")):
    for folder, _, files in os.walk(base):
        if "Intermediate" in folder or "Binaries" in folder:
            continue
        for name in files:
            if not name.endswith((".cpp", ".h", ".ini")):
                continue
            try:
                text = open(os.path.join(folder, name), encoding="utf-8", errors="ignore").read()
            except OSError:
                continue
            for match in re.findall(r"/Game/[A-Za-z0-9_/.]+", text):
                package = match.split(".")[0]
                code_refs.setdefault(package, set()).add(os.path.relpath(os.path.join(folder, name), REPO))
for package in code_refs:
    roots.add(package)

# Erreichbarkeit: alle Abhängigkeiten der Wurzeln, transitiv
reachable = set()
stack = [r for r in roots if r in packages]
while stack:
    package = stack.pop()
    if package in reachable:
        continue
    reachable.add(package)
    for dep in registry.get_dependencies(package, OPTIONS) or []:
        dep = str(dep)
        if dep.startswith("/Game/") and dep not in reachable:
            stack.append(dep)

report = []
for package, entry in sorted(packages.items()):
    referencers = [str(r) for r in (registry.get_referencers(package, OPTIONS) or []) if str(r) != package]
    path = package_file(package)
    report.append({
        "package": package,
        "classes": entry["classes"],
        "bytes": os.path.getsize(path) if path else 0,
        "referencers": referencers,
        "reachable": package in reachable,
        "code": sorted(code_refs.get(package, [])),
        "redirector": "ObjectRedirector" in entry["classes"],
    })

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as handle:
    json.dump({"roots": sorted(roots), "always_cook": always_cook, "assets": report}, handle, indent=1)
unreal.log_warning("GENESIS-Audit: %d Pakete, %d erreichbar, %d Redirectors -> %s" % (
    len(report), len(reachable), sum(1 for r in report if r["redirector"]), OUT))
