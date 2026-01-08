# Master Controller Documentation

## Overview

The Master Controller is the central intelligence for the multi-node MPPT system. It:
- Broadcasts voltage setpoints to all 4 nodes
- Receives real-time status from all nodes
- Optimizes system voltage for maximum power
- Detects and handles system faults
- Provides comprehensive monitoring and diagnostics

---

## Control Architecture

### Three-Level Optimization Strategy

```
┌──────────────────────────────────────────────────────┐
│ LEVEL 1: System Voltage Optimization                 │
│ ────────────────────────────────────────────────────  │
│ Goal: Find optimal total voltage for maximum power   │
│ Method: Gradually ramp voltage up until efficiency   │
│         drops, then maintain that level              │
│ Interval: 2000ms                                     │
│ Range: 36V - 60V (9V - 15V per node)                │
└────────────┬─────────────────────────────────────────┘
             │
         ┌───┴───────────────┐
         ▼                   ▼
    Voltage OK         Efficiency
    Continue          Dropping
    ┌────────┐        ┌────────┐
    │Increase│        │Decrease│
    │+0.1V   │        │-0.1V   │
    └────────┘        └────────┘
         │                   │
         └───────┬───────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────┐
│ LEVEL 2: Voltage Balancing Across Nodes              │
│ ────────────────────────────────────────────────────  │
│ Goal: Keep all nodes at same voltage (±1V)          │
│ Method: Monitor voltage differences, adjust setpoint│
│ Action: If imbalance > threshold, reduce setpoint   │
│ Benefit: Prevents overloading individual nodes      │
└────────────┬─────────────────────────────────────────┘
             │
    ┌────────┴─────────────┐
    ▼                      ▼
 Balanced             Imbalanced
 Continue             │V_max - V_min│ > 1V
 ┌──────┐            ┌──────────┐
 │MPPT  │            │Reduce    │
 │Ramp  │            │Setpoint  │
 └──────┘            └──────────┘
         │                   │
         └───────┬───────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────┐
│ LEVEL 3: Individual Node MPPT                        │
│ ────────────────────────────────────────────────────  │
│ Goal: Each node tracks its individual MPP           │
│ Method: P&O algorithm within master's voltage limit │
│ Benefit: Adapts to shading on individual nodes      │
│ Execution: On each node (500ms interval)            │
└──────────────────────────────────────────────────────┘
```

### Power Flow Optimization

```
Master sets voltage → All nodes adjust to voltage
    ↓
Each node's MPPT tracks MPP within voltage constraint
    ↓
Status feedback to master every 2 seconds
    ↓
Master calculates efficiency
    ↓
If efficiency good: try increase voltage (±0.1V)
If efficiency dropping: reduce voltage
    ↓
Repeat cycle
```

---

## Communication Protocol

### Broadcast Command (Master → All Nodes)

**Interval**: 2000ms (every 2 seconds)
**Message Structure**:

```cpp
struct MasterCommand {
  uint8_t node_id;        // 0xFF = broadcast to all
  float target_voltage;   // 9.0-15.0V per node
  float max_current;      // 30.0A maximum
  uint8_t command;        // 0=normal, 1=shutdown, 2=reset
};
// Size: ~13 bytes
```

**Example Commands**:
```
Normal: {0xFF, 12.0, 30.0, 0}     → All nodes: 12V per node
Reduce: {0xFF, 11.5, 30.0, 0}     → All nodes: 11.5V per node
Shutdown: {0xFF, 0.0, 0.0, 1}     → Emergency stop
```

### Status Report (Node → Master)

**Interval**: 2000ms (every 2 seconds)
**Message Structure**:

