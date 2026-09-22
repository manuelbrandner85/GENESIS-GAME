"""GENESIS – die gesprochenen Sätze der Geburt und der ersten Stunde (Mutter und Hebamme).

Jede Zeile gehört zu einem Moment der Simulation (siehe GenesisSceneSpeech.cpp, gleiche IDs):
  L_ Eröffnungsphase (Latent/Active)    T_ Übergangsphase        P_ Austreibung (Pressen)
  C_ Erschwernis (Herztöne, Schulter, Steiß)                    D_ die ersten Minuten nach der Geburt
  S_ Haut an Haut      E_ Blickkontakt      F_ Suchen nach der Brust      Z_ Einschlafen
  G_ Schwangerschaft – die Stimme der Mutter von außen, durch Bauchdecke und Fruchtwasser
Suffix _F / _M: Variante für Mädchen / Jungen (aus dem Genom des Kindes).

Fachlich geprüft: Hecheln statt Pressen beim Durchtritt des Kopfes (Dammschutz), Seitenlage bei
Herztonabfällen durch Nabelschnurdruck, Beine anziehen (McRoberts) bei Schulterdystokie,
Haut-an-Haut-Kontakt direkt nach der Geburt (Wärme), das Kind sucht die Brust selbst (Bonding).

Aufruf: python Tools/Audio/speech_lines.py > auftraege.json   (erzeugt die kie.ai-Aufträge)
"""
import json

MODEL = "google/gemini-3-1-flash-tts"
SCENE = ("A quiet German hospital delivery room at night. Real, unscripted speech between a woman giving birth "
         "and an experienced midwife. Natural German pronunciation, no theatrical acting.")

VOICES = {
    "M": {"voice_name": "Sulafat", "style": "Empathetic", "pace": "Natural",
          "audio_profile": "A German woman of about thirty giving birth to her first child. Warm, natural voice; "
                           "exhausted, emotional and completely honest.",
          "context": "German. The mother. Every line is heartfelt, never performed."},
    "H": {"voice_name": "Gacrux", "style": "Empathetic", "pace": "Natural",
          "audio_profile": "An experienced German midwife in her fifties. Calm, grounded, warm and clear; "
                           "she has attended thousands of births and knows exactly what to say.",
          "context": "German. The midwife. Calm authority, warmth, short clear sentences."},
}

