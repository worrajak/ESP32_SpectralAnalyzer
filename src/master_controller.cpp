#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// ===== MASTER CONTROLLER FOR MULTI-NODE MPPT SYSTEM =====
// Controls 4 DC converter nodes via ESP-NOW
// Broadcasts voltage setpoint and receives status from all nodes

// ===== GPIO PIN DEFINITIONS =====
#define LED_STATUS_PIN       25   // LED for system status
#define BUTTON_PIN           32   // Optional: button for manual control
#define SERIAL_BAUD          115200

// ===== SYSTEM CONFIGURATION =====
#define NUM_NODES            4
#define TARGET_SYSTEM_VOLTAGE 48.0   // Total system voltage (V)
#define TARGET_NODE_VOLTAGE   (TARGET_SYSTEM_VOLTAGE / NUM_NODES)  // 12V per node

// ===== VOLTAGE CONTROL PARAMETERS =====
#define VOLTAGE_RAMP_STEP    0.1    // Voltage change per update (0.1V per step)
#define VOLTAGE_RAMP_INTERVAL 2000  // ms - Interval between voltage adjustments
#define MIN_SYSTEM_VOLTAGE   36.0   // Minimum safe voltage (30V + margin)
#define MAX_SYSTEM_VOLTAGE   60.0   // Maximum safe voltage (15V × 4 nodes)
#define VOLTAGE_BALANCE_TOLERANCE 1.0  // Tolerance for voltage balancing (V)

// ===== FAULT DETECTION PARAMETERS =====
#define NODE_TIMEOUT         5000   // ms - Time to consider node offline
#define OVERVOLTAGE_THRESHOLD 14.0  // Per node voltage limit
#define OVERCURRENT_THRESHOLD 35.0  // System current limit
#define EFFICIENCY_WARNING   80.0   // Minimum efficiency warning
#define SHADING_CURRENT_DROP 0.3    // Allowable current drop (0-1 scale)

// ===== COMMUNICATION PARAMETERS =====
#define STATUS_REPORT_INTERVAL 1000 // ms - Report master status
#define COMMAND_BROADCAST_INTERVAL 2000 // ms - Send commands to nodes

// ===== DATA STRUCTURES =====
typedef struct {
  uint8_t node_id;           // Node identifier
  float input_voltage;       // Solar panel voltage
  float input_current;       // Solar panel current
  float input_power;         // Solar panel power
  float output_voltage;      // Output voltage
  float output_current;      // Output current
  float output_power;        // Output power
  float duty_cycle_percent;  // PWM duty cycle %
  float efficiency;          // Conversion efficiency %
  uint8_t status;            // Node status
  uint32_t timestamp;        // Timestamp
} NodeStatus_t;

typedef struct {
  uint8_t node_id;           // 0xFF for broadcast, 1-4 for specific node
  float target_voltage;      // Desired output voltage per node
  float max_current;         // Maximum allowed current
  uint8_t command;           // 0=normal, 1=shutdown, 2=reset
} MasterCommand_t;

typedef struct {
  uint32_t last_update;
  bool is_online;
  float voltage_setpoint;
  uint16_t consecutive_errors;
  uint8_t last_status;
} NodeTracker_t;

typedef struct {
  float total_input_power;
  float total_output_power;
  float total_output_current;
  float system_voltage;
  float system_efficiency;
  uint8_t num_nodes_online;
  uint8_t num_shaded_nodes;
  bool has_fault;
  uint8_t fault_code;
} SystemState_t;

// ===== FAULT CODES =====
#define FAULT_NONE                   0x00
#define FAULT_NODE_OFFLINE           0x01
#define FAULT_OVERVOLTAGE_NODE       0x02
#define FAULT_OVERCURRENT_SYSTEM     0x04
#define FAULT_LOW_EFFICIENCY         0x08
#define FAULT_VOLTAGE_IMBALANCE      0x10
#define FAULT_SHADING_DETECTED       0x20

