/**
 * SPECTRAL ANALYSIS - Plant Phenotyping Module
 * AS7343 14-channel spectral sensor (410-940nm + Clear + Flicker)
 * 
 * Calculations:
 * - Chlorophyll Index (CI) - leaf greenness
 * - Anthocyanin Index (ARI) - stress pigment
 * - Normalized Difference Vegetation Index (NDVI) - plant vigor
 * - Water Stress Index (WSI) - NIR water absorption
 * - Red:Far-Red Ratio (R:FR) - plant morphology
 * - Flicker detection - AC light compensation (50/60Hz)
 * - Dark/White balance calibration
 */

#ifndef SPECTRAL_ANALYSIS_H
#define SPECTRAL_ANALYSIS_H

#include <Arduino.h>

// ==========================================
// CHANNEL MAPPING - AS7343 to Plant Indices
// ==========================================
// AS7343 has: 415, 445, 480, 510, 545, 580, 610, 645, 680, 705, 940nm + CLR
// Mapping to plant phenotyping wavelengths:
enum SpectralChannel {
  CH_VIOLET_410   = 0,   // 415nm (NDVI, chlorophyll)
  CH_BLUE_440     = 1,   // 445nm (anthocyanin)
  CH_BLUE_470     = 2,   // 480nm (chlorophyll a)
  CH_GREEN_510    = 3,   // 510nm (greenness)
  CH_GREEN_550    = 4,   // 545nm (vegetation)
  CH_YELLOW_590   = 5,   // 580nm (carotenoid, xanthophyll)
  CH_RED_630      = 6,   // 610nm (photosynthesis, water stress)
  CH_RED_680      = 7,   // 645nm (chlorophyll absorption peak)
  CH_NIR_RED_700  = 8,   // 680nm (red edge - NDVI critical)
  CH_NIR_730      = 9,   // 705nm (NIR shoulder - water stress)
  CH_NIR_810      = 10,  // 940nm (far NIR - water, morphology) - REUSED FOR 810
  CH_NIR_860      = 11,  // 940nm (far NIR - water, morphology) - REUSED FOR 860
  CH_CLEAR        = 12,  // Clear channel (total light)
  CH_FLICKER      = 13   // Flicker detection (AC ripple)
};

// ==========================================
// GLOBAL SPECTRAL DATA
// ==========================================
extern uint16_t as7343_ch[];         // Reference AS7343 channel data from as7343_sensor.h

uint16_t spectral_ch[14];            // Local mapping (same data)
float spectral_indices[8];           // Calculated vegetation indices

// Indices in spectral_indices array:
#define IDX_NDVI          0     // Normalized Difference Vegetation Index
#define IDX_CHLOROPHYLL   1     // Chlorophyll concentration
#define IDX_ANTHOCYANIN   2     // Anthocyanin stress pigment
#define IDX_WATER_STRESS  3     // Water stress index (WSI)
#define IDX_RED_FAR_RED   4     // Red:Far-Red morphology ratio
#define IDX_PHOTOSYN      5     // Photosynthetic activity
#define IDX_CAROTENOID    6     // Carotenoid/xanthophyll pigments
#define IDX_FLICKER_60HZ  7     // Flicker detection (AC mains)

// Calibration data
struct {
  uint16_t dark_ref[14];        // Dark/black reference
  uint16_t white_ref[14];       // White balance reference
  float    gain_correction[14]; // Per-channel gain
  bool     calibrated;
  uint32_t calibration_time;
} spectral_calibration = {
  .calibrated = false,
  .calibration_time = 0
};

// ==========================================
// SPECTRAL INDEX CALCULATIONS
// ==========================================

float calculate_ndvi() {
  // Use YELLOW (580nm) vs BLUE (445nm) as proxy
  float yellow = as7343_ch[CH_YELLOW_590];      // 580nm
  float blue = as7343_ch[CH_BLUE_440];           // 445nm
  
  if ((yellow + blue) == 0) return 0.0;
  return (yellow - blue) / (yellow + blue);
}

float calculate_chlorophyll_index() {
  float green = as7343_ch[CH_GREEN_550];
  float blue = as7343_ch[CH_BLUE_440];
  
  if (blue == 0) return 0.0;
  return green / blue;
}

float calculate_anthocyanin_index() {
  float blue = as7343_ch[CH_BLUE_440];
  float red = as7343_ch[CH_RED_630];
  
  if (blue == 0 || red == 0) return 0.0;
  
  float inv_blue = 1.0 / blue;
  float inv_red = 1.0 / red;
  return inv_blue - inv_red;
}

float calculate_water_stress_index() {
  float yellow_nir = as7343_ch[CH_YELLOW_590];
  float green = as7343_ch[CH_GREEN_550];
  
  if (green == 0) return 0.0;
  return yellow_nir / green;
}

