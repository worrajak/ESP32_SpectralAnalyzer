# ESP32 DC Optimizer with MPPT

## 🔋 Project Overview

This project implements a **Maximum Power Point Tracking (MPPT) Solar Charge Controller** using the **ESP32 microcontroller** with a **Synchronous Buck Converter** topology. The system efficiently charges batteries from solar panels by continuously tracking and maintaining the maximum power point of the PV array.

### Key Features
- ✅ **MPPT Algorithm**: Perturb & Observe (P&O) method
- ✅ **ADS1015 External ADC**: 12-bit precision for voltage/current measurements
- ✅ **Synchronous Buck Converter**: Efficient DC-DC step-down with low losses
- ✅ **Real-time Monitoring**: Solar panel and battery parameters
- ✅ **20 kHz PWM Control**: For quiet, efficient operation
- ✅ **Efficiency Calculation**: Real-time system efficiency tracking
- ✅ **Over-current/Over-voltage Protection**: Built-in safety features

---

## 📋 Hardware Architecture

### Block Diagram
```
Solar Panel (Up to 60V)
        ↓
[Input Protection] ← Fuse (F1: 30A)
        ↓
[MOSFET Driver] ← High-side (Q1, Q3) & Low-side (Q2, Q4)
        ↓
[Synchronous Buck Converter]
        ↓
[Output Inductor] → [Output Filter Capacitor]
        ↓
[Battery] (12V nominal)
```

### Circuit Components

#### **Power Stage**
| Component | Part Number | Function |
|-----------|------------|----------|
| High-side MOSFET | IRF2104EN | Gate drive for high-side FET |
| Low-side MOSFET | IRF2104EN | Gate drive for low-side FET |
| Power MOSFETs | IRF3205 | Main switching elements (Q1-Q4) |
| Catch Diode | SB3100 | Freewheeling diode |
| Input Inductor (L1) | 100µH | Input current smoothing |
| Output Inductor (L2) | 100µH | Output current smoothing |
| Input Capacitor | 470µF | Input filtering |
| Output Capacitor | 100µF | Output filtering |

#### **Voltage Regulators**
- **3.3V Buck Regulator** (XL7005A): Powers ESP32 and ADS1015
- **5V Regulator** (AMS1117-5.0): Powers peripheral circuits
- **12V Regulator** (Optional): For external devices

#### **Sensing Circuit**
| Signal | Input | Sensor | Scale |
|--------|-------|--------|-------|
| Solar Voltage (A3) | 0-60V | Resistive Divider (R1, R2) | ~0.03V/step |
| Solar Current (A2) | 0-30A | Current Sense (CSR1015GS) | ~0.002A/step |
| Battery Voltage (A1) | 0-25V | Resistive Divider | ~0.015V/step |
| Battery Current (A0) | 0-10A | Current Sense | ~0.003A/step |

#### **Control & Communication**
- **ESP32 Microcontroller** (Freenove WROVER)
  - 2-core, 240 MHz CPU
  - WiFi & Bluetooth capabilities
  - 12-bit ADC (internal, not used in this design)
  
- **ADS1015 External ADC** (I2C address: 0x48)
  - 12-bit resolution
  - 4 Single-ended inputs
  - ±4.096V range
  - I2C Communication (GPIO2=SCL, GPIO4=SDA)

- **USB-TTL Serial Interface** (CH340)
  - For programming and serial monitoring
  - GPIO19 (TX), GPIO23 (RX)

---

## 🎛️ GPIO Pin Mapping

### PWM Control (Synchronous Buck)
| GPIO | Signal | Function |
|------|--------|----------|
| 14 | PWM_HIGH | High-side FET drive (20 kHz) |
| 27 | PWM_LOW | Low-side FET drive (complementary) |

### I2C Interface (ADS1015)
| GPIO | Signal | Function |
|------|--------|----------|
| 2 | SCL | I2C Clock |
| 4 | SDA | I2C Data |

### UART Interface
| GPIO | Signal | Function |
|------|--------|----------|
| 19 | TX | Serial TX |
| 23 | RX | Serial RX |

### Interface Ports (for future expansion)
| GPIO | Port | Function |
|------|------|----------|
| 27 | GPIO Interface Port | Spare GPIO |
| 14 | GPIO Interface Port | Spare GPIO |
| 10, 9, 17, 16 | Interface Port | Additional expansion |

---

## 📊 ADS1015 ADC Channel Assignment

The ADS1015 has 4 single-ended input channels for measuring system parameters:

```
┌─────────────────────────────────────┐
│      ADS1015 (12-bit ADC)           │
│   Address: 0x48 (I2C)               │
├─────────────────────────────────────┤
│  Channel  │  Signal          │ Use   │
├───────────┼──────────────────┼───────┤
│    A0     │ Batt Current     │ Output│
│    A1     │ Batt Voltage     │ Output│
│    A2     │ Solar Current    │ Input │
│    A3     │ Solar Voltage    │ Input │
└─────────────────────────────────────┘
```

### Voltage Scaling
- **A3 (Solar Voltage)**: 0-60V input → 0-2047 ADC counts
  - Scale: 0.03V per ADC step
  - Divider: R1 (200kΩ) + R2 (5.1kΩ)
  
- **A1 (Battery Voltage)**: 0-25V input → 0-2047 ADC counts
  - Scale: 0.015V per ADC step
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

## 📈 System Parameters

### Electrical Specifications
| Parameter | Min | Typical | Max | Unit |
|-----------|-----|---------|-----|------|
| Input Voltage (Solar) | 12 | 36 | 60 | V |
| Output Voltage (Battery) | 10 | 12.6 | 16 | V |
| Input Current | 0 | 15 | 30 | A |
| Output Current | 0 | 20 | 40 | A |
| Switching Frequency | - | 20 | - | kHz |
| Efficiency @ Full Load | - | >95 | - | % |

### Thermal
- Operating Temperature: -10°C to +60°C
- Switching losses are minimal due to synchronous topology

---

## 🚀 Getting Started

### Prerequisites
- PlatformIO IDE with ESP32 support
- Adafruit ADS1X15 library (v2.4.0+)
- Adafruit BusIO library (v1.14.5+)

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/worrajak/ESP32_DCOptimizer.git
   cd "DC Optimizer with MPPT"
   ```

2. **Install dependencies** (automatic with PlatformIO):
   ```bash
   pio run -t download
   ```

3. **Build the project**:
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

**Last Updated**: January 8, 2026
**Version**: 1.0.0