// ===== GLOBAL VARIABLES =====
NodeStatus_t node_status[NUM_NODES + 1];     // Index 1-4 for nodes
NodeTracker_t node_tracker[NUM_NODES + 1];   // Index 1-4
SystemState_t system_state = {0};
MasterCommand_t current_command = {0xFF, TARGET_NODE_VOLTAGE, OVERCURRENT_THRESHOLD, 0};

uint32_t last_broadcast_time = 0;
uint32_t last_report_time = 0;
uint32_t last_voltage_adjustment_time = 0;
uint32_t system_startup_time = 0;

bool has_received_any_status = false;
uint8_t global_fault_state = FAULT_NONE;
float voltage_setpoint = TARGET_NODE_VOLTAGE;
bool emergency_shutdown = false;

// ===== FUNCTION DECLARATIONS =====
void initialize_esp_now(void);
void broadcast_command_to_nodes(void);
void calculate_system_state(void);
void optimize_voltage_setpoint(void);
void handle_faults(void);
void print_system_status(void);
void print_node_details(void);
void led_status_indication(void);
void on_data_receive(const uint8_t *mac_addr, const uint8_t *data, int len);
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status);
void perform_voltage_balance(void);
void detect_faults(void);
void emergency_stop(void);

// ===== SETUP =====
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(2000);
  
  Serial.println("\n\n==========================================");
  Serial.println("=== MASTER CONTROLLER - MULTI-NODE MPPT ===");
  Serial.println("=== System: 4 × 35V PV Panel (250Wp) ===");
  Serial.println("=== Series Output: 48V ===");
  Serial.println("==========================================\n");
  
  // Initialize GPIO
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_STATUS_PIN, LOW);
  
  // Initialize WiFi (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  Serial.print("Master MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Update this address in node_config.h on each node!\n");
  
  // Initialize ESP-NOW
  initialize_esp_now();
  
  // Initialize node tracker
  for (int i = 1; i <= NUM_NODES; i++) {
    node_tracker[i].last_update = 0;
    node_tracker[i].is_online = false;
    node_tracker[i].voltage_setpoint = TARGET_NODE_VOLTAGE;
    node_tracker[i].consecutive_errors = 0;
    node_tracker[i].last_status = 0;
  }
  
  system_startup_time = millis();
  Serial.println("Master initialization complete!");
  Serial.println("Waiting for nodes to connect...\n");
}

// ===== MAIN LOOP =====
void loop() {
  uint32_t current_time = millis();
  
  // Broadcast commands to nodes (2000ms interval)
  if (current_time - last_broadcast_time >= COMMAND_BROADCAST_INTERVAL) {
    broadcast_command_to_nodes();
    last_broadcast_time = current_time;
  }
  
  // Calculate system state and optimize voltage (2000ms interval)
  if (current_time - last_voltage_adjustment_time >= VOLTAGE_RAMP_INTERVAL) {
    calculate_system_state();
    optimize_voltage_setpoint();
    detect_faults();
    handle_faults();
    last_voltage_adjustment_time = current_time;
  }
  
  // Print status report (1000ms interval)
  if (current_time - last_report_time >= STATUS_REPORT_INTERVAL) {
    print_system_status();
    print_node_details();
    last_report_time = current_time;
  }
  
  // LED status indication
  led_status_indication();
  
  // Check emergency button
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("\n!!! EMERGENCY BUTTON PRESSED !!!");
      emergency_shutdown = true;
      emergency_stop();
    }
  }
  
  delay(10);
}

// ===== INITIALIZE ESP-NOW =====
void initialize_esp_now(void) {
  Serial.println("Initializing ESP-NOW...");
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW initialization failed!");
    return;
  }
  
  esp_now_register_recv_cb(on_data_receive);
  esp_now_register_send_cb(on_data_sent);
  
  Serial.println("ESP-NOW initialized successfully!");
  Serial.println("Waiting for node status messages...\n");
}

