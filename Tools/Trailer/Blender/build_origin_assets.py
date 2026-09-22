# GENESIS Conception (gemeinsam Spiel + Trailer SEQ_001) - Eizelle, Eileiter-Schleimhaut, Schwebeteilchen (headless).
# Das Spermium ist NICHT hier: SM_GEN_SpermCell entsteht in Tools/Blender/Conception/build_sperm_cell.py.
#
# Massstab: 1 Mikrometer = 1 cm (Faktor 10.000). So gelten reale Objektive/Blenden in Unreal ohne Miniatur-Effekt.
# Reale Masse (Mensch): Eizelle ca. 110 um, Zona pellucida ca. 13 um dick, Corona-radiata-Zellen ca. 10-15 um,
# Epithel der Ampulle ca. 10-12 um je Zelle, Schleimhautfalten mehrere 100 um hoch.
#
# Aufruf:
#   "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b --factory-startup --python Tools\Trailer\Blender\build_origin_assets.py -- [--preview]
# Ausgabe: ArtSource/Generated/Conception/Environment_Assets.blend und SM_GEN_*.fbx, Vorschau-Renders

import math
import os
import sys

import bmesh
import bpy
import numpy as np

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
OUT = os.path.join(REPO, "ArtSource", "Generated", "Conception")
EXPORT = OUT
PREVIEW = os.path.join(REPO, "ArtSource", "Generated", "Conception", "LookDev")
UM = 0.01                     # 1 um in Blender-Metern
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
rng = np.random.default_rng(1306)


# ---------------------------------------------------------------- Hilfen
def periodic_noise_3d(points, freq, seed, octaves=3):
    """Glatte, deterministische Wertrauschen-Summe (vektorisiert) fuer Verformungen."""
    r = np.random.default_rng(seed)
    total = np.zeros(len(points))
    amp = 1.0
    norm = 0.0
    for o in range(octaves):
        f = freq * (2.0 ** o)
        dirs = r.normal(size=(6, 3))
        dirs /= np.linalg.norm(dirs, axis=1, keepdims=True)
        phases = r.uniform(0, 2 * math.pi, 6)
        layer = sum(np.sin(points @ d * f * 2 * math.pi + p) for d, p in zip(dirs, phases)) / 6.0
        total += layer * amp
        norm += amp
        amp *= 0.5
    return total / norm


# ---------------------------------------------------------------- Eizelle
def icosphere(name, radius, subdiv):
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=radius)
    bm.to_mesh(me)
    bm.free()
    for p in me.polygons:
        p.use_smooth = True
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    return ob


def displace(ob, func):
    me = ob.data
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3)
    co = func(co)
    me.vertices.foreach_set("co", co.ravel())
    me.update()


def spherical_uv(ob):
    me = ob.data
    co = np.array([v.co[:] for v in me.vertices])
    n = co / np.linalg.norm(co, axis=1, keepdims=True)
    u = 0.5 + np.arctan2(n[:, 1], n[:, 0]) / (2 * math.pi)
    v = 0.5 + np.arcsin(np.clip(n[:, 2], -1, 1)) / math.pi
    uv = me.uv_layers.new(name="UVMap")
    loops = np.array([me.loops[i].vertex_index for i in range(len(me.loops))])
    uv.data.foreach_set("uv", np.stack([u[loops] * 4.0, v[loops] * 2.0], axis=1).astype(np.float32).ravel())


