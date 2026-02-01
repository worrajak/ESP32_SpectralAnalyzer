/**
 * OLED Display Functions (Simplified)
 * Focused on status display, LoRa data, and debug info
 * No MPPT or ADC monitoring
 */

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "lora_config.h"

// ==========================================
// GLOBAL DISPLAY OBJECT
// ==========================================

extern Adafruit_SSD1306 display;

// ==========================================
// DISPLAY INITIALIZATION
// ==========================================

/**
 * Initialize OLED Display (SSD1306 via I2C)
 * @return true if display initialized successfully
 */
bool init_oled_display() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
        Serial.println("[OLED] FAILED to initialize!");
        return false;
    }
    
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Initializing...");
    display.display();
    
    Serial.println("[OLED] Initialized (128x64, I2C Address: 0x3C)");
    return true;
}

// ==========================================
// BASIC DISPLAY FUNCTIONS
// ==========================================

/**
 * Clear and display single message
 * @param message Text to display
 */
void oled_show_message(const char* message) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(message);
    display.display();
}

/**
 * Display two-line message
 * @param line1 First line
 * @param line2 Second line
 */
void oled_show_message(const char* line1, const char* line2) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.println(line1);
    
    display.setCursor(0, 16);
    display.println(line2);
    
    display.display();
}

/**
 * Display three-line message
 * @param line1 First line
 * @param line2 Second line
 * @param line3 Third line
 */
void oled_show_message(const char* line1, const char* line2, const char* line3) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.println(line1);
    
    display.setCursor(0, 16);
    display.println(line2);
    
    display.setCursor(0, 32);
    display.println(line3);
    
    display.display();
}

// ==========================================
// BOOT MESSAGES
// ==========================================

/**
 * Display boot starting message
 */
void oled_boot_start() {
    oled_show_message("===== BOOT =====", "Initializing...");
}

/**
 * Display boot complete message
 */
void oled_boot_complete() {
    oled_show_message("=== READY ===", "ESP32 LoRa GW");
}

/**
 * Display error message
 * @param error Error description
 */
void oled_show_error(const char* error) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("!!! ERROR !!!");
    display.println(error);
    display.display();
}

// ==========================================
// MODE DISPLAY
// ==========================================

/**
 * Display current operation mode
 * @param mode 0=NODE, 1=GATEWAY
 * @param deviceId Device ID number
 */
void oled_show_mode(int mode, int deviceId) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    if (mode == 0) {
        display.println("MODE: NODE+RELAY");
    } else {
        display.println("MODE: GATEWAY");
    }
    
    display.setCursor(0, 16);
    display.print("Device ID: ");
    display.println(deviceId);
    
    display.display();
}

// ==========================================
// LORA STATUS DISPLAY
// ==========================================

/**
 * Display LoRa initialization status
 * @param status true if LoRa OK
 */
void oled_show_lora_status(bool status) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.println("LoRa Status");
    
    display.setCursor(0, 16);
    if (status) {
        display.println("Status: OK");
    } else {
        display.println("Status: FAILED!");
    }
    
    display.display();
}

// ==========================================
// PACKET RECEPTION DISPLAY
// ==========================================

/**
 * Display received packet information
 * @param nodeId Source node ID
 * @param rssi Signal strength (dBm)
 * @param seq Sequence number
 */
void oled_show_packet_rx(int nodeId, int rssi, int seq) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("RX From Node ");
    display.println(nodeId);
    
    display.setCursor(0, 16);
    display.print("RSSI: ");
    display.print(rssi);
    display.println(" dBm");
    
    display.setCursor(0, 32);
    display.print("Seq: ");
    display.println(seq);
    
    display.display();
}

/**
 * Display received sensor data (temperature)
 * @param nodeId Node ID
 * @param temperature Temperature value
 * @param rssi Signal strength
 */
void oled_show_sensor_data(int nodeId, float temperature, int rssi) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("Node ");
    display.print(nodeId);
    display.println(" Data");
    
    display.setCursor(0, 16);
    display.print("Temp: ");
    display.print(temperature, 1);
    display.println("C");
    
    display.setCursor(0, 32);
    display.print("RSSI: ");
    display.print(rssi);
    display.println(" dBm");
    
    display.display();
}

// ==========================================
// STATISTICS DISPLAY
// ==========================================

/**
 * Display packet statistics
 * @param rxCount Packets received
 * @param txCount Packets transmitted
 * @param nodeCount Active nodes
 */
void oled_show_statistics(int rxCount, int txCount, int nodeCount) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.println("=== Statistics ===");
    
    display.setCursor(0, 16);
    display.print("RX: ");
    display.println(rxCount);
    
    display.setCursor(0, 24);
    display.print("TX: ");
    display.println(txCount);
    
    display.setCursor(0, 32);
    display.print("Nodes: ");
    display.println(nodeCount);
    
    display.display();
}

// ==========================================
// WIFI STATUS DISPLAY
// ==========================================

/**
 * Display WiFi connection status
 * @param connected true if connected
 * @param ssid WiFi network name
 * @param ip IP address
 */
void oled_show_wifi_status(bool connected, const char* ssid, const char* ip) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.println("WiFi Status");
    
    display.setCursor(0, 16);
    if (connected) {
        display.println("Status: Connected");
        display.setCursor(0, 24);
        display.print("SSID: ");
        display.println(ssid);
        display.setCursor(0, 32);
        display.print("IP: ");
        display.println(ip);
    } else {
        display.println("Status: Disconnected");
    }
    
    display.display();
}

