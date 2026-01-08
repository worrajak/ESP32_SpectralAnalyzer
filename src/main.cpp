#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// ===== GPIO PIN DEFINITIONS (From Detailed Schematic) =====
// PWM Control for Synchronous Buck
#define PWM_PIN_HIGH_SIDE    14            // GPIO14 - High-side FET drive (PWM)
#define PWM_PIN_LOW_SIDE     27            // GPIO27 - Low-side FET drive (PWM)

// I2C Pins for ADS1015
#define I2C_SDA              4             // GPIO4 - SDA (I2C Devices)
#define I2C_SCL              2             // GPIO2 - SCL (I2C Devices)

// UART Pins
#define UART_TX              19            // GPIO19 - TX (UART TX)
#define UART_RX              23            // GPIO23 - RX (UART RX)

// Status LED
#define LED_PIN              25            // Status LED (if available)

// Interface Ports (for future use)
#define GPIO_INTERFACE_27    27
#define GPIO_INTERFACE_14    14
#define GPIO_INTERFACE_10    10
#define GPIO_INTERFACE_9     9
#define GPIO_INTERFACE_17    17
#define GPIO_INTERFACE_16    16

// ===== PWM CONFIGURATION =====
#define PWM_FREQUENCY        20000         // 20 kHz switching frequency
#define PWM_RESOLUTION       12            // 12-bit resolution (0-4095)
#define PWM_CHANNEL_HIGH     0
#define PWM_CHANNEL_LOW      1

// ===== MPPT CONFIGURATION =====
#define SAMPLING_INTERVAL    100           // ms - ADC sampling interval
#define MPPT_INTERVAL        500           // ms - MPPT algorithm execution
#define MPPT_STEP            5             // Duty cycle step for perturbation

// ===== ADS1015 CONFIGURATION =====
// ADS1015 has 12-bit resolution, ~3mV per step at ±4.096V
// Channel assignments from schematic:
// A0 = Battery Current (Output Current)
// A1 = Battery Voltage (Output Voltage)
// A2 = Solar/PV Current (Input Current)
// A3 = Solar/PV Voltage (PV Voltage)
#define ADS1015_ADDR         0x48         // Default I2C address for ADS1015

// ===== ADC CALIBRATION FACTORS =====
// These need to be tuned based on your actual sensor circuit
// ADS1015 readings are in ADC counts (0-2047 for ±4.096V)
#define ADC_SOLAR_V_SCALE    0.0300       // V/ADC (60V range / 2047 steps, adjust for divider ratio)
#define ADC_SOLAR_I_SCALE    0.00200      // A/ADC (current sensor gain, adjust based on shunt resistor)
#define ADC_BATT_V_SCALE     0.0150       // V/ADC (30V range / 2047 steps, adjust for divider ratio)
#define ADC_BATT_I_SCALE     0.00300      // A/ADC (current sensor gain)

// ===== DATA STRUCTURES =====
typedef struct {
  float voltage;
  float current;
  float power;
} SolarData_t;

typedef struct {
  float voltage;
  float current;
  float power;
} BatteryData_t;

typedef struct {
  SolarData_t solar;
  BatteryData_t battery;
  uint16_t duty_cycle;
  float efficiency;
} SystemState_t;

// ===== GLOBAL VARIABLES =====
SystemState_t current_state = {0};
SystemState_t previous_state = {0};
uint32_t last_sampling_time = 0;
uint32_t last_mppt_time = 0;
uint16_t duty_cycle = 2048;  // Start at 50% duty cycle

// ADS1015 ADC object
Adafruit_ADS1015 ads;  // 12-bit ADC

// ===== FUNCTION DECLARATIONS =====
void initialize_pwm(void);
void initialize_i2c_adc(void);
void read_adc_values(void);
void update_pwm_duty_cycle(uint16_t duty);
void mppt_algorithm(void);
void print_system_status(void);
void print_adc_raw_values(void);

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n=== DC OPTIMIZER WITH MPPT - ESP32 + ADS1015 ===");
  Serial.println("Initializing system...");
  
  // Initialize I2C for ADS1015
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Initialize ADS1015 ADC
  initialize_i2c_adc();
  
  // Initialize PWM for buck control
  initialize_pwm();
  
  Serial.println("System initialization complete!");
  Serial.println("Starting MPPT control...\n");
  
  last_sampling_time = millis();
  last_mppt_time = millis();
  
  // Start with 50% duty cycle
  update_pwm_duty_cycle(duty_cycle);
}

