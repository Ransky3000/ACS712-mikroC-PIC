/*
 * File:   Communication_protocol_test.c
 * Description: Test Suite for HC-12 Master-Slave Protocol
 * Verify: Packet Construction, Integrity Check, Parsing
 */

#include <xc.h>
#include "HC12-RF_Protocol.h"
#include "Soft_UART.h" // For Debug Output

// Mock UART_Write for testing (if UART_Lib not linked yet)
// In real app, this is in UART_Lib.c
// void UART_Write(char data) { /* HW UART */ }

void main() {
    // 1. Setup Debug
    Soft_UART_Init(&PORTB, 6, 7, 9600, 0); 
    Soft_UART_println("--- Protocol Test Start ---");
    
    // 2. Test Packet Construction
    RF_Packet_t tx_pkt;
    tx_pkt.target_id = 0x02; // Outlet 2
    tx_pkt.sender_id = 0x00; // Master
    tx_pkt.command   = CMD_RELAY_ON;
    tx_pkt.data      = 0x0000; // No data for command
    
    unsigned char frame[PACKET_SIZE];
    RF_Build_Packet(frame, &tx_pkt);
    
    // Verify Frame Content
    if(frame[0] == 0xAA && frame[1] == 0x02 && frame[7] == 0xBB) {
        Soft_UART_println("[PASS] Build Structure");
    } else {
        Soft_UART_println("[FAIL] Build Structure");
    }
    
    // 3. Test Parsing (Simulate Received Data)
    // Case A: Valid Packet (Current Reading: 1234 mA)
    // Data = 1234 (0x04D2)
    // XOR Checksum: 01 ^ 02 ^ 05 ^ 04 ^ D2 = XOR Result
    unsigned char rx_valid[] = {0xAA, 0x01, 0x02, CMD_REPORT_DATA, 0x04, 0xD2, 0x00, 0xBB};
    
    // Manually calc expected CRC
    unsigned char expected_crc = 0x01 ^ 0x02 ^ CMD_REPORT_DATA ^ 0x04 ^ 0xD2; 
    rx_valid[6] = expected_crc; // Inject valid CRC
    
    RF_Packet_t result;
    if(RF_Parse_Packet(rx_valid, &result)) {
        if(result.data == 1234) {
            Soft_UART_println("[PASS] Parse Data Valid");
        } else {
            Soft_UART_println("[FAIL] Data Mismatch");
        }
    } else {
        Soft_UART_println("[FAIL] Parse Rejected Valid Packet");
    }
    
    // 4. Test Error Handling (Corrupt CRC)
    rx_valid[6] = 0xFF; // Bad CRC
    if(!RF_Parse_Packet(rx_valid, &result)) {
        Soft_UART_println("[PASS] Reject Bad CRC");
    } else {
        Soft_UART_println("[FAIL] Accepted Bad CRC!");
    }
    
    Soft_UART_println("--- Test Complete ---");
    while(1);
}
