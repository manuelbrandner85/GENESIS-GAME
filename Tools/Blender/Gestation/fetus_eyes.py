# GENESIS: Der Kreislauf des Lebens
#
# Trägt den halben Augenabstand je Woche in fetus_weeks.json nach, ohne die Netze neu zu bauen (build_fetus.py schreibt
# ihn beim nächsten vollen Bau selbst). Der Abstand hängt nur von den Maßen ab, nicht von der Haltung.
#
#   blender -b --python-expr "g={'__name__':'genesis'}; exec(open(r'...fetus_eyes.py', encoding='utf-8').read(), g)"

import json
import os

import bpy

HIER = r"C:\Users\manue\Desktop\Genesis Game\Tools\Blender\Gestation"
g = {"__name__": "genesis"}
exec(open(os.path.join(HIER, "build_fetus.py"), encoding="utf-8").read(), g)
pfad = os.path.join(g["OUT_DIR"], "fetus_weeks.json")
tabelle = json.load(open(pfad, encoding="utf-8"))
for woche in sorted(int(w) for w in tabelle):
    g["clean_scene"]()
    basemesh, rig = g["create_baby"](woche)
    base = g["measure"](basemesh, rig)
    g["proportion"](rig, base, woche)
    bpy.context.view_layer.update()
    links = rig.matrix_world @ rig.pose.bones["eye.L"].head
    rechts = rig.matrix_world @ rig.pose.bones["eye.R"].head
    tabelle[str(woche)]["eye_half_cm"] = float((links - rechts).length / 2.0 / g["CM"])
    print("GENESIS: SSW %d – halber Augenabstand %.3f cm" % (woche, tabelle[str(woche)]["eye_half_cm"]))
json.dump(tabelle, open(pfad, "w", encoding="utf-8"), indent=1)
