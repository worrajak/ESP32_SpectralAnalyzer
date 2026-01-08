# Fault Detection & Automatic Voltage Compensation Integration Guide

## Overview

This guide explains how to integrate the enhanced fault detection and automatic voltage compensation into your existing master and node controller firmware.

## Part 1: Master Controller Enhancements

### Step 1: Add Fault Detection Constants

Add to `src/master_controller.cpp` at the top, after other `#define` statements:

```cpp
// Fault detection thresholds
#define FAULT_MIN_POWER_DETECTION     1.0    // Nodes with <1W are considered offline
#define FAULT_VOLTAGE_COLLAPSE        5.0    // Voltage <5V indicates hardware failure
#define FAULT_CURRENT_COLLAPSE        0.5    // Current <0.5A indicates open circuit
#define MIN_NODES_FOR_COMPENSATION    2      // Need at least 2 nodes to compensate
#define MAX_COMPENSATION_VOLTAGE      16.0   // Don't exceed 16V per node (4 nodes = 64V)
#define TARGET_SYSTEM_VOLTAGE         48.0   // Target battery voltage
#define COMPENSATION_CHECK_INTERVAL   2000   // Check every 2 seconds
```

### Step 2: Add Global Variables

Add to the global variables section in `src/master_controller.cpp`:

```cpp
unsigned long last_fault_check_time = 0;
unsigned long last_compensation_time = 0;
bool compensation_active = false;
int previous_num_faulty = 0;
```

### Step 3: Add Enhanced Functions

Copy the three main functions from `src/master_enhancements.cpp` into `src/master_controller.cpp`:
- `detect_node_failures()`
- `analyze_and_compensate_faults()`
- `print_system_status_enhanced()`

### Step 4: Update Main Loop

In the main control loop (approximately after `detect_faults()`), add:

```cpp
// Detect node failures and apply voltage compensation
if (current_time - last_fault_check_time >= COMPENSATION_CHECK_INTERVAL) {
  detect_node_failures();
  analyze_and_compensate_faults();
  last_fault_check_time = current_time;
}
```

### Step 5: Update Status Reporting

Replace the fault detection section of `print_system_status()` with:

```cpp
// Print faulty nodes summary
if (num_faults > 0) {
  print_system_status_enhanced();
}
```

---

## Part 2: Node Controller Enhancements

### Step 1: Add Fault Detection Constants

Add to `src/node_controller.cpp` at the top, after other `#define` statements:

```cpp
#define FAULT_MIN_VOLTAGE       2.0     // Node voltage < 2V = open circuit
#define FAULT_MIN_CURRENT       0.1     // Node current < 0.1A = no power draw
#define FAULT_MIN_POWER         0.5     // Node power < 0.5W = offline
#define FAULT_POWER_DROP_PCT    90.0    // 90% power drop = hard failure

#define NODE_STATUS_NORMAL        0
#define NODE_STATUS_SHADING       1
#define NODE_STATUS_OVERVOLTAGE   2
#define NODE_STATUS_OVERCURRENT   3
#define NODE_STATUS_SOFT_FAULT    254
#define NODE_STATUS_HARD_FAULT    255
```

### Step 2: Add Global Variables

Add to the global variables section in `src/node_controller.cpp`:

```cpp
float prev_input_power = 0.0;
uint8_t node_fault_status = NODE_STATUS_NORMAL;
```

### Step 3: Add Fault Detection Functions

Copy from `src/node_enhancements.cpp`:
- `detect_node_fault()`
- `report_fault_if_detected()`
- `print_fault_diagnostics()` (optional, for debugging)

### Step 4: Update ADC Reading and Power Calculation

After calculating power in the main loop, add:

```cpp
// Calculate power
float power = voltage * current;

// NEW: Detect and report faults
report_fault_if_detected(voltage, current, power);

// Update status structure
node_status.status = node_fault_status;
```

### Step 5: Update NodeStatus_t Structure

Ensure your `NodeStatus_t` structure includes the status field:

```cpp
typedef struct {
  uint8_t node_id;
  float input_voltage;
  float input_current;
  float input_power;
  uint8_t duty_cycle;
  float efficiency;
  uint8_t status;              // Node status code (0-3, 254, 255)
  unsigned long timestamp;
} NodeStatus_t;
```

---

## Part 3: Hardware Integration

### Bypass Diode Configuration

To enable system operation when one node fails, add bypass diodes:

#### Schottky Diodes Specifications:
- **Part Number**: STPS3045TV (or equivalent)
- **Rating**: 30A, 100V
- **Type**: Schottky (low forward voltage drop)
- **Mounting**: One diode per node output

#### Wiring Diagram:

```
Panel 1 Output ──[MOSFET 1]──[DIODE 1]── ┐
                                          ├─ Series Chain to Battery
Panel 2 Output ──[MOSFET 2]──[DIODE 2]── ┤
                                          │
Panel 3 Output ──[MOSFET 3]──[DIODE 3]── ┤
                                          │
Panel 4 Output ──[MOSFET 4]──[DIODE 4]── ┘
```

**Diode Connection:**
- **Cathode** (black stripe): Connected to next node's output
- **Anode**: Connected to current node's output

**Benefits:**
- If Node 3 fails (high impedance), current can bypass through DIODE 3
- Other nodes continue charging battery through remaining paths
- Master detects Node 3 offline and increases voltage on Nodes 1, 2, 4

---

## Part 4: Testing Fault Scenarios

### Test 1: Simulated Panel Failure

1. Apply full sun to Nodes 1, 2, 4
2. Disconnect Node 3's panel input
3. Observe:
   - Node 3 reports HARD_FAULT
   - Master detects 3 working nodes
   - Master calculates: 48V ÷ 3 = 16V per node
   - Voltage setpoint changes from 12V to 16V
   - Other nodes increase output voltage
   - System current may slightly increase

