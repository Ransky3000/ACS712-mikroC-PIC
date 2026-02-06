/*
 * File:   Command_relay.c
 * Description: Simplified Relay Control Test (No Sensors, No TimerLib)
 * - Focused on verifying Hardware UART and Relay Logic
 */

// CONFIG1
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (Internal oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RA5/MCLR/VPP Pin Function Select bit (MCLR enabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable bit 
#pragma config CPD = OFF        // Data EE Memory Code Protection bit
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits
#pragma config CCPMX = RB0      // CCP1 Pin Selection bit
#pragma config CP = OFF         // Flash Program Memory Code Protection bit

// CONFIG2
#pragma config FCMEN = OFF     
#pragma config IESO = OFF       

#include <xc.h>

#include "UART_Lib.h"
#include "Soft_UART.h"
#include "HC12-RF_Protocol.h"

#define _XTAL_FREQ 8000000

// --- Pin Definitions ---
#define RELAY_A_PIN RA2
#define RELAY_B_PIN RA3

// --- Configuration ---
#define DEVICE_ID   0xFE
#define SOCKET_A    1
#define SOCKET_B    2

// --- Helper Prototypes ---
void Process_Command(RF_Packet_t *pkt);
void Send_ACK(unsigned char target, unsigned char cmd);
void Process_Debug_Shortcut(char key);
void Log_Packet(char* label, unsigned char* frame); 

// --- Central Interrupt Service Routine ---
void __interrupt() ISR(void) {
    // Only Soft_UART for now. Timer0 disabled to prevent freeze.
    Soft_UART_ISR();
}

void main() {
    // 1. Setup Oscillator (8MHz)
    OSCCON = 0b01110000;
    while(!OSCCONbits.IOFS); 

    // 2. Pin Setup
    ANSEL = 0; // Digital I/O only (No Sensors)
    
    // TRIS: 1=Input, 0=Output
    TRISA = 0b00000000; // All Output (RA2, RA3 Relays) except maybe RA5(MCLR)
    TRISB = 0b00010100; // RB2(RX) Input, RB5(TX) Output
    
    // Init Relays OFF (Active Low -> High)
    RELAY_A_PIN = 1;
    RELAY_B_PIN = 1;
    
    // 3. Init Libraries
    UART_Init(); // Hard UART (9600)
    Soft_UART_Init(&PORTB, 6, 7, 9600, 0); // Soft UART (Debug)
    
    Soft_UART_println("--- Relay Test Boot ---");
    Soft_UART_println("Keys: 1=A_ON, 2=A_OFF, 3=B_ON, 4=B_OFF");
    
    // 5. Rx State Machine
    RF_Packet_t rx_pkt;
    unsigned char rx_idx = 0;
    
    while(1) {
        // Poll Hardware UART
        if (UART_Data_Ready()) {
            char byte = UART_Read();
            
            // --- DEBUG MODE: Check for Shortcuts '1'-'4' ---
            // DISABLED FOR HARDWARE INTEGRATION (ESP32)
            
            // if (byte >= '1' && byte <= '4') {
            //     Process_Debug_Shortcut(byte);
            //     continue; 
            // }
            
            // Packet Sync
            if (rx_idx == 0 && byte != SOF_BYTE) {
                continue; 
            }
            
            rx_pkt.frame[rx_idx++] = byte;
            
            // Packet Full?
            if (rx_idx >= PACKET_SIZE) {
                if (RF_Verify_Packet(&rx_pkt)) {
                    Soft_UART_print("Cmd Recv for ID: ");
                    Soft_UART_Write(rx_pkt.fields.target_id + '0'); // Hex char rough
                    Soft_UART_println("");
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
    if (pkt->fields.target_id != DEVICE_ID) return;
    
    unsigned char socket = (unsigned char)(pkt->fields.data_l & 0xFF);
    
    switch (pkt->fields.command) {
        case CMD_PING:
            Send_ACK(pkt->fields.sender_id, CMD_PING);
            break;
            
        case CMD_RELAY_ON:
            if (socket == SOCKET_A) {
                RELAY_A_PIN = 0; // Active Low
                Soft_UART_println("Relay A: ON");
                UART_Write_Text("Relay A: ON\r\n"); // Mirror to Hard UART
            }
            else if (socket == SOCKET_B) {
                RELAY_B_PIN = 0; 
                Soft_UART_println("Relay B: ON");
                UART_Write_Text("Relay B: ON\r\n"); // Mirror to Hard UART
            }
            Send_ACK(pkt->fields.sender_id, CMD_RELAY_ON);
            break;
            
        case CMD_RELAY_OFF:
            if (socket == SOCKET_A) {
                RELAY_A_PIN = 1; // Active Low
                Soft_UART_println("Relay A: OFF");
                UART_Write_Text("Relay A: OFF\r\n"); // Mirror to Hard UART
            }
            else if (socket == SOCKET_B) {
                RELAY_B_PIN = 1;
                Soft_UART_println("Relay B: OFF");
                UART_Write_Text("Relay B: OFF\r\n"); // Mirror to Hard UART
            }
            Send_ACK(pkt->fields.sender_id, CMD_RELAY_OFF);
            break;
            
        default:
            break;
    }
}

void Send_ACK(unsigned char target, unsigned char cmd) {
    RF_Packet_t tx;
    RF_Init_Packet(&tx);
    
    tx.fields.target_id = target;       // Reply to the Sender (Requester)
    tx.fields.sender_id = DEVICE_ID;    // From ME (0xFE)
    tx.fields.command   = CMD_ACK;   
    RF_Set_Data(&tx, cmd);
    
    RF_Sign_Packet(&tx); // Finalize
    
    Log_Packet("TX ACK", tx.frame); 

    for(int i=0; i<PACKET_SIZE; i++) {
        UART_Write(tx.frame[i]);
    }
}

void Log_Packet(char* label, unsigned char* frame) {
    Soft_UART_print(label);
    Soft_UART_print(": ");
    
    char hex[3];
    const char hex_map[] = "0123456789ABCDEF";
    
    for(int i=0; i<PACKET_SIZE; i++) {
        unsigned char b = frame[i];
        hex[0] = hex_map[(b >> 4) & 0x0F];
        hex[1] = hex_map[b & 0x0F];
        hex[2] = '\0';
        Soft_UART_print(hex);
        Soft_UART_Write(' ');
    }
    Soft_UART_println("");
}

void Process_Debug_Shortcut(char key) {
    RF_Packet_t mock_pkt;
    RF_Init_Packet(&mock_pkt);
    
    mock_pkt.fields.target_id = DEVICE_ID;
    mock_pkt.fields.sender_id = 0x0A; 
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
