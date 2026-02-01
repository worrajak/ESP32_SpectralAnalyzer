/**
 * WiFi & MQTT Functions
 * WiFi connectivity, MQTT messaging, and web server handlers
 */

#ifndef WIFI_FUNCTIONS_H
#define WIFI_FUNCTIONS_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "lora_config.h"

// ==========================================
// GLOBAL NETWORK OBJECTS
// ==========================================

extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern WebServer server;

// Forward declarations of status structures (defined in main.cpp)
extern struct SystemStatus {
    bool lora_ok;
    bool oled_ok;
    bool bt_ok;
    bool wifi_connected;
    bool mqtt_connected;
    bool ap_mode;
    String wifi_ssid;
    String ip_address;
    String ap_ip;
    int packets_received;
    int packets_sent;
    int device_mode;
} sysStatus;

extern struct Config {
    int device_mode;
    int device_id;
    bool enable_wifi;
    bool enable_bt;
    bool enable_lora;
    int send_interval;
    String device_name;
} config;

// ==========================================
// WIFI EVENT HANDLER
// ==========================================

/**
 * WiFi Event Handler Callback
 * Handles WiFi connection state changes
 */
void wifi_event_handler(WiFiEvent_t event) {
    switch(event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("[WiFi] STA mode started");
            break;
            
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("[WiFi] Connected to SSID");
            break;
            
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("[WiFi] Got IP: ");
            Serial.println(WiFi.localIP());
            sysStatus.wifi_connected = true;
            sysStatus.ip_address = WiFi.localIP().toString();
            sysStatus.wifi_ssid = WiFi.SSID();
            break;
            
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("[WiFi] Disconnected from SSID");
            sysStatus.wifi_connected = false;
            break;
            
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("[WiFi] AP mode started");
            sysStatus.ap_mode = true;
            sysStatus.ap_ip = WiFi.softAPIP().toString();
            Serial.print("[WiFi] AP IP: ");
            Serial.println(sysStatus.ap_ip);
            break;
            
        case ARDUINO_EVENT_WIFI_AP_STOP:
            Serial.println("[WiFi] AP mode stopped");
            sysStatus.ap_mode = false;
            break;
            
        default:
            break;
    }
}

// ==========================================
// MQTT CALLBACK & FUNCTIONS
// ==========================================

/**
 * MQTT Message Callback
 * Called when message received on subscribed topic
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("[MQTT] Message received on topic: ");
    Serial.println(topic);
    Serial.print("[MQTT] Payload: ");
    
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

/**
 * Connect to MQTT Broker
 * Establishes connection if WiFi is available
 */
void connect_mqtt() {
    if (!sysStatus.wifi_connected) {
        Serial.println("[MQTT] WiFi not connected - cannot connect to MQTT");
        return;
    }
    
    if (mqttClient.connected()) {
        return;  // Already connected
    }
    
    Serial.println("[MQTT] Connecting to broker...");
    
    if (mqttClient.connect("ESP32-LoRa-Gateway", MQTT_USER, MQTT_PASS)) {
        Serial.println("[MQTT] Connected successfully");
        sysStatus.mqtt_connected = true;
        mqttClient.setCallback(mqtt_callback);
        mqttClient.subscribe("lora/gateway/config");
        Serial.println("[MQTT] Subscribed to: lora/gateway/config");
    } else {
        Serial.print("[MQTT] Connection failed, error code: ");
        Serial.println(mqttClient.state());
        sysStatus.mqtt_connected = false;
    }
}

/**
 * Disconnect from MQTT Broker
 */
void disconnect_mqtt() {
    if (mqttClient.connected()) {
        mqttClient.disconnect();
        sysStatus.mqtt_connected = false;
        Serial.println("[MQTT] Disconnected");
    }
}

/**
 * Publish message to MQTT topic
 * @param topic MQTT topic
 * @param message Message payload
 * @return true if successful
 */
bool mqtt_publish(const char* topic, const char* message) {
    if (!mqttClient.connected()) {
        return false;
    }
    
    if (mqttClient.publish(topic, message)) {
        Serial.print("[MQTT] Published to ");
        Serial.print(topic);
        Serial.print(": ");
        Serial.println(message);
        return true;
    }
    
    return false;
}

// ==========================================
// WIFI CONNECTION FUNCTIONS
// ==========================================

/**
 * Initialize WiFi in Station Mode (client)
 * Attempts to connect to known networks
 * @return true if connected successfully
 */
