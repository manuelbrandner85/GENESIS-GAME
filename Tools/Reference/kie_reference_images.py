"""GENESIS – Look-Dev-Zielbilder über kie.ai (Nano Banana Pro, 4K). Schlüssel nur aus KIE_API_KEY."""
import json, os, sys, time, urllib.request

KEY = os.environ["KIE_API_KEY"]
BASE = "https://api.kie.ai/api/v1"
OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)


def call(method, path, body=None):
    req = urllib.request.Request(BASE + path, method=method,
                                 data=json.dumps(body).encode() if body else None,
                                 headers={"Authorization": "Bearer " + KEY, "Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=60) as r:
        return json.loads(r.read().decode())


COMMON = (" Photorealistic scientific reference image, true-to-scale human biology, natural optical imperfections, "
          "no text, no labels, no scale bar, no watermark, no illustration style, no CGI look.")
PROMPTS = {
    "01_Spermien_Eileiter": "High-speed microscopy photograph (differential interference contrast, 400x) of living human sperm "
        "swimming close to the ciliated epithelium of the fallopian tube ampulla. Flat paddle-shaped heads about 5 micrometres long, "
        "thin flagella about 50 micrometres long showing a travelling bending wave that grows toward the tip, motion blur on the "
        "tail ends, mucosal folds and beating cilia in soft focus, warm dim transmitted light, dark background." + COMMON,
    "02_Keim_4Zellen_Hoffman": "Hoffman modulation contrast micrograph from an IVF time-lapse incubator of a human embryo on day 2 "
        "with four blastomeres inside the zona pellucida (about 16 micrometres thick). Grey-beige translucent granular cytoplasm, "
        "faintly visible nuclei, a small polar body and a few tiny cytoplasmic fragments in the perivitelline space, relief-like "
        "oblique lighting from one side, thin bright halos at the edges, neutral grey background." + COMMON,
    "03_Blastozyste_schluepft": "Hoffman modulation contrast micrograph of a hatching human blastocyst on day 6: the trophectoderm "
        "herniates through a breach in the thinned zona pellucida, a compact inner cell mass on one side, large fluid-filled "
        "blastocoel, flattened trophectoderm cells forming a thin epithelium, grey-beige tones, oblique relief lighting, neutral "
        "grey background, 200x." + COMMON,
    "04_Gebaermutterschleimhaut": "Hysteroscopic macro photograph of human uterine endometrium in the secretory phase during the "
        "implantation window: thick velvety pink-red mucosa with gland openings, fine branching superficial blood vessels, "
        "a moist glistening surface with soft specular highlights from the endoscope light, shallow depth of field." + COMMON,
    "05_Embryo_Fruchtblase": "Macro photograph in the tradition of classic embryology photography of a human embryo at about seven "
        "weeks after fertilization (Carnegie stage 18, 15 millimetres) inside the intact translucent amniotic sac and chorion, "
        "hand plates with finger rays, dark retinal pigment in the eye, liver and heart bulge, fine blood vessels in the chorionic "
        "villi, warm transmitted light, black background, scientific and respectful." + COMMON,
    "06_Kreisssaal": "Photograph of a modern German hospital delivery room at dusk, seen from the door: an electric birthing bed with "
        "the back section raised to 45 degrees, a cotton blanket, a CTG monitor on a cart, an infant radiant warmer, an IV pole, "
        "a birthing rope hanging from the ceiling, a birthing ball, half-closed venetian blinds with blue evening light, dimmed "
        "LED ceiling panels and a warm wall lamp above the headboard, linoleum floor with coved skirting, realistic wear, "
        "no people, 24 mm lens, natural exposure." + COMMON,
}

credit = call("GET", "/chat/credit")
print("Guthaben vorher:", credit.get("data"))
if not isinstance(credit.get("data"), (int, float)) or credit["data"] < 200:
    sys.exit("Zu wenig Guthaben – abgebrochen.")

tasks = {}
for name, prompt in PROMPTS.items():
    body = {"model": "nano-banana-pro", "callBackUrl": "https://example.com/genesis-none",
            "input": {"prompt": prompt, "aspect_ratio": "16:9", "resolution": "4K", "output_format": "png"}}
    r = call("POST", "/jobs/createTask", body)
    print(name, "->", r.get("code"), r.get("msg"), (r.get("data") or {}).get("taskId"))
    if r.get("code") == 200:
        tasks[name] = r["data"]["taskId"]
    time.sleep(1)

done = {}
deadline = time.time() + 900
while len(done) < len(tasks) and time.time() < deadline:
    time.sleep(10)
    for name, tid in tasks.items():
        if name in done:
            continue
        r = call("GET", "/jobs/recordInfo?taskId=" + tid)
        d = r.get("data") or {}
        state = d.get("state") or d.get("status")
        if state in ("success", "SUCCESS"):
            urls = json.loads(d.get("resultJson") or "{}").get("resultUrls") or []
            if urls:
                path = os.path.join(OUT, name + ".png")
                urllib.request.urlretrieve(urls[0], path)
                done[name] = path
                print("fertig:", name, os.path.getsize(path) // 1024, "KB")
        elif state in ("fail", "FAILED", "failed"):
            done[name] = None
            print("FEHLER:", name, d.get("failMsg") or d.get("failCode"))

print("Guthaben nachher:", call("GET", "/chat/credit").get("data"))
