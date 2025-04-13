/*
 * ECmiacro02 Firmware
 * 
 * Description:
 * This firmware is for ECmiacro02, a 2-key USB macro pad using electrostatic capacitive switches.
 * It sends Volume Up, Volume Down, and Mute commands via USB HID.
 * 
 * Author: QuadState
 * License: MIT License
 * Project URL: https://github.com/QuadState/ecmiacro02
 * 
 * USB descriptor handling is based on CH55xduino's HID keyboard example by Deqing Sun:
 * https://github.com/DeqingSun/ch55xduino/tree/ch55xduino/ch55xduino/ch55x/libraries/Generic_Examples/examples/05.USB/HidKeyboard
 * 
 * Media key (Consumer Control) functionality was independently implemented,
 * although a similar approach exists in CH55xduino examples.
 * 
 * Application logic and key assignments are original.
 * 
 * Created: 2025
 * 
 * USB VID/PID: 0x1209 / 0xEC02 (pending registration via pid.codes)
 */

__sfr __at(0x8E) CKCON;

#ifdef USER_USB_RAM
#include "src/USBHIDKeyboard.h"
#else
#error "This example needs to be compiled with a USER USB setting"
#endif

#define SWITCH_COUNT 2

#define STABLE_THRESHOLD 15
#define WAIT_THRESHOLD 35

// Button event types
typedef enum {
  EVENT_NONE,
  EVENT_PRESS_TOP,
  EVENT_PRESS_BOTTOM,
  EVENT_PRESS_BOTH,
  EVENT_RELEASE_TOP,
  EVENT_RELEASE_BOTTOM
} ButtonEvent;

// Switch configuration structure
typedef struct {
  uint8_t pin;
  uint8_t mask;
  uint16_t releaseThreshold;
  uint16_t pressThreshold;
} SwitchConfig;

// Configuration for switches
const SwitchConfig switchConfigs[SWITCH_COUNT] = {
  { 14, 0x10, 34, 37 },  // Top key
  { 15, 0x20, 40, 48 }   // Bottom key
};

// Switch states
typedef enum {
  STATE_RELEASED,
  STATE_PRESSED,
  STATE_WAIT
} SwitchStateType;

// Structure to hold switch states
typedef struct {
  SwitchStateType state;
  uint8_t stableCount;
  uint8_t waitCount;
} SwitchState;

SwitchState switchStates[SWITCH_COUNT] = {
  { STATE_RELEASED, 0, 0 },
  { STATE_RELEASED, 0, 0 }
};

// Operation modes
typedef enum {
  MODE_PRESS_TOP,
  MODE_PRESS_BOTTOM,
  MODE_PRESS_BOTH,
  MODE_RELEASE
} OperationMode;

OperationMode currentMode = MODE_RELEASE;

// Timer initialization (16-bit timer)
void initTimer1(void) {
  CKCON |= (1 << 4);
  CKCON &= 0x0F;
  TMOD |= 0x10;
}

// Interrupt controls
void disableInterrupts(void) {
  EA = 0;
  USB_INT_FG = 0;
}

void enableInterrupts(void) {
  EA = 1;
}

// Timer utilities
void resetTimer(void) {
  TR1 = 0;
  TH1 = 0;
  TL1 = 0;
  TF1 = 0;
}

uint16_t getTimerCycleCount(void) {
  return ((uint16_t)TH1 << 8) | TL1;
}

// Measure the discharge time for capacitive switches
uint16_t measureDischargeCycleCount(uint8_t mask) {
  disableInterrupts();
  resetTimer();

  TR1 = 1;
  P1_DIR_PU &= ~mask;
  while (P1 & mask)
    ;
  TR1 = 0;
  P1_DIR_PU |= mask;

  uint16_t count = TF1 ? 0 : getTimerCycleCount();

  enableInterrupts();
  return count;
}

