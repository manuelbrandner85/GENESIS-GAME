# GENESIS Trailer - Voice-over ueber kie.ai (Google Gemini 3.1 Flash TTS), Takes + automatische Pruefung.
#
# Status der Stimmen: PLACEHOLDER (KI-Stimmen, keine Imitation realer Personen). Finale Freigabe durch den Game Director.
# Hinweis: ElevenLabs ueber kie.ai lieferte am 17.09.2026 nur "Internal Error" (0 Credits verbraucht) -> Gemini TTS.
#
# Aufruf (API-Schluessel NIE ins Repo schreiben):
#   $env:KIE_API_KEY = "..."
#   & "C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe" Tools\Trailer\Audio\generate_vo_kie.py [--only VO_01a,VO_14] [--takes A,B]
# Ausgabe:
#   Genesis/Saved/TrailerAudio/VO_Takes/<VO>_TAKE_<X>.wav   alle Takes
#   Genesis/Saved/TrailerAudio/VO/<VO>.wav                    gewaehlter Take (vom Mix verwendet)
#   Docs/Trailer/VO_Takes.json                                Messwerte + Auswahl

import argparse
import json
import os
import shutil
import sys
import time
import urllib.request
import wave

import numpy as np

API = "https://api.kie.ai/api/v1"
MODEL = "google/gemini-3-1-flash-tts"
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
AUDIO = os.path.join(REPO, "Genesis", "Saved", "TrailerAudio")
TAKES_DIR = os.path.join(AUDIO, "VO_Takes")
SELECTED_DIR = os.path.join(AUDIO, "VO")
REPORT = os.path.join(REPO, "Docs", "Trailer", "VO_Takes.json")
JOBS = os.path.join(TAKES_DIR, "jobs.json")   # Auftrags-IDs sofort sichern -> kein doppeltes Bezahlen bei Abbruch
USER_AGENT = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) GENESIS-Trailer-Tools/1.0"
DATA = json.load(open(os.path.join(REPO, "Docs", "Trailer", "ShotList.json"), encoding="utf-8"))

SCENE = ("Cinematic reveal trailer for a video game about the whole circle of life: conception, birth, childhood, love, "
         "loss, old age, death, afterlife and rebirth. Recorded close to a high-end microphone in a quiet studio.")

# Casting je Sprecherrolle und Take. Erzaehler: TAKE_A und TAKE_B mit unterschiedlichen Stimmen fuer das Casting.
CASTING = {
    "narrator": {
        "context": ("German voice-over. Calm, warm, deep and human. Tender, a little mysterious, never pathetic. "
                    "Not a loud movie-trailer announcer. Unhurried natural pace with small breaths and short, meaningful pauses."),
        "A": {"voice_name": "Algieba", "style": "Empathetic", "pace": "Natural",
              "audio_profile": "A calm, warm, deep-voiced German narrator in his fifties who has lived a full life and speaks gently, almost like sharing a secret."},
        "B": {"voice_name": "Charon", "style": "Empathetic", "pace": "Natural",
              "audio_profile": "A calm, grounded, deep German narrator with a soft, warm timbre, speaking slowly and intimately to one listener."},
    },
    "mother": {
        "context": "German. A mother in labour, exhausted but loving, whispering to her unborn child.",
        "A": {"voice_name": "Sulafat", "style": "Whisper", "pace": "Natural",
              "audio_profile": "A young German mother, breathless and tender, whispering reassurance."},
        "B": {"voice_name": "Vindemiatrix", "style": "Whisper", "pace": "Natural",
              "audio_profile": "A young German mother, breathless and tender, whispering reassurance."},
    },
    "daughter": {
        "context": "German. A grown daughter at the deathbed of her parent, quietly holding back tears.",
        "A": {"voice_name": "Vindemiatrix", "style": "Whisper", "pace": "Natural",
              "audio_profile": "A German woman in her forties, gentle and fragile, whispering through tears."},
        "B": {"voice_name": "Gacrux", "style": "Whisper", "pace": "Natural",
              "audio_profile": "A German woman in her forties, gentle and fragile, whispering through tears."},
    },
    "child": {
        "context": "German. A small child, curious and slightly puzzled, whispering a single question in the dark.",
        "A": {"voice_name": "Leda", "style": "Whisper", "pace": "Natural",
              "audio_profile": "A six-year-old German child with a very light, innocent voice, whispering softly and curiously."},
        "B": {"voice_name": "Puck", "style": "Whisper", "pace": "Natural",
              "audio_profile": "A six-year-old German child with a very light, innocent voice, whispering softly and curiously."},
    },
}

