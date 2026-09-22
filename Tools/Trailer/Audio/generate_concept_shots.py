# GENESIS Trailer - KONZEPTBILDER der Lebensphasen (noch nicht spielbar): Schluesselbild (Nano Banana Pro, mit
# Referenz der Hauptfigur) -> Video (Kling 3.0 Omni). Idempotent: vorhandene Dateien werden uebersprungen.
#
# Aufruf: $env:KIE_API_KEY="..."; & "<Blender-Python>" Tools\Trailer\Audio\generate_concept_shots.py [--only SH_A06,SH_A07] [--keyframes-only]
# Ergebnis: ArtSource/Generated/Trailer/Concept/KF_<id>.png, V_<id>.mp4, Docs/Trailer/Concept_Shots_Manifest.json

import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(__file__))
import kie_media as k

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
OUT = os.path.join(REPO, "ArtSource", "Generated", "Trailer", "Concept")
MANIFEST = os.path.join(REPO, "Docs", "Trailer", "Concept_Shots_Manifest.json")
REF_SHEET = os.path.join(OUT, "REF_Protagonist_Ages.png")

LOOK = ("Photorealistic cinematic film still, shot on 35mm film with an anamorphic lens, natural film grain, subtle halation, "
        "realistic skin with pores, motivated practical light only, restrained natural color grade, no text, no watermark, no logo. ")
HERO = "the man from the reference image (same face, grey-green eyes, small scar through the left eyebrow)"

