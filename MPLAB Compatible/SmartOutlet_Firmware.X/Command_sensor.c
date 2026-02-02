/*
 * File:   Command_sensor.c
 * Description: Event-Driven Sensor Test (ACS712)
 * - Behavior matches Command_relay.c:
 *   1. Polls HC12 for Command (or Debug Key)
 *   2. On Command: Reading BOTH Sensors
 *   3. Output: SoftUART (Text), HardUART (Text + Packet)
 */

// CONFIG1
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits
#pragma config WDTE = OFF       // Watchdog Timer Enable bit
#pragma config PWRTE = OFF      // Power-up Timer Enable bit
#pragma config MCLRE = ON       // RA5/MCLR/VPP Pin Function Select bit
#pragma config BOREN = OFF      // Brown-out Reset Enable bit
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
#include "Timer_lib.h"
#include "ACS712.h"
#include "ADC_Lib.h"
#include "HC12-RF_Protocol.h"

#define _XTAL_FREQ 8000000

// --- Globals ---
ACS712_t sensorA;
ACS712_t sensorB;

// --- Helper Prototypes ---
void Process_Command(RF_Packet_t *pkt);
void Perform_Read_And_Report(unsigned char sender_id);
void Process_Debug_Shortcut(char key);
void print_int_to_uart(unsigned int val, unsigned char is_soft);

// --- Central Interrupt Service Routine ---
void __interrupt() ISR(void) {
    Timer_ISR(); 
    Soft_UART_ISR();
}

void main() {
    // 1. Setup Oscillator (8MHz)
    OSCCON = 0b01110000;
    while(!OSCCONbits.IOFS); 

    // 2. Pin Setup
    ANSEL = 0b00000011; // AN0, AN1 Analog
    TRISA = 0b11111111; // All Inputs
    TRISB = 0b00000100; // RB2(RX) Input
    
    // 3. Init Libraries
    UART_Init();                        
    Soft_UART_Init(&PORTB, 6, 7, 9600, 0); 
    Time_Init(8);                       
    ADC_Init(); 

    // 4. Init Sensors
    // Channel 0 (AN0), 5000mV Ref, 1023 Res
    ACS712_Init(&sensorA, 0, 5000, 1023); 
    // Channel 1 (AN1), 5000mV Ref, 1023 Res
    ACS712_Init(&sensorB, 1, 5000, 1023); 
    
    Soft_UART_println("--- Sensor Test Boot ---");
    Soft_UART_println("Calibrating (Ensure 0A)...");
    
    ACS712_Calibrate(&sensorA);
    ACS712_Calibrate(&sensorB);
    
    Soft_UART_println("Ready. Waiting for Cmd/Key...");
    
    // 5. Rx State Machine (Identical to Command_relay.c)
    RF_Packet_t rx_pkt;
    unsigned char rx_idx = 0;
    
    while(1) {
        // Poll Hardware UART
        if (UART_Data_Ready()) {
            char byte = UART_Read();
            
            // --- DEBUG MODE: Check for Shortcuts '1'-'9' ---
            // DISABLED FOR HARDWARE INTEGRATION (ESP32)
            /*
            if (byte >= '1' && byte <= '9') {
                Process_Debug_Shortcut(byte);
                continue; 
            }
            */
            
            // Packet Sync
            if (rx_idx == 0 && byte != SOF_BYTE) {
                continue; 
            }
            
            rx_pkt.frame[rx_idx++] = byte;
            
            // Packet Full?
            if (rx_idx >= PACKET_SIZE) {
                if (RF_Verify_Packet(&rx_pkt)) {
                    Soft_UART_print("Cmd Recv from ID: ");
                    Soft_UART_Write(rx_pkt.fields.sender_id + '0'); // Hex char rough
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
    // For this test, we accept ANY command (Ping, Relay, Read) 
    // and respond by Reading Sensors and Reporting Data.
    // Ideally we check for CMD_READ_CURRENT, but user said "each time I command"
    
    Perform_Read_And_Report(pkt->fields.sender_id);
}

void Process_Debug_Shortcut(char key) {
    Soft_UART_print("Debug Key: ");
    Soft_UART_Write(key);
    Soft_UART_println("");
    
    // Simulate Command from Master (ID 0)
    Perform_Read_And_Report(0x00); 
}

void Perform_Read_And_Report(unsigned char sender_id) {
    // 1. Read Sensors (Blocking ~34ms total)
    unsigned int valA = ACS712_ReadAC(&sensorA, 60);
    unsigned int valB = ACS712_ReadAC(&sensorB, 60);
    
    // 2. Print Text to SoftUART
    Soft_UART_print("S1: ");
    print_int_to_uart(valA, 1);
    Soft_UART_print(" mA | S2: ");
    print_int_to_uart(valB, 1);
    Soft_UART_println(" mA");
    
    // 3. Print Text to HardUART (Mirror)
    UART_Write_Text("S1: ");
    print_int_to_uart(valA, 0);
    UART_Write_Text(" mA | S2: ");
    print_int_to_uart(valB, 0);
    UART_Write_Text(" mA\r\n");
    
    // 4. Send Protocol Packets (Report Data)
    
    // Packet A
    RF_Packet_t tx;
    RF_Init_Packet(&tx);
    tx.fields.target_id = sender_id; // Reply to whoever asked
    tx.fields.sender_id = 0x01;      // ID 1 = Socket A
    tx.fields.command   = CMD_REPORT_DATA;
    RF_Set_Data(&tx, valA);
    RF_Sign_Packet(&tx);
    
    // Send Packet A
    for(int i=0; i<PACKET_SIZE; i++) UART_Write(tx.frame[i]);
    
    __delay_ms(10); // Small gap
    
    // Packet B
    tx.fields.target_id = sender_id;
    tx.fields.sender_id = 0x02;      // ID 2 = Socket B
    RF_Set_Data(&tx, valB);
    RF_Sign_Packet(&tx);
    
    // Send Packet B
    for(int i=0; i<PACKET_SIZE; i++) UART_Write(tx.frame[i]);
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
