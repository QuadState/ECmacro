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
#ifndef __USB_CONST_DATA_H__
#define __USB_CONST_DATA_H__

// clang-format off
#include <stdint.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"
#include "usbCommonDescriptors/StdDescriptors.h"
#include "usbCommonDescriptors/HIDClassCommon.h"
// clang-format on

#define EP0_ADDR 0
#define EP1_ADDR 10

#define KEYBOARD_EPADDR 0x81
#define KEYBOARD_LED_EPADDR 0x01
#define KEYBOARD_EPSIZE 8

/** Type define for the device configuration descriptor structure. This must be
 * defined in the application code, as the configuration descriptor contains
 * several sub-descriptors which vary between devices, and which describe the
 * device's usage to the host.
 */
typedef struct {
  USB_Descriptor_Configuration_Header_t Config;

  // Keyboard HID Interface
  USB_Descriptor_Interface_t HID_Interface;
  USB_HID_Descriptor_HID_t HID_KeyboardHID;
  USB_Descriptor_Endpoint_t HID_ReportINEndpoint;
  // USB_Descriptor_Endpoint_t HID_ReportOUTEndpoint;
} USB_Descriptor_Configuration_t;

extern __code USB_Descriptor_Device_t DeviceDescriptor;
extern __code USB_Descriptor_Configuration_t ConfigurationDescriptor;
extern __code uint8_t ReportDescriptor[];
extern __code uint8_t LanguageDescriptor[];
extern __code uint16_t SerialDescriptor[];
extern __code uint16_t ProductDescriptor[];
extern __code uint16_t ManufacturerDescriptor[];

#endif
