/* File: ADC_Lib.h */
#ifndef ADC_LIB_H
#define ADC_LIB_H

#include <xc.h>

// Initialize ADC Module
// Sets Right Justified, Fosc/16 (assuming 8MHz), and Turns ON
void ADC_Init();

// Reads analog value from specified channel (0-6)
// Returns 0-1023 (10-bit)
// Blocking call (~20us)
unsigned int ADC_Read(unsigned char channel);

#endif
