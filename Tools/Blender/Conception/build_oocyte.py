# GENESIS: Der Kreislauf des Lebens
# Baut die reife menschliche Eizelle mit Hülle und Corona radiata (SM_GEN_Oocyte*) aus realen Maßen.
#
# Aufruf:
#   blender -b --factory-startup -P Tools/Blender/Conception/build_oocyte.py -- --out ArtSource/Generated/Conception [--export] [--render]
#
# Anatomie (reife Eizelle, Metaphase II):
#   - Ooplasma (Zellleib) ~110 µm Durchmesser, körnig (Organellen), unter der Membran Cortikalgranula
#   - Perivitelliner Spalt 1–5 µm mit dem ersten Polkörper (~10 µm)
#   - Zona pellucida 17 µm dick (16,7–17,7 µm, Valeri 2011), gallertig, aus Glykoproteinfasern – daran binden die Spermien
#   - Corona radiata: 2–3 Lagen radial gestreckter Cumuluszellen (10–20 µm) in einer Hyaluronsäure-Matrix,
#     deren Fortsätze zur Eizelle beim Eisprung schon zurückgezogen sind (GENESIS-047 Teil 2c)
#   - Der gesamte Cumulus-Oozyten-Komplex misst mehrere hundert µm
#
# Maßstab: 1 µm = 0,01 m in Blender = 1 Unreal-Einheit. Ursprung = Mittelpunkt der Eizelle.

import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from genesis_blender_common import (UM, configure_color_management, configure_cycles, enable_gpu, ensure_dir, measure_exposure, new_node,
                                    reset_scene, script_args)
import bmesh
import bpy
import numpy as np
from mathutils import Vector

OOPLASM_RADIUS = 55.0
PERIVITELLINE = 3.0
ZONA_INNER = OOPLASM_RADIUS + PERIVITELLINE
ZONA_THICKNESS = 17.0  # 16,7–17,7 µm (Valeri 2011, Docs/38)
POLAR_BODY_RADIUS = 5.0
# Transzonale Fortsätze (Coronazellen → Eizelle): zum Eisprung zurückgezogen, siehe build_corona
WITH_TRANSZONAL_PROJECTIONS = False
CORONA_LAYERS = 3

rng = np.random.default_rng(190619)


def value_noise(points, frequency, seed, octaves=3):
    """Deterministische, glatte Summe aus Sinuswellen – für organische Verformungen."""
    generator = np.random.default_rng(seed)
    total = np.zeros(len(points))
    amplitude, norm = 1.0, 0.0
    for octave in range(octaves):
        scale = frequency * (2.0 ** octave)
        directions = generator.normal(size=(6, 3))
        directions /= np.linalg.norm(directions, axis=1, keepdims=True)
        phases = generator.uniform(0.0, 2.0 * math.pi, 6)
        layer = sum(np.sin(points @ direction * scale + phase) for direction, phase in zip(directions, phases)) / 6.0
        total += layer * amplitude
        norm += amplitude
        amplitude *= 0.5
    return total / norm


def sphere(name, radius, subdivisions):
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdivisions, radius=radius * UM)
    bm.to_mesh(mesh)
    bm.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def read_vertices(obj):
    mesh = obj.data
    data = np.empty(len(mesh.vertices) * 3)
    mesh.vertices.foreach_get("co", data)
    return data.reshape(-1, 3) / UM


def write_vertices(obj, coordinates):
    obj.data.vertices.foreach_set("co", (coordinates * UM).ravel())
    obj.data.update()


