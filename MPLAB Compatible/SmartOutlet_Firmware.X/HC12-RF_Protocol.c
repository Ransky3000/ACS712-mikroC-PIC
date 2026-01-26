/*
 * File:   HC12-RF_Protocol.c
 * Author: Antigravity
 * 
 * Description: Implementation of 8-Byte Fixed Frame Protocol
 */

#include "HC12-RF_Protocol.h"
#include "UART_Lib.h" // Requires UART Library for transmission

// Helper: Calculate simple Checksum (XOR of payload)
unsigned char RF_Calculate_Checksum(unsigned char *buf) {
    unsigned char xor_sum = 0;
    // Checksum covers: Target, Sender, Cmd, Data_H, Data_L (Bytes 1 to 5)
    // Frame: [SOF] [TGT] [SND] [CMD] [DH] [DL] [CRC] [EOF]
    // Index:   0     1     2     3    4    5     6     7
    
    xor_sum ^= buf[1];
    xor_sum ^= buf[2];
    xor_sum ^= buf[3];
    xor_sum ^= buf[4];
    xor_sum ^= buf[5];
    
    return xor_sum;
}

void RF_Build_Packet(unsigned char *buffer, RF_Packet_t *pkt) {
    buffer[0] = SOF_BYTE;
    buffer[1] = pkt->target_id;
    buffer[2] = pkt->sender_id;
    buffer[3] = pkt->command;
    buffer[4] = (pkt->data >> 8) & 0xFF; // Data High
    buffer[5] = (pkt->data) & 0xFF;      // Data Low
    
    buffer[6] = RF_Calculate_Checksum(buffer);
    buffer[7] = EOF_BYTE;
}

unsigned char RF_Parse_Packet(unsigned char *buffer, RF_Packet_t *pkt) {
    // 1. Check Framing
    if (buffer[0] != SOF_BYTE || buffer[7] != EOF_BYTE) {
        return 0; // Malformed Packet
    }
    
    // 2. Check Integrity (Checksum)
    unsigned char calc_crc = RF_Calculate_Checksum(buffer);
    if (buffer[6] != calc_crc) {
        return 0; // Data Corrupted
    }
    
    // 3. Extract Data
    pkt->target_id = buffer[1];
    pkt->sender_id = buffer[2];
    pkt->command   = buffer[3];
    pkt->data      = (buffer[4] << 8) | buffer[5];
    
    return 1; // Valid Packet
}
