# GENESIS Trailer - einzelne Frames aus einem Video als JPG ziehen (Sichtpruefung).
# Aufruf: blender -b --factory-startup --python Tools\Trailer\Blender\extract_frames.py -- <video> <ausgabeordner> [anzahl=3]
import os
import sys

import bpy

args = sys.argv[sys.argv.index("--") + 1:]
video, out = os.path.abspath(args[0]), os.path.abspath(args[1])
count = int(args[2]) if len(args) > 2 else 3
os.makedirs(out, exist_ok=True)
scene = bpy.context.scene
clip = bpy.data.movieclips.load(video)
scene.render.resolution_x, scene.render.resolution_y = clip.size[0], clip.size[1]
scene.render.resolution_percentage = 50
se = scene.sequence_editor_create()
strip = se.strips.new_movie("m", video, 1, 1)
scene.frame_start, scene.frame_end = 1, strip.frame_final_duration
scene.render.image_settings.file_format = "JPEG"
base = os.path.splitext(os.path.basename(video))[0]
for i in range(count):
    f = 1 + int((strip.frame_final_duration - 2) * i / max(1, count - 1))
    scene.frame_set(f)
    scene.render.filepath = os.path.join(out, "%s_%03d.jpg" % (base, f))
    bpy.ops.render.render(write_still=True)
    print("FRAME", scene.render.filepath)
