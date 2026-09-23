# GENESIS: Der Kreislauf des Lebens
#
# Ersetzt im Vorfilm einen Satz des Erzaehlers, ohne den Trailer anzufassen.
#
# Anlass (GENESIS-047, Docs/38): Der Trailer sagt "Millionen machen sich auf den Weg. Nur einer kommt an."
# Das ist biologisch falsch - viele kommen an, eine einzige Zelle verschmilzt (Dubois 2025,
# DOI 10.1038/s44319-025-00670-8). Der Vorfilm bekommt deshalb eine eigene Tonspur: Musik, Geraeusche und
# Stimme aus den Stems des Trailers, nur der eine Satz neu - gleiche Stimme (Gemini "Algieba"), gleiche
# Anweisung wie im Trailer, eine durchgehende Aufnahme.
#
# Schritte (Python von Blender, hat numpy):
#   python vorfilm_stimme.py --pruefen            Stems gegen die Mischung pruefen, alten Satz vermessen
#   python vorfilm_stimme.py --erzeugen           zwei Aufnahmen bei kie.ai (KIE_API_KEY nur in der Umgebung dieses Aufrufs)
#   python vorfilm_stimme.py --mischen A          Aufnahme A einsetzen -> Genesis/Saved/Intro/GENESIS_Vorfilm_Ton.wav
# Danach Tools/Intro/build_vorfilm.py (nimmt die Tonspur, wenn vorhanden) und die Untertitel in
# GenesisBootFlow.cpp auf die ausgegebenen Zeiten setzen.

import json
import os
import sys
import time
import urllib.error
import urllib.request
import wave

import numpy as np

WURZEL = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STEMS = os.path.join(WURZEL, "Genesis", "Saved", "Trailer", "v2", "GENESIS_Trailer_v2_%s.wav")
AUFNAHMEN = os.path.join(WURZEL, "ArtSource", "Generated", "Intro")          # per .gitignore ausgeschlossen
TON = os.path.join(WURZEL, "Genesis", "Saved", "Intro", "GENESIS_Vorfilm_Ton.wav")

API = "https://api.kie.ai/api/v1"
USER_AGENT = "GENESIS-Pipeline/1.0"

# Wie VO_S02 im Trailer (Tools/Trailer/Audio/generate_trailer_audio_kie.py), nur der Inhalt richtig
TEXT = ("[softly] Millionen machen sich auf den Weg. [pause] [quietly] Hunderte kommen an. "
        "[short pause] [quietly, with weight] Nur eine verschmilzt.")
NARRATOR = {"voice_name": "Algieba", "style": "Empathetic", "pace": "Natural", "accent": "Neutral",
            "audio_profile": "A calm, warm, deep-voiced German narrator in his fifties who has lived a full life and speaks gently, "
                             "almost like sharing a secret. Cinematic trailer delivery, intimate, never shouting."}
SCENE = ("Cinematic reveal trailer for a video game about the whole circle of life. Recorded close to a high-end "
         "microphone in a quiet studio. Long pauses, weight in every word.")
CONTEXT = ("German voice-over. Calm, warm, deep and human, tender and a little mysterious. "
           "Not a loud announcer. Unhurried, with small breaths.")

# Zeitfenster im Trailer (gemessen an der Sprachspur): Satz davor endet 9,84 s, Satz danach beginnt 19,52 s.
FRUEHESTER_BEGINN = 10.9     # mindestens ~1 s Luft nach "... bereits begonnen."
ALTER_BEGINN = 12.22
SPAETESTES_ENDE = 19.30      # ~0,2 s Luft vor "Aus einer Zelle werden zwei."
SCHNITT = (10.3, 19.40)      # in diesen Grenzen wird die alte Stimme entfernt (beide in gemessener Stille)
STILLE_DB = -34.0
PAUSE_MAX = 1.1              # laengere Atempausen in der Aufnahme werden in der Stille gekuerzt


def lies(pfad):
    with wave.open(pfad, "rb") as w:
        rate, kanaele, breite, n = w.getframerate(), w.getnchannels(), w.getsampwidth(), w.getnframes()
        roh = w.readframes(n)
    if breite == 2:
        daten = np.frombuffer(roh, np.int16).astype(np.float64) / 32768.0
    elif breite == 4:
        daten = np.frombuffer(roh, np.int32).astype(np.float64) / 2147483648.0
    else:
        raise ValueError("Bittiefe %d nicht unterstuetzt: %s" % (breite, pfad))
    return rate, daten.reshape(-1, kanaele)


