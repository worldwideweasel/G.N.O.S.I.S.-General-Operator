/* SPDX-License-Identifier: GPL-3.0-only or GPL-3.0-or-later */
/*
 * G.N.O.S.I.S. – General Operator is a touch-controlled Eurorack sequencer 
 * and performance interface you can play by hand and / or clock signals.
 * Music Weasel
 * v1.0
 *
 * https://github.com/worldwideweasel/G.N.O.S.I.S.-General-Operator.git
 *
 * Copyright (C) 2026 Fred Roessler <fretze@posteo.de>.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "Adafruit_MCP4725.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// ============================================================================
// CALIBRATION & PARAMETERS
// ============================================================================

// --- Pressure Sensitivity (Pressure CV) ---
#define PRESSURE_DELTA_MIN 200  // Minimum delta threshold where CV starts (suppresses baseline noise)
#define PRESSURE_DELTA_MAX 600  // Maximum delta peak value under full finger pressure
#define DAC_MIN 0               // DAC value at minimal pressure (0 = 0V)
#define DAC_MAX 2047            // DAC value at maximum pressure (2047 = ~5V DAC Out → ~10V CV Out after Op-Amp)

// --- Debounce Configuration ---
#define TOUCH_RELEASE_DEBOUNCE_MS 1  // Wait time in ms after release to prevent contact chatter

// ============================================================================
// HARDWARE INSTANCES & HARDWARE MAPPING
// ============================================================================

Adafruit_MPR121 cap = Adafruit_MPR121();
Adafruit_MCP4725 dac;

// --- Physical Pin Mapping ---
const int STEP_PINS[] = { 9, 8, 7, 6, 5, 4, 3, 2 };  // The 8 physical Step LED/Gate outputs

#define CLOCK_PIN 12      // Input: External Clock signal
#define DIRECTION_PIN A0  // Input: Direction control (Forward/Reverse)
#define RESET_PIN 11      // Input: Reset to Step 16
#define ZERO_PIN 10       // Input: Zero Mode (all steps inactive)
#define SWITCHROW 13      // Output: Controls Row-Select (Row A: Steps 16-9 / Row B: Steps 8-1)
#define TOUCHGATE A1      // Output: Gate Out, active whenever a touchpad is touched
#define MODE_PIN A2       // Input: Mode-Switch (HIGH = Touch directly selects/jumps to step)

// ============================================================================
// GLOBAL STATE
// ============================================================================

// --- Touch State ---
uint16_t currtouched = 0;
int activePad = -1;               // Currently pressed pad (0–7), -1 = none
unsigned long lastTouchTime = 0;  // Timestamp for the release debounce timer

// --- MPR121 Sensitivity Thresholds ---
int touchThreshold = 15;   // Touch detection threshold (higher = less sensitive)
int releaseThreshold = 6;  // Release detection threshold (hysteresis against chatter)

// --- Sequencer State ---
int currentStep16 = 16;   // Internal counter (Steps 1 to 16)
bool lastFwd, lastReset, lastZero;
bool jumpEnabled = true;  // Stores the current state of the Mode input

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // 1. Manual I2C bus reset (ensures startup stability)
  pinMode(A5, OUTPUT);  // SCL line
  for (int i = 0; i < 10; i++) {
    digitalWrite(A5, HIGH);
    delay(10);
    digitalWrite(A5, LOW);
    delay(10);
  }
  pinMode(A5, INPUT);  // Reset back to input for Wire.h

  // 2. I2C Bus & MPR121 Hardware Reset
  Wire.begin();
  Wire.setClock(400000); // Fast Mode (400 kHz) for minimal I2C latency
  
  Wire.beginTransmission(0x5A);
  Wire.write(0x80);
  Wire.write(0x63);
  Wire.endTransmission();
  delay(250);

  // 3. Configure Outputs
  for (int i = 0; i < 8; i++) pinMode(STEP_PINS[i], OUTPUT);
  pinMode(SWITCHROW, OUTPUT);
  pinMode(TOUCHGATE, OUTPUT);

  // 4. Configure Inputs
  pinMode(CLOCK_PIN, INPUT);
  pinMode(DIRECTION_PIN, INPUT);
  pinMode(RESET_PIN, INPUT);
  pinMode(ZERO_PIN, INPUT);
  pinMode(MODE_PIN, INPUT);

  // 5. Initialize MPR121 Touch Controller
  if (!cap.begin(0x5A)) {
    while (1); // Halt execution on hardware error
  }
  cap.setAutoconfig(true);
  cap.setThresholds(touchThreshold, releaseThreshold);

  // 6. Initialize MCP4725 DAC
  if (!dac.begin(0x60)) {
    while (1); // Halt execution on hardware error
  }
  dac.setVoltage(0, false);  // Initialize CV output to 0V

  // 7. Read initial states of control inputs
  lastFwd = digitalRead(CLOCK_PIN);
  lastReset = digitalRead(RESET_PIN);
  lastZero = digitalRead(ZERO_PIN);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  static int lastStep = -1;

  // 1. Process inputs & update logic
  handleTouchMPR();
  handleSequencer();
  updatePressureCV();

  // 2. Update Touch-Gate output (HIGH as long as any pad is touched)
  digitalWrite(TOUCHGATE, (activePad != -1) ? HIGH : LOW);

  // 3. Update hardware output pins only when the step changes
  if (currentStep16 != lastStep) {
    updateOutputs();
    lastStep = currentStep16;
  }
}

// ============================================================================
// LOGIC FUNCTIONS
// ============================================================================

/**
 * Reads the MPR121 touch sensor, handles pad priority, 
 * and applies release debouncing.
 */
