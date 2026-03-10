/*
 * File:   __main__.c
 * Description: Merged Firmware - FINAL v5.4.0 (Relays + Sensors + Improved ACK)
 * 
 * Features:
 * 1. Relay Control (Active Low) via HC12 & Debug Keys 1-4
 * 2. Sensor Reading (ACS712) via HC12 & Debug Key 5
 * 3. Clean Protocol (No text on HardUART, SoftUART removed for memory)
 * 4. 50ms Inter-packet delay for ESP32 stability
 * 5. ACK Packet includes Socket ID (DataH)
 * 6. 300ms Inrush Delay & Multi-Press Button (Config/Override)
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
#include "Timer_lib.h"
#include "ADC_Lib.h"
#include "ACS712.h"
#include "HC12-RF_Protocol.h"

// --- Hardware Definitions ---
#define _XTAL_FREQ 8000000

#define RELAY_A_PIN RA3
#define RELAY_B_PIN RA2

#define CFG_BTN    RB3   // Config/Reset button (Input, active LOW)
#define CFG_LED    RB4   // Status LED (LOW=defaults, HIGH=configured)

// --- Defaults ---
#define DEFAULT_DEVICE_ID  0x01
#define DEFAULT_ID_MASTER  0x01
#define DEFAULT_THRESHOLD  5000
#define SOCKET_A    1
#define SOCKET_B    2

// --- Runtime Config (loaded from EEPROM) ---
unsigned char device_id = DEFAULT_DEVICE_ID;
unsigned char id_master = DEFAULT_ID_MASTER;
unsigned int overload_threshold_ma = DEFAULT_THRESHOLD;

// --- Config Mode / Reset ---
unsigned char config_mode = 0;
unsigned char button_press_count = 0;
unsigned char btn_was_low = 0;
unsigned long last_btn_press_time = 0;
unsigned long last_btn_edge = 0;

// --- Globals ---
ACS712_t sensorA;
ACS712_t sensorB;

// Overload State Tracking
unsigned char isOverloadedA = 0;
unsigned long cooldownStartA = 0;
unsigned char overloadStrikesA = 0;

unsigned char isOverloadedB = 0;
unsigned long cooldownStartB = 0;
unsigned char overloadStrikesB = 0;

unsigned long last_print = 0;
unsigned long last_sensor_check = 0;

// --- Helper Prototypes ---
void Process_Command(RF_Packet_t *pkt);
void Send_ACK(unsigned char target, unsigned char cmd, unsigned char socket);
void Perform_Read_And_Report(unsigned char sender_id);
void Process_Debug_Shortcut(char key);
unsigned char is_configured(void);
void Flicker_LED_5_Times(void);

// --- ISR ---
void __interrupt() ISR(void) {
    UART_ISR();    // Capture UART bytes first (highest priority)
    Timer_ISR();   // millis() counter
}

void main() {
    // 1. Oscillator Setup
    OSCCON = 0b01110000; // 8MHz
    while(!OSCCONbits.IOFS); 

    // 2. Pin Setup
    ANSEL = 0b00000011; // AN0, AN1 Analog (Sensors)
    
    TRISA = 0b00000011; // RA0, RA1 Input (Sensors), Others Output
    TRISB = 0b00001100; // RB2(RX) Input, RB3(CFG_BTN) Input, RB4(CFG_LED) Output
    
    // Init Relays OFF (NC Wiring: Energize -> NC opens -> disconnected)
    RELAY_A_PIN = 0;
    RELAY_B_PIN = 0;
    
    // 3. Init Libraries
    UART_Init();
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
    
    // EEPROM Load (0x00-01=Threshold, 0x02=DeviceID, 0x03=MasterID)
    unsigned char hi = eeprom_read(0x00);
    unsigned char lo = eeprom_read(0x01);
    if (hi != 0xFF) overload_threshold_ma = (hi << 8) | lo;
    
    unsigned char eid = eeprom_read(0x02);
    if (eid != 0xFF) device_id = eid;
    
    unsigned char ema = eeprom_read(0x03);
    if (ema != 0xFF) id_master = ema;
    
    CFG_LED = is_configured() ? 1 : 0;
    
    ACS712_Calibrate(&sensorA);
    ACS712_Calibrate(&sensorB);
    
    // 5. Rx State Machine
    RF_Packet_t rx_pkt;
    unsigned char rx_idx = 0;
    
    while(1) {
        // --- 1. PRIORITY: UART COMMANDS ---
        
        // (OERR is now handled inside UART_ISR automatically)
        
        if (UART_Data_Ready()) {
            char byte = UART_Read();
            
            // --- SIMULATION MODE ---
            // if (byte >= '1' && byte <= '8') {
            //     Process_Debug_Shortcut(byte);
            //     continue;
            // }
            // -----------------------
            
            // Sync
            if (rx_idx == 0 && byte != SOF_BYTE) continue;
            
            rx_pkt.frame[rx_idx++] = byte;
            
            // Full Packet?
            if (rx_idx >= PACKET_SIZE) {
                if (RF_Verify_Packet(&rx_pkt)) {
                    Process_Command(&rx_pkt);
                }
                rx_idx = 0;
            }
            continue; // Skip sensor reading this cycle to process next byte fast
        }
        
        // --- 2. BUTTON CHECK (Multi-Press Logic) ---
        if (CFG_BTN == 0) {
            if (!btn_was_low) {
                btn_was_low = 1;
            }
        } else {
            if (btn_was_low && (millis() - last_btn_edge >= 50)) {
                if (!config_mode) button_press_count++;
                last_btn_press_time = millis();
                last_btn_edge = millis();
            }
            btn_was_low = 0;
        }
        
        // --- 2-Second Timeout Executor ---
        if (button_press_count > 0 && (millis() - last_btn_press_time >= 2000)) {
            
            if (button_press_count == 3) {
                // Factory Reset (3 Presses)
                eeprom_write(0x00, DEFAULT_THRESHOLD >> 8);
                eeprom_write(0x01, DEFAULT_THRESHOLD & 0xFF);
                eeprom_write(0x02, DEFAULT_DEVICE_ID);
                eeprom_write(0x03, DEFAULT_ID_MASTER);
                device_id = DEFAULT_DEVICE_ID;
                id_master = DEFAULT_ID_MASTER;
                overload_threshold_ma = DEFAULT_THRESHOLD;
                CFG_LED = 0;
                Flicker_LED_5_Times();
            }
            else if (button_press_count == 5) {
                // Config Mode (5 Presses)
                config_mode = 1;
                Flicker_LED_5_Times();
            }
            else if (button_press_count == 7) {
                // Manual Override (7 Presses)
                RELAY_A_PIN = 1;
                RELAY_B_PIN = 1;
                Flicker_LED_5_Times();
            }
            
            button_press_count = 0; // Reset for next sequence
        }
        
        // --- 3. OVERLOAD PROTECTION (Gated: Every 100ms) ---
        if (millis() - last_sensor_check >= 100) {
            last_sensor_check = millis();
            
            // --- Socket A ---
            if (isOverloadedA) {
                if (millis() - cooldownStartA >= 5000) {
                     isOverloadedA = 0;
                     RELAY_A_PIN = 1; // Retry ON (de-energize, NC closes)
                }
            } else {
                unsigned int currentA = ACS712_ReadAC(&sensorA, 60);
                if (currentA > overload_threshold_ma) {
                    overloadStrikesA++;
                    if (overloadStrikesA >= 3) {
                        RELAY_A_PIN = 0; // Trip OFF (energize, NC opens)
                        isOverloadedA = 1;
                        cooldownStartA = millis();
                        overloadStrikesA = 0;
                    }
                } else {
                    overloadStrikesA = 0;
                }
            }
            
            // --- Socket B ---
            if (isOverloadedB) {
                if (millis() - cooldownStartB >= 5000) {
                     isOverloadedB = 0;
                     RELAY_B_PIN = 1; // Retry ON (de-energize, NC closes)
                }
            } else {
                unsigned int currentB = ACS712_ReadAC(&sensorB, 60);
                if (currentB > overload_threshold_ma) {
                    overloadStrikesB++;
                    if (overloadStrikesB >= 3) {
                        RELAY_B_PIN = 0; // Trip OFF (energize, NC opens)
                        isOverloadedB = 1;
                        cooldownStartB = millis();
                        overloadStrikesB = 0;
                    }
                } else {
                    overloadStrikesB = 0;
                }
            }
        }
    }
}

void Process_Command(RF_Packet_t *pkt) {
    // Only process if addressed to me AND from my master
    if (pkt->fields.target_id != device_id) return;
    if (pkt->fields.sender_id != id_master) return;
    
    unsigned char socket = (unsigned char)(pkt->fields.data_l & 0xFF);
    
    switch (pkt->fields.command) {
        case CMD_PING:
            Send_ACK(pkt->fields.sender_id, CMD_PING, 0);
            break;
            
        case CMD_RELAY_ON:
            if (socket == SOCKET_A && !isOverloadedA)      { RELAY_A_PIN = 1; }
            else if (socket == SOCKET_B && !isOverloadedB) { RELAY_B_PIN = 1; }
            Send_ACK(pkt->fields.sender_id, CMD_RELAY_ON, socket);
            break;
            
        case CMD_RELAY_OFF:
            if (socket == SOCKET_A)      { RELAY_A_PIN = 0; }
            else if (socket == SOCKET_B) { RELAY_B_PIN = 0; }
            // Send ACK with Socket ID
            Send_ACK(pkt->fields.sender_id, CMD_RELAY_OFF, socket);
            break;
            
        case CMD_READ_CURRENT:
            Perform_Read_And_Report(pkt->fields.sender_id);
            break;
            
        case CMD_SET_THRESHOLD: {
            unsigned int new_limit = (pkt->fields.data_h << 8) | pkt->fields.data_l;
            overload_threshold_ma = new_limit;
            eeprom_write(0x00, pkt->fields.data_h);
            eeprom_write(0x01, pkt->fields.data_l);
            CFG_LED = is_configured() ? 1 : 0;
            Send_ACK(pkt->fields.sender_id, CMD_SET_THRESHOLD, 0);
            break;
        }
        
        case CMD_SET_DEVICE_ID:
            if (config_mode) {
                device_id = pkt->fields.data_l;
                eeprom_write(0x02, device_id);
                config_mode = 0;
                CFG_LED = is_configured() ? 1 : 0;
                Send_ACK(pkt->fields.sender_id, CMD_SET_DEVICE_ID, 0);
            }
            break;
        
        case CMD_SET_ID_MASTER:
            if (config_mode) {
                id_master = pkt->fields.data_l;
                eeprom_write(0x03, id_master);
                config_mode = 0;
                CFG_LED = is_configured() ? 1 : 0;
                Send_ACK(pkt->fields.sender_id, CMD_SET_ID_MASTER, 0);
            }
            break;
            
        default:
            break;
    }
}

void Send_ACK(unsigned char target, unsigned char cmd, unsigned char socket) {
    RF_Packet_t tx;
    RF_Init_Packet(&tx);
    
    tx.fields.target_id = target;       
    tx.fields.sender_id = device_id;    
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
    // 1. Read Sensors or Check Overload
    unsigned int valA = 0;
    unsigned int valB = 0;
    
    if (isOverloadedA) {
        valA = 0xFFFF; // Status Code: OVERLOAD
    } else {
        valA = ACS712_ReadAC(&sensorA, 60);
    }
    
    if (isOverloadedB) {
        valB = 0xFFFF; // Status Code: OVERLOAD
    } else {
        valB = ACS712_ReadAC(&sensorB, 60);
    }
    
    // 2. Print to SoftUART
    // (Removed to save memory)
    
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
    
    mock_pkt.fields.target_id = device_id;
    mock_pkt.fields.sender_id = id_master; // Must match for validation
    RF_Set_Data(&mock_pkt, 0);

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
            // Read Sensors
            mock_pkt.fields.command = CMD_READ_CURRENT;
            Process_Command(&mock_pkt);
            break;
        case '6':
            // Cfg 10000mA
            mock_pkt.fields.command = CMD_SET_THRESHOLD;
            RF_Set_Data(&mock_pkt, 10000);
            Process_Command(&mock_pkt);
            break;
        case '7':
        case '8':
            if (!config_mode) { break; }
            if (key == '7') {
                mock_pkt.fields.command = CMD_SET_DEVICE_ID;
                RF_Set_Data(&mock_pkt, 0xFE);
            } else {
                mock_pkt.fields.command = CMD_SET_ID_MASTER;
                RF_Set_Data(&mock_pkt, 0x0A);
            }
            Process_Command(&mock_pkt);
            break;
        default: return; 
    }
}

unsigned char is_configured(void) {
    return (device_id != DEFAULT_DEVICE_ID &&
            id_master != DEFAULT_ID_MASTER &&
            overload_threshold_ma != DEFAULT_THRESHOLD);
}

void Flicker_LED_5_Times(void) {
    unsigned char original_state = CFG_LED;
    for (int i = 0; i < 5; i++) {
        CFG_LED = 1;
        __delay_ms(50);
        CFG_LED = 0;
        __delay_ms(50);
    }
    CFG_LED = original_state; // Restore to what it was
}
