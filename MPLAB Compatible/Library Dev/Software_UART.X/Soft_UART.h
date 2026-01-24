/* File: Soft_UART.h */
#ifndef SOFT_UART_H
#define SOFT_UART_H

#include <xc.h>

// Initialize Soft UART (Non-Blocking)
// port: Address of PORT register (e.g. &PORTB)
// rx_pin: bit number (0-7)
// tx_pin: bit number (0-7)
// baud_rate: e.g. 9600
// inverted: 0=Normal, 1=Inverted
void Soft_UART_Init(volatile unsigned char *port, unsigned char rx_pin, unsigned char tx_pin, unsigned long baud_rate, unsigned char inverted);

// Read byte (Non-blocking)
// Returns data. Sets *error to 0 (Success), 1 (No Data), 2 (Overflow)
char Soft_UART_Read(char *error);

// Write byte (Non-blocking / Buffered)
void Soft_UART_Write(char udata);

// Reset State
void Soft_UART_Break();

#endif
