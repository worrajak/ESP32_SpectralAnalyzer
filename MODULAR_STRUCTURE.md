# ESP32 LoRa Gateway - Header Files Organization

This document describes the modular header files created to organize the ESP32 LoRa Gateway codebase.

## File Structure

```
include/
├── lora_config.h           # Configuration constants (LOCKED & EDITABLE)
├── hardware_init.h         # Hardware initialization (OLED, SPI, I2C, GPIO)
├── lora_functions.h        # LoRa radio functions (RadioHead, encryption, CRC)
├── wifi_functions.h        # WiFi & MQTT functions
├── data_structures.h       # Global data structures and helpers
└── node_config.h          # (Existing - node specific config)
```

## Header Files Description

### 1. **lora_config.h**
Central configuration file for all constants and GPIO pins.

**Contains:**
- Operating mode definitions (NODE_RELAY, GATEWAY, CONFIG)
- WiFi networks credentials (LOCKED)
- MQTT broker configuration (LOCKED)
- LoRa radio parameters (frequency, SF, bandwidth, etc.)
- GPIO pin assignments (LEDs, buttons, SPI, I2C)
- System constants and timeouts

**Usage:**
```cpp
#include "lora_config.h"
// Access: LORA_FREQ, GATEWAY_ADDRESS, OLED_SDA, etc.
```

---

### 2. **hardware_init.h**
Hardware-level initialization and I/O functions.

**Functions:**
- `init_i2c()` - Initialize I2C bus for OLED
- `init_spi()` - Initialize SPI bus for LoRa
- `init_oled()` - Setup SSD1306 display
- `init_gpio()` - Setup GPIO pins (LED, button)
- `init_lora_gpio()` - LoRa reset sequence
- `init_temperature_sensor()` - DS18B20 setup
- `read_temperature()` - Read temperature value
- `update_oled()` - Display status on OLED
- `blink_led()` - LED indicator control
- `display_boot_message()` - Show boot message

**Global Objects:**
```cpp
extern Adafruit_SSD1306 display;
extern OneWire oneWire;
extern DallasTemperature sensors;
```

**Usage:**
```cpp
#include "hardware_init.h"

void setup() {
    init_i2c();
    init_spi();
    init_oled();
    init_gpio();
    init_temperature_sensor();
}
```

---

### 3. **lora_functions.h**
RadioHead library functions and LoRa-specific operations.

**Functions:**
- `configure_lora()` - Set frequency, SF, bandwidth, TX power
- `init_lora_radio()` - Initialize RadioHead manager
- `xor_encrypt_str()` - Encrypt data with XOR cipher
- `xor_decrypt_str()` - Decrypt XOR encrypted data
- `calculateCRC16()` - Calculate CRC16 checksum
- `verify_crc16()` - Verify CRC16 integrity
- `get_hash()` - Generate deduplication hash
- `get_last_rssi()` - Get last received signal strength
- `get_last_snr()` - Get signal-to-noise ratio
- `print_radio_diagnostics()` - Debug LoRa status

**Global Objects:**
```cpp
extern RH_RF95 rf95;
extern RHReliableDatagram manager;
```

**Usage:**
```cpp
#include "lora_functions.h"

// Encrypt message
uint8_t encrypted[255];
int encLen = 0;
xor_encrypt_str("Hello", encrypted, encLen);

// Calculate CRC
uint16_t crc = calculateCRC16(encrypted, encLen);

// Verify integrity
bool ok = verify_crc16(encrypted, encLen, receivedCRC);
```

---

### 4. **wifi_functions.h**
WiFi connectivity and MQTT communication functions.

**Functions:**
- `wifi_event_handler()` - WiFi state change callback
- `mqtt_callback()` - MQTT message callback
- `connect_mqtt()` - Connect to MQTT broker
- `disconnect_mqtt()` - Close MQTT connection
- `mqtt_publish()` - Publish MQTT message
- `init_wifi_sta()` - WiFi Station mode (client)
- `init_wifi_ap()` - WiFi Access Point mode
- `get_wifi_status_json()` - Get status as JSON
- `print_wifi_diagnostics()` - Debug WiFi status
- `init_web_server()` - Start web server
- `stop_web_server()` - Stop web server

**Global Objects:**
```cpp
extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern WebServer server;
```

**Usage:**
```cpp
#include "wifi_functions.h"

// Initialize WiFi
WiFi.onEvent(wifi_event_handler);
if (init_wifi_sta()) {
    Serial.println("Connected!");
}

// Connect to MQTT
connect_mqtt();

// Publish data
mqtt_publish("lora/data", "{...json...}");
```

---

### 5. **data_structures.h**
Global data structures and helper functions.

**Structures:**

