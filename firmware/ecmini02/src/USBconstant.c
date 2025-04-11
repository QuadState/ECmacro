/*
 * ECmini02 USB HID Keyboard Firmware
 * 
 * Based on: CH55xduino (https://github.com/DeqingSun/ch55xduino)
 * Original files: Generic_Examples/05.USB/HidKeyboard
 * 
 * Modified by: QuadState
 * Project URL: https://github.com/QuadState/ECmini02
 * License: MIT (firmware)
 *
 * Description:
 * - USB HID implementation for CH552 microcontroller
 * - Supports basic 6KRO keyboard and consumer key operations
 * - Customized for ECmini02, a compact 2-key macro pad using electrostatic capacitive switches
 *
 * MIT License applies unless otherwise noted.
 */
#include "USBconstant.h"

// Device descriptor
__code USB_Descriptor_Device_t DeviceDescriptor = {
    .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

    .USBSpecification = VERSION_BCD(1, 1, 0),
    .Class = 0x00,
    .SubClass = 0x00,
    .Protocol = 0x00,

    .Endpoint0Size = DEFAULT_ENDP0_SIZE,

    .VendorID = 0x1209,
    .ProductID = 0xEC02,
    .ReleaseNumber = VERSION_BCD(1, 0, 0),

    .ManufacturerStrIndex = 1,
    .ProductStrIndex = 2,
    .SerialNumStrIndex = 3,

    .NumberOfConfigurations = 1};

/** Configuration descriptor structure. This descriptor, located in FLASH
 * memory, describes the usage of the device in one of its supported
 * configurations, including information about any device interfaces and
 * endpoints. The descriptor is read out by the USB host during the enumeration
 * process when selecting a configuration so that the host may correctly
 * communicate with the USB device.
 */
__code USB_Descriptor_Configuration_t ConfigurationDescriptor = {
    .Config = {.Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t),
                          .Type = DTYPE_Configuration},

               .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
               .TotalInterfaces = 1,

               .ConfigurationNumber = 1,
               .ConfigurationStrIndex = NO_DESCRIPTOR,

               .ConfigAttributes = (USB_CONFIG_ATTR_RESERVED),

               .MaxPowerConsumption = USB_CONFIG_POWER_MA(200)},

    .HID_Interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                                 .Type = DTYPE_Interface},

                      .InterfaceNumber = 0,
                      .AlternateSetting = 0x00,

                      .TotalEndpoints = 1,

                      .Class = HID_CSCP_HIDClass,
                      .SubClass = HID_CSCP_BootSubclass,
                      .Protocol = HID_CSCP_KeyboardBootProtocol,

                      .InterfaceStrIndex = NO_DESCRIPTOR},

    .HID_KeyboardHID = {.Header = {.Size = sizeof(USB_HID_Descriptor_HID_t),
                                   .Type = HID_DTYPE_HID},

                        .HIDSpec = VERSION_BCD(1, 1, 0),
                        .CountryCode = 0x00,
                        .TotalReportDescriptors = 1,
                        .HIDReportType = HID_DTYPE_Report,
                        .HIDReportLength = sizeof(ReportDescriptor)},

    .HID_ReportINEndpoint = {.Header = {.Size =
                                            sizeof(USB_Descriptor_Endpoint_t),
                                        .Type = DTYPE_Endpoint},

                             .EndpointAddress = KEYBOARD_EPADDR,
                             .Attributes =
                                 (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC |
                                  ENDPOINT_USAGE_DATA),
                             .EndpointSize = KEYBOARD_EPSIZE,
                             .PollingIntervalMS = 10},

    /*.HID_ReportOUTEndpoint = {.Header = {.Size =
                                             sizeof(USB_Descriptor_Endpoint_t),
                                         .Type = DTYPE_Endpoint},

                              .EndpointAddress = KEYBOARD_LED_EPADDR,
                              .Attributes =
                                  (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC |
                                   ENDPOINT_USAGE_DATA),
                              .EndpointSize = KEYBOARD_EPSIZE,
                              .PollingIntervalMS = 10},*/
};

__code uint8_t ReportDescriptor[] = {
  // --- Report ID 1: 通常キーボード ---
  0x05, 0x01,       // Usage Page (Generic Desktop)
  0x09, 0x06,       // Usage (Keyboard)
  0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       // Report ID = 1
    0x05, 0x07,       //   Usage Page (Keyboard)
    0x19, 0xE0, 0x29, 0xE7,
    0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x08, 0x81, 0x02,  // Modifier (8bit)
    0x75, 0x08, 0x95, 0x01, 0x81, 0x03,  // Reserved
    0x75, 0x08, 0x95, 0x06,
    0x15, 0x00, 0x25, 0xFF,
    0x05, 0x07, 0x19, 0x00, 0x29, 0xE7, 0x81, 0x00,  // Keycode (6byte)
  0xC0,

  // --- Report ID 2: Consumer Control (音量キー) ---
  0x05, 0x0C,       // Usage Page (Consumer Devices)
  0x09, 0x01,       // Usage (Consumer Control)
  0xA1, 0x01,       // Collection (Application)
    0x85, 0x02,       // Report ID = 2
    0x15, 0x00,       // Logical Minimum (0)
    0x26, 0xFF, 0x03, // Logical Maximum (0x03FF)
    0x19, 0x00,       // Usage Minimum (0)
    0x2A, 0xFF, 0x03, // Usage Maximum (0x03FF)
    0x75, 0x10,       // Report Size (16 bits)
    0x95, 0x01,       // Report Count (1)
    0x81, 0x00,       // Input (Data, Array)
  0xC0
};

// String Descriptors
__code uint8_t LanguageDescriptor[] = {0x04, 0x03, 0x09,
                                       0x04}; // Language Descriptor
__code uint16_t SerialDescriptor[] = {        // Serial String Descriptor
    (((7 + 1) * 2) | (DTYPE_String << 8)),
    'V',
    'o',
    'l',
    'u',
    'm',
    'e',
    '2'};
__code uint16_t ProductDescriptor[] = {
    // Produce String Descriptor
    (((8 + 1) * 2) | (DTYPE_String << 8)),
    'E',
    'C',
    'm',
    'i',
    'n',
    'i',
    '0',
    '2',
};
__code uint16_t ManufacturerDescriptor[] = {
    // SDCC is little endian
    (((9 + 1) * 2) | (DTYPE_String << 8)), 'Q', 'u', 'a', 'd', 'S', 't', 'a', 't', 'e',
};
