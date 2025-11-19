"""Offline analysis utilities for vibration + acoustic telemetry."""
from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Tuple

import numpy as np


def load_payloads(path: Path):
    data = json.loads(path.read_text())
    if isinstance(data, dict):
        return [data]
    return data


def accel_severity(accel_rms: Tuple[float, float, float]) -> float:
    """Calculate overall vibration severity per ISO 10816 (mm/s)."""
    # Convert from m/s^2 to mm/s using integration over assumed 1 Hz bandwidth.
    mps2_to_mms = 1000.0
    return float(np.linalg.norm(accel_rms) * mps2_to_mms)


def main(file_path: str):
    payloads = load_payloads(Path(file_path))
    accel_matrix = np.array([p["accel_rms"] for p in payloads])
    spl = np.array([p["spl_db"] for p in payloads])

    severity = [accel_severity(tuple(row)) for row in accel_matrix]
    print("=== Telemetry summary ===")
    print(f"Samples: {len(payloads)}")
    print(f"Accel RMS mean (m/s^2): {accel_matrix.mean(axis=0)}")
    print(f"Accel severity (mm/s) median: {np.median(severity):.2f}")
    print(f"SPL mean (dB): {spl.mean():.1f}")

    # FFT across time for X axis to find dominant frequencies
    fft = np.fft.rfft(accel_matrix[:, 0] - accel_matrix[:, 0].mean())
    freqs = np.fft.rfftfreq(len(accel_matrix), d=2.0)  # 2 s between payloads
    dom_idx = np.argmax(np.abs(fft))
    print(f"Dominant vibration frequency: {freqs[dom_idx]:.2f} Hz")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python analyze_vibration.py payload.json")
        raise SystemExit(1)
    main(sys.argv[1])
