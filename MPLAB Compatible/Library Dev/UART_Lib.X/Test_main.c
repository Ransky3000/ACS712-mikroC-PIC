/*
  File: Test_main.c
  MCU: PIC16F88
  Oscillator: 8.00 MHz
*/

// --- CONFIGURATION BITS ---
#pragma config FOSC = INTOSCIO  // Internal oscillator, port I/O
#pragma config WDTE = OFF       // Watchdog Timer disabled
#pragma config PWRTE = ON       // Power-up Timer enabled
#pragma config MCLRE = ON       // RA5/MCLR pin function is MCLR
#pragma config BOREN = OFF      // Brown-out Reset disabled
#pragma config LVP = OFF        // Low-Voltage Programming disabled
#pragma config CPD = OFF        // Data memory code protection off
#pragma config WRT = OFF        // Flash program memory write protection off
#pragma config CCPMX = RB0      // CCP1 pin selection
#pragma config CP = OFF         // Flash program memory code protection off
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor disabled
#pragma config IESO = OFF       // Internal External Switchover disabled

#include <xc.h>
#include "UART_Lib.h"

#define _XTAL_FREQ 8000000 // Required for __delay_ms

void main() {
    // 1. Hardware Setup
    OSCCON = 0x70;  // 8MHz Internal Osc
    ANSEL = 0x00;   // All Digital

    // 2. Init Library
    UART_Init();

    // 3. Main Loop
    while(1) {
        UART_Write_Text("Hello from MPLAB XC8!\r\n");
        __delay_ms(1000);
        
        // Optional: Echo back if data received
        if (UART_Data_Ready()) {
            char c = UART_Read();
            UART_Write_Text("Echo: ");
            UART_Write(c);
            UART_Write('\r');
            UART_Write('\n');
        }
    }
}
