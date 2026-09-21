# GENESIS: Der Kreislauf des Lebens
# Erzeugt die Töne des Spielrahmens über kie.ai: Erzählerstimme des Prologs (ElevenLabs
# Multilingual v2) und das Hauptthema für Vorspann und Menü (Suno).
#
# Aufruf (mit dem Python aus Blender, eine System-Python-Installation gibt es hier nicht):
#   $env:KIE_API_KEY = "..."      # nur als Umgebungsvariable – nie in eine Datei schreiben
#   & "C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe" Tools\Frontend\generate_frontend_audio_kie.py --vo --music
#
# Ergebnis: ArtSource/Generated/Frontend/Audio/*.mp3 und Docs/Audio/Frontend_Audio_Manifest.json
# (Aufgaben-IDs, Stimme, Guthaben vorher/nachher – ohne Schlüssel).
#
# Rechte: Die Erzählerstimme ist eine synthetische Standardstimme aus dem ElevenLabs-Katalog,
# kein Klon einer realen Person. Die Musik ist eine Suno-Generierung; ob sie in einer
# Veröffentlichung verwendet werden darf, hängt von den Nutzungsbedingungen von kie.ai/Suno für
# den verwendeten Tarif ab und ist vor einer Veröffentlichung zu prüfen.

import json
import os
import sys
import time
import urllib.error
import urllib.request

API = "https://api.kie.ai/api/v1"
USER_AGENT = "Mozilla/5.0 (GENESIS build tools)"
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(REPO, "ArtSource", "Generated", "Frontend", "Audio")
MANIFEST = os.path.join(REPO, "Docs", "Audio", "Frontend_Audio_Manifest.json")

# Erzähler: ruhig, tief, tröstend. Bewusst keine Stimme, die einer bekannten Person nachempfunden ist.
NARRATOR = {"id": "nPczCjzI2devNBz1zQrb", "name": "Brian", "character": "Deep, Resonant and Comforting"}
# Zwei Alternativen für den ersten Satz – damit der Game Director hören und wählen kann
CASTING = [
    {"id": "LruHrtVF6PSyGItzMNHS", "name": "Benjamin", "character": "Deep, Warm, Calming"},
    {"id": "hqfrgApggtO1785R4Fsn", "name": "Theodore", "character": "Serene and Grounded"},
]

# Die Sätze des Prologs – dieselben wie im Trailer, ohne die Regieanweisungen in Klammern
LINES = [
    ("VO_Prolog_01a", "Bevor du deinen ersten Atemzug nahmst …"),
    ("VO_Prolog_01b", "… hatte deine Geschichte bereits begonnen."),
    ("VO_Prolog_02", "Du wirst lieben."),
    ("VO_Prolog_03", "Du wirst verlieren."),
    ("VO_Prolog_04", "Du wirst Entscheidungen treffen …"),
    ("VO_Prolog_05", "… deren Folgen du vielleicht erst Jahrzehnte später verstehst."),
    ("VO_Prolog_06", "Doch kein Weg ist falsch."),
    ("VO_Prolog_07", "Jeder Weg hinterlässt Spuren."),
    ("VO_Prolog_08", "Am Ende bleiben nicht die Jahre."),
    ("VO_Prolog_09", "Es bleiben die Augenblicke."),
]

MUSIC = {
    "asset": "MX_Frontend_Theme",
    "title": "Genesis - Der Kreislauf des Lebens",
    "style": ("cinematic ambient orchestral film score, intimate felt piano, warm legato string pads, "
              "soft celesta and harp, slow 62 bpm, tender, hopeful, a sense of wonder at the beginning of life, "
              "gentle low heartbeat-like pulse, slowly building, delicate, spacious reverb, no drums"),
    "negativeTags": "vocals, singing, choir lyrics, drums, trap, EDM, rock, electric guitar, dubstep",
    "model": "V5",
}


def request(method, path, body=None):
    key = os.environ.get("KIE_API_KEY")
    if not key:
        sys.exit("KIE_API_KEY ist nicht gesetzt.")
    data = json.dumps(body).encode("utf-8") if body is not None else None
    for attempt in range(6):
        req = urllib.request.Request(API + path, data=data, method=method, headers={
            "Authorization": "Bearer " + key, "Content-Type": "application/json", "User-Agent": USER_AGENT})
        try:
            with urllib.request.urlopen(req, timeout=60) as response:
                result = json.loads(response.read().decode("utf-8"))
        except urllib.error.HTTPError as error:
            result = {"code": error.code, "msg": error.read().decode("utf-8", "replace")}
        # kie.ai drosselt Serien von Aufträgen – dann warten statt abbrechen
        if result.get("code") != 429:
            return result
        time.sleep(5 * (attempt + 1))
    return result


def credit():
    return request("GET", "/chat/credit").get("data")


def download(url, path):
    # Der Dateiserver lehnt Anfragen ohne User-Agent ab (403)
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=120) as response, open(path, "wb") as file:
        file.write(response.read())
    return os.path.getsize(path)


