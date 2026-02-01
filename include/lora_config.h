/**
 * LoRa Configuration & Constants
 * Central definitions for all LoRa parameters
 */

#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

// ==========================================
// OPERATING MODES
// ==========================================
#define MODE_NODE_RELAY 0
#define MODE_GATEWAY 1
#define MODE_CONFIG 2

// ==========================================
// LOCKED CONFIGURATION (DO NOT EDIT)
// ==========================================

// WiFi Networks (LOCKED)
#define WIFI_SSID_1 "Khuankhai_2.4GHz_plus"
#define WIFI_PASS_1 "053811519"
#define WIFI_SSID_2 "DeskTopJak"
#define WIFI_PASS_2 "1234567890"
#define WIFI_SSID_3 "FindX8ProWM"
#define WIFI_PASS_3 "1234567890"

// MQTT Configuration (LOCKED)
#define MQTT_SERVER "203.154.91.187"
#define MQTT_PORT 1883
#define MQTT_USER "prach"
#define MQTT_PASS "prach1234"
#define MQTT_TOPIC "lora/stm32"
#define BT_DEVICE_NAME "LoRa-Gateway-BT"

// LoRa Configuration (LOCKED)
#define LORA_FREQ 923E6
#define LORA_SF 7
#define LORA_BW 125000
#define LORA_CR 5
#define LORA_SYNC_WORD 0x12
#define LORA_TX_POWER 14
#define FIXED_CRYPTO_KEY "1234567890000000"
#define GW_MIN_RSSI -130
#define GATEWAY_ADDRESS 1

// ==========================================
// EDITABLE CONFIGURATION (CAN BE CHANGED IN CONFIG MODE)
// ==========================================

// RSSI Thresholds
#define GW_MIN_RSSI_THRESHOLD -130

// GPIO Pins Configuration
#define CONFIG_BUTTON_PIN 0   // BOOT Button - hold to enter config mode
#define LED_PIN 2             // Status LED
#define DS18B20_PIN 26        // Temperature sensor pin

// AS7343 Spectral Sensor (I2C) - 11-channel spectral sensor
#define AS7343_I2C_ADDRESS 0x39  // AS7343 default I2C address (7-bit)

// OLED Display (I2C)
#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDRESS 0x3C

// LoRa Radio (SPI) - SX127x Module
#define LORA_SCK 18           // SPI Clock
#define LORA_MISO 19          // SPI MISO
#define LORA_MOSI 23          // SPI MOSI
#define LORA_SS 16            // Chip Select
#define LORA_RST 4            // Reset Pin
#define LORA_DIO0 35          // Interrupt Pin

// ==========================================
// SYSTEM CONSTANTS
// ==========================================

#define MAX_PACKET_LEN 255
#define DEFAULT_SEND_INTERVAL 60
#define DEFAULT_DEVICE_ID 1
#define CONFIG_TIMEOUT_MS 300000  // 5 minutes
#define MQTT_RECONNECT_INTERVAL 5000
#define WIFI_CONNECT_TIMEOUT 10000
#define DEDUP_BUFFER_SIZE 50

#endif // LORA_CONFIG_H
