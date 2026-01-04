/*
  Test_DC.c
  Description: Reads DC Current from ACS712
  Hardware: PIC16F88 @ 8MHz
*/

#include "ACS712.h"

ACS712_t mySensor;
char text[15];

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

    UART1_Write_Text("ACS712 DC Test\r\n");

    // 2. Initialize Sensor (20A Model)
    ACS712_Init(&mySensor, 0, 5.0, 1023);
    ACS712_SetSensitivity(&mySensor, 0.100);

    // 3. Calibrate
    UART1_Write_Text("Calibrating Zero Point...\r\n");
    ACS712_Calibrate(&mySensor);
    UART1_Write_Text("Ready.\r\n");

    while(1) {
        float dc_amps = ACS712_ReadDC(&mySensor);
        
        UART1_Write_Text("DC Current: ");
        FloatToStr(dc_amps, text);
        UART1_Write_Text(text);
        UART1_Write_Text(" A\r\n");

        Delay_ms(500);
    }
}
