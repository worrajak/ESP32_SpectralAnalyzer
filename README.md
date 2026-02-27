# ESP32 Spectrum Analyzer — Plant Health Monitor

🌱 **Real-time Plant Health Monitoring using AS7343 Spectral Sensor**

[![GitHub](https://img.shields.io/badge/GitHub-ESP32_SpectralAnalyzer-blue?logo=github)](https://github.com/worrajak/ESP32_SpectralAnalyzer)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-PlatformIO-orange)](https://platformio.org/)
[![Status](https://img.shields.io/badge/Status-Active-brightgreen)](https://github.com/worrajak/ESP32_SpectralAnalyzer)

---

## 🔋 Project Overview

A real-time spectral analysis system built on the **ESP32 (Freenove WROVER)** using the **AS7343 12-channel spectral sensor** and an **SSD1306 OLED display**. The system reads plant-reflected light across 12 wavelength bands and computes 7 vegetation health indices, displayed live on OLED and serial monitor.

### Key Features

- ✅ **AS7343 Spectral Sensor** — 12 calibrated spectral channels (405–855 nm)
- ✅ **7 Vegetation Indices** — NDVI, Chlorophyll CI, Anthocyanin ARI, Water Stress, Red:FarRed, Photosynthetic Activity, Carotenoid Index
- ✅ **Health Level Scoring** — 0–5 scale for Vigor, Chlorophyll, Stress, Water status
- ✅ **SSD1306 OLED** — 128×64 real-time display with organized multi-column layout
- ✅ **Dark Calibration** — Automatic sensor offset correction on startup
- ✅ **Serial Logging** — Detailed debug output with raw channel values and computed indices
- ✅ **I2C Bus** — AS7343 (0x39) + OLED (0x3C) on shared bus
- ✅ **LoRa Ready** — RadioHead library integrated for future wireless expansion

---

## 📋 Hardware

| Component | Model | Interface | Address |
|-----------|-------|-----------|---------|
| MCU | Freenove ESP32 WROVER | — | — |
| Spectral Sensor | AS7343 | I2C | 0x39 |
| OLED Display | SSD1306 128×64 | I2C | 0x3C |
| Radio (future) | LoRa (RadioHead) | SPI | — |

### GPIO Pin Mapping

| GPIO | Signal | Device |
|------|--------|--------|
| 21 | SDA | I2C (AS7343 + OLED) |
| 22 | SCL | I2C (AS7343 + OLED) |
| 5 | CS | LoRa (future) |
| 18 | SCK | LoRa SPI (future) |
| 23 | MOSI | LoRa SPI (future) |
| 19 | MISO | LoRa SPI (future) |

---

## 📊 AS7343 Channel Map

The AS7343 uses 3 SMUX passes to read all 12 channels via I2C. Channel index matches `readAllChannels()` output from the Adafruit library:

```
Index │ Name  │ Wavelength │ Color      │ Biological role
──────┼───────┼────────────┼────────────┼──────────────────────────────
  0   │ FZ    │  450 nm    │ Blue       │ Chlorophyll absorption
  1   │ FY    │  555 nm    │ Green-Yel  │ Green reflectance peak
  2   │ FXL   │  600 nm    │ Orange     │ Carotenoid / xanthophyll
  3   │ NIR   │  855 nm    │ Near-IR    │ NDVI NIR band, water content
  4   │ CLR1  │  broadband │ Clear      │ Pass 1 illumination
  5   │ FD1   │  —         │ Flicker    │ Pass 1 AC flicker detect
  6   │ F2    │  425 nm    │ Violet-Bl  │ Anthocyanin
  7   │ F3    │  475 nm    │ Blue-Cyan  │ Chlorophyll b / carotenoid
  8   │ F4    │  515 nm    │ Green      │ Carotenoid reference
  9   │ F6    │  640 nm    │ Red        │ Red reflectance
 10   │ CLR2  │  broadband │ Clear      │ Pass 2 illumination
 11   │ FD2   │  —         │ Flicker    │ Pass 2 AC flicker detect
 12   │ F1    │  405 nm    │ Violet     │ Anthocyanin
 13   │ F5    │  550 nm    │ Green      │ Vegetation green reflectance
 14   │ F7    │  690 nm    │ Deep Red   │ Chlorophyll absorption peak (NDVI Red)
 15   │ F8    │  745 nm    │ NIR shldr  │ Red edge / plant structure
 16   │ CLR   │  broadband │ Clear      │ Overall illumination (representative)
 17   │ FD    │  —         │ Flicker    │ AC flicker detect
```

---

## 📈 Vegetation Indices

All indices use calibrated channel values from `as7343_ch[]`. Formulas match `include/spectral_analysis.h`.

| Index | Channels Used | Formula | Typical Range |
|-------|--------------|---------|---------------|
| **NDVI** | NIR₈₅₅, F7₆₉₀ | (NIR − Red) / (NIR + Red) | −1 to +1 |
| **Chlorophyll CI** | NIR₈₅₅, F7₆₉₀ | (NIR / Red) − 1 | 0 to 5+ |
| **Anthocyanin ARI** | F5₅₅₀, F7₆₉₀ | 1/Green − 1/Red | ~−0.001 to +0.001 |
| **Water Stress** | NIR₈₅₅, FXL₆₀₀ | NIR / Orange | 0 to 3+ |
| **Red:NIR-shoulder** | F7₆₉₀, F8₇₄₅ | Red / NIR-shoulder | 0 to 2 |
| **Photosynthetic** | F3₄₇₅, F5₅₅₀, FXL₆₀₀, F7₆₉₀ | (Blue + Green) / (Orange + Red) | 0 to 2 |
| **Carotenoid** | F4₅₁₅, FXL₆₀₀ | Green₅₁₅ / Orange₆₀₀ | 0 to 2 |

### Health Level Scoring (0–5 scale)

| Level | Vigor (NDVI) | Chlorophyll (CI) | Stress (ARI) | Water (WSI) |
|-------|-------------|-----------------|--------------|-------------|
| 5 | > 0.7 | > 4.0 | > 0.0004 | > 5.0 |
| 4 | > 0.5 | > 3.0 | > 0.0003 | > 3.0 |
| 3 | > 0.3 | > 2.0 | > 0.0002 | > 1.5 |
| 2 | > 0.1 | > 0.5 | > 0.0001 | > 1.0 |
| 1 | > 0.0 | > 0.0 | > 0.0 | > 0.5 |
| 0 | ≤ 0.0 | ≤ 0.0 | ≤ 0.0 | ≤ 0.5 |

> **Stress note**: Stress level = 0 means ARI ≤ 0 (no anthocyanin — plant is **not** stressed). ARI > 0 indicates stress pigment expression.

---

## 🖥️ Serial Output Format

```
[DEBUG] FZ450:2999 FY555:2998 FXL600:2998 NIR855:1518 | F6_640:2413 F7_690:1467 F8_745:2302
[AS7343] FZ450:2999 FY555:2998 FXL600:2998 NIR855:1518 F2 425:2998 F3 475:2484 F4 515:2998 F6 640:2413 F1 405:1574 F5 550:2055 F7 690:1467 F8 745:2302

[VEGETATION INDICES]
  NDVI:                0.017
  Chlorophyll CI:      0.03
  Anthocyanin ARI:     -0.00020
  Water Stress:        0.51
  Red:NIR-shoulder:    0.64
  Photosynthetic:      1.02
  Carotenoid:          1.00
  Clear (Illumination):2999

[HEALTH] Vigor:1 Chlor:1 Stress:0 Water:1
```

> ⚠️ **Saturation warning**: Channels reading 2998–2999 are at ADC full-scale. Reduce sensor GAIN (`AGAIN`) or integration time (`ATIME`/`ASTEP`) for more dynamic range and accurate NDVI.

---

## 💾 OLED Display Layout

```
╔══ SPECTRAL ANALYSIS ══╗
║ NDVI: 0.017  CLR:2999  ║
║ Chl:  0.03  ARI:-0.000 ║
║ H2O:  0.51  R:FR: 0.64 ║
║ PAR:  1.02  Car:  1.00 ║
║────────────────────────║
║ V:1  Cl:1  S:0  W:1    ║
╚════════════════════════╝
```

---

## 🗂️ Project Structure

```
SpectrumAnalyzer/
├── src/
│   └── main.cpp                 # Main loop & orchestration
├── include/
│   ├── spectral_analysis.h      # Index calculations & health scoring
│   ├── as7343_sensor.h          # AS7343 driver (readAllChannels, calibration)
│   ├── oled_display.h           # SSD1306 display layout
│   ├── data_structures.h        # Shared types & enums
│   ├── hardware_init.h          # I2C / GPIO init
│   ├── debug_functions.h        # Serial debug helpers
│   ├── lora_config.h            # LoRa radio settings (future)
│   ├── lora_functions.h         # LoRa TX/RX (future)
│   ├── wifi_functions.h         # WiFi helpers (future)
│   └── node_config.h            # Multi-node ID config (future)
├── lib/                         # Local libraries
├── platformio.ini               # Build config
└── README.md
```

---

## ⚡ Quick Start

```bash
# Clone
git clone https://github.com/worrajak/ESP32_SpectralAnalyzer.git
cd ESP32_SpectralAnalyzer/SpectrumAnalyzer

# Build
pio run

# Upload (adjust port as needed)
pio run --target upload --upload-port COM9

# Monitor
pio device monitor --port COM9 --baud 115200
```

### platformio.ini

```ini
[env:freenove_esp32_wrover]
platform  = espressif32
board     = freenove_esp32_wrover
framework = arduino
monitor_speed = 115200
upload_speed  = 460800

lib_deps =
    adafruit/Adafruit SSD1306 @ ^2.5.0
    adafruit/Adafruit GFX Library @ ^1.11.0
    https://github.com/PaulStoffregen/RadioHead.git
    adafruit/Adafruit AS7343 @ ^1.0.0
```

---

## 🔧 Calibration

### Dark Calibration (auto on boot)
Runs automatically at startup — sensor reads with illumination off to establish offset baseline for all 18 channels.

### White Balance (manual, optional)
Uncomment the white-balance trigger in `src/main.cpp` and point the sensor at a white diffuser card under target illumination before taking measurements.

### Gain / Integration Time Tuning
If channels saturate (≥ 2998 ADC counts), lower the gain register (`AGAIN`) or shorten integration time (`ATIME`/`ASTEP`) in `include/as7343_sensor.h`.

---

## 🐛 Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| AS7343 not detected | Bad I2C wiring or no power | Check SDA=GPIO21, SCL=GPIO22, VCC=3.3V |
| OLED blank | Wrong I2C address | Verify address = 0x3C with I2C scanner |
| All channels ≈ 2999 (saturated) | Gain too high | Lower `AGAIN` or `ASTEP` in as7343_sensor.h |
| NDVI always ≈ 0 | Channels clipping | Same as above; need dynamic range |
| Serial not printing | Wrong baud rate | Set monitor to 115200 bps |
| Health levels all low | Light source too weak | Increase ambient light or use LED illumination |

---

## 📊 Version History

### v2.2 (2026-02-27)
- ✅ Verified all vegetation index formulas against live serial data
- ✅ Corrected channel wavelength table (actual AS7343 bands: 405–855 nm)
- ✅ Rewrote README — removed stale MPPT/DC Optimizer content
- ✅ Added sensor saturation warning and NDVI low-value interpretation guidance

### v2.1 (2026-02-01)
- ✅ Improved OLED display layout (multi-column, visual separator line)
- ✅ Health level scaling 0–5 for Vigor, Chlorophyll, Stress, Water

### v2.0 (2026-01-31)
- ✅ AS7343 spectral sensor integration complete
- ✅ Real-time NDVI and 6 additional vegetation indices
- ✅ SSD1306 OLED visualization
- ✅ Dark calibration on startup
- ✅ LoRa (RadioHead) library included for future expansion

### v1.0 (2026-01-15)
- ✅ Initial project scaffold

---

## 🔗 Resources

- [AS7343 Datasheet](https://ams.com/as7343)
- [Adafruit AS7343 Library](https://github.com/adafruit/Adafruit_AS7343)
- [PlatformIO ESP32 Docs](https://docs.platformio.org/en/latest/boards/espressif32/freenove_esp32_wrover.html)
- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)

---

## 📄 License

MIT License — see [LICENSE](LICENSE)

---

## 👤 Author

**Worrajak**  
GitHub: [@worrajak](https://github.com/worrajak)  
Repository: [ESP32_SpectralAnalyzer](https://github.com/worrajak/ESP32_SpectralAnalyzer)

---

**Last Updated**: February 27, 2026 | **Version**: 2.2
