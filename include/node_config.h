#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

// Multi-Node DC Converter Configuration
// Set NODE_ID to 1, 2, 3, or 4 for each physical node
// Set MASTER_MAC_ADDR to your master ESP32's MAC address

#define NODE_ID 1
#define MASTER_MAC_ADDR {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}

// Panel Specifications
#define PANEL_VOLTAGE_MAX        35.0
#define PANEL_POWER_RATING       250.0
#define NUM_SERIES_NODES         4
#define TARGET_SYSTEM_VOLTAGE    48.0
#define TARGET_NODE_VOLTAGE      (TARGET_SYSTEM_VOLTAGE / NUM_SERIES_NODES)

// Control Parameters  
#define VOLTAGE_HYSTERESIS       0.5
#define MAX_CURRENT_LIMIT        30.0

#endif
