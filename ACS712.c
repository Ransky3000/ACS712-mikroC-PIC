#include "ACS712.h"

// Helper: Integer Sqrt
unsigned long isqrt(unsigned long n) {
    unsigned long x = n;
    unsigned long y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / y) / 2;
    }
    return x;
}

void ACS712_Init(ACS712_t* sensor, unsigned short channel, unsigned int v_ref_mv, unsigned int adc_res) {
    sensor->adc_channel = channel;
    sensor->voltage_reference_mv = v_ref_mv;
    sensor->adc_resolution = adc_res;
    
    // Default 5A model (185 mV/A)
    sensor->sensitivity_mV_A = 185; 
    sensor->zero_point = adc_res / 2; 
}

void ACS712_SetSensitivity(ACS712_t* sensor, unsigned int sensitivity_mV_A) {
    sensor->sensitivity_mV_A = sensitivity_mV_A;
}

unsigned int ACS712_GetZeroPoint(ACS712_t* sensor) {
    return sensor->zero_point;
}

void ACS712_SetZeroPoint(ACS712_t* sensor, unsigned int zero) {
    sensor->zero_point = zero;
}

void ACS712_Calibrate(ACS712_t* sensor) {
    unsigned int i;
    unsigned long accumulator = 0;
    
    for (i = 0; i < 100; i++) {
        accumulator += ADC_Read(sensor->adc_channel);
        Delay_ms(1);
    }
    
    sensor->zero_point = (unsigned int)(accumulator / 100);
}

// Returns mA
unsigned int ACS712_ReadDC(ACS712_t* sensor) {
    unsigned int i;
    unsigned long accumulator = 0;
    long avg_adc_scaled;
    long voltage_uv; 
    long amps_mA;
    long zero = (long)sensor->zero_point;
    
    // Moved declarations up
    long diff_x100;
    long avg;
    long diff;
    long mv;
    
    for (i = 0; i < 100; i++) {
        accumulator += ADC_Read(sensor->adc_channel);
    }
    
    // avg = accumulator/100.
    // raw diff = avg - zero. 
    // Let's keep precision. Diff * 100 = accumulator - (zero * 100)
    
    diff_x100 = (long)accumulator - (zero * 100);
    
    // ADC counts * (Vref_mV / Res) = Voltage_mV
    // Voltage_mV * 1000 = Voltage_uV
    // We have diff_x100 (which is effectively counts * 100)
    // Volts_mV = (Counts * Vref_mV) / Res
    // Volts_mV = (diff_x100 * Vref_mV) / (Res * 100)
    
    // Let's simplify.
    // mV = ( (accumulator/100 - zero) * Vref ) / Res
    
    avg = accumulator / 100;
    diff = avg - zero; 
    
    // mV = (diff * Vref_mv) / Res
    mv = (diff * (long)sensor->voltage_reference_mv) / (long)sensor->adc_resolution;
    
    // mA = (mV * 1000) / Sens_mV_A
    amps_mA = (mv * 1000) / (long)sensor->sensitivity_mV_A;
    
    if (amps_mA < 0) amps_mA = -amps_mA; // Absolute value for simple read
    
    return (unsigned int)amps_mA;
}

unsigned int ACS712_ReadAC(ACS712_t* sensor, unsigned int frequency) {
    unsigned long period_ms = 1000 / frequency;
    unsigned long start_time;
    unsigned long accumulator = 0;
    unsigned long count = 0;
    
    long sample;
    long zero_p = (long)sensor->zero_point;
    
    unsigned long avg_sq;
    unsigned long rms_raw; // ADC units
    unsigned long voltage_rms_mv;
    unsigned long amps_rms_mA;

    // extern unsigned long millis(); 

    start_time = millis();

    while ((millis() - start_time) < period_ms) {
         sample = (long)ADC_Read(sensor->adc_channel);
         sample -= zero_p;
         accumulator += (sample * sample);
         count++;
    }
    
    if (count == 0) return 0;
    
    avg_sq = accumulator / count;
    rms_raw = isqrt(avg_sq); 
    
    // mV_RMS = (RMS_Raw * Vref) / Res
    voltage_rms_mv = (rms_raw * (unsigned long)sensor->voltage_reference_mv) / (unsigned long)sensor->adc_resolution;
    
    // mA = (mV * 1000) / Sens
    amps_rms_mA = (voltage_rms_mv * 1000) / (unsigned long)sensor->sensitivity_mV_A;
    
    return (unsigned int)amps_rms_mA;
}
