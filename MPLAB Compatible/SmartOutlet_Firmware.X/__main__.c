/*
 * File:   __main__.c
 * Description: Merged Firmware - Chunk 1 (Relay Control & Basic Framework)
 * 
 * Features active in this chunk:
 * 1. Pin Initialization (Relays, UART)
 * 2. HC12 Packet Parsing
 * 3. Relay Logic (Active Low)
 * 4. Debug Keys ('1'-'4' only)
 * 
 * NOTE: Sensors are NOT initialized yet (Chunk 2).
 */

// CONFIG1
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
#include "HC12-RF_Protocol.h"

// --- Hardware Definitions ---
#define _XTAL_FREQ 8000000

#define RELAY_A_PIN RA2
#define RELAY_B_PIN RA3

// --- ID Configuration ---
#define DEVICE_ID   0xFE
#define SOCKET_A    1
#define SOCKET_B    2

// --- Helper Prototypes ---
void Process_Command(RF_Packet_t *pkt);
void Send_ACK(unsigned char target, unsigned char cmd);
void Process_Debug_Shortcut(char key);

// --- ISR ---
void __interrupt() ISR(void) {
    // Timer0 will be added in Chunk 2 for Sensors
    Soft_UART_ISR();
}

void main() {
    // 1. Oscillator Setup
    OSCCON = 0b01110000; // 8MHz
    while(!OSCCONbits.IOFS); 

    // 2. Pin Setup
    ANSEL = 0; // Digital I/O (For now - Chunk 1)
    
    TRISA = 0b00000000; // All Output (Relays on RA2, RA3)
    TRISB = 0b00000100; // RB2(RX) Input, RB5(TX) Output
    
    // Init Relays OFF (Active Low -> High)
    RELAY_A_PIN = 1;
    RELAY_B_PIN = 1;
    
    // 3. Init Libraries
    UART_Init();
    Soft_UART_Init(&PORTB, 6, 7, 9600, 0); 
    
    Soft_UART_println("--- Merged Firmware: Chunk 1 ---");
    Soft_UART_println("Relay Control ONLY");
    Soft_UART_println("Keys: 1-4 Active");
    
    // 4. Rx State Machine
    RF_Packet_t rx_pkt;
    unsigned char rx_idx = 0;
    
    while(1) {
        if (UART_Data_Ready()) {
            char byte = UART_Read();
            
            // --- SIMULATION MODE ---
            // Keys 1-4 for Relays
            if (byte >= '1' && byte <= '4') {
                Process_Debug_Shortcut(byte);
                continue; 
            }
            // -----------------------
            
            // Sync
            if (rx_idx == 0 && byte != SOF_BYTE) continue; 
            
            rx_pkt.frame[rx_idx++] = byte;
            
            // Full Packet?
            if (rx_idx >= PACKET_SIZE) {
                if (RF_Verify_Packet(&rx_pkt)) {
                    Process_Command(&rx_pkt);
                } else {
                    Soft_UART_println("Err: Bad CRC");
                }
                rx_idx = 0;
            }
        }
    }
}

void Process_Command(RF_Packet_t *pkt) {
    // Only process for ME or Broadcast
    if (pkt->fields.target_id != DEVICE_ID) return;
    
    unsigned char socket = (unsigned char)(pkt->fields.data_l & 0xFF);
    
    switch (pkt->fields.command) {
        case CMD_PING:
            Send_ACK(pkt->fields.sender_id, CMD_PING);
            break;
            
        case CMD_RELAY_ON:
            if (socket == SOCKET_A)      RELAY_A_PIN = 0; // Active Low
            else if (socket == SOCKET_B) RELAY_B_PIN = 0;
            Send_ACK(pkt->fields.sender_id, CMD_RELAY_ON);
            break;
            
        case CMD_RELAY_OFF:
            if (socket == SOCKET_A)      RELAY_A_PIN = 1; 
            else if (socket == SOCKET_B) RELAY_B_PIN = 1;
            Send_ACK(pkt->fields.sender_id, CMD_RELAY_OFF);
            break;
            
        case CMD_READ_CURRENT:
            // Placeholder for Chunk 2
            Soft_UART_println("CMD_READ: Not Implemented in Chunk 1");
            break;
            
        default:
            break;
    }
}

void Send_ACK(unsigned char target, unsigned char cmd) {
    RF_Packet_t tx;
    RF_Init_Packet(&tx);
    
    tx.fields.target_id = target;       
    tx.fields.sender_id = DEVICE_ID;    
    tx.fields.command   = CMD_ACK;   
    RF_Set_Data(&tx, cmd);
    RF_Sign_Packet(&tx);
    
    for(int i=0; i<PACKET_SIZE; i++) UART_Write(tx.frame[i]);
}

void Process_Debug_Shortcut(char key) {
    RF_Packet_t mock_pkt;
    RF_Init_Packet(&mock_pkt);
    
    mock_pkt.fields.target_id = DEVICE_ID;
    mock_pkt.fields.sender_id = 0x0A; // Mock Sender
    RF_Set_Data(&mock_pkt, 0);

    Soft_UART_print("Debug Key: ");
    Soft_UART_Write(key);
    Soft_UART_println("");

    switch(key) {
        case '1': 
            mock_pkt.fields.command = CMD_RELAY_ON;
            RF_Set_Data(&mock_pkt, SOCKET_A);
            break;
        case '2': 
            mock_pkt.fields.command = CMD_RELAY_OFF;
            RF_Set_Data(&mock_pkt, SOCKET_A);
            break;
        case '3': 
            mock_pkt.fields.command = CMD_RELAY_ON;
            RF_Set_Data(&mock_pkt, SOCKET_B);
            break;
        case '4': 
            mock_pkt.fields.command = CMD_RELAY_OFF;
            RF_Set_Data(&mock_pkt, SOCKET_B);
            break;
        default: return; 
    }
    
    Process_Command(&mock_pkt);
}
