/*
  File: 2_ADC_with_Soft_UART_and_HARD_UART_test.c
  Description: 
    - Reads AN0 -> Sends via Software UART (TX=RB4)
    - Reads AN1 -> Sends via Hardware UART (TX=RB5)
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
#include "UART_Lib.h"
#include "ADC_Lib.h"

#define _XTAL_FREQ 8000000

// Helper: Integer to String
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
    ANSEL = 0b00000011; // AN0 and AN1 Analog
    
    // Set Inputs
    TRISAbits.TRISA0 = 1; // AN0
    TRISAbits.TRISA1 = 1; // AN1
    
    // 1. Init Hardware UART (Standard 9600)
    UART_Init();
    
    // 2. Init Software UART 
    // schematic: RB4 is connected to Soft_UART RX terminal (so RB4 is TX)
    // We pick RB6 as dummy RX pin since we are only sending.
    // Soft_UART_Init(Port, RxPin, TxPin, Baud, Inverted)
    Soft_UART_Init(&PORTB, 6, 7, 9600, 0);
    
    // 3. Init ADC
    ADC_Init();
    
    UART_Write_Text("Hardware UART Ready\r\n");
    Soft_UART_Write_Text("Software UART Ready\r\n");
    
    while(1) {
        unsigned int val0, val1;
        char buf0[10];
        char buf1[10];
        
        // --- AN0 -> Software UART ---
        val0 = ADC_Read(0);
        IntToText(buf0, val0);
        
        Soft_UART_Write_Text("AN0: ");
        Soft_UART_Write_Text(buf0);
        Soft_UART_Write_Text("\r\n");
        
        // --- AN1 -> Hardware UART ---
        val1 = ADC_Read(1);
        IntToText(buf1, val1);
        
        UART_Write_Text("AN1: ");
        UART_Write_Text(buf1);
        UART_Write_Text("\r\n");
        
        __delay_ms(300);
    }
}