def schreib(pfad, rate, daten):
    os.makedirs(os.path.dirname(pfad), exist_ok=True)
    # Gleiche Skala wie beim Lesen (32768): unveraenderte Samples kommen bitgenau wieder heraus
    werte = np.clip(np.round(daten * 32768.0), -32768, 32767).astype(np.int16)
    with wave.open(pfad, "wb") as w:
        w.setnchannels(werte.shape[1])
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(werte.tobytes())


def db(x):
    return 20.0 * np.log10(max(float(x), 1e-12))


def huelle_db(signal, rate, fenster=0.02):
    """Pegel (dBFS) je 20-ms-Fenster, beide Kanaele zusammen."""
    mono = signal.mean(axis=1) if signal.ndim == 2 else signal
    n = int(rate * fenster)
    stuecke = len(mono) // n
    rms = np.sqrt((mono[:stuecke * n].reshape(stuecke, n) ** 2).mean(axis=1))
    return 20.0 * np.log10(np.maximum(rms, 1e-9)), fenster


def sprachgrenzen(signal, rate, von=0.0, bis=None, schwelle=-45.0):
    pegel, schritt = huelle_db(signal, rate)
    a = int(von / schritt)
    b = len(pegel) if bis is None else int(bis / schritt)
    laut = np.where(pegel[a:b] > schwelle)[0]
    if len(laut) == 0:
        return None
    return (a + laut[0]) * schritt, (a + laut[-1] + 1) * schritt


def pausen(signal, rate, schwelle=-45.0, mindest=0.25):
    """Stille Abschnitte (Beginn, Ende) innerhalb der Sprache."""
    pegel, schritt = huelle_db(signal, rate)
    still = pegel <= schwelle
    ergebnis, beginn = [], None
    for i, s in enumerate(still):
        if s and beginn is None:
            beginn = i
        if not s and beginn is not None:
            if (i - beginn) * schritt >= mindest and beginn > 0:
                ergebnis.append((beginn * schritt, i * schritt))
            beginn = None
    return ergebnis


# Der Trailer-Master (Tools/Trailer/Audio/build_trailer_v2_audio.py) ist keine einfache Summe der Stems:
#   Mix = tanh(L / 0.89 * 1.1) / tanh(1.1) * 0.89,  L = 0,85*Musik + 0,8*Geraeusch*Ducking + Stimme  (in Stem-Einheiten)
# Die Saettigung ist bekannt und umkehrbar. Die Stimme wird deshalb in der linearen Summe getauscht und danach
# wieder gesaettigt; ausserhalb des Fensters bleibt jedes Sample des Trailers unveraendert.
SAETTIGUNG = 1.1
SKALA = 0.89


def linear(mix):
    y = np.clip(mix * np.tanh(SAETTIGUNG) / SKALA, -0.999999, 0.999999)
    return np.arctanh(y) / SAETTIGUNG * SKALA


def gesaettigt(summe):
    return np.tanh(summe / SKALA * SAETTIGUNG) / np.tanh(SAETTIGUNG) * SKALA


def pruefen():
    rate, mix = lies(STEMS % "Mix")
    for teil in ("Stem_Music", "Stem_SFX", "Stem_Voice"):
        r, d = lies(STEMS % teil)
        assert r == rate and d.shape == mix.shape, teil
    rueck = gesaettigt(linear(mix))
    print("GENESIS: %d Hz, %d Kanaele, %.2f s; Saettigung hin und zurueck: max. Fehler %.1f dBFS"
          % (rate, mix.shape[1], len(mix) / rate, db(np.abs(rueck - mix).max())))
    _, stimme = lies(STEMS % "Stem_Voice")
    print("GENESIS: alter Satz in der Sprachspur:", sprachgrenzen(stimme, rate, 11.0, 19.45))
    for grenze in SCHNITT:
        i = int(grenze * rate)
        print("GENESIS: Stimme am Schnitt %.2f s: %.1f dBFS" % (grenze, db(np.sqrt((stimme[i - 480:i + 480] ** 2).mean()))))


