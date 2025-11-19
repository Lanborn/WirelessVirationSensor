# Wireless Vibration & Noise Sensor

This repository contains a complete reference design for a wireless vibration and acoustic monitoring sensor built around an ESP32 microcontroller. The sensing front-end combines an **ADXL345** 3-axis accelerometer for vibration data and an **INMP445** digital MEMS microphone for acoustic/noise measurements. Firmware streams synchronized features to a lightweight cloud endpoint, where data can be stored, visualized, and analyzed for anomaly detection.

## Repository layout

```
├── README.md                 – high-level overview
├── docs/
│   ├── system_architecture.md – hardware + dataflow description
│   └── calibration.md         – calibration and installation guide
├── firmware/
│   └── esp32/
│       ├── platformio.ini     – PlatformIO build configuration
│       └── src/
│           ├── config.h       – Wi-Fi + MQTT settings (to be edited)
│           └── main.cpp       – ESP32 firmware
├── server/
│   ├── requirements.txt       – FastAPI + MQTT utilities
│   └── app.py                 – reference ingestion/visualization service
└── data_processing/
    └── analyze_vibration.py   – offline spectral & RMS analysis pipeline
```

## Quick start

1. **Firmware**
   * Install [PlatformIO](https://platformio.org/).
   * Copy `firmware/esp32/src/config.h.example` to `config.h` and fill in Wi-Fi/MQTT credentials.
   * Build and flash: `pio run -t upload`.

2. **Server**
   * `cd server && python -m venv .venv && source .venv/bin/activate`
   * `pip install -r requirements.txt`
   * `uvicorn app:app --reload`

3. **Analysis**
   * Record data by letting the ESP32 stream for a few minutes.
   * Run `python data_processing/analyze_vibration.py sample_data.json`

See the documents in `docs/` for background, installation, and calibration procedures.
