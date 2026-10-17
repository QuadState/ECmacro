/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT */
#ifndef QUADSTATE_KBD_H
#define QUADSTATE_KBD_H
#include <stdint.h>
#ifdef __SDCC
#define KBD_XDATA __xdata
#define KBD_CODE __code
#define KBD_ISR(n) __interrupt(n) __using(1)
extern volatile __bit kbd_timing_disturbed;
#else
#define KBD_XDATA
#define KBD_CODE
#define KBD_ISR(n)
extern volatile uint8_t kbd_timing_disturbed;
#endif
#define KBD_CLOCK_HZ 2000000UL
#define KBD_QUEUE_SIZE 6
#define KBD_ACCEPTED 0
#define KBD_FULL 1
#define KBD_OFFLINE 2
#define KBD_INVALID 3
#define KBD_PENDING 0
#define KBD_ACKED 1
#define KBD_CANCELED 2
#define KBD_UNKNOWN 3
#define KBD_FAULT_NONE 0
#define KBD_FAULT_STALL 1
#define KBD_FAULT_OVERFLOW 2
#define KBD_FAULT_HALTED 3

typedef struct { uint8_t modifiers, keys[6], media; } kbd_state_t;
typedef struct { uint16_t session, serial; } kbd_ticket_t;
typedef struct {
    uint16_t session, accepted, completed;
    uint8_t queued, ready, suspended, fault;
} kbd_diag_t;

void kbd_init(void);
void kbd_poll(void);
uint32_t kbd_now(void);
uint16_t kbd_session(void);
uint8_t kbd_ready(void);
uint8_t kbd_submit(const kbd_state_t *state, kbd_ticket_t *ticket);
uint8_t kbd_status(const kbd_ticket_t *ticket);
void kbd_cancel(void);
void kbd_reconnect(void);
void kbd_get_diagnostics(kbd_diag_t *out);
uint8_t kbd_feature_take(uint8_t *buffer);
uint8_t kbd_feature_reply(const uint8_t *buffer);
/* Complete GET_REPORT data transfer and its status OUT have been observed. */
uint8_t kbd_feature_reply_sent(void);
/* Only call after deliberate application authorization and reply_sent(). */
void kbd_isp(void);

/* Include this header in the translation unit containing main for vectors. */
void kbd_usb_isr(void) KBD_ISR(8);
void kbd_timer2_isr(void) KBD_ISR(5);
#endif
