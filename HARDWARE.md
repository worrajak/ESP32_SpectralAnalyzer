# Hardware Documentation

## Circuit Overview

This document provides detailed hardware specifications and design rationale for the ESP32 DC Optimizer with MPPT.

---

## Power Stage Design

### Synchronous Buck Converter Topology

**Why Synchronous Buck?**
- Step-down voltage conversion (Solar → Battery)
- High efficiency (>95%) compared to linear regulators
- Suitable for continuous high-current applications
- Reduced heat generation through synchronous rectification

### Switching MOSFETs

#### High-Side Switch (Q1, Q3)
- **Part**: IRF3205
- **Ratings**: 55V, 110A (pulsed), RDS(on) = 8mΩ
- **Driven by**: IRF2104EN gate driver
- **Function**: Main high-side switching element
- **Gate Resistor**: 10Ω (current limiting)

#### Low-Side Switch (Q2, Q4)
- **Part**: IRF3205
- **Ratings**: Same as high-side
- **Driven by**: IRF2104EN gate driver (complementary)
- **Function**: Synchronous rectification
- **Gate Resistor**: 10Ω (current limiting)

#### Catch Diode
- **Part**: SB3100
- **Ratings**: 30A, 100V
- **Function**: Freewheeling path for inductor current
- **Placement**: Parallel with low-side MOSFET (back-to-back)

### Gate Driver

**IRF2104EN Specifications:**
- Supply Voltage: 4.5V - 20V
- Output Current: ±2A peak
- Propagation Delay: ~40ns
- Dead-time: Inherent (10-20ns typical)
- Bootstrap Configuration for High-Side Drive

**Bootstrap Circuit:**
```
VCC ──── D1 ──── Q_HS Gate
            │
            C_boot
            │
           GND
```
- Diode (D1): 1N4007 (voltage isolation)
- Bootstrap Cap: 470µF (charge storage)
- Refresh interval: ~100µs

---

## Passive Components

### Inductors

#### Input Inductor (L1)
- **Value**: 100µH
- **Current Rating**: 30A continuous
- **Core**: Iron powder or ferrite
- **DCR**: <0.1Ω
- **Purpose**: Input current smoothing, reduces input ripple current
- **Effect**: Smoother power delivery from solar panel

#### Output Inductor (L2)
- **Value**: 100µH
- **Current Rating**: 40A continuous
- **Core**: Iron powder or ferrite
- **DCR**: <0.08Ω
- **Purpose**: Output current smoothing, reduces output ripple
- **Effect**: Cleaner power to battery

### Capacitors

#### Input Bulk Capacitor (C_in)
- **Value**: 470µF
- **Voltage**: 63V
- **Type**: Electrolytic
- **ESR**: <0.5Ω
- **Purpose**: Input filtering, surge suppression
- **Placement**: Close to input connector

#### High-Voltage Ceramic Bypass (C17, C18)
- **Value**: 0.1µF
- **Voltage**: 100V
- **Placement**: Near power MOSFETs
- **Purpose**: High-frequency decoupling

#### Bootstrap Capacitor (C_boot)
- **Value**: 470µF
- **Voltage**: 16V
- **Type**: Electrolytic
- **Purpose**: High-side gate driver supply
- **Refresh**: Every 100µs via D1

#### Output Filter Capacitor (C_out)
- **Value**: 100µF
- **Voltage**: 25V
- **ESR**: <0.5Ω
- **Purpose**: Output voltage ripple reduction
- **Effect**: Clean battery charging voltage

### Resistors

#### Current Sense Resistor
- **Part**: CSR1015GS (shunt resistor)
- **Value**: 10mΩ
- **Power Rating**: 10W continuous
- **Accuracy**: ±1%
- **Voltage Drop @ 30A**: 300mV
- **Output**: Connected to ADS1015 A2 (via divider)

#### Gate Resistors
- **Value**: 10Ω
- **Purpose**: Gate current limiting, ESD protection
- **Placement**: Between gate driver output and MOSFET gate
- **Effect**: Reduces EMI and prevents overshooting

#### Voltage Divider Resistors

