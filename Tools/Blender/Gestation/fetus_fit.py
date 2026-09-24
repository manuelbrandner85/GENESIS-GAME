# GENESIS: Der Kreislauf des Lebens
#
# Passt das Kind in die Höhle? Dieselbe Rechnung wie im Spiel (AGenesisWombScene::FetusOrientation und FitFetus),
# aus der Hülle in fetus_weeks.json – damit sich die Haltung in Blender prüfen lässt, ohne Unreal zu starten.
#
#   python fetus_fit.py [fetus_weeks.json]
#
# Ausgabe je Woche: wie weit der am weitesten herausragende Punkt über die Höhle (mit 4 % Rand für Wand und
# Plazenta) hinausreicht. 1,00 = passt genau.

import json
import math
import os
import sys

import numpy as np

TABELLE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))),
    "ArtSource", "Generated", "Gestation", "fetus_weeks.json")

# GenesisWombPerception::CavityRadiusCm
STUETZEN = [(8, 1.5), (12, 3.5), (16, 6.5), (20, 9.5), (24, 11.5), (28, 13.3), (32, 15.0), (36, 16.3), (40, 17.2)]


def hoehlen_radius(woche):
    if woche <= STUETZEN[0][0]:
        return STUETZEN[0][1]
    for (w0, r0), (w1, r1) in zip(STUETZEN, STUETZEN[1:]):
        if woche <= w1:
            return r0 + (r1 - r0) * (woche - w0) / (w1 - w0)
    return STUETZEN[-1][1]


def rahmen_zx(z, x):
    """FRotationMatrix::MakeFromZX: Z exakt, X so nah wie möglich. Spalten = Achsen."""
    z = z / np.linalg.norm(z)
    y = np.cross(z, x)
    y /= np.linalg.norm(y)
    x = np.cross(y, z)
    return np.stack([x, y, z], axis=1)


def modell_fuer(tabelle, woche):
    """AGenesisWombScene::CurrentFetusStage: das nächstkleinere gebaute Alter, auf die Scheitel-Steiß-Länge der Woche skaliert."""
    wochen = sorted((float(w) for w in tabelle if float(w) >= 10), key=float)
    best = max([w for w in wochen if w <= woche], default=wochen[0])
    weiter = min([w for w in wochen if w > woche], default=None)
    crl = tabelle["%g" % best]["crl"]
    laenge = crl
    if weiter is not None:
        laenge = crl + (tabelle["%g" % weiter]["crl"] - crl) * min(1.0, max(0.0, (woche - best) / (weiter - best)))
    return tabelle["%g" % best]["hull"], laenge / crl, best


def passung(huelle, woche, rand=0.96, massstab=1.0):
    p = np.array(huelle, dtype=float) * massstab
    mitte = p.mean(axis=0)
    cov = (p - mitte).T @ (p - mitte)
    achse = np.array([0.2, 0.1, 1.0])
    for _ in range(40):
        achse = cov @ achse
        achse /= np.linalg.norm(achse)
    if np.dot(achse, -mitte) < 0:
        achse = -achse
    kopf_unten = min(1.0, max(0.0, (woche - 31.0) / 2.0))
    kopf_unten = kopf_unten * kopf_unten * (3 - 2 * kopf_unten)
    winkel = math.pi * kopf_unten
    oben = np.array([0.0, -math.sin(winkel), math.cos(winkel)])     # Z um X gedreht
    blick = np.array([1.0, 0.0, 0.0])
    bauch = blick - oben * np.dot(blick, oben)
    bauch /= np.linalg.norm(bauch)
    drehung = rahmen_zx(oben, bauch) @ rahmen_zx(achse, np.array([1.0, 0.0, 0.0])).T
    punkte = p @ drehung.T
    r = hoehlen_radius(woche)
    halb = np.array([8.6, 8.2, 10.5]) * (r / 10.0) * rand
    augen = -punkte.mean(axis=0)
    schlimmst = 0.0
    for _ in range(200):
        q = (punkte + augen) / halb
        aus = np.linalg.norm(q, axis=1)
        i = int(np.argmax(aus))
        schlimmst = float(aus[i])
        if schlimmst <= 1.0:
            break
        augen -= (q[i] / schlimmst) * halb * min(schlimmst - 1.0, 0.2) * 0.3
    laenge = float(np.ptp(punkte @ (drehung @ achse)))
    return schlimmst, laenge, r


if __name__ == "__main__":
    tabelle = json.load(open(TABELLE, encoding="utf-8"))
    # Jede Woche so, wie das Spiel sie zeigt (Momente: 10, 12, 16, 19, 20,5, 24, 28, 31, 34, 37; Drehung 31–33 im Zeitraffer)
    for woche in [w * 0.5 for w in range(20, 81)]:
        huelle, massstab, modell = modell_fuer(tabelle, woche)
        s, laenge, r = passung(huelle, woche, massstab=massstab)
        s_wand, _, _ = passung(huelle, woche, rand=1.0, massstab=massstab)
        drehung = 31.0 < woche < 33.0
        print("SSW %4.1f (Modell %2d): Höhle r=%.1f cm, Kind %.1f cm lang, Passung %.3f (ohne Rand %.3f)%s"
              % (woche, modell, r, laenge, s, s_wand,
                 ("  <- ragt heraus" + (" (in der Drehung, Zeitraffer)" if drehung else "")) if s_wand > 1.0 else ""))
