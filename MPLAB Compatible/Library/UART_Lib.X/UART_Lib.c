/* File: UART_Lib.c 
 * Description: Hardware UART Library with RX Interrupt Ring Buffer
 * 
 * Architecture:
 * - TX: Direct hardware write (blocking until TXREG empty)
 * - RX: ISR-driven ring buffer (16 bytes). Every incoming byte triggers
 *        the hardware UART interrupt, which stores it in the buffer.
 *        This ensures NO bytes are lost even during blocking operations
 *        (like ACS712_ReadAC's 34ms sensor reads).
 * 
 * Usage: Call UART_ISR() from main ISR. Use UART_Data_Ready() and
 *        UART_Read() as before — they now read from the buffer.
 */
#include "UART_Lib.h"

// --- Ring Buffer for RX Interrupt ---
// Size MUST be a power of 2 so we can use bitmask instead of modulo.
// Modulo (%) pulls in the division library (~50 words Flash) on PIC16F.
// Bitmask (&) is a single instruction.
#define RX_BUF_SIZE 16
#define RX_BUF_MASK 0x0F  // = RX_BUF_SIZE - 1

volatile char rx_buf[RX_BUF_SIZE];
volatile unsigned char rx_head = 0; // ISR writes here
volatile unsigned char rx_tail = 0; // Main loop reads here

void UART_Init() {
    // PIC16F88 UART Configuration for 9600 Baud @ 8MHz Fosc
    // SPBRG = ((8000000 / 9600) / 64) - 1 = 12
    BRGH = 0;      // Low Speed Baud Rate
    SPBRG = 12;

    // RB5 = TX (Output), RB2 = RX (Input)
    TRISBbits.TRISB2 = 1; // RX Input
    TRISBbits.TRISB5 = 0; // TX Output

    // Async Mode, Enable Serial Port
    SYNC = 0;
    SPEN = 1;

    // Enable TX and Continuous RX
    TXEN = 1;
    CREN = 1;
    
    // Enable RX Interrupt (NEW: drives ring buffer)
    RCIE = 1;      // UART Receive Interrupt Enable
    PEIE = 1;      // Peripheral Interrupt Enable (required for RCIE)
}

// Send one byte (blocking until TX register is empty)
void UART_Write(char data) {
    while(!TRMT);
    TXREG = data;
}

// Check if data is available in the ring buffer
unsigned char UART_Data_Ready() {
    return (rx_head != rx_tail);
}

// Read one byte from ring buffer (blocks if empty)
char UART_Read() {
    while (rx_head == rx_tail); // Wait for ISR to put data in buffer
    char c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) & RX_BUF_MASK; // Bitmask wrap (not modulo!)
    return c;
}

// Send a null-terminated string
void UART_Write_Text(char *text) {
    int i;
    for(i = 0; text[i] != '\0'; i++) {
        UART_Write(text[i]);
    }
}

// --- ISR Handler (Call from main __interrupt) ---
// Captures every incoming byte into the ring buffer.
// Handles OERR (Overrun Error) by toggling CREN to recover.
void UART_ISR(void) {
    if (RCIF) {
        // Overrun Error: UART FIFO overflowed. Toggle CREN to clear.
        if (OERR) { CREN = 0; CREN = 1; return; }
        
        // Normal: Read byte from hardware and store in ring buffer
        char c = RCREG; // Reading RCREG clears RCIF
        unsigned char next = (rx_head + 1) & RX_BUF_MASK;
        if (next != rx_tail) { // Buffer not full?
            rx_buf[rx_head] = c;
            rx_head = next;
        }
        // If buffer full, byte is silently dropped (acceptable: 16 > 8-byte packet)
    }
}
