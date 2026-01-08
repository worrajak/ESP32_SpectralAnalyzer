// ===== ENHANCED NODE CONTROLLER FUNCTIONS =====
// Add these functions to node_controller.cpp

#include <Wire.h>
#include "Adafruit_ADS1015.h"

// ===== FAULT DETECTION CONFIGURATION =====
#define FAULT_MIN_VOLTAGE       2.0     // Node voltage < 2V = open circuit
#define FAULT_MIN_CURRENT       0.1     // Node current < 0.1A = no power draw
#define FAULT_MIN_POWER         0.5     // Node power < 0.5W = offline
#define FAULT_POWER_DROP_PCT    90.0    // 90% power drop = hard failure

#define SOFT_FAULT_THRESHOLD    10.0    // Power loss > 10W = soft fault
#define HARD_FAULT_THRESHOLD    5.0     // Power loss > 90% = hard fault

// Status codes to report to master
#define NODE_STATUS_NORMAL        0
#define NODE_STATUS_SHADING       1
#define NODE_STATUS_OVERVOLTAGE   2
#define NODE_STATUS_OVERCURRENT   3
#define NODE_STATUS_SOFT_FAULT    254
#define NODE_STATUS_HARD_FAULT    255

// ===== GLOBAL VARIABLES FOR FAULT TRACKING =====
float prev_input_power = 0.0;           // Previous power reading
float current_input_power = 0.0;        // Current power reading
uint8_t node_fault_status = NODE_STATUS_NORMAL;
unsigned long last_power_calc_time = 0;

// ===== DETECT LOCAL NODE FAULT =====
uint8_t detect_node_fault(float voltage, float current, float power) {
  // Returns current fault status for this node
  
  uint8_t status = NODE_STATUS_NORMAL;
  
  // Check 1: Complete power loss (open circuit or panel broken)
  if (voltage < FAULT_MIN_VOLTAGE && current < FAULT_MIN_CURRENT) {
    // Panel is disconnected or completely broken
    return NODE_STATUS_HARD_FAULT;
  }
  
  // Check 2: Voltage collapse (short circuit or internal failure)
  if (voltage < FAULT_MIN_VOLTAGE && current > 1.0) {
    // High current but low voltage = short circuit
    return NODE_STATUS_HARD_FAULT;
  }
  
  // Check 3: Power too low to charge battery
  if (power < FAULT_MIN_POWER && voltage > FAULT_MIN_VOLTAGE) {
    // Voltage is OK but power is zero = dead panel
    return NODE_STATUS_HARD_FAULT;
  }
  
  // Check 4: Severe power loss (comparing to previous reading)
  if (prev_input_power > SOFT_FAULT_THRESHOLD) {
    float power_drop_pct = 100.0 * (prev_input_power - power) / prev_input_power;
    
    if (power_drop_pct > FAULT_POWER_DROP_PCT) {
      // More than 90% power drop = likely hard failure
      if (power < SOFT_FAULT_THRESHOLD) {
        return NODE_STATUS_HARD_FAULT;
      } else {
        return NODE_STATUS_SOFT_FAULT;
      }
    } else if (power_drop_pct > 50.0) {
      // Significant power loss but not complete = soft fault (shading?)
      return NODE_STATUS_SHADING;
    }
  }
  
  // Check 5: Voltage regulation check
  if (voltage > TARGET_VOLTAGE + 2.0) {
    return NODE_STATUS_OVERVOLTAGE;
  }
  
  // Check 6: Current check
  if (current > 35.0) {  // Max 35A for safety
    return NODE_STATUS_OVERCURRENT;
  }
  
  return NODE_STATUS_NORMAL;
}