// ===== BROADCAST COMMAND TO ALL NODES =====
void broadcast_command_to_nodes(void) {
  if (emergency_shutdown) {
    current_command.command = 1;  // Shutdown command
  }
  
  // Broadcast to all nodes (0xFF)
  current_command.node_id = 0xFF;
  current_command.target_voltage = voltage_setpoint;
  current_command.max_current = OVERCURRENT_THRESHOLD;
  
  uint8_t broadcast_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(broadcast_addr, (uint8_t *)&current_command, sizeof(MasterCommand_t));
}

// ===== CALCULATE SYSTEM STATE =====
void calculate_system_state(void) {
  system_state.total_input_power = 0.0;
  system_state.total_output_power = 0.0;
  system_state.total_output_current = 0.0;
  system_state.system_voltage = 0.0;
  system_state.num_nodes_online = 0;
  system_state.num_shaded_nodes = 0;
  system_state.has_fault = false;
  
  uint32_t current_time = millis();
  
  // Aggregate status from all nodes
  for (int i = 1; i <= NUM_NODES; i++) {
    if (current_time - node_tracker[i].last_update < NODE_TIMEOUT) {
      node_tracker[i].is_online = true;
      system_state.num_nodes_online++;
      
      system_state.total_input_power += node_status[i].input_power;
      system_state.total_output_power += node_status[i].output_power;
      system_state.total_output_current = node_status[i].output_current;  // Series = same current
      system_state.system_voltage += node_status[i].output_voltage;
      
      if (node_status[i].status == 1) {
        system_state.num_shaded_nodes++;
      }
    } else {
      node_tracker[i].is_online = false;
    }
  }
  
  // Calculate overall efficiency
  if (system_state.total_input_power > 0.1) {
    system_state.system_efficiency = (system_state.total_output_power / system_state.total_input_power) * 100.0;
  } else {
    system_state.system_efficiency = 0.0;
  }
}

// ===== OPTIMIZE VOLTAGE SETPOINT =====
void optimize_voltage_setpoint(void) {
  if (system_state.num_nodes_online < 2) {
    // Not enough nodes online, hold current voltage
    return;
  }
  
  // Strategy: Gradually increase voltage until efficiency drops
  // or until we hit a limit
  
  // Check for overvoltage condition
  float max_node_voltage = 0.0;
  float min_node_voltage = 100.0;
  
  for (int i = 1; i <= NUM_NODES; i++) {
    if (node_tracker[i].is_online) {
      max_node_voltage = max(max_node_voltage, node_status[i].output_voltage);
      min_node_voltage = min(min_node_voltage, node_status[i].output_voltage);
    }
  }
  
  // Voltage balancing: if imbalance > threshold, don't increase voltage
  if ((max_node_voltage - min_node_voltage) > VOLTAGE_BALANCE_TOLERANCE) {
    // Voltage imbalance detected, reduce voltage slightly
    if (voltage_setpoint > MIN_SYSTEM_VOLTAGE / NUM_NODES) {
      voltage_setpoint -= VOLTAGE_RAMP_STEP * 0.5;
    }
    return;
  }
  
  // Check efficiency: if dropping, reduce voltage
  if (system_state.system_efficiency < EFFICIENCY_WARNING && voltage_setpoint > MIN_SYSTEM_VOLTAGE / NUM_NODES) {
    voltage_setpoint -= VOLTAGE_RAMP_STEP;
    return;
  }
  
  // Normal operation: try to increase voltage gradually to find MPP
  float system_voltage_target = voltage_setpoint * NUM_NODES;
  
  if (system_voltage_target < MAX_SYSTEM_VOLTAGE - 2.0) {
    // Can increase voltage
    voltage_setpoint += VOLTAGE_RAMP_STEP;
  } else if (system_voltage_target > MAX_SYSTEM_VOLTAGE) {
    // Too high, decrease
    voltage_setpoint -= VOLTAGE_RAMP_STEP;
  }
  
  // Constrain voltage
  voltage_setpoint = constrain(voltage_setpoint, 
                               MIN_SYSTEM_VOLTAGE / NUM_NODES,
                               MAX_SYSTEM_VOLTAGE / NUM_NODES);
}

