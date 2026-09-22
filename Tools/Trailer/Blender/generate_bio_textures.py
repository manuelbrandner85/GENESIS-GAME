# GENESIS Trailer / GENESIS-010 - nahtlos kachelbare Gewebetexturen (numpy, periodisch per Konstruktion).
#
# Aufruf:
#   "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b --factory-startup --python Tools\Trailer\Blender\generate_bio_textures.py
# Ausgabe: ArtSource/Generated/Conception/Textures/T_GEN_*.png (gemeinsam Spiel + Trailer)
#   T_GEN_Mucosa_BC (sRGB), T_GEN_Mucosa_N (DirectX-Normal, Gruen invertiert fuer Unreal), T_GEN_Mucosa_R (Rauheit, linear)
#   T_GEN_Cell_N (feine Zellmembran-Struktur), T_GEN_Cytoplasm_BC (Granula)
# Kachelgroessen in der Welt (Massstab 1 um = 1 cm): Mucosa 4 m = 400 um -> ca. 36 Epithelzellen je Kante (ca. 11 um)

import math
import os

import bpy
import numpy as np

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
OUT = os.path.join(REPO, "ArtSource", "Generated", "Conception", "Textures")
rng = np.random.default_rng(2026)


def fft_noise(n, beta, seed):
    """Periodisches 1/f^beta-Rauschen, normiert auf 0..1."""
    r = np.random.default_rng(seed)
    white = r.standard_normal((n, n))
    f = np.fft.fftfreq(n)
    fx, fy = np.meshgrid(f, f, indexing="ij")
    k = np.sqrt(fx ** 2 + fy ** 2)
    k[0, 0] = 1.0
    spec = np.fft.fft2(white) / k ** beta
    spec[0, 0] = 0
    out = np.real(np.fft.ifft2(spec))
    out -= out.min()
    return out / out.max()


def voronoi(n, cells, seed, jitter=0.85):
    """Periodisches Voronoi: F1, F2-F1 (Zellrand) und Zell-ID-Zufallswert."""
    r = np.random.default_rng(seed)
    pts = (np.stack(np.meshgrid(np.arange(cells), np.arange(cells), indexing="ij"), -1) + 0.5
           + (r.random((cells, cells, 2)) - 0.5) * jitter)
    ids = r.random((cells, cells))
    coords = (np.arange(n) + 0.5) / n * cells
    f1 = np.full((n, n), 1e9)
    f2 = np.full((n, n), 1e9)
    idv = np.zeros((n, n))
    gx, gy = np.meshgrid(coords, coords, indexing="ij")
    cx, cy = np.floor(gx).astype(int), np.floor(gy).astype(int)
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            nx, ny = cx + dx, cy + dy
            p = pts[nx % cells, ny % cells]
            px = p[..., 0] + (nx - nx % cells)
            py = p[..., 1] + (ny - ny % cells)
            d = np.sqrt((gx - px) ** 2 + (gy - py) ** 2)
            closer = d < f1
            f2 = np.where(closer, f1, np.minimum(f2, d))
            idv = np.where(closer, ids[nx % cells, ny % cells], idv)
            f1 = np.where(closer, d, f1)
    return f1, f2 - f1, idv


def normal_from_height(h, strength):
    dx = (np.roll(h, -1, 0) - np.roll(h, 1, 0)) * 0.5 * strength
    dy = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) * 0.5 * strength
    nz = np.ones_like(h)
    length = np.sqrt(dx ** 2 + dy ** 2 + nz ** 2)
    # Tangentenraum: R = X, G = Y (DirectX: Y nach unten -> invertiert)
    return np.stack([-dx / length, dy / length, nz / length], -1) * 0.5 + 0.5


def save(name, data, srgb, bits=8):
    n = data.shape[0]
    if data.ndim == 2:
        data = np.stack([data] * 3, -1)
    rgba = np.concatenate([data, np.ones((n, n, 1))], -1)
    img = bpy.data.images.new(name, n, n, alpha=False)
    img.colorspace_settings.name = "sRGB" if srgb else "Non-Color"
    # Blender-Pixel liegen zeilenweise von unten; unsere Arrays sind [x, y] -> transponieren
    img.pixels.foreach_set(np.ascontiguousarray(rgba.transpose(1, 0, 2)).astype(np.float32).ravel())
    img.filepath_raw = os.path.join(OUT, name + ".png")
    img.file_format = "PNG"
    img.save()   # rohe Pixelwerte, ohne View-Transform der Szene
    print("GENESIS_TEXTURE", name, n, "sRGB" if srgb else "linear", bits, "bit")


def linear_to_srgb(c):
    c = np.clip(c, 0, 1)
    return np.where(c <= 0.0031308, 12.92 * c, 1.055 * np.power(c, 1 / 2.4) - 0.055)


def mucosa(n=2048):
    f1, edge, cid = voronoi(n, 36, 7)
    dome = np.clip(1.0 - f1 / 0.75, 0, 1) ** 1.5                    # leicht gewoelbte Epithelzellen
    border = np.exp(-edge / 0.06)                                    # Zellgrenzen
    micro = fft_noise(n, 1.6, 8)
    macro = fft_noise(n, 2.6, 9)
    vessels = np.clip((fft_noise(n, 2.2, 10) - 0.55) * 6.0, 0, 1) ** 2  # angedeutete Kapillarschatten
    height = dome * 0.55 - border * 0.35 + micro * 0.25 + macro * 0.4
    # Albedo linear: Schleimhaut rosa-beige, Kapillaren roetlicher, Zellgrenzen minimal dunkler
    base = np.array([0.42, 0.20, 0.17])
    col = base[None, None, :] * (0.82 + 0.3 * macro[..., None]) * (0.9 + 0.12 * (cid[..., None] - 0.5))
    col = col * (1 - 0.18 * border[..., None])
    col = col * (1 - 0.35 * vessels[..., None]) + np.array([0.30, 0.05, 0.05]) * 0.35 * vessels[..., None]
    col = np.clip(col, 0.03, 0.8)
    save("T_GEN_Mucosa_BC", linear_to_srgb(col), True)
    save("T_GEN_Mucosa_N", normal_from_height(height, 18.0), False)
    rough = np.clip(0.22 + 0.16 * micro + 0.12 * border + 0.08 * (macro - 0.5), 0.12, 0.6)   # nasser Schleim
    save("T_GEN_Mucosa_R", rough, False)
    print("GENESIS_TEXTURE Albedo Mittel (linear)", np.round(col.reshape(-1, 3).mean(0), 3))


def cell_detail(n=1024):
    f1, edge, _ = voronoi(n, 20, 21, jitter=0.7)
    fine = fft_noise(n, 1.2, 22)
    height = -np.exp(-edge / 0.05) * 0.4 + fine * 0.35 + np.clip(1 - f1, 0, 1) * 0.25
    save("T_GEN_Cell_N", normal_from_height(height, 10.0), False)


def cytoplasm(n=1024):
    grains = fft_noise(n, 0.6, 31)
    grains = np.clip((grains - 0.55) * 5, 0, 1)
    clouds = fft_noise(n, 2.4, 32)
    col = np.array([0.52, 0.44, 0.36])[None, None, :] * (0.75 + 0.35 * clouds[..., None]) * (1 - 0.35 * grains[..., None])
    save("T_GEN_Cytoplasm_BC", linear_to_srgb(col), True)


os.makedirs(OUT, exist_ok=True)
mucosa()
cell_detail()
cytoplasm()
print("GENESIS_TEXTURES_OK")
