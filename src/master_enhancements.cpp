// ===== ENHANCED MASTER CONTROLLER FUNCTIONS =====
// Add these functions to master_controller.cpp

// ===== DETECT NODE FAILURES =====
void detect_node_failures(void) {
  // Identify faulty nodes by analyzing power and voltage
  // A node is considered faulty if:
  // 1. Power < 1W (completely offline)
  // 2. Voltage < 5V (panel broken/short)
  // 3. Current < 0.5A (open circuit)
  // 4. Power drop > 90% from previous reading (hard failure)
  
  for (int i = 1; i <= NUM_NODES; i++) {
    if (!node_tracker[i].is_online) {
      continue;  // Already marked offline
    }
    
    // Check for complete power loss
    if (node_status[i].input_power < FAULT_MIN_POWER_DETECTION) {
      node_tracker[i].last_status = 255;  // Mark as hard fault
      continue;
    }
    
    // Check for voltage collapse
    if (node_status[i].input_voltage < FAULT_VOLTAGE_COLLAPSE) {
      node_tracker[i].last_status = 255;  // Mark as hard fault
      continue;
    }
    
    // Check for current collapse
    if (node_status[i].input_current < FAULT_CURRENT_COLLAPSE) {
      node_tracker[i].last_status = 255;  // Mark as hard fault
      continue;
    }
    
    // Check for power loss >90%
    if (node_status[i].input_power > 10.0) {  // Only check if significant power
      float prev_power = node_status[i].input_power;  // Would need to track previous
      // This would require adding power history tracking
    }
    
    node_tracker[i].last_status = node_status[i].status;
  }
}

// ===== ANALYZE FAULTS AND APPLY VOLTAGE COMPENSATION =====
void analyze_and_compensate_faults(void) {
  // Count working vs faulty nodes
  int num_working = 0;
  int num_faulty = 0;
  int faulty_nodes[NUM_NODES];
  int working_nodes[NUM_NODES];
  
  memset(faulty_nodes, 0, sizeof(faulty_nodes));
  memset(working_nodes, 0, sizeof(working_nodes));
  
  // Categorize nodes
  for (int i = 1; i <= NUM_NODES; i++) {
    // Node is working if:
    // - Online
    // - Power > minimum threshold
    // - Voltage > minimum threshold
    // - Status != fault
    
    bool is_working = (node_tracker[i].is_online && 
                       node_status[i].input_power > FAULT_MIN_POWER_DETECTION &&
                       node_status[i].input_voltage > FAULT_VOLTAGE_COLLAPSE &&
                       node_status[i].input_current > FAULT_CURRENT_COLLAPSE);
    
    if (is_working) {
      working_nodes[num_working] = i;
      num_working++;
    } else {
      faulty_nodes[num_faulty] = i;
      num_faulty++;
    }
  }
  
  // If faulty nodes detected, apply compensation
  if (num_faulty > 0) {
    // Print fault analysis
    Serial.println();
    Serial.println("!!! ─────────────────────────────────────────── !!!");
    Serial.print("!!! ALERT: Node Failure Detected - ");
    Serial.print(num_faulty);
    Serial.println(" node(s) offline !!!");
    Serial.println("!!! ─────────────────────────────────────────── !!!");
    
    Serial.println("[FAULT ANALYSIS] Node Status:");
    for (int i = 1; i <= NUM_NODES; i++) {
      Serial.print("  Node ");
      Serial.print(i);
      Serial.print(": ");
      
      if (node_status[i].input_power < FAULT_MIN_POWER_DETECTION) {
        Serial.print("FAULTY (No Power)");
      } else if (node_status[i].input_voltage < FAULT_VOLTAGE_COLLAPSE) {
        Serial.print("FAULTY (Voltage Collapsed)");
      } else if (node_status[i].input_current < FAULT_CURRENT_COLLAPSE) {
        Serial.print("FAULTY (Open Circuit)");
      } else {
        Serial.print(node_status[i].input_power, 0);
        Serial.print("W ✓ Working");
      }
      Serial.println();
    }
    
    // Check if we can compensate
    if (num_working >= MIN_NODES_FOR_COMPENSATION) {
      // Calculate compensation voltage
      float compensation_voltage = TARGET_SYSTEM_VOLTAGE / (float)num_working;
      
      // Safety check: don't exceed maximum voltage per node
      if (compensation_voltage > MAX_COMPENSATION_VOLTAGE) {
        compensation_voltage = MAX_COMPENSATION_VOLTAGE;
      }
      
      Serial.println();
      Serial.println("[COMPENSATION] Activating Voltage Compensation");
      Serial.print("[COMPENSATION] Recalculating:");
      Serial.print(" Faulty=");
      Serial.print(num_faulty);
      Serial.print(" Working=");
      Serial.println(num_working);
      
      Serial.print("[COMPENSATION] Target System Voltage: ");
      Serial.print(TARGET_SYSTEM_VOLTAGE, 1);
      Serial.print("V ÷ ");
      Serial.print(num_working);
      Serial.print(" nodes = ");
      Serial.print(compensation_voltage, 2);
      Serial.println("V per node");
      
      // Set new voltage setpoint
      voltage_setpoint = compensation_voltage;
      
      // Immediately broadcast new command
      Serial.println("[COMPENSATION] Broadcasting new voltage setpoint...");
      broadcast_command_to_nodes();
      
    } else if (num_working == 1) {
      // Only 1 node working - very risky, show warning
      Serial.println();
      Serial.println("!!! CRITICAL: Only 1 node working !!!");
      Serial.println("!!! System cannot maintain 48V target !!!");
      Serial.println("!!! Maximum achievable: 35V × 1 = 35V !!!");
      Serial.println("!!! Recommend immediate repair or shutdown !!!");
      
      // Still try to operate in degraded mode
      voltage_setpoint = TARGET_SYSTEM_VOLTAGE;  // Try to get 48V (will fail)
      
    } else {
      // No working nodes
      Serial.println();
      Serial.println("!!! CRITICAL: All nodes offline !!!");
      emergency_stop();
    }
    
    Serial.println();
  }
}