#### Config
```cpp
struct Config {
    int device_mode;      // 0=NODE, 1=GATEWAY
    int device_id;        // 1-250
    bool enable_wifi;     // Enable WiFi
    bool enable_bt;       // Enable Bluetooth
    bool enable_lora;     // Enable LoRa
    int send_interval;    // Seconds (NODE mode)
    String device_name;   // Device name
};
```

#### SystemStatus
```cpp
struct SystemStatus {
    bool lora_ok, oled_ok, bt_ok, ds18b20_ok;
    bool wifi_connected, mqtt_connected, ap_mode;
    String wifi_ssid, ip_address, ap_ip;
    int packets_received, packets_sent, gateway_relayed;
    int device_mode;
    unsigned long uptime_ms;
};
```

#### NodeInfo
```cpp
struct NodeInfo {
    float t, h, b, v, a, w, wh;  // Sensor values
    int rssi, via, seq;
    String path;
    unsigned long ts_local, ts_recv;
};
```

**Global Storage:**
```cpp
extern std::map<int, NodeInfo> nodesDataStore;    // Node data by ID
extern std::vector<uint32_t> seenMsgs;            // Deduplication buffer
```

**Helper Functions:**
- `clear_nodes_data()` - Reset all node data
- `clear_dedup_buffer()` - Clear message hashes
- `get_node_count()` - Get active node count
- `get_node_data(id)` - Retrieve node info
- `update_node_data(id, info)` - Update node
- `is_duplicate_msg(hash)` - Check duplicate
- `add_msg_hash(hash)` - Add to dedup buffer
- `print_system_status()` - Print full status

**Usage:**
```cpp
#include "data_structures.h"

// Get node count
int nodes = get_node_count();

// Retrieve node data
NodeInfo node = get_node_data(101);
Serial.println(node.t);  // Temperature

// Check for duplicate
if (is_duplicate_msg(hash)) {
    Serial.println("Duplicate message!");
}

// Print complete status
print_system_status(sysStatus);
```

---

## Integration with main.cpp

### Before (main.cpp)
```cpp
// All 1500+ lines in one file
#include <Arduino.h>
#include <SPI.h>
// ... 40+ includes
// ... configuration constants mixed with code
// ... all functions in one file
```

### After (main.cpp with headers)
```cpp
#include <Arduino.h>
#include "lora_config.h"
#include "hardware_init.h"
#include "lora_functions.h"
#include "wifi_functions.h"
#include "data_structures.h"

// Global objects initialization
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RH_RF95 rf95(LORA_SS, LORA_DIO0);
RHReliableDatagram manager(rf95, GATEWAY_ADDRESS);
BluetoothSerial SerialBT;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
WebServer server(80);

// Configuration and status
Config config;
SystemStatus sysStatus;
std::map<int, NodeInfo> nodesDataStore;
std::vector<uint32_t> seenMsgs;

// ... rest of main.cpp (setup, loop, handlers)
```

---

## Benefits of Modular Structure

1. **Reusability** - Functions can be used in other ESP32 projects
2. **Maintainability** - Easy to locate and modify specific functionality
3. **Testability** - Each module can be tested independently
4. **Documentation** - Clear separation of concerns
5. **Scalability** - Easy to add new features without cluttering code
6. **Collaboration** - Multiple developers can work on different modules
7. **Code Organization** - Logical grouping of related functions
8. **Performance** - Only included headers are compiled

---

## Compilation

All headers are self-contained and can be included in any order (except data_structures.h which should be included after platform-specific headers).

**Recommended include order in main.cpp:**
```cpp
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
// ... all Arduino/library includes

#include "lora_config.h"          // 1. Configuration constants
#include "data_structures.h"       // 2. Data structures
#include "hardware_init.h"         // 3. Hardware functions
#include "lora_functions.h"        // 4. LoRa functions
#include "wifi_functions.h"        // 5. Network functions
```

---

## File Sizes

- **lora_config.h** - ~1.5 KB
- **hardware_init.h** - ~3.5 KB
- **lora_functions.h** - ~4.0 KB
- **wifi_functions.h** - ~6.0 KB
- **data_structures.h** - ~5.0 KB

**Total** - ~20 KB (original main.cpp: ~60 KB with all functions)

---

## Future Enhancements

1. Add `bluetooth_functions.h` for Bluetooth serial operations
2. Add `web_handlers.h` for web server route handlers
3. Add `config_manager.h` for LittleFS configuration operations
4. Add `mqtt_handlers.h` for MQTT-specific message processing
5. Add `packet_parser.h` for decoding sensor payloads

---

## Notes

- All headers use `#ifndef` guards to prevent multiple inclusion
- Global objects are declared as `extern` in headers and defined in main.cpp
- Forward declarations prevent circular dependencies
- Inline helper functions in data_structures.h provide zero-overhead abstraction

