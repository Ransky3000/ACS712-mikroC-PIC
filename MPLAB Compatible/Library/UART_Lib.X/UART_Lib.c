/* File: UART_Lib.c */
#include "UART_Lib.h"

// --- Ring Buffer for RX Interrupt ---
#define RX_BUF_SIZE 16
volatile char rx_buf[RX_BUF_SIZE];
volatile unsigned char rx_head = 0; // ISR writes here
volatile unsigned char rx_tail = 0; // Main loop reads here

void UART_Init() {
    // PIC16F88 UART Configuration for 9600 Baud @ 8MHz Fosc
    
    // 1. Set Baud Rate
    // SPBRG = ((Fosc / Baud) / 64) - 1   (if BRGH=0)
    // SPBRG = ((8000000 / 9600) / 64) - 1 = 12.02 -> 12
    BRGH = 0;      // Low Speed Baud Rate
    SPBRG = 12;

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
    
    // 5. Enable RX Interrupt (NEW)
    RCIE = 1;      // UART Receive Interrupt Enable
    PEIE = 1;      // Peripheral Interrupt Enable
}

void UART_Write(char data) {
    // Wait until Transmit Register is empty (TRMT = 1)
    while(!TRMT);
    TXREG = data;
}

unsigned char UART_Data_Ready() {
    // Check ring buffer instead of hardware RCIF
    return (rx_head != rx_tail);
}

char UART_Read() {
    // Wait for data in ring buffer
    while (rx_head == rx_tail);
    char c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    return c;
}

void UART_Write_Text(char *text) {
    int i;
    for(i = 0; text[i] != '\0'; i++) {
        UART_Write(text[i]);
    }
}

// --- ISR Handler (Call from main ISR) ---
void UART_ISR(void) {
    if (RCIF) {
        // Handle Overrun Error: Toggle CREN to clear
        if (OERR) {
            CREN = 0;
            CREN = 1;
            return;
        }
        // Handle Framing Error: Read and discard
        if (FERR) {
            volatile char discard = RCREG;
            return;
        }
        // Normal byte: Store in ring buffer
        char c = RCREG; // Clears RCIF
        unsigned char next = (rx_head + 1) % RX_BUF_SIZE;
        if (next != rx_tail) { // Buffer not full
            rx_buf[rx_head] = c;
            rx_head = next;
        }
        // If buffer full, byte is silently dropped
    }
}
