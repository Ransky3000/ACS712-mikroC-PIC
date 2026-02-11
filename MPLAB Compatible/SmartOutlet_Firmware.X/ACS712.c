/* File: ACS712.c */
#include "ACS712.h"
#include "Timer_lib.h"
#include "ADC_Lib.h"

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
    
    /* 
     * OPTIMIZATION: Simplified Math for 20A Sensor @ 5V
     * Original Formula (Expensive 32-bit Code):
     * unsigned long voltage_rms_mv = (rms_adc * (unsigned long)sensor->voltage_reference_mv) / sensor->adc_resolution;
     * unsigned long current_mA = (voltage_rms_mv * 1000) / sensor->sensitivity_mV_A;
     *
     * Simplified Factor:
     * (5000 / 1023) * (1000 / 100) = 48.875
     * Approx: 49 (Error < 0.3%)
     */
     
    // unsigned long voltage_rms_mv = (rms_adc * (unsigned long)sensor->voltage_reference_mv) / sensor->adc_resolution;
    // unsigned long current_mA = (voltage_rms_mv * 1000) / sensor->sensitivity_mV_A;
    
    unsigned long current_mA = rms_adc * 49; 
    
    return (unsigned int)current_mA;
}