def add_uv_and_attribute(obj, radial_fraction=None, tint=None, contact=None):
    """
    Kugel-UV plus Farbattribut: R = Zufall je Region (oder je Zelle, wenn tint übergeben wird),
    G = radialer Anteil, B = Rauschen.
    """
    mesh = obj.data
    co = read_vertices(obj)
    length = np.maximum(np.linalg.norm(co, axis=1), 1e-6)
    normal = co / length[:, None]
    u = 0.5 + np.arctan2(normal[:, 1], normal[:, 0]) / (2.0 * math.pi)
    v = 0.5 + np.arcsin(np.clip(normal[:, 2], -1.0, 1.0)) / math.pi
    uv_layer = mesh.uv_layers.new(name="UVMap")
    loop_vertices = np.array([loop.vertex_index for loop in mesh.loops])
    uv_layer.data.foreach_set("uv", np.stack([u[loop_vertices], v[loop_vertices]], axis=1).astype(np.float32).ravel())

    noise = 0.5 + 0.5 * value_noise(co / 20.0, 1.0, 77, octaves=4)
    if tint is not None and len(tint) == len(co):
        # Jede Zelle bekommt ihren eigenen Ton – ohne das sehen 950 Zellen aus wie gegossen
        noise = tint
    fraction = radial_fraction if radial_fraction is not None else np.clip(length / 120.0, 0.0, 1.0)
    # Dritter Kanal: normalerweise nur das Gegenstück zum Ton, bei gepackten Zellen die Berührungstiefe.
    # Sie sagt dem Material, wo eine Zelle an der nächsten anliegt – dort kommt kein Licht hin.
    third = contact if contact is not None and len(contact) == len(co) else 1.0 - noise
    color = mesh.color_attributes.new(name="Cell", type="FLOAT_COLOR", domain="POINT")
    color.data.foreach_set("color", np.stack([noise, fraction, third, np.ones(len(co))], axis=1).astype(np.float32).ravel())


def build_ooplasm():
    """Zellleib: nahezu kugelig, mit feiner Unregelmäßigkeit und Mikrovilli-Relief (im Material verfeinert)."""
    obj = sphere("SM_GEN_OocyteCytoplasm", OOPLASM_RADIUS, 6)
    co = read_vertices(obj)
    direction = co / np.linalg.norm(co, axis=1, keepdims=True)
    radius = OOPLASM_RADIUS * (1.0 + 0.012 * value_noise(direction, 2.0, 11) + 0.006 * value_noise(direction, 9.0, 12))
    write_vertices(obj, direction * radius[:, None])
    add_uv_and_attribute(obj, np.ones(len(co)))
    return obj


def build_zona():
    """Zona pellucida als echte Schale mit Dicke – nicht als einzelne Fläche, sonst fehlt die Brechung an den Kanten."""
    obj = sphere("SM_GEN_OocyteZona", 1.0, 6)
    co = read_vertices(obj)
    direction = co / np.linalg.norm(co, axis=1, keepdims=True)
    outer = (ZONA_INNER + ZONA_THICKNESS) * (1.0 + 0.010 * value_noise(direction, 2.5, 21) + 0.005 * value_noise(direction, 12.0, 22))
    write_vertices(obj, direction * outer[:, None])

    modifier = obj.modifiers.new("Thickness", "SOLIDIFY")
    modifier.thickness = ZONA_THICKNESS * UM
    modifier.offset = -1.0
    modifier.use_even_offset = True
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    obj.select_set(False)
    co = read_vertices(obj)
    fraction = np.clip((np.linalg.norm(co, axis=1) - ZONA_INNER) / ZONA_THICKNESS, 0.0, 1.0)
    add_uv_and_attribute(obj, fraction)
    return obj


def build_polar_body():
    """Erster Polkörper im perivitellinen Spalt – das Nebenprodukt der Reifeteilung."""
    obj = sphere("SM_GEN_OocytePolarBody", POLAR_BODY_RADIUS, 3)
    co = read_vertices(obj)
    direction = co / np.linalg.norm(co, axis=1, keepdims=True)
    radius = POLAR_BODY_RADIUS * (1.0 + 0.10 * value_noise(direction, 3.0, 31))
    position = np.array([0.0, 0.0, OOPLASM_RADIUS + PERIVITELLINE - POLAR_BODY_RADIUS * 0.6])
    write_vertices(obj, direction * radius[:, None] + position)
    add_uv_and_attribute(obj, np.ones(len(co)))
    return obj




