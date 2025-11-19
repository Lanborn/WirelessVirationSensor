# System Architecture

The wireless vibration and acoustic monitoring solution is broken down into four layers:

1. **Sensor front-end** – ADXL345 3-axis accelerometer (I²C/SPI) and INMP445 digital MEMS microphone (I²S) capture mechanical vibration and airborne noise respectively.
2. **Edge node (ESP32)** – Samples sensors, performs windowed FFT/RMS feature extraction, timestamps data, and sends JSON payloads over Wi-Fi + MQTT.
3. **Ingestion & storage** – FastAPI service exposes a REST endpoint for historical queries while an MQTT broker ingests live telemetry. Data is persisted to TimescaleDB/InfluxDB (placeholder in this repo).
4. **Analytics & alerting** – Python pipeline computes spectral signatures, vibration severity metrics (ISO 10816), and noise SPL metrics. Alerts can be pushed via webhooks or integration with SCADA.

![architecture](https://i.imgur.com/6qJwqzW.png)

## Hardware overview

| Component  | Function                          | Notes |
|------------|-----------------------------------|-------|
| ESP32-WROOM | Wi-Fi MCU with dual cores + I²C/I²S | Provides enough processing for FFT and MQTT | 
| ADXL345    | ±2g/4g/8g/16g MEMS accelerometer   | Use 3200 Hz data rate, 13-bit resolution |
| INMP445    | 24-bit PDM/I²S MEMS microphone     | Use 48 kHz sample rate, 24-bit depth |
| Li-ion battery + charger | Portable power | Add buck regulator for 3.3 V rail |

## Data pipeline

1. **Sampling** – ADXL345 sampled at 1600 Hz per axis; INMP445 audio at 48 kHz.
2. **Processing** – ESP32 performs Hann-windowed FFT on 1024-sample windows for vibration; calculates 1/3 octave bands for audio.
3. **Transmission** – JSON payload includes RMS acceleration, peak frequencies, SPL, device health telemetry. Published via MQTT topic `factory/{device_id}/vibration`.
4. **Storage** – FastAPI service stores JSON to disk (prototype) or forwards to time-series DB.
5. **Analytics** – Offline script loads JSON, plots spectra, compares to thresholds.

## Network topology

* ESP32 connects to plant Wi-Fi (2.4 GHz), publishes MQTT to broker (Mosquitto) either locally or through the FastAPI gateway.
* Firmware supports OTA updates by hosting binary on FastAPI and using ESP32 HTTP update class.

## Extending the design

* Add LTE/NB-IoT module via UART for remote deployments.
* Swap ADXL345 for higher bandwidth sensors (ADXL355, IIS3DWB) if >1 kHz vibration is needed.
* Integrate edge ML (TinyML) for on-node anomaly detection.