// ===== MAIN LOOP =====
void loop() {
  uint32_t current_time = millis();
  
  // ADC Sampling (100 ms interval)
  if (current_time - last_sampling_time >= SAMPLING_INTERVAL) {
    read_adc_values();
    last_sampling_time = current_time;
  }
  
  // MPPT Algorithm (500 ms interval)
  if (current_time - last_mppt_time >= MPPT_INTERVAL) {
    mppt_algorithm();
    update_pwm_duty_cycle(duty_cycle);
    print_system_status();
    last_mppt_time = current_time;
  }
  
  delay(10);
}

// ===== PWM INITIALIZATION =====
void initialize_pwm(void) {
  Serial.println("Configuring PWM for synchronous buck control...");
  
  // Configure high-side FET PWM (GPIO 14)
  ledcSetup(PWM_CHANNEL_HIGH, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PWM_PIN_HIGH_SIDE, PWM_CHANNEL_HIGH);
  
  // Configure low-side FET PWM (GPIO 27 - complementary with dead-time)
  ledcSetup(PWM_CHANNEL_LOW, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PWM_PIN_LOW_SIDE, PWM_CHANNEL_LOW);
  
  // Set initial duty cycle (50%)
  ledcWrite(PWM_CHANNEL_HIGH, 2048);
  ledcWrite(PWM_CHANNEL_LOW, 2048);
  
  Serial.println("PWM initialized:");
  Serial.println("  - GPIO14: High-side FET (20 kHz)");
  Serial.println("  - GPIO27: Low-side FET (20 kHz)");
  Serial.println("  - Resolution: 12-bit\n");
}

// ===== ADS1015 I2C ADC INITIALIZATION =====
void initialize_i2c_adc(void) {
  Serial.println("Initializing ADS1015 12-bit I2C ADC...");
  
  // Check if ADS1015 is found on I2C bus
  if (!ads.begin(ADS1015_ADDR)) {
    Serial.println("ERROR: ADS1015 not found at address 0x48!");
    Serial.println("Check I2C connections (GPIO2=SCL, GPIO4=SDA)");
    while (1);  // Halt if ADC not found
  }
  
  // Configure ADS1015 for differential/single-ended measurements
  // Using ±4.096V range for better resolution
  ads.setGain(GAIN_TWO);  // ±4.096V range
  
  Serial.println("ADS1015 initialized successfully:");
  Serial.println("  - I2C Address: 0x48");
  Serial.println("  - Voltage Range: ±4.096V");
  Serial.println("  - Resolution: 12-bit (2047 steps max)");
  Serial.println("  - A0: Battery Current (Output)");
  Serial.println("  - A1: Battery Voltage (Output)");
  Serial.println("  - A2: Solar/PV Current (Input)");
  Serial.println("  - A3: Solar/PV Voltage (Input)\n");
}

// ===== READ ADC VALUES FROM ADS1015 =====
void read_adc_values(void) {
  // Save previous state
  previous_state = current_state;
  
  // Read analog values from ADS1015 (12-bit, range 0-2047)
  int16_t adc_batt_i = ads.readADC_SingleEnded(0);  // A0 - Battery Current (Output)
  int16_t adc_batt_v = ads.readADC_SingleEnded(1);  // A1 - Battery Voltage (Output)
  int16_t adc_solar_i = ads.readADC_SingleEnded(2); // A2 - Solar/PV Current (Input)
  int16_t adc_solar_v = ads.readADC_SingleEnded(3); // A3 - Solar/PV Voltage (Input)
  
  // Convert to real values with calibration
  // Ensure positive values only
  current_state.solar.voltage = max(0.0, adc_solar_v * ADC_SOLAR_V_SCALE);
  current_state.solar.current = max(0.0, adc_solar_i * ADC_SOLAR_I_SCALE);
  current_state.solar.power = current_state.solar.voltage * current_state.solar.current;
  
  current_state.battery.voltage = max(0.0, adc_batt_v * ADC_BATT_V_SCALE);
  current_state.battery.current = max(0.0, adc_batt_i * ADC_BATT_I_SCALE);
  current_state.battery.power = current_state.battery.voltage * current_state.battery.current;
  
  // Calculate efficiency
  if (current_state.solar.power > 0.1) {  // Avoid division by zero
    current_state.efficiency = (current_state.battery.power / current_state.solar.power) * 100.0;
  } else {
    current_state.efficiency = 0;
  }
  
  current_state.duty_cycle = duty_cycle;
  
  // Debug: Print raw ADC values occasionally (every 5 seconds)
  static uint32_t last_debug = 0;
  if (millis() - last_debug > 5000) {
    print_adc_raw_values();
    last_debug = millis();
  }
}

