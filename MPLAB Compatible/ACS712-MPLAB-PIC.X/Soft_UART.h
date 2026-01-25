/* * File:   Soft_UART.h
 * Author: Ransky
 * Description: Non-Blocking Software UART Headers
 */

#ifndef SOFT_UART_H
#define SOFT_UART_H

#include <xc.h>

// --- Public Function Prototypes ---

// Init: Sets up Pins, Timer2, and Interrupts
// Note: 'port' should be the address of the PORT register (e.g., &PORTB)
void Soft_UART_Init(volatile unsigned char *port, unsigned char rx_pin, unsigned char tx_pin, unsigned long baud_rate, unsigned char inverted);



// Write: Adds char to TX buffer
void Soft_UART_Write(char data);

// Write Text: Sends a string
void Soft_UART_print(char *text);

// Println: Sends text followed by Newline
void Soft_UART_println(char *text);

// (Internal ISR handles transmission)

#endif