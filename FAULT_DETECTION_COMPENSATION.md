# Node Fault Detection & Voltage Compensation System

## Problem Scenario

When one panel fails (broken, hard shading, short circuit):

```
Before Failure:
Node 1: 35V, 8A = 280W ✓
Node 2: 35V, 8A = 280W ✓
Node 3: 35V, 8A = 280W ✓
Node 4: 35V, 8A = 280W ✓
System: 48V, 8A = 384W

After Panel 3 Failure:
Node 1: 35V, 8A = 280W ✓
Node 2: 35V, 8A = 280W ✓
Node 3: 0V,  0A = 0W  ✗ FAILED
Node 4: 35V, 8A = 280W ✓
System: 35V, 8A = 280W ← Can't charge 48V battery!
```

## Solution: Voltage Compensation

When Master detects Node 3 is offline:
1. Identify which node is faulty
2. Command working nodes to increase voltage
3. Compensate: Each working node increases from 12V → 16V
4. New total: 16V × 3 = 48V ✓ Battery charges!

```
After Detection & Compensation:
Node 1: 35V, 6A = 210W (Boosted) ✓
Node 2: 35V, 6A = 210W (Boosted) ✓
Node 3: 0V,  0A = 0W   (BYPASSED) ✗
Node 4: 35V, 6A = 210W (Boosted) ✓
System: 48V, 6A = 288W ← Still works!

Note: Current drops due to increased load on good nodes,
but system continues to charge battery at correct voltage
```

## Implementation Details

### Node-Level Fault Detection

Each node monitors for failure conditions:

```cpp
// In node_controller.cpp - add to read_adc_values()

// Detect node fault conditions
void detect_node_fault(void) {
  // Condition 1: Solar voltage collapsed (panel broken)
  if (previous_state.solar.voltage > 20.0 && 
      current_state.solar.voltage < 5.0) {
    current_state.status = 255;  // Fault code: panel failure
    send_fault_to_master();
    return;
  }
  
  // Condition 2: Current collapsed (open circuit)
  if (previous_state.solar.current > 5.0 && 
      current_state.solar.current < 0.5) {
    current_state.status = 255;  // Fault code: open circuit
    send_fault_to_master();
    return;
  }
  
  // Condition 3: Power suddenly dropped >90% (hard shading)
  if (previous_state.solar.power > 50.0) {
    float power_drop = (previous_state.solar.power - current_state.solar.power) / 
                       previous_state.solar.power;
    if (power_drop > 0.9) {  // >90% power loss
      current_state.status = 254;  // Fault code: severe failure
      send_fault_to_master();
      return;
    }
  }
}
```

### Master-Level Fault Analysis & Compensation

Master detects faulty node and implements compensation:

```cpp
// In master_controller.cpp - new function

void analyze_and_compensate_faults(void) {
  // Find faulty nodes (offline or failed)
  int num_working = 0;
  int faulty_nodes[4] = {0};
  int num_faulty = 0;
  
  for (int i = 1; i <= NUM_NODES; i++) {
    if (node_tracker[i].is_online && node_status[i].input_power > 1.0) {
      num_working++;
    } else {
      faulty_nodes[num_faulty] = i;
      num_faulty++;
    }
  }
  
  // If at least one node is faulty, implement compensation
  if (num_faulty > 0 && num_working > 0) {
    Serial.print("[COMPENSATION] ");
    Serial.print(num_faulty);
    Serial.print(" faulty node(s) detected: ");
    for (int i = 0; i < num_faulty; i++) {
      Serial.print(faulty_nodes[i]);
      Serial.print(" ");
    }
    Serial.println();
    
    // Calculate new voltage to compensate
    // num_working nodes need to provide 48V total
    float compensation_voltage = TARGET_SYSTEM_VOLTAGE / num_working;
    
    Serial.print("[COMPENSATION] Increasing working nodes to ");
    Serial.print(compensation_voltage, 2);
    Serial.println("V each");
    
    // Send compensation command to all nodes
    current_command.target_voltage = compensation_voltage;
    broadcast_command_to_nodes();
  }
}
```

### Status Codes Definition

