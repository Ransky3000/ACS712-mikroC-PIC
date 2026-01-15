/* File: ACS712.h */
#ifndef ACS712_H
#define ACS712_H

#include <xc.h>

typedef struct {
    unsigned char adc_channel; // ADC Pin (0-7)
    unsigned int voltage_reference_mv;   // Vref in mV (e.g. 5000)
    int adc_resolution;        // e.g. 1023
    unsigned int zero_point;   // Zero point (ADC Value, e.g. 512)
    unsigned int sensitivity_mV_A; // mV per Amp (e.g. 185, 100, 66)
} ACS712_t;

// Initialize
void ACS712_Init(ACS712_t* sensor, unsigned char channel, unsigned int v_ref_mv, int adc_res);

// Set Sensitivity (mV per Amp)
void ACS712_SetSensitivity(ACS712_t* sensor, unsigned int sens_mv_a);

// Calibrate Zero Point
void ACS712_Calibrate(ACS712_t* sensor);

// Read AC Current (Return: milliAmps)
unsigned int ACS712_ReadAC(ACS712_t* sensor, unsigned char frequency);

#endif
