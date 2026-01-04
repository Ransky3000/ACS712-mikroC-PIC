#ifndef ACS712_H
#define ACS712_H

/*
  ACS712 Driver for PIC16F88 (mikroC PRO for PIC)
  PURE INTEGER VERSION (Lite) - To fit in Demo Limit
  
  Dependencies:
  - ADC Library
  - TimeLib (Custom)
  - UART (for Test)
*/

typedef struct {
    unsigned short adc_channel;  // AN0, AN1
    unsigned int  voltage_reference_mv; // e.g. 5000 (mV)
    unsigned int  adc_resolution;       // e.g. 1023
    unsigned int  zero_point;           // e.g. 512
    unsigned int  sensitivity_mV_A;     // e.g. 100 or 185 (mV per Amp)
} ACS712_t;

// Init: v_ref in mV (5000), adc_res (1023)
void ACS712_Init(ACS712_t* sensor, unsigned short channel, unsigned int v_ref_mv, unsigned int adc_res);

// Set Sens in mV/A (e.g. 100 for 20A module)
void ACS712_SetSensitivity(ACS712_t* sensor, unsigned int sensitivity_mV_A);

unsigned int ACS712_GetZeroPoint(ACS712_t* sensor);
void ACS712_SetZeroPoint(ACS712_t* sensor, unsigned int zero);

void ACS712_Calibrate(ACS712_t* sensor);

// Returns mA
unsigned int ACS712_ReadDC(ACS712_t* sensor);
unsigned int ACS712_ReadAC(ACS712_t* sensor, unsigned int frequency);

#endif