void handleTouchMPR() {
  currtouched = cap.touched();
  jumpEnabled = (digitalRead(MODE_PIN) == HIGH);

  int detectedPad = -1;

  // Determine the highest active touchpad (Mono Priority)
  if (currtouched & 0xFF) {
    for (int i = 7; i >= 0; i--) {
      if (currtouched & _BV(i)) {
        detectedPad = i;
        break;
      }
    }
  }

  // Handle touch state and debouncing
  if (detectedPad != -1) {
    activePad = detectedPad;
    if (jumpEnabled) {
      currentStep16 = 16 - activePad;  // Jump directly to touched step
    }
    lastTouchTime = millis();
  } else {
    // Release active pad only after the debounce timer expires
    if (millis() - lastTouchTime > TOUCH_RELEASE_DEBOUNCE_MS) {
      activePad = -1;
    }
  }
}

/**
 * Controls sequencer progression based on Clock, Direction, Reset, 
 * and Zero signals.
 */
void handleSequencer() {
  bool clk = digitalRead(CLOCK_PIN);
  bool dir = digitalRead(DIRECTION_PIN);
  bool rst = digitalRead(RESET_PIN);
  bool zro = digitalRead(ZERO_PIN);

  // Process clock tick (ignored while a pad is held in active MODE)
  if (activePad == -1 || !jumpEnabled) {
    if (clk == LOW && lastFwd == HIGH) {  // Falling edge (Clock Tick)
      if (dir == HIGH) {
        currentStep16--;  // Forward sequence
        if (currentStep16 < 1) currentStep16 = 16;
      } else {
        currentStep16++;  // Reverse sequence
        if (currentStep16 > 16) currentStep16 = 1;
      }
    }
  }

  // Reset edge (Resets sequencer to Step 16 / Start)
  if (rst == LOW && lastReset == HIGH) currentStep16 = 16;
  
  // Zero edge (Activates idle state Step 17)
  if (zro == LOW && lastZero == HIGH) currentStep16 = 17;

  lastFwd = clk;
  lastReset = rst;
  lastZero = zro;
}

/**
 * Calculates capacitance delta (finger pressure) and outputs 
 * the mapped voltage value via the I2C DAC.
 */
void updatePressureCV() {
  static int lastDacValue = -1;

  // Idle state: reset voltage to 0V
  if (activePad == -1) {
    if (lastDacValue != 0) {
      dac.setVoltage(0, false);
      lastDacValue = 0;
    }
    return;
  }

  // Calculate delta value from raw and baseline capacitance data
  int raw = cap.filteredData(activePad);
  int baseline = cap.baselineData(activePad);
  int delta = baseline - raw;

  // Constrain delta to configured range and map to DAC resolution
  delta = constrain(delta, PRESSURE_DELTA_MIN, PRESSURE_DELTA_MAX);
  int dacValue = map(delta, PRESSURE_DELTA_MIN, PRESSURE_DELTA_MAX, DAC_MIN, DAC_MAX);

  // Reduce I2C bus traffic: send only if value changes significantly
  if (abs(dacValue - lastDacValue) > 4) {
    dac.setVoltage(dacValue, false);
    lastDacValue = dacValue;
  }
}

/**
 * Drives the physical step output pins and handles row toggling (Switchrow).
 */
void updateOutputs() {
  // Zero Mode: Turn off all step outputs
  if (currentStep16 == 17) {
    allStepsLow();
    digitalWrite(SWITCHROW, HIGH);
    return;
  }

  // Determine row assignment (Row A: Steps 16-9, Row B: Steps 8-1)
  bool rowA = (currentStep16 > 8);
  digitalWrite(SWITCHROW, rowA ? HIGH : LOW);

  // Map 16-step value to physical 8-pin array index
  int physicalStep = (currentStep16 > 8) ? (currentStep16 - 8) : currentStep16;
  int pinIndex = 8 - physicalStep;

  // Set physical step pins
  for (int i = 0; i < 8; i++) {
    digitalWrite(STEP_PINS[i], (i == pinIndex) ? HIGH : LOW);
  }
}

/**
 * Helper function: Sets all 8 step pins to LOW.
 */
void allStepsLow() {
  for (int i = 0; i < 8; i++) digitalWrite(STEP_PINS[i], LOW);
}
