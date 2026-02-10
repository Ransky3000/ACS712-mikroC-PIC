/*
 * Central_control_command_test.ino
 * 
 * Description:
 * ESP32 Master Controller for Testing PIC16F88 Smart Outlet.
 * - Allows user to Type/Paste Hex Strings in Serial Monitor.
 * - Sends them as Binary Packets via HC12 (Serial2).
 * - Displays Responses from PIC.
 * 
 * Hardware Connection (ESP32):
 * - HC12 TX -> ESP32 GPIO 16 (RX2)
 * - HC12 RX -> ESP32 GPIO 17 (TX2)
 * - GND -> GND
 * - VCC -> 5V (or 3.3V if supported)
 */

#include <HardwareSerial.h>

// --- Configuration ---
#define HC12_RX_PIN 16 // Connect to HC12 TX
#define HC12_TX_PIN 17 // Connect to HC12 RX
#define BAUDRATE    9600

HardwareSerial HC12(2); // Use UART2

void setup() {
  Serial.begin(115200);
  HC12.begin(BAUDRATE, SERIAL_8N1, HC12_RX_PIN, HC12_TX_PIN);
  
  delay(1000);
  Serial.println("\n--- ESP32 HC12 Master Test ---");
  Serial.println("Paste these Hex Frames into the Input Box and hit Enter:");
  Serial.println("");
  Serial.println("--- RELAY A ON ---");
  Serial.println("  1. Device 1 (FE): AA FE 00 02 00 01 FD BB");
  Serial.println("  2. Device 2 (FD): AA FD 00 02 00 01 FE BB");
  Serial.println("--- RELAY A OFF ---");
  Serial.println("  1. Device 1 (FE): AA FE 00 03 00 01 FC BB");
  Serial.println("  2. Device 2 (FD): AA FD 00 03 00 01 FF BB");
  Serial.println("--- RELAY B ON ---");
  Serial.println("  1. Device 1 (FE): AA FE 00 02 00 02 FE BB");
  Serial.println("  2. Device 2 (FD): AA FD 00 02 00 02 FD BB"); // Not working here | Edited: Resolved
  Serial.println("--- RELAY B OFF ---");
  Serial.println("  1. Device 1 (FE): AA FE 00 03 00 02 FF BB");
  Serial.println("  2. Device 2 (FD): AA FD 00 03 00 02 FC BB"); // Not working here | Edited: Resolved
  Serial.println("--- READ SENSORS ---");
  Serial.println("  1. Device 1 (FE): AA FE 00 04 00 00 FA BB");
  Serial.println("  2. Device 2 (FD): AA FD 00 04 00 00 F9 BB"); // Not working here | Edited: Resolved
  Serial.println("----------------------------------------------");
  Serial.println("Listening for PIC Response...");
}

// --- Globals ---
uint8_t rxBuffer[8];
uint8_t rxIndex = 0;

void loop() {
  // 1. Read from PC (Serial Monitor) -> Send to HC12
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0) {
      Serial.print("[PC] Sending: ");
      Serial.println(input);
      sendHexString(input);
    }
  }
  
  // 2. Read from HC12 -> Parse Packet
  if (HC12.available()) {
    while (HC12.available()) {
      uint8_t byte = HC12.read();
      
      // If we see ASCII text (Debug msg), just print it directly
      // Heuristic: If we are not inside a packet (rxIndex==0) and byte is ASCII char
      if (rxIndex == 0 && byte != 0xAA) {
        if (byte >= 32 && byte <= 126) Serial.write(byte); // Printable
        else if (byte == 13 || byte == 10) Serial.write(byte); // Newline
        continue;
      }
      
      // Packet Framing
      if (rxIndex == 0 && byte != 0xAA) continue; // Wait for Start
      
      rxBuffer[rxIndex++] = byte;
      
      // Packet Full?
      if (rxIndex >= 8) {
        parsePacket(rxBuffer);
        rxIndex = 0; // Reset
      }
    }
  }
}

void parsePacket(uint8_t* frame) {
  uint8_t cmd = frame[3]; // Command
  uint8_t sender = frame[2]; // From ID
  uint8_t dataH = frame[4];
  uint8_t dataL = frame[5];
  uint16_t val16 = (dataH << 8) | dataL;
  
  Serial.println("\n--- RX PACKET ---");
  
  // Print Raw
  Serial.print("RAW: ");
  for(int i=0; i<8; i++) {
    if(frame[i]<0x10) Serial.print("0");
    Serial.print(frame[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  // Interpret
  Serial.print("FROM: ");
  if (sender == 0xFE) Serial.println("PIC (Default)");
  else if (sender == 0x01) Serial.println("Socket A");
  else if (sender == 0x02) Serial.println("Socket B");
  else { Serial.print("ID "); Serial.println(sender, HEX); }
  
  Serial.print("TYPE: ");
  if (cmd == 0x06) {
    Serial.println("ACKNOWLEDGE");
    
    // Check Socket ID (DataH)
    Serial.print("TARGET: ");
    if (dataH == 0x01) Serial.println("Socket A");
    else if (dataH == 0x02) Serial.println("Socket B");
    else if (dataH == 0x00) Serial.println("System (Ping)");
    else Serial.println("Unknown");

    Serial.print("ACTION: ");
    if (dataL == 0x02) Serial.println("Relay ON");
    else if (dataL == 0x03) Serial.println("Relay OFF");
    else if (dataL == 0x01) Serial.println("Pong");
    else Serial.println("Unknown");
  }
  else if (cmd == 0x05) {
    Serial.println("DATA REPORT");
    Serial.print("VALUE: ");
    Serial.print(val16);
    Serial.print(" mA (");
    Serial.print(val16 / 1000.0, 2);
    Serial.println(" A)");
  }
  else {
    Serial.print("CMD ");
    Serial.println(cmd, HEX);
  }
  Serial.println("-----------------");
}

// Helper: Converts "AA FE 00 ..." string to binary bytes
void sendHexString(String hexStr) {
  // Remove spaces if any
  hexStr.replace(" ", "");
  
  int len = hexStr.length();
  if (len % 2 != 0) {
    Serial.println("Error: Hex string must have even number of characters.");
    return;
  }
  
  for (int i = 0; i < len; i += 2) {
    String byteStr = hexStr.substring(i, i+2);
    // Parse hex byte
    uint8_t b = (uint8_t) strtol(byteStr.c_str(), NULL, 16);
    HC12.write(b);
  }
}