def build_corona():
    """
    Corona radiata und Cumulus: dicht gepackte Zellen, die sich gegenseitig platt drücken.

    Der entscheidende Punkt ist die Packung. Einzelne Ellipsoide nebeneinander sehen aus wie Popcorn;
    lebendes Gewebe besteht aus Zellen, die aneinander anliegen. Wo zwei Zellen ineinanderstünden,
    legt sich deshalb die eine um die andere: Jeder Punkt, der in der Nachbarzelle läge, wird auf
    deren Oberfläche hinausgeschoben. Die beiden teilen sich dann eine **gekrümmte** Grenzfläche.

    Bis GENESIS-033 wurde stattdessen an einer Ebene abgeschnitten (Potenz-Voronoi). Das ist die
    übliche Schaum-Näherung und war hier falsch: Ein großer Teil der Kugel fällt dabei auf eine
    Kreisscheibe zusammen, deren Rand eine scharfe Kante ist – und wenn die Nachbarzelle kleiner
    ist als diese Scheibe, sieht man von außen direkt auf sie. Die Zellen sahen aus wie
    geschnittener Stein, und kein Verrundungsradius konnte das heilen.

    Was dabei entsteht, steht in der Vertexfarbe:
      R = Ton der Zelle, G = Lage im Komplex, B = Berührungstiefe (1 = freie Oberfläche, 0 = platt gedrückt).
    Die Berührungsflächen bekommen im Material kein Licht – erst dadurch sieht man einzelne Zellen
    statt einer Masse aus hellen Kugeln.
    """
    centers = np.zeros((0, 3))
    radii = np.zeros(0)
    axes = []
    layers = []
    tints_per_cell = []

    attempts = 0
    while len(centers) < 2600 and attempts < 600000:
        attempts += 1
        direction = rng.normal(size=3)
        direction /= np.linalg.norm(direction)
        # Fünf sich überlappende Lagen: dicht gepackt wie bei einer Brombeere
        layer = int(rng.choice([0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 4]))
        distance = ZONA_INNER + ZONA_THICKNESS + 6.0 + layer * 10.0 + rng.normal(0.0, 2.8)
        # Lücken in der Matrix: nicht überall sitzen Zellen, nach außen immer weniger
        if 0.5 + 0.5 * value_noise(direction[None, :], 1.7, 41)[0] < 0.10 + 0.17 * layer:
            continue
        position = direction * distance
        # Innen dicht, außen locker – und das ist der eigentliche Zustand des Komplexes zum Eisprung:
        # Die Corona radiata liegt der Zona noch dicht an, der äußere Cumulus ist **expandiert**.
        # Die Zellen dort haben Hyaluronsäure abgegeben, die Gallerte hat sie auseinandergedrückt.
        # Deshalb wächst der Mindestabstand nach außen; in die Lücken kommen später die Fäden.
        spacing = 5.4 + 1.15 * layer
        if len(centers) and np.min(np.linalg.norm(centers - position, axis=1)) < spacing:
            continue

        # Größenstreuung wie im Gewebe: Cumuluszellen messen 7–16 µm, keine zwei sind gleich
        long_axis = rng.uniform(7.5, 15.5)
        cross = long_axis * rng.uniform(0.62, 0.92)
        third = cross * rng.uniform(0.82, 1.08)

        # Nur die innerste Lage richtet sich radial aus (Corona radiata: die Zellen stehen strahlig um die Zona)
        jitter = rng.normal(size=3) * (0.35 + 0.45 * layer)
        x_axis = direction + jitter
        x_axis /= np.linalg.norm(x_axis)
        helper = np.array([0.0, 0.0, 1.0]) if abs(x_axis[2]) < 0.9 else np.array([1.0, 0.0, 0.0])
        y_axis = np.cross(helper, x_axis)
        y_axis /= np.linalg.norm(y_axis)
        z_axis = np.cross(x_axis, y_axis)
        roll = rng.uniform(0.0, 2.0 * math.pi)
        y_axis, z_axis = y_axis * math.cos(roll) + z_axis * math.sin(roll), -y_axis * math.sin(roll) + z_axis * math.cos(roll)

        centers = np.vstack([centers, position])
        radii = np.append(radii, 0.5 * (long_axis + cross) * 0.5)
        axes.append((np.stack([x_axis, y_axis, z_axis]), np.array([long_axis * 0.5, cross * 0.5, third * 0.5])))
        layers.append(layer)
        tints_per_cell.append(float(rng.uniform(0.15, 1.0)))

    # Eine Vorlage für alle Zellen. Unterteilung 4 statt 3 – und das ist kein Feinschliff, sondern
    # die Ursache eines sichtbaren Fehlers: Bei Unterteilung 3 ist eine Dreieckskante rund 2,7 µm
    # lang. Der weiche Übergang an den Berührungsflächen misst aber nur wenige Zehntel Mikrometer.
    # Er fiel damit komplett zwischen zwei Punkte – die Zellen sahen aus wie geschliffener Stein.
    # Mit Unterteilung 4 (1 280 statt 320 Flächen je Zelle, Kante ≈ 1,35 µm) hat der Übergang
    # überhaupt erst Stützpunkte. Nanite trägt die vierfache Menge ohne Mehrkosten im Bild.
    template = bmesh.new()
    bmesh.ops.create_icosphere(template, subdivisions=4, radius=1.0)
    template.verts.ensure_lookup_table()
    unit = np.array([vert.co[:] for vert in template.verts])
    template_faces = [tuple(vert.index for vert in face.verts) for face in template.faces]
    template.free()

    cells = bmesh.new()
    tints = []
    contacts = []
    clipped_total = 0

    for index in range(len(centers)):
        position = centers[index]
        basis, scale = axes[index]
        layer = layers[index]

        local = unit * scale
        local *= (1.0 + 0.24 * value_noise(local / max(scale[1], 1e-3), 0.7, int(rng.integers(1e6))))[:, None]
        world = local @ basis + position
        free = np.linalg.norm(world - position, axis=1)

        # Nachbarn: Wo zwei Zellen ineinanderstehen würden, legt sich die eine um die andere.
        #
        # Vorher wurde an einer **Ebene** abgeschnitten (Potenz-Voronoi). Das ist die übliche
        # Schaum-Näherung, und sie ist hier die Ursache eines hartnäckigen Fehlers gewesen: Ein
        # großer Teil der Kugel fällt dabei auf eine Kreisscheibe zusammen. Deren Rand ist eine
        # scharfe Kante, und wenn die Nachbarzelle kleiner ist als diese Scheibe, schaut man von
        # außen direkt auf sie. Genau das hat die Zellen wie geschnittenen Stein, wie Kartons
        # aussehen lassen – und kein Verrundungsradius konnte es heilen, weil die ebene Fläche
        # selbst das Problem war.
        #
        # Jetzt wird an der **tatsächlichen Oberfläche der Nachbarzelle** abgeschnitten: Jeder Punkt,
        # der in der Nachbarin läge, wird auf deren Ellipsoid hinausgeschoben. Zwei Zellen teilen
        # sich damit eine gekrümmte Grenzfläche, so wie zwei aneinandergedrückte Tropfen. Es gibt
        # keine ebene Fläche mehr, keinen scharfen Scheibenrand und keinen Spalt dazwischen.
        distances = np.linalg.norm(centers - position, axis=1)
        neighbours = np.where((distances > 1e-6) & (distances < 26.0))[0]
        for other in neighbours:
            other_basis, other_scale = axes[other]
            # Wo liegt der Punkt in den Achsen der Nachbarzelle? 1 ist genau ihre Oberfläche.
            relative = (world - centers[other]) @ other_basis.T
            depth = np.linalg.norm(relative / np.maximum(other_scale, 1e-6), axis=1)

            touching = depth < 1.0
            if not np.any(touching):
                continue

            # Zielpunkt: derselbe Punkt, aber auf die Oberfläche der Nachbarin hinausgeschoben.
            # An der Grenze (Tiefe genau 1) ist der Zielpunkt der Punkt selbst – die Fläche bleibt
            # also zusammenhängend. Was bleibt, ist der Knick der Membran an der Berührungslinie,
            # und den gibt es an zwei aneinanderliegenden Zellen wirklich.
            factor = 1.0 / np.maximum(depth[touching], 1e-6)
            target = centers[other] + (relative[touching] * factor[:, None]) @ other_basis
            world[touching] = target
            clipped_total += int(np.count_nonzero(touching))

        # Berührungstiefe: Wie weit ist dieser Punkt gegenüber der freien Form eingedrückt?
        pressed = np.linalg.norm(world - position, axis=1)
        contact = np.clip(pressed / np.maximum(free, 1e-6), 0.0, 1.0)

        mesh_temp = bpy.data.meshes.new("tmp")
        temp = bmesh.new()
        added = [temp.verts.new(Vector(point * UM)) for point in world]
        temp.verts.ensure_lookup_table()
        for face in template_faces:
            temp.faces.new([added[i] for i in face])
        bmesh.ops.recalc_face_normals(temp, faces=temp.faces)
        temp.to_mesh(mesh_temp)
        temp.free()

        tints.extend([tints_per_cell[index]] * len(mesh_temp.vertices))
        contacts.extend(contact.tolist())
        cells.from_mesh(mesh_temp)
        bpy.data.meshes.remove(mesh_temp)

        # Transzonale Fortsätze der innersten Lage: beim Eisprung nicht mehr vorhanden. Nach dem LH-Anstieg ziehen sie
        # sich zurück (Maus: 4–8 h danach ganz, Voraussetzung für die Reifeteilung; Amargant 2023, DOI
        # 10.1093/humrep/dead162); der Eisprung folgt beim Menschen nach ~36 h. Bis GENESIS-047 Teil 2c standen hier
        # 3 µm dicke Kegel bis in die Zona. Der Zufallszug bleibt, damit alle Zellen ihre Form behalten.
        if layer == 0 and rng.random() < 0.75 and WITH_TRANSZONAL_PROJECTIONS:
            direction = position / np.linalg.norm(position)
            y_axis, z_axis = basis[1], basis[2]
            process = bmesh.new()
            tip = direction * (ZONA_INNER + ZONA_THICKNESS * 0.35)
            base = position - direction * scale[0] * 0.8
            axis = tip - base
            height = np.linalg.norm(axis)
            axis_dir = axis / max(height, 1e-6)
            ring_vertices = []
            for segment in range(6):
                ring = []
                fraction = segment / 5.0
                radius_here = 1.5 * (1.0 - 0.75 * fraction)
                center = base + axis_dir * height * fraction
                for side in range(5):
                    angle = 2.0 * math.pi * side / 5.0
                    offset = (y_axis * math.cos(angle) + z_axis * math.sin(angle)) * radius_here
                    ring.append(process.verts.new(Vector((center + offset) * UM)))
                ring_vertices.append(ring)
            for lower, upper in zip(ring_vertices[:-1], ring_vertices[1:]):
                for side in range(5):
                    other = (side + 1) % 5
                    process.faces.new((lower[side], lower[other], upper[other], upper[side]))
            bmesh.ops.recalc_face_normals(process, faces=process.faces)
            mesh_process = bpy.data.meshes.new("tmp_process")
            process.to_mesh(mesh_process)
            process.free()
            tints.extend([tints_per_cell[index]] * len(mesh_process.vertices))
            contacts.extend([1.0] * len(mesh_process.vertices))
            cells.from_mesh(mesh_process)
            bpy.data.meshes.remove(mesh_process)

    mesh = bpy.data.meshes.new("SM_GEN_OocyteCorona")
    cells.to_mesh(mesh)
    cells.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj = bpy.data.objects.new("SM_GEN_OocyteCorona", mesh)
    bpy.context.scene.collection.objects.link(obj)
    co = read_vertices(obj)
    add_uv_and_attribute(obj, np.clip((np.linalg.norm(co, axis=1) - ZONA_INNER - ZONA_THICKNESS) / 60.0, 0.0, 1.0),
                         tint=np.array(tints), contact=np.array(contacts))
    contact_array = np.array(contacts)
    pressed_share = 100.0 * float(np.mean(contact_array < 0.90))
    print("GENESIS: Corona-Zellen", len(centers), "Vertices", len(co),
          "| deutlich gedrueckte Punkte", round(pressed_share, 1), "%",
          "| Beruehrungstiefe P10", round(float(np.percentile(contact_array, 10)), 3),
          "Median", round(float(np.percentile(contact_array, 50)), 3))
    return obj, centers, radii, np.array(layers)


