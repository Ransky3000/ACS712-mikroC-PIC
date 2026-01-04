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
    
    // Note regarding millis():
    // Standard simple approach without relying on external TimeLib (to keep driver generic):
    // We might just use Delay-based loop or checks.
    // However, user HAS TimeLib and expects to use it?
    // Actually, driver should be generic. 
    // We can use a loop count estimation or just block for period_ms approx.
    // But since we have no built-in millis in core mikroC, blocking for a fixed sample count is safer?
    // Or we assume standard loop.
    //
    // Let's use the optimized approach we did in Arduino: sum of squares.
    // BUT we don't have millis() easily inside the library unless we pass a function pointer or use a known timer.
    // 
    // TRADEOUT: To keep this specific library simple and portable, 
    // we will stick to a fixed SAMPLE COUNT that approximates one cycle, 
    // OR we just sample fast for a duration using Delay_us checks?
    //
    // Let's try the "loop for duration" using built-in Delay typically isn't variable.
    // Wait, mikroC Delay_ms requires constant. Vdelay_ms takes variable.
    
    // REFACTOR: We'll sample for a specific number of samples that roughly covers 20ms? 
    // ADC_Read takes time. 
    // Let's rely on simple blocking sampling for now.
    
    zero_p = sensor->zero_point;
    
    // We can't easily loop "for 20ms" without a timer source.
    // We will just take a large number of samples (e.g., 300) which usually exceeds 20ms on PIC sampling speeds
    // This is "good enough" for basic RMS if we capture at least one cycle.
    // Better yet, just take 1000 samples? No, too slow.
    
    // Let's assume user provides TimeLib? No, that couples it.
    // We will use a fixed loop count. 
    // On 8MHz PIC, 1 instruction = 0.5us. ADC_Read takes ~20us?
    
    // Let's try 400 samples. 
    
    for (count = 0; count < 400; count++) {
         float sample = ADC_Read(sensor->adc_channel);
         sample -= zero_p;
         accumulator += (sample * sample);
         // No delay, sample as fast as possible
    }
    
    avg_adc = (float)accumulator / count;
    rms_adc = sqrt(avg_adc);
    
    voltage_rms = rms_adc * (sensor->voltage_reference / sensor->adc_resolution);
    amps_rms = voltage_rms / sensor->sensitivity;
    
    return amps_rms;
}
