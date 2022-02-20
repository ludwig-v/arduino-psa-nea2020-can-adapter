/*
Copyright 2022, Ludwig V. <https://github.com/ludwig-v>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License at <http://www.gnu.org/licenses/> for
more details.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
*/

/////////////////////
//    Libraries    //
/////////////////////

#include <EEPROM.h>
#include <SPI.h>
#include <Time.h>
#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h> // https://github.com/PaulStoffregen/DS1307RTC
#include <mcp2515.h> // https://github.com/autowp/arduino-mcp2515 + https://github.com/watterott/Arduino-Libs/tree/master/digitalWriteFast

/////////////////////
//  Configuration  //
/////////////////////

#define CS_PIN_CAN0 10
#define CS_PIN_CAN1 9
#define SERIAL_SPEED 115200
#define CAN_SPEED_AEE2010 CAN_125KBPS // Entertainment CAN bus - Low speed
#define CAN_SPEED_NEA2020 CAN_500KBPS // NEA2020 CAN bus - High speed
#define CAN_FREQ MCP_8MHZ // Switch to 16MHZ if you have a 16Mhz module

////////////////////
// Initialization //
////////////////////

MCP2515 CAN0(CS_PIN_CAN0); // CAN-BUS Shield N°1
MCP2515 CAN1(CS_PIN_CAN1); // CAN-BUS Shield N°2

////////////////////
//   Variables    //
////////////////////

// My variables
bool debugGeneral = false; // Get some debug informations on Serial
bool debugCAN0 = false; // Read data sent by ECUs from the car using https://github.com/alexandreblin/python-can-monitor
bool debugCAN1 = false; // Read data sent by IVI (NEA2020) using https://github.com/alexandreblin/python-can-monitor
bool resetEEPROM = false;

// Default variables
bool SerialEnabled = false;
byte personalizationSettingsNEA2020[] = {0x00, 0x06, 0x2E, 0x05, 0xD0}; // EN
bool volUP = false;
bool volDOWN = false;

// CAN-BUS Messages
struct can_frame canMsgSnd;
struct can_frame canMsgRcv;

void setup() {
  int tmpVal;

  if (resetEEPROM) {
    EEPROM.update(0, 0);
    EEPROM.update(1, 0);
    EEPROM.update(2, 0);
    EEPROM.update(3, 0);
    EEPROM.update(4, 0);
  }

  if (debugCAN0 || debugCAN1 || debugGeneral) {
    SerialEnabled = true;
  }

  // Read data from EEPROM
  personalizationSettingsNEA2020[0] = EEPROM.read(0);
  personalizationSettingsNEA2020[1] = EEPROM.read(1);
  personalizationSettingsNEA2020[2] = EEPROM.read(2);
  personalizationSettingsNEA2020[3] = EEPROM.read(3);
  personalizationSettingsNEA2020[4] = EEPROM.read(4);

  if (SerialEnabled) {
    // Initalize Serial for debug
    Serial.begin(SERIAL_SPEED);

    // CAN-BUS from car
    Serial.println("Initialization CAN0");
  }

  CAN0.reset();
  CAN0.setBitrate(CAN_SPEED_AEE2010, CAN_FREQ);
  while (CAN0.setNormalMode() != MCP2515::ERROR_OK) {
    delay(100);
  }

  if (SerialEnabled) {
    // CAN-BUS to NEA2020 device(s)
    Serial.println("Initialization CAN1");
  }

  CAN1.reset();
  CAN1.setBitrate(CAN_SPEED_NEA2020, CAN_FREQ);
  while (CAN1.setNormalMode() != MCP2515::ERROR_OK) {
    delay(100);
  }
}

