/**
 * AS7343 11-Channel Spectral Sensor Functions
 * Measures light intensity across visible and near-infrared wavelengths
 * I2C interface
 */

#ifndef AS7343_SENSOR_H
#define AS7343_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "lora_config.h"

// ==========================================
// AS7343 REGISTERS
// ==========================================

#define AS7343_CTRL 0x80
#define AS7343_ENABLE 0x80
#define AS7343_ATIME 0x81
#define AS7343_WTIME 0x83
#define AS7343_GAIN 0x8A        // Gain control
#define AS7343_FD_TIME 0x8E     // Flick detection time
#define AS7343_STATUS 0x93
#define AS7343_CONFIG 0x8D      // Channel configuration
#define AS7343_DATA_START 0x95  // Start of 12 channel data registers
#define AS7343_BANK 0xAC        // Register bank selection (to access red/NIR)

// ==========================================
// AS7343 SENSOR VARIABLES
// ==========================================

// Store spectral channel readings (11 channels + clear)
uint16_t as7343_ch[12] = {0};  // Channels: 415nm, 445nm, 480nm, 510nm, 545nm, 580nm, 610nm, 645nm, 680nm, 705nm, 940nm, Clear

// Channel names
const char* as7343_names[] = {"415", "445", "480", "510", "545", "580", "610", "645", "680", "705", "940", "CLR"};

bool as7343_ready = false;

// ==========================================
// AS7343 INITIALIZATION
// ==========================================

/**
 * Scan I2C bus for devices
 */
void scan_i2c_bus() {
  Serial.println("\n[I2C] ===== BUS SCAN START =====");
  int found = 0;
  
  for (int addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    int error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("[I2C] FOUND at 0x");
      Serial.print(addr, HEX);
      
      // Identify known devices
      if (addr == 0x3C) Serial.print(" (OLED SSD1306)");
      if (addr == 0x77) Serial.print(" (AS7343 - EXPECTED)");
      
      Serial.println();
      found++;
    }
  }
  
  Serial.print("[I2C] Total devices found: ");
  Serial.println(found);
  Serial.println("[I2C] ===== BUS SCAN END =====\n");
}

void init_as7343() {
  Serial.println("\n[AS7343] ===== INITIALIZATION START =====");
  Serial.flush();
  
  // First, verify I2C bus is working by checking OLED
  Serial.print("[I2C] Checking if OLED is present at 0x3C... ");
  Serial.flush();
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0) {
    Serial.println("YES (I2C bus working)");
  } else {
    Serial.println("NO (I2C bus may not be working)");
  }
  Serial.flush();
  
  Serial.println("[AS7343] Attempting to initialize spectral sensor...");
  Serial.flush();
  delay(100);
  
  // Try multiple times at 0x39
  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.print("[AS7343] Attempt ");
    Serial.print(attempt);
    Serial.print("/3: Querying 0x39... ");
    Serial.flush();
    
    Wire.beginTransmission(AS7343_I2C_ADDRESS);
    int error = Wire.endTransmission();
    
    if (error == 0) {
      as7343_ready = true;
      Serial.println("SUCCESS!");
      Serial.println("[AS7343] Spectral Sensor Initialized");
      Serial.println("  I2C Address: 0x39");
      Serial.println("  Channels: 415, 445, 480, 510, 545, 580, 610, 645, 680, 705, 940nm + Clear");
      Serial.flush();
      
      // Configure AS7343 for measurement
      Serial.println("[AS7343] Configuring measurement mode...");
      Serial.flush();
      
      // Step 1: Power on and enable AEN (ADC Enable)
      Wire.beginTransmission(AS7343_I2C_ADDRESS);
      Wire.write(AS7343_ENABLE);
      Wire.write(0x01);  // POWER_ON (bit 0)
      Wire.endTransmission();
      delay(10);
      
      // Step 2: Set integration time (ATIME) - very short to prevent saturation
      // 0x00 = 2.78ms, 0x10 = ~11ms, 0xFF = 711ms
      // Using 0x10 (~11ms) to avoid saturation
      Wire.beginTransmission(AS7343_I2C_ADDRESS);
      Wire.write(AS7343_ATIME);
      Wire.write(0x10);  // ~11ms integration time
      Wire.endTransmission();
      delay(10);
      
      // Step 3: Set GAIN for better sensitivity but avoid saturation
      // 0 = 0.5x, 1 = 1x, 2 = 2x, 3 = 4x, 4 = 8x, 5 = 16x, 6 = 32x, 7 = 64x, 8 = 128x, 9 = 256x
      // Using 1x (gain = 1) for minimal amplification with fast 11ms sampling
      Wire.beginTransmission(AS7343_I2C_ADDRESS);
      Wire.write(AS7343_GAIN);
      Wire.write(0x01);  // 1x gain - minimal to prevent saturation
      Wire.endTransmission();
      delay(10);
      
      // Step 4: Configure channel bank to measure all wavelengths
      Wire.beginTransmission(AS7343_I2C_ADDRESS);
      Wire.write(AS7343_BANK);
      Wire.write(0x00);  // Bank 0 for normal measurement
      Wire.endTransmission();
      delay(10);
      
      // Step 5: Enable measurement mode (AEN)
      Wire.beginTransmission(AS7343_I2C_ADDRESS);
      Wire.write(AS7343_ENABLE);
      Wire.write(0x03);  // POWER_ON + AEN (measurement enable)
      Wire.endTransmission();
      delay(100);  // Wait for first measurement
      
      Serial.println("[AS7343] Fast-sampling mode enabled (1x gain, 11ms integration)");
      Serial.println("[AS7343] ===== INITIALIZATION SUCCESS =====\n");
      Serial.flush();
      return;
    } else {
      Serial.print("FAILED (error=");
      Serial.print(error);
      Serial.println(")");
      Serial.flush();
      delay(200);
    }
  }
  
  as7343_ready = false;
  Serial.println("\n[AS7343] NOT RESPONDING at 0x39 - scanning I2C bus...");
  Serial.flush();
  scan_i2c_bus();
  Serial.println("[AS7343] ===== INITIALIZATION FAILED =====\n");
  Serial.flush();
}