def build_strands(centers, radii, layers):
    """
    Die Hyaluronsäure-Matrix zwischen den Zellen.

    Beim Eisprung geben die Cumuluszellen Hyaluronsäure ab; sie bindet Wasser, quillt auf und drückt
    die Zellen auseinander – der Komplex **expandiert**. Was dabei zwischen den Zellen steht, ist kein
    leerer Raum, sondern ein zähes, fast klares Gel, das an beiden Zellen hängt und beim Auseinander-
    driften Fäden zieht. Genau diese Fäden sind das, was den Komplex im Mikroskop wie eine Wolke
    aussehen lässt – und was die Spermien abbremst, bevor sie die Zona erreichen.

    Gebaut wird zwischen Zellen, deren Oberflächen 0,5–7 µm auseinanderliegen: ein dünner Strang mit
    Durchhang, an den Enden breiter (dort haftet er an der Zelle), in der Mitte am dünnsten.
    """
    strands = bmesh.new()
    count = 0
    per_cell = np.zeros(len(centers), dtype=int)

    order = rng.permutation(len(centers))
    for index in order:
        distances = np.linalg.norm(centers - centers[index], axis=1)
        neighbours = np.argsort(distances)[1:9]
        for other in neighbours:
            if per_cell[index] >= 3 or per_cell[other] >= 3:
                continue
            gap = distances[other] - radii[index] - radii[other]
            # Zu dicht: Dort berühren sich die Zellen ohnehin. Zu weit: Der Faden wäre gerissen.
            if gap < 0.5 or gap > 7.0:
                continue
            # Nach außen zieht die Gallerte mehr Fäden – dort ist der Komplex expandiert
            layer = max(int(layers[index]), int(layers[other]))
            if rng.random() > 0.25 + 0.18 * layer:
                continue

            start = centers[index]
            end = centers[other]
            axis = end - start
            length = np.linalg.norm(axis)
            axis_dir = axis / length
            helper = np.array([0.0, 0.0, 1.0]) if abs(axis_dir[2]) < 0.9 else np.array([1.0, 0.0, 0.0])
            side = np.cross(helper, axis_dir)
            side /= np.linalg.norm(side)
            up = np.cross(axis_dir, side)

            # Durchhang quer zur Verbindung: Ein Faden aus Gel hängt, er steht nicht
            sag_direction = side * rng.uniform(-1.0, 1.0) + up * rng.uniform(-1.0, 1.0)
            sag_norm = np.linalg.norm(sag_direction)
            sag_direction = sag_direction / sag_norm if sag_norm > 1e-6 else side
            sag = gap * rng.uniform(0.12, 0.35)

            rings = []
            segments = 5
            sides = 5
            for segment in range(segments + 1):
                fraction = segment / segments
                # Von Zelloberfläche zu Zelloberfläche, nicht von Mittelpunkt zu Mittelpunkt
                along = radii[index] + fraction * (length - radii[index] - radii[other])
                center = start + axis_dir * along + sag_direction * sag * math.sin(math.pi * fraction)
                # An den Enden breiter: Dort haftet das Gel an der Zelle
                radius_here = rng.uniform(0.09, 0.22) * (1.0 + 1.1 * (1.0 - math.sin(math.pi * fraction)))
                ring = []
                for corner in range(sides):
                    angle = 2.0 * math.pi * corner / sides
                    offset = (side * math.cos(angle) + up * math.sin(angle)) * radius_here
                    ring.append(strands.verts.new(Vector((center + offset) * UM)))
                rings.append(ring)
            for lower, upper in zip(rings[:-1], rings[1:]):
                for corner in range(sides):
                    other_corner = (corner + 1) % sides
                    strands.faces.new((lower[corner], lower[other_corner], upper[other_corner], upper[corner]))

            per_cell[index] += 1
            per_cell[other] += 1
            count += 1

    bmesh.ops.recalc_face_normals(strands, faces=strands.faces)
    mesh = bpy.data.meshes.new("SM_GEN_OocyteStrands")
    strands.to_mesh(mesh)
    strands.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj = bpy.data.objects.new("SM_GEN_OocyteStrands", mesh)
    bpy.context.scene.collection.objects.link(obj)
    co = read_vertices(obj)
    add_uv_and_attribute(obj, np.clip((np.linalg.norm(co, axis=1) - ZONA_INNER - ZONA_THICKNESS) / 60.0, 0.0, 1.0))
    print("GENESIS: Matrixfäden", count, "| Vertices", len(co),
          "| Zellen mit Faden", int(np.count_nonzero(per_cell)), "von", len(centers))
    return obj