/**
 * Display WiFi AP mode information
 * @param apIp Access Point IP address
 * @param apSsid AP SSID name
 */
void oled_show_wifi_ap(const char* apIp, const char* apSsid) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.println("WiFi AP Mode");
    
    display.setCursor(0, 16);
    display.print("SSID: ");
    display.println(apSsid);
    
    display.setCursor(0, 32);
    display.print("IP: ");
    display.println(apIp);
    
    display.display();
}

// ==========================================
// MQTT STATUS DISPLAY
// ==========================================

/**
 * Display MQTT connection status
 * @param connected true if connected to broker
 * @param broker MQTT broker address
 */
void oled_show_mqtt_status(bool connected, const char* broker) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.println("MQTT Status");
    
    display.setCursor(0, 16);
    if (connected) {
        display.println("Status: Connected");
    } else {
        display.println("Status: Disconnected");
    }
    
    display.setCursor(0, 32);
    display.print("Broker: ");
    display.println(broker);
    
    display.display();
}

// ==========================================
// BLUETOOTH STATUS DISPLAY
// ==========================================

/**
 * Display Bluetooth status
 * @param enabled true if BT enabled
 * @param connected true if client connected
 * @param deviceName BT device name
 */
void oled_show_bluetooth_status(bool enabled, bool connected, const char* deviceName) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.println("Bluetooth Status");
    
    display.setCursor(0, 16);
    display.print("Enabled: ");
    display.println(enabled ? "Yes" : "No");
    
    display.setCursor(0, 24);
    display.print("Connected: ");
    display.println(connected ? "Yes" : "No");
    
    display.setCursor(0, 32);
    display.print("Name: ");
    display.println(deviceName);
    
    display.display();
}

// ==========================================
// SYSTEM INFO DISPLAY
// ==========================================

/**
 * Display complete system status
 * @param mode Operating mode (0=NODE, 1=GATEWAY)
 * @param deviceId Device ID
 * @param rxCount Packets received
 * @param txCount Packets transmitted
 */
void oled_show_system_info(int mode, int deviceId, int rxCount, int txCount) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Line 1: Mode and Device ID
    display.setCursor(0, 0);
    display.print(mode == 0 ? "[NODE]" : "[GATE]");
    display.print(" ID:");
    display.println(deviceId);
    
    // Line 2: Packet counts
    display.setCursor(0, 10);
    display.print("RX:");
    display.print(rxCount);
    display.print(" TX:");
    display.println(txCount);
    
    // Separator
    display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    
    // Line 3: Status message
    display.setCursor(0, 25);
    display.println("Ready");
    
    display.display();
}

// ==========================================
// PROGRESS/ACTIVITY DISPLAY
// ==========================================

/**
 * Display boot progress
 * @param step Current step (1-5)
 * @param message Status message
 */
void oled_show_boot_progress(int step, const char* message) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.println("===== BOOT =====");
    
    display.setCursor(0, 16);
    display.print("Step ");
    display.print(step);
    display.println("/5");
    
    display.setCursor(0, 32);
    display.println(message);
    
    // Progress bar
    int barWidth = (step * 25);
    display.drawRect(0, 56, 128, 8, SSD1306_WHITE);
    if (barWidth > 0) {
        display.fillRect(1, 57, barWidth - 2, 6, SSD1306_WHITE);
    }
    
    display.display();
}

// ==========================================
// TEMPORARY/OVERLAY DISPLAY
// ==========================================

/**
 * Display temporary alert (5 seconds)
 * @param title Alert title
 * @param message Alert message
 */
void oled_show_alert(const char* title, const char* message) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.println("*** ALERT ***");
    
    display.setCursor(0, 16);
    display.println(title);
    
    display.setCursor(0, 32);
    display.println(message);
    
    display.display();
}

/**
 * Display notification
 * @param message Notification message
 */
void oled_show_notification(const char* message) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print(">>> ");
    display.println(message);
    
    display.display();
}

// ==========================================
// UTILITY FUNCTIONS
// ==========================================

/**
 * Scroll text (simple left-to-right animation)
 * @param text Text to scroll
 * @param speed Delay in milliseconds
 * @param duration Total duration in milliseconds
 */
void oled_scroll_text(const char* text, int speed = 100, int duration = 2000) {
    unsigned long startTime = millis();
    int textLen = strlen(text);
    int maxX = 128;
    
    while (millis() - startTime < duration) {
        for (int x = maxX; x > -textLen * 6; x -= 2) {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(x, 30);
            display.println(text);
            display.display();
            delay(speed);
        }
    }
}

/**
 * Invert display colors
 */
void oled_invert_display() {
    display.invertDisplay(true);
}

/**
 * Normal display colors
 */
void oled_normal_display() {
    display.invertDisplay(false);
}

/**
 * Blink display (turn on/off)
 * @param count Number of blinks
 * @param speed Blink speed in milliseconds
 */
void oled_blink_display(int count = 3, int speed = 100) {
    for (int i = 0; i < count; i++) {
        display.clearDisplay();
        display.display();
        delay(speed);
        
        // Redraw current content
        oled_show_message("Ready");
        delay(speed);
    }
}

#endif // OLED_DISPLAY_H
