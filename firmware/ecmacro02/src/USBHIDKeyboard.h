/*
 * ECmacro02 USB HID Keyboard Firmware
 * 
 * Based on: CH55xduino (https://github.com/DeqingSun/ch55xduino)
 * Original files: Generic_Examples/05.USB/HidKeyboard
 * 
 * Modified by: QuadState
 * Project URL: https://github.com/QuadState/ECmacro
 * License: MIT (firmware)
 *
 * Description:
 * - USB HID implementation for CH552 microcontroller
 * - Supports basic 6KRO keyboard and consumer key operations
 * - Customized for ECmacro02, a compact 2-key macro pad using electrostatic capacitive switches
 *
 * MIT License applies unless otherwise noted.
 */
#ifndef __USB_HID_KBD_H__
#define __USB_HID_KBD_H__

// clang-format off
#include <stdint.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"
// clang-format on

#define KEY_LEFT_CTRL 0x80
#define KEY_LEFT_SHIFT 0x81
#define KEY_LEFT_ALT 0x82
#define KEY_LEFT_GUI 0x83
#define KEY_RIGHT_CTRL 0x84
#define KEY_RIGHT_SHIFT 0x85
#define KEY_RIGHT_ALT 0x86
#define KEY_RIGHT_GUI 0x87

#define KEY_UP_ARROW 0xDA
#define KEY_DOWN_ARROW 0xD9
#define KEY_LEFT_ARROW 0xD8
#define KEY_RIGHT_ARROW 0xD7
#define KEY_BACKSPACE 0xB2
#define KEY_TAB 0xB3
#define KEY_RETURN 0xB0
#define KEY_ESC 0xB1
#define KEY_INSERT 0xD1
#define KEY_DELETE 0xD4
#define KEY_PAGE_UP 0xD3
#define KEY_PAGE_DOWN 0xD6
#define KEY_HOME 0xD2
#define KEY_END 0xD5
#define KEY_CAPS_LOCK 0xC1
#define KEY_F1 0xC2
#define KEY_F2 0xC3
#define KEY_F3 0xC4
#define KEY_F4 0xC5
#define KEY_F5 0xC6
#define KEY_F6 0xC7
#define KEY_F7 0xC8
#define KEY_F8 0xC9
#define KEY_F9 0xCA
#define KEY_F10 0xCB
#define KEY_F11 0xCC
#define KEY_F12 0xCD
#define KEY_F13 0xF0
#define KEY_F14 0xF1
#define KEY_F15 0xF2
#define KEY_F16 0xF3
#define KEY_F17 0xF4
#define KEY_F18 0xF5
#define KEY_F19 0xF6
#define KEY_F20 0xF7
#define KEY_F21 0xF8
#define KEY_F22 0xF9
#define KEY_F23 0xFA
#define KEY_F24 0xFB

#ifdef __cplusplus
extern "C" {
#endif

void USBInit(void);

uint8_t Keyboard_press(__data uint8_t k);
uint8_t Keyboard_release(__data uint8_t k);
void Keyboard_releaseAll(void);

uint8_t Keyboard_write(__data uint8_t c);
void Keyboard_print(const char *str);

uint8_t Keyboard_getLEDStatus();

// メディアキー定義（必要なものだけ）
#define MEDIA_VOLUME_UP    0x00E9
#define MEDIA_VOLUME_DOWN  0x00EA
#define MEDIA_MUTE         0x00E2

// API定義
uint8_t MediaKey_press(uint16_t keycode);
uint8_t MediaKey_release(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
