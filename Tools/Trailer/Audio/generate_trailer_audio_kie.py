# GENESIS Trailer (In-Engine-Fassung) - Musik (Suno) und Erzaehler (Gemini 3.1 Flash TTS) ueber kie.ai.
#
# Aufruf (Blender-Python; API-Schluessel nur als Umgebungsvariable, nie in eine Datei):
#   $env:KIE_API_KEY = "..."
#   & "C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe" Tools\Trailer\Audio\generate_trailer_audio_kie.py [--music] [--vo]
# Ergebnis: ArtSource/Generated/Trailer/Audio/*.mp3|wav und Docs/Trailer/Trailer_Audio_Manifest.json (ohne Schluessel)
#
# Erzaehler = dieselbe Stimme wie im Prolog des Spiels (Gemini "Algieba"), damit Trailer und Spiel zusammengehoeren.
# Rechte: synthetische Standardstimme ohne Vorbild einer realen Person; Suno-Musik - Nutzungsbedingungen des Tarifs
# vor einer Veroeffentlichung pruefen.

import json
import os
import sys
import time
import urllib.error
import urllib.request

API = "https://api.kie.ai/api/v1"
USER_AGENT = "Mozilla/5.0 (GENESIS trailer tools)"
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
OUT = os.path.join(REPO, "ArtSource", "Generated", "Trailer", "Audio")
MANIFEST = os.path.join(REPO, "Docs", "Trailer", "Trailer_Audio_Manifest.json")

MUSIC_JOBS = [
    {"asset": "MX_Trailer_Epic",
     "title": "Genesis - Jede Entscheidung hinterlaesst ein Echo",
     "style": ("epic emotional cinematic movie trailer score, opens with a lone felt piano motif over a deep slow heartbeat pulse, "
               "long hush, then legato strings and low brass slowly rise, ticking ostinato, tension builds in waves, "
               "huge orchestral climax with wordless choir, taiko and big braams, then a sudden hard stop into silence, "
               "ends on one soft piano note, 70 bpm, awe, wonder, fragility of life, original and intimate yet vast"),
     "negativeTags": "lyrics, singing words, rap, EDM, dubstep, trap, rock, electric guitar, cheerful, pop",
     "model": "V5"},
    {"asset": "MX_Trailer_Hybrid",
     "title": "Genesis - Der Kreislauf des Lebens (Trailer)",
     "style": ("modern hybrid orchestral trailer music, intimate music-box and piano theme, sub bass heartbeat, "
               "slowly swelling strings, rising risers and pulses, emotional build, massive drums and brass at the peak, "
               "wordless ethereal female choir, abrupt cut to silence, gentle outro, cinematic, dramatic, hopeful, 72 bpm"),
     "negativeTags": "lyrics, singing words, rap, EDM, dubstep, trap, rock, electric guitar, cheerful, pop",
     "model": "V5"},
]

NARRATOR = {"voice_name": "Algieba", "style": "Empathetic", "pace": "Natural", "accent": "Neutral",
            "audio_profile": "A calm, warm, deep-voiced German narrator in his fifties who has lived a full life and speaks gently, "
                             "almost like sharing a secret. Cinematic trailer delivery, intimate, never shouting."}
SCENE = ("Cinematic reveal trailer for a video game about the whole circle of life. Recorded close to a high-end "
         "microphone in a quiet studio. Long pauses, weight in every word.")
CONTEXT = ("German voice-over. Calm, warm, deep and human, tender and a little mysterious. "
           "Not a loud announcer. Unhurried, with small breaths.")

# Neue Saetze der In-Engine-Fassung (die uebrigen stammen aus den Takes vom 17.09.)
VO_LINES = [
    ("VO_G_03", "[softly] Millionen machen sich auf den Weg."),
    ("VO_G_04", "[quietly, with weight] Nur einer kommt an."),
    ("VO_G_05", "[with quiet wonder] Aus einer Zelle werden zwei."),
    ("VO_G_06", "[tenderly] Aus zweien … ein Mensch."),
    ("VO_G_09", "[slowly] Und jede Entscheidung …"),
    ("VO_G_10", "[softly, with gravity] … hinterlässt ein Echo."),
]


def request(method, path, body=None):
    key = os.environ.get("KIE_API_KEY")
    if not key:
        sys.exit("KIE_API_KEY ist nicht gesetzt.")
    data = json.dumps(body).encode("utf-8") if body is not None else None
    for attempt in range(8):
        req = urllib.request.Request(API + path, data=data, method=method, headers={
            "Authorization": "Bearer " + key, "Content-Type": "application/json", "User-Agent": USER_AGENT})
        try:
            with urllib.request.urlopen(req, timeout=60) as response:
                result = json.loads(response.read().decode("utf-8"))
        except urllib.error.HTTPError as error:
            result = {"code": error.code, "msg": error.read().decode("utf-8", "replace")}
        if result.get("code") != 429:
            return result
        time.sleep(8 * (attempt + 1))
    return result