```cpp
// Node status codes (for clarity)
#define NODE_STATUS_NORMAL        0    // Operating normally
#define NODE_STATUS_SHADING       1    // Partial shading
#define NODE_STATUS_OVERVOLTAGE   2    // Output voltage too high
#define NODE_STATUS_OVERCURRENT   3    // Output current too high
#define NODE_STATUS_SOFT_FAULT    254  // Severe degradation
#define NODE_STATUS_HARD_FAULT    255  // Complete failure
```

## Hardware Consideration: Bypass Diodes

To prevent the faulty node from blocking current flow, add **bypass diodes**:

```
Without Bypass Diode:
│
├─ [Node 1 Output] ─┬─
├─ [Node 2 Output] ─┤
├─ [Node 3 Output] ─┤  ← If Node 3 blocks = breaks entire system
├─ [Node 4 Output] ─┤
│                    └─

With Bypass Diode (Recommended):
│
├─ [Node 1] ──┬───┬─
├─ [Node 2] ──┤ D ├─  ← Diode D3 allows current to bypass Node 3
├─ [Node 3] ──┤   ├─
├─ [Node 4] ──┤ D ├─
│             └───┘

Part: Schottky Diode (low forward voltage drop)
Rating: 30A, 100V minimum
Part Number: STPS3045 or similar
Voltage Drop: ~0.3V at 20A (vs 0.7V silicon diode)

Connection:
Output of each node → [Schottky Diode] → Series chain
Cathode (-) to next node
```

### Bypass Diode Benefits:
✅ Allows current to flow around faulty node
✅ Prevents voltage accumulation
✅ Protects power electronics
✅ System can operate at reduced power
✅ Emergency mode until repairs

## System Operation with Faulty Node

### Timeline:

```
T=0: Node 3 panel breaks (open circuit)
     │
T+100ms: Node 3 detects voltage collapse
         │
T+200ms: Node 3 sends fault status to Master
         │
T+300ms: Master receives fault signal
         │
T+400ms: Master analyzes: 3 working nodes, 1 faulty
         │
T+500ms: Master calculates: 48V ÷ 3 = 16V per node
         │
T+600ms: Master broadcasts new setpoint (16V/node)
         │
T+700ms: Nodes 1,2,4 begin increasing voltage
         │
T+1000ms: System stabilizes at 48V with 3 nodes
          │
          Output: 48V × 6A = 288W (from 280W pre-failure)
          System continues charging battery!
```

### Current vs Voltage Trade-off:

```
Before Failure: 48V × 8A = 384W
                   ↓
After Failure & Compensation: 48V × 6A = 288W
                   ↓
Power Loss: 384W - 288W = 96W (25% power loss)
But: System still functional! Only 1/4 power lost instead of total loss
```

## Master Algorithm: Fault Detection & Compensation

```python
def handle_node_failure():
    # Count working nodes
    working = count_online_nodes_with_power()
    
    if working < NUM_NODES:
        faulty_count = NUM_NODES - working
        print(f"Detected {faulty_count} faulty node(s)")
        
        # Identify which nodes are faulty
        faulty_list = find_faulty_nodes()
        print(f"Faulty nodes: {faulty_list}")
        
        if working >= 2:
            # Enough nodes to compensate
            compensation_voltage = TARGET_VOLTAGE / working
            
            print(f"Compensating: {working} nodes × {compensation_voltage}V = {TARGET_VOLTAGE}V")
            broadcast_new_setpoint(compensation_voltage)
            
        else:
            # Only 1 or 0 nodes working
            print("CRITICAL: Cannot maintain target voltage!")
            emergency_shutdown()
```

## Configuration: Fault Thresholds

In `master_controller.cpp`:

