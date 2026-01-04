#include "ACS712.h"

// Note: Ensure 'ADC' library is checked in Library Manager

void ACS712_Init(ACS712_t* sensor, unsigned short channel, float v_ref, int adc_res) {
    sensor->adc_channel = channel;
    sensor->voltage_reference = v_ref;
    sensor->adc_resolution = adc_res;
    
    // Defaults
    sensor->sensitivity = 0.185; // Default to 5A model
    sensor->zero_point = adc_res / 2.0; 
}

void ACS712_SetSensitivity(ACS712_t* sensor, float sensitivity) {
    sensor->sensitivity = sensitivity;
}

float ACS712_GetZeroPoint(ACS712_t* sensor) {
    return sensor->zero_point;
}

void ACS712_SetZeroPoint(ACS712_t* sensor, float zero) {
    sensor->zero_point = zero;
}

void ACS712_Calibrate(ACS712_t* sensor) {
    unsigned int i;
    unsigned long accumulator = 0;
    
    // Average 100 samples
    for (i = 0; i < 100; i++) {
        accumulator += ADC_Read(sensor->adc_channel);
        Delay_ms(1); // Small settling time
    }
    
    sensor->zero_point = (float)accumulator / 100.0;
}

float ACS712_ReadDC(ACS712_t* sensor) {
    unsigned int i;
    unsigned long accumulator = 0;
    float avg_adc;
    float voltage;
    
    // Oversampling for stability
    for (i = 0; i < 100; i++) {
        accumulator += ADC_Read(sensor->adc_channel);
    }
    avg_adc = (float)accumulator / 100.0;
    
    // Calculate Voltage: (ADC_Value / Resolution) * V_Ref
    voltage = (avg_adc / sensor->adc_resolution) * sensor->voltage_reference;
    
    // Calculate Voltage relative to Zero Point
    // We need the zero point in VOLTS first, or just subtract ADC zero
    // Let's do it fundamentally:
    // V_current = (ADC_Read - ADC_Zero) * (V_Ref / ADC_Res)
    
    voltage = (avg_adc - sensor->zero_point) * (sensor->voltage_reference / sensor->adc_resolution);
    
    return voltage / sensor->sensitivity;
}

// Helper: Fast Integer Square Root (Babylonian/Newton method)
unsigned long isqrt(unsigned long n) {
    unsigned long x = n;
    unsigned long y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / y) / 2;
    }
    return x;
}

// Fixed-Point AC Read (Returns mA)
// Eliminates Float Math & Sqrt Library to save space (Demo Limit)
unsigned int ACS712_ReadAC_Int(ACS712_t* sensor, unsigned int frequency) {
    unsigned long period_ms = 1000 / frequency;
    unsigned long start_time;
    unsigned long accumulator = 0;
    unsigned long count = 0;
    
    long sample;
    long zero_p = (long)sensor->zero_point; // Use integer zero point (e.g. 512)
    
    unsigned long avg_sq;
    unsigned long rms_raw;
    unsigned long voltage_rms_mv;
    unsigned long amps_rms_mA;
    
    // External generic millis() prototype
    // extern unsigned long millis(); 

    start_time = millis();

    while ((millis() - start_time) < period_ms) {
         sample = (long)ADC_Read(sensor->adc_channel);
         sample -= zero_p; // Center at 0
         
         // Accumulate squares (long * long = long)
         // Max ADC diff is ~512. 512^2 = 262,144.
         // Accumulator (unsigned long 32-bit) can hold ~4 billion. 
         // We can safe sum ~16,000 samples. 20ms is fine.
         accumulator += (sample * sample);
         count++;
    }
    
    if (count == 0) return 0;
    
    avg_sq = accumulator / count;
    rms_raw = isqrt(avg_sq); // Result is in ADC units (0-1023)
    
    // Calculate Volts (mV) = (RMS_Raw * V_Ref_mV) / ADC_Res
    // V_Ref = 5000mV
    // Use 32-bit math: rms * 5000 / 1023
    voltage_rms_mv = (rms_raw * 5000) / 1023;
    
    // Calculate Amps (mA) = Voltage_RMS_mV / Sensitivity_mV_per_A
    // Sensitivity is float in struct (0.100), but we need integer mV/A.
    // 0.100 V/A = 100 mV/A. 
    // We will compute sensitivity_mV = sensitivity * 1000
    // amps_mA = voltage_rms_mv / (sensitivity * 1000) -> logic:
    // amps_mA = (voltage_rms_mv * 1000) / (sensitivity_V * 1000) ?? No.
    // Amps = Volts / Sens. 
    // mA = mV / (V/A / 1000)? No.
    // mA = mV / (mV/mA).
    // Sensitivity is V/A. e.g. 0.1 V/A = 100 mV/A.
    // Current (A) = Volts / 0.1
    // Current (mA) = Volts / 0.1 * 1000 = Volts * 10000.
    // Let's stick to mV.
    // Current (A) = Volts_RMS / Sensitivity_V_A
    // Current (mA) = (Volts_RMS * 1000) / Sensitivity_V_A
    // Current (mA) = Voltage_RMS_mV / Sensitivity_V_A
    // e.g. 100mV measured / 0.1 V/A = 1A = 1000mA.
    // 100 / 0.1 = 1000. Correct.
    
    // Avoid float divide. multiply numerator by 10 or 100?
    // amps_mA = voltage_rms_mv / sensitivity; (if sensitivity was int mV/A)
    // struct has float sensitivity (e.g. 0.100).
    // Let's do: (voltage_rms_mv * 1000) / (sensitivity * 1000)
    
    amps_rms_mA = (unsigned long)(voltage_rms_mv) / (unsigned long)(sensor->sensitivity * 1000.0);
    
    // Correction factor to match mA scale (x1000)
    amps_rms_mA = amps_rms_mA * 1000; 
    
    return (unsigned int)amps_rms_mA;
}