// ===== ENHANCED SYSTEM STATUS REPORTING =====
void print_system_status_enhanced(void) {
  // Count faulty nodes
  int num_faulty = 0;
  for (int i = 1; i <= NUM_NODES; i++) {
    if (!node_tracker[i].is_online || 
        node_status[i].input_power < FAULT_MIN_POWER_DETECTION) {
      num_faulty++;
    }
  }
  
  if (num_faulty > 0) {
    Serial.print("⚠️  FAULTY NODES: ");
    Serial.print(num_faulty);
    Serial.print("/");
    Serial.println(NUM_NODES);
    
    for (int i = 1; i <= NUM_NODES; i++) {
      if (!node_tracker[i].is_online || 
          node_status[i].input_power < FAULT_MIN_POWER_DETECTION) {
        Serial.print("   └─ Node ");
        Serial.print(i);
        Serial.print(": ");
        
        if (!node_tracker[i].is_online) {
          Serial.println("OFFLINE (No response)");
        } else if (node_status[i].input_power < FAULT_MIN_POWER_DETECTION) {
          Serial.print("NO POWER (");
          Serial.print(node_status[i].input_power, 1);
          Serial.println("W)");
        }
      }
    }
  }
}

// ===== NODE STATUS CODES =====
/*
#define NODE_STATUS_NORMAL        0     // Operating normally
#define NODE_STATUS_SHADING       1     // Partial shading
#define NODE_STATUS_OVERVOLTAGE   2     // Output voltage too high
#define NODE_STATUS_OVERCURRENT   3     // Output current too high
#define NODE_STATUS_SOFT_FAULT    254   // Severe degradation (90% power loss)
#define NODE_STATUS_HARD_FAULT    255   // Complete failure (no power)
*/

// ===== MODIFIED MAIN LOOP ADDITION =====
/*
In the main loop(), add this after detect_faults():

    // NEW: Detect node failures and apply compensation
    if (current_time - last_fault_check_time >= COMPENSATION_CHECK_INTERVAL) {
      detect_node_failures();
      analyze_and_compensate_faults();
      last_fault_check_time = current_time;
    }
*/
