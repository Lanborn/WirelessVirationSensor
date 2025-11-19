# Calibration & Installation Guide

## 1. Bench calibration

1. Connect ESP32 to USB and open serial monitor at 115200 baud.
2. With the ADXL345 flat on a table, run the firmware and issue the `CALIBRATE` command over the serial console.
3. The firmware collects 5 seconds of data, averages each axis, and stores offset/gain in NVS flash.
4. For the INMP445, play a 1 kHz 94 dB SPL calibrator tone. Enter the known SPL via the serial console, the firmware adjusts microphone gain.

## 2. Mounting

* Use magnetic base or adhesive to fix the accelerometer as close as possible to the vibration source.
* Align axis arrow with machine rotation axis for consistent measurements.
* For the microphone, orient the sound port away from airflow and shield from direct dust/water (IP54 enclosure recommended).

## 3. Network provisioning

* Hold the `BOOT` button for 5 seconds to start captive portal.
* Connect to SSID `VibeNode-Setup`, open `192.168.4.1`, and enter Wi-Fi credentials + MQTT broker address.
* Credentials stored in NVS encrypted partition.

## 4. Verification checklist

| Step | Expected result |
|------|-----------------|
| Power on | LED heartbeat + MQTT status message |
| Tap test | FFT peak appears around tap frequency (5-20 Hz) |
| Noise test | Microphone SPL increases >10 dB when clapping | 

## 5. Maintenance

* Re-run calibration every 6 months or after sensor relocation.
* Inspect wiring/enclosure for condensation/dust.
* Update firmware OTA via `/ota/update` endpoint.
