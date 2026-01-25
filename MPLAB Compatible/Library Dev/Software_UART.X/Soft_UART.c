/* File: Soft_UART.c */
#include "Soft_UART.h"

// --- Configuration ---
#define TX_BUFFER_SIZE 8

// --- Global State Variables ---
volatile unsigned char *suart_port;
unsigned char suart_tx_mask;
unsigned char suart_inverted;
unsigned int  suart_timer_reload;

// TX State
volatile char tx_buffer[TX_BUFFER_SIZE];
volatile unsigned char tx_head = 0;
volatile unsigned char tx_tail = 0;
volatile unsigned char tx_state = 0; // 0=Idle, 1=Start, 2-9=Data, 10=Stop
volatile unsigned char tx_byte = 0;
volatile unsigned char tx_bit_count = 0;

// --- Helper Macros ---
#define TX_PIN_HIGH() do { if(!suart_inverted) *suart_port |= suart_tx_mask; else *suart_port &= ~suart_tx_mask; } while(0)
#define TX_PIN_LOW()  do { if(!suart_inverted) *suart_port &= ~suart_tx_mask; else *suart_port |= suart_tx_mask; } while(0)

void Soft_UART_Init(volatile unsigned char *port, unsigned char rx_pin, unsigned char tx_pin, unsigned long baud_rate, unsigned char inverted) {
    suart_port = port;
    // suart_rx_mask is unused
    suart_tx_mask = (1 << tx_pin);
    suart_inverted = inverted;
    
    // Calculate 8-bit Timer2 Period (PR2)
    // PR2 = (Fosc / (4 * Baud * Prescale)) - 1
    unsigned long ticks = 8000000 / (4 * baud_rate);
    PR2 = ticks - 1; 
    
    // Configure Pins
    volatile unsigned char *tris = port + 0x80; 
    *tris &= ~suart_tx_mask; // Output for TX
    // Rx pin Tris left untouched since we aren't reading
    
    TX_PIN_HIGH(); 
    
    // Setup Timer2 (8-bit with Hardware Period Register)
    T2CONbits.TOUTPS = 0; // Postscaler 1:1
    T2CONbits.T2CKPS = 0; // Prescaler 1:1
    T2CONbits.TMR2ON = 1;
    
    TMR2IE = 1; // Enable Timer2 Interrupt
    PEIE = 1;
    GIE = 1;
}

void Soft_UART_Write(char udata) {
    unsigned char next_head = (tx_head + 1) % TX_BUFFER_SIZE;
    if (next_head != tx_tail) {
        tx_buffer[tx_head] = udata;
        tx_head = next_head;
    }
}

// Read function removed for memory optimization

void Soft_UART_Break() {
    tx_head = tx_tail = 0;
    tx_state = 0;
}

// --- INTERRUPT SERVICE ROUTINE ---
void __interrupt() ISR(void) {
    
    // 1. Timer2 (Baud Rate Generator - Auto Reload!)
    if (TMR2IF) {
        TMR2IF = 0;
        
        // --- TX State Machine ---
        if (tx_state == 0) { // Idle
            if (tx_head != tx_tail) {
                tx_byte = tx_buffer[tx_tail];
                tx_tail = (tx_tail + 1) % TX_BUFFER_SIZE;
                tx_state = 1; // Go to Start
                tx_bit_count = 0;
            }
        }
        else if (tx_state == 1) { // Start
            TX_PIN_LOW();
            tx_state = 2; // Go to Data
        }
        else if (tx_state == 2) { // Data
            if (tx_byte & 0x01) TX_PIN_HIGH(); else TX_PIN_LOW();
            tx_byte >>= 1;
            tx_bit_count++;
            if (tx_bit_count >= 8) tx_state = 3; // Go to Stop
        }
        else if (tx_state == 3) { // Stop
            TX_PIN_HIGH();
            tx_state = 0; // Back to Idle
        }
    }
}