**Solar Voltage Divider (A3):**
- R1: 200kΩ (high side)
- R2: 5.1kΩ (low side)
- Ratio: (200 + 5.1) / 5.1 = 40.2
- 60V input → 1.49V ADS1015 input
- Resistor Power: 1/4W (conservative design)
- Tolerance: 1% (for accuracy)

**Battery Voltage Divider (A1):**
- Similar design, adjusted for 0-25V range
- Ratio: ~3:1
- 25V input → 1.56V ADS1015 input

---

## Voltage Regulators

### 3.3V Buck Regulator (U6)

**Part**: XL7005A
- **Input**: 12V (from input conditioning)
- **Output**: 3.3V @ 500mA
- **Efficiency**: ~95%
- **Switching Frequency**: 500kHz (high frequency, small components)
- **Inductor (L3)**: 100µH
- **Input Cap (C17)**: 100µF
- **Output Cap (C10)**: 100µF
- **Output**: Powers ESP32 + ADS1015

### 5V Regulator (U3)

**Part**: AMS1117-5.0
- **Input**: 12V (linear regulator)
- **Output**: 5V @ 1A
- **Dropout**: ~1.3V
- **Purpose**: Powers peripheral circuits
- **Input Cap (C14)**: 100µF
- **Output Cap (C6)**: 100µF
- **Thermal Dissipation**: Can generate heat at high current

### 12V Buck Regulator (U5)

**Part**: XL7005A
- **Input**: Higher voltage input (when available)
- **Output**: 12V @ 1A
- **Purpose**: Optional external 12V supply
- **Similar design to 3.3V regulator**

---

## Sensing Circuits

### Voltage Sensing

#### Solar Voltage (A3)
**Signal Path:**
```
Solar Panel → Voltage Divider (R1, R2) → ADS1015 A3
```

**Calculation:**
- Maximum Input: 60V
- ADS1015 Max Safe: 3.3V (with ±4.096V range)
- Required Attenuation: 60V / 3.3V = 18.2:1
- Divider Ratio: (R1 + R2) / R2 = 40.2
- Adjustment: Series resistor or different divider values

#### Battery Voltage (A1)
**Signal Path:**
```
Battery → Voltage Divider → ADS1015 A1
```

**Calculation:**
- Maximum Input: 25V
- ADS1015 Safe Input: 3.3V
- Required Attenuation: 25V / 3.3V = 7.6:1
- Divider Design: 10kΩ high, 1.5kΩ low ≈ 7.67:1

### Current Sensing

#### Solar Current (A2)
**Signal Path:**
```
Solar Input → Shunt Resistor (CSR1015GS) → Current Sensor Amp → ADS1015 A2
```

**Conversion:**
- Shunt: 10mΩ @ 30A → 300mV drop
- Amplifier: INA series or similar (depending on circuit)
- Output: 0-3.3V corresponding to 0-30A

#### Battery Current (A0)
**Signal Path:**
```
Battery Output → Shunt Resistor → Current Sensor Amp → ADS1015 A0
```

**Similar design with lower current range (0-10A typical)**

---

## Control Circuit (ESP32)

### Microcontroller
**ESP32-WROVER**
- Dual-core CPU @ 240MHz
- 4MB PSRAM
- WiFi + Bluetooth
- ADC (internal, not used here)
- PWM capable (LED PWM controller)
- I2C, SPI, UART interfaces

### Pin Configuration

**Critical Pins:**
- GPIO14: PWM_HIGH (20kHz, 12-bit)
- GPIO27: PWM_LOW (20kHz, 12-bit)
- GPIO2: I2C SCL (ADS1015)
- GPIO4: I2C SDA (ADS1015)
- GPIO19: UART TX
- GPIO23: UART RX

### Power Supply
- Input: 3.3V from XL7005A regulator
- Decoupling: 100nF + 10µF per power pin
- Current: ~100mA typical, 300mA peak

---

## I2C Interface (ADS1015)

### ADS1015 External ADC

**Key Features:**
- 12-bit resolution
- 4 single-ended or 2 differential inputs
- ±4.096V range (our configuration)
- Programmable gain amplifier
- I2C Interface (up to 400kHz)
- I2C Address: 0x48 (default)