```cpp
// Add these defines for fault detection

// Power loss threshold for detection
#define FAULT_POWER_LOSS_THRESHOLD   0.90   // 90% loss = fault
#define FAULT_MIN_POWER_DETECTION    1.0    // Minimum 1W to be "working"

// Voltage thresholds
#define FAULT_VOLTAGE_COLLAPSE       5.0    // Less than 5V = likely broken
#define FAULT_VOLTAGE_RECOVERY       15.0   // Need >15V to be working

// Current thresholds
#define FAULT_CURRENT_COLLAPSE       0.5    // Less than 0.5A = open circuit
#define FAULT_CURRENT_MINIMUM        1.0    // Need >1A to be "working"

// Compensation parameters
#define MIN_NODES_FOR_COMPENSATION   2      // Need at least 2 nodes
#define MAX_COMPENSATION_VOLTAGE     20.0   // Safety limit per node
#define COMPENSATION_HYSTERESIS      0.5    // Debounce time (seconds)
```

## Serial Output Example

### Normal Operation:
```
═══════════════════════════════════════════════════════════
              MASTER CONTROLLER - SYSTEM STATUS
═══════════════════════════════════════════════════════════
Nodes Online: 4/4
System Voltage: 48.23V (Target: 48.0V)
System Current: 20.15A (Max: 35.0A)
Status: NORMAL
═══════════════════════════════════════════════════════════
```

### When Node 3 Fails:
```
!!! ALERT: Node 3 power loss detected !!!
[FAULT ANALYSIS] Node Status:
  Node 1: 280W ✓ Working
  Node 2: 280W ✓ Working
  Node 3: 0W   ✗ FAULTY (Voltage collapsed)
  Node 4: 280W ✓ Working

[COMPENSATION] 1 faulty node(s) detected: 3

[COMPENSATION] Recalculating:
  - Faulty nodes: 1
  - Working nodes: 3
  - Required system voltage: 48V
  - New per-node voltage: 16.0V

[COMPENSATION] Increasing working nodes to 16.00V each

System Recovery:
  Node 1: 16V × 6A = 96W (boosted from 8A)
  Node 2: 16V × 6A = 96W (boosted from 8A)
  Node 3: Bypassed via diode
  Node 4: 16V × 6A = 96W (boosted from 8A)

New System: 48V × 6A = 288W (was 384W)
Status: OPERATING IN DEGRADED MODE
```

## Implementation Steps

### Step 1: Update Node Controller
- Add fault detection in `read_adc_values()`
- Send fault status in `send_status_to_master()`
- Include node ID in every status message

### Step 2: Update Master Controller
- Add `analyze_and_compensate_faults()` function
- Detect which node(s) are faulty
- Calculate compensation voltage
- Broadcast new setpoint to working nodes

### Step 3: Add Hardware (Bypass Diodes)
- Install Schottky diodes on each node output
- Rating: 30A, 100V
- Cathode to next node, anode to previous

### Step 4: Testing
1. Simulate failure by disconnecting one PV panel
2. Observe Master detecting fault
3. Verify compensation voltage increase
4. Check that system voltage stays at 48V
5. Monitor power output (should be reduced but stable)

## Advantages of This Approach

✅ **Redundancy**: System survives single node failure
✅ **Automatic**: No manual intervention needed
✅ **Safe**: Voltage stays regulated at 48V
✅ **Efficient**: Continues charging battery (albeit at reduced power)
✅ **Diagnostic**: Master reports exactly which node failed
✅ **Simple**: No complex load sharing needed

## Safety Considerations

⚠️ **Important**:
1. **Bypass Diodes Required**: Without them, faulty node acts as open circuit
2. **Heat Management**: Remaining nodes work harder - monitor temperature
3. **Voltage Stress**: Increased voltage on good nodes - ensure rated properly
4. **Current Limit**: May exceed node limits - watch for over-current

## Example: 4 Nodes, 2 Fail

```
Original: 4 × 12V, 8A = 48V, 8A, 384W

Scenario 1: Node 1 fails
  3 × 16V, 6A = 48V, 6A, 288W ✓ Works

Scenario 2: Nodes 1 & 2 fail
  2 × 24V, 4A = 48V, 4A, 192W ✓ Works (at reduced power)

Scenario 3: Nodes 1, 2, 3 fail
  1 × 48V, 2A = 48V, 2A, 96W ✓ Barely works
  (Not practical - better to shut down)

Beyond 3 failures: System cannot maintain 48V
  Decision: Emergency shutdown and repair
```