def build_oocyte():
    cyto = icosphere("SM_GEN_OocyteCytoplasm", 55.0 * UM, 6)
    displace(cyto, lambda c: c * (1.0 + 0.012 * periodic_noise_3d(c / (55 * UM), 2.0, 11))[:, None])
    spherical_uv(cyto)

    zona = icosphere("SM_GEN_OocyteZona", 1.0, 6)
    def zona_shape(c):
        n = c / np.linalg.norm(c, axis=1, keepdims=True)
        radius = 68.0 * UM * (1.0 + 0.01 * periodic_noise_3d(n, 3.0, 21) + 0.006 * periodic_noise_3d(n, 18.0, 22))
        return n * radius[:, None]
    displace(zona, zona_shape)
    # Wandstaerke als echte Schale (Innenseite 55,5 um -> ca. 12,5 um Zona)
    mod = zona.modifiers.new("Thickness", "SOLIDIFY")
    mod.thickness = 12.5 * UM
    mod.offset = -1.0
    mod.use_even_offset = True
    bpy.context.view_layer.objects.active = zona
    zona.select_set(True)
    bpy.ops.object.modifier_apply(modifier="Thickness")
    zona.select_set(False)
    spherical_uv(zona)

    # Corona radiata: radial gestreckte Zellen in lockeren Schichten, Luecken ueber Rauschen
    cells_bm = bmesh.new()
    count = 0
    attempts = 0
    centers = np.zeros((0, 3))
    while count < 650 and attempts < 40000:
        attempts += 1
        d = rng.normal(size=3)
        d /= np.linalg.norm(d)
        layer = rng.choice([0, 0, 0, 1, 1, 2])
        dist = (76.0 + layer * 14.0 + rng.normal(0, 3.5)) * UM
        density = periodic_noise_3d(d[None, :], 1.6, 31)[0]
        if density < -0.05 + 0.12 * layer:
            continue
        if len(centers) and np.min(np.linalg.norm(centers - d * dist, axis=1)) < 11.0 * UM:
            continue
        centers = np.vstack([centers, d * dist])
        size = rng.uniform(3.8, 5.2) * UM
        tmp = bmesh.new()
        bmesh.ops.create_icosphere(tmp, subdivisions=3, radius=1.0)
        co = np.array([v.co[:] for v in tmp.verts])
        co *= np.array([size * rng.uniform(1.5, 2.1), size, size * rng.uniform(0.8, 1.0)])
        co *= (1.0 + 0.12 * periodic_noise_3d(co / size, 0.35, int(rng.integers(1e6))))[:, None]
        # Achse x radial ausrichten, zufaellige Rolle
        x = d
        helper = np.array([0.0, 0.0, 1.0]) if abs(d[2]) < 0.9 else np.array([1.0, 0.0, 0.0])
        y = np.cross(helper, x)
        y /= np.linalg.norm(y)
        z = np.cross(x, y)
        roll = rng.uniform(0, 2 * math.pi)
        y, z = y * math.cos(roll) + z * math.sin(roll), -y * math.sin(roll) + z * math.cos(roll)
        world = co @ np.stack([x, y, z]) + d * dist
        for v, p in zip(tmp.verts, world):
            v.co = p
        me_tmp = bpy.data.meshes.new("tmp")
        tmp.to_mesh(me_tmp)
        tmp.free()
        cells_bm.from_mesh(me_tmp)
        bpy.data.meshes.remove(me_tmp)
        count += 1
    me = bpy.data.meshes.new("SM_GEN_OocyteCorona")
    cells_bm.to_mesh(me)
    cells_bm.free()
    for p in me.polygons:
        p.use_smooth = True
    corona = bpy.data.objects.new("SM_GEN_OocyteCorona", me)
    bpy.context.scene.collection.objects.link(corona)
    spherical_uv(corona)
    return cyto, zona, corona, count


# ---------------------------------------------------------------- Schleimhautfalten (Eileiter-Ampulle)
def build_mucosa():
    size, res = 30.0, 700
    xs = np.linspace(-size / 2, size / 2, res)
    gx, gy = np.meshgrid(xs, xs, indexing="ij")
    pts = np.stack([gx.ravel(), gy.ravel(), np.zeros(gx.size)], axis=1)
    warp = periodic_noise_3d(pts, 0.06, 41) * 2.2
    folds = np.abs(np.sin((gy.ravel() + warp) * 2 * math.pi / 3.2)) ** 0.55          # Laengsfalten
    secondary = np.abs(np.sin((gy.ravel() * 0.7 + gx.ravel() * 0.25 + warp * 1.7) * 2 * math.pi / 1.1)) ** 0.8
    bumps = periodic_noise_3d(pts, 0.9, 42, octaves=4)
    height = folds * 1.35 + secondary * 0.28 * (0.5 + 0.5 * periodic_noise_3d(pts, 0.1, 43)) + bumps * 0.08
    verts = np.stack([gx.ravel(), gy.ravel(), height], axis=1)
    idx = np.arange(res * res).reshape(res, res)
    faces = np.stack([idx[:-1, :-1].ravel(), idx[1:, :-1].ravel(), idx[1:, 1:].ravel(), idx[:-1, 1:].ravel()], axis=1)
    me = bpy.data.meshes.new("SM_GEN_MucosaFolds")
    me.vertices.add(len(verts))
    me.vertices.foreach_set("co", verts.ravel())
    me.loops.add(faces.size)
    me.loops.foreach_set("vertex_index", faces.ravel())
    me.polygons.add(len(faces))
    me.polygons.foreach_set("loop_start", np.arange(0, faces.size, 4))
    me.polygons.foreach_set("use_smooth", np.ones(len(faces), dtype=bool))
    me.update(calc_edges=True)
    uv = me.uv_layers.new(name="UVMap")
    lv = faces.ravel()
    uv.data.foreach_set("uv", (verts[lv, :2] / 4.0).astype(np.float32).ravel())   # 4 m Kachel
    ob = bpy.data.objects.new("SM_GEN_MucosaFolds", me)
    bpy.context.scene.collection.objects.link(ob)
    return ob