### Test 2: Shading Scenario

1. Apply full sun to Nodes 1, 2, 4
2. Partially shade Node 3 (50% power loss)
3. Observe:
   - Node 3 reports SOFT_FAULT status
   - Master detects but doesn't trigger full compensation
   - System continues with 4-node control
   - Efficiency may decrease slightly

### Test 3: Recovery Test

1. Start with Node 3 shaded/failed
2. Restore Node 3 (remove shade, reconnect)
3. Observe:
   - Node 3 reports NORMAL status
   - Master detects recovery
   - Voltage setpoint returns to 12V
   - System stabilizes

---

## Part 5: Serial Output Examples

### Normal 4-Node Operation:

```
[SYSTEM] State: OPTIMIZING | V_target: 12.00V | Eff: 92.3%
[NODE 1] 1.2A @ 12.1V = 14.5W [Duty: 48%] - Normal
[NODE 2] 1.2A @ 12.0V = 14.4W [Duty: 49%] - Normal
[NODE 3] 1.2A @ 12.1V = 14.5W [Duty: 48%] - Normal
[NODE 4] 1.2A @ 12.0V = 14.4W [Duty: 49%] - Normal
[SYSTEM] Total: 4.8A @ 48.2V = 231.4W | Efficiency: 92.3%
```

### Node 3 Failure Detected:

```
!!! ───────────────────────────────────────────── !!!
!!! ALERT: Node Failure Detected - 1 node(s) offline !!!
!!! ───────────────────────────────────────────── !!!
[FAULT ANALYSIS] Node Status:
  Node 1: 14.5W ✓ Working
  Node 2: 14.4W ✓ Working
  Node 3: FAULTY (No Power)
  Node 4: 14.4W ✓ Working

[COMPENSATION] Activating Voltage Compensation
[COMPENSATION] Recalculating: Faulty=1 Working=3
[COMPENSATION] Target System Voltage: 48.0V ÷ 3 nodes = 16.00V per node
[COMPENSATION] Broadcasting new voltage setpoint...

[SYSTEM] State: COMPENSATING | V_target: 16.00V | Eff: 91.1%
[NODE 1] 1.5A @ 15.9V = 23.9W [Duty: 35%] - Normal
[NODE 2] 1.5A @ 16.0V = 24.0W [Duty: 35%] - Normal
[NODE 3] 0.0A @ 0.1V = 0.0W [Duty: 0%] - HARD_FAULT
[NODE 4] 1.5A @ 16.0V = 24.0W [Duty: 35%] - Normal
[SYSTEM] Total: 4.5A @ 47.9V = 215.5W | Efficiency: 91.1%
```

---

## Part 6: Configuration Checklist

- [ ] Added FAULT_MIN_POWER_DETECTION, FAULT_VOLTAGE_COLLAPSE constants to master
- [ ] Added FAULT_MIN_VOLTAGE, FAULT_MIN_CURRENT to node
- [ ] Added detect_node_failures() to master_controller.cpp
- [ ] Added analyze_and_compensate_faults() to master_controller.cpp
- [ ] Added detect_node_fault() to node_controller.cpp
- [ ] Added report_fault_if_detected() to node_controller.cpp
- [ ] Updated main loop with fault checking (2-second interval)
- [ ] Updated NodeStatus_t structure to include status field
- [ ] Installed bypass diodes on all 4 nodes
- [ ] Tested with simulated failures

---

## Part 7: Troubleshooting

### Issue: Master not detecting node failures

**Solution:**
1. Verify node is sending status updates via ESP-NOW
2. Check FAULT_MIN_POWER_DETECTION threshold (default 1.0W)
3. Ensure node's input_power value is correctly calculated
4. Check serial output: `[FAULT ANALYSIS]` should print node status

### Issue: Voltage compensation not activating

**Solution:**
1. Verify num_faulty > 0 and num_working >= MIN_NODES_FOR_COMPENSATION (2)
2. Check that voltage_setpoint is being updated
3. Confirm broadcast_command_to_nodes() is called
4. Monitor serial: `[COMPENSATION] Broadcasting...` should appear

### Issue: System still can't maintain 48V with 3 nodes

**Solution:**
1. Verify compensation voltage calculated correctly: 48V ÷ 3 = 16V per node
2. Check if nodes can actually reach 16V output (may be limited by buck converter design)
3. Reduce TARGET_SYSTEM_VOLTAGE if necessary (e.g., 45V instead of 48V)
4. Verify battery can accept up to 16V per node configuration

### Issue: False fault detection (nodes incorrectly marked faulty)

**Solution:**
1. Increase FAULT_MIN_POWER_DETECTION threshold (e.g., 2.0W instead of 1.0W)
2. Increase FAULT_VOLTAGE_COLLAPSE threshold if panels sag in low light (e.g., 7.0V)
3. Add hysteresis: don't trigger fault until condition persists for 5+ seconds
4. Check ADC accuracy - verify voltage/current readings with multimeter

---

## Part 8: Future Enhancements

1. **Node Identification in Master**: Add more detailed node status reporting
2. **Graceful Degradation**: Allow system to operate with 2 nodes (requires 24V battery or converter)
3. **Hot-swapping**: Support adding a node back without full system reset
4. **Predictive Fault Detection**: Monitor power trends to predict failures
5. **MPPT Optimization**: Adjust per-node voltage for 3-node operation differently than 4-node
6. **Logging**: Store fault events with timestamps for analysis

---

**Last Updated**: January 2026  
**Version**: 2.0 - Fault Compensation Implementation  
**Status**: Ready for integration into main firmware
