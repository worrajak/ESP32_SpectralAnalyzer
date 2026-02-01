#include <Arduino.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>
#include <Adafruit_SSD1306.h>
#include "lora_config.h"
#include "lora_functions.h"
#include "oled_display.h"
#include "hardware_init.h"
#include "as7343_sensor.h"
#include "spectral_analysis.h"

// ===== GLOBAL OBJECT DEFINITIONS =====
RH_RF95 rf95(LORA_SS, LORA_DIO0);  // Chip select and interrupt pins
RHReliableDatagram manager(rf95, GATEWAY_ADDRESS);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ===== CONFIGURATION =====
#define UPDATE_INTERVAL      1000  // Update display every 1 second
#define LORA_CHECK_INTERVAL  100   // Check LoRa every 100ms
#define SENSOR_READ_INTERVAL 500   // Read sensor every 500ms

// ===== GLOBAL VARIABLES =====
uint32_t last_update_time = 0;
uint32_t last_lora_check = 0;
uint32_t last_sensor_read = 0;
uint8_t msg_count = 0;
int16_t last_rssi = 0;
char last_message[64] = "";
uint8_t last_msg_len = 0;
uint32_t last_rx_time = 0;

// ===== FUNCTION DECLARATIONS =====
void display_status(void);
void check_lora_rx(void);

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n\n============================================");
  Serial.println("   ESP32 LoRa + OLED + AS7343 SPECTRAL   ");
  Serial.println("   Startup Sequence [v2.0]");
  Serial.println("============================================\n");
  Serial.println("Initializing hardware...");
  
  // Initialize hardware buses and GPIO
  init_i2c();
  init_spi();
  init_gpio();
  init_lora_gpio();
  
  // Initialize OLED Display
  if (!init_oled()) {
    Serial.println("[ERROR] OLED initialization failed!");
    while (1);
  }
  
  // Initialize LoRa Radio
  if (!rf95.init()) {
    Serial.println("[ERROR] LoRa initialization failed!");
    while (1);
  }
  configure_lora();
  
  // Initialize AS7343 Spectral Sensor
  Serial.println("\n[SETUP] About to init AS7343...");
  Serial.flush();
  init_as7343();
  Serial.println("[SETUP] AS7343 init complete.");
  Serial.flush();
  
  Serial.println("System ready!");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("LoRa+OLED Ready");
  display.display();
  
  last_update_time = millis();
  last_lora_check = millis();
}

// ===== MAIN LOOP =====
void loop() {
  uint32_t current_time = millis();
  
  // Check for incoming LoRa messages (PAUSED)
  // if (current_time - last_lora_check >= LORA_CHECK_INTERVAL) {
  //   check_lora_rx();
  //   last_lora_check = current_time;
  // }
  
  // Read sensor data and calculate spectral indices
  if (current_time - last_sensor_read >= SENSOR_READ_INTERVAL) {
    read_as7343();
    apply_spectral_calibration();           // Apply dark/white balance
    calculate_all_indices();                // Calculate vegetation indices
    calculate_health_levels();              // Calculate 0-5 health levels
    print_as7343_data();
    print_vegetation_indices();
    print_health_description();             // Print health levels 0-5
    last_sensor_read = current_time;
  }
  
  // Update display
  if (current_time - last_update_time >= UPDATE_INTERVAL) {
    display_status();
    last_update_time = current_time;
  }
  
  delay(10);
}