void loop() {
  int tmpVal;

  // Receive CAN messages from the car
  if (CAN0.readMessage( & canMsgRcv) == MCP2515::ERROR_OK) {
    int id = canMsgRcv.can_id;
    int len = canMsgRcv.can_dlc;

    if (debugCAN0) {
      Serial.print("FRAME:ID=");
      Serial.print(id);
      Serial.print(":LEN=");
      Serial.print(len);

      char tmp[3];
      for (int i = 0; i < len; i++) {
        Serial.print(":");

        snprintf(tmp, 3, "%02X", canMsgRcv.data[i]);

        Serial.print(tmp);
      }

      Serial.println();

      CAN1.sendMessage( & canMsgRcv);
    } else if (!debugCAN1) {
      if (id == 0x220) {
        // New profile frame
        canMsgSnd.data[0] = personalizationSettingsNEA2020[0];
        canMsgSnd.data[1] = personalizationSettingsNEA2020[1];
        canMsgSnd.data[2] = personalizationSettingsNEA2020[2];
        canMsgSnd.data[3] = personalizationSettingsNEA2020[3];
        canMsgSnd.data[4] = personalizationSettingsNEA2020[4];
        canMsgSnd.can_id = 0x220;
        canMsgSnd.can_dlc = 5;
        CAN1.sendMessage( & canMsgSnd);
      } else if (id == 0x21F) {
        // Forward original message
        CAN1.sendMessage( & canMsgRcv);

        volUP = bitRead(canMsgRcv.data[0], 3);
        volDOWN = bitRead(canMsgRcv.data[0], 2);
      } else if (id == 0x122 && len == 8) {
        // FMUX
        canMsgSnd.data[0] = canMsgRcv.data[0];
        canMsgSnd.data[1] = canMsgRcv.data[1];
        canMsgSnd.data[2] = canMsgRcv.data[2];
        canMsgSnd.data[3] = canMsgRcv.data[3];
        canMsgSnd.data[4] = canMsgRcv.data[4];
        canMsgSnd.data[5] = canMsgRcv.data[5];
        canMsgSnd.data[6] = canMsgRcv.data[6];
        canMsgSnd.data[7] = canMsgRcv.data[7];

        // Media/Music
        if (bitRead(canMsgRcv.data[2], 4)) {
          bitWrite(canMsgSnd.data[2], 4, 0); // NAC
          bitWrite(canMsgSnd.data[0], 2, 1); // IVI = Home
        }
        // Phone
        if (bitRead(canMsgRcv.data[2], 3)) {
          bitWrite(canMsgSnd.data[2], 3, 0); // NAC
          bitWrite(canMsgSnd.data[0], 2, 1); // IVI = Home
        }
        // Web
        if (bitRead(canMsgRcv.data[2], 2)) {
          bitWrite(canMsgSnd.data[2], 2, 0); // NAC
          bitWrite(canMsgSnd.data[0], 2, 1); // IVI = Home
        }
        // Nav
        if (bitRead(canMsgRcv.data[2], 0)) {
          bitWrite(canMsgSnd.data[2], 0, 0); // NAC
          bitWrite(canMsgSnd.data[0], 2, 1); // IVI = Home
        }
        // Drive
        if (bitRead(canMsgRcv.data[3], 7)) {
          bitWrite(canMsgSnd.data[3], 7, 0); // NAC
          bitWrite(canMsgSnd.data[0], 3, 1); // IVI = ADAS
        }
        // Climate
        if (bitRead(canMsgRcv.data[3], 6)) {
          bitWrite(canMsgSnd.data[3], 6, 0); // NAC
          bitWrite(canMsgSnd.data[0], 5, 1); // IVI = Climate
        }

        // Vol+ from 0x21F
        if (volUP) {
          bitWrite(canMsgSnd.data[2], 4, 1); // IVI = Vol+
        } else {
          bitWrite(canMsgSnd.data[2], 4, 0);
        }
        // Vol- from 0x21F
        if (volDOWN) {
          bitWrite(canMsgSnd.data[2], 3, 1); // IVI = Vol-
        } else {
          bitWrite(canMsgSnd.data[2], 3, 0);
        }

        canMsgSnd.can_id = 0x122;
        canMsgSnd.can_dlc = 8;
        CAN1.sendMessage( & canMsgSnd);
      } else {
        CAN1.sendMessage( & canMsgRcv);
      }
    } else {
      CAN1.sendMessage( & canMsgRcv);
    }
  }

  // Forward messages from the NEA2020 device(s) to the car
  if (CAN1.readMessage( & canMsgRcv) == MCP2515::ERROR_OK) {
    int id = canMsgRcv.can_id;
    int len = canMsgRcv.can_dlc;

    if (debugCAN1) {
      Serial.print("FRAME:ID=");
      Serial.print(id);
      Serial.print(":LEN=");
      Serial.print(len);

      char tmp[3];
      for (int i = 0; i < len; i++) {
        Serial.print(":");

        snprintf(tmp, 3, "%02X", canMsgRcv.data[i]);

        Serial.print(tmp);
      }

      Serial.println();

      CAN0.sendMessage( & canMsgRcv);
    } else if (!debugCAN0) {
      if (id == 0x057 || id == 0x257) {
        // Block mileage sending to BSI
      } else if (id == 0x128 || id == 0x117) {
        // Block IVI CMB sending to BSI
      } else if (id == 0x21B) {
        // Store personalization settings for the recurring frame
        personalizationSettingsNEA2020[0] = canMsgRcv.data[0];
        personalizationSettingsNEA2020[1] = canMsgRcv.data[1];
        personalizationSettingsNEA2020[2] = canMsgRcv.data[2];
        personalizationSettingsNEA2020[3] = canMsgRcv.data[3];
        personalizationSettingsNEA2020[4] = canMsgRcv.data[4];

        // Enable languages changing
        bitWrite(personalizationSettingsNEA2020[2], 3, 1);

        EEPROM.update(0, personalizationSettingsNEA2020[0]);
        EEPROM.update(1, personalizationSettingsNEA2020[1]);
        EEPROM.update(2, personalizationSettingsNEA2020[2]);
        EEPROM.update(3, personalizationSettingsNEA2020[3]);
        EEPROM.update(4, personalizationSettingsNEA2020[4]);
      } else {
        CAN0.sendMessage( & canMsgRcv);
      }
    } else {
      CAN0.sendMessage( & canMsgRcv);
    }

  }
}