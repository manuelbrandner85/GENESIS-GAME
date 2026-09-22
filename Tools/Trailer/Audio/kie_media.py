# GENESIS Trailer - gemeinsame Helfer fuer kie.ai (Bilder, Videos, Upload). API-Schluessel nur aus KIE_API_KEY.
# Alle erzeugten Bilder/Videos sind KONZEPTBILDER (keine Spielaufnahmen) und werden im Manifest so gefuehrt.

import base64
import json
import os
import sys
import time
import urllib.error
import urllib.request

API = "https://api.kie.ai/api/v1"
UPLOAD = "https://kieai.redpandaai.co/api/file-base64-upload"
USER_AGENT = "Mozilla/5.0 (GENESIS trailer tools)"


def _key():
    key = os.environ.get("KIE_API_KEY")
    if not key:
        sys.exit("KIE_API_KEY ist nicht gesetzt.")
    return key


def request(method, url, body=None, timeout=90):
    data = json.dumps(body).encode("utf-8") if body is not None else None
    result = {}
    for attempt in range(10):
        req = urllib.request.Request(url if url.startswith("http") else API + url, data=data, method=method, headers={
            "Authorization": "Bearer " + _key(), "Content-Type": "application/json", "User-Agent": USER_AGENT})
        try:
            with urllib.request.urlopen(req, timeout=timeout) as response:
                result = json.loads(response.read().decode("utf-8"))
        except urllib.error.HTTPError as error:
            result = {"code": error.code, "msg": error.read().decode("utf-8", "replace")}
        except (urllib.error.URLError, TimeoutError) as error:
            result = {"code": 599, "msg": str(error)}
        if result.get("code") not in (429, 599):
            return result
        time.sleep(8 * (attempt + 1))
    return result


def credit():
    return request("GET", "/chat/credit").get("data")


def upload(path, remote_name=None):
    """Lokales Bild hochladen (temporaer bei kie.ai), liefert eine URL fuer image_input/image_urls."""
    ext = os.path.splitext(path)[1].lower().lstrip(".")
    mime = "image/png" if ext == "png" else "image/jpeg"
    with open(path, "rb") as f:
        data = "data:%s;base64,%s" % (mime, base64.b64encode(f.read()).decode("ascii"))
    result = request("POST", UPLOAD, {"base64Data": data, "uploadPath": "genesis-trailer",
                                     "fileName": remote_name or os.path.basename(path)}, timeout=300)
    url = (result.get("data") or {}).get("downloadUrl") or (result.get("data") or {}).get("fileUrl")
    if not url:
        raise RuntimeError("Upload fehlgeschlagen: %s" % json.dumps(result)[:400])
    return url


def create(model, inputs):
    result = request("POST", "/jobs/createTask", {"model": model, "input": inputs})
    task = (result.get("data") or {}).get("taskId")
    if not task:
        raise RuntimeError("Auftrag abgelehnt (%s): %s" % (model, json.dumps(result, ensure_ascii=False)[:600]))
    return task


def wait(task, timeout=1800, poll=6):
    started = time.time()
    while time.time() - started < timeout:
        info = request("GET", "/jobs/recordInfo?taskId=" + task).get("data") or {}
        state = info.get("state")
        if state == "success":
            return json.loads(info["resultJson"]).get("resultUrls") or [], info
        if state == "fail":
            raise RuntimeError("Auftrag %s fehlgeschlagen: %s" % (task, info.get("failMsg")))
        time.sleep(poll)
    raise RuntimeError("Auftrag %s nicht rechtzeitig fertig" % task)


def download(url, path):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=600) as response, open(path, "wb") as f:
        f.write(response.read())
    return os.path.getsize(path)
