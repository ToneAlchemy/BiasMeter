 /** Arduino Tube Amp Bias Meter (V11.4 Platinum - GOLD MASTER)
 * * Copyright (C) 2024-2026 Charlie Isaac (ToneAlchemy.com)
 * * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * * ======================================================================
 * ATTRIBUTION & HISTORY
 * This project is an evolutionary adaptation based on:
 * 1. "Dual Channel Arduino Bias Tester beta 14" by Kiel Lydestad (3DBeerGoggles).
 * Original License: Creative Commons Attribution 4.0 International (CC BY 4.0).
 * 2. "ArduinoBiasMeter" by John Wagner.
 * Original License: Creative Commons Attribution 4.0 International (CC BY 4.0).
 * * Per the terms of the CC BY 4.0 license, this adaptation is released
 * under the GNU General Public License v3.0 (GPLv3) to ensure all future
 * derivatives remain open-source.
 * ======================================================================
 */
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ADS1X15.h>
#include <EEPROM.h>
#include <math.h>
 
// --- PIN DEFINITIONS (Arduino Nano) ---
#define SCLK 13
#define MOSI 11
#define CS 10
#define DC 9
#define RST 8

// --- CONFIGURATION ---
#define NUM_SAMPLES 20
#define ADC_NOISE_THRESHOLD 10
#define MAX_TUBES 10

// --- COLOR DEFINITIONS ---
#define ST7735_GREY 0x7BEF
const float mV_per_bit = 0.0078125;

// --- OBJECTS ---
Adafruit_ST7735 tft = Adafruit_ST7735(CS, DC, MOSI, SCLK, RST);
Adafruit_ADS1115 ads;

// --- MEMORY MAP ---
const int EEPROM_ID = 12357;
struct CalibrationData {
  int id;
  float vScaleA;
  float vScaleB;
  float rShuntA;
  float rShuntB;
  float voltLimit;
  uint8_t checksum; // Data Integrity Check
};

struct TubeData {
  char name[7];
  int maxDiss;
  float screenFactor;
};

CalibrationData calData;
TubeData currentTubeDB[MAX_TUBES];
int tubeCount = 0;

// --- VARIABLES ---
float VoltageA = 0.0, CurrentA = 0.0, PowerA = 0.0;
float VoltageB = 0.0, CurrentB = 0.0, PowerB = 0.0;
float CurrentDiff = 0.0;

float VoltageAPrevious = -1, CurrentAPrevious = -1, PowerAPrevious = -1, WattPercAPrevious = -1;
float VoltageBPrevious = -1, CurrentBPrevious = -1, PowerBPrevious = -1, WattPercBPrevious = -1;
float CurrentDiffPrevious = -1;

bool FirstBiasRun = false;
bool resetMgrState = false;

// Buttons
int guiClick = 7;
int guiLClick = 5;
int guiRClick = 6;

// GUI State
#define GUI_MENU_SELECT_MODE 0
#define GUI_BIAS_MODE 1
#define GUI_CALIBRATION_MODE 2
#define GUI_TUBE_MANAGER 3
int guiMode = GUI_MENU_SELECT_MODE;

boolean movedMenu = false;
int curMenuItem = 0;
int lastMenuItem = -1;
unsigned long lastRedrawTime = 0;
const unsigned long redrawInterval = 200;

unsigned long startTime = 0;
const unsigned long startupDelay = 2000;

// Used for Bias Mode
TubeData activeTube;

// --- PROTOTYPES ---
void drawMetric(int x, int y, float val, float &prevVal, char* suffix, uint16_t color);
int processMainMenu();
void drawMainMenuStatic();
void updateMainMenuDynamic();
void doBias();
void doCalibration();
void doTubeManager();
void loadData();
void saveTubeDB();
void saveCalibration();
boolean buttonClicked();
int getInputsV10();
void drawCursor(int fieldIdx, uint16_t color);
void displaySplashScreen();
void displayBiasScreenLayout();
void checkAndHandleSafety(float vA, float vB);
uint8_t calculateChecksum(CalibrationData &data);

// =========================================================================
// SETUP
// =========================================================================
void setup() {
  pinMode(guiClick, INPUT_PULLUP);
  pinMode(guiRClick, INPUT_PULLUP);
  pinMode(guiLClick, INPUT_PULLUP);

  loadData();

  tft.initR(INITR_18BLACKTAB);
  tft.setRotation(3);
  displaySplashScreen();

  if (!ads.begin(0x48)) {
    tft.fillScreen(ST7735_RED);
    tft.println(F("ADC ERROR!"));
    while (1);
  }

  ads.setGain(GAIN_SIXTEEN);
  ads.setDataRate(RATE_ADS1115_860SPS);

  lastMenuItem = -1;
  startTime = millis();
 
}

