#ifndef ACS712_H
#define ACS712_H

/*
  ACS712 Driver for PIC16F88 (mikroC PRO for PIC)
  
  Dependencies:
  - ADC Library (System)
  - C_Math Library (System) for sqrt
  - Conversions (Optional, for examples)
*/

// Struct to hold sensor state (replaces C++ Class)
typedef struct {
    unsigned short adc_channel; // AN0, AN1, etc.
    float voltage_reference;     // usually 5.0
    int adc_resolution;          // usually 1023
    float zero_point;            // calibration value
    float sensitivity;           // V/A (e.g., 0.185 for 5A)
} ACS712_t;

// API Functions
void ACS712_Init(ACS712_t* sensor, unsigned short channel, float v_ref, int adc_res);
void ACS712_SetSensitivity(ACS712_t* sensor, float sensitivity);
float ACS712_GetZeroPoint(ACS712_t* sensor);
void ACS712_SetZeroPoint(ACS712_t* sensor, float zero);

void ACS712_Calibrate(ACS712_t* sensor);
float ACS712_ReadDC(ACS712_t* sensor);
// Returns mA (Integer) to avoid float math
unsigned int ACS712_ReadAC_Int(ACS712_t* sensor, unsigned int frequency);

#endif
