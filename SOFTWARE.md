# Software Documentation

## Overview

This document describes the firmware architecture, algorithms, and implementation details of the ESP32 DC Optimizer with MPPT.

---

## Firmware Architecture

### Main Loop Flow

```
┌─────────────────────────────────────┐
│      Initialization (setup)         │
│  - Initialize I2C + ADS1015        │
│  - Configure PWM (GPIO14, GPIO27)  │
│  - Set initial duty cycle          │
└──────────────┬──────────────────────┘
               │
               ↓
┌─────────────────────────────────────┐
│     Main Loop (loop)                 │
│  - Check timing conditions          │
│  - Execute sampling or MPPT         │
│  - Non-blocking design              │
└──────────────┬──────────────────────┘
               │
       ┌───────┴────────┐
       ↓                ↓
   ┌────────────┐  ┌──────────────┐
   │  Sampling  │  │  MPPT Update │
   │ (100ms)    │  │  (500ms)     │
   │ Read ADC   │  │ Calculate    │
   │            │  │ Update PWM   │
   └────────────┘  └──────────────┘
```

---

## Core Data Structures

### SystemState_t
```cpp
typedef struct {
  struct {
    float voltage;   // Solar panel voltage (V)
    float current;   // Solar panel current (A)
    float power;     // Solar power (W)
  } solar;
  
  struct {
    float voltage;   // Battery voltage (V)
    float current;   // Battery current (A)
    float power;     // Battery power (W)
  } battery;
  
  uint16_t duty_cycle;  // PWM duty cycle (0-4095)
  float efficiency;     // System efficiency (%)
} SystemState_t;
```

This structure holds all system measurements and is updated every 100ms.

---

## MPPT Algorithm: Perturb & Observe

### Theory

The P&O algorithm works by:
1. Measuring power at current operating point
2. Slightly perturbing the operating voltage
3. Observing if power increased or decreased
4. Moving in the direction that increases power

### Mathematical Formulation

```
dP/dV = 0  (at maximum power point)

P_new - P_old = ΔP
V_new - V_old = ΔV

If ΔP > 0 and ΔV > 0: increase V (increase duty)
If ΔP > 0 and ΔV < 0: decrease V (decrease duty)
If ΔP < 0 and ΔV > 0: decrease V (decrease duty)
If ΔP < 0 and ΔV < 0: increase V (increase duty)
```

### Implementation

```cpp
void mppt_algorithm(void) {
  // Startup phase
  if (current_state.solar.voltage < 5.0 || startup_count < 5) {
    duty_cycle += 50;
    startup_count++;
    return;
  }
  
  // Check power availability
  if (current_state.solar.power < 0.1) {
    return;  // No sun
  }
  
  // Calculate deltas
  float power_delta = current_state.solar.power - previous_state.solar.power;
  float voltage_delta = current_state.solar.voltage - previous_state.solar.voltage;
  
  // Perturbation logic
  if (power_delta > 0.1) {
    // Power increased - continue direction
    if (voltage_delta > 0) {
      duty_cycle += MPPT_STEP;  // Voltage increased, increase more
    } else {
      duty_cycle -= MPPT_STEP;  // Voltage decreased, increase more
    }
  } else if (power_delta < -0.1) {
    // Power decreased - reverse direction
    if (voltage_delta > 0) {
      duty_cycle -= MPPT_STEP;
    } else {
      duty_cycle += MPPT_STEP;
    }
  }
  
  // Safety: constrain duty cycle
  duty_cycle = constrain(duty_cycle, 204, 3891);
}
```

### Parameters

| Parameter | Value | Effect |
|-----------|-------|--------|
| MPPT_STEP | 5 | Step size for duty cycle change |
| MPPT_INTERVAL | 500ms | Update frequency |
| Power threshold | 0.1W | Minimum power detection |
| Voltage threshold | 0.1V | Minimum voltage change |

### Tuning Guide

**Increase MPPT_STEP if:**
- Solar irradiance changes quickly (clouds, sun)
- MPPT oscillates around MPP (undershoot)
- Response time is too slow

**Decrease MPPT_STEP if:**
- Excessive oscillation around MPP
- Noisy power measurements
- Need finer tracking precision

---

## ADC Reading & Conversion

### ADS1015 Interface

