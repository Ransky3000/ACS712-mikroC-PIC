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
  Serial.println("1. RELAY A ON:    AA FE 00 02 00 01 FD BB");
  Serial.println("2. RELAY A OFF:   AA FE 00 03 00 01 FC BB");
  Serial.println("3. RELAY B ON:    AA FE 00 02 00 02 FE BB");
  Serial.println("4. RELAY B OFF:   AA FE 00 03 00 02 FF BB");
  Serial.println("5. READ SENSORS:  AA FE 00 04 00 00 FA BB");
  Serial.println("----------------------------------------------");
  Serial.println("Listening for PIC Response...");
}

void loop() {
  // 1. Read from PC (Serial Monitor) -> Send to HC12
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim(); // Remove whitespace/newlines (crucial for copy-paste)
    
    if (input.length() > 0) {
      Serial.print("[PC] Sending: ");
      Serial.println(input);
      
      // Parse Hex String to Bytes
      sendHexString(input);
    }
  }
  
  // 2. Read from HC12 -> Print to PC
  if (HC12.available()) {
    Serial.print("[RX] ");
    while (HC12.available()) {
      uint8_t byte = HC12.read();
      
      // Print as Hex
      if (byte < 0x10) Serial.print("0");
      Serial.print(byte, HEX);
      Serial.print(" ");
      
      // Optional: Check if it's text (ASCII range)
      // if (byte >= 32 && byte <= 126) Serial.print((char)byte); 
    }
    Serial.println();
  }
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