def anfrage(methode, pfad, koerper=None):
    schluessel = os.environ.get("KIE_API_KEY")
    if not schluessel:
        sys.exit("KIE_API_KEY ist nicht gesetzt (nur fuer diesen Aufruf setzen, nie speichern).")
    daten = json.dumps(koerper).encode("utf-8") if koerper is not None else None
    for versuch in range(8):
        req = urllib.request.Request(API + pfad, data=daten, method=methode, headers={
            "Authorization": "Bearer " + schluessel, "Content-Type": "application/json", "User-Agent": USER_AGENT})
        try:
            with urllib.request.urlopen(req, timeout=60) as antwort:
                ergebnis = json.loads(antwort.read().decode("utf-8"))
        except urllib.error.HTTPError as fehler:
            ergebnis = {"code": fehler.code, "msg": fehler.read().decode("utf-8", "replace")}
        if ergebnis.get("code") != 429:
            return ergebnis
        time.sleep(8 * (versuch + 1))
    return ergebnis


def erzeugen():
    os.makedirs(AUFNAHMEN, exist_ok=True)
    vorher = anfrage("GET", "/chat/credit").get("data")
    print("GENESIS: Guthaben vorher", vorher)
    sprecher = dict(NARRATOR, speaker_id="Speaker 1")
    koerper = {"model": "google/gemini-3-1-flash-tts", "input": {
        "temperature": 0.85, "scene": SCENE, "sample_context": CONTEXT, "speakers": [sprecher],
        "dialogue_turns": [{"speaker_id": "Speaker 1", "text": TEXT}]}}
    auftraege = []
    for take in ("A", "B"):
        ergebnis = anfrage("POST", "/jobs/createTask", koerper)
        auftrag = (ergebnis.get("data") or {}).get("taskId")
        if not auftrag:
            sys.exit("TTS abgelehnt: %s" % json.dumps(ergebnis, ensure_ascii=False))
        auftraege.append((take, auftrag))
        time.sleep(3.5)
    for take, auftrag in auftraege:
        beginn = time.time()
        while True:
            info = anfrage("GET", "/jobs/recordInfo?taskId=" + auftrag).get("data") or {}
            if info.get("state") == "success":
                url = json.loads(info["resultJson"])["resultUrls"][0]
                break
            if info.get("state") == "fail" or time.time() - beginn > 300:
                sys.exit("TTS %s fehlgeschlagen: %s" % (take, info.get("failMsg")))
            time.sleep(3)
        pfad = os.path.join(AUFNAHMEN, "VO_Vorfilm_S02_TAKE_%s.wav" % take)
        req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
        with urllib.request.urlopen(req, timeout=180) as antwort, open(pfad, "wb") as datei:
            datei.write(antwort.read())
        rate, d = lies(pfad)
        print("GENESIS: Aufnahme %s: %.2f s, Sprache %s" % (take, len(d) / rate, sprachgrenzen(d, rate)))
    nachher = anfrage("GET", "/chat/credit").get("data")
    print("GENESIS: Guthaben nachher", nachher, "verbraucht", round(vorher - nachher, 2))


