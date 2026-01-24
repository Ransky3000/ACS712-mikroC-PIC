/*
  File: ADC_and_Software_UART_test.c
  Description: Reads ADC (AN0) and sends value via Non-Blocking Soft UART.
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
#include "Soft_UART.h"
#include "ADC_Lib.h"

#define _XTAL_FREQ 8000000

// Helper: Convert unsigned int to string
void IntToText(char* buf, unsigned int n) {
    int i = 0;
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (n > 0) {
        buf[i++] = (char)((n % 10) + '0');
        n /= 10;
    }
    buf[i] = '\0';
    // Reverse
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

void Soft_UART_Write_Text(char *text) {
    int i;
    for(i = 0; text[i] != '\0'; i++) {
        Soft_UART_Write(text[i]);
    }
}

void main() {
    OSCCON = 0x70;    // 8MHz
    ANSEL = 0b00000001; // AN0 Analog
    TRISAbits.TRISA0 = 1; // Input
    
    // Init Soft UART: PORTB, RX=RB6, TX=RB7, 9600 Baud
    // (Using RB6/RB7 to match your previous test)
    Soft_UART_Init(&PORTB, 6, 7, 9600, 0);
    
    // Init ADC
    ADC_Init();
    
    Soft_UART_Write_Text("ADC + SoftUART Test\r\n");

    while(1) {
        unsigned int val;
        char buffer[12];
        
        // Read ADC (Blocking ~20us)
        val = ADC_Read(0);
        
        // Convert to text
        IntToText(buffer, val);
        
        // Print (Non-Blocking / Buffered)
        Soft_UART_Write_Text("ADC: ");
        Soft_UART_Write_Text(buffer);
        Soft_UART_Write_Text("\r\n");
        
        __delay_ms(500); 
    }
}
