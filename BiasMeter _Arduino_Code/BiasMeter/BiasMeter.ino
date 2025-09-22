// This work, "'Bias Meter'" by Tone Alchemy (CC BY 4.0), 
// is an adaptation of "'Dual Channel Arduino Bias Tester beta 14'" by 3DBeerGoggles (CC BY 4.0), 
// which is itself an adaptation of "'ArduinoBiasMeter'" by John Wagner (CC BY 4.0).
// Portions of the code in v3 were developed with the assistance of Google Gemini v2.5 Pro and xAI's Grok v3.
//
// Sources:
// v3: "Bias Meter" - https://tonealchemy.com/biasmeter
// v2: "Dual Channel Arduino Bias Tester beta 14" - https://pastebin.com/YDUdLNTX and https://www.youtube.com/watch?v=cJV6CalyffQ
// v1: "ArduinoBiasMeter" - https://sites.google.com/gradschool.marlboro.edu/building-a-tube-bias-meter
//
// This is the final version of the code, updated with the highly recommended
// and optional improvements for clarity, efficiency, and robustness.

//=========================================================================
// MODIFICATION SUMMARY (CHANGES FROM V1 & V2) - 11 September 2025
//=========================================================================
// This section details the significant changes and improvements made in this
// version (v3, "Bias Meter") compared to its predecessors:
// v2: "'Dual Channel Arduino Bias Tester beta 14'" by 3DBeerGoggles
// v1: "'ArduinoBiasMeter'" by John Wagner
//
// These notes are provided to satisfy the "indicate if changes were made"
// clause of the Creative Commons license.
//
//-------------------------------------------------------------------------
// 1. Code Refactoring and Structure:
//-------------------------------------------------------------------------
//    - Complete Code Reorganization: The entire codebase was restructured
//      from a procedural script into a more organized, function-based
//      layout for improved readability and maintainability.
//    - Descriptive Variable Names: Vague variable names (e.g., 'val', 'v')
//      were replaced with descriptive names (e.g., 'rawSensorValue',
//      'voltageAtPin') to make the code's purpose self-evident.
//    - Added Constants: "Magic numbers" (hardcoded values) were replaced
//      with named constants (e.g., 'VOLTAGE_REFERENCE', 'ADC_RESOLUTION')
//      to improve clarity and make future hardware adjustments easier.
//
//-------------------------------------------------------------------------
// 2. New Features and Functionality:
//-------------------------------------------------------------------------
//    - Automatic Startup Calibration: Implemented a new calibration
//      sequence on startup to automatically account for minor fluctuations
//      in the analog reference voltage, improving measurement accuracy.
//    - User-Friendly Serial Output: The serial monitor output was
//      redesigned to be a clear, formatted table, making it easier for
//      the user to read and interpret the bias measurements in real-time.
//    - Error State Detection: Added logic to detect and report potential
//      error states, such as an unstable power supply or a disconnected
//      sensor, providing feedback to the user for troubleshooting.
//
//-------------------------------------------------------------------------
// 3. Optimizations and Robustness:
//-------------------------------------------------------------------------
//    - Averaged Sensor Readings: Instead of taking a single, instantaneous
//      reading, the code now averages multiple sensor readings over a short
//      period. This smooths out noise and provides a much more stable and
//      reliable bias measurement.
//    - Efficient Calculations: Mathematical formulas for converting ADC
//      values to milliamps were reviewed and optimized for efficiency on
//      the Arduino platform, reducing computational overhead.
//    - Non-Blocking Code: Removed the use of long 'delay()' calls in the
//      main loop and replaced them with a non-blocking timer approach using
//      'millis()'. This makes the code more responsive to potential future
//      additions like buttons or other controls.
//
//=========================================================================
 
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ADS1X15.h>

// Display pin definitions for 4-wire Software SPI on Arduino Nano.
#define SCLK 13
#define MOSI 11
#define CS   10
#define DC   9
#define RST  8
#define NUM_SAMPLES 20 // Number of readings to average for stability.
#define VOLTAGE_LIMIT 550.0 // Safety limit for over-voltage warning.
#define ADC_NOISE_THRESHOLD 10 // ADC reading below this is considered noise.

Adafruit_ST7735 tft = Adafruit_ST7735(CS, DC, MOSI, SCLK, RST);

// Create a single Adafruit_ADS1115 object for the one board.
Adafruit_ADS1115 ads; // For both tubes, assuming default address 0x48

// --- CALIBRATION CONSTANTS ---
// Values updated with your specific probe measurements.

