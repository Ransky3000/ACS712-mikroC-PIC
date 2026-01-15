/* File: ADC_Lib.c */
#include "ADC_Lib.h"

// We need _XTAL_FREQ for __delay_us
#ifndef _XTAL_FREQ
#define _XTAL_FREQ 8000000
#endif

void ADC_Init() {
    // ADCON1 Configuration
    // Bit 7 (ADFM): 1 = Right Justified (Result in 10-bit normal format)
    // Bit 6 (ADCS2): 1 = Fosc/16 (Divider)
    // Bits 5-4 (VCFG): 00 = Vref+ is VDD, Vref- is VSS
    ADCON1 = 0b11000000; 

    // ADCON0 Configuration
    // Bits 7-6 (ADCS1:0): 01 = Fosc/8 ?? Let's check datasheet.
    // Ideally we want Tad between 1.6us and really fast.
    // Let's stick to simplest: defaults often work, but let's be explicit.
    // ADON (Bit 0) = 1 (Turn On)
    ADCON0bits.ADON = 1;
}

unsigned int ADC_Read(unsigned char channel) {
    // 1. Select Channel
    // ADCON0 Bits 5-3 are CHS2:CHS0
    // We clear them first, then OR in the channel shifted by 3
    ADCON0 &= 0b11000111;       // Clear bits 5,4,3
    ADCON0 |= (channel << 3);   // Set channel

    // 2. Acquisition Delay
    // The capacitor needs time to charge to the input voltage.
    // 20us is usually plenty safe.
    __delay_us(20);

    // 3. Start Conversion
    ADCON0bits.GO_nDONE = 1;

    // 4. Wait for Completion (Blocking)
    while (ADCON0bits.GO_nDONE);

    // 5. Return Result (10-bit)
    // ADRESH is high byte, ADRESL is low byte
    return ((unsigned int)ADRESH << 8) + ADRESL;
}
