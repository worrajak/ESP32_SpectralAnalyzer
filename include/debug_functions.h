/**
 * Serial Debug & Logging Functions
 * Terminal output and debugging utilities
 */

#ifndef DEBUG_FUNCTIONS_H
#define DEBUG_FUNCTIONS_H

#include <Arduino.h>

// ==========================================
// DEBUG LEVELS
// ==========================================
#define DEBUG_NONE 0
#define DEBUG_ERROR 1
#define DEBUG_WARN 2
#define DEBUG_INFO 3
#define DEBUG_VERBOSE 4

// Current debug level
#define DEBUG_LEVEL DEBUG_VERBOSE

// ==========================================
// SERIAL DEBUG OUTPUT
// ==========================================

/**
 * Initialize Serial port for debugging
 * @param baud Baud rate (default: 115200)
 */
void init_serial_debug(unsigned long baud = 115200) {
    Serial.begin(baud);
    delay(500);
    Serial.println("\n\n");
    Serial.println("=========================================");
    Serial.println("  ESP32 LoRa Gateway - Debug Console");
    Serial.println("=========================================");
    Serial.println("[DEBUG] Serial initialized at " + String(baud) + " baud");
}

/**
 * Print timestamp with debug level
 * @param level Debug level (ERROR, WARN, INFO, VERBOSE)
 */
void print_debug_header(const char* level) {
    unsigned long ms = millis();
    unsigned long sec = ms / 1000;
    unsigned long msec = ms % 1000;
    
    Serial.print("[");
    Serial.print(sec);
    Serial.print(".");
    Serial.printf("%03lu", msec);
    Serial.print("] ");
    Serial.print(level);
    Serial.print(": ");
}

// ==========================================
// ERROR LOGGING
// ==========================================

/**
 * Log error message
 * @param message Error message
 */
void debug_error(const char* message) {
    if (DEBUG_LEVEL >= DEBUG_ERROR) {
        print_debug_header("[ERROR]");
        Serial.println(message);
    }
}

/**
 * Log error with value
 * @param message Error message
 * @param value Numeric value
 */
void debug_error(const char* message, int value) {
    if (DEBUG_LEVEL >= DEBUG_ERROR) {
        print_debug_header("[ERROR]");
        Serial.print(message);
        Serial.println(value);
    }
}

// ==========================================
// WARNING LOGGING
// ==========================================

/**
 * Log warning message
 * @param message Warning message
 */
void debug_warn(const char* message) {
    if (DEBUG_LEVEL >= DEBUG_WARN) {
        print_debug_header("[WARN]");
        Serial.println(message);
    }
}

/**
 * Log warning with value
 * @param message Warning message
 * @param value Numeric value
 */
void debug_warn(const char* message, float value) {
    if (DEBUG_LEVEL >= DEBUG_WARN) {
        print_debug_header("[WARN]");
        Serial.print(message);
        Serial.println(value);
    }
}

// ==========================================
// INFO LOGGING
// ==========================================

/**
 * Log info message
 * @param message Info message
 */
void debug_info(const char* message) {
    if (DEBUG_LEVEL >= DEBUG_INFO) {
        print_debug_header("[INFO]");
        Serial.println(message);
    }
}

/**
 * Log info with value
 * @param message Info message
 * @param value Numeric value
 */
void debug_info(const char* message, int value) {
    if (DEBUG_LEVEL >= DEBUG_INFO) {
        print_debug_header("[INFO]");
        Serial.print(message);
        Serial.println(value);
    }
}

/**
 * Log info with float value
 * @param message Info message
 * @param value Float value
 * @param decimals Decimal places
 */
void debug_info(const char* message, float value, int decimals = 2) {
    if (DEBUG_LEVEL >= DEBUG_INFO) {
        print_debug_header("[INFO]");
        Serial.print(message);
        Serial.println(value, decimals);
    }
}

// ==========================================
// VERBOSE LOGGING
// ==========================================

/**
 * Log verbose message
 * @param message Verbose message
 */
void debug_verbose(const char* message) {
    if (DEBUG_LEVEL >= DEBUG_VERBOSE) {
        print_debug_header("[VERBOSE]");
        Serial.println(message);
    }
}

/**
 * Log verbose with value
 * @param message Message
 * @param value Value
 */
void debug_verbose(const char* message, int value) {
    if (DEBUG_LEVEL >= DEBUG_VERBOSE) {
        print_debug_header("[VERBOSE]");
        Serial.print(message);
        Serial.println(value);
    }
}

// ==========================================
// SYSTEM DIAGNOSTICS
// ==========================================

/**
 * Print system initialization message
 */
void print_init_banner() {
    Serial.println("\n========== SYSTEM INITIALIZATION ==========");
}

