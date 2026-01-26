/* File: ADC_Lib.h */
#ifndef ADC_LIB_H
#define ADC_LIB_H

#include <xc.h>

void ADC_Init();
unsigned int ADC_Read(unsigned char channel);

#endif
