/* File: UART_Lib.h */
#ifndef UART_LIB_H
#define UART_LIB_H

#include <xc.h>

// Initialize UART for 9600 baud @ 8MHz
// Note: Hardcoded for Demo Simplicity
void UART_Init();

// Send a single character
void UART_Write(char data);

// Check if data is available to read
unsigned char UART_Data_Ready();

// Read a single character (Blocking)
char UART_Read();

// Send a string
void UART_Write_Text(char *text);

#endif