// =========================================================================
// MAIN LOOP
// =========================================================================
void loop() {
 
   if (millis() - startTime < startupDelay) {
    if (digitalRead(guiLClick) == LOW) {
       guiMode = GUI_CALIBRATION_MODE;
       tft.fillScreen(ST7735_BLACK);
       startTime = 0;
    }
    return;
  }

  // --- MENU SELECT ---
  if (guiMode == GUI_MENU_SELECT_MODE) {
    static bool menuInitialized = false;
    if (!menuInitialized) {
       tft.fillScreen(ST7735_BLACK);
       drawMainMenuStatic();
       updateMainMenuDynamic();
       menuInitialized = true;
       lastMenuItem = curMenuItem;
    }

    if (curMenuItem != lastMenuItem) {
       updateMainMenuDynamic();
       lastMenuItem = curMenuItem;
    }
   
    int menuAction = processMainMenu();
    if (menuAction == 1) {
       menuInitialized = false;
       if (curMenuItem < tubeCount) {
          activeTube = currentTubeDB[curMenuItem];
          guiMode = GUI_BIAS_MODE;
          tft.fillScreen(ST7735_BLACK);
          displayBiasScreenLayout();
          FirstBiasRun = true;
          lastRedrawTime = millis();
       }

       else if (curMenuItem == tubeCount) {
          strcpy(activeTube.name, "RAW");
          activeTube.maxDiss = 100;
          activeTube.screenFactor = 0.0;
          guiMode = GUI_BIAS_MODE;
          tft.fillScreen(ST7735_BLACK);
          displayBiasScreenLayout();
          FirstBiasRun = true;
          lastRedrawTime = millis();
       }

       else if (curMenuItem == tubeCount + 1) {
          guiMode = GUI_TUBE_MANAGER;
          resetMgrState = true;
          tft.fillScreen(ST7735_BLACK);
       }
       else if (curMenuItem == tubeCount + 2) {
          guiMode = GUI_CALIBRATION_MODE;
          tft.fillScreen(ST7735_BLACK);
       }
    }
  }

  else if (guiMode == GUI_BIAS_MODE) {
    unsigned long currentTime = millis();
    if (currentTime - lastRedrawTime >= redrawInterval) {
        doBias();
        lastRedrawTime = currentTime;
    }
    if (digitalRead(guiClick) == LOW) {
       delay(20);
       while(digitalRead(guiClick) == LOW);
       guiMode = GUI_MENU_SELECT_MODE;
    }
  }

  else if (guiMode == GUI_CALIBRATION_MODE) doCalibration();
  else if (guiMode == GUI_TUBE_MANAGER) doTubeManager();
  delay(10);
}
// =========================================================================
// DATA & EEPROM & CHECKSUM
// =========================================================================
uint8_t calculateChecksum(CalibrationData &data) {
  uint8_t sum = 0;
  const uint8_t *p = (const uint8_t *)&data;
  // Sum everything EXCEPT the last byte (which is the checksum itself)
  for (unsigned int i = 0; i < sizeof(CalibrationData) - 1; i++) {
    sum += p[i];
  }
  return sum;
}
void loadData() {
  int eeAddress = 0;
  EEPROM.get(eeAddress, calData);
  eeAddress += sizeof(CalibrationData);
  // Validate ID AND Checksum
  uint8_t calcSum = calculateChecksum(calData);
 
  if (calData.id != EEPROM_ID || calData.checksum != calcSum) {
    // DATA CORRUPT OR NEW CHIP -> LOAD DEFAULTS
    calData.id = EEPROM_ID;
    calData.vScaleA = 10.001;
    calData.vScaleB = 10.001;
    calData.rShuntA = 1.00;
    calData.rShuntB = 1.00;
    calData.voltLimit = 600.0;
    calData.checksum = calculateChecksum(calData); // Calculate initial sum
    // --- DEFAULT TUBE LIST ---
    tubeCount = 6;
    strcpy(currentTubeDB[0].name, "6L6GC"); currentTubeDB[0].maxDiss = 30; currentTubeDB[0].screenFactor = 0.055;
    strcpy(currentTubeDB[1].name, "EL34"); currentTubeDB[1].maxDiss = 25; currentTubeDB[1].screenFactor = 0.13;
    strcpy(currentTubeDB[2].name, "6V6"); currentTubeDB[2].maxDiss = 14; currentTubeDB[2].screenFactor = 0.045;
    strcpy(currentTubeDB[3].name, "EL84"); currentTubeDB[3].maxDiss = 12; currentTubeDB[3].screenFactor = 0.05;
    strcpy(currentTubeDB[4].name, "KT88"); currentTubeDB[4].maxDiss = 42; currentTubeDB[4].screenFactor = 0.06;
    strcpy(currentTubeDB[5].name, "6550"); currentTubeDB[5].maxDiss = 42; currentTubeDB[5].screenFactor = 0.06;
    EEPROM.put(0, calData);
    eeAddress = sizeof(CalibrationData);
    EEPROM.put(eeAddress, tubeCount);
    eeAddress += sizeof(int);
    for(int i=0; i<MAX_TUBES; i++) {
       EEPROM.put(eeAddress, currentTubeDB[i]);
       eeAddress += sizeof(TubeData);
    }
  } else {
    // CALIBRATION VALID, LOAD TUBES
    EEPROM.get(eeAddress, tubeCount);
   
    // SAFETY CLAMP
    if (tubeCount > MAX_TUBES) tubeCount = MAX_TUBES;
    if (tubeCount < 0) tubeCount = 0;
    eeAddress += sizeof(int);
    for(int i=0; i<tubeCount; i++) {
       EEPROM.get(eeAddress, currentTubeDB[i]);
       eeAddress += sizeof(TubeData);
    }
  }
}
void saveCalibration() {
  calData.checksum = calculateChecksum(calData); // Update Sum
  EEPROM.put(0, calData);
}
void saveTubeDB() {
  int eeAddress = sizeof(CalibrationData);
  EEPROM.put(eeAddress, tubeCount);
  eeAddress += sizeof(int);
  for(int i=0; i<tubeCount; i++) {
     EEPROM.put(eeAddress, currentTubeDB[i]);
     eeAddress += sizeof(TubeData);
  }
}
// =========================================================================
// INPUT HANDLER
// =========================================================================
int getInputsV10() {
  static unsigned long lastInputTime = 0;
 
  if (millis() - lastInputTime < 300) return 0;
  bool pressed = false;
  bool sawCenter = false;
  int cleanDirectional = 0;
  unsigned long pressStart = 0;
  while (true) {
    
    bool L = (digitalRead(guiLClick) == LOW);
    bool R = (digitalRead(guiRClick) == LOW);
    bool C = (digitalRead(guiClick) == LOW);
    if (L || R || C) {
      if (!pressed) {
        pressed = true;
        pressStart = millis();
        sawCenter = false;
        cleanDirectional = 0;
      }
      if (C) sawCenter = true;
      if (!sawCenter) {
        if (L && !R) cleanDirectional = 1;
        else if (R && !L) cleanDirectional = 2;
        else cleanDirectional = 0;
      } else {
        cleanDirectional = 0;
      }
    } else {
      if (pressed) {
        if (millis() - pressStart < 80) { pressed = false; continue; }
        delay(20);
        if (sawCenter) { lastInputTime = millis(); return 3; }
        if (cleanDirectional == 1) { lastInputTime = millis(); return 1; }
        else if (cleanDirectional == 2) { lastInputTime = millis(); return 2; }
        return 0;
      }
    }
   
    if (pressed && (millis() - pressStart > 2500)) pressed = false;
    delay(8);
  }
}
// =========================================================================
// TUBE MANAGER
// =========================================================================
void drawCursor(int fieldIdx, uint16_t color) {
    tft.setTextSize(1); tft.setTextColor(color, ST7735_BLACK);
    if (fieldIdx == 0) { tft.setCursor(5, 35); tft.print(F(">")); }
    if (fieldIdx == 1) { tft.setCursor(5, 65); tft.print(F(">")); }
    if (fieldIdx == 2) { tft.setCursor(5, 80); tft.print(F(">")); }
    if (fieldIdx == 8) { tft.setCursor(50, 100); tft.print(F(">")); }
}
void doTubeManager() {
  static int mgrState = 0;
  static int mgrMenuPos = 0;
  static int lastMgrMenuPos = -1;
  static int selectedTubeIdx = 0;
  static int lastSelectedTubeIdx = -1;
  static bool fullRedrawNeeded = true;
 
  // RESET STATE IF REQUESTED
  if (resetMgrState) {
      mgrState = 0;
      mgrMenuPos = 0;
      fullRedrawNeeded = true;
      resetMgrState = false;
  }
 
  static TubeData tempTube;
  static int editField = 0;
  static int lastEditField = -1;
  static bool inNameCharEdit = false;
  static int nameCharIdx = 0;
  static char charSet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789- ";
  // --- STATE 0: ACTION MENU ---
  if (mgrState == 0) {
    if (fullRedrawNeeded) {
       tft.fillScreen(ST7735_BLACK);
       tft.setTextSize(1);
       tft.setTextColor(ST7735_YELLOW); tft.setCursor(45, 10); tft.print(F("MANAGE"));
       tft.setTextColor(ST7735_WHITE);
       tft.setCursor(40, 40); tft.print(F("ADD NEW"));
       tft.setCursor(40, 60); tft.print(F("EDIT TUBE"));
       tft.setCursor(40, 80); tft.print(F("DELETE"));
       tft.setCursor(40, 100); tft.print(F("EXIT"));
       lastMgrMenuPos = -1;
       fullRedrawNeeded = false;
    }
   
    if (mgrMenuPos != lastMgrMenuPos) {
       tft.setTextSize(1);
       if(lastMgrMenuPos != -1) {
          tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
          int y = 40 + (lastMgrMenuPos * 20);
          tft.setCursor(40, y);
          if(lastMgrMenuPos==0) tft.print(F("ADD NEW"));
          if(lastMgrMenuPos==1) tft.print(F("EDIT TUBE"));
          if(lastMgrMenuPos==2) tft.print(F("DELETE"));
          if(lastMgrMenuPos==3) tft.print(F("EXIT"));
       }
       tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
       int y = 40 + (mgrMenuPos * 20);
       tft.setCursor(40, y);
       if(mgrMenuPos==0) tft.print(F("ADD NEW"));
       if(mgrMenuPos==1) tft.print(F("EDIT TUBE"));
       if(mgrMenuPos==2) tft.print(F("DELETE"));
       if(mgrMenuPos==3) tft.print(F("EXIT"));
       lastMgrMenuPos = mgrMenuPos;
    }
   
    int input = getInputsV10();
    if (input == 1) { mgrMenuPos--; if(mgrMenuPos<0) mgrMenuPos=3; }
    if (input == 2) { mgrMenuPos++; if(mgrMenuPos>3) mgrMenuPos=0; }
   
    if (input == 3) {
       if (mgrMenuPos == 0) { // ADD
          if (tubeCount >= MAX_TUBES) {
             tft.fillScreen(ST7735_RED); tft.setTextSize(2); tft.setCursor(20,60); tft.print(F("FULL!")); delay(1000); fullRedrawNeeded=true;
          } else {
             strcpy(tempTube.name, "USER ");
             tempTube.maxDiss = 25;
             tempTube.screenFactor = 0.05;
             selectedTubeIdx = tubeCount;
             mgrState = 2; editField = 0; lastEditField = -1; inNameCharEdit = false; fullRedrawNeeded = true;
          }
       }
       else if (mgrMenuPos == 1) { // EDIT
          if (tubeCount == 0) {
             tft.fillScreen(ST7735_RED); tft.setTextSize(2); tft.setCursor(20,60); tft.print(F("NO TUBES")); delay(1000); fullRedrawNeeded=true;
          } else {
             mgrState = 1; selectedTubeIdx = 0; lastSelectedTubeIdx = -1; fullRedrawNeeded = true;
          }
       }
       else if (mgrMenuPos == 2) { // DELETE
          if (tubeCount == 0) {
             tft.fillScreen(ST7735_RED); tft.setTextSize(2); tft.setCursor(20,60); tft.print(F("NO TUBES")); delay(1000); fullRedrawNeeded=true;
          } else {
             mgrState = 3; selectedTubeIdx = 0; lastSelectedTubeIdx = -1; fullRedrawNeeded = true;
          }
       }
       else {
          guiMode = GUI_MENU_SELECT_MODE;
          tft.fillScreen(ST7735_BLACK);
          lastMenuItem=-1; mgrState=0; fullRedrawNeeded=true;
       }
    }
  }
  // --- STATE 1: SELECT TUBE TO EDIT ---
  else if (mgrState == 1) {
    if (fullRedrawNeeded) {
       tft.fillScreen(ST7735_BLACK);
       tft.setTextSize(1);
       tft.setTextColor(ST7735_CYAN); tft.setCursor(30, 10); tft.print(F("EDIT WHICH?"));
       tft.setTextColor(ST7735_WHITE);
       tft.setCursor(40, 100); tft.print(F("Click to Select"));
       lastSelectedTubeIdx = -1;
       fullRedrawNeeded = false;
    }
    if (selectedTubeIdx != lastSelectedTubeIdx) {
       tft.fillRect(0, 50, 160, 20, ST7735_BLACK);
       tft.setTextColor(ST7735_WHITE); tft.setTextSize(2);
       tft.setCursor(44, 50); tft.print(currentTubeDB[selectedTubeIdx].name);
       lastSelectedTubeIdx = selectedTubeIdx;
    }
    int input = getInputsV10();
    if (input == 1) { selectedTubeIdx--; if(selectedTubeIdx<0) selectedTubeIdx=tubeCount-1; }
    if (input == 2) { selectedTubeIdx++; if(selectedTubeIdx>=tubeCount) selectedTubeIdx=0; }
    if (input == 3) {
       tempTube = currentTubeDB[selectedTubeIdx];
       mgrState = 2; editField = 0; lastEditField = -1; inNameCharEdit = false; fullRedrawNeeded = true;
    }
  }
  // --- STATE 3: SELECT TUBE TO DELETE ---
  else if (mgrState == 3) {
    if (fullRedrawNeeded) {
       tft.fillScreen(ST7735_BLACK);
       tft.setTextSize(1);
       tft.setTextColor(ST7735_RED); tft.setCursor(30, 10); tft.print(F("DELETE WHICH?"));
       tft.setTextColor(ST7735_WHITE);
       tft.setCursor(40, 100); tft.print(F("Click to Delete"));
       lastSelectedTubeIdx = -1;
       fullRedrawNeeded = false;
    }
    if (selectedTubeIdx != lastSelectedTubeIdx) {
       tft.fillRect(0, 50, 160, 20, ST7735_BLACK);
       tft.setTextColor(ST7735_WHITE); tft.setTextSize(2);
       tft.setCursor(44, 50); tft.print(currentTubeDB[selectedTubeIdx].name);
       lastSelectedTubeIdx = selectedTubeIdx;
    }
    int input = getInputsV10();
    if (input == 1) { selectedTubeIdx--; if(selectedTubeIdx<0) selectedTubeIdx=tubeCount-1; }
    if (input == 2) { selectedTubeIdx++; if(selectedTubeIdx>=tubeCount) selectedTubeIdx=0; }
    if (input == 3) {
       mgrState = 4; fullRedrawNeeded = true;
    }
  }
  // --- STATE 4: DELETE CONFIRMATION ---
  else if (mgrState == 4) {
      if (fullRedrawNeeded) {
          tft.fillScreen(ST7735_RED);
          tft.setTextColor(ST7735_WHITE); tft.setTextSize(2);
          tft.setCursor(25, 30); tft.print(F("CONFIRM?"));
          tft.setTextSize(1);
          tft.setCursor(20, 60); tft.print(F("Click: DELETE"));
          tft.setCursor(20, 80); tft.print(F("L/R: CANCEL"));
          fullRedrawNeeded = false;
      }
      int input = getInputsV10();
      if (input == 3) { // Center = CONFIRMED DELETE
          for(int i=selectedTubeIdx; i<tubeCount-1; i++) { currentTubeDB[i] = currentTubeDB[i+1]; }
          tubeCount--; saveTubeDB();
          tft.fillScreen(ST7735_BLACK); tft.setTextSize(2); tft.setTextColor(ST7735_RED);
          tft.setCursor(40,60); tft.print(F("DELETED")); delay(1000);
          mgrState = 0; fullRedrawNeeded = true;
      }
      else if (input == 1 || input == 2) { // L/R = CANCEL
          mgrState = 0; fullRedrawNeeded = true;
      }
  }
  // --- STATE 2: EDITOR ---
  else if (mgrState == 2) {
    bool forceRender = fullRedrawNeeded;
    if (fullRedrawNeeded) {
       tft.fillScreen(ST7735_BLACK);
       tft.setTextSize(1);
       tft.setTextColor(ST7735_YELLOW); tft.setCursor(40, 5); tft.print(F("EDITOR"));
       tft.setTextColor(ST7735_GREY);
       tft.setCursor(20, 65); tft.print(F("Watts:"));
       tft.setCursor(20, 80); tft.print(F("Scrn%:"));
       tft.setTextSize(2);
       for(int i=0; i<6; i++) {
          int x = 20 + (i*15);
          tft.setCursor(x, 30);
          tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
          tft.print(tempTube.name[i]);
       }
       tft.setTextSize(1);
      
       tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
       tft.setCursor(60, 65); tft.print(tempTube.maxDiss); tft.print(F("W"));
       tft.setCursor(60, 80); tft.print(tempTube.screenFactor * 100.0, 1); tft.print(F("%"));
      
       tft.setCursor(60, 100); tft.print(F("[SAVE]"));
      
       drawCursor(editField, ST7735_CYAN);
       forceRender = true;
       fullRedrawNeeded = false;
    }
   
    int input = getInputsV10();
    bool changed = false;
   
    if (input == 3) {
        if (editField == 8) {
             tempTube.name[6] = '\0';
             if (mgrMenuPos == 0) {
                 if (tubeCount < MAX_TUBES) {
                    currentTubeDB[tubeCount] = tempTube;
                    tubeCount++;
                 }
             }
             else {
                 currentTubeDB[selectedTubeIdx] = tempTube;
             }
             saveTubeDB();
             tft.fillScreen(ST7735_GREEN); tft.setTextSize(2); tft.setCursor(50,50); tft.setTextColor(ST7735_BLACK, ST7735_GREEN); tft.print(F("SAVED"));
             delay(1000);
             mgrState = 0; fullRedrawNeeded = true;
             return;
        }
        else if (editField == 0) {
             if (!inNameCharEdit) { inNameCharEdit = true; nameCharIdx = 0; }
             else { nameCharIdx++; if (nameCharIdx > 5) { inNameCharEdit = false; nameCharIdx = 0; } }
             forceRender = true;
        }
        else {
             drawCursor(editField, ST7735_BLACK);
             if (editField == 1) editField = 2; else if (editField == 2) editField = 8;
             drawCursor(editField, ST7735_CYAN);
             forceRender = true;
        }
    }
    else if (input == 1 || input == 2) {
        if (inNameCharEdit && editField == 0) { changed = true; }
        else if (editField == 1 || editField == 2) { changed = true; }
        else {
            drawCursor(editField, ST7735_BLACK);
            if (input == 1) { if(editField==0) editField=8; else if(editField==1) editField=0; else if(editField==2) editField=1; else editField=2; }
            else { if(editField==0) editField=1; else if(editField==1) editField=2; else if(editField==2) editField=8; else editField=0; }
            drawCursor(editField, ST7735_CYAN);
        }
    }
    if (changed) {
       if (editField == 0 && inNameCharEdit) {
          char c = tempTube.name[nameCharIdx];
          char *p = strchr(charSet, c);
          int idx = (p) ? (p - charSet) : 0;
          if (input == 1) idx--; else idx++;
          if (idx < 0) idx = strlen(charSet)-1; if (idx >= (int)strlen(charSet)) idx = 0;
          tempTube.name[nameCharIdx] = charSet[idx];
       }
       else if (editField == 1) {
          if(input == 1) tempTube.maxDiss--; else tempTube.maxDiss++;
          if(tempTube.maxDiss < 1) tempTube.maxDiss = 1; if(tempTube.maxDiss > 100) tempTube.maxDiss = 100;
       }
       else if (editField == 2) {
          if(input == 1) tempTube.screenFactor -= 0.005; else tempTube.screenFactor += 0.005;
          if(tempTube.screenFactor < 0) tempTube.screenFactor = 0; if(tempTube.screenFactor > 0.3) tempTube.screenFactor = 0.3;
       }
       forceRender = true;
    }
    // --- RENDERING (ABBREVIATED FOR CLARITY - Logic unchanged) ---
    tft.setTextSize(2);
    for(int i=0; i<6; i++) {
       if (forceRender || (changed && editField==0)) {
          int x = 20 + (i*15);
          tft.fillRect(x, 30, 12, 20, ST7735_BLACK);
          uint16_t c = (editField==0 && inNameCharEdit && i==nameCharIdx) ? ST7735_RED : ST7735_WHITE;
          tft.setTextColor(c, ST7735_BLACK);
          tft.setCursor(x, 30); tft.print(tempTube.name[i]);
          tft.fillRect(x, 45, 12, 5, ST7735_BLACK);
          if (editField == 0 && inNameCharEdit && i == nameCharIdx) { tft.setCursor(x, 45); tft.print(F("^")); }
       }
    }
    tft.setTextSize(1);
    if (forceRender || (changed && editField==1)) {
       uint16_t c = (editField == 1) ? ST7735_RED : ST7735_WHITE;
       tft.setTextColor(c, ST7735_BLACK);
       tft.fillRect(60, 65, 50, 10, ST7735_BLACK);
       tft.setCursor(60, 65); tft.print(tempTube.maxDiss); tft.print(F("W"));
    }
    if (forceRender || (changed && editField==2)) {
       uint16_t c = (editField == 2) ? ST7735_RED : ST7735_WHITE;
       tft.setTextColor(c, ST7735_BLACK);
       tft.fillRect(60, 80, 50, 10, ST7735_BLACK);
       tft.setCursor(60, 80); tft.print(tempTube.screenFactor * 100.0, 1); tft.print(F("%"));
    }
    if (forceRender) {
       uint16_t c = (editField == 8) ? ST7735_GREEN : ST7735_WHITE;
       tft.setTextColor(c, ST7735_BLACK);
       tft.setCursor(60, 100); tft.print(F("[SAVE]"));
    }
    lastEditField = editField;
  }
}
// =========================================================================
// MENU
// =========================================================================
int processMainMenu() {
  int totalItems = tubeCount + 3;
  int result = 0;
  int input = getInputsV10();
 
  if (input == 1) { // Left
    movedMenu = true;
    curMenuItem--; if (curMenuItem < 0) curMenuItem = totalItems - 1;
    result = -1;
  }
  else if (input == 2) { // Right
    movedMenu = true;
    curMenuItem++; if (curMenuItem >= totalItems) curMenuItem = 0;
    result = 2;
  }
  else if (input == 3) { // Center
    result = 1;
  }
  else {
    movedMenu = false;
  }
  return result;
}
void drawMainMenuStatic() {
  tft.setCursor(17, 0); tft.setTextColor(ST7735_WHITE, ST7735_BLACK); tft.setTextSize(2);
  tft.println(F("Select Tube"));
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK); tft.setTextSize(1);
  tft.setCursor(20, 58); tft.print(F("< "));
  tft.setCursor(120, 58); tft.print(F(" >"));
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK); tft.setCursor(30, 115);
  tft.println(F("Click to select."));
}
void updateMainMenuDynamic() {
  tft.fillRect(32, 58, 88, 20, ST7735_BLACK);
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK); tft.setTextSize(1);
 
  if (curMenuItem < tubeCount) {
     tft.setCursor(35, 58);
     tft.print(currentTubeDB[curMenuItem].name);
     tft.print(F(" "));
     tft.print(currentTubeDB[curMenuItem].maxDiss);
     tft.print(F("W"));
  } else if (curMenuItem == tubeCount) {
     tft.setCursor(50, 58); tft.print(F("RAW MODE"));
  } else if (curMenuItem == tubeCount + 1) {
     tft.setCursor(40, 58); tft.print(F("TUBE MANAGER"));
  } else {
     tft.setCursor(45, 58); tft.print(F("CAL SETUP"));
  }
}
// =========================================================================
// CALIBRATION LOGIC!
// =========================================================================
void doCalibration() {
  static int calItem = 0;
  static int lastCalItem = -1;
  static float lastVal = -999;
  static unsigned long holdStartTime = 0;
  static unsigned long lastActionTime = 0;
  static bool isHolding = false;
 
  long totalReading = 0;
  int sampleCount = 100;
  for(int i=0; i<sampleCount; i++) {
      if (calItem == 0) totalReading += ads.readADC_SingleEnded(1);
      else if (calItem == 1) totalReading += ads.readADC_SingleEnded(3);
      else if (calItem == 2) totalReading += ads.readADC_SingleEnded(0);
      else if (calItem == 3) totalReading += ads.readADC_SingleEnded(2);
      else totalReading += ads.readADC_SingleEnded(1);
  }
  int16_t reading = totalReading / sampleCount;
  float *valToMod;
  float step = 0.01; float turboStep = 0.1;
  if (calItem == 0) valToMod = &calData.vScaleA;
  else if (calItem == 1) valToMod = &calData.vScaleB;
  else if (calItem == 2) valToMod = &calData.rShuntA;
  else if (calItem == 3) valToMod = &calData.rShuntB;
  else { valToMod = &calData.voltLimit; step = 1.0; turboStep = 10.0; }
  // Calibration uses standard button logic to allow "holding" for fast scroll
  bool leftPressed = (digitalRead(guiLClick) == LOW);
  bool rightPressed = (digitalRead(guiRClick) == LOW);
 
  if (leftPressed || rightPressed) {
    unsigned long now = millis();
    if (!isHolding) { isHolding = true; holdStartTime = now; lastActionTime = 0; }
   
    int delayTime = 250; float currentStep = step;
    if (now - holdStartTime > 800) { currentStep = turboStep; delayTime = 50; }
   
    if (now - lastActionTime > delayTime) {
      if (leftPressed) *valToMod -= currentStep;
      if (rightPressed) *valToMod += currentStep;
      // --- SAFETY CLAMPS (New in V11.3) ---
      if (calItem < 2) { // Voltage Scale (5x to 20x)
          if (*valToMod < 5.0) *valToMod = 5.0;
          if (*valToMod > 20.0) *valToMod = 20.0;
      }
      else if (calItem < 4) { // Shunt Resistor (0.5 to 5.0 ohms)
          if (*valToMod < 0.5) *valToMod = 0.5;
          if (*valToMod > 5.0) *valToMod = 5.0;
      }
      else { // Voltage Limit (300V to 800V)
          if (*valToMod < 300.0) *valToMod = 300.0;
          if (*valToMod > 800.0) *valToMod = 800.0;
      }
     
      lastActionTime = now;
    }
  } else { isHolding = false; }
  if (calItem != lastCalItem) {
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(32, 10); tft.setTextSize(1); tft.setTextColor(ST7735_YELLOW, ST7735_BLACK); tft.print(F("CALIBRATION MODE"));
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
   
    switch(calItem) {
      case 0: tft.setCursor(32, 30); tft.print(F("Adj Volt Scale A")); break;
      case 1: tft.setCursor(32, 30); tft.print(F("Adj Volt Scale B")); break;
      case 2: tft.setCursor(35, 30); tft.print(F("Adj Shunt Res A")); break;
      case 3: tft.setCursor(35, 30); tft.print(F("Adj Shunt Res B")); break;
      case 4: tft.setCursor(29, 30); tft.print(F("Adj Voltage Limit")); break;
    }
    lastVal = -999; lastCalItem = calItem;
  }
  if (abs(*valToMod - lastVal) > 0.00001) {
    tft.setTextSize(2); tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
    if (calItem == 4) { tft.setCursor(62, 55); tft.print(*valToMod, 0); }
    else { tft.setCursor(50, 55); tft.print(*valToMod, 2); }
    lastVal = *valToMod;
  }
 
  tft.setTextSize(1); tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  if (calItem < 2) {
    float calcV = ((float)reading * mV_per_bit) * (*valToMod);
    tft.setCursor(38, 85); tft.print(F("Live V: ")); tft.print(calcV, 1); tft.print(F("V "));
  } else if (calItem < 4) {
    float calcmA = ((float)reading * mV_per_bit) / (*valToMod);
    tft.setCursor(38, 85); tft.print(F("Live I: ")); tft.print(calcmA, 1); tft.print(F("mA "));
  } else {
    int16_t refRead = ads.readADC_SingleEnded(1);
    float liveV = ((float)refRead * mV_per_bit) * calData.vScaleA;
    tft.setCursor(38, 85); tft.print(F("Live A: ")); tft.print(liveV, 1); tft.print(F("V "));
  }
  if (buttonClicked()) {
    calItem++; lastCalItem = -1;
    if (calItem > 4) {
      calItem = 0;
      saveCalibration(); // USES NEW CHECKSUM SAVE
      tft.fillScreen(ST7735_GREEN); tft.setTextColor(ST7735_BLACK); tft.setTextSize(2);
      tft.setCursor(44, 55); tft.print(F("SAVED!")); delay(1000);
      guiMode = GUI_MENU_SELECT_MODE; tft.fillScreen(ST7735_BLACK); lastMenuItem = -1;
    }
  }
}
// =========================================================================
// BIAS LOOP
// =========================================================================
void doBias() {
  long total_adc0_A = 0, total_adc1_A = 0, total_adc2_B = 0, total_adc3_B = 0;
 
  for (int i = 0; i < NUM_SAMPLES; i++) {
    total_adc0_A += ads.readADC_SingleEnded(0);
    total_adc1_A += ads.readADC_SingleEnded(1);
    total_adc2_B += ads.readADC_SingleEnded(2);
    total_adc3_B += ads.readADC_SingleEnded(3);
  }

  VoltageA = (((float)total_adc1_A / NUM_SAMPLES) * mV_per_bit) * calData.vScaleA;
  float rawCathodeA = (total_adc0_A / NUM_SAMPLES > ADC_NOISE_THRESHOLD) ?
                      ((float)(total_adc0_A / NUM_SAMPLES) * mV_per_bit) / calData.rShuntA : 0.0;
 

  VoltageB = (((float)total_adc3_B / NUM_SAMPLES) * mV_per_bit) * calData.vScaleB;
   float rawCathodeB = (total_adc2_B / NUM_SAMPLES > ADC_NOISE_THRESHOLD) ?
                      ((float)(total_adc2_B / NUM_SAMPLES) * mV_per_bit) / calData.rShuntB : 0.0;
  checkAndHandleSafety(VoltageA, VoltageB);
  CurrentA = rawCathodeA; CurrentB = rawCathodeB;
 
  if (activeTube.screenFactor > 0.0) {
      float factor = activeTube.screenFactor;
      CurrentA -= (rawCathodeA * factor);
      CurrentB -= (rawCathodeB * factor);
  }
 
  if (CurrentA < 0) CurrentA = 0; if (VoltageA < 0) VoltageA = 0;
  if (CurrentB < 0) CurrentB = 0; if (VoltageB < 0) VoltageB = 0;
 
  PowerA = (CurrentA * VoltageA) / 1000.0;
  PowerB = (CurrentB * VoltageB) / 1000.0;
 
  float maxDiss = (float)activeTube.maxDiss;
  float WattPercA = (maxDiss > 0) ? (PowerA / maxDiss) * 100.0 : 0;
  float WattPercB = (maxDiss > 0) ? (PowerB / maxDiss) * 100.0 : 0;
 
  CurrentDiff = abs(CurrentA - CurrentB);
  drawMetric(25, 45, VoltageA, VoltageAPrevious, " ", ST7735_WHITE);
  drawMetric(25, 61, CurrentA, CurrentAPrevious, " ", ST7735_WHITE);
  drawMetric(25, 77, PowerA, PowerAPrevious, " ", ST7735_WHITE);
  uint16_t colorA = ST7735_WHITE;
  if (WattPercA > 80.0) colorA = ST7735_RED; else if (WattPercA > 70.0) colorA = ST7735_YELLOW; else if (WattPercA < 50.0) colorA = ST7735_CYAN;
  drawMetric(25, 93, WattPercA, WattPercAPrevious, "% ", colorA);
  drawMetric(110, 45, VoltageB, VoltageBPrevious, " ", ST7735_WHITE);
  drawMetric(110, 61, CurrentB, CurrentBPrevious, " ", ST7735_WHITE);
  drawMetric(110, 77, PowerB, PowerBPrevious, " ", ST7735_WHITE);
  uint16_t colorB = ST7735_WHITE;
  if (WattPercB > 80.0) colorB = ST7735_RED; else if (WattPercB > 70.0) colorB = ST7735_YELLOW; else if (WattPercB < 50.0) colorB = ST7735_CYAN;
  drawMetric(110, 93, WattPercB, WattPercBPrevious, "% ", colorB);
 
  drawMetric(90, 115, CurrentDiff, CurrentDiffPrevious, " ", ST7735_WHITE);
  FirstBiasRun = false;
}
void drawMetric(int x, int y, float val, float &prevVal, char* suffix, uint16_t color) {
  if (FirstBiasRun || (int)(val * 10) != (int)(prevVal * 10)) {
      tft.setCursor(x, y); tft.setTextColor(color, ST7735_BLACK);
      tft.print(val, 1); if (suffix) tft.print(suffix); tft.print(" "); prevVal = val;
  }
}
// =========================================================================
// SAFETY LOGIC
// =========================================================================
void checkAndHandleSafety(float vA, float vB) {
  float userLimit = calData.voltLimit;
  float safeLevel = userLimit - 50.0;
  if (vA > userLimit || vB > userLimit) {
    char badProbe = (vA > vB) ? 'A' : 'B';
    float badVolts = (vA > vB) ? vA : vB;
   
    tft.fillScreen(ST7735_RED); tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(2); tft.setCursor(38, 25); tft.println(F("DANGER!"));
    tft.setTextSize(1); tft.setCursor(44, 50); tft.print(F("OVER VOLTAGE"));
    tft.setTextSize(2); tft.setCursor(38, 70); tft.print(F("PROBE ")); tft.print(badProbe);
    tft.setCursor(38, 95); tft.print(badVolts, 1); tft.print(F("V"));
   
    delay(2000);
    while (true) {
      
      int16_t rA = ads.readADC_SingleEnded(1); int16_t rB = ads.readADC_SingleEnded(3);
      float curVA = ((float)rA * mV_per_bit) * calData.vScaleA;
      float curVB = ((float)rB * mV_per_bit) * calData.vScaleB;
     
      if (curVA < safeLevel && curVB < safeLevel) {
        tft.fillScreen(ST7735_BLACK); displayBiasScreenLayout(); FirstBiasRun = true; return;
      }
      tft.invertDisplay(true); delay(500); tft.invertDisplay(false); delay(500);
    }
  }
}
// =========================================================================
// SPLASH SCREEN
// =========================================================================
void displaySplashScreen() {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE); tft.setTextSize(2);
  tft.setCursor(20, 40); tft.println(F("Bias Meter")); delay(500);
  tft.setTextColor(ST7735_CYAN); tft.setTextSize(1);
  tft.setCursor(45, 64); tft.println(F("Tone Alchemy"));
  tft.setTextColor(ST7735_MAGENTA); tft.setCursor(60, 80);
  tft.println(F("v11.4"));

  // --- New code to display the attribution URL ---
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(10, 100);
  tft.println("Credits/License Info:");
  tft.setCursor(5, 112);
  tft.println("tonealchemy.com/biasmeter");
  delay(3000);
  tft.fillScreen(ST7735_BLACK);
}