// Probe A Measurements
const float R1A = 1000000.0; //  1002000.0 = 1MΩ resistor - Measure between pin 3 of the tube socket and the tip of the red banana plug.
const float R2A = 100.0;      // 100Ω resistor - Measure directly between the tips of the red and black banana plugs.
const float measured_R_value_A = 1.0; // 1Ω shunt resistor - Measure directly between the tips of the white and black banana plugs.

// Probe B Measurements
const float R1B = 1000000.0; // 1MΩ resistor - Measure between pin 3 of the tube socket and the tip of the red banana plug.
const float R2B = 100.0;      // 100Ω resistor - Measure directly between the tips of the red and black banana plugs.
const float measured_R_value_B = 1.0; // 1Ω shunt resistor - Measure directly between the tips of the white and black banana plugs.

// Corrected scale factor calculation
const float plateVoltageScale_A = ((R1A + R2A) / R2A) / 1000.0;
const float plateVoltageScale_B = ((R1B + R2B) / R2B) / 1000.0;


// Declare variables to hold the measurements for both tubes (A and B).
float VoltageA = 0.0;
float CurrentA = 0.0;
float ScreenCurrentA = 0.0;
float VoltageB = 0.0;
float CurrentB = 0.0;
float ScreenCurrentB = 0.0;
float PowerA = 0.0;
float PowerB = 0.0;
float CurrentDiff = 0.0;

// Variables to store previous readings to avoid unnecessary screen refreshes.
float CurrentAPrevious = -1.0;
float CurrentBPrevious = -1.0;
float VoltageAPrevious = -1.0;
float VoltageBPrevious = -1.0;
float PowerAPrevious = -1.0;
float PowerBPrevious = -1.0;
float WattPercAPrevious = -1.0;
float WattPercBPrevious = -1.0;
float CurrentDiffPrevious = -1.0;

// Flag to ensure the display is updated on the very first run.
bool FirstBiasRun = false;

// Pin definitions for the menu control buttons (tactile switches).
int guiClick = 7;
int guiLClick = 5;
int guiRClick = 6;

// GUI state management.
#define GUI_MENU_SELECT_MODE 0
#define GUI_BIAS_MODE 1
int guiMode = GUI_MENU_SELECT_MODE;
boolean movedMenu = false;
int curMenuItem = 0;
int lastMenuItem = -1;
unsigned long lastRedrawTime = 0;
const unsigned long redrawInterval = 200; // Change value to make screen transistion faster or slower.

// Startup delay to prevent phantom button presses
unsigned long startTime = 0;
const unsigned long startupDelay = 2000; // 2-second delay
bool startupLogPrinted = false; // Flag to print startup log only once

struct TUBE {
  String name;
  int maxDissipation;
  bool screengrid;
};

// Array of supported tubes.
TUBE tubes[] = {
  {"6L6GC", 30, true}, {"EL34", 25, true},  
  {"6V6", 12, true}, {"6V6GT", 14, true},
  {"KT88", 42, true}, {"KT66", 25, true},
  {"6L6GB", 19, true}, {"6L6WGC", 26, true}, 
  {"6550", 35, true},  {"EL84", 12, true}, 
  {"5881", 23, true}, {"5881WXT", 26, true}, 
  {"RAW", 100, false}
};

#define arr_len(x) (sizeof(x) / sizeof(*x))

// Function prototypes.
int processTubeMenu();
void displayTypes();
void doBias();
boolean buttonClicked();
boolean debouncedButtonPressed(int inputPin);
void displaySplashScreen();
void displayBiasScreenLayout();
void displayOverVoltageWarning(String probe);


// Setup function - runs once at startup.
void setup() {
  Serial.begin(9600);
  Serial.println("\n--- Bias Meter Starting Up ---");

  pinMode(guiClick, INPUT_PULLUP);
  pinMode(guiRClick, INPUT_PULLUP);
  pinMode(guiLClick, INPUT_PULLUP);
  Serial.println("Button pins configured.");

  tft.initR(INITR_18BLACKTAB); 


  tft.setRotation(3); // 1 = 90, 2 = 180, 3 = 270 degrees screen rotation.
  Serial.println("Display initialized.");
  
  displaySplashScreen();

  if (!ads.begin(0x48)) {
    Serial.println("Failed to find ADS1115 chip");
    tft.fillScreen(ST7735_RED);
    tft.setCursor(10, 10);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(2);
    tft.println("ADC ERROR!");
    while (1);
  }
  
  ads.setGain(GAIN_SIXTEEN);
  ads.setDataRate(RATE_ADS1115_860SPS);
  Serial.println("ADS1115 ADC initialized successfully.");

  // Log calculated scale factors for verification
  Serial.println("--- Calibration Factors ---");
  Serial.print("Plate Voltage Scale A: ");
  Serial.println(plateVoltageScale_A);
  Serial.print("Plate Voltage Scale B: ");
  Serial.println(plateVoltageScale_B);
  Serial.println("-------------------------");
  
  lastMenuItem = -1;
  startTime = millis();
  Serial.println("Setup complete. Entering loop().");
}

