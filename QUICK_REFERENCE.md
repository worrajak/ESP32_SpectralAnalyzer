# Quick Reference - ESP32 LoRa Gateway

## 3 Core Components

### 1️⃣ OLED Display Functions
```cpp
oled_boot_start()                          // Show boot message
oled_show_mode(mode, id)                   // Display mode & device ID
oled_show_lora_status(true/false)          // LoRa status
oled_show_packet_rx(nodeId, rssi, seq)     // RX packet info
oled_show_sensor_data(id, temp, rssi)      // Temperature display
oled_show_system_info(mode, id, rx, tx)    // Full system status
oled_show_statistics(rx, tx, nodes)        // Packet statistics
oled_show_error("message")                 // Error display
oled_show_alert("title", "msg")            // Alert notification
```

### 2️⃣ LoRa Radio Functions
```cpp
init_lora_radio()                          // Initialize RadioHead
configure_lora()                           // Set frequency, SF, BW
xor_encrypt_str(text, buf, len)           // Encrypt with XOR key
xor_decrypt_str(buf, len)                 // Decrypt data
calculateCRC16(buf, len)                  // Calculate checksum
verify_crc16(buf, len, crc)               // Verify integrity
get_last_rssi()                           // Get signal strength
get_hash(sender, seq)                     // Create dedup hash
```

### 3️⃣ Serial Debug Functions
```cpp
init_serial_debug(115200)                 // Initialize serial
debug_info("message")                     // Info level log
debug_warn("message")                     // Warning level log
debug_error("message")                    // Error level log
debug_verbose("message")                  // Verbose level log
print_packet_received(from, len, rssi)    // Log RX packet
print_decrypted_data(data)                // Log decrypted text
print_hex_dump(buf, len)                  // Hex dump to serial
print_memory_stats()                      // Memory information
print_uptime(millis())                    // System uptime
```

---

## Basic Loop Structure

```cpp
void loop() {
    // Receive LoRa packet
    if (manager.available()) {
        uint8_t rxbuf[255];
        uint8_t len = sizeof(rxbuf);
        uint8_t from;
        
        if (manager.recvfromAck(rxbuf, &len, &from)) {
            int rssi = rf95.lastRssi();
            
            // 1. Log reception
            print_packet_received(from, len, rssi);
            sysStatus.packets_received++;
            
            // 2. Display on OLED
            oled_show_packet_rx(from, rssi, 0);
            
            // 3. Verify CRC
            uint16_t rxCRC = ((uint16_t)rxbuf[len-2] << 8) | rxbuf[len-1];
            if (verify_crc16(rxbuf, len-2, rxCRC)) {
                
                // 4. Decrypt
                String data = xor_decrypt_str(rxbuf, len-2);
                print_decrypted_data(data);
                
                // 5. Parse & display
                // Extract node ID, temperature, etc.
                
                // 6. Update display
                oled_show_sensor_data(nodeId, temp, rssi);
                
                blink_led(1, 50);
            }
        }
    }
    
    delay(10);
}
```

---

## Pin Assignments

| Component | GPIO | Purpose |
|-----------|------|---------|
| **OLED** | 21 (SDA) | I2C Data |
| | 22 (SCL) | I2C Clock |
| **LoRa SPI** | 18 | Clock (SCK) |
| | 19 | Master In (MISO) |
| | 23 | Master Out (MOSI) |
| | 16 | Chip Select (SS) |
| | 4 | Reset (RST) |
| | 35 | Interrupt (DIO0) |
| **GPIO** | 2 | LED Status |
| | 0 | Config Button (BOOT) |
| | 26 | Temperature Sensor |

---

## LoRa Configuration

| Parameter | Value |
|-----------|-------|
| Frequency | 923 MHz |
| Spreading Factor | 7 |
| Bandwidth | 125 kHz |
| Coding Rate | 4/5 |
| TX Power | 14 dBm |
| Sync Word | 0x12 |
| Min RSSI | -130 dBm |
| Gateway Address | 1 |
| Crypto Key | `1234567890000000` |

---

## Packet Format

**TX Payload:**
```
[Encrypted Data (N bytes)] [CRC16_HIGH] [CRC16_LOW]
```

**Decrypted Format:**
```
N:NODE_ID|S:SEQUENCE|T:TEMPERATURE
```

**Example:**
```
N:101|S:42|T:23.5
```

---

## Data Structures

### Config Structure
```cpp
struct Config {
    int device_mode;        // 0=NODE, 1=GATEWAY
    int device_id;          // 1-250
    bool enable_wifi;       // WiFi on/off
    bool enable_bt;         // Bluetooth on/off
    bool enable_lora;       // LoRa on/off
    int send_interval;      // Seconds (NODE)
    String device_name;     // Device name
};
```

