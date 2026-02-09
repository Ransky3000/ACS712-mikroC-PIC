/*
 * File:   __main__.c
 * Description: Merged Firmware - FINAL (Relays + Sensors + Improved ACK)
 * 
 * Features:
 * 1. Relay Control (Active Low) via HC12 & Debug Keys 1-4
 * 2. Sensor Reading (ACS712) via HC12 & Debug Key 5
 * 3. Clean Protocol (No text on HardUART)
 * 4. 50ms Inter-packet delay for ESP32 stability
 * 5. ACK Packet includes Socket ID (DataH)
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
#include "Timer_lib.h"
#include "ADC_Lib.h"
#include "ACS712.h"
#include "HC12-RF_Protocol.h"

// --- Hardware Definitions ---
#define _XTAL_FREQ 8000000

#define RELAY_A_PIN RA2
#define RELAY_B_PIN RA3

// --- ID Configuration ---
#define DEVICE_ID   0xFE
#define SOCKET_A    1
#define SOCKET_B    2

// --- Globals ---
ACS712_t sensorA;
ACS712_t sensorB;

// --- Helper Prototypes ---
void Process_Command(RF_Packet_t *pkt);
void Send_ACK(unsigned char target, unsigned char cmd, unsigned char socket);
void Process_Debug_Shortcut(char key);
void Perform_Read_And_Report(unsigned char sender_id);
void print_int_to_uart(unsigned int val, unsigned char is_soft);

// --- ISR ---
void __interrupt() ISR(void) {
    Timer_ISR(); // Logic for millis() / delays if used by libraries
    Soft_UART_ISR();
}

void main() {
    // 1. Oscillator Setup
    OSCCON = 0b01110000; // 8MHz
    while(!OSCCONbits.IOFS); 

    // 2. Pin Setup
    ANSEL = 0b00000011; // AN0, AN1 Analog (Sensors)
    
    TRISA = 0b00000011; // RA0, RA1 Input (Sensors), Others Output
    TRISB = 0b00000100; // RB2(RX) Input, RB5(TX) Output
    
    // Init Relays OFF (Active Low -> High)
    RELAY_A_PIN = 1;
    RELAY_B_PIN = 1;
    
    // 3. Init Libraries
    UART_Init();
    Soft_UART_Init(&PORTB, 6, 7, 9600, 0); 
    Time_Init(8);   // Init Timer for timestamps/delays
    ADC_Init();     // Init ADC Module

    // 4. Init Sensors
    // Channel 0 (AN0), 5000mV Ref, 1023 Res
    ACS712_Init(&sensorA, 0, 5000, 1023); 
    // Channel 1 (AN1), 5000mV Ref, 1023 Res
    ACS712_Init(&sensorB, 1, 5000, 1023); 

    // Sensitivity (100mV/A for 20A Module)
    ACS712_SetSensitivity(&sensorA, 100); 
    ACS712_SetSensitivity(&sensorB, 100); 
    
    Soft_UART_println("--- Merged Firmware: Chunk 2 (FINAL) ---");
    Soft_UART_println("Calibrating Sensors...");
    
    ACS712_Calibrate(&sensorA);
    ACS712_Calibrate(&sensorB);
    
    Soft_UART_println("Ready. Keys: 1-5 Active");
    
    // 5. Rx State Machine
    RF_Packet_t rx_pkt;
    unsigned char rx_idx = 0;
    
    while(1) {
        if (UART_Data_Ready()) {
            char byte = UART_Read();
            
            // --- SIMULATION MODE ---
            // Keys 1-4 (Relays), 5 (Sensors)
            // Comment this block out for HW DEPLOY
            /*
            if (byte >= '1' && byte <= '5') {
                Process_Debug_Shortcut(byte);
                continue; 
            }
            */
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
            Send_ACK(pkt->fields.sender_id, CMD_PING, 0);
            break;
            
        case CMD_RELAY_ON:
            if (socket == SOCKET_A)      RELAY_A_PIN = 0; // Active Low
            else if (socket == SOCKET_B) RELAY_B_PIN = 0;
            // Send ACK with Socket ID
            Send_ACK(pkt->fields.sender_id, CMD_RELAY_ON, socket);
            break;
            
        case CMD_RELAY_OFF:
            if (socket == SOCKET_A)      RELAY_A_PIN = 1; 
            else if (socket == SOCKET_B) RELAY_B_PIN = 1;
            // Send ACK with Socket ID
            Send_ACK(pkt->fields.sender_id, CMD_RELAY_OFF, socket);
            break;
            
        case CMD_READ_CURRENT:
            Perform_Read_And_Report(pkt->fields.sender_id);
            break;
            
        default:
            break;
    }
}