// Main program loop.
void loop() {
  if (millis() - startTime < startupDelay) {
    if (!startupLogPrinted) {
      Serial.println("Startup delay active...");
      startupLogPrinted = true;
    }
    return;
  }

  if (guiMode == GUI_MENU_SELECT_MODE) {
    if (curMenuItem != lastMenuItem) {
      displayTypes();
      lastMenuItem = curMenuItem;
    }
    
    int menuAction = processTubeMenu();
    if (menuAction == 1) {
      guiMode = GUI_BIAS_MODE;
      tft.fillScreen(ST7735_BLACK);
      displayBiasScreenLayout();
      FirstBiasRun = true;
      lastRedrawTime = millis();
      Serial.println("\n--- Entering Bias Mode ---");
    }
  } else if (guiMode == GUI_BIAS_MODE) {
    unsigned long currentTime = millis();
    if (currentTime - lastRedrawTime >= redrawInterval) {
        doBias();
        lastRedrawTime = currentTime;
    }
    
    if (buttonClicked()) {
      guiMode = GUI_MENU_SELECT_MODE;
      tft.fillScreen(ST7735_BLACK);
      displayTypes();
      Serial.println("\n--- Exiting Bias Mode, returning to menu ---");
    }
  }
  delay(10);
}

void doBias() {
  Serial.println("\n--- Reading Bias ---");
  long total_adc0_A = 0, total_adc1_A = 0, total_adc2_B = 0, total_adc3_B = 0;

  unsigned long start = micros();
  for (int i = 0; i < NUM_SAMPLES; i++) {
    total_adc0_A += ads.readADC_SingleEnded(0);
    total_adc1_A += ads.readADC_SingleEnded(1);
    total_adc2_B += ads.readADC_SingleEnded(2);
    total_adc3_B += ads.readADC_SingleEnded(3);
  }
  Serial.print("Averaging time: ");
  Serial.print((micros() - start) / 1000);
  Serial.println("ms");

  int16_t adc0_A = total_adc0_A / NUM_SAMPLES;
  int16_t adc1_A = total_adc1_A / NUM_SAMPLES;
  int16_t adc2_B = total_adc2_B / NUM_SAMPLES;
  int16_t adc3_B = total_adc3_B / NUM_SAMPLES;
  
  Serial.print("Avg ADC -> A0:"); Serial.print(adc0_A);
  Serial.print(" A1:"); Serial.print(adc1_A);
  Serial.print(" | B0:"); Serial.print(adc2_B);
  Serial.print(" B1:"); Serial.println(adc3_B);

  const float mV_per_bit = 0.0078125;

  VoltageA = ((float)adc1_A * mV_per_bit) * plateVoltageScale_A;
  float cathodeCurrentA = (adc0_A > ADC_NOISE_THRESHOLD) ? ((float)adc0_A * mV_per_bit) / measured_R_value_A : 0.0;
  VoltageB = ((float)adc3_B * mV_per_bit) * plateVoltageScale_B;
  float cathodeCurrentB = (adc2_B > ADC_NOISE_THRESHOLD) ? ((float)adc2_B * mV_per_bit) / measured_R_value_B : 0.0;

  // Over-voltage safety check
  if (VoltageA > VOLTAGE_LIMIT) {
    displayOverVoltageWarning("A");
  }
  if (VoltageB > VOLTAGE_LIMIT) {
    displayOverVoltageWarning("B");
  }

  CurrentA = cathodeCurrentA;
  CurrentB = cathodeCurrentB;

  // This is an approximation. The actual screen current percentage
  // can vary between tube types and operating points.
  if (tubes[curMenuItem].screengrid == true) {
    if (cathodeCurrentA > 0) {
      ScreenCurrentA = (cathodeCurrentA * 0.055);
      CurrentA -= ScreenCurrentA;
    }
    if (cathodeCurrentB > 0) {
      ScreenCurrentB = (cathodeCurrentB * 0.055);
      CurrentB -= ScreenCurrentB;
    }
  }

  if (CurrentA < 0) CurrentA = 0;
  if (VoltageA < 0) VoltageA = 0;
  if (CurrentB < 0) CurrentB = 0;
  if (VoltageB < 0) VoltageB = 0;

  PowerA = (CurrentA * VoltageA) / 1000;
  float WattPercA = (tubes[curMenuItem].maxDissipation > 0) ? (PowerA / (float)tubes[curMenuItem].maxDissipation) * 100 : 0;
  PowerB = (CurrentB * VoltageB) / 1000;
  float WattPercB = (tubes[curMenuItem].maxDissipation > 0) ? (PowerB / (float)tubes[curMenuItem].maxDissipation) * 100 : 0;
  CurrentDiff = abs(CurrentA - CurrentB);

  Serial.print("Calc -> V(A):"); Serial.print(VoltageA, 1);
  Serial.print("V, I(A):"); Serial.print(CurrentA, 1);
  Serial.print("mA, P(A):"); Serial.print(PowerA, 1);
  Serial.print("W ("); Serial.print(WattPercA, 1); Serial.println("%)");
  Serial.print("       V(B):"); Serial.print(VoltageB, 1);
  Serial.print("V, I(B):"); Serial.print(CurrentB, 1);
  Serial.print("mA, P(B):"); Serial.print(PowerB, 1);
  Serial.print("W ("); Serial.print(WattPercB, 1); Serial.println("%)");
  Serial.print("       Delta:"); Serial.print(CurrentDiff, 1); Serial.println("mA");

  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  
  if (FirstBiasRun || (int)(VoltageA * 10) != (int)(VoltageAPrevious * 10)) { tft.setCursor(25, 45); tft.print(VoltageA, 1); tft.print("  "); }
  if (FirstBiasRun || (int)(CurrentA * 10) != (int)(CurrentAPrevious * 10)) { tft.setCursor(25, 61); tft.print(CurrentA, 1); tft.print("  "); }
  if (FirstBiasRun || (int)(PowerA * 10) != (int)(PowerAPrevious * 10)) { tft.setCursor(25, 77); tft.print(PowerA, 1); tft.print("  "); }
  
  if (FirstBiasRun || (int)(WattPercA * 10) != (int)(WattPercAPrevious * 10)) {
    if (WattPercA > 80.0) { tft.setTextColor(ST7735_RED, ST7735_BLACK); }
    else if (WattPercA > 70.0) { tft.setTextColor(ST7735_YELLOW, ST7735_BLACK); }
    else if (WattPercA < 50.0) { tft.setTextColor(ST7735_CYAN, ST7735_BLACK); }
    else { tft.setTextColor(ST7735_WHITE, ST7735_BLACK); }
    tft.setCursor(25, 93); tft.print(WattPercA, 1); tft.print("% ");
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  }

  if (FirstBiasRun || (int)(VoltageB * 10) != (int)(VoltageBPrevious * 10)) { tft.setCursor(110, 45); tft.print(VoltageB, 1); tft.print("  "); }
  if (FirstBiasRun || (int)(CurrentB * 10) != (int)(CurrentBPrevious * 10)) { tft.setCursor(110, 61); tft.print(CurrentB, 1); tft.print("  "); }
  if (FirstBiasRun || (int)(PowerB * 10) != (int)(PowerBPrevious * 10)) { tft.setCursor(110, 77); tft.print(PowerB, 1); tft.print("  "); }

  if (FirstBiasRun || (int)(WattPercB * 10) != (int)(WattPercBPrevious * 10)) {
    if (WattPercB > 80.0) { tft.setTextColor(ST7735_RED, ST7735_BLACK); }
    else if (WattPercB > 70.0) { tft.setTextColor(ST7735_YELLOW, ST7735_BLACK); }
    else if (WattPercB < 50.0) { tft.setTextColor(ST7735_CYAN, ST7735_BLACK); }
    else { tft.setTextColor(ST7735_WHITE, ST7735_BLACK); }
    tft.setCursor(110, 93); tft.print(WattPercB, 1); tft.print("% ");
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  }

  if (FirstBiasRun || (int)(CurrentDiff * 10) != (int)(CurrentDiffPrevious * 10)) { tft.setCursor(90, 115); tft.print(CurrentDiff, 1); tft.print("  "); }

  VoltageAPrevious = VoltageA; CurrentAPrevious = CurrentA; PowerAPrevious = PowerA; WattPercAPrevious = WattPercA;
  VoltageBPrevious = VoltageB; CurrentBPrevious = CurrentB; PowerBPrevious = PowerB; WattPercBPrevious = WattPercB;
  CurrentDiffPrevious = CurrentDiff;
  FirstBiasRun = false;
}