def build_matrix():
    """
    Hyaluronsäure-Matrix des Cumulus: eine gallertige, fast klare Masse, in der die Zellen stecken.
    Ohne sie sähe der Komplex aus wie lose schwebende Kugeln. Sie streut Licht und macht den Komplex wolkig.
    """
    obj = sphere("SM_GEN_OocyteMatrix", 1.0, 5)
    co = read_vertices(obj)
    direction = co / np.linalg.norm(co, axis=1, keepdims=True)
    # Die Gallerte muss die äußerste Zelllage einschließen (bis ~128 µm), sonst ragen Zellen sichtbar aus der Wolke
    radius = 118.0 * (1.0 + 0.16 * value_noise(direction, 1.2, 51) + 0.08 * value_noise(direction, 3.5, 52))
    write_vertices(obj, direction * radius[:, None])
    add_uv_and_attribute(obj, np.clip((radius - ZONA_INNER - ZONA_THICKNESS) / 60.0, 0.0, 1.0))
    return obj


def build_cumulus_cells(variants=4):
    """
    Zellen des äußeren Cumulus (GENESIS-047 Teil 2) als Formvarianten zum Instanzieren: frei in der Gallerte,
    an niemandem gedrückt. Einheitsgröße (Radius 1 µm) – die Instanz in Unreal trägt die Halbachsen (7–16 µm lang),
    die Lage und den Ton (AGenesisOocyte::RebuildCumulus). 13.400 Zellen als ein Netz wären 17 Millionen Flächen
    in einer Datei; als Instanzen teilen sie sich vier Netze.

    Vertexfarbe wie bei der Corona: R = Ton (hier nur Vorgabe, im Spiel je Instanz), G = Lage, B = 1 (freie Oberfläche).
    """
    cells = []
    for variant in range(variants):
        obj = sphere("SM_GEN_CumulusCell_%d" % variant, 1.0, 4)
        unit = read_vertices(obj) / np.linalg.norm(read_vertices(obj), axis=1, keepdims=True)
        # Dieselbe Unregelmäßigkeit wie bei den Coronazellen: keine Kugel, keine zwei gleich
        bumps = 1.0 + 0.24 * value_noise(unit, 0.7, 300 + variant) + 0.05 * value_noise(unit, 3.0, 400 + variant)
        write_vertices(obj, unit * bumps[:, None])
        for polygon in obj.data.polygons:
            polygon.use_smooth = True
        count = len(obj.data.vertices)
        add_uv_and_attribute(obj, np.ones(count), tint=np.full(count, 0.5), contact=np.ones(count))
        cells.append(obj)
    return cells


