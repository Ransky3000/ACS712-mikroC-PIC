/* 
 * File:   HC12-RF_Protocol.h
 * Author: Antigravity
 * 
 * Description: Master-Slave Polling Protocol for HC-12 Master-Slave Communication
 * Packet Size: 8 Bytes Fixed
 * Frame: [START] [TARGET] [SENDER] [CMD] [DATA_H] [DATA_L] [CRC] [END]
 */

#ifndef HC12_RF_PROTOCOL_H
#define HC12_RF_PROTOCOL_H

#include <xc.h>
#include "UART_Lib.h" // Requires UART Library for transmission

// --- Configuration ---
#define PACKET_SIZE 8
#define SOF_BYTE    0xAA  // Start of Frame
#define EOF_BYTE    0xBB  // End of Frame

// --- Command Codes ---
#define CMD_PING         0x01 // Check if alive
#define CMD_RELAY_ON     0x02 // Turn specific socket ON
#define CMD_RELAY_OFF    0x03 // Turn specific socket OFF
#define CMD_READ_CURRENT 0x04 // Request Current Reading
#define CMD_REPORT_DATA  0x05 // Response with Data (Data=mA)
#define CMD_ACK          0x06 // Acknowledge action

// --- Device IDs ---
#define ID_MASTER   0x00
// Slaves 1..N defined in main config

// --- Data Structure ---
typedef struct {
    unsigned char target_id;
    unsigned char sender_id;
    unsigned char command;
    unsigned int  data;       // 16-bit Data (e.g., Current in mA)
} RF_Packet_t;

// --- Public Functions ---

// build_packet: Creates a byte array from struct
void RF_Build_Packet(unsigned char *buffer, RF_Packet_t *pkt);

// parse_packet: Validates and extracts struct from byte array
// Returns: 1 if valid (CRC OK), 0 if invalid
unsigned char RF_Parse_Packet(unsigned char *buffer, RF_Packet_t *pkt);

#endif