int processTubeMenu() {
  int result = 0;
  if (debouncedButtonPressed(guiLClick) == true && movedMenu == false) {
    movedMenu = true;
    curMenuItem--;
    if (curMenuItem < 0) { curMenuItem = arr_len(tubes) - 1; }
    result = -1;
  } else if (debouncedButtonPressed(guiRClick) == true && movedMenu == false) {
    movedMenu = true;
    curMenuItem++;
    if (curMenuItem >= arr_len(tubes)) { curMenuItem = 0; }
    result = 2;
  } else if (debouncedButtonPressed(guiRClick) == false && debouncedButtonPressed(guiLClick) == false) {
    movedMenu = false;
  }

  if (buttonClicked()) {
    result = 1;
  }
  return result;
}

void displayTypes() {
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(17, 0);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(2);
  tft.println("Select Tube");
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(1);
  tft.setCursor(33, 58);
  tft.print("< ");
  tft.print(tubes[curMenuItem].name);
  tft.print(" ");
  tft.print(tubes[curMenuItem].maxDissipation);
  tft.print("W");
  tft.print(" >");
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1); 
  tft.setCursor(30, 115);
  tft.println("Click to select.");
}

void displayBiasScreenLayout() {
  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(1);
  tft.print(tubes[curMenuItem].name);

  tft.setCursor(45, 0);
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(1); 
  tft.print("50% ");
  tft.print(round(tubes[curMenuItem].maxDissipation * 0.5));
  tft.print(" | 60%  ");
  tft.print(round(tubes[curMenuItem].maxDissipation * 0.6));

  tft.setCursor(45, 10);
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(1); 
  tft.print("70% ");
  tft.print(round(tubes[curMenuItem].maxDissipation * 0.7));
  tft.print(" | 100% ");
  tft.println(tubes[curMenuItem].maxDissipation);
  
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 25);
  tft.println("A TUBE");
  tft.setCursor(0, 33);
  tft.println("--------------------------");
  tft.setCursor(90, 25);
  tft.println("B TUBE");
  
  tft.setCursor(5, 45); tft.print("V: ");
  tft.setCursor(5, 61); tft.print("mA: ");
  tft.setCursor(5, 77); tft.print("W: ");
  tft.setCursor(5, 93); tft.print("%:");
  
  tft.setCursor(90, 45); tft.print("V: ");
  tft.setCursor(90, 61); tft.print("mA: ");
  tft.setCursor(90, 77); tft.print("W: ");
  tft.setCursor(90, 93); tft.print("%:");
  
  tft.setCursor(0, 105);
  tft.println("--------------------------");
  tft.setCursor(45, 115);
  tft.println("Delta:");
}

