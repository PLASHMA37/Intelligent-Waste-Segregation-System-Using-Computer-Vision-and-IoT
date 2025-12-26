import time
import io
import requests
import hashlib
from ultralytics import YOLO
from PIL import Image
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

ESP = "http://10.238.38.113"
IMAGE_URL = f"{ESP}/image.jpg"
RESULT_URL = f"{ESP}/result"
TIMEOUT = 5
SLEEP_IDLE = 0.4

model = YOLO(r"C:/Users/unpra/Downloads/PH Ambulances.v1i.yolov11/dry-wet-metal/train_results2/weights/best.pt")
print("Model classes:", model.names)

MAP_TO_OUTPUT = {"Dry_Waste": "PLASTIC", "Wet_Waste": "WET"}
last_hash = None

session = requests.Session()
adapter = HTTPAdapter(max_retries=5)
session.mount("http://", adapter)

def hash_bytes(b: bytes) -> str:
    return hashlib.sha1(b).hexdigest()

def pick_token(results):
    if not results:
        return "WET"
    names = results[0].names
    boxes = results[0].boxes
    if boxes is None or len(boxes) == 0:
        return "PLASTIC"
    best_label, best_conf = None, -1.0
    for b in boxes:
        c = float(b.conf.item())
        cls = int(b.cls.item())
        lab = names.get(cls, str(cls))
        if c > best_conf:
            best_label, best_conf = lab, c
    if best_label in MAP_TO_OUTPUT:
        return MAP_TO_OUTPUT[best_label]
    bl = best_label.lower() if best_label else ""
    if "dry" in bl:
        return "PLASTIC"
    if "wet" in bl:
        return "WET"
    return "WET"

def classify_frame(jpeg_bytes: bytes) -> str:
    try:
        img = Image.open(io.BytesIO(jpeg_bytes)).convert("RGB")
        results = model.predict(
            source=img,
            imgsz=320,
            save=True,
            project="C:/Users/unpra/Downloads/DRY-WET-METAL.v1i.folder/Results",
            conf=0.25,
            verbose=False
        )
        return pick_token(results)
    except Exception as e:
        print("Classification error:", e)
        return "WET"

def post_result(token: str):
    try:
        session.post(RESULT_URL, data=token, headers={"Content-Type": "text/plain"}, timeout=TIMEOUT)
    except requests.exceptions.RequestException as e:
        print("Post error:", e)

while True:
    try:
        try:
            r = session.get(IMAGE_URL, timeout=TIMEOUT)
            if r.status_code == 200 and r.content:
                h = hash_bytes(r.content)
                if h != last_hash:
                    token = classify_frame(r.content)
                    post_result(token)
                    print("Sent:", token)
                    last_hash = h
        except requests.exceptions.RequestException as e:
            print(f"⚠️ ESP request error: {e}")
        time.sleep(SLEEP_IDLE)

    except KeyboardInterrupt:
        print("Exiting...")
        break

    except Exception as e:
        print("Loop error:", e)
        time.sleep(1.0)