// ===== REPORT FAULT STATUS TO MASTER =====
void report_fault_if_detected(float voltage, float current, float power) {
  // Detect fault condition
  uint8_t current_status = detect_node_fault(voltage, current, power);
  
  // Only report status changes or persist fault
  if (current_status != NODE_STATUS_NORMAL) {
    if (node_fault_status != current_status) {
      // Status changed - immediately notify master
      node_fault_status = current_status;
      
      Serial.print("[FAULT] Node ");
      Serial.print(NODE_ID);
      Serial.print(" Status: ");
      
      switch(current_status) {
        case NODE_STATUS_SHADING:
          Serial.println("SHADING (Soft Fault)");
          break;
        case NODE_STATUS_SOFT_FAULT:
          Serial.println("SOFT FAULT (Severe Degradation)");
          break;
        case NODE_STATUS_HARD_FAULT:
          Serial.print("HARD FAULT - V:");
          Serial.print(voltage, 1);
          Serial.print("V I:");
          Serial.print(current, 1);
          Serial.print("A P:");
          Serial.print(power, 1);
          Serial.println("W");
          break;
      }
    }
  } else if (node_fault_status != NODE_STATUS_NORMAL) {
    // Status recovered
    node_fault_status = NODE_STATUS_NORMAL;
    Serial.print("[FAULT] Node ");
    Serial.print(NODE_ID);
    Serial.println(" recovered to NORMAL");
  }
  
  // Update power history
  prev_input_power = power;
}

// ===== MODIFIED BROADCAST STRUCTURE =====
/*
Modify the NodeStatus_t structure to include fault reporting:

typedef struct {
  uint8_t node_id;
  float input_voltage;
  float input_current;
  float input_power;
  uint8_t duty_cycle;
  float efficiency;
  uint8_t status;              // NEW: Node status code (0-3, 254, 255)
  unsigned long timestamp;
} NodeStatus_t;
*/

// ===== ENHANCED MAIN LOOP ADDITION =====
/*
In the main loop(), after reading ADC and calculating power:

    // Calculate power
    float power = voltage * current;
    
    // NEW: Detect and report faults
    report_fault_if_detected(voltage, current, power);
    
    // Update status structure with fault status
    node_status.status = node_fault_status;  // Include in broadcast
    
    // Continue with normal MPPT control...
*/

// ===== SERIAL DIAGNOSTIC OUTPUT =====
void print_fault_diagnostics(float voltage, float current, float power) {
  // Print detailed fault diagnostics for debugging
  
  Serial.println();
  Serial.println("[DIAGNOSTIC] Node Fault Analysis:");
  Serial.print("  Input Voltage: ");
  Serial.print(voltage, 2);
  Serial.print("V (min=");
  Serial.print(FAULT_MIN_VOLTAGE, 1);
  Serial.println("V)");
  
  Serial.print("  Input Current: ");
  Serial.print(current, 2);
  Serial.print("A (min=");
  Serial.print(FAULT_MIN_CURRENT, 1);
  Serial.println("A)");
  
  Serial.print("  Input Power: ");
  Serial.print(power, 1);
  Serial.print("W (min=");
  Serial.print(FAULT_MIN_POWER, 1);
  Serial.println("W)");
  
  if (prev_input_power > 0.1) {
    float power_drop_pct = 100.0 * (prev_input_power - power) / prev_input_power;
    Serial.print("  Power Drop: ");
    Serial.print(power_drop_pct, 1);
    Serial.print("% (threshold=");
    Serial.print(FAULT_POWER_DROP_PCT, 1);
    Serial.println("%)");
  }
  
  Serial.print("  Fault Status: ");
  switch(node_fault_status) {
    case NODE_STATUS_NORMAL:
      Serial.println("NORMAL");
      break;
    case NODE_STATUS_SHADING:
      Serial.println("SHADING");
      break;
    case NODE_STATUS_SOFT_FAULT:
      Serial.println("SOFT FAULT");
      break;
    case NODE_STATUS_HARD_FAULT:
      Serial.println("HARD FAULT ⚠️");
      break;
    default:
      Serial.print("UNKNOWN (");
      Serial.print(node_fault_status);
      Serial.println(")");
  }
  
  Serial.println();
}

// ===== CONFIGURATION FOR EACH NODE =====
/*
Update include/node_config.h with:

#define NODE_ID                 1         // Set 1-4 for each node
#define TARGET_VOLTAGE          12.0      // 12V per node in 48V system
#define MAX_CURRENT             35.0      // Max 35A per node
#define TARGET_SYSTEM_VOLTAGE   48.0      // 4 nodes × 12V = 48V

#define FAULT_MIN_VOLTAGE       2.0       // < 2V = panel disconnected
#define FAULT_MIN_CURRENT       0.1       // < 0.1A = no power draw
#define FAULT_MIN_POWER         0.5       // < 0.5W = offline
#define FAULT_POWER_DROP_PCT    90.0      // > 90% drop = hard failure
*/
