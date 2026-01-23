/* File: ACS712.c */
#include "ACS712.h"

// Integer Square Root Helper
unsigned long isqrt(unsigned long n) {
    unsigned long x = n;
    unsigned long y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / y) / 2;
    }
    return x;
}

void ACS712_Init(ACS712_t* sensor, unsigned char channel, unsigned int v_ref_mv, int adc_res) {
    sensor->adc_channel = channel;
    sensor->voltage_reference_mv = v_ref_mv;
    sensor->adc_resolution = adc_res;
    
    sensor->sensitivity_mV_A = 100; // Default 100mV/A (20A)
    sensor->zero_point = adc_res / 2; 
}

void ACS712_SetSensitivity(ACS712_t* sensor, unsigned int sens_mv_a) {
    sensor->sensitivity_mV_A = sens_mv_a;
}

void ACS712_Calibrate(ACS712_t* sensor) {
    // UPDATED: Average over 100ms to cancel out AC sine wave
    // 100ms covers 6 cycles of 60Hz (99.6ms) or 5 cycles of 50Hz (100ms)
    unsigned long start = micros();
    unsigned long accumulator = 0;
    unsigned int count = 0;
    
    // Loop for 100,000 microseconds (100ms)
    while (micros() - start < 100000) {
        accumulator += ADC_Read(sensor->adc_channel);
        count++;
        // Small delay if needed? implicit ADC delay (~20us) is enough.
    }
    
    if (count > 0) {
       sensor->zero_point = (unsigned int)(accumulator / count);
    }
}

unsigned int ACS712_ReadAC(ACS712_t* sensor, unsigned char frequency) {
    unsigned long period_us;
    
    // Lookup for period (avoid division)
    if (frequency == 60) period_us = 16666;
    else if (frequency == 50) period_us = 20000;
    else period_us = 16666;

    unsigned long start_time = micros();
    unsigned long accumulator = 0;
    unsigned long count = 0;
    
    long sample;
    long zero = (long)sensor->zero_point;

    while ((micros() - start_time) < period_us) {
         sample = (long)ADC_Read(sensor->adc_channel);
         sample -= zero;
         accumulator += (unsigned long)(sample * sample);
         count++;
    }
    
    if (count == 0) return 0;
    
    // RMS of ADC Steps
    unsigned long avg_sq = accumulator / count;
    unsigned long rms_adc = isqrt(avg_sq);
    
    // Calculate Voltage RMS (mV)
    // V = (RMS_ADC * V_Ref_mV) / Resolution
    // Use unsigned long for calc to prevent overflow
    unsigned long voltage_rms_mv = (rms_adc * (unsigned long)sensor->voltage_reference_mv) / sensor->adc_resolution;
    
    // Calculate Amps RMS (mA)
    // mA = (mV * 1000) / Sensitivity_mV_per_A
    // Wait, Sensitivity is mV/A. 
    // Example: 1 Amp -> 100mV.
    // If we have 100mV RMS read:
    // Amps = 100mV / 100 (mV/A) = 1 Amp.
    // We want milliAmps.
    // mA = (Voltage_mV * 1000) / Sensitivity
    
    unsigned long current_mA = (voltage_rms_mv * 1000) / sensor->sensitivity_mV_A;
    
    return (unsigned int)current_mA;
}