def auf_rate(daten, von, nach):
    """Bandbegrenzt umrechnen (FFT, ganze Aufnahme) - linear interpoliert klirrt die Stimme in den Hoehen."""
    if von == nach:
        return daten
    n_neu = int(round(len(daten) * nach / von))
    spektrum = np.fft.rfft(daten, axis=0)
    ziel = np.zeros((n_neu // 2 + 1, daten.shape[1]), dtype=complex)
    k = min(len(spektrum), len(ziel))
    ziel[:k] = spektrum[:k]
    return np.fft.irfft(ziel, n=n_neu, axis=0) * (n_neu / len(daten))


def pausen_kuerzen(daten, rate):
    """Atempausen ueber PAUSE_MAX in der Mitte der Stille kuerzen (30 ms Ueberblendung)."""
    for beginn, ende in reversed(pausen(daten, rate)):
        zuviel = (ende - beginn) - PAUSE_MAX
        if zuviel <= 0:
            continue
        mitte = (beginn + ende) / 2.0
        a, b = int((mitte - zuviel / 2) * rate), int((mitte + zuviel / 2) * rate)
        blende = int(0.03 * rate)
        rampe = np.linspace(0.0, 1.0, blende)[:, None]
        uebergang = daten[a - blende:a] * (1 - rampe) + daten[b - blende:b] * rampe
        daten = np.concatenate([daten[:a - blende], uebergang, daten[b:]])
    return daten


def mischen(take):
    rate, mix = lies(STEMS % "Mix")
    _, musik = lies(STEMS % "Stem_Music")
    _, stimme = lies(STEMS % "Stem_Voice")

    # Die alte Stimme nur in gemessener Stille schneiden
    for grenze in SCHNITT:
        i = int(grenze * rate)
        pegel = db(np.sqrt((stimme[i - 480:i + 480] ** 2).mean()))
        if pegel > STILLE_DB:
            sys.exit("Schnitt bei %.2f s liegt nicht in der Stille (%.1f dBFS)" % (grenze, pegel))
    alt_a, alt_b = sprachgrenzen(stimme, rate, SCHNITT[0], SCHNITT[1])
    a, b = int(alt_a * rate), int(alt_b * rate)
    alt_rms = np.sqrt((stimme[a:b] ** 2).mean())

    r, neu = lies(os.path.join(AUFNAHMEN, "VO_Vorfilm_S02_TAKE_%s.wav" % take))
    if neu.shape[1] == 1:
        neu = np.repeat(neu, stimme.shape[1], axis=1)
    neu = auf_rate(neu, r, rate)
    s_a, s_b = sprachgrenzen(neu, rate)
    neu = neu[max(0, int((s_a - 0.05) * rate)):int((s_b + 0.25) * rate)]
    neu = pausen_kuerzen(neu, rate)
    s_a, s_b = sprachgrenzen(neu, rate)
    # Gleiche Lautheit wie der alte Satz (RMS ueber die Sprache)
    neu *= alt_rms / np.sqrt((neu[int(s_a * rate):int(s_b * rate)] ** 2).mean())

    dauer = s_b - s_a
    beginn = min(ALTER_BEGINN, SPAETESTES_ENDE - dauer)
    if beginn < FRUEHESTER_BEGINN:
        sys.exit("Aufnahme %s ist zu lang (%.2f s Sprache, Platz fuer %.2f s)" % (take, dauer, SPAETESTES_ENDE - FRUEHESTER_BEGINN))

    neue_stimme = np.zeros_like(stimme)
    start = int((beginn - s_a) * rate)
    neue_stimme[start:start + len(neu)] = neu
    s0, s1 = int(SCHNITT[0] * rate), int(SCHNITT[1] * rate)

    # Lineare Summe: alte Stimme raus, neue rein
    summe = linear(mix)
    summe[s0:s1] += neue_stimme[s0:s1] - stimme[s0:s1]

    # Beginnt der neue Satz frueher als der alte, lag die Musik dort noch nicht unter der Stimme.
    # Wie im Trailer: Musik mindestens 12 dB unter dem Erzaehler, weich (0,3 s) ein- und ausgeblendet.
    if beginn < alt_a - 0.05:
        a0, a1 = int((beginn - 0.2) * rate), int(alt_a * rate)
        v = np.sqrt((neu[int(s_a * rate):int(s_b * rate)] ** 2).mean())
        m = np.sqrt(((0.85 * musik[a0:a1]) ** 2).mean()) + 1e-9
        tiefe = float(np.clip(v / (m * 10 ** (12.0 / 20.0)), 0.07, 1.0))
        extra = np.ones(len(summe))
        extra[a0:a1] = tiefe
        k = int(0.3 * rate)
        extra = np.convolve(extra, np.ones(k) / k, mode="same")
        summe += (0.85 * musik) * (extra - 1.0)[:, None]
        print("GENESIS: Musik vor dem alten Satzbeginn zusaetzlich um %.1f dB abgesenkt" % db(tiefe))

    neu_mix = mix.copy()
    fenster = slice(s0 - int(0.5 * rate), s1 + int(0.5 * rate))
    neu_mix[fenster] = gesaettigt(summe[fenster])
    spitze = np.abs(neu_mix).max()
    if spitze > 0.999:
        sys.exit("Mischung uebersteuert (Spitze %.1f dBFS)" % db(spitze))
    schreib(TON, rate, neu_mix)
    print("GENESIS: Tonspur %s (Spitze %.1f dBFS)" % (TON, db(spitze)))
    print("GENESIS: neuer Satz %.2f - %.2f s (vorher %.2f - %.2f s)" % (beginn, beginn + dauer, alt_a, alt_b))
    versatz = beginn - s_a
    print("GENESIS: Satzgrenzen fuer die Untertitel (Pausen in der Aufnahme):")
    for p_a, p_b in pausen(neu, rate):
        if s_a < p_a < s_b:
            print("   Pause %.2f - %.2f s" % (p_a + versatz, p_b + versatz))


if __name__ == "__main__":
    if "--pruefen" in sys.argv:
        pruefen()
    elif "--erzeugen" in sys.argv:
        erzeugen()
    elif "--mischen" in sys.argv:
        mischen(sys.argv[sys.argv.index("--mischen") + 1])
    else:
        print(__doc__ or "Aufruf: --pruefen | --erzeugen | --mischen A")
