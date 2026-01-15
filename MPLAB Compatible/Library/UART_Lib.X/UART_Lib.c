/* File: UART_Lib.c */
#include "UART_Lib.h"

void UART_Init() {
    // PIC16F88 UART Configuration for 9600 Baud @ 8MHz Fosc
    
    // 1. Set Baud Rate
    // SPBRG = ((Fosc / Baud) / 16) - 1   (if BRGH=1)
    // SPBRG = ((8000000 / 9600) / 16) - 1 = 51.08 -> 51
    BRGH = 1;      // High Speed Baud Rate
    SPBRG = 51;

    // 2. Configure TX/RX Pins
    // RB5 is TX (Output), RB2 is RX (Input)
    TRISBbits.TRISB2 = 1; // RX Input
    TRISBbits.TRISB5 = 0; // TX Output (Manual set, though module handles it)

    // 3. Enable Synchronous Serial Port
    SYNC = 0;      // Asynchronous mode
    SPEN = 1;      // Enable Serial Port

    // 4. Enable Transmission and Reception
    TXEN = 1;      // Enable Transmission
    CREN = 1;      // Enable Continuous Reception
}

void UART_Write(char data) {
    // Wait until Transmit Register is empty (TRMT = 1)
    while(!TRMT);
    TXREG = data;
}

unsigned char UART_Data_Ready() {
    // Return 1 if data is in buffer (Receive Interrupt Flag)
    return RCIF; 
}

char UART_Read() {
    // Wait for data (Blocking)
    while(!RCIF);
    return RCREG;
}

void UART_Write_Text(char *text) {
    int i;
    for(i = 0; text[i] != '\0'; i++) {
        UART_Write(text[i]);
    }
}