// ===== PRINT RAW ADC VALUES =====
void print_adc_raw_values(void) {
  int16_t adc0 = ads.readADC_SingleEnded(0);  // Battery Current
  int16_t adc1 = ads.readADC_SingleEnded(1);  // Battery Voltage
  int16_t adc2 = ads.readADC_SingleEnded(2);  // Solar Current
  int16_t adc3 = ads.readADC_SingleEnded(3);  // Solar Voltage
  
  Serial.println("--- RAW ADC VALUES (Debug) ---");
  Serial.print("A0 (Batt I): "); Serial.print(adc0);
  Serial.print(" | A1 (Batt V): "); Serial.print(adc1);
  Serial.print(" | A2 (Solar I): "); Serial.print(adc2);
  Serial.print(" | A3 (Solar V): "); Serial.println(adc3);
}

// ===== UPDATE PWM DUTY CYCLE =====
void update_pwm_duty_cycle(uint16_t new_duty) {
  // Constrain duty cycle between 5% and 95%
  uint16_t constrained_duty = constrain(new_duty, 204, 3891);
  
  // High-side PWM
  ledcWrite(PWM_CHANNEL_HIGH, constrained_duty);
  
  // Low-side PWM (complementary for synchronous rectification)
  // Dead-time is inherently handled by LED PWM controller
  ledcWrite(PWM_CHANNEL_LOW, 4095 - constrained_duty);
  
  duty_cycle = constrained_duty;
}

// ===== MPPT ALGORITHM: Perturb & Observe =====
void mppt_algorithm(void) {
  static uint8_t startup_count = 0;
  
  // Startup phase: ramp up slowly
  if (current_state.solar.voltage < 5.0 || startup_count < 5) {
    duty_cycle += 50;
    startup_count++;
    return;
  }
  
  // Check if solar panel is generating power
  if (current_state.solar.power < 0.1) {
    return;  // No sun, do nothing
  }
  
  float power_delta = current_state.solar.power - previous_state.solar.power;
  float voltage_delta = current_state.solar.voltage - previous_state.solar.voltage;
  
  // Perturb & Observe Algorithm
  if (power_delta > 0.1) {
    // Power increased, continue in same direction
    if (voltage_delta > 0) {
      duty_cycle += MPPT_STEP;  // Increase duty cycle
    } else {
      duty_cycle -= MPPT_STEP;  // Decrease duty cycle
    }
  } else if (power_delta < -0.1) {
    // Power decreased, reverse direction
    if (voltage_delta > 0) {
      duty_cycle -= MPPT_STEP;  // Decrease duty cycle
    } else {
      duty_cycle += MPPT_STEP;  // Increase duty cycle
    }
  }
  
  // Constrain duty cycle
  duty_cycle = constrain(duty_cycle, 204, 3891);
}

// ===== PRINT SYSTEM STATUS =====
void print_system_status(void) {
  Serial.println("================================================");
  Serial.print("Solar Panel: ");
  Serial.print(current_state.solar.voltage, 2);
  Serial.print("V | ");
  Serial.print(current_state.solar.current, 3);
  Serial.print("A | ");
  Serial.print(current_state.solar.power, 2);
  Serial.println("W");
  
  Serial.print("Battery:     ");
  Serial.print(current_state.battery.voltage, 2);
  Serial.print("V | ");
  Serial.print(current_state.battery.current, 3);
  Serial.print("A | ");
  Serial.print(current_state.battery.power, 2);
  Serial.println("W");
  
  Serial.print("Duty Cycle:  ");
  Serial.print((duty_cycle * 100.0 / 4095), 1);
  Serial.print("% | Efficiency: ");
  Serial.print(current_state.efficiency, 1);
  Serial.println("%");
  Serial.println("================================================\n");
}