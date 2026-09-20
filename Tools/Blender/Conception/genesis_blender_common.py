# GENESIS: Der Kreislauf des Lebens
# Gemeinsame Helfer für die Blender-Skripte der Entstehungsszenen (headless: blender -b -P <skript> -- <args>).
#
# Maßstab der Mikrowelt: 1 µm = 1 cm in Unreal = 0,01 m in Blender (Faktor 10.000).
# Unreal kann Mikrometer nicht präzise darstellen (Gleitkomma, Near Clip, Lumen); alle Proportionen bleiben real.

import math
import os
import sys

import bpy

UM = 0.01  # 1 Mikrometer in Blender-Metern


def script_args():
    """Argumente nach '--' als dict (--key value)."""
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    result = {}
    key = None
    for item in argv:
        if item.startswith("--"):
            key = item[2:]
            result[key] = True
        elif key:
            result[key] = item
            key = None
    return result


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.unit_settings.length_unit = "METERS"
    return scene


def smoothstep(edge0, edge1, x):
    if edge1 == edge0:
        return 1.0 if x >= edge1 else 0.0
    t = max(0.0, min(1.0, (x - edge0) / (edge1 - edge0)))
    return t * t * (3.0 - 2.0 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def enable_gpu(scene):
    """Cycles auf OptiX (RTX) – tatsächlich prüfen, nicht annehmen."""
    scene.render.engine = "CYCLES"
    prefs = bpy.context.preferences.addons["cycles"].preferences
    prefs.compute_device_type = "OPTIX"
    prefs.get_devices()
    active = []
    for device in prefs.devices:
        device.use = device.type == "OPTIX"
        if device.use:
            active.append(device.name)
    scene.cycles.device = "GPU" if active else "CPU"
    print("GENESIS: Cycles-Geraet", scene.cycles.device, active)
    return bool(active)


def configure_color_management(scene, exposure=0.0):
    scene.view_settings.view_transform = "AgX"
    for look in ("AgX - Medium High Contrast", "Medium High Contrast", "AgX - Base Contrast", "None"):
        try:
            scene.view_settings.look = look
            break
        except TypeError:
            continue
    scene.view_settings.exposure = exposure


def configure_cycles(scene, samples=512, noise_threshold=0.01, width=1600, height=900):
    cycles = scene.cycles
    cycles.samples = samples
    cycles.use_adaptive_sampling = True
    cycles.adaptive_threshold = noise_threshold
    cycles.adaptive_min_samples = 32
    cycles.use_denoising = True
    cycles.denoiser = "OPENIMAGEDENOISE"
    cycles.max_bounces = 16
    cycles.diffuse_bounces = 6
    cycles.glossy_bounces = 6
    cycles.transmission_bounces = 16
    cycles.volume_bounces = 4
    cycles.transparent_max_bounces = 32
    cycles.sample_clamp_direct = 0.0
    cycles.sample_clamp_indirect = 8.0
    for name, value in (("volume_step_rate", 0.25), ("volume_preview_step_rate", 0.5), ("volume_max_steps", 2048)):
        if hasattr(cycles, name):
            setattr(cycles, name, value)
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_depth = "16"


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)
    return path


def new_node(tree, node_type, location, **props):
    node = tree.nodes.new(node_type)
    node.location = location
    for name, value in props.items():
        setattr(node, name, value)
    return node


def measure_exposure(path, mask_threshold=0.02):
    """Belichtung messen statt schätzen: Luminanz-Perzentile (Anzeige-Werte 0..1) und Anteil ausgebrannter Pixel im Motiv."""
    image = bpy.data.images.load(path, check_existing=False)
    pixels = list(image.pixels)
    lums = []
    for index in range(0, len(pixels), 4):
        lum = 0.2126 * pixels[index] + 0.7152 * pixels[index + 1] + 0.0722 * pixels[index + 2]
        if lum > mask_threshold:
            lums.append(lum)
    bpy.data.images.remove(image)
    if not lums:
        print("GENESIS_EXPOSURE", os.path.basename(path), "kein Motiv ueber Schwelle")
        return None
    lums.sort()

    def pct(p):
        return lums[min(len(lums) - 1, int(p * len(lums)))]

    clipped = sum(1 for value in lums if value >= 0.98) / len(lums)
    print("GENESIS_EXPOSURE", os.path.basename(path), "motiv_px", len(lums), "p50 %.3f p90 %.3f p99 %.3f p999 %.3f ausgebrannt %.4f" % (pct(0.5), pct(0.9), pct(0.99), pct(0.999), clipped))
    return {"p50": pct(0.5), "p99": pct(0.99), "clipped": clipped}