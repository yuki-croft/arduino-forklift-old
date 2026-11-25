// ====================================================================================================
// TOOL (one-time setup, not final) | any UNO + HC-05 / HC-06 | sends AT commands to configure a Bluetooth module
// ====================================================================================================
/*
 * tool_bluetooth_at_setup.ino  -  HC-05 / HC-06 AT command bridge (one-time pre-configuration)
 *
 * This is NOT part of the forklift/master firmware. Upload it to any UNO, wire ONE module,
 * open the Serial Monitor and type AT commands. Every byte is passed through unchanged,
 * so no command syntax is hard-coded here (command lists: see PROBLEM_1PAGER.md item 14).
 *
 * Wiring (same pins as the IoT12 PPT slide 10 example):
 *   module TXD -> Arduino D2 (Arduino RX)
 *   module RXD <- Arduino D3 (Arduino TX)  -- through a 5V->3.3V divider (martyncurrey: RX is 3.3V only)
 *   module VCC -> 5V, GND -> GND
 *
 * Settings per module (from martyncurrey.com articles; verify against your own board):
 *   HC-06 : AT mode at power-up, 9600 baud, Serial Monitor line ending = "No line ending", uppercase
 *   HC-05 : AT mode needs EN/KEY (pin 34 on zs-040) HIGH at power-up, AT baud 38400,
 *           Serial Monitor line ending = "Both NL & CR"
 *   -> change MODULE_AT_BAUD below, re-upload, and power-cycle the module.
 */
#include <SoftwareSerial.h>  // ★ EXTERNAL: Arduino built-in library (same as IoT12 PPT slide 10)

const uint8_t PIN_MODULE_TXD = 2;     // module TXD (Arduino receives)
const uint8_t PIN_MODULE_RXD = 3;     // module RXD (Arduino transmits)
const long    USB_BAUD       = 9600;  // Serial Monitor speed
const long    MODULE_AT_BAUD = 9600;  // 9600 = HC-06, 38400 = HC-05 (see header)

SoftwareSerial btModule(PIN_MODULE_TXD, PIN_MODULE_RXD);  // (rx, tx)

void setup() {
  Serial.begin(USB_BAUD);
  btModule.begin(MODULE_AT_BAUD);
  Serial.println(F("AT bridge ready"));
}

void loop() {
  if (btModule.available() > 0) Serial.write(btModule.read());
  if (Serial.available() > 0)   btModule.write(Serial.read());
}
