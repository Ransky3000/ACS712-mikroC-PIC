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
    // Declarations at TOP of scope (C89/mikroC)
    unsigned long previousMillis = 0;
    unsigned int ac_mA; // Moved declaration here

    // 1. Hardware Init (Scope Top)
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
    ACS712_Init(&mySensor, 0, 5000, 1023);
    ACS712_SetSensitivity(&mySensor, 100);

    // 3. Initial Calibrate
    UART1_Write_Text("Calibrating Zero Point...\r\n");
    ACS712_Calibrate(&mySensor);
    UART1_Write_Text("Ready.\r\n");
    
    previousMillis = millis();

    while(1) {
        // Read RMS at 60Hz - Returns mA
        ac_mA = ACS712_ReadAC(&mySensor, 60);

        UART1_Write_Text("AC: ");
        Serial_Print_mA_as_Amps(ac_mA);
        UART1_Write_Text(" A\r\n");

        
        // Periodic Calibration (Every 5 Seconds)
        if (millis() - previousMillis >= 5000) {
            previousMillis = millis();
            UART1_Write_Text("Re-Calibrating...\r\n");
            ACS712_Calibrate(&mySensor); // Integer Calib is fast and small
            UART1_Write_Text("Done.\r\n");
        }

        Delay_ms(500);
    }
}