# Regie-Tags je Zeile (Gemini-Tonangaben). Standard: [softly]
DIRECTION = {
    "VO_01a": "[softly]", "VO_01b": "[softly]", "VO_M1": "[whispering, breathless]", "VO_02": "[warmly]",
    "VO_03": "[quietly, with sadness]", "VO_04": "[thoughtfully]", "VO_05": "[thoughtfully]", "VO_06": "[gently]",
    "VO_07": "[softly]", "VO_08": "[slowly, tenderly]", "VO_09": "[tenderly]", "VO_D1": "[whispering, holding back tears]",
    "VO_10": "[softly, mysterious]", "VO_11": "[with quiet wonder]", "VO_12": "[softly]", "VO_13": "[with quiet awe]",
    "VO_14": "[whispering, curious]",
}


def request(method, path, body=None):
    key = os.environ.get("KIE_API_KEY")
    if not key:
        sys.exit("KIE_API_KEY ist nicht gesetzt.")
    data = json.dumps(body).encode("utf-8") if body is not None else None
    for attempt in range(8):
        req = urllib.request.Request(API + path, data=data, method=method, headers={
            "Authorization": "Bearer " + key, "Content-Type": "application/json", "User-Agent": USER_AGENT})
        with urllib.request.urlopen(req, timeout=60) as r:
            res = json.loads(r.read().decode("utf-8"))
        if res.get("code") != 429:
            return res
        wait = 10 * (attempt + 1)
        print("    Ratenlimit - warte", wait, "s")
        time.sleep(wait)
    return res


def download(url, path):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=120) as r, open(path, "wb") as f:
        f.write(r.read())


def slot_seconds(lines, i):
    """Verfuegbare Zeit bis zur naechsten Zeile bzw. Trailerende."""
    nxt = lines[i + 1]["start"] if i + 1 < len(lines) else DATA["duration"]
    return nxt - lines[i]["start"]