```cpp
struct NodeStatus {
  uint8_t node_id;        // 1-4
  float input_voltage;    // Solar panel voltage (V)
  float input_current;    // Solar panel current (A)
  float input_power;      // Solar input power (W)
  float output_voltage;   // Converter output voltage (V)
  float output_current;   // Converter output current (A)
  float output_power;     // Converter output power (W)
  float duty_cycle_percent;  // PWM duty cycle (0-100%)
  float efficiency;       // Node efficiency (0-100%)
  uint8_t status;         // 0=normal, 1=shading, 2=overvoltage, 3=overcurrent
  uint32_t timestamp;     // Milliseconds since node startup
};
// Size: ~44 bytes
```

---

## System State Calculation

Every 2 seconds, master calculates aggregate system state:

```cpp
System Voltage = Sum of all node output voltages
System Current = Output current (same for all in series)
Total Input Power = Sum of all node solar input power
Total Output Power = Sum of all node output power
System Efficiency = (Total Output Power / Total Input Power) × 100%
Nodes Online = Count of nodes responding within 5 seconds
```

**Performance Metrics**:
- Input Power: Solar array total generation
- Output Power: Actual power delivered to battery
- System Efficiency: Conversion efficiency (target >85%)
- Current Bottleneck: Lowest-power node determines system current

---

## Fault Detection & Handling

### Fault Codes

| Code | Name | Threshold | Action |
|------|------|-----------|--------|
| 0x01 | Node Offline | No response >5s | Reduce voltage, alert |
| 0x02 | Over-voltage | >14V per node | Emergency reduce |
| 0x04 | Over-current | >35A system | Reduce voltage |
| 0x08 | Low Efficiency | <80% | Reduce voltage |
| 0x10 | Voltage Imbalance | ΔV > 1V | Reduce voltage |
| 0x20 | Shading Detected | Status=1 from node | Informational |

### Fault Response Flowchart

```
Fault Detected?
    ↓ YES
    ├─→ Node Offline
    │   └─→ If all offline: Emergency Stop
    │   └─→ If some offline: Reduce voltage
    │
    ├─→ Over-voltage (>14V)
    │   └─→ Voltage_setpoint -= 0.2V (fast reduction)
    │
    ├─→ Over-current (>35A)
    │   └─→ Voltage_setpoint -= 0.1V
    │
    ├─→ Low Efficiency (<80%)
    │   └─→ Voltage_setpoint -= 0.1V
    │
    ├─→ Voltage Imbalance (>1V)
    │   └─→ Voltage_setpoint -= 0.05V
    │
    └─→ Shading (Info only)
        └─→ Log but continue operating
        └─→ System current naturally drops
```

### Recovery Strategy

Once fault clears:
1. Fault monitoring continues
2. If no fault for 10 cycles: begin voltage recovery
3. Slowly increase voltage (0.1V steps)
4. Monitor for fault recurrence
5. Return to normal MPPT optimization

---

## Voltage Optimization Algorithm

### Algorithm Logic

```python
def optimize_voltage():
    # Check voltage balance first
    if voltage_imbalance > TOLERANCE:
        voltage -= RAMP_STEP × 0.5  # Go slower if imbalanced
        return
    
    # Check efficiency
    if efficiency < EFFICIENCY_WARNING:
        voltage -= RAMP_STEP
        return
    
    # Normal MPP tracking
    system_voltage = voltage × 4
    
    if system_voltage < MAX:
        voltage += RAMP_STEP  # Try increasing for more power
    elif system_voltage > MAX:
        voltage -= RAMP_STEP  # Reduce if exceeding limit
    
    # Constrain within safe limits
    voltage = constrain(voltage, MIN/4, MAX/4)
```

### Parameters

| Parameter | Value | Purpose |
|-----------|-------|---------|
| VOLTAGE_RAMP_STEP | 0.1V | Voltage change per update |
| VOLTAGE_RAMP_INTERVAL | 2000ms | Update frequency |
| MIN_SYSTEM_VOLTAGE | 36.0V | Minimum safe voltage |
| MAX_SYSTEM_VOLTAGE | 60.0V | Maximum safe voltage |
| VOLTAGE_BALANCE_TOLERANCE | 1.0V | Max imbalance before action |