def export_fbx(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE", use_tspace=False,
                             colors_type="LINEAR", add_leaf_bones=False)
    print("GENESIS: exportiert", os.path.basename(path), round(os.path.getsize(path) / 1e6, 1), "MB")


def make_material(name, base_color, subsurface_radius, subsurface_scale, transmission, roughness, ior=1.04):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = new_node(tree, "ShaderNodeOutputMaterial", (600, 0))
    bsdf = new_node(tree, "ShaderNodeBsdfPrincipled", (300, 0))
    bsdf.inputs["Base Color"].default_value = base_color
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["IOR"].default_value = ior
    bsdf.inputs["Transmission Weight"].default_value = transmission
    bsdf.inputs["Subsurface Weight"].default_value = 1.0 - transmission
    bsdf.inputs["Subsurface Radius"].default_value = subsurface_radius
    bsdf.inputs["Subsurface Scale"].default_value = subsurface_scale * UM
    coords = new_node(tree, "ShaderNodeTexCoord", (-600, -300))
    grain = new_node(tree, "ShaderNodeTexNoise", (-400, -300))
    grain.inputs["Scale"].default_value = 1.0 / (1.2 * UM)
    tree.links.new(coords.outputs["Object"], grain.inputs["Vector"])
    bump = new_node(tree, "ShaderNodeBump", (0, -300))
    bump.inputs["Strength"].default_value = 0.25
    bump.inputs["Distance"].default_value = 0.3 * UM
    tree.links.new(grain.outputs["Fac"], bump.inputs["Height"])
    tree.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    tree.links.new(bsdf.outputs["BSDF"], output.inputs["Surface"])
    return material