```cpp
// Single-ended readings (12-bit)
int16_t adc_raw = ads.readADC_SingleEnded(channel);
// Returns: 0-2047 (12-bit range at ±4.096V)

// Voltage conversion
float voltage = adc_raw * ADC_SCALE_FACTOR;
```

### Calibration Factors

**Calculation Method:**

1. **For Voltage Measurements:**
   ```
   Divider Ratio = (R_high + R_low) / R_low
   Max Input Voltage = 60V (solar) or 25V (battery)
   ADC Input @ Max = Max_Voltage / Divider_Ratio
   ADC Scale = Max_Voltage / 2047 steps
   
   Example (Solar Voltage):
   Divider Ratio = 40.2
   Max ADC Input = 60V / 40.2 = 1.49V
   ADC Scale = 60V / 2047 = 0.0293V/step
   ```

2. **For Current Measurements:**
   ```
   Shunt Voltage @ Max Current = Max_Current × R_shunt
   Amplifier Gain = ADC_Range / Shunt_Range
   ADC Scale = Max_Current / 2047 steps
   
   Example (Solar Current):
   Max Current = 30A
   Shunt Resistor = 10mΩ
   Max Shunt Voltage = 30A × 0.01Ω = 300mV
   ADC Scale = 30A / 2047 = 0.0146A/step (before amplifier)
   ```

### Measurement Sequence

```cpp
void read_adc_values(void) {
  // Read 4 channels in sequence
  int16_t adc_batt_i = ads.readADC_SingleEnded(0);
  int16_t adc_batt_v = ads.readADC_SingleEnded(1);
  int16_t adc_solar_i = ads.readADC_SingleEnded(2);
  int16_t adc_solar_v = ads.readADC_SingleEnded(3);
  
  // Convert to real units
  float v_solar = adc_solar_v * ADC_SOLAR_V_SCALE;
  float i_solar = adc_solar_i * ADC_SOLAR_I_SCALE;
  float p_solar = v_solar * i_solar;
  
  // Repeat for battery
  float v_batt = adc_batt_v * ADC_BATT_V_SCALE;
  float i_batt = adc_batt_i * ADC_BATT_I_SCALE;
  float p_batt = v_batt * i_batt;
  
  // Calculate efficiency
  efficiency = (p_batt / p_solar) * 100;
}
```

### I2C Reading Performance

**ADS1015 Timing:**
- Conversion time: ~1ms (typical)
- I2C transmission: ~100µs
- Total per channel: ~1.1ms
- All 4 channels: ~4.4ms
- Well within 100ms sampling interval

---

## PWM Control & Gate Driving

### PWM Configuration

```cpp
void initialize_pwm(void) {
  // High-side FET (GPIO14)
  ledcSetup(PWM_CHANNEL_HIGH, 20000, 12);    // 20kHz, 12-bit
  ledcAttachPin(14, PWM_CHANNEL_HIGH);
  
  // Low-side FET (GPIO27)
  ledcSetup(PWM_CHANNEL_LOW, 20000, 12);     // 20kHz, 12-bit
  ledcAttachPin(27, PWM_CHANNEL_LOW);
  
  // Set initial 50% duty cycle
  ledcWrite(PWM_CHANNEL_HIGH, 2048);
  ledcWrite(PWM_CHANNEL_LOW, 2048);
}
```

### PWM Resolution & Frequency Trade-off

**12-bit Resolution @ 20kHz:**
- Frequency: 20,000 cycles/second
- Resolution: 4096 steps per cycle
- Period: 50µs
- Minimum step: 12.2ns

**Advantages:**
- Fine control resolution (0.024% per step)
- Quiet operation (20kHz above audible range)
- Good EMI behavior
- Industry standard

### Duty Cycle Calculation

```cpp
void update_pwm_duty_cycle(uint16_t new_duty) {
  // Constrain to safe limits (5%-95%)
  uint16_t safe_duty = constrain(new_duty, 204, 3891);
  
  // High-side PWM
  ledcWrite(PWM_CHANNEL_HIGH, safe_duty);
  
  // Low-side PWM (complementary)
  ledcWrite(PWM_CHANNEL_LOW, 4095 - safe_duty);
  
  // Duty = (PWM_value / 4096) × 100%
  // Example: 2048 → 50% duty
  //          1024 → 25% duty
  //          3072 → 75% duty
}
```

