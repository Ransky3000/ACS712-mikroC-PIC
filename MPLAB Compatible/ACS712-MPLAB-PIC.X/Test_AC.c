/*
  File: Test_AC.c
  Description: Reads AC RMS Current in milliAmps (Integer Version)
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
#include "ACS712.h"
#include "UART_Lib.h"
#include "ADC_Lib.h"
#include "Timer_lib.h"

#define _XTAL_FREQ 8000000

ACS712_t mySensor;

// Custom helper to convert int to string
void IntToText(char* buf, int n) {
    int i = 0;
    int sign = n;
    
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    if (n < 0) n = -n;
    
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    
    if (sign < 0) buf[i++] = '-';
    buf[i] = '\0';
    
    // Reverse string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buf[start];
        buf[start] = buf[end];
        buf[end] = temp;
        start++;
        end--;
    }
}

void main() {
    OSCCON = 0x70;    // 8MHz
    ANSEL = 0b00000001; // AN0 Analog
    TRISAbits.TRISA0 = 1; // Input
    
    UART_Init();
    ADC_Init();
    Time_Init(8); 
    
    UART_Write_Text("ACS712 AC Integer Test\r\n");

    // Init: 5000mV Ref, 100mV/A sensitivity
    ACS712_Init(&mySensor, 0, 5000, 1023); 
    ACS712_SetSensitivity(&mySensor, 100); 
    
    UART_Write_Text("Calibrating...\r\n");
    ACS712_Calibrate(&mySensor);
    unsigned long previousMillis = 0;
    const unsigned long calibrationInterval = 10000; // 5 seconds

    while(1) {
        unsigned long currentMillis = millis();
        
        // Periodic Calibration (Non-blocking check)
        if (currentMillis - previousMillis >= calibrationInterval) {
            previousMillis = currentMillis;
            UART_Write_Text("Recalibrating...\r\n");
            ACS712_Calibrate(&mySensor);
            UART_Write_Text("Done.\r\n");
        }

        unsigned long sum_ma = 0;
        unsigned int ma_val;
        int i;
        
        // Take average of 20 readings
        for(i=0; i<20; i++) {
            sum_ma += ACS712_ReadAC(&mySensor, 60);
        }
        
        ma_val = (unsigned int)(sum_ma / 20);
        
        // Threshold: If below 80mA, force to 0
        if (ma_val < 80) ma_val = 0;
        
        // Format as Amps: X.XXX A
        // Example: 1234 mA -> 1.234 A
        unsigned int amps_whole = ma_val / 1000;
        unsigned int amps_frac = ma_val % 1000;
        char buffer[10];

        UART_Write_Text("Current: ");
        
        // Print Whole Part
        IntToText(buffer, amps_whole);
        UART_Write_Text(buffer);
        
        UART_Write('.');
        
        // Print Fractional Part (Manual leading zeros)
        if (amps_frac < 100) UART_Write('0');
        if (amps_frac < 10) UART_Write('0');
        
        IntToText(buffer, amps_frac);
        UART_Write_Text(buffer);
        
        UART_Write_Text(" A\r\n");
        
        __delay_ms(100); 
    }
}
