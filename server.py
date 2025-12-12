# server.py
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
import time

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

telemetry_data = []
logs = []
last_calibration_time = None


@app.get("/")
def dashboard():
    return FileResponse("dashboard.html")


@app.get("/dashboard.js")
def get_js():
    return FileResponse("dashboard.js")


@app.post("/ingest")
async def ingest(data: dict):
    global last_calibration_time

    entry = {
        "timestamp": time.time(),
        "tilt_angle": data.get("tilt_angle", 0),
        "posture_label": data.get("posture_label", "Unknown"),
        "light": data.get("light", 0),
        "buzzer": 1 if data.get("buzzer") else 0,
        "calibration": 1 if data.get("calibration") else 0,
    }

    telemetry_data.append(entry)

    telemetry_data[:] = telemetry_data[-1000:]

    if entry["calibration"] == 1:
        last_calibration_time = entry["timestamp"]
        logs.append(f"[CALIBRATION] Button pressed at {time.ctime()}")

    if entry["buzzer"] == 1:
        logs.append(f"[BUZZER] Triggered at {time.ctime()} (light={entry['light']})")

    logs.append(f"[POSTURE] {entry['posture_label']} (angle={entry['tilt_angle']:.1f}°)")

    logs[:] = logs[-50:]

    return {"status": "ok"}


@app.get("/telemetry")
async def telemetry():
    return {
        "data": telemetry_data[-300:],
        "logs": logs[-50:],
        "last_calibration": last_calibration_time
    }