### Dead-Time Consideration

**Why Dead-Time is Needed:**
- Prevents shoot-through (both FETs on simultaneously)
- Protects MOSFETs from catastrophic failure
- Critical for synchronous buck operation

**ESP32 Implementation:**
- Hardware PWM controller has inherent dead-time (~10-20ns)
- Sufficient for 20kHz operation at 5V supply
- For 12V+ supplies, may need external circuit

---

## Timing & Control Flow

### Interrupt-Free Design (Polling-Based)

**Why Polling Instead of Interrupts?**
- Simpler code (no ISR conflicts)
- Predictable timing
- Easier debugging
- Sufficient for 20kHz PWM + 100ms sampling

### Timing Diagram

```
Time (ms):    0      100      200      300      400      500
              │       │        │        │        │        │
Sampling:     │ S ──  S ──     S ──     S ──     S ──     S ──
(100ms)       │ │     │        │        │        │        │
              │ └─────┘        └────────┘        └────────┘
              │
MPPT:         │       M ──     │        │        M ──     │
(500ms)       │       │        │        │        │        │
              │       └────────────────────────────────────┘
              │
Serial:       │       P ──     │        │        P ──     │
(500ms)       │       │        │        │        │        │
              │       └────────────────────────────────────┘
              │
Debug:        │       │        │        │        │        D
(5000ms)      │       │        │        │        │        │

Legend: S=Sampling, M=MPPT, P=Print, D=Debug
```

### Non-Blocking Implementation

```cpp
void loop() {
  uint32_t current_time = millis();
  
  // ADC Sampling (100ms)
  if (current_time - last_sampling_time >= SAMPLING_INTERVAL) {
    read_adc_values();
    last_sampling_time = current_time;
  }
  
  // MPPT Update (500ms)
  if (current_time - last_mppt_time >= MPPT_INTERVAL) {
    mppt_algorithm();
    update_pwm_duty_cycle(duty_cycle);
    print_system_status();
    last_mppt_time = current_time;
  }
  
  delay(10);  // Small delay to prevent watchdog timeout
}
```

**Advantages:**
- No blocking waits
- Responsive to all timing events
- Easy to add new periodic tasks
- CPU can handle other operations

---

## System Status Reporting

### Serial Output Format

```
================================================
Solar Panel: 36.45V | 5.234A | 190.45W
Battery:     12.85V | 14.56A | 187.18W
Duty Cycle:  42.3% | Efficiency: 98.3%
================================================

--- RAW ADC VALUES (Debug) ---
A0 (Batt I): 1456 | A1 (Batt V): 1285 | A2 (Solar I): 523 | A3 (Solar V): 1818
```

### Data Interpretation Guide

**Efficiency Calculation:**
```
Efficiency = (Battery Power / Solar Power) × 100%
Normal range: 92-98%
Low efficiency (<90%): Check connections, calibration
High efficiency (>100%): Error in measurement
```

**Duty Cycle:**
- 0%: Output disconnected
- 25%: Lower voltage output
- 50%: Mid-range operation
- 75%: Higher voltage output
- 100%: Full charging (unregulated)
- Typical MPPT range: 30-70%

**Power Levels:**
- <1W: Low light or no sun
- 10-50W: Partial sun
- 50-200W: Good sunlight
- >200W: Bright sun or multiple panels

---

## Error Handling & Protection

### Software Protection Features

```cpp
// Over-voltage shutdown
if (current_state.solar.voltage > 65.0) {
  duty_cycle = 204;  // Minimum duty
  Serial.println("ERROR: Solar voltage too high!");
}

// Over-current shutdown
if (current_state.solar.current > 35.0) {
  duty_cycle = 204;  // Minimum duty
  Serial.println("ERROR: Solar current too high!");
}

// Reverse polarity detection (voltage = 0)
if (current_state.solar.voltage < 0.5 && warm_start) {
  Serial.println("WARNING: Check solar panel connection");
}
```

### ADS1015 I2C Error Detection

```cpp
if (!ads.begin(ADS1015_ADDR)) {
  Serial.println("ERROR: ADS1015 not found!");
  while(1);  // Halt system
}
```

### Graceful Degradation

If sensors fail:
1. Continue PWM at last known duty
2. Print error to serial
3. Reduce MPPT step size (if available)
4. Attempt recovery on next cycle