### Example Optimization Timeline

```
Time: 0-5 sec: Startup, waiting for nodes
      ↓ Receives first status from all nodes
Time: 5-10 sec: Nodes online, system at 12V/node = 48V total
      ↓ Input power = 800W, Output = 688W, Eff = 86%
Time: 10-15 sec: Efficiency good, try increase
      ↓ Voltage → 12.1V/node = 48.4V
      ↓ Input power = 850W, Output = 714W, Eff = 84%
Time: 15-20 sec: Efficiency dropping, hold
      ↓ Voltage stays at 12.0V/node
Time: 20 sec onward: Continuous optimization based on conditions
```

---

## System Monitoring & Display

### Console Output Format

**System Status (every 1 second)**:
```
═══════════════════════════════════════════════════════════
              MASTER CONTROLLER - SYSTEM STATUS
═══════════════════════════════════════════════════════════
Nodes Online: 4/4
System Voltage: 48.23V (Target: 48.0V)
System Current: 20.15A (Max: 35.0A)
Input Power: 850.2W | Output Power: 731.7W | Efficiency: 86.1%
Voltage Setpoint: 12.06V/node | Shaded: 0 node(s)
Status: NORMAL
═══════════════════════════════════════════════════════════
```

**Node Details Table**:
```
Node Details:
┌─────┬────────┬────────┬────────┬────────┬─────────┬────────┐
│Node │  Input │ Output │ Output │ Duty%  │  Eff%   │ Status │
│ ID  │  V/C   │   V    │   P    │        │        │        │
├─────┼────────┼────────┼────────┼────────┼─────────┼────────┤
│ 1   │ 35.2/8 │ 12.04  │ 96.3W  │ 42.1%  │ 85.2%  │ NORMAL │
│ 2   │ 35.1/8 │ 12.05  │ 96.4W  │ 42.2%  │ 85.3%  │ NORMAL │
│ 3   │ 35.0/8 │ 12.06  │ 96.5W  │ 42.3%  │ 85.4%  │ NORMAL │
│ 4   │ 35.3/8 │ 12.04  │ 96.3W  │ 42.1%  │ 85.2%  │ NORMAL │
└─────┴────────┴────────┴────────┴────────┴─────────┴────────┘
```

---

## LED Status Indication

| LED State | Meaning | Action |
|-----------|---------|--------|
| **Steady On** | Normal operation | All nodes online, power flowing |
| **Slow Blink** (1Hz) | Waiting/Partial | Some nodes offline or low power |
| **Fast Blink** (5Hz) | Fault condition | Check console for fault code |
| **Very Fast** (10Hz) | Emergency shutdown | All systems halted |
| **Off** | No nodes online | No communication or startup |

---

## Startup Sequence

1. **0-2 sec**: ESP32 boots, initializes WiFi & ESP-NOW
2. **2-5 sec**: Master waits for nodes to connect
3. **5 sec+**: Receives first status from nodes
4. **10 sec+**: All 4 nodes should be online
5. **20 sec+**: Normal operation, optimization begins

**Indicators**:
- Node offline for >5 seconds: Marked as offline
- System waits for responses every 2 seconds
- Voltage optimization starts once 2+ nodes online

---

## Recovery from Fault

### Scenario: One Node Loses Connection

```
Timeline:
T+0: Node 3 loses connection (WiFi interrupted)
T+2: Master doesn't receive Node 3 status
T+4: Status timeout > 2 seconds, mark Node 3 offline
T+4: Detect only 3/4 nodes online
T+4: Trigger FAULT_NODE_OFFLINE
T+4: Reduce voltage setpoint by 0.2V as precaution
T+6: Broadcast reduced voltage to remaining 3 nodes

Nodes 1,2,4 continue operating at lower voltage
System current = limited by Nodes 1,2,4
When Node 3 reconnects:
T+10: Receive status from Node 3
T+10: Mark online, clear offline flag
T+12: Resume normal voltage optimization
T+20: Return to previous optimal voltage if stable
```

