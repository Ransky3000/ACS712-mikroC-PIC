/*
 * File:   Detect_overload_and_control_relay.c
 * Author: Antigravity
 *
 * Description: 
 * PROTOTYPE Firmware to test Overload Protection Logic.
 * - Monitors ACS712 sensors (Socket A & B)
 * - If Current > 3.00A (3000mA), Relay is turned OFF immediately.
 * - Prints status to Soft_UART for debugging.
 *
 * NOTE: This is a standalone test file. Logic will be merged to __main__.c later.
 */

// --- CONFIGURATION (Same as __main__.c) ---
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits
#pragma config WDTE = OFF       // Watchdog Timer
#pragma config PWRTE = OFF      // Power-up Timer
#pragma config MCLRE = ON       // MCLR Pin Function
#pragma config BOREN = OFF      // Brown-out Reset
#pragma config LVP = OFF        // Low-Voltage Programming
#pragma config CPD = OFF        // Data EE Code Protection
#pragma config WRT = OFF        // Flash Write Enable
#pragma config CCPMX = RB0      // CCP1 Pin
#pragma config CP = OFF         // Flash Code Protection

// CONFIG2
#pragma config FCMEN = OFF     
#pragma config IESO = OFF       

#include <xc.h>
#include "UART_Lib.h"
#include "Soft_UART.h"
#include "Timer_lib.h"
#include "ADC_Lib.h"
#include "ACS712.h"

// --- Hardware Definitions ---
#define _XTAL_FREQ 8000000

#define RELAY_A_PIN RA2
#define RELAY_B_PIN RA3

// --- Constants ---
#define OVERLOAD_THRESHOLD_MA 7000 // 7.00 Amps

// --- Globals ---
ACS712_t sensorA;
ACS712_t sensorB;

// --- Helper Functions ---
void print_int_to_soft_uart(unsigned int val);

// --- ISR ---
void __interrupt() ISR(void) {
    Timer_ISR(); // For millis()
    Soft_UART_ISR();
}

void main() {
    // 1. Oscillator Setup
    OSCCON = 0b01110000; // 8MHz
    while(!OSCCONbits.IOFS); 

    // 2. Pin Setup
    ANSEL = 0b00000011; // AN0, AN1 Analog (Sensors)
    
    TRISA = 0b00000011; // RA0, RA1 Input (Sensors), Others Output
    TRISB = 0b00000100; // RB2(RX) Input, RB5(TX) Output - Soft UART
    
    // Init Relays OFF (Active Low -> High = OFF)
    RELAY_A_PIN = 1; 
    RELAY_B_PIN = 1;
    
    // 3. Init Libraries
    UART_Init(); 
    Soft_UART_Init(&PORTB, 6, 7, 9600, 0); 
    Time_Init(8);   
    ADC_Init();     

    // 4. Init Sensors
    ACS712_Init(&sensorA, 0, 5000, 1023); 
    ACS712_Init(&sensorB, 1, 5000, 1023); 

    // Sensitivity (100mV/A for 20A Module)
    ACS712_SetSensitivity(&sensorA, 100); 
    ACS712_SetSensitivity(&sensorB, 100); 
    
    Soft_UART_println("--- Overload Protection Prototype (Non-Blocking) ---");
    Soft_UART_println("Calibrating Sensors...");
    
    ACS712_Calibrate(&sensorA);
    ACS712_Calibrate(&sensorB);
    
    Soft_UART_println("Ready. Turning Relays ON in 0.5s...");
    __delay_ms(500);
    
    // Turn Relays ON
    RELAY_A_PIN = 0;
    RELAY_B_PIN = 0;
    Soft_UART_println("Relays ON. Monitoring...");

    unsigned int currentA = 0;
    unsigned int currentB = 0;
    
    // Overload State Tracking
    unsigned char isOverloadedA = 0;
    unsigned long cooldownStartA = 0;
    
    unsigned char isOverloadedB = 0;
    unsigned long cooldownStartB = 0;

    unsigned long last_print = 0;
    
    while(1) {
        // --- SOCKET A LOGIC ---
        if (isOverloadedA) {
            // In Cool Down
            if (millis() - cooldownStartA >= 5000) {
                // Cool down over -> Retry
                isOverloadedA = 0;
                RELAY_A_PIN = 0; // Turn ON
                Soft_UART_println("Socket A: Cooldown Complete. Retrying...");
            }
        } else {
            // Normal Operation
            currentA = ACS712_ReadAC(&sensorA, 60);
            
            // Check
            if (currentA > OVERLOAD_THRESHOLD_MA) {
                RELAY_A_PIN = 1; // OFF
                isOverloadedA = 1;
                cooldownStartA = millis();
                
                Soft_UART_println("!!! OVERLOAD Socket A !!!");
                Soft_UART_print("Val: "); print_int_to_soft_uart(currentA); Soft_UART_println("mA");
                Soft_UART_println("Action: Cutoff + 5s Cooldown");
            }
        }

        // --- SOCKET B LOGIC ---
        if (isOverloadedB) {
            // In Cool Down
            if (millis() - cooldownStartB >= 5000) {
                // Cool down over -> Retry
                isOverloadedB = 0;
                RELAY_B_PIN = 0; // Turn ON
                Soft_UART_println("Socket B: Cooldown Complete. Retrying...");
            }
        } else {
            // Normal Operation
            currentB = ACS712_ReadAC(&sensorB, 60);
            
            // Check
            if (currentB > OVERLOAD_THRESHOLD_MA) {
                RELAY_B_PIN = 1; // OFF
                isOverloadedB = 1;
                cooldownStartB = millis();
                
                Soft_UART_println("!!! OVERLOAD Socket B !!!");
                 Soft_UART_print("Val: "); print_int_to_soft_uart(currentB); Soft_UART_println("mA");
                Soft_UART_println("Action: Cutoff + 5s Cooldown");
            }
        }
        
        // --- NON-BLOCKING DISPLAY (1Hz) ---
        if (millis() - last_print >= 1000) {
            last_print = millis();
            
            Soft_UART_print("A: ");
            if (isOverloadedA) {
                Soft_UART_print("OVERLOAD");
            } else {
                print_int_to_soft_uart(currentA);
                Soft_UART_print(" mA");
            }
            
            Soft_UART_print(" | B: ");
            if (isOverloadedB) {
                Soft_UART_print("OVERLOAD");
            } else {
                print_int_to_soft_uart(currentB);
                Soft_UART_print(" mA");
            }
            Soft_UART_println("");
        }
    }
}

void print_int_to_soft_uart(unsigned int val) {
    if(val == 0) {
        Soft_UART_Write('0');
        return;
    }
    char buffer[6];
    int i = 0;
    while(val > 0) {
        buffer[i++] = (val % 10) + '0';
        val /= 10;
    }
    while(--i >= 0) {
        Soft_UART_Write(buffer[i]);
    }
}
