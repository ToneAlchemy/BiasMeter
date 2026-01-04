// This work, "Bias Meter V11.3 - PLATINUM" by Charlie Isaac (charlieisaac@protonmail.com) / https://ToneAlchemy.com/biasmeter / https://github.com/ToneAlchemists/BiasMeter (CC BY 4.0)
//
// ORIGINAL ATTRIBUTION:
// Based on "ArduinoBiasMeter" by John Wagner (mrjohnwagner@gmail.com)
// and "Dual Channel Arduino Bias Tester" by Kiel Lydestad (3DBeerGoggles).
// Licensed under Creative Commons Attribution 4.0 International (CC BY 4.0).
//

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ADS1X15.h>
#include <EEPROM.h> 

// --- PIN DEFINITIONS (Arduino Nano) ---
#define SCLK 13
#define MOSI 11
#define CS   10
#define DC   9
#define RST  8

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
boolean buttonClicked();
boolean debouncedButtonPressed(int inputPin);
int getInputsV10(); 
void drawCursor(int fieldIdx, uint16_t color);
void displaySplashScreen();
void displayBiasScreenLayout();
void checkAndHandleSafety(float vA, float vB);

// =========================================================================
// SETUP
// =========================================================================
void setup() {
  // Serial disabled to save memory
  // Serial.begin(9600);
  
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
       delay(20); // Added Debounce
       while(digitalRead(guiClick) == LOW); 
       guiMode = GUI_MENU_SELECT_MODE;
    }
  }
  
  else if (guiMode == GUI_CALIBRATION_MODE) doCalibration();
  else if (guiMode == GUI_TUBE_MANAGER) doTubeManager();
  
  delay(10);
}

// =========================================================================
// DATA & EEPROM
// =========================================================================
void loadData() {
  int eeAddress = 0;
  EEPROM.get(eeAddress, calData);
  eeAddress += sizeof(CalibrationData);

  if (calData.id != EEPROM_ID) {
    calData.id = EEPROM_ID;
    calData.vScaleA = 10.001; 
    calData.vScaleB = 10.001; 
    calData.rShuntA = 1.00; 
    calData.rShuntB = 1.00; 
    calData.voltLimit = 600.0; 
    
    tubeCount = 3;
    strcpy(currentTubeDB[0].name, "6L6GC"); currentTubeDB[0].maxDiss = 30; currentTubeDB[0].screenFactor = 0.055;
    strcpy(currentTubeDB[1].name, "EL34");  currentTubeDB[1].maxDiss = 25; currentTubeDB[1].screenFactor = 0.13;
    strcpy(currentTubeDB[2].name, "6V6");   currentTubeDB[2].maxDiss = 14; currentTubeDB[2].screenFactor = 0.045;
    
    EEPROM.put(0, calData);
    eeAddress = sizeof(CalibrationData);
    EEPROM.put(eeAddress, tubeCount);
    eeAddress += sizeof(int);
    for(int i=0; i<MAX_TUBES; i++) {
       EEPROM.put(eeAddress, currentTubeDB[i]);
       eeAddress += sizeof(TubeData);
    }
  } else {
    EEPROM.get(eeAddress, tubeCount);
    
    // CRITICAL SAFETY CLAMP
    if (tubeCount > MAX_TUBES) tubeCount = MAX_TUBES;
    if (tubeCount < 0) tubeCount = 0;
    
    eeAddress += sizeof(int);
    for(int i=0; i<tubeCount; i++) { // Only load what we have
       EEPROM.get(eeAddress, currentTubeDB[i]);
       eeAddress += sizeof(TubeData);
    }
  }
}

void saveTubeDB() {
  int eeAddress = sizeof(CalibrationData);
  EEPROM.put(eeAddress, tubeCount);
  eeAddress += sizeof(int);
  for(int i=0; i<tubeCount; i++) { // Only save valid tubes
     EEPROM.put(eeAddress, currentTubeDB[i]);
     eeAddress += sizeof(TubeData);
  }
}

