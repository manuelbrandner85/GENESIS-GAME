# GENESIS: Der Kreislauf des Lebens
# Baut den Kreißsaal, in den das Kind hineingeboren wird – in echten Maßen.
#
# Aufruf:
#   blender -b --factory-startup -P Tools/Blender/Birth/build_delivery_room.py -- --out ArtSource/Generated/Birth --export
#
# Maßstab wie der Geburtskanal: 1 mm = 1 Unreal-Einheit, in Blender 1 mm = 1 cm (UNIT = 0,01).
# Ursprung = Ausgang des Geburtskanals. +X zeigt zum Fußende des Bettes in den Raum, +Z nach oben,
# der Boden liegt 800 mm tiefer (Sitzfläche eines Entbindungsbettes ~ 750–800 mm über dem Boden).
#
# Warum so genau, wenn ein Neugeborenes ohnehin nur auf 25 cm scharf sieht? Weil Unschärfe nichts
# verzeiht, was an Licht, Farbe und Maßstab falsch ist: Ein Fenster, das doppelt so groß ist, ein
# Deckenfeld in der falschen Höhe, eine Wand in falscher Helligkeit – das bleibt auch verschwommen
# sichtbar. Was die Unschärfe schluckt, sind Kleinteile; genau dort wird hier gespart.
#
# Referenzmaße (Kreißsaal in Deutschland, übliche Ausstattung):
#   Raum 5,4 × 4,6 m, lichte Höhe 2,9 m, Rasterdecke 625 mm mit LED-Feldern (4000 K, nach der Geburt gedimmt)
#   Entbindungsbett 2100 × 1000 mm, Sitzfläche ~800 mm, Rückenteil hochgestellt (45°), Fußteil abgesenkt
#   Fenster 1500 × 1400 mm, Brüstung 850 mm, Lamellen-Jalousie halb geschlossen
#   Hygienische Hohlkehle am Boden (PVC hochgezogen, 100 mm), Wandfarbe hell, eine Wand ruhig getönt
#   Wärmebett (Reanimationseinheit) mit Heizstrahler, CTG auf Wagen, Infusionsständer, Wanduhr,
#   Geburtsseil (Tuch) von der Decke, Gebärball, Hocker der Hebamme, Waschbecken, Schrankzeile

import math
import os
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Conception"))
from genesis_blender_common import UM as UNIT, ensure_dir, reset_scene, script_args
import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

FLOOR = -800.0
CEILING = FLOOR + 2900.0
ROOM_X = (-2000.0, 3400.0)
ROOM_Y = (-2300.0, 2300.0)

rng = np.random.default_rng(20260921)


# ---------------------------------------------------------------------------------------------------------------------
# Grundformen – immer mit Fase, nie mit CAD-Kante
# ---------------------------------------------------------------------------------------------------------------------
def mm(vector):
    return Vector(vector) * UNIT


def new_object(name, bm, material):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    slot_material = bpy.data.materials.get(material) or bpy.data.materials.new(material)
    obj.data.materials.append(slot_material)
    # Kanten, die schärfer als 40° sind, bleiben hart – sonst verschmieren die Normalen gefaster Kästen
    obj.data.set_sharp_from_angle(angle=math.radians(40.0))
    return obj


