# Multi-Node MPPT System Documentation

## System Overview

Multi-node DC converter system where 4 nodes are connected in **series** with centralized master control via **ESP-NOW**.

### Architecture

```
Node 1 (35V PV)  │  Node 2 (35V PV)  │  Node 3 (35V PV)  │  Node 4 (35V PV)
      12V out    │       12V out     │       12V out     │       12V out
        │        │         │         │         │         │         │
        └────────┴─────────┴─────────┴─────────┴─────────┘
                         Series = 48V
                           │
                         Battery
```

## Node Controller Features

### ESP-NOW Communication
- Receives voltage setpoint commands from master
- Sends status updates every 2 seconds
- Command structure: target voltage, max current, control command

### Control Algorithm (3 Layers)

1. **Voltage Control (1000ms)** - Track master setpoint
2. **MPPT Control (500ms)** - Fine-tune within voltage constraint
3. **Safety Limits** - Current, duty cycle, voltage protection

### Shading Detection & Response

When one panel is shaded:
- Its current drops (detected in series)
- MPPT adjusts to maintain output voltage
- Series current becomes bottleneck
- Other nodes adapt to lower current

### Message Structures

**Master → Node:**
```cpp
struct MasterCommand {
  uint8_t node_id;        // 0xFF = broadcast
  float target_voltage;   // 12.0V per node
  float max_current;      // 30.0A limit
  uint8_t command;        // 0=run, 1=shutdown
};
```

**Node → Master:**
```cpp
struct NodeStatus {
  uint8_t node_id;        // 1-4
  float input_voltage;    // Solar voltage
  float input_current;    // Solar current
  float output_voltage;   // Output voltage
  float output_current;   // Output current
  float efficiency;       // Conversion %
  uint8_t status;         // 0=normal, 1=shading, 2=overvoltage
};
```

## Configuration

1. Get Master MAC: `Serial.println(WiFi.macAddress());`
2. Update `include/node_config.h`:
   - Set `NODE_ID` (1, 2, 3, or 4)
   - Set `MASTER_MAC_ADDR` with master's MAC

3. Calibrate ADC for each node using serial output

## Expected Performance

**Single Node (Sunny):**
- Solar: 35V × 8A = 280W
- Output: 12V × 20A = 240W
- Efficiency: ~86%

**Series System (All Sunny):**
- Total Solar: 1120W
- Series Output: 48V × 20A = 960W
- System Efficiency: ~86%

**With One Shaded Node (20% power):**
- System current drops due to series bottleneck
- MPPT on shaded node increases internal duty cycle
- Total system power reduces but efficiency maintained

## Safety

⚠️ Series 48V system - Dangerous!
- Use proper disconnects and PPE
- Prevent reverse current with bypass diodes
- Monitor for over-voltage per node
- Current sharing via series connection

## Next: Master Controller

Master ESP32 will:
- Broadcast voltage setpoint to all nodes
- Receive status from all 4 nodes
- Optimize overall system voltage
- Handle fault detection