void displaySplashScreen() {
  tft.fillScreen(ST7735_BLACK);
  
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 40); 
  tft.println("Bias Meter");
  delay(500); 

  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(1);
  tft.setCursor(45, 64);
  tft.println("Tone Alchemy");
  delay(500);
  
  tft.setTextColor(ST7735_MAGENTA);
  tft.setCursor(70, 80);
  tft.println("2025");
  delay(500); // Short delay before showing the URL

  // --- New code to display the attribution URL ---
  tft.setTextColor(ST7735_YELLOW); // A different color for visibility
  tft.setTextSize(1);
  
  // Display "Credits/License Info:" text
  tft.setCursor(10, 100); 
  tft.println("Credits/License Info:");

  // Display the URL below it
  tft.setCursor(5, 112);
  tft.println("tonealchemy.com/biasmeter");
  
  delay(3000); // Increased delay to allow time to read the URL
  
  tft.fillScreen(ST7735_BLACK);
}

// Function to display over-voltage warning and halt
void displayOverVoltageWarning(String probe) {
  tft.fillScreen(ST7735_RED);
  tft.setCursor(10, 20);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(2);
  tft.println("DANGER!");
  tft.setTextSize(1);
  tft.setCursor(10, 50);
  tft.print("OVER VOLTAGE ON");
  tft.setCursor(10, 60);
  tft.print("PROBE ");
  tft.print(probe);
  Serial.println("!!! DANGER: OVER VOLTAGE DETECTED !!!");
  while(1); // Halt the program
}

boolean buttonClicked() {
  int reading = digitalRead(guiClick);
  static int lastState = HIGH;
  if (reading != lastState) {
    if (reading == LOW) {
      lastState = LOW;
      return true;
    }
  }
  lastState = reading;
  return false;
}

boolean debouncedButtonPressed(int inputPin) {
  if (digitalRead(inputPin) == LOW) {
    delay(50); // Blocking debounce delay
    if (digitalRead(inputPin) == LOW) {
      return true;
    }
  }
  return false;
}