# id, Stufe (H = Video 1080p/4 s, L = Video 720p/3 s, S = nur Standbild -> Kamerafahrt im Schnitt), Referenz noetig, Bild-Prompt, Bewegungs-Prompt
SHOTS = [
    ("SH_A01_GrassRun", "H", True, None, None),   # Test vom 22.09. (bereits erzeugt)
    ("SH_A02_LiftedUp", "L", True,
     "A young father lifts the 6-year-old boy from the reference image high into the evening sun in a garden, backlit, the boy laughs with arms spread, lens flare from the low sun, 28mm lens, warm late summer light.",
     "The father lifts the boy up into the sunlight, slow upward crane movement, the boy's hair moves in the wind, gentle and joyful."),
    ("SH_A03_EmptyBasket", "S", False,
     "A worn empty dog basket on a wooden hallway floor in grey morning light, a faded red dog collar lies in it, a small child's hand rests on the blanket, quiet grief, 50mm lens, static composition, muted colors.",
     "Almost still: the small hand slowly strokes the blanket once, dust drifts in the grey light, static camera."),
    ("SH_A04_RainWindow", "L", True,
     "The 6-year-old boy from the reference image sits alone at a rain-streaked window, looking out, not crying, only a still gaze, raindrops on the glass in the foreground, cool overcast light, 85mm lens, shallow depth of field.",
     "Raindrops run down the glass, the boy breathes slowly and blinks once, very slow push-in."),
    ("SH_A05_RooftopFriends", "S", True,
     "The 16-year-old teenager from the reference image laughs with three friends sitting on a city rooftop at night, city lights bokeh behind them, handheld feeling, 28mm lens, cool night tones with warm skin from a nearby lamp.",
     "Handheld camera slowly circles the laughing group, city lights twinkle, natural movement."),
    ("SH_A06_FirstLove", "S", False,
     "Extreme close-up of two teenage hands on a wooden park bench at dusk, the fingers almost touch for the first time, warm street lamp light, 100mm macro lens, very shallow depth of field.",
     "The two hands slowly move closer and the fingertips touch hesitantly, tiny movement, static camera."),
    ("SH_A07_Crossroads", "L", True,
     "Wide shot: " + HERO + " at about 20 years old stands alone at a fork of two country roads in thick morning fog, seen from behind at a distance, one road slightly brighter, 24mm lens, desaturated cool fog.",
     "Fog drifts slowly, the young man hesitates and then takes a first step onto the left road, slow crane up."),
    ("SH_A08_HospitalWait", "S", True,
     HERO + " at about 35 years old waits alone on a plastic chair in a night hospital corridor under cold fluorescent light, elbows on knees, looking at a closed door, 35mm lens.",
     "He rubs his hands nervously and looks up at the door, the fluorescent light flickers slightly, slow dolly-in."),
    ("SH_A09_WeddingRing", "S", False,
     "Macro close-up: a simple gold wedding ring is slid onto a woman's finger by a man's hand, soft daylight, 100mm macro, creamy bokeh, emotional and intimate.",
     "The ring slides slowly onto the finger, the hands tremble slightly, static macro camera."),
    ("SH_A10_BabyFinger", "S", False,
     "Macro close-up: a newborn baby's tiny hand grips an adult man's index finger in a softly lit nursery, 100mm macro, warm morning light.",
     "The tiny fingers tighten around the finger, gentle breathing motion, static camera."),
    ("SH_A11_EraMedieval", "S", False,
     "A medieval village in the fog at dusk, a figure in a wool cloak carries a burning torch along a muddy path between thatched houses, smoke, 35mm lens, painterly realistic, historically accurate.",
     "The torch bearer walks slowly through the fog, the flame flickers, slow dolly forward."),
    ("SH_A12_EraFuture", "S", False,
     "A believable future city at dawn seen from a high balcony, elegant towers with vertical gardens, soft haze, calm, not neon, not sci-fi kitsch, 35mm lens, realistic architecture photography.",
     "Slow pull back from the balcony, morning haze moves, a few birds cross the sky."),
    ("SH_A13_OldBench", "H", True,
     "The 78-year-old man from the reference image sits alone on a weathered wooden bench on a cliff above the sea in late afternoon light, wind in his thin grey hair and coat, slightly bent posture, 50mm lens, calm, melancholic, beautiful.",
     "Wind moves his hair and coat, the sea glitters, he breathes slowly, very slow dolly-in, long contemplative shot."),
    ("SH_A14_OldPhoto", "S", True,
     "Close-up of wrinkled old hands holding a faded old photograph of the 6-year-old boy from the reference image laughing with a golden dog in tall grass, wind at the edge of the photo, late afternoon light, 85mm lens.",
     "The old thumb slowly strokes over the photo, the paper trembles in the wind, static camera."),
    ("SH_A15_LastBreath", "H", True,
     "Extreme close-up of the eye of the very old man from the reference image lying in bed, soft window light, a tiny reflection of family members in the pupil, wet eye, fine wrinkles, 100mm macro, peaceful.",
     "The old eye slowly closes for the last time, one final breath, total stillness, static macro camera."),
    ("SH_A16_Ghost", "L", True,
     "A quiet bedroom at dusk, the old man from the reference image lies still in bed surrounded by his grieving family, and a faint translucent luminous version of himself stands beside the bed looking at them, desaturated colors, soft light, not horror, 28mm lens.",
     "The translucent figure slowly raises a hand towards his daughter, the family moves in slow motion, camera slowly orbits."),
    ("SH_A17_Afterlife", "H", False,
     "An immense cosmic landscape beyond life: glowing paths of golden light lead across a dark reflective plain towards an infinite library of light rising into a gold, blue and violet nebula sky, tiny luminous human souls walk the paths, religiously neutral, awe-inspiring, ultra wide 24mm.",
     "Slow majestic crane up revealing the endless landscape, the light paths shimmer, souls drift forward, nebula clouds move slowly."),
    ("SH_A18_DogReturns", "S", False,
     "A scruffy golden mixed-breed dog runs joyfully out of warm white light along a glowing golden path towards the camera, backlit halo, dreamlike but photorealistic, 50mm lens.",
     "The dog runs towards the camera with wagging tail, slow motion, light particles in the air."),
    ("SH_A19_NewBaby", "S", False,
     "A newborn baby in a clay-walled room lit by a single oil lamp, a young mother of another culture gently lays her hand on the baby's forehead, warm flickering light, 85mm lens, timeless, respectful.",
     "The oil lamp flickers, the mother's hand rests gently, the baby breathes and moves its fingers, slow push-in."),
    ("SH_A20_HandsPlanet", "S", False,
     "Two luminous, softly glowing human hands made of warm light hold a small newborn planet between them in deep space, the planet shows oceans, swirling clouds and a thin blue atmosphere, stars and a faint golden thread around it, photorealistic, awe.",
     "The hands gently turn the small planet, clouds swirl on it, city lights begin to sparkle on its night side, slow orbit of the camera."),
]


