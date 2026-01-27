/*
 * File:   Communication_protocol_test.c
 * Description: Test Suite for HC-12 Master-Slave Protocol
 * Verify: Packet Construction, Integrity Check, Parsing
 */

// CONFIG1
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (Internal oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RA5/MCLR/VPP Pin Function Select bit (MCLR enabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable bit (RB3 is digital I/O)
#pragma config CPD = OFF        // Data EE Memory Code Protection bit (Code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off)
#pragma config CCPMX = RB0      // CCP1 Pin Selection bit (CCP1 on RB0)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

// CONFIG2
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal External Switchover mode disabled)

#include <xc.h>
#include "HC12-RF_Protocol.h"
#include "Soft_UART.h" // For Debug Output
#include "Timer_lib.h"   // Changed from Timer_lib.h to match Library Dev

// Mock UART_Write for testing (if UART_Lib not linked yet)
// In real app, this is in UART_Lib.c
// void UART_Write(char data) { /* HW UART */ }

// --- Central Interrupt Service Routine ---
void __interrupt() ISR(void) {
    // Call Library ISRs
    Soft_UART_ISR();
    Timer_ISR();
}

void main() {
    // 1. Setup Oscillator (8MHz)
    OSCCON = 0b01110000;
    while(!OSCCONbits.IOFS); // Wait for stable

    // 2. Setup Libraries
    Time_Init(8); // Init Timer0
    Soft_UART_Init(&PORTB, 6, 7, 9600, 0); 
    Soft_UART_println("--- Protocol Test Start ---");
    
    // 2. Test Packet Construction
    RF_Packet_t tx_pkt;
    RF_Init_Packet(&tx_pkt);
    
    tx_pkt.fields.target_id = 0x02; // Outlet 2
    tx_pkt.fields.sender_id = 0x00; // Master
    tx_pkt.fields.command   = CMD_RELAY_ON;
    RF_Set_Data(&tx_pkt, 0x0000);   // No data
    
    // Finalize (Calc CRC)
    RF_Sign_Packet(&tx_pkt);
    
    // Verify Frame Content directly (Zero Copy!)
    if(tx_pkt.fields.sof == 0xAA && tx_pkt.fields.target_id == 0x02 && tx_pkt.fields.eof == 0xBB) {
        Soft_UART_println("[PASS] Build Structure");
    } else {
        Soft_UART_println("[FAIL] Build Structure");
    }
    
    // 3. Test Parsing (Simulate Received Data)
    // Case A: Valid Packet (Current Reading: 1234 mA)
    RF_Packet_t rx_pkt;
    
    // Manually Fill Buffer to simulate reception
    rx_pkt.frame[0] = 0xAA;
    rx_pkt.frame[1] = 0x01; // Target
    rx_pkt.frame[2] = 0x02; // Sender
    rx_pkt.frame[3] = CMD_REPORT_DATA;
    rx_pkt.frame[4] = 0x04; // 1234 >> 8
    rx_pkt.frame[5] = 0xD2; // 1234 & 0xFF
    // frame[6] calc below
    rx_pkt.frame[7] = 0xBB;
    
    // Inject valid CRC
    rx_pkt.fields.checksum = 0x01 ^ 0x02 ^ CMD_REPORT_DATA ^ 0x04 ^ 0xD2; 
    
    if(RF_Verify_Packet(&rx_pkt)) {
        if(RF_Get_Data(&rx_pkt) == 1234) {
            Soft_UART_println("[PASS] Parse Data Valid");
        } else {
            Soft_UART_println("[FAIL] Data Mismatch");
        }
    } else {
        Soft_UART_println("[FAIL] Parse Rejected Valid Packet");
    }
    
    // 4. Test Error Handling (Corrupt CRC)
    rx_pkt.fields.checksum = 0xFF; // Bad CRC logic
    
    if(!RF_Verify_Packet(&rx_pkt)) {
        Soft_UART_println("[PASS] Reject Bad CRC");
    } else {
        Soft_UART_println("[FAIL] Accepted Bad CRC!");
    }
    
    Soft_UART_println("--- Test Complete ---");
    while(1);
}