boolean buttonClicked() {
  int reading = digitalRead(guiClick);
  static int lastState = HIGH;
  if (reading != lastState) { if (reading == LOW) { lastState = LOW; return true; } }
  lastState = reading; return false;
}
// =========================================================================
// BIAS LIVE READING SCREEN
// =========================================================================
void displayBiasScreenLayout() {
  tft.setCursor(0, 0); tft.setTextColor(ST7735_CYAN); tft.setTextSize(1);
  tft.print(activeTube.name);
 
  tft.setCursor(45, 0); tft.print(F("50% ")); tft.print(round(activeTube.maxDiss * 0.5));
  tft.print(F(" | 60% ")); tft.print(round(activeTube.maxDiss * 0.6));
  tft.setCursor(45, 10); tft.print(F("70% ")); tft.print(round(activeTube.maxDiss * 0.7));
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(5, 25); tft.println(F("A TUBE")); tft.setCursor(0, 33); tft.println(F("--------------------------"));
  tft.setCursor(90, 25); tft.println(F("B TUBE"));
  tft.setCursor(5, 45); tft.print(F("V: ")); tft.setCursor(5, 61); tft.print(F("mA: "));
  tft.setCursor(5, 77); tft.print(F("W: ")); tft.setCursor(5, 93); tft.print(F("%:"));
  tft.setCursor(90, 45); tft.print(F("V: ")); tft.setCursor(90, 61); tft.print(F("mA: "));
  tft.setCursor(90, 77); tft.print(F("W: ")); tft.setCursor(90, 93); tft.print(F("%:"));
  tft.setCursor(0, 105); tft.println(F("--------------------------"));
  tft.setCursor(45, 115); tft.println(F("Delta:"));
}