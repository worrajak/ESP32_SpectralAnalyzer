# ESP32 LoRa Gateway - Simplified Integration Guide

## Overview
This configuration focuses on **3 core components**:
1. **OLED Display** - Status and data visualization
2. **LoRa Radio** - Wireless communication via RadioHead
3. **Serial Debug** - Terminal logging and diagnostics

**Removed:** MPPT, ADC, Advanced Power Management

---

## Header Files (Simplified)

```
include/
├── lora_config.h           # Configuration constants
├── hardware_init.h         # GPIO, SPI, I2C, Temperature
├── lora_functions.h        # LoRa radio & encryption
├── wifi_functions.h        # WiFi & MQTT (optional)
├── data_structures.h       # Data structures & helpers
├── debug_functions.h       # Serial debug & logging
├── oled_display.h          # OLED display functions
└── node_config.h           # (Existing)
```

---

## Quick Start in main.cpp

### 1. Include Headers
```cpp
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "lora_config.h"
#include "data_structures.h"
#include "debug_functions.h"
#include "hardware_init.h"
#include "lora_functions.h"
#include "oled_display.h"
#include "wifi_functions.h"
```

### 2. Declare Global Objects
```cpp
// Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// LoRa Radio
RH_RF95 rf95(LORA_SS, LORA_DIO0);
RHReliableDatagram manager(rf95, GATEWAY_ADDRESS);

// Bluetooth
BluetoothSerial SerialBT;

// Temperature Sensor
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

// WiFi/MQTT (Optional)
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer server(80);

// Configuration & Status
Config config;
SystemStatus sysStatus;
std::map<int, NodeInfo> nodesDataStore;
std::vector<uint32_t> seenMsgs;
```

### 3. Setup() Function
```cpp
void setup() {
    // Initialize Serial Debug
    init_serial_debug(115200);
    debug_info("System initializing...");
    
    // Initialize GPIO
    init_gpio();
    blink_led(3, 100);
    
    // Initialize I2C and OLED
    init_i2c();
    init_oled_display();
    oled_boot_start();
    
    // Initialize SPI and LoRa
    init_spi();
    init_lora_gpio();
    if (init_lora_radio()) {
        sysStatus.lora_ok = true;
        debug_info("LoRa initialized successfully");
        oled_show_lora_status(true);
    } else {
        debug_error("LoRa initialization failed!");
        oled_show_error("LoRa Failed!");
        while(1) blink_led(5, 50);
    }
    
    // Initialize Temperature Sensor
    init_temperature_sensor();
    
    // Display boot complete
    oled_boot_complete();
    print_boot_complete();
}
```

### 4. Loop() Function
```cpp
void loop() {
    // Check for LoRa packets
    if (manager.available()) {
        uint8_t rxbuf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(rxbuf);
        uint8_t from;
        
        if (manager.recvfromAck(rxbuf, &len, &from)) {
            int rssi = rf95.lastRssi();
            
            // Log packet reception
            print_packet_received(from, len, rssi);
            sysStatus.packets_received++;
            
            // Display on OLED
            oled_show_packet_rx(from, rssi, 0);
            
            if (rssi >= GW_MIN_RSSI && len >= 2) {
                // Verify CRC
                uint16_t receivedCRC = ((uint16_t)rxbuf[len-2] << 8) | rxbuf[len-1];
                if (verify_crc16(rxbuf, len-2, receivedCRC)) {
                    // Decrypt data
                    String decrypted = xor_decrypt_str(rxbuf, len-2);
                    print_decrypted_data(decrypted);
                    
                    // Parse and display sensor data
                    // (implementation depends on payload format)
                    
                    blink_led(1, 50);
                } else {
                    debug_warn("CRC verification failed");
                }
            } else {
                debug_warn("Signal too weak or packet too short");
            }
        }
    }
    
    // Periodic display update
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 5000) {
        oled_show_system_info(config.device_mode, 
                              config.device_id,
                              sysStatus.packets_received,
                              sysStatus.packets_sent);
        lastUpdate = millis();
    }
    
    delay(10);
}
```

---

## Debug Output Examples

### Serial Terminal Output
```
=========================================
  ESP32 LoRa Gateway - Debug Console
=========================================
[DEBUG] Serial initialized at 115200 baud

========== SYSTEM INITIALIZATION ==========
[INFO] System initializing...
[INFO] GPIO pins initialized
[I2C] Bus initialized on GPIO21(SDA), GPIO22(SCL)
[OLED] Initialized (128x64, I2C Address: 0x3C)
[SPI] Bus initialized
[LoRa] Configuration Complete:
  Frequency: 923.00 MHz
  Spreading Factor: 7
  Bandwidth: 125 kHz
  Coding Rate: 4/5
  TX Power: 14 dBm
[LoRa] RadioHead Manager Initialized
  Gateway Address: 1

========== BOOT COMPLETE ==========
Device ready for operation

--- LoRa Packet Received ---
From Node: 101
Length: 25
RSSI: -95 dBm
Decrypted: N:101|S:42|T:23.5

=== LoRa Data Received ===
Node ID: 101
Sequence: 42
Temperature: 23.50 °C
```

### OLED Display Sequence