**Pin Configuration:**
```
ADS1015 Pin Layout (MSOP-10):
├─ A0: Solar/Battery Current
├─ A1: Solar/Battery Voltage
├─ A2: Solar/Battery Current
├─ A3: Solar/Battery Voltage
├─ GND: Ground
├─ VDD: +3.3V power
├─ SCL: I2C Clock (GPIO2)
├─ SDA: I2C Data (GPIO4)
└─ ADDR: I2C Address select (tied to GND for 0x48)
```

### I2C Pull-Up Resistors
- **SCL Pull-up**: 4.7kΩ to 3.3V
- **SDA Pull-up**: 4.7kΩ to 3.3V
- **Decoupling**: 100nF ceramic cap on VDD

### I2C Communication Speed
- Clock Frequency: 100kHz (standard mode) to 400kHz (fast mode)
- Our firmware: 400kHz for faster reading

---

## Protection Circuits

### Input Protection
```
Solar Panel → Diode (reverse polarity) → Fuse (30A) → Inductor L1 → MOSFET
```

**Fuse (F1)**
- Type: 30A slow-blow automotive fuse
- Rating: For 30A continuous current
- Purpose: Protect against short circuits
- Breaking capacity: >500A

**Reverse Polarity Diode**
- Part: 1N4007 or similar
- Rating: 1A, 1000V minimum
- Placement: Across input with correct polarity
- Purpose: Prevent damage from reversed panel connection

**Input Inductor (L1)**
- Acts as EMI filter
- Reduces input ripple current
- Smooth current draw from solar panel

### Output Protection
```
Output → Inductor L2 → Filter Cap → Battery Disconnect
```

**Battery Disconnect**
- Manual switch recommended
- Prevents accidental short-circuit
- Allows safe maintenance

### Over-Current Protection
- Monitored via A2 (solar current)
- Software-based shutdown at 35A
- Hardware fuse as secondary protection

### Over-Voltage Protection
- Monitored via A3 (solar voltage)
- Software shutdown if >65V
- Input transient suppression

---

## Thermal Management

### Power Loss Analysis

**MOSFET Switching Losses:**
- RDS(on) loss: I²R @ 20kHz switching
- Example @ 20A: 20² × 0.008Ω = 3.2W
- Relatively small at high efficiency

**Inductor Losses:**
- Core loss + copper loss
- L1 & L2 combined: ~1-2W
- Critical for large inductors

**Gate Drive Loss:**
- ~100mW for 2 MOSFETs @ 20kHz
- Minor contribution

**Total Expected Loss:**
- At 20A output: ~5-10W
- Heatsink required for MOSFETs
- Passive cooling usually sufficient

### Heatsink Design
- Mount high and low-side MOSFETs on heatsinks
- Recommended: Aluminum plate, 0.5°C/W minimum
- Thermal paste: Standard compound
- Target: <60°C at 25°C ambient + 20A load

---

## Wiring & Layout

### PCB Design Considerations

**Power Traces:**
- Solar input: 10mm (≥4oz copper)
- Output to battery: 8mm (≥4oz copper)
- Ground plane: Continuous throughout
- Star grounding at battery

**Signal Traces:**
- I2C SCL/SDA: 0.2mm, twisted pair
- PWM signals: 0.5mm, short and direct
- UART: 0.3mm, away from power traces

**Layer Stackup (4-layer recommended):**
1. Top: Components + signals
2. GND plane (continuous)
3. Power distribution (12V, 5V, 3.3V)
4. Bottom: Return paths + signals

**Critical Distances:**
- Gate drivers: <5cm from MOSFETs
- Bootstrap cap: <2cm from gate driver
- ADS1015: <5cm from ESP32
- I2C pull-ups: Near ESP32

---

## Bill of Materials (BOM)

See [BOM.csv](BOM.csv) for complete parts list with suppliers and costs.

---

## References

- [Synchronous Buck Design Guide](https://www.analog.com/en/analog-dialogue/articles/synchronous-buck-converter-design.html)
- [MOSFET Gate Drive Design](https://www.infineon.com/dgdl/irf2104.pdf)
- [ADC Selection and Design](https://www.ti.com/lit/an/sbaa110/sbaa110.pdf)
- [High Frequency PCB Design](https://www.analog.com/en/analog-dialogue/articles/pcb-design-techniques.html)

