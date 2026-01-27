/*
 * File:   HC12-RF_Protocol.c
 * Author: Antigravity
 * 
 * Description: Implementation of 8-Byte Fixed Frame Protocol
 */

#include "HC12-RF_Protocol.h"

#include "HC12-RF_Protocol.h"

// Helper: Internal Checksum Calculation (Unrolled for Speed/Size)
unsigned char RF_Calculate_Checksum(RF_Packet_t *pkt) {
    unsigned char xor_sum = 0;
    // Checksum covers bytes 1 to 5 (Target to Data_L)
    xor_sum ^= pkt->frame[1];
    xor_sum ^= pkt->frame[2];
    xor_sum ^= pkt->frame[3];
    xor_sum ^= pkt->frame[4];
    xor_sum ^= pkt->frame[5];
    return xor_sum;
}

void RF_Init_Packet(RF_Packet_t *pkt) {
    // Only clear logic fields. SOF/EOF handled by Sign/Verify
    pkt->fields.target_id = 0;
    pkt->fields.sender_id = 0;
    pkt->fields.command = 0;
    pkt->fields.data_h = 0;
    pkt->fields.data_l = 0;
    pkt->fields.checksum = 0;
}

void RF_Sign_Packet(RF_Packet_t *pkt) {
    pkt->fields.sof = SOF_BYTE; // Ensure Framing
    pkt->fields.eof = EOF_BYTE;
    pkt->fields.checksum = RF_Calculate_Checksum(pkt);
}

unsigned char RF_Verify_Packet(RF_Packet_t *pkt) {
    // 1. Check Framing
    if (pkt->fields.sof != SOF_BYTE || pkt->fields.eof != EOF_BYTE) {
        return 0; // Malformed
    }
    
    // 2. Check Integrity
    unsigned char calc_crc = RF_Calculate_Checksum(pkt);
    if (pkt->fields.checksum != calc_crc) {
        return 0; // Corrupted
    }
    
    return 1; // Valid
}