1. **Boot Start**
   ```
   ===== BOOT =====
   Initializing...
   ```

2. **LoRa Status**
   ```
   LoRa Status
   Status: OK
   ```

3. **Ready State**
   ```
   [GATE] ID:1
   RX:5 TX:0
   ```

4. **Packet Reception**
   ```
   RX From Node 101
   RSSI: -95 dBm
   Seq: 42
   ```

5. **Sensor Data Display**
   ```
   Node 101 Data
   Temp: 23.5C
   RSSI: -95 dBm
   ```

---

## Key Functions by Component

### Serial Debug (`debug_functions.h`)
```cpp
init_serial_debug(115200);      // Initialize serial
debug_info("message");           // Info log
debug_warn("message");           // Warning log
debug_error("message");          // Error log
debug_verbose("message");        // Verbose log
print_packet_received(...);      // Packet info
print_decrypted_data(data);      // Data display
print_hex_dump(buf, len);        // Hex dump
print_memory_stats();            // Memory info
print_uptime(millis());          // Uptime display
```

### OLED Display (`oled_display.h`)
```cpp
init_oled_display();             // Initialize OLED
oled_boot_start();               // Boot message
oled_show_lora_status(true);     // LoRa status
oled_show_packet_rx(id, rssi, seq);    // RX packet
oled_show_sensor_data(id, temp, rssi); // Sensor data
oled_show_system_info(...);      // System status
oled_show_statistics(...);       // Stats display
oled_show_error("message");      // Error message
```

### LoRa Radio (`lora_functions.h`)
```cpp
init_lora_radio();               // Initialize radio
configure_lora();                // Configure parameters
xor_encrypt_str(text, buf, len); // Encrypt data
xor_decrypt_str(buf, len);       // Decrypt data
calculateCRC16(buf, len);        // Calculate CRC
verify_crc16(buf, len, crc);     // Verify CRC
get_last_rssi();                 // Get signal strength
get_hash(sender, seq);           // Generate dedup hash
```

### Hardware (`hardware_init.h`)
```cpp
init_i2c();                      // I2C initialization
init_spi();                      // SPI initialization
init_gpio();                     // GPIO setup
init_lora_gpio();                // LoRa reset
init_temperature_sensor();       // DS18B20 setup
read_temperature();              // Read temp sensor
blink_led(times, delay_ms);      // LED control
```

---

## Packet Format

### Transmit Format (NODE)
```
N:NODE_ID|S:SEQUENCE|T:TEMPERATURE|CRC16
```

Example:
```
N:101|S:42|T:23.5|0x1A2B
```

### Processing Flow
1. **Receive** - RadioHead manager receives packet
2. **RSSI Check** - Verify signal strength >= -130 dBm
3. **CRC Verify** - Check last 2 bytes for CRC16
4. **Decrypt** - XOR decrypt payload (except CRC)
5. **Parse** - Extract node ID, sequence, temperature
6. **Dedup** - Check if message hash already seen
7. **Store** - Save in `nodesDataStore` map
8. **Display** - Show on OLED
9. **Log** - Print to serial terminal
10. **Relay** - Send via Bluetooth (if enabled)

---

## Configuration

### Core Settings (`lora_config.h`)

**LoRa:**
- Frequency: 923 MHz
- Spreading Factor: 7
- Bandwidth: 125 kHz
- TX Power: 14 dBm
- Crypto Key: `1234567890000000` (XOR)

**GPIO:**
- LED: GPIO2
- CONFIG_BUTTON: GPIO0 (BOOT)
- OLED SDA: GPIO21
- OLED SCL: GPIO22
- LoRa SS: GPIO16, RST: GPIO4, DIO0: GPIO35

**Device:**
- Default Mode: NODE_RELAY (0)
- Default ID: 1
- Default Device Name: "ESP32-LoRa"

---

## Testing Checklist

- [ ] Serial debug output appears
- [ ] OLED displays boot messages
- [ ] LoRa radio initializes
- [ ] Packets received show on OLED
- [ ] Decryption works correctly
- [ ] CRC verification passes
- [ ] LED blinks on packet RX
- [ ] Statistics update correctly

---

## Troubleshooting

### OLED Not Displaying
```
Check I2C address: 0x3C (verify with I2C scanner)
Check SDA (GPIO21) and SCL (GPIO22) connections
```

### LoRa Not Working
```
Check SPI connections: SCK(18), MOSI(23), MISO(19), SS(16)
Check Reset (GPIO4) and DIO0 (GPIO35)
Verify frequency 923 MHz matches node
```

### Packets Not Received
```
Check RSSI >= -130 dBm (minimum threshold)
Verify LoRa antenna connections
Check encryption key matches sender
```

### Serial Debug Not Showing
```
Check baud rate: 115200
Verify USB connection
Check GPIO1 (TX) and GPIO3 (RX) are free
```

---

## Memory Usage

- **Heap Available:** ~250 KB (with WiFi disabled)
- **PSRAM:** Not used
- **LittleFS:** ~1 MB (config storage)

---

## Next Steps

1. Test with actual LoRa nodes
2. Add more sensor types (humidity, pressure, voltage)
3. Implement web dashboard
4. Add MQTT relay for cloud integration
5. Implement OTA firmware updates