bool init_wifi_sta() {
    Serial.println("[WiFi] Initializing Station Mode...");
    WiFi.onEvent(wifi_event_handler);
    WiFi.mode(WIFI_STA);
    
    // Try first network
    Serial.println("[WiFi] Attempting to connect to: " + String(WIFI_SSID_1));
    WiFi.begin(WIFI_SSID_1, WIFI_PASS_1);
    
    int timeout = WIFI_CONNECT_TIMEOUT / 100;  // 100ms units
    while (!sysStatus.wifi_connected && timeout > 0) {
        delay(100);
        timeout--;
        Serial.print(".");
    }
    
    if (sysStatus.wifi_connected) {
        Serial.println("\n[WiFi] Connected!");
        return true;
    }
    
    // Try second network
    Serial.println("[WiFi] First network failed. Trying: " + String(WIFI_SSID_2));
    WiFi.begin(WIFI_SSID_2, WIFI_PASS_2);
    
    timeout = WIFI_CONNECT_TIMEOUT / 100;
    while (!sysStatus.wifi_connected && timeout > 0) {
        delay(100);
        timeout--;
        Serial.print(".");
    }
    
    if (sysStatus.wifi_connected) {
        Serial.println("\n[WiFi] Connected!");
        return true;
    }
    
    // Try third network
    Serial.println("[WiFi] Second network failed. Trying: " + String(WIFI_SSID_3));
    WiFi.begin(WIFI_SSID_3, WIFI_PASS_3);
    
    timeout = WIFI_CONNECT_TIMEOUT / 100;
    while (!sysStatus.wifi_connected && timeout > 0) {
        delay(100);
        timeout--;
        Serial.print(".");
    }
    
    if (sysStatus.wifi_connected) {
        Serial.println("\n[WiFi] Connected!");
        return true;
    }
    
    Serial.println("\n[WiFi] All networks failed");
    return false;
}

/**
 * Initialize WiFi in Access Point Mode
 * Creates a WiFi hotspot
 * @param ssid AP SSID
 * @param password AP Password
 * @return true if successful
 */
bool init_wifi_ap(const char* ssid, const char* password) {
    Serial.println("[WiFi] Initializing Access Point Mode...");
    WiFi.mode(WIFI_AP);
    
    if (WiFi.softAP(ssid, password)) {
        sysStatus.ap_mode = true;
        sysStatus.ap_ip = WiFi.softAPIP().toString();
        Serial.print("[WiFi] AP SSID: ");
        Serial.println(ssid);
        Serial.print("[WiFi] AP IP: ");
        Serial.println(sysStatus.ap_ip);
        return true;
    }
    
    Serial.println("[WiFi] Failed to start AP mode");
    return false;
}

/**
 * Get current WiFi status as JSON
 * @return JSON string with WiFi status
 */
String get_wifi_status_json() {
    StaticJsonDocument<256> doc;
    
    doc["connected"] = sysStatus.wifi_connected;
    doc["ap_mode"] = sysStatus.ap_mode;
    doc["ssid"] = sysStatus.wifi_ssid;
    doc["ip_address"] = sysStatus.ip_address;
    doc["ap_ip"] = sysStatus.ap_ip;
    doc["mqtt_connected"] = sysStatus.mqtt_connected;
    doc["signal_strength"] = WiFi.RSSI();
    
    String json;
    serializeJson(doc, json);
    return json;
}

/**
 * Print WiFi diagnostic information
 */
void print_wifi_diagnostics() {
    Serial.println("[WiFi Diagnostics]");
    Serial.print("  Mode: ");
    Serial.println((WiFi.getMode() == WIFI_STA) ? "STA" : (WiFi.getMode() == WIFI_AP) ? "AP" : "APSTA");
    Serial.print("  STA Connected: ");
    Serial.println(sysStatus.wifi_connected ? "Yes" : "No");
    
    if (sysStatus.wifi_connected) {
        Serial.print("  SSID: ");
        Serial.println(sysStatus.wifi_ssid);
        Serial.print("  IP Address: ");
        Serial.println(sysStatus.ip_address);
        Serial.print("  Signal Strength: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    }
    
    if (sysStatus.ap_mode) {
        Serial.print("  AP SSID: LoRa-Gateway-AP");
        Serial.print("  AP IP: ");
        Serial.println(sysStatus.ap_ip);
    }
    
    Serial.print("  MQTT Connected: ");
    Serial.println(sysStatus.mqtt_connected ? "Yes" : "No");
}

// ==========================================
// WEB SERVER INITIALIZATION
// ==========================================

/**
 * Initialize Web Server (base setup)
 * Registers route handlers
 */
void init_web_server() {
    server.begin();
    Serial.println("[WebServer] Started on port 80");
}

/**
 * Stop Web Server
 */
void stop_web_server() {
    server.stop();
    Serial.println("[WebServer] Stopped");
}

#endif // WIFI_FUNCTIONS_H