LINES = [
    # Eröffnungsphase
    ("L_H_Atmen_01", "H", "[calm, steady]", "Gut so. Tief in den Bauch atmen … und ganz lang wieder aus."),
    ("L_H_Atmen_02", "H", "[calm, soothing]", "Die Wehe ist gleich vorbei. Lassen Sie die Schultern locker."),
    ("L_H_Lob_01", "H", "[warmly]", "Das machen Sie richtig gut. Der Muttermund geht schön auf."),
    ("L_M_Wehe_01", "M", "[breathing heavily through a strong contraction, a low moan]", "Hhh … oh Gott … hhh …"),
    ("L_M_Wehe_02", "M", "[out of breath, exhausted, after a contraction]", "Die war heftig …"),
    # Übergangsphase
    ("T_M_KannNicht", "M", "[exhausted, desperate, close to tears]", "Ich kann nicht mehr … ich kann einfach nicht mehr."),
    ("T_H_Doch", "H", "[calm, firm, reassuring]", "Doch, Sie können. Das ist jetzt die schwerste Phase – das heißt, Ihr Kind ist bald da."),
    # Austreibung
    ("P_H_Schieben_01", "H", "[energetic, encouraging]", "Jetzt kommt die Wehe – Kinn auf die Brust und mitschieben! Weiter, weiter, weiter …"),
    ("P_H_Schieben_02", "H", "[encouraging]", "Und noch einmal tief Luft holen … und schieben!"),
    ("P_H_Pause", "H", "[softly]", "Und ausruhen. Ganz locker lassen. Jetzt haben Sie Pause."),
    ("P_H_Kopf", "H", "[focused, calm]", "Ich sehe das Köpfchen! Jetzt nicht mehr pressen – nur noch hecheln, ganz kurz, so wie ich."),
    ("P_M_Pressen", "M", "[pushing with all her strength, a long strained groan]", "Mmmmh … aaaah …"),
    # Erschwernisse
    ("C_H_Herztoene", "H", "[calm, attentive]", "Die Herztöne gehen bei den Wehen etwas runter. Drehen Sie sich bitte einmal auf die linke Seite – so ist es besser."),
    ("C_H_Schulter", "H", "[urgent but controlled]", "Die Schulter hängt. Beine ganz fest zu sich heranziehen – jetzt! Und nicht pressen."),
    ("C_H_Steiss", "H", "[calm, reassuring]", "Ihr Kind kommt mit dem Po zuerst. Das dauert etwas länger – wir haben Zeit, alles ist vorbereitet."),
    # Geboren
    ("D_H_Da_F", "H", "[joyful, warm]", "Da ist sie! Herzlichen Glückwunsch – ein Mädchen."),
    ("D_H_Da_M", "H", "[joyful, warm]", "Da ist er! Herzlichen Glückwunsch – ein Junge."),
    ("D_M_Hallo", "M", "[crying and laughing at the same time, overwhelmed]", "Oh … oh mein Gott … hallo … hallo, du …"),
    ("D_H_Brust_F", "H", "[gently]", "Ich lege sie Ihnen direkt auf die Brust. Haut an Haut bleibt sie am besten warm."),
    ("D_H_Brust_M", "H", "[gently]", "Ich lege ihn Ihnen direkt auf die Brust. Haut an Haut bleibt er am besten warm."),
    ("D_H_Atmung_F", "H", "[calm, reassuring]", "Sie atmet gut. Und schauen Sie – sie wird schon ganz rosig."),
    ("D_H_Atmung_M", "H", "[calm, reassuring]", "Er atmet gut. Und schauen Sie – er wird schon ganz rosig."),
    # Versorgung auf der Brust: Abtrocknen mit einem warmen Tuch, dann ein zweites, trockenes darüber (sie spricht mit dem Kind)
    ("D_H_Trocken", "H", "[softly, in a warm, slightly raised sing-song voice talking to a newborn, while rubbing it dry]", "So, du Kleines – jetzt rubbel ich dich erst mal schön trocken."),
    ("D_H_Tuch", "H", "[gently, contented]", "Und ein warmes Tuch drüber. So liegst du gut."),
    # Haut an Haut
    ("S_M_Warm", "M", "[softly, tender, close to the baby]", "Schhh … ich bin da. Du bist ja ganz warm."),
    ("S_M_Geschafft", "M", "[whispering, tender, exhausted]", "Wir haben's geschafft, du und ich."),
    ("S_M_Beruhigen", "M", "[soothing, softly, in a gentle rocking rhythm]", "Schhh, schhh … alles gut. Mama ist da. Mama ist ja da."),
    # Blickkontakt
    ("E_M_Blick_01", "M", "[whispering, amazed]", "Hallo … du schaust mich ja an. Ich bin deine Mama."),
    ("E_M_Blick_02", "M", "[softly, moved, a small smile in her voice]", "Da bist du. Ich hab so lange auf dich gewartet."),
    # Die Brust suchen
    ("F_H_Suchen_F", "H", "[softly]", "Schauen Sie – sie sucht schon die Brust. Lassen Sie sie ruhig selbst machen."),
    ("F_H_Suchen_M", "H", "[softly]", "Schauen Sie – er sucht schon die Brust. Lassen Sie ihn ruhig selbst machen."),
    # Einschlafen
    ("Z_M_Schlaf", "M", "[whispering, very softly]", "Schlaf ruhig, mein Schatz."),
    # Schwangerschaft: die Mutter spricht mit dem Bauch
    ("G_M_Bauch_01", "M", "[softly, smiling]", "Hallo, du da drin. Na – bist du wach?"),
    ("G_M_Bauch_02", "M", "[tenderly, quietly]", "Nicht mehr lange, dann sehen wir uns."),
]


def jobs():
    result = []
    for line_id, speaker, direction, text in LINES:
        voice = dict(VOICES[speaker])
        context = voice.pop("context")
        voice.update({"speaker_id": "Speaker 1", "accent": "Neutral"})
        result.append({"name": line_id, "model": MODEL, "input": {
            "temperature": 0.9, "scene": SCENE, "sample_context": context,
            "speakers": [voice], "dialogue_turns": [{"speaker_id": "Speaker 1", "text": direction + " " + text}]}})
    return result


if __name__ == "__main__":
    print(json.dumps(jobs(), ensure_ascii=False, indent=1))