def tts(voice, text, previous_text="", next_text=""):
    body = {"model": "elevenlabs/text-to-speech-multilingual-v2", "input": {
        "text": text, "voice": voice,
        # Etwas mehr Stabilität als üblich: ein Erzähler, kein Schauspieler, der sich in jedem Satz neu erfindet
        "stability": 0.55, "similarity_boost": 0.78, "style": 0.18, "speed": 0.9,
        "language_code": "de",
        "previous_text": previous_text, "next_text": next_text}}
    result = request("POST", "/jobs/createTask", body)
    task = (result.get("data") or {}).get("taskId")
    if not task:
        raise RuntimeError("TTS-Auftrag abgelehnt: %s" % json.dumps(result, ensure_ascii=False))
    return task


def wait_job(task, timeout=240):
    started = time.time()
    while time.time() - started < timeout:
        info = request("GET", "/jobs/recordInfo?taskId=" + task).get("data") or {}
        state = info.get("state")
        if state == "success":
            return json.loads(info["resultJson"])["resultUrls"][0]
        if state in ("fail", "failed"):
            raise RuntimeError("Auftrag %s fehlgeschlagen: %s" % (task, info.get("failMsg")))
        time.sleep(3)
    raise RuntimeError("Auftrag %s nicht rechtzeitig fertig" % task)


def generate_vo(manifest):
    entries = []
    jobs = []
    for index, (asset, text) in enumerate(LINES):
        previous_text = LINES[index - 1][1] if index > 0 else ""
        next_text = LINES[index + 1][1] if index + 1 < len(LINES) else ""
        jobs.append((asset, text, NARRATOR, tts(NARRATOR["id"], text, previous_text, next_text)))
        time.sleep(1.5)
    for alternative in CASTING:
        asset = "CAST_%s_VO_Prolog_01a" % alternative["name"]
        jobs.append((asset, LINES[0][1], alternative, tts(alternative["id"], LINES[0][1], "", LINES[1][1])))
        time.sleep(1.5)

    for asset, text, voice, task in jobs:
        url = wait_job(task)
        path = os.path.join(OUT, asset + ".mp3")
        size = download(url, path)
        print("GENESIS: Stimme %-32s %-9s %6d Bytes" % (asset, voice["name"], size))
        entries.append({"asset": asset, "text": text, "voice": voice, "task": task, "file": os.path.relpath(path, REPO)})
    manifest["vo"] = entries


def generate_music(manifest):
    body = {
        "customMode": True, "instrumental": True, "model": MUSIC["model"],
        "title": MUSIC["title"], "style": MUSIC["style"], "negativeTags": MUSIC["negativeTags"],
        # Pflichtfeld der Schnittstelle; das Ergebnis wird hier abgefragt, nicht zugestellt
        "callBackUrl": "https://example.invalid/genesis-callback",
    }
    result = request("POST", "/generate", body)
    task = (result.get("data") or {}).get("taskId")
    if not task:
        raise RuntimeError("Suno-Auftrag abgelehnt: %s" % json.dumps(result, ensure_ascii=False))
    print("GENESIS: Musik-Auftrag", task)

    started = time.time()
    tracks = None
    while time.time() - started < 600:
        info = request("GET", "/generate/record-info?taskId=" + task).get("data") or {}
        status = info.get("status")
        if status == "SUCCESS":
            tracks = (info.get("response") or {}).get("sunoData") or []
            break
        if status and ("FAIL" in status or "ERROR" in status):
            raise RuntimeError("Suno fehlgeschlagen: %s %s" % (status, info.get("errorMessage")))
        time.sleep(8)
    if not tracks:
        raise RuntimeError("Suno nicht rechtzeitig fertig (Auftrag %s)" % task)

    entries = []
    for index, track in enumerate(tracks):
        # Suno liefert zwei Fassungen. Beide werden behalten; die erste wird Menümusik,
        # die zweite liegt als Alternative daneben.
        asset = MUSIC["asset"] if index == 0 else "%s_Alt%d" % (MUSIC["asset"], index)
        path = os.path.join(OUT, asset + ".mp3")
        size = download(track["audioUrl"], path)
        print("GENESIS: Musik %-24s %6.1f s %8d Bytes" % (asset, float(track.get("duration") or 0), size))
        entries.append({"asset": asset, "title": track.get("title"), "duration": track.get("duration"),
                        "suno_id": track.get("id"), "tags": track.get("tags"), "file": os.path.relpath(path, REPO)})
    manifest["music"] = {"task": task, "model": MUSIC["model"], "style": MUSIC["style"],
                         "negativeTags": MUSIC["negativeTags"], "tracks": entries}


def main():
    args = sys.argv[1:]
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(os.path.dirname(MANIFEST), exist_ok=True)
    manifest = {}
    if os.path.exists(MANIFEST):
        with open(MANIFEST, encoding="utf-8") as file:
            manifest = json.load(file)

    before = credit()
    print("GENESIS: Guthaben vorher", before)
    if "--vo" in args:
        generate_vo(manifest)
    if "--music" in args:
        generate_music(manifest)
    after = credit()
    print("GENESIS: Guthaben nachher", after, "– verbraucht", round(float(before) - float(after), 2))

    manifest.setdefault("runs", []).append({"time": time.strftime("%Y-%m-%d %H:%M:%S"), "args": args,
                                            "credits_before": before, "credits_after": after})
    with open(MANIFEST, "w", encoding="utf-8") as file:
        json.dump(manifest, file, ensure_ascii=False, indent=2)


if __name__ == "__main__":
    main()
