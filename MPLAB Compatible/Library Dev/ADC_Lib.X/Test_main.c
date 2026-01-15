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
#include "ADC_Lib.h"

#define _XTAL_FREQ 8000000 // Required for __delay_ms

void main() {
    // 1. Hardware Setup
    OSCCON = 0x70;  // 8MHz Internal Osc
    
    // ANSEL: Configure AN0 (RA0) as Analog Input
    // Set Bit 0 to 1. All others Digital (0).
    ANSEL = 0b00000001; 
    
    TRISAbits.TRISA0 = 1; // RA0 is Input

    // 2. Init Library
    ADC_Init();

    // 3. Main Loop
    while(1) {
        unsigned int val;
        
        // Read Analog Value from Channel 0 (AN0)
        val = ADC_Read(0);
        
        // Logic: You can set a breakpoint here to check 'val'
        // or add UART code to print it.
        
        __delay_ms(100);
    }
}
