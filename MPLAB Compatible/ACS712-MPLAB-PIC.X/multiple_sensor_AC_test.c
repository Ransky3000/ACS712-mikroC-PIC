/*
 * File:   multiple_sensor_AC_test.c
 * Author: Antigravity
 *
 * Created on Jan 23, 2026
 *
 * Description: 
 * Test code for reading TWO ACS712 sensors simultaneously (sequentially)
 * using the ACS712 Library ported for MPLAB X XC8.
 */

// CONFIG1
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (Internal oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RA5/MCLR/VPP Pin Function Select bit (MCLR enabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable bit (RB3 is digital I/O)
#pragma config CPD = OFF        // Data EE Memory Code Protection bit (Code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off)
#pragma config CCPMX = RB0      // CCP1 Pin Selection bit (CCP1 on RB0)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

// CONFIG2
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal External Switchover mode disabled)

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>

// Include our custom libraries
#include "ACS712.h"
#include "UART_Lib.h"
#include "ADC_Lib.h"
#include "Timer_lib.h"

#define _XTAL_FREQ 8000000

// Buffer for string formatting
char buffer[64];

void main(void) {
    // 1. Oscillator Setup (8MHz)
    OSCCON = 0b01110000;
    while(!OSCCONbits.IOFS); // Wait for stable

    // 2. Initialize Low-Level Libraries
    UART_Init();      // 9600 Baud
    ADC_Init();       // ADC ON, Right Justified
    Time_Init(8);     // Timer0 for micros() @ 8MHz

    UART_Write_Text("Double ACS712 Test Started...\r\n");

    // 3. Create Sensor Objects
    ACS712_t sensor1;
    ACS712_t sensor2;

    // 4. Initialize Sensors
    // Sensor 1 on Channel 0 (AN0), 5000mV Ref, 1023 Res
    ACS712_Init(&sensor1, 0, 5000, 1023); 
    
    // Sensor 2 on Channel 1 (AN1), 5000mV Ref, 1023 Res
    ACS712_Init(&sensor2, 1, 5000, 1023);

    UART_Write_Text("Calibrating Sensors (Ensure 0A code flow)...\r\n");
    
    // 5. Calibrate (Zero Point)
    // IMPORTANT: No current should be flowing during this step
    ACS712_Calibrate(&sensor1);
    UART_Write_Text("Sensor 1 Calibrated.\r\n");
    
    ACS712_Calibrate(&sensor2);
    UART_Write_Text("Sensor 2 Calibrated.\r\n");

    // 6. Main Loop
    while(1) {
        // Read Sensor 1 (AC 60Hz) - Takes ~17ms
        unsigned int current1 = ACS712_ReadAC(&sensor1, 60);
        
        // Read Sensor 2 (AC 60Hz) - Takes ~17ms
        unsigned int current2 = ACS712_ReadAC(&sensor2, 60);

        // Format Output - Manual Float conversion to save space
        // Example: 1234 mA -> 1.234 A
        
        // Sensor 1
        unsigned int s1_int = current1 / 1000;
        unsigned int s1_dec = current1 % 1000;
        
        // Sensor 2
        unsigned int s2_int = current2 / 1000;
        unsigned int s2_dec = current2 % 1000;

        // "S1: 1.234 A | S2: 5.678 A"
        // Note: %03u ensures "005" for 5mA, etc.
        sprintf(buffer, "S1: %u.%03u A | S2: %u.%03u A\r\n", s1_int, s1_dec, s2_int, s2_dec);
        UART_Write_Text(buffer);

        __delay_ms(300); // Update every half second
    }
}
