/**
 * Data Structures & System Definitions
 * Configuration, node data, and system status structures
 */

#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <Arduino.h>
#include <map>
#include <vector>

// ==========================================
// CONFIGURATION STRUCTURE
// ==========================================

/**
 * Device Configuration
 * Can be modified via web interface and saved to SPIFFS/LittleFS
 */
struct Config {
    int device_mode;        // 0=NODE+RELAY, 1=GATEWAY
    int device_id;          // Device ID (1-250 for LoRa nodes)
    bool enable_wifi;       // Enable WiFi connectivity
    bool enable_bt;         // Enable Bluetooth Serial
    bool enable_lora;       // Enable LoRa radio
    int send_interval;      // Send interval in seconds (NODE mode)
    String device_name;     // Device identifier string
    
    // Constructor with defaults
    Config() : 
        device_mode(0),
        device_id(1),
        enable_wifi(false),
        enable_bt(true),
        enable_lora(true),
        send_interval(60),
        device_name("ESP32-LoRa") {}
};

// ==========================================
// SYSTEM STATUS STRUCTURE
// ==========================================

/**
 * System Runtime Status
 * Real-time status of all subsystems
 */
struct SystemStatus {
    // Hardware status
    bool lora_ok;           // LoRa radio operational
    bool oled_ok;           // OLED display operational
    bool bt_ok;             // Bluetooth operational
    bool ds18b20_ok;        // Temperature sensor operational
    
    // Connectivity
    bool wifi_connected;    // WiFi station connected
    bool mqtt_connected;    // MQTT broker connected
    bool ap_mode;           // WiFi Access Point active
    
    // Network info
    String wifi_ssid;       // Connected WiFi network name
    String ip_address;      // Station mode IP address
    String ap_ip;           // Access Point IP address
    
    // Statistics
    int packets_received;   // Total LoRa packets received
    int packets_sent;       // Total packets transmitted
    int gateway_relayed;    // Packets relayed (NODE mode)
    
    // Runtime info
    int device_mode;        // Current operating mode
    unsigned long uptime_ms; // System uptime in milliseconds
    
    // Constructor with defaults
    SystemStatus() :
        lora_ok(false),
        oled_ok(false),
        bt_ok(false),
        ds18b20_ok(false),
        wifi_connected(false),
        mqtt_connected(false),
        ap_mode(false),
        wifi_ssid(""),
        ip_address(""),
        ap_ip(""),
        packets_received(0),
        packets_sent(0),
        gateway_relayed(0),
        device_mode(0),
        uptime_ms(0) {}
};

// ==========================================
// NODE DATA STORAGE STRUCTURE
// ==========================================

/**
 * Information from a single LoRa node
 * Stores sensor readings and metadata
 */
struct NodeInfo {
    float t;                // Temperature (°C) or Voltage (V)
    float h;                // Humidity (%) - optional
    float b;                // Battery (V) or Power (W)
    float v;                // Voltage (V) - PZEM
    float a;                // Current (A) - PZEM
    float w;                // Power (W) - PZEM
    float wh;               // Energy (Wh) - PZEM
    
    int rssi;               // Received Signal Strength (dBm)
    String path;            // Route path (DIRECT, RELAY, etc)
    int via;                // Relay node ID (if relayed)
    int seq;                // Sequence number
    
    unsigned long ts_local; // Local timestamp (millis)
    unsigned long ts_recv;  // Receive timestamp
    
    // Constructor with defaults
    NodeInfo() :
        t(0), h(0), b(0), v(0), a(0), w(0), wh(0),
        rssi(-130), path(""), via(0), seq(0),
        ts_local(0), ts_recv(0) {}
};

// ==========================================
// GLOBAL DATA STORAGE
// ==========================================

// Map: Node ID -> Node Data
extern std::map<int, NodeInfo> nodesDataStore;

// Deduplication buffer: stores message hashes
extern std::vector<uint32_t> seenMsgs;

// ==========================================
// HELPER FUNCTIONS FOR DATA STRUCTURES
// ==========================================

/**
 * Clear all node data
 */
inline void clear_nodes_data() {
    nodesDataStore.clear();
    Serial.println("[Data] Cleared all node data");
}

/**
 * Clear deduplication buffer
 */
inline void clear_dedup_buffer() {
    seenMsgs.clear();
    Serial.println("[Data] Cleared deduplication buffer");
}

/**
 * Get node count
 * @return Number of active nodes
 */
inline int get_node_count() {
    return nodesDataStore.size();
}

/**
 * Get node data by ID
 * @param node_id Node ID to retrieve
 * @return NodeInfo structure, or default if not found
 */
inline NodeInfo get_node_data(int node_id) {
    if (nodesDataStore.find(node_id) != nodesDataStore.end()) {
        return nodesDataStore[node_id];
    }
    return NodeInfo();  // Return default/empty struct
}

/**
 * Update node data
 * @param node_id Node ID
 * @param info New node information
 */
inline void update_node_data(int node_id, const NodeInfo& info) {
    nodesDataStore[node_id] = info;
}

/**
 * Check if message is duplicate
 * @param hash Message hash (combination of sender + sequence)
 * @return true if duplicate found
 */
inline bool is_duplicate_msg(uint32_t hash) {
    for (uint32_t h : seenMsgs) {
        if (h == hash) {
            return true;
        }
    }
    return false;
}

/**
 * Add message hash to deduplication buffer
 * @param hash Message hash to add
 * @param max_buffer Maximum buffer size before cleanup
 */
inline void add_msg_hash(uint32_t hash, int max_buffer = 50) {
    seenMsgs.push_back(hash);
    if ((int)seenMsgs.size() > max_buffer) {
        seenMsgs.erase(seenMsgs.begin());
    }
}

/**
 * Print system status to Serial
 */
void print_system_status(const SystemStatus& status) {
    Serial.println("\n========== SYSTEM STATUS ==========");
    Serial.print("Mode: ");
    Serial.println((status.device_mode == 0) ? "NODE+RELAY" : "GATEWAY");
    Serial.println("\n[Hardware]");
    Serial.print("  LoRa: ");
    Serial.println(status.lora_ok ? "OK" : "FAIL");
    Serial.print("  OLED: ");
    Serial.println(status.oled_ok ? "OK" : "FAIL");
    Serial.print("  Bluetooth: ");
    Serial.println(status.bt_ok ? "OK" : "FAIL");
    Serial.print("  DS18B20: ");
    Serial.println(status.ds18b20_ok ? "OK" : "FAIL");
    
    Serial.println("\n[Connectivity]");
    Serial.print("  WiFi: ");
    Serial.println(status.wifi_connected ? "Connected" : "Disconnected");
    if (status.wifi_connected) {
        Serial.print("    SSID: ");
        Serial.println(status.wifi_ssid);
        Serial.print("    IP: ");
        Serial.println(status.ip_address);
    }
    
    if (status.ap_mode) {
        Serial.print("    AP IP: ");
        Serial.println(status.ap_ip);
    }
    
    Serial.print("  MQTT: ");
    Serial.println(status.mqtt_connected ? "Connected" : "Disconnected");
    
    Serial.println("\n[Statistics]");
    Serial.print("  RX Packets: ");
    Serial.println(status.packets_received);
    Serial.print("  TX Packets: ");
    Serial.println(status.packets_sent);
    Serial.print("  Relayed: ");
    Serial.println(status.gateway_relayed);
    Serial.print("  Active Nodes: ");
    Serial.println(nodesDataStore.size());
    
    Serial.println("===================================\n");
}

#endif // DATA_STRUCTURES_H