/**
 * Print system ready message
 */
void print_ready_banner() {
    Serial.println("\n========== SYSTEM READY ==========");
}

/**
 * Print boot complete
 */
void print_boot_complete() {
    Serial.println("\n========== BOOT COMPLETE ==========");
    Serial.println("Device ready for operation\n");
}

/**
 * Print received packet information
 * @param from Sender node ID
 * @param len Packet length
 * @param rssi RSSI value
 */
void print_packet_received(uint8_t from, uint8_t len, int rssi) {
    Serial.println("\n--- LoRa Packet Received ---");
    Serial.print("From Node: ");
    Serial.println(from);
    Serial.print("Length: ");
    Serial.println(len);
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm");
}

/**
 * Print decrypted data
 * @param data Decrypted data string
 */
void print_decrypted_data(const String& data) {
    Serial.print("Decrypted: ");
    Serial.println(data);
}

/**
 * Print parsed node data
 * @param nodeId Node ID
 * @param seq Sequence number
 * @param temp Temperature value
 */
void print_node_data(int nodeId, int seq, float temp) {
    Serial.println("\n=== LoRa Data Received ===");
    Serial.print("Node ID: ");
    Serial.println(nodeId);
    Serial.print("Sequence: ");
    Serial.println(seq);
    Serial.print("Temperature: ");
    Serial.print(temp, 2);
    Serial.println(" °C");
}

/**
 * Print Bluetooth transmission
 * @param data Data transmitted
 */
void print_bt_transmission(const String& data) {
    Serial.print("BT TX: ");
    Serial.println(data);
}

/**
 * Print MQTT publication
 * @param topic MQTT topic
 * @param message Message payload
 */
void print_mqtt_publish(const char* topic, const char* message) {
    Serial.print("MQTT Pub [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(message);
}

/**
 * Print separator line
 * @param char Character to use for line (default: '=')
 * @param count Number of characters (default: 40)
 */
void print_separator(char ch = '=', int count = 40) {
    for (int i = 0; i < count; i++) {
        Serial.print(ch);
    }
    Serial.println();
}

/**
 * Print section header
 * @param title Section title
 */
void print_section(const char* title) {
    Serial.println();
    print_separator();
    Serial.println(title);
    print_separator();
}

/**
 * Get free heap memory
 * @return Free heap in bytes
 */
size_t get_free_heap() {
    return ESP.getFreeHeap();
}

/**
 * Print memory statistics
 */
void print_memory_stats() {
    Serial.println("\n[Memory Statistics]");
    Serial.print("  Free Heap: ");
    Serial.print(get_free_heap());
    Serial.println(" bytes");
    Serial.print("  Total Heap: ");
    Serial.print(ESP.getHeapSize());
    Serial.println(" bytes");
}

/**
 * Print uptime
 * @param ms Milliseconds since boot
 */
void print_uptime(unsigned long ms) {
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    Serial.print("Uptime: ");
    if (days > 0) {
        Serial.print(days);
        Serial.print("d ");
    }
    Serial.print(hours % 24);
    Serial.print("h ");
    Serial.print(minutes % 60);
    Serial.print("m ");
    Serial.print(seconds % 60);
    Serial.println("s");
}

/**
 * Hexdump buffer to serial
 * @param buffer Data buffer
 * @param len Buffer length
 * @param bytes_per_line Bytes to print per line (default: 16)
 */
void print_hex_dump(const uint8_t* buffer, size_t len, int bytes_per_line = 16) {
    Serial.println("[Hex Dump]");
    
    for (size_t i = 0; i < len; i += bytes_per_line) {
        // Print offset
        Serial.printf("%04X: ", (unsigned int)i);
        
        // Print hex values
        for (int j = 0; j < bytes_per_line && (i + j) < len; j++) {
            Serial.printf("%02X ", buffer[i + j]);
        }
        
        // Print ASCII
        Serial.print("| ");
        for (int j = 0; j < bytes_per_line && (i + j) < len; j++) {
            char c = buffer[i + j];
            Serial.print((c >= 32 && c <= 126) ? c : '.');
        }
        Serial.println();
    }
}

// ==========================================
// TIMESTAMP UTILITIES
// ==========================================

/**
 * Get formatted time string
 * @return Time string "HH:MM:SS"
 */
String get_time_string() {
    unsigned long ms = millis();
    unsigned long sec = (ms / 1000) % 60;
    unsigned long min = (ms / 60000) % 60;
    unsigned long hr = (ms / 3600000) % 24;
    
    char buf[9];
    sprintf(buf, "%02lu:%02lu:%02lu", hr, min, sec);
    return String(buf);
}

#endif // DEBUG_FUNCTIONS_H
