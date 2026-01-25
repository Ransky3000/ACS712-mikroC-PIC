/* File: Test_main.c */
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

#define _XTAL_FREQ 8000000

void main() {
    OSCCON = 0x70; // 8MHz
    ANSEL = 0;     // Digital
    
    // Init Soft UART
    // PORTB, RX=RB4, TX=RB1, 9600 Baud, Normal
    Soft_UART_Init(&PORTB, 6, 7, 9600, 0); 
    
    while(1) {
        Soft_UART_Write('H');
        Soft_UART_Write('i');
        Soft_UART_Write('\r');
        Soft_UART_Write('\n');
        
        __delay_ms(300);
        

    }
}