def download(url, path):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=180) as response, open(path, "wb") as file:
        file.write(response.read())
    return os.path.getsize(path)


def music(manifest):
    tasks = []
    for job in MUSIC_JOBS:
        body = {"customMode": True, "instrumental": True, "model": job["model"], "title": job["title"],
                "style": job["style"], "negativeTags": job["negativeTags"],
                "callBackUrl": "https://example.invalid/genesis-callback"}
        result = request("POST", "/generate", body)
        task = (result.get("data") or {}).get("taskId")
        if not task:
            raise RuntimeError("Suno-Auftrag abgelehnt: %s" % json.dumps(result, ensure_ascii=False))
        print("GENESIS_AUDIO Musik-Auftrag", job["asset"], task)
        tasks.append((job, task))
        time.sleep(4)
    entries = manifest.get("music", [])
    for job, task in tasks:
        started, tracks = time.time(), None
        while time.time() - started < 900:
            info = request("GET", "/generate/record-info?taskId=" + task).get("data") or {}
            status = info.get("status") or ""
            if status == "SUCCESS":
                tracks = (info.get("response") or {}).get("sunoData") or []
                break
            if "FAIL" in status or "ERROR" in status:
                raise RuntimeError("Suno fehlgeschlagen: %s %s" % (status, info.get("errorMessage")))
            time.sleep(10)
        if not tracks:
            raise RuntimeError("Suno nicht rechtzeitig fertig: " + task)
        for i, track in enumerate(tracks):
            asset = "%s_%s" % (job["asset"], "ABCD"[i])
            path = os.path.join(OUT, asset + ".mp3")
            size = download(track["audioUrl"], path)
            print("GENESIS_AUDIO Musik %-22s %6.1f s %9d Bytes" % (asset, float(track.get("duration") or 0), size))
            entries.append({"asset": asset, "task": task, "duration": track.get("duration"), "suno_id": track.get("id"),
                            "style": job["style"], "file": os.path.relpath(path, REPO)})
    manifest["music"] = entries


def voice(manifest):
    tasks = []
    for asset, text in VO_LINES:
        speaker = dict(NARRATOR, speaker_id="Speaker 1")
        body = {"model": "google/gemini-3-1-flash-tts", "input": {
            "temperature": 0.85, "scene": SCENE, "sample_context": CONTEXT, "speakers": [speaker],
            "dialogue_turns": [{"speaker_id": "Speaker 1", "text": text}]}}
        for take in ("A", "B"):
            result = request("POST", "/jobs/createTask", body)
            task = (result.get("data") or {}).get("taskId")
            if not task:
                raise RuntimeError("TTS abgelehnt: %s" % json.dumps(result, ensure_ascii=False))
            tasks.append((asset, take, text, task))
            time.sleep(3.5)
    entries = manifest.get("vo", [])
    for asset, take, text, task in tasks:
        started = time.time()
        while True:
            info = request("GET", "/jobs/recordInfo?taskId=" + task).get("data") or {}
            if info.get("state") == "success":
                url = json.loads(info["resultJson"])["resultUrls"][0]
                break
            if info.get("state") == "fail" or time.time() - started > 300:
                raise RuntimeError("TTS %s fehlgeschlagen: %s" % (asset, info.get("failMsg")))
            time.sleep(3)
        path = os.path.join(OUT, "%s_TAKE_%s.wav" % (asset, take))
        size = download(url, path)
        print("GENESIS_AUDIO Stimme %-10s TAKE_%s %8d Bytes" % (asset, take, size))
        entries.append({"asset": asset, "take": take, "text": text, "voice": NARRATOR["voice_name"], "task": task,
                        "file": os.path.relpath(path, REPO)})
    manifest["vo"] = entries


def main():
    os.makedirs(OUT, exist_ok=True)
    manifest = json.load(open(MANIFEST, encoding="utf-8")) if os.path.exists(MANIFEST) else {}
    before = request("GET", "/chat/credit").get("data")
    print("GENESIS_AUDIO Guthaben vorher", before)
    if "--music" in sys.argv:
        music(manifest)
    if "--vo" in sys.argv:
        voice(manifest)
    after = request("GET", "/chat/credit").get("data")
    manifest["credits"] = {"before": before, "after": after}
    json.dump(manifest, open(MANIFEST, "w", encoding="utf-8"), ensure_ascii=False, indent=2)
    print("GENESIS_AUDIO Guthaben nachher", after, "verbraucht", round(before - after, 2))


main()