// ===== CHECK FOR INCOMING LORA MESSAGES (PAUSED) =====
// Disabled to focus on spectral analysis
/*
void check_lora_rx(void) {
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len)) {
      last_rssi = rf95.lastRssi();
      last_rx_time = millis();
      
      // Check if message has CRC (at least 3 bytes: encrypted data + 2 bytes CRC)
      if (len >= 3) {
        // Extract CRC from last 2 bytes
        uint16_t rxCrc = ((uint16_t)buf[len - 2] << 8) | buf[len - 1];
        uint8_t encLen = len - 2;  // Encrypted data length (without CRC)
        
        // Calculate CRC of encrypted data
        uint16_t calcCrc = calculateCRC16(buf, encLen);
        
        Serial.print("[LoRa RX] Len: ");
        Serial.print(len);
        Serial.print(" | RSSI: ");
        Serial.print(last_rssi);
        Serial.print(" | CRC RX: 0x");
        Serial.print(rxCrc, HEX);
        Serial.print(" CALC: 0x");
        Serial.println(calcCrc, HEX);
        
        if (rxCrc == calcCrc) {
          // CRC valid - decrypt
          String decrypted = xor_decrypt_str(buf, encLen);
          strncpy(last_message, decrypted.c_str(), sizeof(last_message) - 1);
          last_message[sizeof(last_message) - 1] = '\0';
          last_msg_len = encLen;
          
          Serial.print("[DECRYPTED] ");
          Serial.println(last_message);
        } else {
          // CRC failed - show as hex
          snprintf(last_message, sizeof(last_message), "CRC ERR");
          last_msg_len = encLen;
          Serial.println("[CRC FAILED]");
        }
      } else {
        // Short message - treat as plain text
        memset(last_message, 0, sizeof(last_message));
        for (uint8_t i = 0; i < len && i < sizeof(last_message) - 1; i++) {
          if (buf[i] >= 32 && buf[i] <= 126) {
            last_message[i] = (char)buf[i];
          } else {
            last_message[i] = '.';
          }
        }
        last_msg_len = len;
        Serial.print("[Plain text] ");
        Serial.println(last_message);
      }
      
      msg_count++;
      
    } else {
      Serial.println("[LoRa] RX Failed");
    }
  }
}
*/

// ===== DISPLAY STATUS =====
void display_status(void) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // ===== HEADER =====
  display.setCursor(0, 0);
  display.println("=== SPECTRAL ANALYSIS ===");
  
  // ===== ROW 1: NDVI + Clear =====
  display.setCursor(0, 10);
  display.print("NDVI:");
  display.print(spectral_indices[IDX_NDVI], 2);
  display.print("  ");
  display.print("Clear:");
  display.println((int)spectral_ch[CH_CLEAR]);
  
  // ===== ROW 2: Chlorophyll + Anthocyanin =====
  display.setCursor(0, 19);
  display.print("Chlor:");
  display.print(spectral_indices[IDX_CHLOROPHYLL], 2);
  display.print(" ");
  display.print("Anth:");
  display.println(spectral_indices[IDX_ANTHOCYANIN], 2);
  
  // ===== ROW 3: Water + Red:FarRed =====
  display.setCursor(0, 28);
  display.print("Water:");
  display.print(spectral_indices[IDX_WATER_STRESS], 2);
  display.print(" ");
  display.print("R:FR:");
  display.println(spectral_indices[IDX_RED_FAR_RED], 2);
  
  // ===== ROW 4: Photosyn + Carotenoid =====
  display.setCursor(0, 37);
  display.print("Photo:");
  display.print(spectral_indices[IDX_PHOTOSYN], 2);
  display.print(" ");
  display.print("Car:");
  display.println(spectral_indices[IDX_CAROTENOID], 2);
  
  // ===== SEPARATOR =====
  display.drawLine(0, 46, 128, 46, SSD1306_WHITE);
  
  // ===== ROW 5: HEALTH LEVELS =====
  display.setCursor(0, 50);
  display.print("Health:");
  display.print("V:");
  display.print(health_levels.vigor);
  display.print(" C:");
  display.print(health_levels.chlorophyll);
  display.print(" S:");
  display.print(health_levels.stress);
  display.print(" W:");
  display.println(health_levels.water);
  
  // ===== ROW 6: STATUS =====
  display.setCursor(0, 59);
  display.print("Status: ");
  if (as7343_ready) {
    display.println("OK");
  } else {
    display.println("NO SENSOR");
  }
  
  display.display();
}