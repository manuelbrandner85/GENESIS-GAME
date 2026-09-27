# GENESIS: Der Kreislauf des Lebens
#
# Die Hüllen der frühen Wochen (SSW 8–12, Docs/37 Teil 2c): In dieser Zeit schwimmt der Embryo nicht frei in der
# Gebärmutter, sondern in der dünnen, glasklaren Amnionblase; sie liegt in der größeren Chorionhöhle (extraembryonales
# Zölom), und dort schwimmt auch der Dottersack an seinem dünnen Stiel.
#
#   * Amnion: Durchmesser ≈ 1,1 × Scheitel-Steiß-Länge − 0,07 cm (Horrow 1992, AJR 158:359, DOI 10.2214/ajr.158.2.1729798);
#     wächst, bis es um SSW 12–14 an der Chorionwand anliegt. Glatt, glänzend, kaum dicker als 0,02–0,05 mm.
#   * Dottersack: normal 4–5 mm, sichtbar SSW 5–10 (Rempen 1987, Geburtshilfe Frauenheilkd 47:477,
#     DOI 10.1055/s-2008-1035856), danach Rückbildung; kugelig bis leicht birnenförmig, mit Dottergefäßen.
#   * Dottergang: dünner Stiel vom Dottersack zum Nabelstrang.
#
# Ausgabe (Einheitsgrößen, AGenesisWombScene skaliert und legt sie je Woche): SM_GEN_Amnion (Radius 1 cm, Mitte 0),
# SM_GEN_YolkSac (Radius 1 cm, Stiel-Ansatz bei −X), SM_GEN_YolkStalk (Zylinder Radius 1 cm, Länge 1 cm entlang +X).
#
#   blender -b --factory-startup --python-expr "g={'__name__':'genesis'}; exec(open(r'...build_early_sac.py', encoding='utf-8').read(), g); g['export_all']()"

import math
import os

import bpy
import numpy as np

CM = 0.01
OUT_DIR = r"C:\Users\manue\Desktop\Genesis Game\ArtSource\Generated\Gestation"
rng = np.random.default_rng(8)


def noise_field(unit, freq, seed):
    """Glattes Rauschen auf der Kugel aus wenigen zufälligen Wellen (reproduzierbar)."""
    r = np.random.default_rng(seed)
    total = np.zeros(len(unit))
    for _ in range(12):
        d = r.normal(size=3)
        d /= np.linalg.norm(d)
        total += np.sin(freq * (unit @ d) * (0.6 + 0.8 * r.random()) + r.random() * 6.283)
    return total / 12.0


def sphere(name, subdiv, shape):
    """Ikosaeder-Kugel (gleichmäßige Dreiecke, keine Pole) mit Formfunktion shape(unit) -> Radius (cm)."""
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdiv, radius=1.0)
    obj = bpy.context.active_object
    obj.name = name
    mesh = obj.data
    co = np.array([v.co[:] for v in mesh.vertices])
    unit = co / np.linalg.norm(co, axis=1, keepdims=True)
    radius = shape(unit)
    for v, u, r in zip(mesh.vertices, unit, radius):
        v.co = tuple(u * r * CM)
    for p in mesh.polygons:
        p.use_smooth = True
    # Kugel-UV (u um die Achse, v von Pol zu Pol) – für die Gefäße des Dottersacks
    uv = mesh.uv_layers.new(name="UV")
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            u = unit[mesh.loops[li].vertex_index]
            uv.data[li].uv = (0.5 + math.atan2(u[1], u[0]) / (2 * math.pi), 0.5 + math.asin(max(-1.0, min(1.0, u[2]))) / math.pi)
    mesh.update()
    return obj


def amnion():
    # Fast kugelig, sanft unregelmäßig (Flüssigkeit spannt die Haut; ±2 %)
    return sphere("SM_GEN_Amnion", 6, lambda u: 1.0 + 0.02 * noise_field(u, 3.0, 11))


def yolk_sac():
    # Kugel, zur Stielseite (−X) leicht ausgezogen (birnenförmig), feine Unebenheit
    return sphere("SM_GEN_YolkSac", 5, lambda u: 1.0 + 0.08 * np.clip(-u[:, 0], 0, 1) ** 3 + 0.015 * noise_field(u, 6.0, 21))


def yolk_stalk():
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=1.0 * CM, depth=1.0 * CM, location=(0.5 * CM, 0, 0),
                                        rotation=(0, math.pi / 2, 0))
    obj = bpy.context.active_object
    obj.name = "SM_GEN_YolkStalk"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj


def export_all(out_dir=OUT_DIR):
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for build in (amnion, yolk_sac, yolk_stalk):
        obj = build()
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        path = os.path.join(out_dir, obj.name + ".fbx")
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                                 object_types={"MESH"}, mesh_smooth_type="FACE", use_tspace=True, add_leaf_bones=False)
        print("GENESIS: exportiert", path, len(obj.data.vertices), "Punkte")
