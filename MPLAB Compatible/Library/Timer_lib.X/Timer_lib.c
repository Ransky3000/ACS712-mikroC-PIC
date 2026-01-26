/* File: TimeLib.c */
#include <xc.h>
#include "Timer_lib.h"

// 1. New Global Variable to store the frequency
// We default to 8, but Time_Init will change it.
volatile unsigned char current_freq_mhz = 8; 

volatile unsigned long micros_counter = 0;

// Interrupt Service Routine (XC8 Syntax)
void Timer_ISR(void) {
    if (TMR0IF) { // Direct Bit Access
        TMR0IF = 0;

        // 2. Logic is now Dynamic (Runtime Check)
        // Since this is inside an interrupt, we keep the math simple!
        if (current_freq_mhz == 8) {
            micros_counter += 128; // 8MHz logic
        } else {
            micros_counter += 256; // 4MHz logic (default)
        }
    }
}

// Now accepts the parameter 'mhz'
void Time_Init(unsigned char mhz) {
    // Save the user's choice to our global variable
    if (mhz == 4 || mhz == 8) {
        current_freq_mhz = mhz;
    }

    OPTION_REG = 0b00001000; // PSA=1 (1:1 Prescaler)
    TMR0 = 0;
    TMR0IF = 0;
    TMR0IE = 1;
    GIE  = 1;
}

unsigned long micros() {
    unsigned long m;
    unsigned char oldSREG = INTCON;
    
    GIE = 0; // Disable Interrupts
    m = micros_counter;
    GIE = 1; // Enable Interrupts (or restore)
    
    // Add Timer Offset
    // We can't safely read TMR0 while it's running and might overflow?
    // Simplified: Just return counter
    return m;
}

unsigned long millis() {
    return micros() / 1000;
}