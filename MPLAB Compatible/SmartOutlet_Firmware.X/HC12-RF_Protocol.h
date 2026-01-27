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
// --- Data Structure (Zero-Copy Union) ---
typedef union {
    struct {
        unsigned char sof;       // Start of Frame (0xAA)
        unsigned char target_id;
        unsigned char sender_id;
        unsigned char command;
        unsigned char data_h;    // Data High Byte
        unsigned char data_l;    // Data Low Byte
        unsigned char checksum;  // XOR Sum
        unsigned char eof;       // End of Frame (0xBB)
    } fields;
    unsigned char frame[PACKET_SIZE]; // Array View
} RF_Packet_t;

// --- Public Functions ---

// RF_Init_Packet: Sets SOF/EOF and zeroes logic fields
void RF_Init_Packet(RF_Packet_t *pkt);

// RF_Set_Data: Macro for Zero-Overhead assignment
#define RF_Set_Data(pkt, value) do { \
    (pkt)->fields.data_h = ((value) >> 8) & 0xFF; \
    (pkt)->fields.data_l = (value) & 0xFF; \
} while(0)

// RF_Get_Data: Macro for Zero-Overhead retrieval
#define RF_Get_Data(pkt) \
    ((unsigned int)(((pkt)->fields.data_h << 8) | (pkt)->fields.data_l))

// RF_Sign_Packet: Calculates and sets the Checksum (Finalize before sending)
void RF_Sign_Packet(RF_Packet_t *pkt);

// RF_Verify_Packet: Checks SOF, EOF, and Checksum
// Returns: 1 if valid, 0 if invalid
unsigned char RF_Verify_Packet(RF_Packet_t *pkt);

#endif