---

## Calibration Procedure

### Step-by-Step Calibration

1. **Solar Voltage (A3):**
   ```
   a) Connect known voltage source (e.g., 36V bench supply)
   b) Read serial monitor for raw ADC value
   c) Calculate: ADC_SOLAR_V_SCALE = Actual_V / ADC_Raw
   d) Repeat at 2-3 different voltages
   e) Average the calculated scale factor
   f) Update in code
   ```

2. **Solar Current (A2):**
   ```
   a) Use current dummy load (resistive)
   b) Measure actual current with multimeter
   c) Read ADC value from serial
   d) Calculate: ADC_SOLAR_I_SCALE = Actual_I / ADC_Raw
   e) Verify at multiple current levels
   ```

3. **Battery Voltage & Current:**
   ```
   Same procedure as solar measurements
   Verify against multimeter readings
   ```

### Calibration Verification

After calibration:
- Compare serial readings with multimeter
- Accuracy should be ±2% over full range
- Test at corner cases (minimum and maximum)
- Check linearity across range

---

## Optimization Tips

### Improving Efficiency

1. **Reduce Switching Losses:**
   - Lower frequency if EMI acceptable (10kHz)
   - Optimize gate drive voltage
   - Use better MOSFETs (lower RDS(on))

2. **Minimize Conduction Losses:**
   - Upgrade to IRF4104 or similar (lower RDS(on))
   - Add heatsink for better heat dissipation
   - Reduce trace resistance (thicker PCB traces)

3. **Optimize Inductors:**
   - Select low-loss cores (iron powder)
   - Minimize DCR (copper loss)
   - Match saturation current to application

### Improving MPPT Tracking

1. **Faster Response:**
   - Increase MPPT_STEP (more aggressive)
   - Decrease MPPT_INTERVAL (more frequent)
   - Use faster ADC sampling

2. **Better Stability:**
   - Implement hysteresis (ignore small changes)
   - Use low-pass filtering on ADC readings
   - Adaptive step size based on irradiance

3. **Noise Reduction:**
   - Add input/output filtering capacitors
   - Use twisted pair I2C cables
   - Shield I2C lines from power traces

---

## Future Enhancements

### Software Features to Add

- [ ] WiFi connectivity for remote monitoring
- [ ] Data logging to SD card
- [ ] Temperature compensation
- [ ] Multiple battery chemistry profiles
- [ ] Energy statistics (daily/monthly)
- [ ] Fault diagnosis & logging
- [ ] Over-temperature shutdown
- [ ] Battery fully-charged detection
- [ ] Night-time sleep mode
- [ ] Web dashboard interface

### Hardware Upgrades

- [ ] Temperature sensor (NTC or DS18B20)
- [ ] Real-time clock (RTC) for timestamps
- [ ] EEPROM for configuration storage
- [ ] Relay for load control
- [ ] Additional sensor inputs
- [ ] CAN bus for multi-unit systems

---

## Debugging Guide

### Common Issues & Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| ADS1015 not detected | I2C connection | Check GPIO2/GPIO4 connections |
| Incorrect ADC values | Calibration needed | Recalibrate with known values |
| MPPT not tracking | PWM not working | Verify GPIO14/GPIO27 output |
| Low efficiency | High losses | Check MOSFETs, inductors, wiring |
| Serial output garbage | Baud rate mismatch | Verify 115200 baud |
| Duty cycle stuck | Algorithm error | Check power threshold |

### Serial Monitor Debugging

```bash
# Open serial port at 115200 baud
pio device monitor -b 115200

# Watch for error messages
# Check raw ADC values in debug output
# Verify timing of measurements
```

### Hardware Testing

```
1. Check 3.3V supply: should be stable 3.3V
2. Check I2C clock: verify toggling at 400kHz
3. Measure PWM frequency: should be 20kHz
4. Verify MOSFET switching: use oscilloscope
5. Check inductor current: smooth ramp
```

---

## References

- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [Adafruit ADS1X15 Library](https://github.com/adafruit/Adafruit_ADS1X15)
- [MPPT Algorithms Review](https://ieeexplore.ieee.org/document/5717456)
- [Synchronous Buck Design](https://www.analog.com/en/analog-dialogue/articles/synchronous-buck-converter-design.html)