float calculate_red_far_red_ratio() {
  float red = as7343_ch[CH_RED_630];
  float yellow = as7343_ch[CH_YELLOW_590];
  
  if (yellow == 0) return 0.0;
  return red / yellow;
}

float calculate_photosynthetic_activity() {
  float chlorophyll_bands = as7343_ch[CH_BLUE_440] + as7343_ch[CH_GREEN_550];
  float carotenoid_bands = as7343_ch[CH_YELLOW_590] + as7343_ch[CH_YELLOW_590];
  
  if (carotenoid_bands == 0) return 0.0;
  return chlorophyll_bands / carotenoid_bands;
}

float calculate_carotenoid_index() {
  float blue = as7343_ch[CH_BLUE_440];
  float yellow = as7343_ch[CH_YELLOW_590];
  
  if (blue == 0 || yellow == 0) return 0.0;
  
  float inv_blue = 1.0 / blue;
  float inv_yellow = 1.0 / yellow;
  return inv_yellow - inv_blue;
}

float calculate_flicker_level() {
  return (float)as7343_ch[CH_CLEAR];
}

/**
 * Calculate all vegetation indices at once
 * Uses as7343_ch[] directly from as7343_sensor.h
 */
void calculate_all_indices() {
  // Debug: show raw channel values being used
  Serial.print("[DEBUG] Ch - 415:");
  Serial.print(as7343_ch[0]);
  Serial.print(" 445:");
  Serial.print(as7343_ch[1]);
  Serial.print(" 545:");
  Serial.print(as7343_ch[4]);
  Serial.print(" 580:");
  Serial.println(as7343_ch[5]);
  
  spectral_indices[IDX_NDVI]         = calculate_ndvi();
  spectral_indices[IDX_CHLOROPHYLL]  = calculate_chlorophyll_index();
  spectral_indices[IDX_ANTHOCYANIN]  = calculate_anthocyanin_index();
  spectral_indices[IDX_WATER_STRESS] = calculate_water_stress_index();
  spectral_indices[IDX_RED_FAR_RED]  = calculate_red_far_red_ratio();
  spectral_indices[IDX_PHOTOSYN]     = calculate_photosynthetic_activity();
  spectral_indices[IDX_CAROTENOID]   = calculate_carotenoid_index();
  spectral_indices[IDX_FLICKER_60HZ] = calculate_flicker_level();
}

// ==========================================
// CALIBRATION FUNCTIONS
// ==========================================

/**
 * Dark Calibration - measure zero-light reference
 * Should be done with lens covered or in dark
 */
void spectral_dark_calibration() {
  Serial.println("\n[SPECTRAL] Dark Calibration - Cover sensor for 3 seconds...");
  delay(3000);
  
  // Copy current AS7343 readings as dark reference
  for (int i = 0; i < 12; i++) {
    spectral_calibration.dark_ref[i] = spectral_ch[i];
  }
  
  Serial.println("[SPECTRAL] Dark calibration complete");
}

/**
 * White Balance Calibration - measure white reference
 * Use Spectralon or white paper under neutral illumination
 */
void spectral_white_balance_calibration() {
  Serial.println("\n[SPECTRAL] White Balance - Point at diffuse white reference for 3 seconds...");
  delay(3000);
  
  // Copy current readings as white reference
  for (int i = 0; i < 12; i++) {
    spectral_calibration.white_ref[i] = spectral_ch[i];
  }
  
  // Calculate per-channel gain correction (normalize to unity)
  for (int i = 0; i < 12; i++) {
    if (spectral_calibration.white_ref[i] > 0) {
      spectral_calibration.gain_correction[i] = 1000.0 / spectral_calibration.white_ref[i];
    } else {
      spectral_calibration.gain_correction[i] = 1.0;
    }
  }
  
  spectral_calibration.calibrated = true;
  spectral_calibration.calibration_time = millis();
  
  Serial.println("[SPECTRAL] White balance calibration complete");
}

/**
 * Apply calibration to raw readings
 */
void apply_spectral_calibration() {
  if (!spectral_calibration.calibrated) return;
  
  for (int i = 0; i < 12; i++) {
    // Subtract dark reference
    int corrected = spectral_ch[i] - spectral_calibration.dark_ref[i];
    if (corrected < 0) corrected = 0;
    
    // Apply white balance gain
    corrected = (float)corrected * spectral_calibration.gain_correction[i];
    if (corrected > 65535) corrected = 65535;
    
    spectral_ch[i] = corrected;
  }
}

// ==========================================
// DISPLAY & ANALYSIS FUNCTIONS
// ==========================================

/**
 * Print spectral channels formatted
 */
void print_spectral_channels() {
  Serial.print("[SPECTRAL] ");
  for (int i = 0; i < 12; i++) {
    Serial.print(spectral_ch[i]);
    if (i < 11) Serial.print(" ");
  }
  Serial.println();
}

