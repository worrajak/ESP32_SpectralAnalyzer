/**
 * AD7343 12-bit ADC Sensor Functions
 * Reads analog sensor data via SPI interface
 */

#ifndef AD7343_SENSOR_H
#define AD7343_SENSOR_H

#include <Arduino.h>
#include <SPI.h>
#include "lora_config.h"

// ==========================================
// AD7343 SENSOR VARIABLES
// ==========================================

// Store last sensor readings
float ad7343_ch0_voltage = 0.0;  // Channel 0 voltage
float ad7343_ch1_voltage = 0.0;  // Channel 1 voltage
uint16_t ad7343_ch0_raw = 0;
uint16_t ad7343_ch1_raw = 0;

// ==========================================
// AD7343 INITIALIZATION
// ==========================================

/**
 * Initialize AD7343 sensor
 * Configure SPI and chip select pin
 */
void init_ad7343() {
  pinMode(AD7343_CS, OUTPUT);
  digitalWrite(AD7343_CS, HIGH);  // Chip select inactive
  
  Serial.println("[AD7343] Initialized on GPIO" + String(AD7343_CS));
  Serial.println("  CLK: GPIO" + String(AD7343_CLK));
  Serial.println("  DIN: GPIO" + String(AD7343_DIN));
  Serial.println("  DOUT: GPIO" + String(AD7343_DOUT));
}

// ==========================================
// AD7343 DATA READING
// ==========================================

/**
 * Read both channels from AD7343
 * Performs SPI communication to read analog values
 */
void read_ad7343() {
  // Pull chip select low to start communication
  digitalWrite(AD7343_CS, LOW);
  delayMicroseconds(10);
  
  // Read 16-bit value (2 bytes) - MSB first
  // First byte contains upper 8 bits + 4 control bits
  // Second byte contains lower 4 bits + 4 zeros
  
  uint8_t byte1 = SPI.transfer(0x00);  // Request channel 0
  uint8_t byte2 = SPI.transfer(0x00);
  
  // Combine bytes to get 12-bit value
  ad7343_ch0_raw = ((uint16_t)byte1 << 4) | ((uint16_t)byte2 >> 4);
  
  // Read channel 1
  byte1 = SPI.transfer(0x00);  // Request channel 1
  byte2 = SPI.transfer(0x00);
  
  ad7343_ch1_raw = ((uint16_t)byte1 << 4) | ((uint16_t)byte2 >> 4);
  
  // Pull chip select high to end communication
  digitalWrite(AD7343_CS, HIGH);
  delayMicroseconds(10);
  
  // Convert to voltage (assuming 3.3V reference, 12-bit resolution)
  // ADC value 0-4095 maps to 0-3.3V
  ad7343_ch0_voltage = (ad7343_ch0_raw / 4095.0) * 3.3;
  ad7343_ch1_voltage = (ad7343_ch1_raw / 4095.0) * 3.3;
}

/**
 * Get channel 0 voltage
 * @return Voltage in volts (0.0 - 3.3V)
 */
float get_ad7343_ch0() {
  return ad7343_ch0_voltage;
}

/**
 * Get channel 1 voltage
 * @return Voltage in volts (0.0 - 3.3V)
 */
float get_ad7343_ch1() {
  return ad7343_ch1_voltage;
}

/**
 * Print AD7343 sensor data to serial
 */
void print_ad7343_data() {
  Serial.print("[AD7343] CH0: ");
  Serial.print(ad7343_ch0_voltage, 3);
  Serial.print("V (");
  Serial.print(ad7343_ch0_raw);
  Serial.print(") | CH1: ");
  Serial.print(ad7343_ch1_voltage, 3);
  Serial.print("V (");
  Serial.print(ad7343_ch1_raw);
  Serial.println(")");
}

#endif // AD7343_SENSOR_H