// Update switch state and detect events
ButtonEvent updateSwitchState(uint8_t index, uint16_t cycleCount) {
  SwitchState *currentSwitch = &switchStates[index];
  SwitchState *otherSwitch = &switchStates[index == 0 ? 1 : 0];
  const SwitchConfig *config = &switchConfigs[index];

  if (currentSwitch->state == STATE_WAIT) {
    if (currentSwitch->waitCount < WAIT_THRESHOLD) {
      currentSwitch->waitCount++;
    }
    if (otherSwitch->stableCount != 0) {
      return EVENT_NONE;
    }
    if (currentSwitch->waitCount >= WAIT_THRESHOLD) {
      currentSwitch->state = STATE_PRESSED;
      currentSwitch->waitCount = 0;
      return index == 0 ? EVENT_PRESS_TOP : EVENT_PRESS_BOTTOM;
    }
  } else if (currentSwitch->state == STATE_RELEASED) {
    if (cycleCount >= config->pressThreshold) {
      currentSwitch->stableCount++;
      if (currentSwitch->stableCount >= STABLE_THRESHOLD) {
        if (otherSwitch->state == STATE_WAIT) {
          currentSwitch->state = STATE_PRESSED;
          otherSwitch->state = STATE_PRESSED;
          currentSwitch->waitCount = 0;
          otherSwitch->waitCount = 0;
          currentSwitch->stableCount = 0;
          otherSwitch->stableCount = 0;
          return EVENT_PRESS_BOTH;
        } else {
          currentSwitch->state = STATE_WAIT;
          currentSwitch->stableCount = 0;
          currentSwitch->waitCount = 0;
        }
      }
    } else {
      currentSwitch->stableCount = 0;
    }
  } else if (currentSwitch->state == STATE_PRESSED) {
    if (cycleCount <= config->releaseThreshold) {
      currentSwitch->stableCount++;
      if (currentSwitch->stableCount >= STABLE_THRESHOLD) {
        currentSwitch->state = STATE_RELEASED;
        currentSwitch->stableCount = 0;
        return index == 0 ? EVENT_RELEASE_TOP : EVENT_RELEASE_BOTTOM;
      }
    } else {
      currentSwitch->stableCount = 0;
      currentSwitch->waitCount = 0;
    }
  }
  return EVENT_NONE;
}

// Initialize pins and USB
void setup(void) {
  for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
    pinMode(switchConfigs[i].pin, INPUT_PULLUP);
  }
  initTimer1();
#ifdef USER_USB_RAM
  USBInit();
#endif
}

// Send HID media commands
void sendVolumeUp() {
#ifdef USER_USB_RAM
  MediaKey_press(MEDIA_VOLUME_UP);
#endif
}

void sendVolumeDown() {
#ifdef USER_USB_RAM
  MediaKey_press(MEDIA_VOLUME_DOWN);
#endif
}

void sendMute() {
#ifdef USER_USB_RAM
  MediaKey_press(MEDIA_MUTE);
#endif
}

void releaseMediaKey() {
#ifdef USER_USB_RAM
  MediaKey_release();
  delay(1);
#endif
}

uint8_t previousMillis = 0;

// Main loop
void loop(void) {
  for (uint8_t i = 0; i < SWITCH_COUNT; i++) {
    uint16_t cycleCount = measureDischargeCycleCount(switchConfigs[i].mask);
    ButtonEvent event = updateSwitchState(i, cycleCount);

    switch (currentMode) {
      case MODE_RELEASE:
        if (event == EVENT_PRESS_TOP) {
          currentMode = MODE_PRESS_TOP;
          sendVolumeUp();
        } else if (event == EVENT_PRESS_BOTTOM) {
          currentMode = MODE_PRESS_BOTTOM;
          sendVolumeDown();
        } else if (event == EVENT_PRESS_BOTH) {
          currentMode = MODE_PRESS_BOTH;
          sendMute();
        }
        break;
      case MODE_PRESS_TOP:
        if (event == EVENT_RELEASE_TOP) {
          currentMode = MODE_RELEASE;
          releaseMediaKey();
        } else if (event == EVENT_PRESS_BOTTOM) {
          currentMode = MODE_PRESS_BOTTOM;
          releaseMediaKey();
          sendVolumeDown();
        }
        break;
      case MODE_PRESS_BOTTOM:
        if (event == EVENT_RELEASE_BOTTOM) {
          currentMode = MODE_RELEASE;
          releaseMediaKey();
        } else if (event == EVENT_PRESS_TOP) {
          currentMode = MODE_PRESS_TOP;
          releaseMediaKey();
          sendVolumeUp();
        }
        break;
      case MODE_PRESS_BOTH:
        if ((event == EVENT_RELEASE_TOP || event == EVENT_RELEASE_BOTTOM)
            && switchStates[0].state != STATE_PRESSED
            && switchStates[1].state != STATE_PRESSED) {
          currentMode = MODE_RELEASE;
          releaseMediaKey();
        }
        break;
    }
#ifndef USER_USB_RAM
    USBSerial_print(cycleCount);
    USBSerial_print((i != SWITCH_COUNT - 1) ? "," : "\r\n");
    delay(10);
#endif
  }

  while (true) {
    uint8_t currentMillis = millis();
    if (previousMillis != currentMillis) {
      previousMillis = currentMillis;
      break;
    }
  }
}