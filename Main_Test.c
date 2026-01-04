/*
  ACS712 Test Main
  Hardware: PIC16F88 @ 8MHz
  Dependencies (Library Manager):
  - ADC, UART, C_Math, Conversions, TimeLib (Your custom lib)
*/

#include "ACS712.h"

// Define the sensor structure
ACS712_t mySensor;
char text[15];

void main() {
    // 1. Hardware Init
    OSCCON = 0x70;    // 8MHz
    ANSEL = 0x01;     // AN0 Analog (RA0)
    TRISA = 0x01;     // RA0 Input
    TRISB = 0x24;     // RB2(RX) & RB5(TX) Inputs for UART

    UART1_Init(9600);
    ADC_Init();
    Time_Init(8);      // Initialize your TimeLib (8MHz)
    Delay_ms(100);

    UART1_Write_Text("ACS712 Driver Test\r\n");

    // 2. Initialize Sensor
    // Channel 0, 5.0V ref, 1023 res
    ACS712_Init(&mySensor, 0, 5.0, 1023);
    
    // Set for 20A module (0.100 V/A)
    ACS712_SetSensitivity(&mySensor, 0.100);

    // 3. Calibrate (Zero Point)
    UART1_Write_Text("Calibrating Zero Point...\r\n");
    ACS712_Calibrate(&mySensor);
    UART1_Write_Text("Done.\r\n");

    while(1) {
        // Test DC Reading
        float dc_amps = ACS712_ReadDC(&mySensor);
        
        // Test AC Reading (RMS) - 60Hz
        float ac_amps = ACS712_ReadAC(&mySensor, 60);

        // Print Results
        UART1_Write_Text("DC: ");
        FloatToStr(dc_amps, text);
        UART1_Write_Text(text);
        UART1_Write_Text(" A  |  AC: ");
        
        FloatToStr(ac_amps, text);
        UART1_Write_Text(text);
        UART1_Write_Text(" A\r\n");

        Delay_ms(1000);
    }
}
