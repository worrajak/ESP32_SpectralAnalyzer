# ESP32 Spectrum Analyzer with OLED Display

🌱 **Real-time Plant Health Monitoring using Spectral Analysis**

[![GitHub](https://img.shields.io/badge/GitHub-ESP32_SpectralAnalyzer-blue?logo=github)](https://github.com/worrajak/ESP32_SpectralAnalyzer)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-PlatformIO-orange)](https://platformio.org/)
[![Status](https://img.shields.io/badge/Status-Active-brightgreen)](https://github.com/worrajak/ESP32_SpectralAnalyzer)

## 🔋 Project Overview

This project implements a **Real-time Spectral Analysis System** using the **ESP32 microcontroller** with the **AS7343 11-channel spectral sensor** and an **SSD1306 OLED display** for real-time visualization. The system measures plant health indicators through spectral data analysis including NDVI, Chlorophyll levels, Water stress, and Photosynthesis indices.

### Key Features
- ✅ **AS7343 Spectral Sensor**: 11-channel multi-spectral measurements
- ✅ **Real-time NDVI Calculation**: Vegetation health index tracking
- ✅ **Multiple Plant Health Indices**: Chlorophyll, Anthocyanin, Water Stress, Photosynthesis, Carotenoid, Red:FarRed ratio
- ✅ **Health Level Scaling**: 0-5 scale for easy interpretation (Vigor, Chlorophyll, Stress, Water levels)
- ✅ **SSD1306 OLED Display**: 128×64 pixel real-time data visualization
- ✅ **Improved UI Layout**: Clean organized display with:
  - Header with section title
  - Multi-column data organization
  - Visual separator line
  - Health status indicator
- ✅ **Precise Spectral Analysis**: Dark calibration and white balance correction
- ✅ **I2C Communication**: AS7343 sensor (address: 0x39) with burst mode support
- ✅ **Serial Monitoring**: Real-time data output for debugging and logging
- ✅ **LoRa Radio Ready**: RadioHead library integrated for wireless capability

---

## 📋 Hardware Architecture

### Core Components

#### **Main Microcontroller**
- **Freenove ESP32 WROVER**
  - Dual-core 240 MHz Xtensa CPU
  - 4MB PSRAM, 4MB Flash
  - WiFi & Bluetooth connectivity
  - 12-bit SAR ADC (not used - external ADC preferred)

#### **Spectral Sensor**
- **AS7343 11-channel Spectral Sensor**
  - I2C Address: 0x39
  - Channels: 415nm, 445nm, 480nm, 515nm, 555nm, 590nm, 630nm, 680nm, 780nm, 860nm, 910nm
  - Provides full spectrum coverage for vegetation analysis
  - Built-in LED control for consistent measurements
  - Supports dark calibration and white balance

#### **Display**
- **SSD1306 OLED Display**
  - I2C Address: 0x3C
  - Resolution: 128 × 64 pixels
  - Real-time spectral data visualization
  - Improved UI with organized layout

#### **Communication**
- **RadioHead Library**
  - LoRa radio support ready
  - Multi-node capability
  - Wireless sensor network potential

---

## 🎛️ GPIO Pin Mapping

### I2C Bus (AS7343 & SSD1306)
| GPIO | Signal | Function |
|------|--------|----------|
| 22 (GPIO22) | SCL | I2C Clock |
| 21 (GPIO21) | SDA | I2C Data |

### SPI Bus (LoRa Radio - Future)
| GPIO | Signal | Function |
|------|--------|----------|
| 5 | CS | Chip Select |
| 18 | SCK | Clock |
| 23 | MOSI | Master Out |
| 19 | MISO | Master In |

### Interrupt Pins
| GPIO | Signal | Function |
|------|--------|----------|
| 15 | LoRa DIO0 | Interrupt (when implemented) |

---

## 📊 AS7343 Sensor Channels

The AS7343 provides 11 spectral channels for complete vegetation analysis:

```
┌───────────────────────────────────────────────────────┐
│        AS7343 11-Channel Spectral Sensor              │
│   Address: 0x39 (I2C) - 400kHz standard mode          │
├───────────────────────────────────────────────────────┤
│ Channel │ Wavelength │ Color    │ Application         │
├─────────┼────────────┼──────────┼─────────────────────┤
│   CH0   │ 415 nm     │ Violet   │ Anthocyanin         │
│   CH1   │ 445 nm     │ Blue     │ Chlorophyll A       │
│   CH2   │ 480 nm     │ Blue     │ Chlorophyll B       │
│   CH3   │ 515 nm     │ Green    │ Carotenoid          │
│   CH4   │ 555 nm     │ Green    │ Green Reflectance   │
│   CH5   │ 590 nm     │ Yellow   │ Carotenoid/Xantho   │
│   CH6   │ 630 nm     │ Red      │ NDVI Red (Primary)  │
│   CH7   │ 680 nm     │ Red      │ Chlorophyll Edge    │
│   CH8   │ 780 nm     │ NIR      │ NDVI NIR (Primary)  │
│   CH9   │ 860 nm     │ NIR      │ Water Absorption    │
│  CH10   │ 910 nm     │ NIR      │ Water Absorption 2  │
│ CLEAR   │ Broadband  │ White    │ Overall Intensity   │
└───────────────────────────────────────────────────────┘
```

---

## 📈 Spectral Indices Calculated
  - Divider: Similar ratio design

- **A2 (Solar Current)**: 0-30A input
  - Sensed via current sensor shunt
  - Scale: ~0.002A per ADC step
  
- **A0 (Battery Current)**: 0-10A input
  - Sensed via current sensor shunt
  - Scale: ~0.003A per ADC step

---

## 🔧 Software Architecture

### Main Components

#### **MPPT Algorithm: Perturb & Observe (P&O)**
```
1. Measure Solar Power (P) and Voltage (V)
2. Compare with previous measurement
3. If Power increased:
   - Continue in same direction
   - Increase/Decrease Duty Cycle accordingly
4. If Power decreased:
   - Reverse direction
   - Adjust Duty Cycle opposite
5. Repeat every 500ms
```

#### **Control Loop Timing**
| Task | Interval | Purpose |
|------|----------|---------|
| ADC Sampling | 100ms | Read all sensor values |
| MPPT Update | 500ms | Calculate new duty cycle |
| Serial Output | 500ms | Display system status |
| Debug Output | 5 seconds | Raw ADC values |

#### **Duty Cycle Control**
- **Range**: 5% to 95% (constrained for safety)
- **Resolution**: 12-bit (0-4095)
- **PWM Frequency**: 20 kHz
- **Dead-time**: Inherent in LED PWM controller

#### **Safety Features**
- Duty cycle limits prevent saturation
- Over-current monitoring (configurable)
- Under-voltage lockout (configurable)
- Thermal shutdown (can be added)

---

## 📈 Spectral Indices Calculated

The system calculates multiple vegetation health indices for comprehensive plant analysis:

| Index | Formula | Range | Application |
|-------|---------|-------|-------------|
| **NDVI** | (NIR - RED) / (NIR + RED) | -1 to +1 | Overall vegetation vigor |
| **Chlorophyll Index** | (NIR / RED) - 1 | 0-3+ | Leaf chlorophyll content |
| **Anthocyanin Index** | (515 - 415) / (515 + 415) | 0-1 | Stress/pigment indicator |
| **Water Stress Index** | (NIR - GREEN) / (NIR + GREEN) | -1 to +1 | Plant water status |
| **Red:FarRed Ratio** | RED / NIR | 0-1 | Photosynthetic capacity |
| **Photosynthesis Index** | (NIR - BLUE) / (NIR + BLUE) | -1 to +1 | Active photosynthesis |
| **Carotenoid Index** | (Yellow - Blue) / (Yellow + Blue) | -1 to +1 | Stress pigments |

### Health Level Scale (0-5)
Each index is mapped to a health level (0-5) for easy interpretation:

- **0**: Critical (severe stress/disease)
- **1**: Poor (significant issues detected)
- **2**: Fair (below optimal)
- **3**: Good (healthy)
- **4**: Very Good (excellent condition)
- **5**: Excellent (peak health)

---

## 💾 OLED Display Features

The 128×64 pixel OLED display shows real-time spectral analysis with an improved layout:

```
═══ SPECTRAL ANALYSIS ═══
NDVI: 0.75  Clear: 1024
Chlor: 2.34 Anth: 0.42
Water: 0.68 R:FR: 0.52
Photo: 0.71 Car: 0.28
─────────────────────────
Health:V:4 C:4 S:3 W:4
Status: OK
```

**Display Features:**
- Real-time data updates (every 1 second)
- 2-column organized layout for data density
- Visual separator line for clarity
- Health level indicators (0-5 scale)
- Sensor status indicator
- Precise 2-decimal formatting

---

## ⚡ Quick Start

```bash
# Clone the repository
git clone https://github.com/worrajak/ESP32_SpectralAnalyzer.git
cd ESP32_SpectralAnalyzer

# Build and upload to ESP32 on COM9
pio run --target upload --upload-port COM9

# Monitor serial output
pio device monitor --port COM9 --baud 115200
```

---

## 🚀 Getting Started

### Prerequisites
- PlatformIO IDE with ESP32 support
- Adafruit SSD1306 library (v2.5.0+)
- Adafruit GFX Library (v1.11.0+)
- RadioHead library (latest)
- ESP32 Freenove WROVER board

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/worrajak/ESP32_DCOptimizer.git
   cd SpectrumAnalyzer
   ```

2. **Install dependencies** (automatic with PlatformIO):
   ```bash
   pio run
   ```

3. **Configure the board** (platformio.ini):
   ```ini
   [env:freenove_esp32_wrover]
   platform = espressif32
   board = freenove_esp32_wrover
   framework = arduino
   monitor_speed = 115200
   upload_speed = 460800
   ```

4. **Build and upload**:
   ```bash
   pio run --target upload --upload-port COM9
   ```

5. **Monitor serial output**:
   ```bash
   pio device monitor --port COM9 --baud 115200
   ```

---

## 📝 Usage

### Main Loop Operation
```cpp
// Main operation cycle:
1. Read AS7343 spectral sensor
2. Apply dark calibration & white balance
3. Calculate all vegetation indices
4. Calculate health levels (0-5)
5. Display data on OLED
6. Output to Serial for logging
```

### Serial Output Format
```
NDVI: 0.75  Chlor: 2.34  Anth: 0.42  Water: 0.68
R:FR: 0.52  Photo: 0.71  Car: 0.28
Health - V:4 C:4 S:3 W:4
```

---

## 🔧 Configuration

### Sensor Calibration
Dark calibration is performed on startup to eliminate sensor offset. White balance can be manually triggered by uncommenting in the code.

### Display Update Rate
Modify `UPDATE_INTERVAL` in `src/main.cpp` (default: 1000ms)

### Sensor Read Rate  
Modify `SENSOR_READ_INTERVAL` in `src/main.cpp` (default: 500ms)

---

## 📁 Project Structure

```
SpectrumAnalyzer/
├── src/
│   └── main.cpp                 # Main program
├── include/
│   ├── as7343_sensor.h          # AS7343 sensor driver
│   ├── oled_display.h           # OLED display functions
│   ├── spectral_analysis.h      # Index calculations
│   ├── data_structures.h        # Data types & structures
│   ├── hardware_init.h          # Hardware initialization
│   ├── lora_config.h            # LoRa configuration
│   └── lora_functions.h         # LoRa functions
├── lib/
│   └── (third-party libraries)
├── platformio.ini               # PlatformIO configuration
└── README.md                    # This file
```

---

## 🐛 Troubleshooting

### AS7343 Not Detected
- Verify I2C connection (SCL on GPIO22, SDA on GPIO21)
- Check I2C address with I2C scanner
- Ensure sensor is powered (3.3V)

### OLED Display Not Showing
- Verify I2C address is 0x3C
- Check I2C bus voltage (should be 3.3V)
- Try I2C scanner to confirm detection

### Serial Output Not Visible
- Ensure USB cable is connected
- Check UART pins (TX: GPIO19, RX: GPIO23)
- Set baud rate to 115200

### Poor Spectral Readings
- Check sensor lens is clean
- Ensure adequate ambient/LED light
- Verify dark calibration was performed
- Check white balance calibration

---

## 📊 Version History

### v2.1 (2026-02-01)
- ✅ Improved OLED display layout with better alignment
- ✅ Multi-column data organization for clarity
- ✅ Visual separator line for section division
- ✅ Updated README with spectral analysis documentation
- ✅ Health level scaling 0-5 for easy interpretation

### v2.0 (2026-01-31)
- ✅ AS7343 spectral sensor integration
- ✅ Real-time NDVI and health indices
- ✅ SSD1306 OLED display support
- ✅ LoRa radio support ready
- ✅ Serial data logging

---

## 📄 License

This project is open source and available under the MIT License.

---

## 👤 Author

**Worrajak**
- GitHub: [@worrajak](https://github.com/worrajak)
- Project: [ESP32_DCOptimizer](https://github.com/worrajak/ESP32_DCOptimizer)

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

### Prerequisites
- PlatformIO IDE with ESP32 support
- Adafruit SSD1306 library (v2.5.0+)
- Adafruit GFX Library (v1.11.0+)
- RadioHead library (latest)
- ESP32 Freenove WROVER board

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/worrajak/ESP32_DCOptimizer.git
   cd SpectrumAnalyzer
   ```bash
   pio run
   ```

4. **Upload to ESP32**:
   ```bash
   pio run -t upload
   ```

5. **Monitor serial output**:
   ```bash
   pio device monitor -b 115200
   ```

---

## 📝 Serial Monitor Output

The system prints status information every 500ms:

```
================================================
Solar Panel: 36.45V | 5.234A | 190.45W
Battery:     12.85V | 14.56A | 187.18W
Duty Cycle:  42.3% | Efficiency: 98.3%
================================================

--- RAW ADC VALUES (Debug) ---
A0 (Batt I): 1456 | A1 (Batt V): 1285 | A2 (Solar I): 523 | A3 (Solar V): 1818
```

### Interpretation
- **Duty Cycle**: Percentage of time high-side MOSFET is ON
- **Efficiency**: Battery Power / Solar Power × 100%
- **Raw ADC Values**: For calibration and troubleshooting

---

## 🔌 Wiring Diagram Summary

### Input Side (Solar Panel)
```
Solar Panel (+) → Fuse (30A) → Input Protection → High-side MOSFET → Inductor L1
Solar Panel (-) → Ground
```

### Output Side (Battery)
```
Synchronous Buck Output → Inductor L2 → Battery (+)
GND → Battery (-)
```

### ESP32 Connections
```
ADS1015:
  GPIO2  → SCL (I2C Clock)
  GPIO4  → SDA (I2C Data)
  GND    → GND
  +3.3V  → VCC (from 3.3V regulator)

PWM Control:
  GPIO14 → High-side FET driver
  GPIO27 → Low-side FET driver

Serial (optional):
  GPIO19 → TX to USB adapter
  GPIO23 → RX from USB adapter
```

---

## 📊 Calibration Guide

### ADC Calibration Steps

1. **Measure reference voltages**:
   - Connect known voltage source to A3 (Solar V)
   - Record raw ADC value and actual voltage
   - Calculate: `ADC_SOLAR_V_SCALE = Actual_V / ADC_Raw`

2. **Repeat for all channels**:
   - A0: Battery Current
   - A1: Battery Voltage
   - A2: Solar Current

3. **Update calibration constants** in `main.cpp`:
   ```cpp
   #define ADC_SOLAR_V_SCALE    0.0300  // Adjust based on measurements
   #define ADC_SOLAR_I_SCALE    0.00200
   #define ADC_BATT_V_SCALE     0.0150
   #define ADC_BATT_I_SCALE     0.00300
   ```

### MPPT Fine-tuning

- **MPPT_STEP**: Larger value = faster tracking, higher oscillation
  - Default: 5 (good for most applications)
  - Increase for faster changing conditions
  - Decrease for stable power source

- **MPPT_INTERVAL**: Time between MPPT updates
  - Default: 500ms (standard)
  - Decrease for variable sun conditions
  - Increase for steady-state operation

---

## 🐛 Troubleshooting

### ADS1015 Not Found
```
ERROR: ADS1015 not found at address 0x48!
```
- Check I2C connections (GPIO2=SCL, GPIO4=SDA)
- Verify I2C address with I2C scanner
- Check 3.3V power supply to ADS1015

### Incorrect ADC Readings
- Run calibration procedure (see above)
- Check voltage divider resistor values
- Verify current sensor connections
- Check for loose connections

### MPPT Not Tracking
- Verify PWM output on GPIO14 and GPIO27
- Check MOSFET gate voltage
- Monitor solar panel voltage changes
- Verify ADC readings are changing

### Low Efficiency
- Check for high parasitic losses
- Verify MOSFET switching frequency
- Monitor inductor saturation
- Check for proper dead-time between high/low side

---

## 📚 References

### Datasheets
- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [ADS1015 Datasheet](https://www.ti.com/lit/ds/symlink/ads1015.pdf)
- [IRF2104 Gate Driver](https://www.infineon.com/dgdl/irf2104.pdf)
- [IRF3205 Power MOSFET](https://www.infineon.com/dgdl/irf3205.pdf)

### MPPT Algorithms
- Perturb & Observe Method (P&O)
- Incremental Conductance
- Fuzzy Logic Control
- Artificial Neural Networks

### Synchronous Buck Converter
- High efficiency for step-down regulation
- Reduced heat generation
- Suitable for high-current applications
- Common in solar MPPT systems

---

## 🔒 Safety Considerations

⚠️ **Important**: This is a high-current power system. Proper safety precautions must be taken:

1. **Input Protection**:
   - Always use appropriate fusing (30A in this design)
   - Include reverse-polarity protection diode
   - Add surge suppression on input

2. **Output Protection**:
   - Battery should have proper disconnect switch
   - Include over-charge protection circuit
   - Monitor for thermal runaway

3. **Electrical Safety**:
   - Use proper gauge wiring
   - Ensure adequate ventilation for power MOSFETs
   - Add heatsinks if necessary
   - Never touch live circuits

4. **Software Safety**:
   - Implement watchdog timer (optional)
   - Add thermal shutdown protection
   - Include current limiting logic
   - Log error conditions

---

## 📝 License & Credits

Project: ESP32 DC Optimizer with MPPT
Author: worrajak
License: MIT (or your chosen license)

**Based on Synchronous Buck Converter Design**
- Reference: Instructables MPPT Solar Charge Controller

---

## 🤝 Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

---

## 📧 Support

For issues, questions, or suggestions:
- Open an issue on GitHub
- Check the troubleshooting section
- Review the calibration guide
- Consult component datasheets

---

## 🎯 Future Enhancements

- [ ] WiFi remote monitoring and control
- [ ] Bluetooth mobile app integration
- [ ] SD card data logging
- [ ] Temperature compensation
- [ ] Multiple battery chemistry support
- [ ] Network synchronization for multi-unit systems
- [ ] Web interface dashboard
- [ ] Email alerts for fault conditions

---

## 📋 Project Versions & Architectures

### Version 1.0: Single-Node MPPT
- Single ESP32 with ADS1015 ADC
- Direct battery charging from solar panel
- Basic MPPT with local control only
- Simple serial monitoring

### Version 2.0: Multi-Node Distributed System (Current)
- **Master Controller**: Central intelligence with WiFi/ESP-NOW
- **4× Node Controllers**: Individual nodes in series configuration
- **Architecture**: 
  ```
  Panel 1 → Node 1 (12V) ┐
  Panel 2 → Node 2 (12V) ├→ Series Chain → 48V Battery
  Panel 3 → Node 3 (12V) ├→ (with Bypass Diodes)
  Panel 4 → Node 4 (12V) ┘
  ```

- **Wireless Protocol**: ESP-NOW (2.4 GHz mesh)
- **Control Levels**:
  1. System-level optimization (master directs overall voltage)
  2. Node balancing (ensure all nodes at same voltage)
  3. Individual MPPT (each node tracks its own MPP within constraints)

- **Fault Detection & Compensation**:
  - Master detects failed nodes (offline, no power, voltage collapse)
  - Automatically recalculates voltage: 48V ÷ (number of working nodes)
  - Example: If 1 node fails → 48V ÷ 3 = 16V per remaining node
  - Includes 6-level fault status codes (NORMAL, SHADING, SOFT_FAULT, HARD_FAULT, etc.)
  - Bypass diodes prevent blocking of series chain

---

## 🔌 Multi-Node System Components

### Master Controller
- Purpose: Central control and optimization
- Location: `src/master_controller.cpp`
- Functions:
  - Broadcasts voltage setpoints every 2 seconds
  - Monitors 4 node status reports
  - Implements 3-level voltage optimization
  - Detects node failures (offline, power loss, shading)
  - Calculates automatic voltage compensation
  - Reports system state and node diagnostics

### Node Controller (×4)
- Purpose: Individual panel control with local MPPT
- Location: `src/node_controller.cpp`
- Functions:
  - Receives voltage setpoint from master
  - Performs local MPPT within master's constraints
  - Measures voltage, current, power via ADS1015
  - Detects local faults (voltage collapse, power loss, shading)
  - Reports status to master every 2 seconds

### Enhanced Fault Detection
- Location: `src/master_enhancements.cpp` and `src/node_enhancements.cpp`
- Features:
  - Multi-level fault detection (soft vs hard faults)
  - Power trend analysis (compares current vs previous power)
  - Voltage/current collapse detection
  - Automatic compensation calculation
  - Serial diagnostic output

### Integration Guide
- Location: `INTEGRATION_GUIDE.md`
- Covers:
  - Step-by-step code integration instructions
  - Hardware bypass diode specifications
  - Testing procedures for fault scenarios
  - Serial output examples
  - Troubleshooting guide

---

## 🛡️ Fault Detection Thresholds

| Condition | Threshold | Action |
|-----------|-----------|--------|
| Node Offline (no response) | >5 seconds | Emergency stop or compensation |
| Voltage Collapse | <5V | Mark as HARD_FAULT |
| Power Loss | <1W | Mark as HARD_FAULT |
| Current Collapse | <0.5A | Mark as HARD_FAULT |
| Power Drop | >90% from baseline | SOFT_FAULT or HARD_FAULT |
| Voltage Imbalance | >1V difference | Adjust master voltage |
| Efficiency Drop | <80% | Reduce voltage setpoint |

---

## 📊 Node Status Codes

```cpp
#define NODE_STATUS_NORMAL        0     // Operating normally
#define NODE_STATUS_SHADING       1     // Partial shading detected
#define NODE_STATUS_OVERVOLTAGE   2     // Output voltage too high
#define NODE_STATUS_OVERCURRENT   3     // Output current too high
#define NODE_STATUS_SOFT_FAULT    254   // Severe degradation (90% power loss)
#define NODE_STATUS_HARD_FAULT    255   // Complete failure (no power)
```

---

## 📊 Node Status Codes

```cpp
#define NODE_STATUS_NORMAL        0     // Operating normally
#define NODE_STATUS_SHADING       1     // Partial shading detected
#define NODE_STATUS_OVERVOLTAGE   2     // Output voltage too high
#define NODE_STATUS_OVERCURRENT   3     // Output current too high
#define NODE_STATUS_SOFT_FAULT    254   // Severe degradation (90% power loss)
#define NODE_STATUS_HARD_FAULT    255   // Complete failure (no power)
```

---

## 📝 Recent Updates

- **v2.1** (2026-02-01): Improved OLED display layout with multi-column organization
- **v2.0** (2026-01-31): AS7343 spectral sensor integration complete
- **v1.0** (2026-01-15): Initial project setup

---

## 🔗 Links & Resources

- **GitHub Repository**: [ESP32_SpectralAnalyzer](https://github.com/worrajak/ESP32_SpectralAnalyzer)
- **PlatformIO**: [platformio.org](https://platformio.org/)
- **AS7343 Datasheet**: [Spectral Sensor Specifications](https://ams.com/as7343)
- **ESP32 Documentation**: [espressif.com](https://www.espressif.com/)

---

**Last Updated**: February 1, 2026  
**Version**: 2.1  
**Repository**: [ESP32_SpectralAnalyzer](https://github.com/worrajak/ESP32_SpectralAnalyzer)
