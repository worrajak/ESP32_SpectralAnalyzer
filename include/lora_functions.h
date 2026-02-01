/**
 * LoRa Radio Functions (RadioHead Library)
 * Radio initialization, encryption, and data handling
 */

#ifndef LORA_FUNCTIONS_H
#define LORA_FUNCTIONS_H

#include <Arduino.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>
#include "lora_config.h"

// ==========================================
// GLOBAL RADIO OBJECTS
// ==========================================

extern RH_RF95 rf95;
extern RHReliableDatagram manager;

// ==========================================
// LORA RADIO INITIALIZATION & CONFIGURATION
// ==========================================

/**
 * Configure LoRa Radio Parameters
 * Sets frequency, spreading factor, bandwidth, etc.
 */
void configure_lora() {
    rf95.setFrequency(LORA_FREQ / 1000000.0);
    rf95.setSpreadingFactor(LORA_SF);
    rf95.setSignalBandwidth(LORA_BW);
    rf95.setCodingRate4(LORA_CR - 4);
    rf95.setTxPower(LORA_TX_POWER, false);
    
    Serial.println("[LoRa] Configuration Complete:");
    Serial.print("  Frequency: ");
    Serial.print(LORA_FREQ / 1E6);
    Serial.println(" MHz");
    Serial.print("  Spreading Factor: ");
    Serial.println(LORA_SF);
    Serial.print("  Bandwidth: ");
    Serial.print(LORA_BW / 1000);
    Serial.println(" kHz");
    Serial.print("  Coding Rate: ");
    Serial.print("4/");
    Serial.println(LORA_CR);
    Serial.print("  TX Power: ");
    Serial.print(LORA_TX_POWER);
    Serial.println(" dBm");
}

/**
 * Initialize LoRa Radio Manager
 * @return true if initialization successful
 */
bool init_lora_radio() {
    if (!manager.init()) {
        Serial.println("[LoRa] INITIALIZATION FAILED!");
        return false;
    }
    
    configure_lora();
    
    Serial.println("[LoRa] RadioHead Manager Initialized");
    Serial.print("  Gateway Address: ");
    Serial.println(GATEWAY_ADDRESS);
    
    return true;
}

// ==========================================
// ENCRYPTION & DECRYPTION FUNCTIONS
// ==========================================

/**
 * XOR Encrypt string data
 * @param plaintext Input string to encrypt
 * @param encrypted Output buffer for encrypted data
 * @param encLen Output parameter for encrypted length
 */
void xor_encrypt_str(const String& plaintext, uint8_t* encrypted, int& encLen) {
    int keyLen = strlen(FIXED_CRYPTO_KEY);
    encLen = plaintext.length();
    
    for (int i = 0; i < encLen; i++) {
        encrypted[i] = (uint8_t)plaintext[i] ^ (uint8_t)FIXED_CRYPTO_KEY[i % keyLen];
    }
    
    Serial.print("[CRYPTO] Encrypted ");
    Serial.print(encLen);
    Serial.println(" bytes");
}

/**
 * XOR Decrypt string data (XOR cipher is symmetric)
 * @param cipher Encrypted data buffer
 * @param len Length of encrypted data
 * @return Decrypted string
 */
String xor_decrypt_str(const uint8_t* cipher, int len) {
    int keyLen = strlen(FIXED_CRYPTO_KEY);
    String res = "";
    
    for (int i = 0; i < len; i++) {
        res += (char)(cipher[i] ^ FIXED_CRYPTO_KEY[i % keyLen]);
    }
    
    res.trim();
    return res;
}

// ==========================================
// CRC16 CALCULATION
// ==========================================

/**
 * Calculate CRC16 checksum (CRC-16-MODBUS algorithm)
 * Used for data integrity verification
 * @param data Input data buffer
 * @param length Length of data
 * @return Calculated CRC16 value
 */
uint16_t calculateCRC16(uint8_t* data, uint8_t length) {
    uint16_t crc = 0xFFFF;
    
    for (int i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];
        
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

/**
 * Verify CRC16 checksum
 * @param data Data buffer (including CRC bytes at end)
 * @param dataLen Length of data WITHOUT CRC bytes
 * @param receivedCRC CRC value received
 * @return true if CRC matches, false otherwise
 */
bool verify_crc16(const uint8_t* data, uint8_t dataLen, uint16_t receivedCRC) {
    uint16_t calculatedCRC = calculateCRC16((uint8_t*)data, dataLen);
    return (calculatedCRC == receivedCRC);
}

// ==========================================
// MESSAGE DEDUPLICATION
// ==========================================

/**
 * Generate hash for message deduplication
 * @param sender Sender node ID
 * @param seq Sequence number
 * @return Hash value combining sender and sequence
 */
uint32_t get_hash(uint8_t sender, uint8_t seq) {
    return ((uint32_t)sender << 16) | seq;
}

// ==========================================
// RADIO STATUS & DIAGNOSTICS
// ==========================================

/**
 * Get last received RSSI (Received Signal Strength Indicator)
 * @return RSSI value in dBm
 */
int get_last_rssi() {
    return rf95.lastRssi();
}

/**
 * Get last received SNR (Signal-to-Noise Ratio)
 * @return SNR value in dB
 */
int get_last_snr() {
    return rf95.lastSNR();
}

/**
 * Print radio diagnostic information
 */
void print_radio_diagnostics() {
    Serial.println("[LoRa Diagnostics]");
    Serial.print("  Spreading Factor: ");
    Serial.println(LORA_SF);
    Serial.print("  Bandwidth: ");
    Serial.print(LORA_BW / 1000);
    Serial.println(" kHz");
    Serial.print("  Last RSSI: ");
    Serial.print(rf95.lastRssi());
    Serial.println(" dBm");
}

#endif // LORA_FUNCTIONS_H