def main():
    only = set()
    if "--only" in sys.argv:
        only = set(sys.argv[sys.argv.index("--only") + 1].split(","))
    manifest = json.load(open(MANIFEST, encoding="utf-8")) if os.path.exists(MANIFEST) else {"shots": {}}
    before = k.credit()
    print("GENESIS_CONCEPT Guthaben vorher", before)
    ref_url = None
    pending = []
    for sid, tier, needs_ref, img_prompt, motion in SHOTS:
        if img_prompt is None or (only and sid not in only):
            continue
        kf = os.path.join(OUT, "KF_%s.png" % sid)
        entry = manifest["shots"].setdefault(sid, {"status": "CONCEPT", "tier": tier})
        if not os.path.exists(kf):
            if needs_ref and ref_url is None:
                ref_url = k.upload(REF_SHEET, "REF_Protagonist_Ages.png")
            task = k.create("nano-banana-pro", {"prompt": LOOK + img_prompt, "image_input": [ref_url] if needs_ref else [],
                                                "aspect_ratio": "16:9", "resolution": "2K", "output_format": "png"})
            urls, info = k.wait(task, 900)
            k.download(urls[0], kf)
            entry.update(keyframe_task=task, keyframe_url=urls[0], keyframe_credits=info.get("creditsConsumed"),
                         image_prompt=LOOK + img_prompt)
            print("GENESIS_CONCEPT Schluesselbild", sid, info.get("creditsConsumed"))
            json.dump(manifest, open(MANIFEST, "w", encoding="utf-8"), ensure_ascii=False, indent=2)
        if "--keyframes-only" in sys.argv or tier == "S":
            continue
        video = os.path.join(OUT, "V_%s.mp4" % sid)
        if os.path.exists(video):
            continue
        url = entry.get("keyframe_url") or k.upload(kf, os.path.basename(kf))
        res, dur = ("1080p", 4) if tier == "H" else ("720p", 3)
        task = k.create("kling-3.0-omni/image-to-video", {"prompt": motion + " Photorealistic, cinematic, natural motion, no morphing.",
                                                          "image_urls": [url], "duration": dur, "resolution": res,
                                                          "aspect_ratio": "auto", "audio": False})
        pending.append((sid, task, video, entry, res, dur, motion))
        time.sleep(3)
    for sid, task, video, entry, res, dur, motion in pending:
        urls, info = k.wait(task, 2400)
        k.download(urls[0], video)
        entry.update(video_task=task, video_credits=info.get("creditsConsumed"), resolution=res, duration=dur, motion_prompt=motion)
        print("GENESIS_CONCEPT Video", sid, res, dur, "s", info.get("creditsConsumed"))
        json.dump(manifest, open(MANIFEST, "w", encoding="utf-8"), ensure_ascii=False, indent=2)
    after = k.credit()
    manifest.setdefault("credits_log", []).append({"before": before, "after": after, "time": time.strftime("%Y-%m-%d %H:%M")})
    json.dump(manifest, open(MANIFEST, "w", encoding="utf-8"), ensure_ascii=False, indent=2)
    print("GENESIS_CONCEPT Guthaben nachher", after, "verbraucht", round(before - after, 2))


main()
