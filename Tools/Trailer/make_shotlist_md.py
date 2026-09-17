# GENESIS Trailer - erzeugt Docs/Trailer/01_ShotList.md aus Docs/Trailer/ShotList.json.
# Aufruf: "C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe" Tools\Trailer\make_shotlist_md.py

import json
import os

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA = json.load(open(os.path.join(REPO, "Docs", "Trailer", "ShotList.json"), encoding="utf-8"))


def cell(value):
    return str(value).replace("|", "/").replace("\n", " ")


out = ["# 01 - Trailer Shot List (generiert)", "",
       "Automatisch erzeugt aus `ShotList.json` mit `Tools/Trailer/make_shotlist_md.py` - nicht von Hand bearbeiten.", "",
       "Version: **{}** · {} fps · {:.0f} s · {} Shots".format(DATA["version"], DATA["fps"], DATA["duration"], len(DATA["shots"])), ""]
for seq in DATA["sequences"]:
    shots = [s for s in DATA["shots"] if s["seq"] == seq["id"]]
    out.append("## {} ({:.1f}-{:.1f} s) · Akt {} · Look: {}".format(seq["id"], shots[0]["start"], shots[-1]["end"], seq["act"], seq["grade"]))
    out.append("")
    out.append("| ShotID | Start | Ende | Dauer | Beschreibung | Kamera | Figuren | Umgebung | Animation | VFX | Audio | Voice | Quelle | Status |")
    out.append("|---|---|---|---|---|---|---|---|---|---|---|---|---|---|")
    for s in shots:
        cam = s["cam"]
        cam_txt = "{} {} mm, {}, Fokus: {}".format(cam["rig"], cam["lens"], cam["move"], cam["focus"])
        voice = s.get("voice", "-")
        vo_text = next((v["text"] for v in DATA["voiceover"] if v["id"] == voice), "")
        source = s["source"] + (" ({})".format(s["gameplay_block"]) if s.get("gameplay_block") else "")
        out.append("| {} | {:.1f} | {:.1f} | {:.1f} | {} | {} | {} | {} | {} | {} | {} | {} | {} | {} |".format(
            s["id"], s["start"], s["end"], s["end"] - s["start"], cell(s["desc"]), cell(cam_txt), cell(s["chars"]), cell(s["env"]),
            cell(s["anim"]), cell(s["vfx"]), cell(s["audio"]), cell(voice + (": " + vo_text if vo_text else "")), cell(source), s["status"]))
    out.append("")

out += ["## Voice-over-Timing", "", "| ID | Start (s) | Sprecher | Text |", "|---|---|---|---|"]
for v in DATA["voiceover"]:
    out.append("| {} | {:.1f} | {} | {} |".format(v["id"], v["start"], v["speaker"], cell(v["text"])))
out += ["", "## Musikdramaturgie", "", "| Von | Bis | Zustand | Inhalt |", "|---|---|---|---|"]
for m in DATA["music"]:
    out.append("| {:.1f} | {:.1f} | {} | {} |".format(m["start"], m["end"], m["state"], cell(m["note"])))
out.append("")
open(os.path.join(REPO, "Docs", "Trailer", "01_ShotList.md"), "w", encoding="utf-8", newline="\n").write("\n".join(out))
print("01_ShotList.md geschrieben")