def analyze(path):
    with wave.open(path) as w:
        sr, n, ch = w.getframerate(), w.getnframes(), w.getnchannels()
        x = np.frombuffer(w.readframes(n), dtype=np.int16).astype(np.float32).reshape(-1, ch).mean(axis=1) / 32768.0
    env = np.array([np.sqrt(np.mean(x[i:i + sr // 100] ** 2)) for i in range(0, len(x), sr // 100)])
    voiced = np.where(env > 0.01)[0]
    speech = (voiced[-1] - voiced[0] + 1) / 100.0 if len(voiced) else 0.0
    f0s = []
    fr = int(0.04 * sr)
    for i in range(0, len(x) - fr, fr):
        s = x[i:i + fr]
        if np.sqrt(np.mean(s ** 2)) < 0.03:
            continue
        s = s - s.mean()
        ac = np.correlate(s, s, "full")[fr - 1:]
        lo, hi = int(sr / 500), int(sr / 60)
        k = lo + int(np.argmax(ac[lo:hi]))
        if ac[k] > 0.3 * ac[0]:
            f0s.append(sr / k)
    return {"file_seconds": round(n / sr, 2), "speech_seconds": round(speech, 2),
            "median_f0_hz": round(float(np.median(f0s)), 1) if f0s else None,
            "peak": round(float(np.max(np.abs(x))), 3), "clipping": bool(np.max(np.abs(x)) > 0.99)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--only", default="")
    parser.add_argument("--takes", default="A,B")
    parser.add_argument("--redo", action="store_true", help="Takes neu erzeugen, auch wenn schon vorhanden")
    args = parser.parse_args()
    only = set(filter(None, args.only.split(",")))
    takes = [t.strip() for t in args.takes.split(",") if t.strip()]
    os.makedirs(TAKES_DIR, exist_ok=True)
    os.makedirs(SELECTED_DIR, exist_ok=True)

    credit_before = request("GET", "/chat/credit")["data"]
    print("Guthaben vorher:", credit_before)

    lines = DATA["voiceover"]
    jobs = json.load(open(JOBS, encoding="utf-8")) if os.path.exists(JOBS) else []
    known = {(j["id"], j["take"]): j for j in jobs if j.get("state") != "fail"}

    def save_jobs():
        json.dump(jobs, open(JOBS, "w", encoding="utf-8"), ensure_ascii=False, indent=2)

    for i, line in enumerate(lines):
        if only and line["id"] not in only:
            continue
        role = CASTING[line["speaker"]]
        for take in takes:
            if (line["id"], take) in known and not args.redo:
                continue
            cast = dict(role[take])
            cast["speaker_id"] = "Speaker 1"
            cast["accent"] = "Neutral"
            text = DIRECTION.get(line["id"], "[softly]") + " " + line["text"]
            body = {"model": MODEL, "input": {
                "temperature": 0.9, "scene": SCENE,
                "sample_context": role["context"],
                "speakers": [cast], "dialogue_turns": [{"speaker_id": "Speaker 1", "text": text}]}}
            res = request("POST", "/jobs/createTask", body)
            if res.get("code") != 200:
                print("  Auftrag abgelehnt", line["id"], take, res)
                continue
            jobs = [j for j in jobs if (j["id"], j["take"]) != (line["id"], take)]
            jobs.append({"id": line["id"], "take": take, "voice": cast["voice_name"], "task": res["data"]["taskId"],
                         "slot": round(slot_seconds(lines, i), 2), "text": text, "state": "submitted"})
            save_jobs()
            print("  Auftrag", line["id"], "TAKE_" + take, cast["voice_name"])
            time.sleep(3.5)

    pending = [j for j in jobs if j.get("state") == "submitted" or (j.get("state") == "success" and not os.path.exists(os.path.join(REPO, j.get("file", "-"))))]
    deadline = time.time() + 900
    while pending and time.time() < deadline:
        time.sleep(4)
        for job in list(pending):
            info = request("GET", "/jobs/recordInfo?taskId=" + job["task"])["data"]
            time.sleep(0.4)
            if info["state"] == "success":
                url = json.loads(info["resultJson"])["resultUrls"][0]
                path = os.path.join(TAKES_DIR, "{}_TAKE_{}.wav".format(job["id"], job["take"]))
                download(url, path)
                job.update(analyze(path), state="success", credits=info.get("creditsConsumed"), file=os.path.relpath(path, REPO))
                pending.remove(job)
                save_jobs()
                print("  fertig", job["id"], "TAKE_" + job["take"], job["speech_seconds"], "s /", job["slot"], "s Platz, f0", job["median_f0_hz"])
            elif info["state"] == "fail":
                job.update(state="fail", error=info.get("failMsg"))
                pending.remove(job)
                save_jobs()
                print("  FEHLER", job["id"], "TAKE_" + job["take"], info.get("failMsg"))

    # Auswahl: bevorzugt TAKE_A (einheitliche Erzaehlerstimme); B nur, wenn A fehlt, clippt oder nicht in den Platz passt
    previous = {}
    if os.path.exists(REPORT):
        for row in json.load(open(REPORT, encoding="utf-8"))["lines"]:
            previous[(row["id"], row["take"])] = row
    for job in jobs:
        if job.get("state") == "success":
            previous[(job["id"], job["take"])] = job
    rows = sorted(previous.values(), key=lambda r: ([l["id"] for l in lines].index(r["id"]), r["take"]))
    selection = {}
    for line in lines:
        cands = [r for r in rows if r["id"] == line["id"] and r.get("state") == "success"]
        def score(r):
            fits = r["speech_seconds"] <= r["slot"] - 0.2
            return (not r["clipping"], fits, r["take"] == "A", -r["speech_seconds"])
        if cands:
            best = max(cands, key=score)
            selection[line["id"]] = best["take"]
            shutil.copyfile(os.path.join(REPO, best["file"]), os.path.join(SELECTED_DIR, line["id"] + ".wav"))
    credit_after = request("GET", "/chat/credit")["data"]
    json.dump({"model": MODEL, "status": "PLACEHOLDER", "credits_before": credit_before, "credits_after": credit_after,
               "selection": selection, "lines": rows}, open(REPORT, "w", encoding="utf-8"), ensure_ascii=False, indent=2)
    print("Auswahl:", selection)
    print("Guthaben nachher:", credit_after, "(verbraucht {:.2f})".format(credit_before - credit_after))
    failed = [j for j in jobs if j.get("state") != "success" and (not only or j["id"] in only)]
    if failed:
        sys.exit("{} Takes fehlgeschlagen".format(len(failed)))


if __name__ == "__main__":
    main()