// =========================================================================
// INPUT HANDLER
// =========================================================================
int getInputsV10() {
  static unsigned long lastInputTime = 0;

  // 1. HARD FREEZE (Ignores Recoil Bounce)
  if (millis() - lastInputTime < 300) {
    return 0;
  }

  bool pressed = false;
  bool sawCenter = false;
  int cleanDirectional = 0;  // 1=Left only, 2=Right only, 0=ambiguous

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
      // Released
      if (pressed) {
        if (millis() - pressStart < 80) {
          pressed = false;
          continue;
        }

        delay(20); 

        if (sawCenter) {
          lastInputTime = millis(); 
          return 3;
        }

        if (cleanDirectional == 1) {
          lastInputTime = millis(); 
          return 1;
        } else if (cleanDirectional == 2) {
          lastInputTime = millis(); 
          return 2;
        }

        return 0;
      }
    }

    if (pressed && (millis() - pressStart > 2500)) {
      pressed = false;
    }

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
             strcpy(tempTube.name, "USER  ");
             tempTube.maxDiss = 25;
             tempTube.screenFactor = 0.05;
             selectedTubeIdx = tubeCount; 
             mgrState = 2; editField = 0; lastEditField = -1; inNameCharEdit = false; fullRedrawNeeded = true;
          }
       }
       else if (mgrMenuPos == 1) { mgrState = 1; selectedTubeIdx = 0; lastSelectedTubeIdx = -1; fullRedrawNeeded = true; } 
       else if (mgrMenuPos == 2) { mgrState = 3; selectedTubeIdx = 0; lastSelectedTubeIdx = -1; fullRedrawNeeded = true; } 
       else { 
          guiMode = GUI_MENU_SELECT_MODE; 
          tft.fillScreen(ST7735_BLACK); 
          lastMenuItem=-1; mgrState=0; fullRedrawNeeded=true; 
       }
    }
  }
  
  // --- STATE 1/3: SELECT TUBE ---
  else if (mgrState == 1 || mgrState == 3) { 
    if (fullRedrawNeeded) {
       tft.fillScreen(ST7735_BLACK);
       tft.setTextSize(1); 
       tft.setTextColor(ST7735_CYAN); tft.setCursor(30, 10); 
       if(mgrState==1) tft.print(F("EDIT WHICH?")); else tft.print(F("DELETE WHICH?"));
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
       if (mgrState == 1) {
          tempTube = currentTubeDB[selectedTubeIdx]; 
          mgrState = 2; editField = 0; lastEditField = -1; inNameCharEdit = false; fullRedrawNeeded = true;
       } else {
          for(int i=selectedTubeIdx; i<tubeCount-1; i++) { currentTubeDB[i] = currentTubeDB[i+1]; }
          tubeCount--; saveTubeDB();
          tft.fillScreen(ST7735_RED); tft.setTextSize(2); tft.setCursor(40,60); tft.print(F("DELETED")); delay(1000);
          mgrState = 0; fullRedrawNeeded = true;
       }
    }
  }
  
  // --- STATE 2: EDITOR (FIXED VISIBILITY) ---
  else if (mgrState == 2) { 
    
    bool forceRender = fullRedrawNeeded;

    if (fullRedrawNeeded) {
       tft.fillScreen(ST7735_BLACK);
       tft.setTextSize(1); 
       tft.setTextColor(ST7735_YELLOW); tft.setCursor(40, 5); tft.print(F("EDITOR"));
       tft.setTextColor(ST7735_GREY);
       tft.setCursor(20, 65); tft.print(F("Watts:"));
       tft.setCursor(20, 80); tft.print(F("Scrn%:"));

       // --- FORCE DRAW NAME IMMEDIATELY ---
       tft.setTextSize(2);
       for(int i=0; i<6; i++) {
          int x = 20 + (i*15);
          tft.setCursor(x, 30);
          tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
          tft.print(tempTube.name[i]);
       }
       tft.setTextSize(1); // Reset for numbers below

       // --- FORCE DRAW NUMBERS IMMEDIATELY ---
       tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
       tft.setCursor(60, 65); tft.print(tempTube.maxDiss); tft.print(F("W"));
       tft.setCursor(60, 80); tft.print(tempTube.screenFactor * 100.0, 1); tft.print(F("%"));
       tft.setCursor(60, 100); tft.print(F("[SAVE]"));
       // -----------------------------------------------------------
       
       drawCursor(editField, ST7735_CYAN); 
       forceRender = true; 
       fullRedrawNeeded = false; 
    }
    
    int input = getInputsV10();

    bool changed = false;

    if (input == 3) { // CENTER = ADVANCE CURSOR
        if (editField == 8) { // SAVE
             // CRITICAL SAFETY CHECK
             if (mgrMenuPos == 0) { // ADD NEW
                 if (tubeCount < MAX_TUBES) { // Explicit guard
                    currentTubeDB[tubeCount] = tempTube; 
                    tubeCount++;
                 }
             } 
             else { // EDIT EXISTING
                 currentTubeDB[selectedTubeIdx] = tempTube; 
             }
             saveTubeDB();
             tft.fillScreen(ST7735_GREEN); 
             tft.setTextSize(2); 
             tft.setCursor(50,50); 
             tft.setTextColor(ST7735_BLACK, ST7735_GREEN); 
             tft.print(F("SAVED")); 
             delay(1000);
             mgrState = 0; fullRedrawNeeded = true;
             return; 
        } 
        else if (editField == 0) { // NAME
             if (!inNameCharEdit) { 
                 inNameCharEdit = true; nameCharIdx = 0; 
             } else {
                 nameCharIdx++;
                 if (nameCharIdx > 5) { 
                     inNameCharEdit = false; nameCharIdx = 0; 
                 } 
             }
             forceRender = true; 
        }
        else { // ADVANCE
             drawCursor(editField, ST7735_BLACK);
             if (editField == 1) editField = 2;
             else if (editField == 2) editField = 8;
             drawCursor(editField, ST7735_CYAN);
             forceRender = true;
        }
    }
    else if (input == 1 || input == 2) { // L/R
        if (inNameCharEdit && editField == 0) {
            changed = true;
        } else if (editField == 1 || editField == 2) {
            changed = true;
        } else {
            drawCursor(editField, ST7735_BLACK); 
            if (input == 1) { 
               if(editField==0) editField=8; else if(editField==1) editField=0; else if(editField==2) editField=1; else editField=2;
            } else {
               if(editField==0) editField=1; else if(editField==1) editField=2; else if(editField==2) editField=8; else editField=0;
            }
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

    // --- RENDERING ---
    // 1. Name (SIZE 2 - Large)
    tft.setTextSize(2); 
    for(int i=0; i<6; i++) {
       if (forceRender || (changed && editField==0)) {
          int x = 20 + (i*15);
          tft.fillRect(x, 30, 12, 20, ST7735_BLACK); 
          uint16_t c = (editField==0 && inNameCharEdit && i==nameCharIdx) ? ST7735_RED : ST7735_WHITE;
          tft.setTextColor(c, ST7735_BLACK);
          
          tft.setCursor(x, 30); tft.print(tempTube.name[i]);
          
          tft.fillRect(x, 45, 12, 5, ST7735_BLACK);
          if (editField == 0 && inNameCharEdit && i == nameCharIdx) {
             tft.setCursor(x, 45); tft.print(F("^"));
          }
       }
    }

    // --- RESET TO SIZE 1 FOR VALUES ---
    tft.setTextSize(1); 
    // ---------------------------------------

    // 2. Watts
    if (forceRender || (changed && editField==1)) {
       uint16_t c = (editField == 1) ? ST7735_RED : ST7735_WHITE;
       tft.setTextColor(c, ST7735_BLACK);
       tft.fillRect(60, 65, 50, 10, ST7735_BLACK);
       tft.setCursor(60, 65); tft.print(tempTube.maxDiss); tft.print(F("W"));
    }

    // 3. Screen
    if (forceRender || (changed && editField==2)) {
       uint16_t c = (editField == 2) ? ST7735_RED : ST7735_WHITE;
       tft.setTextColor(c, ST7735_BLACK);
       tft.fillRect(60, 80, 50, 10, ST7735_BLACK);
       tft.setCursor(60, 80); tft.print(tempTube.screenFactor * 100.0, 1); tft.print(F("%"));
    }

    // 4. Save
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
// CALIBRATION LOGIC
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
      calItem = 0; EEPROM.put(0, calData); 
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
  VoltageA = ((float)(total_adc1_A / NUM_SAMPLES) * mV_per_bit) * calData.vScaleA;
  float rawCathodeA = (total_adc0_A / NUM_SAMPLES > ADC_NOISE_THRESHOLD) ? 
                      ((float)(total_adc0_A / NUM_SAMPLES) * mV_per_bit) / calData.rShuntA : 0.0;
  VoltageB = ((float)(total_adc3_B / NUM_SAMPLES) * mV_per_bit) * calData.vScaleB;
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
  drawMetric(25, 77, PowerA,   PowerAPrevious,    " ", ST7735_WHITE);
  uint16_t colorA = ST7735_WHITE;
  if (WattPercA > 80.0) colorA = ST7735_RED; else if (WattPercA > 70.0) colorA = ST7735_YELLOW; else if (WattPercA < 50.0) colorA = ST7735_CYAN;
  drawMetric(25, 93, WattPercA, WattPercAPrevious, "% ", colorA);

  drawMetric(110, 45, VoltageB, VoltageBPrevious, " ", ST7735_WHITE);
  drawMetric(110, 61, CurrentB, CurrentBPrevious, " ", ST7735_WHITE);
  drawMetric(110, 77, PowerB,   PowerBPrevious,    " ", ST7735_WHITE);
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

  tft.setTextColor(ST7735_MAGENTA); tft.setCursor(70, 80); tft.println(F("v1")); 
  
  // --- New code to display the attribution URL ---
  tft.setTextColor(ST7735_YELLOW); // A different color for visibility
  tft.setTextSize(1);
  
  // Display "Credits/License Info:" text
  tft.setCursor(10, 100); 
  tft.println("Credits/License Info:");

  // Display the URL below it
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

boolean debouncedButtonPressed(int inputPin) {
  if (digitalRead(inputPin) == LOW) { delay(20); if (digitalRead(inputPin) == LOW) return true; }
  return false;
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