void Send_ACK(unsigned char target, unsigned char cmd, unsigned char socket) {
    RF_Packet_t tx;
    RF_Init_Packet(&tx);
    
    tx.fields.target_id = target;       
    tx.fields.sender_id = DEVICE_ID;    
    tx.fields.command   = CMD_ACK;   
    
    // Pack Socket (DataH) and Command (DataL)
    unsigned int payload = (unsigned int)((socket << 8) | cmd);
    RF_Set_Data(&tx, payload);
    
    RF_Sign_Packet(&tx);
    
    for(int i=0; i<PACKET_SIZE; i++) UART_Write(tx.frame[i]);
}

void Perform_Read_And_Report(unsigned char sender_id) {
    // 1. Read Sensors (Blocking ~34ms total)
    // 60 samples per sensor for AC RMS
    unsigned int valA = ACS712_ReadAC(&sensorA, 60);
    unsigned int valB = ACS712_ReadAC(&sensorB, 60);
    
    // 2. Print Text to SoftUART (For Debug Cable)
    Soft_UART_print("S1: ");
    print_int_to_uart(valA, 1);
    Soft_UART_print(" mA | S2: ");
    print_int_to_uart(valB, 1);
    Soft_UART_println(" mA");
    
    // 3. Send Protocol Packets to HC12 (Clean Binary)
    
    // Packet A (Socket 1)
    RF_Packet_t tx;
    RF_Init_Packet(&tx);
    tx.fields.target_id = sender_id; // Reply to requester
    tx.fields.sender_id = 0x01;      // ID 1 = Socket A
    tx.fields.command   = CMD_REPORT_DATA;
    RF_Set_Data(&tx, valA);
    RF_Sign_Packet(&tx);
    
    for(int i=0; i<PACKET_SIZE; i++) UART_Write(tx.frame[i]);
    
    __delay_ms(50); // CRITICAL: 50ms gap to prevent ESP32 buffer overflow
    
    // Packet B (Socket 2)
    tx.fields.target_id = sender_id;
    tx.fields.sender_id = 0x02;      // ID 2 = Socket B
    RF_Set_Data(&tx, valB);
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
            Process_Command(&mock_pkt);
            break;
        case '2': 
            mock_pkt.fields.command = CMD_RELAY_OFF;
            RF_Set_Data(&mock_pkt, SOCKET_A);
            Process_Command(&mock_pkt);
            break;
        case '3': 
            mock_pkt.fields.command = CMD_RELAY_ON;
            RF_Set_Data(&mock_pkt, SOCKET_B);
            Process_Command(&mock_pkt);
            break;
        case '4': 
            mock_pkt.fields.command = CMD_RELAY_OFF;
            RF_Set_Data(&mock_pkt, SOCKET_B);
            Process_Command(&mock_pkt);
            break;
        case '5':
            // Key 5: Read Sensors
            mock_pkt.fields.command = CMD_READ_CURRENT;
            // No need to process socket ID for read
            Process_Command(&mock_pkt);
            break;
        default: return; 
    }
}

// Helper: 1=Soft, 0=Hard
void print_int_to_uart(unsigned int val, unsigned char is_soft) {
    if(val == 0) {
        if(is_soft) Soft_UART_Write('0'); else UART_Write('0');
        return;
    }
    char buffer[6];
    int i = 0;
    while(val > 0) {
        buffer[i++] = (val % 10) + '0';
        val /= 10;
    }
    while(--i >= 0) {
        if(is_soft) Soft_UART_Write(buffer[i]); else UART_Write(buffer[i]);
    }
}
