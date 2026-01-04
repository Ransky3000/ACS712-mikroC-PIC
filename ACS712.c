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

float ACS712_ReadAC(ACS712_t* sensor, unsigned int frequency) {
    unsigned long period_ms = 1000 / frequency;
    unsigned long start_time;
    unsigned long accumulator = 0;
    unsigned long count = 0;
    float avg_adc, zero_p;
    float rms_adc, voltage_rms, amps_rms;
    
    // We assume the user has "TimeLib" installed/checked.
    // In mikroC, if a library is checked, its functions are available globally.
    // However, to avoid 'undeclared identifier' errors during compile if the header isn't included,
    // we should rely on the user checking the box. 
    // BUT, the compiler needs to know 'millis' exists.
    // Since we are writing a SOURCE file (.c) inside a project, we can just declare the prototype extern
    // OR just assume the user added TimeLib to the project.
    
    // Best practice for mikroC libraries: 
    // We will prototype millis() here just in case it's not in a header.
    // extern unsigned long millis(); 
    
    zero_p = sensor->zero_point;
    start_time = millis();

    // Loop for one full cycle (e.g. 20ms for 50Hz)
    while ((millis() - start_time) < period_ms) {
         float sample = ADC_Read(sensor->adc_channel);
         sample -= zero_p;
         accumulator += (sample * sample);
         count++;
         // No delay, sample as fast as possible
    }
    
    avg_adc = (float)accumulator / count;
    rms_adc = sqrt(avg_adc);
    
    voltage_rms = rms_adc * (sensor->voltage_reference / sensor->adc_resolution);
    amps_rms = voltage_rms / sensor->sensitivity;
    
    return amps_rms;
}
