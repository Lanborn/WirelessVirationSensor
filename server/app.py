"""Reference FastAPI server for ingesting ESP32 vibration payloads."""
from __future__ import annotations

import asyncio
from datetime import datetime
from pathlib import Path
from typing import List

from fastapi import FastAPI, UploadFile
from fastapi.middleware.cors import CORSMiddleware
from paho.mqtt import client as mqtt
from pydantic import BaseModel

DATA_DIR = Path("data")
DATA_DIR.mkdir(exist_ok=True)

app = FastAPI(title="Wireless Vibration Sensor Backend")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"]
)

mqtt_client = mqtt.Client()
mqtt_messages: List[dict] = []


class Telemetry(BaseModel):
    device_id: str
    timestamp_ms: int
    accel_rms: List[float]
    spl_db: float
    wifi_rssi: int


@mqtt_client.connect_async
@app.on_event("startup")
async def startup_event():
    """Connect to MQTT broker asynchronously and subscribe to wildcard."""
    broker = "localhost"
    mqtt_client.connect_async(broker, 1883, 60)
    mqtt_client.loop_start()
    mqtt_client.subscribe("factory/+/vibration")

    def on_message(client, userdata, msg):
        payload = msg.payload.decode()
        timestamp = datetime.utcnow().isoformat()
        path = DATA_DIR / f"mqtt_{timestamp}.json"
        path.write_text(payload)

    mqtt_client.on_message = on_message


@app.post("/ingest")
async def ingest(payload: Telemetry):
    """Store JSON payload to disk and keep in memory for quick plotting."""
    mqtt_messages.append(payload.dict())
    file_path = DATA_DIR / f"rest_{payload.timestamp_ms}.json"
    file_path.write_text(payload.json())
    return {"status": "ok"}


@app.get("/telemetry")
async def latest(n: int = 20):
    return list(reversed(mqtt_messages[-n:]))


@app.post("/ota/update")
async def ota_update(file: UploadFile):
    firmware_path = Path("firmware.bin")
    firmware_path.write_bytes(await file.read())
    return {"status": "stored", "bytes": firmware_path.stat().st_size}
