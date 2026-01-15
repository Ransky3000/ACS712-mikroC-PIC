/*
  File: Test_main.c
  MCU: PIC16F88
  Oscillator: 8.00 MHz
*/

// --- CONFIGURATION BITS (XC8) ---
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
#include "TimeLib.h"

// Example: Blinking LED without Delay_ms() (Non-blocking!)
// XC8 Bit Addressing: LATBbits.LATB0 or PORTBbits.RB0
#define LED_PIN      RB0
#define LED_TRIS     TRISB0

unsigned long previousMillis = 0;
const long interval = 1000; // Blink every 1000ms (1 second)

void main() {
    // 1. Hardware Setup
    OSCCON = 0x70;  // 8MHz Internal Osc
    ANSEL = 0x00;   // Digital I/O (ANSEL=0)

    LED_TRIS = 0;    // Output
    LED_PIN = 0;

    // 2. Initialize our new Library
    Time_Init(8);

    // 3. Main Loop
    while(1) {
        unsigned long currentMillis = millis();

        // check if 1000ms has passed
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis; // save the last time you blinked

            // Toggle LED
            LED_PIN = !LED_PIN; 
        }
    }
}