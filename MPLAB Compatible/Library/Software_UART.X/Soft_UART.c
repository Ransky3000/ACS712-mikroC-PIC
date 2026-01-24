/* File: Soft_UART.c */
#include "Soft_UART.h"

// --- Configuration ---
#define TX_BUFFER_SIZE 16
#define RX_BUFFER_SIZE 16

// --- Global State Variables ---
volatile unsigned char *suart_port;
unsigned char suart_rx_mask;
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

// RX State
volatile char rx_buffer[RX_BUFFER_SIZE];
volatile unsigned char rx_head = 0;
volatile unsigned char rx_tail = 0;
volatile unsigned char rx_state = 0; // 0=Idle, 1=Start, 2=Bits
volatile unsigned char rx_byte = 0;
volatile unsigned char rx_bit_count = 0;

// --- Helper Macros ---
#define TX_PIN_HIGH() do { if(!suart_inverted) *suart_port |= suart_tx_mask; else *suart_port &= ~suart_tx_mask; } while(0)
#define TX_PIN_LOW()  do { if(!suart_inverted) *suart_port &= ~suart_tx_mask; else *suart_port |= suart_tx_mask; } while(0)
#define RX_PIN_READ() ((*suart_port & suart_rx_mask) ? !suart_inverted : suart_inverted)

void Soft_UART_Init(volatile unsigned char *port, unsigned char rx_pin, unsigned char tx_pin, unsigned long baud_rate, unsigned char inverted) {
    suart_port = port;
    suart_rx_mask = (1 << rx_pin);
    suart_tx_mask = (1 << tx_pin);
    suart_inverted = inverted;
    
    // Calculate 8-bit Timer2 Period (PR2)
    // T2 uses Fosc/4. We need a prescaler to fit in 8 bytes if needed.
    // 9600 Baud @ 8MHz:
    // Bit Time = 104.16 us
    // Inst Cyc = 0.5 us (2MHz)
    // Ticks = 208.33. 
    // 208 < 256. So Prescaler 1:1 is fine.
    
    // PR2 = (Fosc / (4 * Baud * Prescale)) - 1
    unsigned long ticks = 8000000 / (4 * baud_rate);
    PR2 = ticks - 1; 
    
    // Configure Pins
    volatile unsigned char *tris = port + 0x80; 
    *tris |= suart_rx_mask;  
    *tris &= ~suart_tx_mask; 
    
    TX_PIN_HIGH(); 
    
    // Setup Timer2 (8-bit with Hardware Period Register - DRIFT FREE!)
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

char Soft_UART_Read(char *error) {
    if (rx_head == rx_tail) {
        *error = 1; 
        return 0;
    }
    char data = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    *error = 0; 
    return data;
}

void Soft_UART_Break() {
    tx_head = tx_tail = 0;
    rx_head = rx_tail = 0;
    tx_state = 0;
    RX_PIN_READ(); 
    RBIF = 0;
}

// --- INTERRUPT SERVICE ROUTINE ---
void __interrupt() ISR(void) {
    
    // 1. Timer2 (Baud Rate Generator - Auto Reload!)
    if (TMR2IF) {
        TMR2IF = 0;
        // No manual reload needed! TMR2 resets to 0 automatically.
        
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
    
    // 2. RX Interrupt
    if (RBIF) {
        unsigned char dummy = PORTB; 
        RBIF = 0;
    }
}
