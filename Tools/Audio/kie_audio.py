"""GENESIS – Ton über kie.ai: Sprache (ElevenLabs v3), Geräusche und Klangbetten (Suno sounds).

Aufruf (Schlüssel NUR aus der Umgebung, nie in Dateien):
    $env:KIE_API_KEY = '...'; python Tools/Audio/kie_audio.py <auftraege.json> <zielordner> [--dry]

auftraege.json: Liste von {"name": ..., "model": ..., "input": {...}}
  Sprache:   model "elevenlabs/text-to-dialogue-v3", input {"dialogue": [{"text": ..., "voice": ...}], "stability": 0.5, "language_code": "de"}
  Geräusche: model "ai-music-api/sounds", input {"prompt": ..., "model": "V5_5", "sound_loop": true}

Vor und nach dem Lauf wird das Guthaben gemeldet – jeder Auftrag kostet echtes Geld.
Ergebnisse (mp3) laufen nach 14 Tagen bei kie.ai ab; sie werden sofort heruntergeladen.
"""
import json, os, sys, time, urllib.request

KEY = os.environ.get("KIE_API_KEY")
BASE = "https://api.kie.ai/api/v1"
# Der Dateiserver lehnt Anfragen ohne User-Agent ab (403)
USER_AGENT = "Mozilla/5.0 (GENESIS build tools)"


def call(method, path, body=None):
    request = urllib.request.Request(BASE + path, method=method,
                                     data=json.dumps(body).encode() if body else None,
                                     headers={"Authorization": "Bearer " + KEY, "Content-Type": "application/json", "User-Agent": USER_AGENT})
    with urllib.request.urlopen(request, timeout=60) as response:
        return json.loads(response.read().decode())


def credit():
    return call("GET", "/chat/credit").get("data")


def main():
    if not KEY:
        sys.exit("KIE_API_KEY fehlt")
    jobs = json.load(open(sys.argv[1], encoding="utf-8"))
    out = sys.argv[2]
    os.makedirs(out, exist_ok=True)
    if "--dry" in sys.argv:
        print("Guthaben:", credit(), "–", len(jobs), "Aufträge (nicht gesendet)")
        return

    before = credit()
    print("Guthaben vorher:", before)
    tasks = {}
    for job in jobs:
        if any(os.path.exists(os.path.join(out, job["name"] + ext)) for ext in (".mp3", ".wav")):
            print("vorhanden, übersprungen:", job["name"])
            continue
        body = {"model": job["model"], "input": job["input"]}
        result = call("POST", "/jobs/createTask", body)
        print(job["name"], "->", result.get("code"), result.get("msg"))
        if result.get("code") == 200:
            tasks[job["name"]] = result["data"]["taskId"]
        time.sleep(3.5)  # kie.ai drosselt Serien (429 nach ~20 Aufträgen)

    done = {}
    deadline = time.time() + 1200
    while len(done) < len(tasks) and time.time() < deadline:
        time.sleep(8)
        for name, task in tasks.items():
            if name in done:
                continue
            data = call("GET", "/jobs/recordInfo?taskId=" + task).get("data") or {}
            state = (data.get("state") or data.get("status") or "").lower()
            if state == "success":
                result = json.loads(data.get("resultJson") or "{}")
                urls = result.get("resultUrls") or result.get("audio_urls") or []
                for index, url in enumerate(urls):
                    suffix = "" if index == 0 else "_v%d" % (index + 1)
                    # Gemini liefert WAV, ElevenLabs und Suno MP3 – die Endung kommt aus der Adresse
                    extension = ".wav" if url.split("?")[0].lower().endswith(".wav") else ".mp3"
                    path = os.path.join(out, name + suffix + extension)
                    download = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
                    with urllib.request.urlopen(download, timeout=120) as response, open(path, "wb") as file:
                        file.write(response.read())
                    print("fertig:", os.path.basename(path), os.path.getsize(path) // 1024, "KB")
                done[name] = True
            elif state in ("fail", "failed"):
                done[name] = False
                print("FEHLER:", name, data.get("failMsg") or data.get("failCode"))

    after = credit()
    print("Guthaben nachher:", after, "| verbraucht:", round(before - after, 2), "Credits (~%.2f $)" % ((before - after) * 0.005))


if __name__ == "__main__":
    main()