### SystemStatus Structure
```cpp
struct SystemStatus {
    bool lora_ok;           // LoRa operational
    bool oled_ok;           // OLED operational
    bool bt_ok;             // Bluetooth operational
    bool wifi_connected;    // WiFi connected
    bool mqtt_connected;    // MQTT connected
    int packets_received;   // RX count
    int packets_sent;       // TX count
};
```

### NodeInfo Structure
```cpp
struct NodeInfo {
    float t, h, b;          // Temperature, Humidity, Battery
    float v, a, w, wh;      // Voltage, Amperage, Power, Energy
    int rssi;               // Signal strength
    int seq;                // Sequence number
    unsigned long ts_local; // Timestamp
};
```

---

## Common Tasks

### Display Boot Progress
```cpp
oled_show_boot_progress(1, "Init I2C...");
oled_show_boot_progress(2, "Init SPI...");
oled_show_boot_progress(3, "Init LoRa...");
oled_show_boot_progress(4, "Init BT...");
oled_show_boot_progress(5, "Ready!");
```

### Log Different Message Types
```cpp
debug_error("LoRa init failed!");
debug_warn("Signal weak: -110 dBm");
debug_info("Packet RX", 5);
debug_verbose("Processing packet from node 101");
```

### Handle Received Data
```cpp
// Check for duplicates
if (!is_duplicate_msg(hash)) {
    add_msg_hash(hash, 50);
    // Process new message
} else {
    debug_verbose("Duplicate message ignored");
}
```

### Display Node Statistics
```cpp
oled_show_statistics(
    sysStatus.packets_received,
    sysStatus.packets_sent,
    nodesDataStore.size()
);
```

### Update Every 5 Seconds
```cpp
static unsigned long lastUpdate = 0;
if (millis() - lastUpdate > 5000) {
    oled_show_system_info(
        config.device_mode,
        config.device_id,
        sysStatus.packets_received,
        sysStatus.packets_sent
    );
    lastUpdate = millis();
}
```

---

## Debug Output Levels

| Level | Function | Output |
|-------|----------|--------|
| ERROR | `debug_error()` | 🔴 Critical errors |
| WARN | `debug_warn()` | 🟡 Warnings |
| INFO | `debug_info()` | 🔵 Normal info |
| VERBOSE | `debug_verbose()` | ⚪ Detailed info |

**Set level in `debug_functions.h`:**
```cpp
#define DEBUG_LEVEL DEBUG_VERBOSE  // Set to: ERROR, WARN, INFO, VERBOSE
```

---

## Compilation Checklist

- [ ] All 7 header files in `include/` folder
- [ ] `#include` order in main.cpp correct
- [ ] Global objects declared
- [ ] Libraries installed (RadioHead, ArduinoJson, SSD1306, etc.)
- [ ] Serial baud rate 115200
- [ ] I2C address 0x3C for OLED
- [ ] SPI frequency correct for LoRa

---

## File Locations

```
SpectrumAnalyzer/
├── platformio.ini
├── src/
│   └── main.cpp
└── include/
    ├── lora_config.h
    ├── hardware_init.h
    ├── lora_functions.h
    ├── wifi_functions.h
    ├── data_structures.h
    ├── debug_functions.h
    ├── oled_display.h
    └── node_config.h
```

---

## Serial Terminal Commands

Send from serial monitor (115200 baud):

```
STATUS              // Print system status
NODES               // List active nodes
CLEAR               // Clear stored data
REBOOT              // Restart device
WIFI_SCAN           // Scan WiFi networks
```

---

## Useful Macros

```cpp
#define LORA_FREQ 923E6              // 923 MHz
#define LORA_SF 7                    // Spreading Factor
#define LORA_BW 125000               // Bandwidth 125 kHz
#define GATEWAY_ADDRESS 1            // Gateway ID
#define GW_MIN_RSSI -130             // Min signal threshold
#define FIXED_CRYPTO_KEY "1234567890000000"  // XOR key
```

---

## Memory Tips

- **Heap:** ~250 KB available
- **Store strings in PROGMEM:**
  ```cpp
  const char str[] PROGMEM = "Message";
  ```
- **Use `F()` for literals:**
  ```cpp
  Serial.println(F("Message"));
  ```
- **Monitor with:**
  ```cpp
  print_memory_stats();
  ```

---

## Next Steps

1. ✅ Set up three core components (OLED, LoRa, Debug)
2. ✅ Test packet reception and display
3. ✅ Verify encryption/decryption
4. 🔲 Add Bluetooth relay
5. 🔲 Add WiFi & MQTT
6. 🔲 Build web dashboard
7. 🔲 Implement OTA updates

