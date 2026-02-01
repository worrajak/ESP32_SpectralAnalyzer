/**
 * Hardware Initialization Functions
 * OLED, SPI, I2C, GPIO setup for LoRa + OLED
 */

#ifndef HARDWARE_INIT_H
#define HARDWARE_INIT_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "lora_config.h"

// ==========================================
// GLOBAL HARDWARE OBJECTS
// ==========================================

extern Adafruit_SSD1306 display;

// ==========================================
// HARDWARE INITIALIZATION FUNCTIONS
// ==========================================

/**
 * Initialize I2C Bus for OLED and other I2C devices
 * @return true if successful, false otherwise
 */
bool init_i2c() {
    Wire.begin(OLED_SDA, OLED_SCL);
    Serial.println("[I2C] Bus initialized on GPIO21(SDA), GPIO22(SCL)");
    return true;
}

/**
 * Initialize SPI Bus for LoRa Radio Module
 */
void init_spi() {
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
    Serial.println("[SPI] Bus initialized");
    Serial.print("  SCK: GPIO"); Serial.println(LORA_SCK);
    Serial.print("  MISO: GPIO"); Serial.println(LORA_MISO);
    Serial.print("  MOSI: GPIO"); Serial.println(LORA_MOSI);
    Serial.print("  SS: GPIO"); Serial.println(LORA_SS);
}

/**
 * Initialize OLED Display (SSD1306 via I2C)
 * @return true if display initialized successfully
 */
bool init_oled() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
        Serial.println("[OLED] FAILED to initialize!");
        return false;
    }
    
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("OLED OK");
    display.display();
    
    Serial.println("[OLED] Initialized (128x64, I2C Address: 0x3C)");
    return true;
}

/**
 * Initialize GPIO Pins
 */
void init_gpio() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
    
    Serial.println("[GPIO] Pins initialized");
    Serial.print("  LED: GPIO"); Serial.println(LED_PIN);
    Serial.print("  CONFIG_BUTTON: GPIO"); Serial.println(CONFIG_BUTTON_PIN);
}

/**
 * Initialize LoRa Radio Reset Pin
 */
void init_lora_gpio() {
    pinMode(LORA_RST, OUTPUT);
    digitalWrite(LORA_RST, HIGH);
    delay(100);
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(100);
    
    Serial.println("[LoRa GPIO] Reset sequence completed");
}

/**
 * Display boot message on OLED
 */
void display_boot_message(const char* message) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("=== BOOTING ===");
    display.println(message);
    display.display();
}

#endif // HARDWARE_INIT_H
