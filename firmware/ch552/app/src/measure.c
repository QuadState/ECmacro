/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT
 * WCH CH552 datasheet V1.8 sections 10 and 12.1. No vendor source copied.
 */
#include "ec.h"
#include "ch552_regs.h"
static const EC_CODE uint8_t pins[EC_KEYS]=EC_PINS;
#define PIN_MASK EC_PIN_MASK
void ec_measure_init(void) {
    TR0=0; ET0=0; TMOD=(TMOD&0xf0)|0x01;
    T2MOD|=0x90; /* Timer0 Fsys; Timer2 retains its independent /12 selection. */
    P1_DIR_PU&=(uint8_t)~PIN_MASK; P1_MOD_OC&=(uint8_t)~PIN_MASK;
    P1|=PIN_MASK;
}
uint8_t ec_measure(uint8_t key, uint16_t *raw) {
    uint8_t pin=pins[key], reason=EC_OK;
    kbd_timing_disturbed=0;
    /* Charge through push-pull drive. Only this electrical settling minimum waits. */
    P1_DIR_PU|=pin;
    TL0=0; TH0=0; TF0=0; TR0=1;
    while(TL0<EC_CHARGE_TICKS && !TF0 && !kbd_timing_disturbed) { }
    TR0=0;
    if(!(P1&pin)) reason=EC_CHARGE;
    TL0=0; TH0=0; TF0=0;
    TR0=1;
    P1_DIR_PU&=(uint8_t)~pin; /* High impedance: discharge through external 2 MOhm. */
    while(P1&pin) {
        if(TF0) { reason=EC_TIMEOUT; break; }
        if(kbd_timing_disturbed) { reason=EC_IRQ; break; }
    }
    TR0=0;
    *raw=(uint16_t)TH0<<8 | TL0;
    if(TF0 && !reason) reason=EC_OVERFLOW;
    if(kbd_timing_disturbed || (USB_INT_FG&0x17) || TF2) reason=EC_IRQ;
    return reason;
}
