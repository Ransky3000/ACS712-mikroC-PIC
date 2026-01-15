/*
  File: Test_DC.c
  Description: Reads DC Current from ACS712 (Float Version)
  MCU: PIC16F88
*/

// --- CONFIGURATION BITS ---
#pragma config FOSC = INTOSCIO
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = ON
#pragma config BOREN = OFF
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CCPMX = RB0
#pragma config CP = OFF
#pragma config FCMEN = OFF
#pragma config IESO = OFF

#include <xc.h>
#include <stdio.h>
#include "ACS712.h"
#include "UART_Lib.h"
#include "ADC_Lib.h"
#include "Timer_lib.h"

#define _XTAL_FREQ 8000000

ACS712_t mySensor;

void main() {
    // 1. Hardware Init
    OSCCON = 0x70;    // 8MHz
    ANSEL = 0b00000001; // AN0 Analog
    TRISAbits.TRISA0 = 1; // Input
    
    // 2. Init Libraries
    UART_Init();
    ADC_Init();
    Time_Init(8); 
    
    UART_Write_Text("ACS712 DC Float Test\r\n");

    // 3. Init Sensor
    ACS712_Init(&mySensor, 0, 5.0, 1023);     // Ch0, 5V Ref, 10 bit
    ACS712_SetSensitivity(&mySensor, 0.100);  // 100mV/A (20A Module)
    
    // 4. Calibrate
    UART_Write_Text("Calibrating Zero...\r\n");
    ACS712_Calibrate(&mySensor);
    UART_Write_Text("Ready.\r\n");

    while(1) {
        float amps;
        char buffer[30];
        
        // Read DC
        amps = ACS712_ReadDC(&mySensor);
        
        // Float Formatting
        sprintf(buffer, "DC: %.3f A\r\n", amps);
        UART_Write_Text(buffer);
        
        __delay_ms(500);
    }
}