// ===== PERFORM VOLTAGE BALANCING =====
void perform_voltage_balance(void) {
  // Find max and min voltages across nodes
  float max_v = 0.0;
  float min_v = 100.0;
  uint8_t max_node = 0, min_node = 0;
  
  for (int i = 1; i <= NUM_NODES; i++) {
    if (node_tracker[i].is_online) {
      if (node_status[i].output_voltage > max_v) {
        max_v = node_status[i].output_voltage;
        max_node = i;
      }
      if (node_status[i].output_voltage < min_v) {
        min_v = node_status[i].output_voltage;
        min_node = i;
      }
    }
  }
  
  // If imbalance detected
  if ((max_v - min_v) > VOLTAGE_BALANCE_TOLERANCE) {
    Serial.print("[BALANCING] Voltage imbalance detected: ");
    Serial.print(max_v, 2);
    Serial.print("V (Node ");
    Serial.print(max_node);
    Serial.print(") vs ");
    Serial.print(min_v, 2);
    Serial.print("V (Node ");
    Serial.println(min_node);
    Serial.println(");
    
    // Reduce node with higher voltage to balance
    // This is handled by reducing overall setpoint
    // Each node's MPPT will then adjust individually
  }
}

// ===== DETECT FAULTS =====
void detect_faults(void) {
  global_fault_state = FAULT_NONE;
  
  // Check for offline nodes
  for (int i = 1; i <= NUM_NODES; i++) {
    if (!node_tracker[i].is_online) {
      global_fault_state |= FAULT_NODE_OFFLINE;
      break;
    }
  }
  
  // Check for over-voltage on any node
  for (int i = 1; i <= NUM_NODES; i++) {
    if (node_tracker[i].is_online && node_status[i].output_voltage > OVERVOLTAGE_THRESHOLD) {
      global_fault_state |= FAULT_OVERVOLTAGE_NODE;
      break;
    }
  }
  
  // Check for over-current on system
  if (system_state.total_output_current > OVERCURRENT_THRESHOLD) {
    global_fault_state |= FAULT_OVERCURRENT_SYSTEM;
  }
  
  // Check for low efficiency
  if (system_state.system_efficiency < EFFICIENCY_WARNING && system_state.total_input_power > 10.0) {
    global_fault_state |= FAULT_LOW_EFFICIENCY;
  }
  
  // Check for voltage imbalance
  float max_v = 0.0, min_v = 100.0;
  for (int i = 1; i <= NUM_NODES; i++) {
    if (node_tracker[i].is_online) {
      max_v = max(max_v, node_status[i].output_voltage);
      min_v = min(min_v, node_status[i].output_voltage);
    }
  }
  if ((max_v - min_v) > VOLTAGE_BALANCE_TOLERANCE) {
    global_fault_state |= FAULT_VOLTAGE_IMBALANCE;
  }
  
  // Check for shading
  if (system_state.num_shaded_nodes > 0) {
    global_fault_state |= FAULT_SHADING_DETECTED;
  }
}

// ===== HANDLE FAULTS =====
void handle_faults(void) {
  if (global_fault_state == FAULT_NONE) {
    return;  // No faults
  }
  
  if (global_fault_state & FAULT_NODE_OFFLINE) {
    Serial.println("[FAULT] One or more nodes offline!");
    if (system_state.num_nodes_online == 0) {
      // All nodes offline, emergency stop
      emergency_stop();
    }
  }
  
  if (global_fault_state & FAULT_OVERVOLTAGE_NODE) {
    Serial.println("[FAULT] Over-voltage detected on node!");
    // Reduce system voltage
    voltage_setpoint -= VOLTAGE_RAMP_STEP * 2;
  }
  
  if (global_fault_state & FAULT_OVERCURRENT_SYSTEM) {
    Serial.println("[FAULT] System over-current!");
    voltage_setpoint -= VOLTAGE_RAMP_STEP;
  }
  
  if (global_fault_state & FAULT_VOLTAGE_IMBALANCE) {
    Serial.println("[FAULT] Voltage imbalance detected!");
    perform_voltage_balance();
  }
  
  if (global_fault_state & FAULT_SHADING_DETECTED) {
    // This is not critical, just informational
    if (millis() % 4000 < 2000) {  // Print every 4 seconds
      Serial.println("[INFO] Shading detected on one or more nodes");
    }
  }
}

// ===== EMERGENCY STOP =====
void emergency_stop(void) {
  emergency_shutdown = true;
  voltage_setpoint = 0.0;
  current_command.command = 1;  // Shutdown
  broadcast_command_to_nodes();
  
  digitalWrite(LED_STATUS_PIN, HIGH);  // LED on = emergency
  
  Serial.println("\n!!! EMERGENCY SHUTDOWN ACTIVATED !!!");
  Serial.println("All nodes commanded to shutdown");
  
  while (1) {
    digitalWrite(LED_STATUS_PIN, HIGH);
    delay(100);
    digitalWrite(LED_STATUS_PIN, LOW);
    delay(100);
  }
}

// ===== LED STATUS INDICATION =====
void led_status_indication(void) {
  static uint32_t last_blink = 0;
  uint32_t current_time = millis();
  
  if (emergency_shutdown) {
    // Fast blink = emergency
    if (current_time - last_blink > 200) {
      digitalWrite(LED_STATUS_PIN, !digitalRead(LED_STATUS_PIN));
      last_blink = current_time;
    }
  } else if (global_fault_state != FAULT_NONE) {
    // Slow blink = fault
    if (current_time - last_blink > 500) {
      digitalWrite(LED_STATUS_PIN, !digitalRead(LED_STATUS_PIN));
      last_blink = current_time;
    }
  } else if (system_state.num_nodes_online == NUM_NODES && system_state.total_input_power > 10.0) {
    // Steady on = normal operation
    digitalWrite(LED_STATUS_PIN, HIGH);
  } else if (system_state.num_nodes_online > 0) {
    // Slow blink = waiting for full operation
    if (current_time - last_blink > 1000) {
      digitalWrite(LED_STATUS_PIN, !digitalRead(LED_STATUS_PIN));
      last_blink = current_time;
    }
  } else {
    // Off = no nodes online
    digitalWrite(LED_STATUS_PIN, LOW);
  }
}

// ===== PRINT SYSTEM STATUS =====
void print_system_status(void) {
  if (!has_received_any_status && (millis() - system_startup_time) > 5000) {
    Serial.println("Waiting for node status... (check node MAC addresses)");
    return;
  }
  
  Serial.println("\n═══════════════════════════════════════════════════════════");
  Serial.println("              MASTER CONTROLLER - SYSTEM STATUS");
  Serial.println("═══════════════════════════════════════════════════════════");
  
  // System overview
  Serial.print("Nodes Online: ");
  Serial.print(system_state.num_nodes_online);
  Serial.print("/");
  Serial.print(NUM_NODES);
  Serial.println();
  
  Serial.print("System Voltage: ");
  Serial.print(system_state.system_voltage, 2);
  Serial.print("V (Target: ");
  Serial.print(TARGET_SYSTEM_VOLTAGE, 1);
  Serial.println("V)");
  
  Serial.print("System Current: ");
  Serial.print(system_state.total_output_current, 2);
  Serial.print("A (Max: ");
  Serial.print(OVERCURRENT_THRESHOLD, 1);
  Serial.println("A)");
  
  Serial.print("Input Power: ");
  Serial.print(system_state.total_input_power, 1);
  Serial.print("W | Output Power: ");
  Serial.print(system_state.total_output_power, 1);
  Serial.print("W | Efficiency: ");
  Serial.print(system_state.system_efficiency, 1);
  Serial.println("%");
  
  Serial.print("Voltage Setpoint: ");
  Serial.print(voltage_setpoint, 2);
  Serial.print("V/node | Shaded: ");
  Serial.print(system_state.num_shaded_nodes);
  Serial.println(" node(s)");
  
  // Fault status
  Serial.print("Status: ");
  if (emergency_shutdown) {
    Serial.println("EMERGENCY SHUTDOWN");
  } else if (global_fault_state == FAULT_NONE) {
    Serial.println("NORMAL");
  } else {
    Serial.print("FAULT [");
    if (global_fault_state & FAULT_NODE_OFFLINE) Serial.print("OFFLINE ");
    if (global_fault_state & FAULT_OVERVOLTAGE_NODE) Serial.print("OV ");
    if (global_fault_state & FAULT_OVERCURRENT_SYSTEM) Serial.print("OC ");
    if (global_fault_state & FAULT_LOW_EFFICIENCY) Serial.print("LOW_EFF ");
    if (global_fault_state & FAULT_VOLTAGE_IMBALANCE) Serial.print("IMBALANCE ");
    Serial.println("]");
  }
  
  Serial.println("═══════════════════════════════════════════════════════════");
}

// ===== PRINT NODE DETAILS =====
void print_node_details(void) {
  Serial.println("Node Details:");
  Serial.println("┌─────┬────────┬────────┬────────┬────────┬─────────┬────────┐");
  Serial.println("│Node │  Input │ Output │ Output │ Duty%  │  Eff%  │ Status │");
  Serial.println("│ ID  │  V/C   │   V    │   P    │        │        │        │");
  Serial.println("├─────┼────────┼────────┼────────┼────────┼─────────┼────────┤");
  
  for (int i = 1; i <= NUM_NODES; i++) {
    Serial.print("│ ");
    Serial.print(i);
    
    if (!node_tracker[i].is_online) {
      Serial.println(" │ OFFLINE│        │        │        │        │ OFFLINE│");
      continue;
    }
    
    Serial.print(" │ ");
    Serial.print(node_status[i].input_voltage, 1);
    Serial.print("/");
    Serial.print(node_status[i].input_current, 1);
    
    Serial.print(" │ ");
    Serial.print(node_status[i].output_voltage, 2);
    Serial.print(" │ ");
    Serial.print(node_status[i].output_power, 1);
    Serial.print("W");
    
    Serial.print(" │ ");
    Serial.print(node_status[i].duty_cycle_percent, 1);
    Serial.print("%");
    
    Serial.print(" │ ");
    Serial.print(node_status[i].efficiency, 1);
    Serial.print("%");
    
    Serial.print(" │ ");
    switch (node_status[i].status) {
      case 0: Serial.print("NORMAL"); break;
      case 1: Serial.print("SHADE "); break;
      case 2: Serial.print("OVERVOLT"); break;
      case 3: Serial.print("OVERCUR"); break;
      default: Serial.print("UNKNOWN");
    }
    Serial.println("│");
  }
  
  Serial.println("└─────┴────────┴────────┴────────┴────────┴─────────┴────────┘");
}

// ===== ESP-NOW CALLBACK: DATA RECEIVED =====
void on_data_receive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  if (len == sizeof(NodeStatus_t)) {
    NodeStatus_t status;
    memcpy(&status, data, len);
    
    if (status.node_id >= 1 && status.node_id <= NUM_NODES) {
      // Store node status
      memcpy(&node_status[status.node_id], &status, sizeof(NodeStatus_t));
      node_tracker[status.node_id].last_update = millis();
      node_tracker[status.node_id].is_online = true;
      node_tracker[status.node_id].consecutive_errors = 0;
      has_received_any_status = true;
    }
  }
}

// ===== ESP-NOW CALLBACK: DATA SENT =====
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Command sent to node
  // Typically no action needed here
}