def box(name, center, size, material, bevel=4.0, rotation=None):
    """Ein Kasten mit gefasten Kanten. Maße in mm."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=Vector(size), verts=bm.verts)
    if bevel > 0.0:
        radius = min(bevel, 0.45 * min(size))
        bmesh.ops.bevel(bm, geom=list(bm.edges) + list(bm.verts), offset=radius, segments=3, affect="EDGES", profile=0.5)
    if rotation is not None:
        bmesh.ops.rotate(bm, verts=bm.verts, cent=Vector((0, 0, 0)), matrix=rotation)
    bmesh.ops.translate(bm, vec=Vector(center), verts=bm.verts)
    bmesh.ops.scale(bm, vec=Vector((UNIT, UNIT, UNIT)), verts=bm.verts)
    return new_object(name, bm, material)


def cylinder(name, start, end, radius, material, segments=32, cap_bevel=1.5):
    """Rohr/Stange von start nach end (mm)."""
    start, end = Vector(start), Vector(end)
    axis = end - start
    length = axis.length
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=segments, radius1=radius, radius2=radius, depth=length)
    if cap_bevel > 0.0:
        caps = [edge for edge in bm.edges if abs(edge.verts[0].co.z - edge.verts[1].co.z) < 1e-6]
        bmesh.ops.bevel(bm, geom=caps, offset=min(cap_bevel, radius * 0.4), segments=2, affect="EDGES", profile=0.5)
    rotation = axis.normalized().to_track_quat("Z", "Y").to_matrix().to_4x4()
    bmesh.ops.transform(bm, matrix=rotation, verts=bm.verts)
    bmesh.ops.translate(bm, vec=(start + end) * 0.5, verts=bm.verts)
    bmesh.ops.scale(bm, vec=Vector((UNIT, UNIT, UNIT)), verts=bm.verts)
    return new_object(name, bm, material)


def sphere(name, center, radius, material, squash=(1.0, 1.0, 1.0), subdivisions=4):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdivisions, radius=radius)
    bmesh.ops.scale(bm, vec=Vector(squash), verts=bm.verts)
    bmesh.ops.translate(bm, vec=Vector(center), verts=bm.verts)
    bmesh.ops.scale(bm, vec=Vector((UNIT, UNIT, UNIT)), verts=bm.verts)
    return new_object(name, bm, material)


def join(name, objects):
    """Mehrere Teile zu einem Mesh mit mehreren Materialplätzen."""
    objects = [obj for obj in objects if obj is not None]
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    result = bpy.context.view_layer.objects.active
    result.name = name
    result.data.name = name
    return result


# ---------------------------------------------------------------------------------------------------------------------
# Raum
# ---------------------------------------------------------------------------------------------------------------------
WINDOW_X = (-1150.0, 350.0)          # 1500 mm breit, in der Wand +Y, neben dem Bett
WINDOW_Z = (FLOOR + 850.0, FLOOR + 2250.0)
DOOR_Y = (-1500.0, -400.0)           # Doppelflügeltür (Bettentür) in der Stirnwand +X, 1100 mm


def wall_with_openings(name, axis, position, span, height, openings, material, thickness=180.0):
    """Wandscheibe mit rechteckigen Öffnungen: als Streifen gebaut, damit Laibungen echte Tiefe haben."""
    parts = []
    # Senkrechte Schnitte an den Öffnungskanten, dann jeden Streifen oben/unten um die Öffnung herum
    cuts = sorted({span[0], span[1]} | {value for opening in openings for value in opening[0]})
    for left, right in zip(cuts[:-1], cuts[1:]):
        width = right - left
        if width < 1.0:
            continue
        middle = 0.5 * (left + right)
        blocked = [opening for opening in openings if opening[0][0] <= middle <= opening[0][1]]
        spans = [(FLOOR, FLOOR + height)]
        for opening in blocked:
            low, high = opening[1]
            new_spans = []
            for bottom, top in spans:
                if low > bottom:
                    new_spans.append((bottom, min(top, low)))
                if high < top:
                    new_spans.append((max(bottom, high), top))
            spans = new_spans
        for bottom, top in spans:
            if top - bottom < 1.0:
                continue
            if axis == "y":   # Wand parallel zu X (liegt bei festem Y)
                center = (middle, position, 0.5 * (bottom + top))
                size = (width, thickness, top - bottom)
            else:             # Wand parallel zu Y (liegt bei festem X)
                center = (position, middle, 0.5 * (bottom + top))
                size = (thickness, width, top - bottom)
            parts.append(box("%s_%d" % (name, len(parts)), center, size, material, bevel=1.0))
    return parts


def build_room():
    parts = []
    height = CEILING - FLOOR
    x0, x1 = ROOM_X
    y0, y1 = ROOM_Y

    # Boden: PVC-Bahnenware. Die Schweißnähte zwischen den Bahnen (2 m breit) sind eine leicht dunklere Linie.
    parts.append(box("Floor", (0.5 * (x0 + x1), 0.5 * (y0 + y1), FLOOR - 20.0), (x1 - x0 + 400.0, y1 - y0 + 400.0, 40.0), "Floor", bevel=0.0))
    for seam_y in np.arange(y0 + 2000.0, y1, 2000.0):
        parts.append(box("FloorSeam", (0.5 * (x0 + x1), seam_y, FLOOR + 0.2), (x1 - x0, 3.0, 0.6), "FloorSeam", bevel=0.0))

    # Wände: +Y mit Fenster, +X (Stirnwand) mit Tür, -X hinter dem Kopfende getönt, -Y schlicht
    window = ((WINDOW_X[0], WINDOW_X[1]), (WINDOW_Z[0], WINDOW_Z[1]))
    door = ((DOOR_Y[0], DOOR_Y[1]), (FLOOR, FLOOR + 2130.0))
    parts += wall_with_openings("WallWindow", "y", y1 + 90.0, (x0 - 180.0, x1 + 180.0), height, [window], "Wall")
    parts += wall_with_openings("WallDoor", "x", x1 + 90.0, (y0, y1), height, [door], "Wall")
    parts += wall_with_openings("WallHead", "x", x0 - 90.0, (y0, y1), height, [], "WallAccent")
    parts += wall_with_openings("WallSide", "y", y0 - 90.0, (x0 - 180.0, x1 + 180.0), height, [], "Wall")

    # Hohlkehle: der PVC-Boden zieht 100 mm die Wand hoch, damit sich in keiner Ecke Schmutz hält
    for (center, size) in (
        ((0.5 * (x0 + x1), y1 - 6.0, FLOOR + 50.0), (x1 - x0, 12.0, 100.0)),
        ((0.5 * (x0 + x1), y0 + 6.0, FLOOR + 50.0), (x1 - x0, 12.0, 100.0)),
        ((x0 + 6.0, 0.0, FLOOR + 50.0), (12.0, y1 - y0, 100.0)),
    ):
        parts.append(box("Cove", center, size, "Floor", bevel=5.0))
    for (low, high) in ((y0, DOOR_Y[0]), (DOOR_Y[1], y1)):
        parts.append(box("Cove", (x1 - 6.0, 0.5 * (low + high), FLOOR + 50.0), (12.0, high - low, 100.0), "Floor", bevel=5.0))

    # Decke: Rasterdecke 625 mm, Tragprofile 24 mm breit, Platten leicht zurückliegend
    parts.append(box("CeilingSlab", (0.5 * (x0 + x1), 0.5 * (y0 + y1), CEILING + 25.0), (x1 - x0 + 400.0, y1 - y0 + 400.0, 30.0), "CeilingTile", bevel=0.0))
    grid_x = np.arange(x0, x1 + 1.0, 625.0)
    grid_y = np.arange(y0, y1 + 1.0, 625.0)
    for gx in grid_x:
        parts.append(box("GridX", (gx, 0.5 * (y0 + y1), CEILING + 4.0), (24.0, y1 - y0, 8.0), "CeilingGrid", bevel=1.0))
    for gy in grid_y:
        parts.append(box("GridY", (0.5 * (x0 + x1), gy, CEILING + 4.0), (x1 - x0, 24.0, 8.0), "CeilingGrid", bevel=1.0))

    # LED-Felder 600 × 600 in vier Rasterfeldern (Positionen stehen auch im Unreal-Skript: LED_PANELS)
    for (cx, cy) in LED_PANELS:
        parts.append(box("LedFrame", (cx, cy, CEILING + 2.0), (612.0, 612.0, 14.0), "CeilingGrid", bevel=2.0))
        parts.append(box("LedDiffuser", (cx, cy, CEILING - 5.5), (590.0, 590.0, 2.0), "LedPanel", bevel=0.5))

    # Fenster: Blendrahmen, Flügelrahmen, zwei Scheiben mit 16 mm Zwischenraum, Fensterbank innen
    wx0, wx1 = WINDOW_X
    wz0, wz1 = WINDOW_Z
    frame_y = y1 + 20.0
    for (center, size) in (
        ((0.5 * (wx0 + wx1), frame_y, wz0 + 40.0), (wx1 - wx0, 80.0, 80.0)),
        ((0.5 * (wx0 + wx1), frame_y, wz1 - 40.0), (wx1 - wx0, 80.0, 80.0)),
        ((wx0 + 40.0, frame_y, 0.5 * (wz0 + wz1)), (80.0, 80.0, wz1 - wz0)),
        ((wx1 - 40.0, frame_y, 0.5 * (wz0 + wz1)), (80.0, 80.0, wz1 - wz0)),
        ((0.5 * (wx0 + wx1), frame_y, 0.5 * (wz0 + wz1)), (70.0, 80.0, wz1 - wz0 - 160.0)),   # Mittelpfosten
    ):
        parts.append(box("WindowFrame", center, size, "WindowFrame", bevel=3.0))
    for pane_y in (frame_y - 14.0, frame_y + 14.0):
        parts.append(box("WindowGlass", (0.5 * (wx0 + wx1), pane_y, 0.5 * (wz0 + wz1)), (wx1 - wx0 - 150.0, 4.0, wz1 - wz0 - 150.0), "Glass", bevel=0.0))
    parts.append(box("Sill", (0.5 * (wx0 + wx1), y1 - 70.0, wz0 - 12.0), (wx1 - wx0 + 80.0, 200.0, 24.0), "WindowSill", bevel=3.0))
    # Himmel vor dem Fenster: eine leuchtende Fläche (bedeckter Nachmittagshimmel), weit genug weg für die Unschärfe
    parts.append(box("Sky", (0.5 * (wx0 + wx1), y1 + 1400.0, 0.5 * (wz0 + wz1) + 200.0), (6000.0, 20.0, 4000.0), "Sky", bevel=0.0))

    # Lamellen-Jalousie innen, halb geschlossen (45°): Sie macht aus dem Fensterlicht die Streifen an Wand und Bett
    slat_pitch = 22.0
    blind_top = wz1 - 60.0
    blind_bottom = wz0 + 0.45 * (wz1 - wz0)
    tilt = Matrix.Rotation(math.radians(45.0), 3, "X")
    for index, sz in enumerate(np.arange(blind_top, blind_bottom, -slat_pitch)):
        parts.append(box("Slat", (0.5 * (wx0 + wx1), y1 - 60.0, sz), (wx1 - wx0 - 40.0, 25.0, 0.4), "BlindSlat", bevel=0.0, rotation=tilt))
    parts.append(box("BlindRail", (0.5 * (wx0 + wx1), y1 - 60.0, blind_top + 25.0), (wx1 - wx0 - 20.0, 40.0, 36.0), "BlindSlat", bevel=2.0))
    parts.append(box("BlindBottom", (0.5 * (wx0 + wx1), y1 - 60.0, blind_bottom - 8.0), (wx1 - wx0 - 40.0, 30.0, 10.0), "BlindSlat", bevel=2.0))
    for cord_x in (wx0 + 150.0, wx1 - 150.0):
        cylinder("BlindCord", (cord_x, y1 - 48.0, blind_bottom), (cord_x, y1 - 48.0, blind_top), 1.0, "BlindCord", segments=6, cap_bevel=0.0)

    # Tür: zwei Flügel (Bettentür), Zarge, Sichtfenster, Drücker – geschlossen
    for (low, high, leaf) in ((DOOR_Y[0], -630.0, "DoorLeafA"), (-630.0, DOOR_Y[1], "DoorLeafB")):
        parts.append(box(leaf, (x1 + 60.0, 0.5 * (low + high) , FLOOR + 1062.0), (42.0, high - low - 6.0, 2120.0), "Door", bevel=3.0))
    parts.append(box("DoorVision", (x1 + 36.0, -1060.0, FLOOR + 1450.0), (6.0, 200.0, 600.0), "Glass", bevel=0.0))
    for (center, size) in (
        ((x1 + 20.0, DOOR_Y[0] - 25.0, FLOOR + 1080.0), (60.0, 50.0, 2160.0)),
        ((x1 + 20.0, DOOR_Y[1] + 25.0, FLOOR + 1080.0), (60.0, 50.0, 2160.0)),
        ((x1 + 20.0, 0.5 * (DOOR_Y[0] + DOOR_Y[1]), FLOOR + 2155.0), (60.0, DOOR_Y[1] - DOOR_Y[0] + 100.0, 50.0)),
    ):
        parts.append(box("DoorFrame", center, size, "WindowFrame", bevel=3.0))
    cylinder("DoorHandle", (x1 + 36.0, -680.0, FLOOR + 1050.0), (x1 + 36.0 - 0.1, -800.0, FLOOR + 1050.0), 10.0, "Steel", segments=16)

    room = join("SM_GEN_DeliveryRoom", [obj for obj in bpy.context.scene.objects if obj.type == "MESH"])
    return room


LED_PANELS = [(-162.5, -937.5), (-162.5, 937.5), (1087.5, -937.5), (1087.5, 937.5)]


# ---------------------------------------------------------------------------------------------------------------------
# Entbindungsbett
# ---------------------------------------------------------------------------------------------------------------------
HINGE = Vector((-450.0, 0.0, -40.0))               # Gelenk zwischen Sitz und Rückenteil (Oberkante Matratze)
BACK_ANGLE = math.radians(45.0)
BACK_DIR = Vector((-math.cos(BACK_ANGLE), 0.0, math.sin(BACK_ANGLE)))   # das Rückenteil hinauf
BACK_NORMAL = Vector((math.sin(BACK_ANGLE), 0.0, math.cos(BACK_ANGLE)))


def build_bed():
    before = set(bpy.context.scene.objects)
    # Sitzteil (Matratze) und Sockel
    box("SeatMattress", (-225.0, 0.0, -100.0), (470.0, 900.0, 120.0), "Mattress", bevel=35.0)
    box("SeatBase", (-225.0, 0.0, -185.0), (500.0, 940.0, 50.0), "BedPlastic", bevel=8.0)
    # Rückenteil 45°: Matratze auf einer Platte, um das Gelenk gedreht
    # Drehung um Y so, dass die lange Kante der Platte entlang BACK_DIR liegt
    back_rot = Matrix.Rotation(BACK_ANGLE, 3, "Y")
    back_center = HINGE + BACK_DIR * 420.0 - BACK_NORMAL * 60.0
    box("BackMattress", tuple(back_center), (840.0, 900.0, 120.0), "Mattress", bevel=35.0, rotation=back_rot)
    plate_center = HINGE + BACK_DIR * 430.0 - BACK_NORMAL * 145.0
    box("BackPlate", tuple(plate_center), (880.0, 940.0, 40.0), "BedPlastic", bevel=8.0, rotation=back_rot)
    # Fußteil abgesenkt (Gebärstellung): 300 mm tiefer, kürzer
    box("FootMattress", (230.0, 0.0, -330.0), (420.0, 820.0, 90.0), "Mattress", bevel=30.0)
    box("FootBase", (230.0, 0.0, -400.0), (440.0, 860.0, 40.0), "BedPlastic", bevel=8.0)
    # Hubsäule und Fahrgestell mit Verkleidung, vier Doppelrollen
    box("Column", (-300.0, 0.0, -500.0), (420.0, 520.0, 560.0), "BedPlastic", bevel=25.0)
    box("Chassis", (-250.0, 0.0, FLOOR + 150.0), (1500.0, 780.0, 90.0), "BedPlastic", bevel=15.0)
    for cx in (-950.0, 450.0):
        for cy in (-330.0, 330.0):
            cylinder("Castor", (cx, cy - 25.0, FLOOR + 62.0), (cx, cy + 25.0, FLOOR + 62.0), 62.0, "Rubber", segments=32)
            box("CastorFork", (cx, cy, FLOOR + 110.0), (70.0, 70.0, 60.0), "Steel", bevel=6.0)
    # Seitengitter am Rückenteil (geklappt), Haltegriffe am Sitz
    for side in (-1.0, 1.0):
        # heruntergeklappt: Bei einer Geburt sind die Seitengitter unten, sie lägen sonst zwischen Mutter und Hebamme
        rail_center = HINGE + BACK_DIR * 450.0 + Vector((0.0, side * 490.0, 0.0)) - BACK_NORMAL * 170.0
        box("SideRail", tuple(rail_center), (620.0, 26.0, 180.0), "BedPlastic", bevel=10.0, rotation=back_rot)
        cylinder("GripBar", (-380.0, side * 470.0, -30.0), (-80.0, side * 470.0, -30.0), 14.0, "Steel", segments=24)
        cylinder("GripPost", (-380.0, side * 470.0, -30.0), (-380.0, side * 470.0, -150.0), 12.0, "Steel", segments=24)
        cylinder("GripPost", (-80.0, side * 470.0, -30.0), (-80.0, side * 470.0, -150.0), 12.0, "Steel", segments=24)
    # Kopfteil
    head_center = HINGE + BACK_DIR * 880.0 - BACK_NORMAL * 70.0
    box("Headboard", tuple(head_center), (40.0, 960.0, 380.0), "BedPlastic", bevel=12.0, rotation=back_rot)
    objects = [obj for obj in bpy.context.scene.objects if obj not in before and obj.type == "MESH"]
    return join("SM_GEN_BirthBed", objects)


# ---------------------------------------------------------------------------------------------------------------------
# Die Mutter: Körper als Stütze für die Tücher, sichtbar nur, wo Haut frei ist
# ---------------------------------------------------------------------------------------------------------------------
def body_point(along, lateral=0.0, lift=0.0):
    """Punkt auf dem Rückenteil: along = mm das Rückenteil hinauf, lift = mm über der Matratze."""
    return HINGE + BACK_DIR * along + BACK_NORMAL * lift + Vector((0.0, lateral, 0.0))


def build_body_proxy(scale):
    """
    Der Körper einer Frau (165 cm) halb sitzend im Bett.

    Er dient zweierlei: Unter den Tüchern ist er der Körper, über den der Stoff fällt; oberhalb der
    Decke ist er die Haut von Brust und Schultern, auf der das Kind liegt. Kopf und Gesicht gehören
    nicht hierher – dafür ist eine MetaHuman-Figur vorgesehen (siehe Docs/24).

    Gebaut über ein Skelett mit festen Gliedradien (Skin-Modifier), nicht über Metaballs: Bei
    Metaballs liegt die Oberfläche irgendwo innerhalb des Einflussradius, und der erste Versuch
    ergab eine dürre Gliederpuppe. Hier ist jeder Radius der tatsächliche Radius des Körpers.
    Maße: Rumpfumfang ~95 cm (Radius 150 mm), Oberschenkel 58 cm (92 mm), Wade 36 cm (58 mm),
    Oberarm 30 cm (48 mm), Unterarm 24 cm (38 mm) – Werte einer Frau kurz nach der Geburt.
    """
    joints = {
        "pelvis": (Vector((-190.0, 0.0, 75.0)), 165.0),
        "waist": (body_point(170.0, 0.0, 120.0), 150.0),
        "chest": (body_point(380.0, 0.0, 125.0), 158.0),
        "neck_base": (body_point(560.0, 0.0, 110.0), 70.0),
        "neck_top": (body_point(650.0, 0.0, 100.0), 52.0),
    }
    for side, tag in ((-1.0, "l"), (1.0, "r")):
        joints["shoulder_" + tag] = (body_point(520.0, side * 180.0, 115.0), 58.0)
        joints["elbow_" + tag] = (body_point(250.0, side * 262.0, 125.0), 42.0)
        joints["wrist_" + tag] = (Vector((-150.0, side * 235.0, 185.0)), 30.0)
        joints["hand_" + tag] = (Vector((-60.0, side * 205.0, 195.0)), 26.0)
        joints["hip_" + tag] = (Vector((-150.0, side * 105.0, 55.0)), 96.0)
        joints["knee_" + tag] = (Vector((300.0, side * 300.0, 245.0)), 62.0)
        joints["ankle_" + tag] = (Vector((470.0, side * 355.0, -225.0)), 38.0)
        joints["toe_" + tag] = (Vector((590.0, side * 370.0, -262.0)), 34.0)
    bones = [("pelvis", "waist"), ("waist", "chest"), ("chest", "neck_base"), ("neck_base", "neck_top")]
    for tag in ("l", "r"):
        bones += [("chest", "shoulder_" + tag), ("shoulder_" + tag, "elbow_" + tag), ("elbow_" + tag, "wrist_" + tag),
                  ("wrist_" + tag, "hand_" + tag), ("pelvis", "hip_" + tag), ("hip_" + tag, "knee_" + tag),
                  ("knee_" + tag, "ankle_" + tag), ("ankle_" + tag, "toe_" + tag)]

    names = list(joints)
    mesh = bpy.data.meshes.new("MotherSkeleton")
    mesh.from_pydata([tuple(joints[name][0] * scale) for name in names],
                     [(names.index(a), names.index(b)) for a, b in bones], [])
    skeleton = bpy.data.objects.new("MotherSkeleton", mesh)
    bpy.context.scene.collection.objects.link(skeleton)
    skin = skeleton.modifiers.new("Skin", "SKIN")
    skin.use_smooth_shade = True
    for index, name in enumerate(names):
        radius = joints[name][1] * scale
        mesh.skin_vertices[""].data[index].radius = (radius, radius)
    mesh.skin_vertices[""].data[names.index("pelvis")].use_root = True
    subsurf = skeleton.modifiers.new("Subsurf", "SUBSURF")
    subsurf.levels = 3

    # Brüste (stillend) als eigene Formen, danach mit dem Rumpf zu einer Oberfläche verschmolzen
    parts = [skeleton]
    for side in (-1.0, 1.0):
        center = body_point(355.0, side * 82.0, 250.0) + Vector((0.0, side * 10.0, -22.0))
        breast = sphere("Breast", tuple(center / 1.0), 78.0, "Skin", squash=(0.95, 0.92, 0.85), subdivisions=4)
        parts.append(breast)

    depsgraph = bpy.context.evaluated_depsgraph_get()
    body_mesh = bpy.data.meshes.new_from_object(skeleton.evaluated_get(depsgraph))
    body = bpy.data.objects.new("MotherBodyRaw", body_mesh)
    bpy.context.scene.collection.objects.link(body)
    bpy.data.objects.remove(skeleton)
    merged = join("MotherBodyMerged", [body] + parts[1:])

    # Ein Körper, keine Teile: Voxel-Neuvernetzung (4 mm) und Glätten verbinden Rumpf und Brust weich
    remesh = merged.modifiers.new("Remesh", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = 4.0 * scale
    smooth = merged.modifiers.new("Smooth", "CORRECTIVE_SMOOTH")
    smooth.iterations = 8
    smooth.use_only_smooth = True
    depsgraph = bpy.context.evaluated_depsgraph_get()
    final_mesh = bpy.data.meshes.new_from_object(merged.evaluated_get(depsgraph))
    bpy.data.objects.remove(merged)
    result = bpy.data.objects.new("MotherBody", final_mesh)
    bpy.context.scene.collection.objects.link(result)
    result.data.materials.clear()
    for polygon in result.data.polygons:
        polygon.use_smooth = True
    return result


# ---------------------------------------------------------------------------------------------------------------------
# Tücher: echte Stoffsimulation in echtem Maßstab (Meter), danach auf den Szenenmaßstab gebracht
# ---------------------------------------------------------------------------------------------------------------------
REAL = 0.001    # 1 mm in Metern – der Stoff fällt nur in echtem Maßstab so, wie Stoff fällt


def cloth_sheet(name, center, size, spacing, material, mass, stiffness, bending, frames, thickness, lift_edges=None):
    columns = max(2, int(size[0] / spacing))
    rows = max(2, int(size[1] / spacing))
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=columns, y_segments=rows, size=0.5)
    bmesh.ops.scale(bm, vec=Vector((size[0], size[1], 1.0)), verts=bm.verts)
    # Ein Hauch Unregelmäßigkeit: Ein Tuch, das exakt eben fällt, legt sich in symmetrische Falten
    for vertex in bm.verts:
        vertex.co.z += float(rng.normal(0.0, 1.5))
    bmesh.ops.translate(bm, vec=Vector(center), verts=bm.verts)
    bmesh.ops.scale(bm, vec=Vector((REAL, REAL, REAL)), verts=bm.verts)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)

    cloth = obj.modifiers.new("Cloth", "CLOTH")
    settings = cloth.settings
    settings.quality = 10
    settings.mass = mass
    settings.tension_stiffness = stiffness
    settings.compression_stiffness = stiffness
    settings.shear_stiffness = stiffness
    settings.bending_stiffness = bending
    settings.air_damping = 1.0
    collision = cloth.collision_settings
    collision.distance_min = 0.003
    collision.use_self_collision = True
    collision.self_distance_min = 0.002
    collision.collision_quality = 4
    cloth.point_cache.frame_start = 1
    cloth.point_cache.frame_end = frames
    return obj


def simulate_cloth(cloths, colliders, frames):
    for collider in colliders:
        modifier = collider.modifiers.new("Collision", "COLLISION")
        modifier.settings.thickness_outer = 0.004
        modifier.settings.cloth_friction = 8.0
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 1, frames
    for frame in range(1, frames + 1):
        scene.frame_set(frame)
    depsgraph = bpy.context.evaluated_depsgraph_get()
    results = []
    for cloth in cloths:
        mesh = bpy.data.meshes.new_from_object(cloth.evaluated_get(depsgraph))
        result = bpy.data.objects.new(cloth.name + "_Sim", mesh)
        result.data.materials.append(bpy.data.materials.get(cloth.name + "_mat") or bpy.data.materials.new(cloth.name))
        scene.collection.objects.link(result)
        results.append(result)
    scene.frame_set(1)
    return results


def real_copy(obj):
    """Kopie eines Objekts im Meter-Maßstab als Kollisionskörper für die Stoffsimulation."""
    mesh = obj.data.copy()
    mesh.transform(Matrix.Scale(REAL / UNIT, 4))
    copy = bpy.data.objects.new(obj.name + "_Collider", mesh)
    bpy.context.scene.collection.objects.link(copy)
    return copy


def finish_cloth(obj, material, thickness_mm):
    """Auf den Szenenmaßstab, Materialplatz setzen, etwas Stärke und eine Glättungsstufe."""
    obj.data.transform(Matrix.Scale(UNIT / REAL, 4))
    obj.data.materials.clear()
    obj.data.materials.append(bpy.data.materials.get(material) or bpy.data.materials.new(material))
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    solidify = obj.modifiers.new("Thickness", "SOLIDIFY")
    solidify.thickness = thickness_mm * UNIT
    solidify.offset = 1.0
    subsurf = obj.modifiers.new("Smooth", "SUBSURF")
    subsurf.levels = 1
    subsurf.render_levels = 1
    return obj


def build_textiles(bed, body):
    colliders = [real_copy(bed), real_copy(body)]
    # Der Boden fängt auf, was vom Bett rutscht – ohne ihn fiele ein Tuch ins Bodenlose
    floor_mesh = bpy.data.meshes.new("FloorCollider")
    floor_mesh.from_pydata([(-3.0, -3.0, FLOOR * REAL), (3.0, -3.0, FLOOR * REAL), (3.0, 3.0, FLOOR * REAL), (-3.0, 3.0, FLOOR * REAL)], [], [(0, 1, 2, 3)])
    floor_collider = bpy.data.objects.new("FloorCollider", floor_mesh)
    bpy.context.scene.collection.objects.link(floor_collider)
    colliders.append(floor_collider)
    frames = 70
    # Keine Tücher über den Knien: Ein Laken über beiden Knien hängt als Zelt zwischen den Beinen und
    # verdeckt genau den Blick, mit dem das Kind die Welt zuerst sieht; ein Tuch je Knie rutscht vom
    # gebeugten Knie (in der Simulation wie in echt – gehalten wird es dort nur von einer Hand).
    # Warme Decke über Bauch und Hüfte, bis unter die Brust (dort liegt das Kind auf nackter Haut)
    blanket = cloth_sheet("Blanket", (-260.0, 0.0, 360.0), (520.0, 1150.0), 14.0, "Blanket",
                          mass=0.80, stiffness=25.0, bending=4.0, frames=frames, thickness=4.0)
    # Die blaue Einmalunterlage unter dem Becken – in jedem Kreißsaal liegt eine
    underpad = cloth_sheet("Underpad", (150.0, 0.0, -25.0), (400.0, 600.0), 16.0, "Underpad",
                           mass=0.20, stiffness=40.0, bending=2.0, frames=frames, thickness=1.0)
    simulated = simulate_cloth([blanket, underpad], colliders, frames)
    for obj in [blanket, underpad] + colliders:
        bpy.data.objects.remove(obj)
    materials = {"KneeSheetL_Sim": ("Sheet", 0.6), "KneeSheetR_Sim": ("Sheet", 0.6), "Blanket_Sim": ("Blanket", 4.0), "Underpad_Sim": ("Underpad", 1.0)}
    finished = [finish_cloth(obj, *materials[obj.name]) for obj in simulated]
    return join("SM_GEN_BirthTextiles", finished)


# ---------------------------------------------------------------------------------------------------------------------
# Ausstattung
# ---------------------------------------------------------------------------------------------------------------------
def build_equipment():
    before = set(bpy.context.scene.objects)
    x0, x1 = ROOM_X
    y0, y1 = ROOM_Y

    # Wärmebett (Reanimationseinheit) an der Seitenwand, schräg zum Fußende: Das Kind wird hier gewogen
    wx, wy = 1300.0, y0 + 450.0
    box("WarmerCart", (wx, wy, FLOOR + 450.0), (700.0, 560.0, 700.0), "DeviceWhite", bevel=20.0)
    box("WarmerTray", (wx, wy, FLOOR + 900.0), (760.0, 600.0, 180.0), "DeviceWhite", bevel=25.0)
    box("WarmerMattress", (wx, wy, FLOOR + 975.0), (660.0, 480.0, 40.0), "MattressInfant", bevel=12.0)
    cylinder("WarmerColumn", (wx - 330.0, wy - 200.0, FLOOR + 900.0), (wx - 330.0, wy - 200.0, FLOOR + 1900.0), 35.0, "DeviceWhite")
    box("WarmerHead", (wx - 60.0, wy - 200.0, FLOOR + 1880.0), (620.0, 260.0, 90.0), "DeviceWhite", bevel=20.0)
    box("WarmerHeater", (wx - 60.0, wy - 200.0, FLOOR + 1830.0), (520.0, 180.0, 6.0), "HeaterGlow", bevel=1.0)
    box("WarmerDisplay", (wx - 330.0, wy - 150.0, FLOOR + 1400.0), (60.0, 220.0, 160.0), "DeviceGrey", bevel=10.0)
    box("WarmerScreen", (wx - 298.0, wy - 150.0, FLOOR + 1400.0), (2.0, 180.0, 120.0), "ScreenAmber", bevel=0.0)
    # ein gefaltetes Handtuch für das Kind liegt bereit
    box("TowelFolded", (wx + 120.0, wy + 60.0, FLOOR + 1010.0), (300.0, 220.0, 35.0), "Towel", bevel=12.0)

    # CTG auf fahrbarem Wagen neben dem Kopfende
    cx, cy = -700.0, -780.0
    box("CtgCartTop", (cx, cy, FLOOR + 860.0), (480.0, 420.0, 30.0), "DeviceGrey", bevel=8.0)
    cylinder("CtgCartPost", (cx, cy, FLOOR + 90.0), (cx, cy, FLOOR + 845.0), 22.0, "Steel")
    box("CtgCartFoot", (cx, cy, FLOOR + 60.0), (520.0, 460.0, 30.0), "DeviceGrey", bevel=8.0)
    box("CtgMonitor", (cx, cy, FLOOR + 1010.0), (300.0, 360.0, 270.0), "DeviceWhite", bevel=18.0)
    box("CtgScreen", (cx + 151.0, cy, FLOOR + 1040.0), (2.0, 290.0, 170.0), "ScreenGreen", bevel=0.0)
    box("CtgPaper", (cx + 120.0, cy, FLOOR + 900.0), (160.0, 200.0, 4.0), "Paper", bevel=0.0)

    # Infusionsständer mit Beutel
    ix, iy = -150.0, -640.0
    cylinder("IvPole", (ix, iy, FLOOR + 80.0), (ix, iy, FLOOR + 2000.0), 12.0, "Steel")
    for angle in range(5):
        a = angle * 2.0 * math.pi / 5.0
        cylinder("IvLeg", (ix, iy, FLOOR + 80.0), (ix + 300.0 * math.cos(a), iy + 300.0 * math.sin(a), FLOOR + 40.0), 10.0, "Steel", segments=12)
    cylinder("IvHook", (ix - 90.0, iy, FLOOR + 1990.0), (ix + 90.0, iy, FLOOR + 1990.0), 6.0, "Steel", segments=12)
    sphere("IvBag", (ix - 80.0, iy, FLOOR + 1840.0), 70.0, "IvBag", squash=(0.45, 1.0, 1.6))
    cylinder("IvLine", (ix - 80.0, iy, FLOOR + 1720.0), (-300.0, -400.0, 150.0), 2.0, "IvBag", segments=6, cap_bevel=0.0)

    # Wanduhr über der Tür – die Geburtszeit wird von ihr abgelesen
    cylinder("Clock", (x1 - 20.0, 250.0, FLOOR + 2350.0), (x1 - 60.0, 250.0, FLOOR + 2350.0), 150.0, "ClockFace", segments=64)
    cylinder("ClockRim", (x1 - 18.0, 250.0, FLOOR + 2350.0), (x1 - 66.0, 250.0, FLOOR + 2350.0), 158.0, "DeviceGrey", segments=64)

    # Geburtsseil: ein fest verknotetes Tuch von einem Deckenhaken über den Knien der Frau – dort, wo sie es
    # halb sitzend greifen und sich daran hochziehen kann. Nicht über dem Fußende: Dort steht die Hebamme,
    # und ein Tuch vor ihrem Gesicht hing genau im ersten Bild, das das Kind von ihr sieht.
    rx, ry = 150.0, 0.0
    cylinder("RopeHook", (rx, ry, CEILING), (rx, ry, CEILING - 60.0), 15.0, "Steel", segments=16)
    top = Vector((rx, ry, CEILING - 60.0))
    points = [top + Vector((20.0 * math.sin(i * 0.9), 15.0 * math.cos(i * 0.7), -i * 130.0)) for i in range(11)]
    for a, b in zip(points[:-1], points[1:]):
        cylinder("RopeCloth", a, b, 28.0, "RopeCloth", segments=12, cap_bevel=0.0)
    sphere("RopeKnot", points[-1], 55.0, "RopeCloth", squash=(1.0, 0.8, 0.9))

    # Gebärball und Hocker der Hebamme
    # Unterteilung 6 (10 242 Punkte): Bei 65 cm Durchmesser wären es mit der Vorgabe 7 cm große Facetten
    sphere("BirthBall", (1900.0, 1100.0, FLOOR + 325.0), 325.0, "BallVinyl", squash=(1.0, 1.0, 0.97), subdivisions=6)
    cylinder("StoolSeat", (820.0, 380.0, FLOOR + 560.0), (820.0, 380.0, FLOOR + 620.0), 200.0, "StoolVinyl", segments=128, cap_bevel=12.0)
    cylinder("StoolPost", (820.0, 380.0, FLOOR + 120.0), (820.0, 380.0, FLOOR + 560.0), 25.0, "Steel")
    for angle in range(5):
        a = angle * 2.0 * math.pi / 5.0 + 0.3
        cylinder("StoolLeg", (820.0, 380.0, FLOOR + 110.0), (820.0 + 280.0 * math.cos(a), 380.0 + 280.0 * math.sin(a), FLOOR + 60.0), 14.0, "DeviceGrey", segments=12)

    # Schrankzeile und Waschbecken an der Stirnwand
    for index in range(4):
        sx = x1 - 330.0
        sy = y1 - 400.0 - index * 610.0
        box("CabinetLow", (sx, sy, FLOOR + 450.0), (600.0, 600.0, 900.0), "Cabinet", bevel=4.0)
        box("CabinetHigh", (sx + 150.0, sy, FLOOR + 1900.0), (300.0, 600.0, 700.0), "Cabinet", bevel=4.0)
    box("Worktop", (x1 - 330.0, y1 - 1315.0, FLOOR + 915.0), (640.0, 2440.0, 30.0), "Worktop", bevel=4.0)
    box("Sink", (x1 - 330.0, y1 - 800.0, FLOOR + 905.0), (420.0, 500.0, 40.0), "Steel", bevel=10.0)
    cylinder("Tap", (x1 - 560.0, y1 - 800.0, FLOOR + 930.0), (x1 - 560.0, y1 - 800.0, FLOOR + 1220.0), 14.0, "Steel")
    box("Dispenser", (x1 - 12.0, y1 - 500.0, FLOOR + 1300.0), (110.0, 90.0, 260.0), "DeviceWhite", bevel=12.0)

    # Untersuchungsleuchte an einem Deckenarm – nach der Geburt aus, aber sichtbar
    cylinder("LampMount", (400.0, -300.0, CEILING), (400.0, -300.0, CEILING - 400.0), 35.0, "DeviceWhite")
    cylinder("LampArm", (400.0, -300.0, CEILING - 400.0), (650.0, 0.0, CEILING - 700.0), 28.0, "DeviceWhite")
    sphere("LampHead", (680.0, 20.0, CEILING - 760.0), 280.0, "DeviceWhite", squash=(1.0, 1.0, 0.32))
    # Warme Wandleuchte über dem Kopfende (dimmbar, die einzige warme Quelle im Raum)
    box("WallLamp", (x0 + 40.0, 0.0, FLOOR + 2000.0), (80.0, 700.0, 90.0), "DeviceWhite", bevel=15.0)
    box("WallLampDiffuser", (x0 + 82.0, 0.0, FLOOR + 1970.0), (4.0, 660.0, 50.0), "LampWarm", bevel=0.0)

    objects = [obj for obj in bpy.context.scene.objects if obj not in before and obj.type == "MESH"]
    return join("SM_GEN_DeliveryEquipment", objects)


def export_fbx(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE", use_tspace=False,
                             add_leaf_bones=False)
    print("GENESIS: exportiert %s %.1f MB" % (os.path.basename(path), os.path.getsize(path) / (1024 * 1024)))


def report(obj):
    bounds = np.array([obj.matrix_world @ Vector(corner) for corner in obj.bound_box]) / UNIT
    slots = [slot.material.name for slot in obj.material_slots if slot.material]
    print("GENESIS: %-26s %6.0f × %6.0f × %6.0f mm  %7d Flächen  Materialien: %s" % (
        obj.name, np.ptp(bounds[:, 0]), np.ptp(bounds[:, 1]), np.ptp(bounds[:, 2]), len(obj.data.polygons), ", ".join(slots)))


def main():
    args = script_args()
    out_dir = os.path.abspath(args.get("out", "ArtSource/Generated/Birth"))
    reset_scene()

    room = build_room()
    report(room)
    bed = build_bed()
    report(bed)
    body = build_body_proxy(UNIT)
    textiles = build_textiles(bed, body)
    report(textiles)
    # Der sichtbare Körper: nur die Haut, die über der Decke liegt – Brust, Schultern, Arme
    body.data.materials.append(bpy.data.materials.get("Skin") or bpy.data.materials.new("Skin"))
    body.name = "SM_GEN_MotherBody"
    report(body)
    equipment = build_equipment()
    report(equipment)

    if "export" in args:
        ensure_dir(out_dir)
        for obj in (room, bed, textiles, body, equipment):
            export_fbx(obj, os.path.join(out_dir, obj.name + ".fbx"))


main()


def preview(out_dir):
    """Schnelle Formkontrolle (Workbench): Übersicht von der Tür, Blick aus dem Kanal, Blick von der Brust."""
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_shadows = True
    scene.display.shading.show_cavity = True
    scene.render.resolution_x, scene.render.resolution_y = 1280, 720
    palette = {"Skin": (0.62, 0.42, 0.34), "Sheet": (0.85, 0.85, 0.83), "Blanket": (0.75, 0.70, 0.62),
               "Underpad": (0.25, 0.45, 0.70), "Mattress": (0.22, 0.38, 0.42), "WallAccent": (0.48, 0.55, 0.47),
               "Sky": (0.9, 0.93, 1.0), "Glass": (0.6, 0.7, 0.75), "Floor": (0.52, 0.5, 0.47)}
    for material in bpy.data.materials:
        material.diffuse_color = palette.get(material.name, (0.7, 0.7, 0.7)) + (1.0,)
    views = {
        "overview": ((2500.0, 1500.0, 1500.0), (-300.0, -200.0, -200.0), 18.0),
        "exit": ((160.0, 0.0, 0.0), (1600.0, 0.0, 0.0), 16.0),
        "chest": ((-540.0, 60.0, 440.0), (-540.0, 2300.0, 700.0), 16.0),
    }
    for name, (location, target, lens) in views.items():
        data = bpy.data.cameras.new(name)
        data.lens = lens
        data.clip_start = 5.0 * UNIT
        cam = bpy.data.objects.new(name, data)
        scene.collection.objects.link(cam)
        cam.location = mm(location)
        cam.rotation_euler = (mm(target) - mm(location)).to_track_quat("-Z", "Y").to_euler()
        scene.camera = cam
        scene.render.filepath = os.path.join(out_dir, "Preview_%s.png" % name)
        bpy.ops.render.render(write_still=True)
        print("GENESIS: Vorschau", scene.render.filepath)


if "--preview" in sys.argv:
    preview(os.path.abspath(script_args().get("out", "ArtSource/Generated/Birth")))