// ==========================================
// AS7343 DATA READING
// ==========================================

/**
 * Read all 12 channels from AS7343 with bank switching for red/NIR
 * Bank 0: 415, 445, 480, 510, 545, 580, 610, 645, 680, 705nm
 * Bank 1: 610, 645, 680, 705, 910, 940nm (red and NIR)
 */
void read_as7343() {
  if (!as7343_ready) return;
  
  // Always start with bank 0 - read visible wavelengths (415-705nm) + clear
  Wire.beginTransmission(AS7343_I2C_ADDRESS);
  Wire.write(AS7343_BANK);
  Wire.write(0x00);
  Wire.endTransmission();
  delayMicroseconds(100);
  
  Wire.beginTransmission(AS7343_I2C_ADDRESS);
  Wire.write(AS7343_DATA_START);
  Wire.endTransmission();
  
  // Read all 24 bytes from bank 0 (12 channels)
  Wire.requestFrom(AS7343_I2C_ADDRESS, 24);
  for (int i = 0; i < 12; i++) {
    if (Wire.available() >= 2) {
      uint8_t low = Wire.read();
      uint8_t high = Wire.read();
      as7343_ch[i] = (high << 8) | low;
    }
  }
  
  // Switch to bank 1 for better red/NIR data (some boards swap banks)
  Wire.beginTransmission(AS7343_I2C_ADDRESS);
  Wire.write(AS7343_BANK);
  Wire.write(0x01);
  Wire.endTransmission();
  delayMicroseconds(100);
  
  Wire.beginTransmission(AS7343_I2C_ADDRESS);
  Wire.write(AS7343_DATA_START);
  Wire.endTransmission();
  
  // Try to read alternative red/NIR from bank 1
  Wire.requestFrom(AS7343_I2C_ADDRESS, 24);
  uint16_t bank1_ch[12] = {0};
  for (int i = 0; i < 12; i++) {
    if (Wire.available() >= 2) {
      uint8_t low = Wire.read();
      uint8_t high = Wire.read();
      bank1_ch[i] = (high << 8) | low;
    }
  }
  
  // Use bank 1 data for channels 6-11 if they have better values (red/NIR)
  for (int i = 6; i < 12; i++) {
    if (bank1_ch[i] > as7343_ch[i]) {
      as7343_ch[i] = bank1_ch[i];
    }
  }
  
  // Switch back to bank 0
  Wire.beginTransmission(AS7343_I2C_ADDRESS);
  Wire.write(AS7343_BANK);
  Wire.write(0x00);
  Wire.endTransmission();
}

/**
 * Get channel value by index (0-11)
 */
uint16_t get_as7343_channel(uint8_t ch) {
  if (ch < 12) return as7343_ch[ch];
  return 0;
}

/**
 * Print AS7343 sensor data to serial
 */
void print_as7343_data() {
  if (!as7343_ready) {
    Serial.println("[AS7343] Sensor not ready");
    return;
  }
  
  // Check for saturation
  bool saturated = (as7343_ch[5] >= 65535);  // Channel 5 = 580nm
  
  Serial.print("[AS7343] ");
  for (int i = 0; i < 12; i++) {
    Serial.print(as7343_names[i]);
    Serial.print(":");
    // Show saturated channels with special marker
    if (i == 5 && saturated) {
      Serial.print("SAT");
    } else {
      Serial.print(as7343_ch[i]);
    }
    if (i < 11) Serial.print(" ");
  }
  
  if (saturated) {
    Serial.print(" [⚠ 580nm saturated - reduce gain or integration time]");
  }
  Serial.println();
}

/**
 * Get dominant wavelength region (simple classification)
 * Returns index of highest channel (0-11)
 */
uint8_t get_as7343_dominant() {
  uint16_t maxVal = 0;
  uint8_t maxIdx = 0;
  
  for (int i = 0; i < 11; i++) {  // Skip CLEAR channel
    if (as7343_ch[i] > maxVal) {
      maxVal = as7343_ch[i];
      maxIdx = i;
    }
  }
  
  return maxIdx;
}

#endif // AS7343_SENSOR_H
