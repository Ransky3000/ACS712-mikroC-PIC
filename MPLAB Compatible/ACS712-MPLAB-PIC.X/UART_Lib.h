/* File: UART_Lib.h */
#ifndef UART_LIB_H
#define UART_LIB_H

#include <xc.h>

void UART_Init();
void UART_Write(char data);
unsigned char UART_Data_Ready();
char UART_Read();
void UART_Write_Text(char *text);

#endif