def render_lookdev(objects, out_dir, exposure, samples, energy):
    scene = bpy.context.scene
    materials = {
        "SM_GEN_OocyteCytoplasm": make_material("M_Ooplasm", (0.42, 0.34, 0.26, 1.0), (1.4, 0.9, 0.6), 5.0, 0.10, 0.32),
        "SM_GEN_OocyteZona": make_material("M_Zona", (0.86, 0.80, 0.68, 1.0), (2.0, 1.8, 1.6), 25.0, 0.88, 0.10, ior=1.035),
        "SM_GEN_OocyteCorona": make_material("M_Corona", (0.80, 0.76, 0.70, 1.0), (1.2, 0.8, 0.6), 8.0, 0.86, 0.16, ior=1.045),
        "SM_GEN_OocyteMatrix": make_material("M_Matrix", (0.96, 0.96, 0.95, 1.0), (4.0, 4.0, 4.0), 160.0, 0.995, 0.05, ior=1.008),
        "SM_GEN_OocyteStrands": make_material("M_Strands", (0.94, 0.93, 0.91, 1.0), (3.0, 3.0, 3.0), 90.0, 0.97, 0.08, ior=1.012),
        "SM_GEN_OocytePolarBody": make_material("M_PolarBody", (0.62, 0.58, 0.54, 1.0), (1.0, 0.7, 0.5), 3.0, 0.25, 0.30),
    }
    for obj in objects:
        obj.data.materials.clear()
        obj.data.materials.append(materials[obj.name])

    enable_gpu(scene)
    configure_color_management(scene, exposure=exposure)
    configure_cycles(scene, samples=samples, width=1280, height=720)

    world = bpy.data.worlds.new("World")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes.clear()
    background = new_node(world.node_tree, "ShaderNodeBackground", (0, 0))
    background.inputs["Strength"].default_value = 0.0
    world_output = new_node(world.node_tree, "ShaderNodeOutputWorld", (300, 0))
    world.node_tree.links.new(background.outputs[0], world_output.inputs[0])

    camera_data = bpy.data.cameras.new("Camera")
    camera_data.sensor_width = 36.0
    camera_data.lens = 50.0
    camera_data.clip_start = 0.5 * UM
    camera_data.dof.use_dof = True
    camera_data.dof.aperture_fstop = 11.0
    camera = bpy.data.objects.new("Camera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.location = Vector((320.0, -230.0, 120.0)) * UM
    target = Vector((0.0, 0.0, 0.0))
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera_data.dof.focus_distance = (target - camera.location).length

    light_data = bpy.data.lights.new("EndoscopeLight", "SPOT")
    light_data.energy = energy
    light_data.spot_size = math.radians(80.0)
    light_data.spot_blend = 0.5
    light_data.shadow_soft_size = 1.5 * UM
    light = bpy.data.objects.new("EndoscopeLight", light_data)
    scene.collection.objects.link(light)
    light.parent = camera
    light.location = (2.0 * UM, -1.0 * UM, 0.0)

    scene.render.filepath = os.path.join(out_dir, "LookDev_Oocyte.png")
    bpy.ops.render.render(write_still=True)
    measure_exposure(scene.render.filepath)


def main():
    args = script_args()
    out_dir = ensure_dir(os.path.abspath(args.get("out", "ArtSource/Generated/Conception")))
    reset_scene()

    # Nur die Cumuluszellen (GENESIS-047 Teil 2) – ohne die Corona neu zu packen (112 MB, mehrere Minuten)
    if args.get("cumulus_only"):
        for cell in build_cumulus_cells():
            if args.get("export"):
                export_fbx(cell, os.path.join(out_dir, cell.name + ".fbx"))
        return

    ooplasm = build_ooplasm()
    zona = build_zona()
    polar_body = build_polar_body()
    corona, centers, radii, layers = build_corona()
    strands = build_strands(centers, radii, layers)
    matrix = build_matrix()
    objects = (ooplasm, zona, polar_body, corona, strands, matrix)
    for obj in objects:
        dimensions = obj.dimensions
        print("GENESIS: %-28s %6.1f × %6.1f × %6.1f µm, %d Flächen" % (
            obj.name, dimensions.x / UM, dimensions.y / UM, dimensions.z / UM, len(obj.data.polygons)))

    if args.get("export"):
        # Die Gallerte als Ganzes bleibt in Blender (sie macht das Cycles-Lookdev wolkig), geht aber nicht nach Unreal:
        # Dort wirkte sie als Kugel mit harter Silhouette wie eine Plastikschale und ist ungesetzt (setup_oocyte.py).
        for obj in objects:
            if obj is not matrix:
                export_fbx(obj, os.path.join(out_dir, obj.name + ".fbx"))
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out_dir, "Oocyte.blend"))
    if args.get("render"):
        render_lookdev(list(objects), out_dir, float(args.get("exposure", 0.0)),
                       int(args.get("samples", 256)), float(args.get("energy", 30000.0)))


main()
