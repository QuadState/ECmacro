/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT */
/* ECmacro 2V: SW1 (top) volume up, SW2 (bottom) volume down, both together mute. */
#define EC_KEYS 2
#define EC_PINS {0x10,0x20}                 /* P1.4, P1.5 */
#define EC_PIN_MASK 0x30
#define EC_DEFAULT_BINDINGS(c) do { \
    (c)[18]=1; (c)[21]=2; (c)[30]=4; (c)[32]=40; } while(0)