---

## Shading Response

### Single Node Shading

```
Solar Panel 3 receives 30% sun (70% shade):

Master receives status:
- Node 1: 35.2V, 8.0A, 282W → Normal
- Node 2: 35.1V, 8.0A, 281W → Normal
- Node 3: 35.0V, 2.4A (←SHADED), 84W → Shading
- Node 4: 35.3V, 8.0A, 282W → Normal

System behavior:
- All nodes must operate at 2.4A (series bottleneck)
- Node 3's MPPT increases duty to maintain 12V
- Nodes 1,2,4 adapt to lower current
- System power drops from ~1100W to ~680W
- Master detects efficiency still good
- May reduce voltage slightly to optimize

Recovery:
- When shade clears, Node 3 current returns to 8A
- System current increases back to 8A
- Power returns to ~1100W
- Master can again increase voltage
```

---

## Performance Characteristics

### Normal Operation (All Sunny)

```
Each Node:
- Solar: 35V × 8A = 280W
- Output: 12V × 20A = 240W
- Efficiency: 85.7%

System (4 nodes series):
- Total Solar: 1120W
- Series Output: 48V × 20A = 960W
- System Efficiency: 85.7%
```

### With Shading (Node 3 at 30%)

```
Node 1,2,4: 280W each (normal)
Node 3: 84W (shaded)

Bottleneck: Node 3 at ~2.4A

System:
- Each node × 2.4A
- System power: (282 + 281 + 84 + 282) × 2.4 ÷ 280 ≈ 688W
- Efficiency maintained ~85%
- Current limitation prevents damage
```

---

## Configuration & Customization

### Key Parameters in `master_controller.cpp`

```cpp
// System target
#define TARGET_SYSTEM_VOLTAGE 48.0      // Total series voltage
#define NUM_NODES 4                     // Number of nodes

// Voltage control
#define VOLTAGE_RAMP_STEP 0.1           // Adjust for faster/slower response
#define VOLTAGE_RAMP_INTERVAL 2000      // Adjust update frequency

// Fault thresholds
#define OVERVOLTAGE_THRESHOLD 14.0      // Per-node maximum
#define OVERCURRENT_THRESHOLD 35.0      // System maximum
#define EFFICIENCY_WARNING 80.0         // Minimum acceptable

// Communication
#define NODE_TIMEOUT 5000               // Consider offline after 5s
```

### Tuning Recommendations

**For Faster Tracking** (Variable clouds):
- Decrease VOLTAGE_RAMP_INTERVAL to 1000ms
- Increase VOLTAGE_RAMP_STEP to 0.2V
- Decrease NODE_TIMEOUT to 3000ms

**For More Stable** (Steady light):
- Increase VOLTAGE_RAMP_INTERVAL to 3000ms
- Decrease VOLTAGE_RAMP_STEP to 0.05V
- Increase NODE_TIMEOUT to 10000ms

---

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| No nodes appear | Wrong MAC addresses | Check master MAC on console |
| Nodes offline intermittently | WiFi interference | Move away from sources |
| Voltage unstable | RAMP_STEP too large | Reduce to 0.05V |
| Low efficiency | Temperature increase | Check heatsinks, add cooling |
| System oscillating | MPPT step size too large | Reduce on nodes |
| One node lagging | Node timeout | Check Node's I2C/ADS1015 |

---

## Future Enhancements

- [ ] SD card logging of performance data
- [ ] Web dashboard for remote monitoring
- [ ] Battery chemistry auto-detection
- [ ] Temperature-based derating
- [ ] Predictive voltage optimization (weather API)
- [ ] Load management integration
- [ ] CAN bus support for larger systems
- [ ] Redundant master for high-reliability systems

