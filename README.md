# Gazing Attention (White Mask: physical)

**A wearable mask that feels being looked at. Attention builds up like pressure, and the mask reacts physically when it becomes too much.**

Part of the **White Mask** series → see also [whitemask-phasefield](https://github.com/lileinsteinbrain/whitemask-phasefield) (simulation) and [white-membrane-proto](https://github.com/lileinsteinbrain/white-membrane-proto) (XR prototype).

## The idea
Being watched is not a single event. It accumulates. This project models observation as a force that builds over time: a glance barely registers, a sustained stare slowly pushes the mask into a different state. When observation fades, the mask gradually recovers.

## How it works
```
 Webcam (motion → face → gaze direction) ─┐
 Distance sensor (how close someone is) ──┼─► observation pressure λ ─► coherence σ
 Light sensor (how exposed the wearer is) ┘     (accumulates, leaks,     │
                                                 recovers over time)    ▼
                                      state: COLLAPSE  ·  IDLE  ·  REPHASE
                                                 │
                       vibration motor · small fan · servo that turns the mask · OLED label
```
- **Camera pipeline** in three levels of sophistication: plain motion (`lv1`), face + motion (`lv2`), gaze vector via MediaPipe or WebGazer (`lv3`)
- **Bridge:** `send_lambda.py` forwards the camera's λ value to the microcontroller over serial
- **Mask (ESP32):** combines camera, distance (VL53L0X) and light (BH1750) into σ; low σ = *collapse*, high σ = *rephase*, with hysteresis so states don't flicker

## What's in this repo
| Path | What it is |
|---|---|
| `lv1_motion_proxy.py` | Observation = overall motion in the camera image |
| `lv2_face_motion_proxy.py` | Observation = motion around a detected face |
| `lv3_gaze_vector.py`, `lv3_webgazer.js` | Observation = where someone is actually looking |
| `send_lambda.py` | Small Flask server that sends λ to the mask over serial |
| `whitemask.ino` | Mask firmware: sensor fusion, state machine, actuators |

## Status
Working prototype.
- ✅ Sustained observation produces different behaviour from brief glances
- ✅ Physical response makes the internal "tension" perceptible on the body
- 🔲 Thresholds are hand-tuned; the wearable itself needs mechanical refinement
- 🔲 No user study yet on how people read the mask's responses
