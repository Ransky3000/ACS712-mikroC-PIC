/* File: Timer_lib.h */
#ifndef TIMER_LIB_H
#define TIMER_LIB_H

#include <xc.h>

void Time_Init(unsigned char mhz);
unsigned long micros();
unsigned long millis();

#endif