/**
 * Print all vegetation indices
 */
void print_vegetation_indices() {
  Serial.println("\n[VEGETATION INDICES]");
  Serial.print("  NDVI: ");
  Serial.println(spectral_indices[IDX_NDVI], 3);
  
  Serial.print("  Chlorophyll Index: ");
  Serial.println(spectral_indices[IDX_CHLOROPHYLL], 2);
  
  Serial.print("  Anthocyanin Index: ");
  Serial.println(spectral_indices[IDX_ANTHOCYANIN], 3);
  
  Serial.print("  Water Stress Index: ");
  Serial.println(spectral_indices[IDX_WATER_STRESS], 2);
  
  Serial.print("  Red:Far-Red Ratio: ");
  Serial.println(spectral_indices[IDX_RED_FAR_RED], 2);
  
  Serial.print("  Photosynthetic Activity: ");
  Serial.println(spectral_indices[IDX_PHOTOSYN], 2);
  
  Serial.print("  Carotenoid Index: ");
  Serial.println(spectral_indices[IDX_CAROTENOID], 3);
  
  Serial.print("  Clear Channel (Illumination): ");
  Serial.println(spectral_ch[CH_CLEAR]);
  
  Serial.println();
}

/**
 * Plant health levels (0-5 scale)
 */
struct HealthLevels {
  uint8_t vigor;          // 0=dead, 5=excellent
  uint8_t chlorophyll;    // 0=critical, 5=excellent
  uint8_t stress;         // 0=low stress, 5=critical stress
  uint8_t water;          // 0=critical drought, 5=well-watered
};

HealthLevels health_levels = {0, 0, 0, 0};

/**
 * Calculate health level for VIGOR (0-5)
 * Based on NDVI
 */
uint8_t get_vigor_level() {
  float ndvi = spectral_indices[IDX_NDVI];
  
  if (ndvi > 0.7) return 5;      // EXCELLENT
  if (ndvi > 0.5) return 4;      // GOOD
  if (ndvi > 0.3) return 3;      // FAIR
  if (ndvi > 0.1) return 2;      // POOR
  if (ndvi > 0.0) return 1;      // VERY POOR
  return 0;                       // DEAD
}

/**
 * Calculate health level for CHLOROPHYLL (0-5)
 * Based on Chlorophyll Index
 */
uint8_t get_chlorophyll_level() {
  float chi = spectral_indices[IDX_CHLOROPHYLL];
  
  if (chi > 4.0) return 5;       // EXCELLENT
  if (chi > 3.0) return 4;       // GOOD
  if (chi > 2.0) return 3;       // NORMAL
  if (chi > 0.5) return 2;       // LOW
  if (chi > 0.0) return 1;       // VERY LOW
  return 0;                       // CRITICAL
}

/**
 * Calculate health level for STRESS (0-5)
 * Based on Anthocyanin Index (0=low stress, 5=critical stress)
 */
uint8_t get_stress_level() {
  float ari = spectral_indices[IDX_ANTHOCYANIN];
  
  if (ari > 0.4) return 5;       // CRITICAL STRESS
  if (ari > 0.3) return 4;       // SEVERE STRESS
  if (ari > 0.2) return 3;       // MODERATE STRESS
  if (ari > 0.05) return 2;      // MILD STRESS
  if (ari > 0.0) return 1;       // LOW STRESS
  return 0;                       // NO STRESS
}

/**
 * Calculate health level for WATER (0-5)
 * Based on Water Stress Index
 */
uint8_t get_water_level() {
  float wsi = spectral_indices[IDX_WATER_STRESS];
  
  if (wsi > 5.0) return 5;       // EXCELLENT (abundant water)
  if (wsi > 3.0) return 4;       // ADEQUATE
  if (wsi > 1.5) return 3;       // MARGINAL
  if (wsi > 1.0) return 2;       // STRESSED
  if (wsi > 0.5) return 1;       // SEVERE DEFICIT
  return 0;                       // CRITICAL DROUGHT
}

/**
 * Calculate all health levels at once
 */
void calculate_health_levels() {
  health_levels.vigor = get_vigor_level();
  health_levels.chlorophyll = get_chlorophyll_level();
  health_levels.stress = get_stress_level();
  health_levels.water = get_water_level();
}

/**
 * Print health level descriptions for serial debug
 */
void print_health_description() {
  Serial.print("[HEALTH] Vigor:");
  Serial.print(health_levels.vigor);
  Serial.print(" Chlor:");
  Serial.print(health_levels.chlorophyll);
  Serial.print(" Stress:");
  Serial.print(health_levels.stress);
  Serial.print(" Water:");
  Serial.println(health_levels.water);
}

#endif // SPECTRAL_ANALYSIS_H
