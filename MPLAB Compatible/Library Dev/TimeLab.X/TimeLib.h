/* File: TimeLib.h */
#ifndef TIMELIB_H
#define TIMELIB_H

#include <xc.h>

// Function Prototypes
// Now accepts the frequency (4 or 8) as an argument
// Changed unsigned short -> unsigned char for XC8 (8-bit)
void Time_Init(unsigned char mhz);

unsigned long micros();
unsigned long millis();

#endif