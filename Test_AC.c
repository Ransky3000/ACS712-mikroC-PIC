/*
  Test_AC.c
  Description: Reads AC RMS Current from ACS712 (Integer Version)
  Hardware: PIC16F88 @ 8MHz
*/

#include "ACS712.h"

ACS712_t mySensor;
char text[15];

// Integer-based print function (takes value * 1000)
// For integers, we pass the value directly (e.g. 1234 mA = 1.234 A)
void Serial_Print_mA_as_Amps(unsigned int val_mA) {
    unsigned int whole = val_mA / 1000;
    unsigned int frac = val_mA % 1000;
    
    UART1_Write((whole % 10) + '0'); // Print 1 digit for Amps check
    UART1_Write('.');
    
    UART1_Write((frac / 100) + '0');
    UART1_Write(((frac / 10) % 10) + '0');
    UART1_Write((frac % 10) + '0');
}

void main() {
    // 1. Hardware Init
    OSCCON = 0x70;    // 8MHz
    ANSEL = 0x01;     // AN0 Analog
    TRISA = 0x01;     // RA0 Input
    TRISB = 0x24;     // RB2/RB5 UART

    UART1_Init(9600);
    ADC_Init();
    Time_Init(8);      // Init TimeLib
    Delay_ms(100);

    UART1_Write_Text("ACS712 AC Test (Integer)\r\n");

    // 2. Initialize Sensor
    // Init with: channel 0, Vref=5000mV, Res=1023
    ACS712_Init(&mySensor, 0, 5000, 1023);
    
    // Set Sensitivity: 100 mV/A (for 20A module)
    ACS712_SetSensitivity(&mySensor, 100);

    // 3. Manual Zero for Test
    mySensor.zero_point = 512; 
    UART1_Write_Text("Ready.\r\n");

    while(1) {
        // Read RMS at 60Hz - Returns mA
        unsigned int ac_mA = ACS712_ReadAC(&mySensor, 60);

        UART1_Write_Text("AC: ");
        Serial_Print_mA_as_Amps(ac_mA);
        UART1_Write_Text(" A\r\n");

        Delay_ms(500);
    }
}