def build_debris():
    obs = []
    for i in range(6):
        ob = icosphere("SM_GEN_Debris_{:02d}".format(i + 1), 1.0, 2)
        size = rng.uniform(0.25, 1.1) * UM
        stretch = np.array([rng.uniform(1.0, 2.2), rng.uniform(0.7, 1.2), rng.uniform(0.5, 1.0)])
        seed = 100 + i
        displace(ob, lambda c, s=size, st=stretch, sd=seed: c * st * s * (1.0 + 0.25 * periodic_noise_3d(c, 0.6, sd))[:, None])
        spherical_uv(ob)
        obs.append(ob)
    return obs


# ---------------------------------------------------------------- Export und Pruefung
def export_fbx(objects, filename):
    bpy.ops.object.select_all(action="DESELECT")
    for ob in objects:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.fbx(filepath=os.path.join(EXPORT, filename), use_selection=True, apply_unit_scale=True,
                             apply_scale_options="FBX_SCALE_UNITS", object_types={"MESH"}, use_mesh_modifiers=True,
                             mesh_smooth_type="FACE", use_tspace=False, add_leaf_bones=False, bake_anim=False,
                             axis_forward="-Z", axis_up="Y")


def report(ob):
    me = ob.data
    dims = [round(d / UM, 2) for d in ob.dimensions]
    tris = sum(len(p.vertices) - 2 for p in me.polygons)
    print("GENESIS_ASSET {:24s} Masse(um) {} Dreiecke {} UV {}".format(ob.name, dims, tris, [u.name for u in me.uv_layers]))


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    os.makedirs(EXPORT, exist_ok=True)

    cyto, zona, corona, cells = build_oocyte()
    mucosa = build_mucosa()
    debris = build_debris()
    print("GENESIS_ASSET Corona-Zellen:", cells)
    for ob in [cyto, zona, corona, mucosa] + debris:
        report(ob)
    for ob in (cyto, zona, corona, mucosa):
        export_fbx([ob], ob.name + ".fbx")
    export_fbx(debris, "SM_GEN_Debris.fbx")
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "Environment_Assets.blend"))
    print("GENESIS_ASSETS_OK")

    if "--preview" in ARGS:
        preview(cyto, zona, corona, [mucosa] + debris)


def preview(cyto, zona, corona, hidden=()):
    """Schneller Form-Check in Cycles (Dunkelfeld-artige Gegenlicht-Situation)."""
    scene = bpy.context.scene
    for ob in hidden:
        ob.hide_render = True
    scene.render.engine = "CYCLES"
    prefs = bpy.context.preferences.addons["cycles"].preferences
    prefs.compute_device_type = "OPTIX"
    prefs.get_devices()
    for d in prefs.devices:
        d.use = d.type == "OPTIX"
    scene.cycles.device = "GPU"
    scene.cycles.samples = 128
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = 1280, 720
    scene.view_settings.view_transform = "AgX"
    world = bpy.data.worlds.new("W")
    world.use_nodes = True
    bg = world.node_tree.nodes.new("ShaderNodeBackground")
    bg.inputs["Color"].default_value = (0.02, 0.006, 0.004, 1)
    world.node_tree.links.new(bg.outputs[0], world.node_tree.nodes.new("ShaderNodeOutputWorld").inputs[0])
    scene.world = world

    mat = bpy.data.materials.new("Lookdev")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.75, 0.68, 0.62, 1)
    bsdf.inputs["Roughness"].default_value = 0.35
    bsdf.inputs["Subsurface Weight"].default_value = 0.6
    bsdf.inputs["Subsurface Radius"].default_value = (0.5 * UM, 0.3 * UM, 0.2 * UM)
    for ob in (cyto, zona, corona):
        ob.data.materials.append(mat)

    light = bpy.data.lights.new("Back", "AREA")
    light.energy = 400.0
    light.size = 2.0
    light.color = (1.0, 0.62, 0.38)
    lo = bpy.data.objects.new("Back", light)
    scene.collection.objects.link(lo)
    cam_data = bpy.data.cameras.new("Cam")
    cam_data.lens = 65
    cam_data.sensor_width = 24.89
    cam = bpy.data.objects.new("Cam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam

    target = bpy.data.objects.new("Target", None)
    scene.collection.objects.link(target)
    for ob_ in (cam, lo):
        con = ob_.constraints.new("TRACK_TO")
        con.target = target
        con.track_axis = "TRACK_NEGATIVE_Z"
        con.up_axis = "UP_Y"
    os.makedirs(PREVIEW, exist_ok=True)
    shots = [("oocyte", (), (0.0, -4.6, 1.2), (0, 0, 0), (0.6, 3.5, 1.8), 50)]
    for label, hide, cam_loc, target_loc, light_loc, lens in shots:
        for ob in (cyto, zona, corona):
            ob.hide_render = ob in hide
        cam.location, target.location, lo.location = cam_loc, target_loc, light_loc
        cam_data.lens = lens
        scene.render.filepath = os.path.join(PREVIEW, "LookDev_{}_form.png".format(label))
        bpy.ops.render.render(write_still=True)
        print("GENESIS_PREVIEW", scene.render.filepath)


main()
