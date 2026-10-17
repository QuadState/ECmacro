/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT */
#ifndef EC_H
#define EC_H
#include <stdint.h>
#include "kbd.h"
/* The model comes from models/<name>.h: the Makefile builds every model there
 * and passes its header as EC_MODEL_H. Each model sets:
 *   EC_KEYS                 number of keys (2 or 4)
 *   EC_PINS, EC_PIN_MASK    P1 bit of SW1, SW2, ... as wired on the PCB
 *   EC_DEFAULT_BINDINGS(c)  initial key assignments in the config block c
 */
#ifndef EC_MODEL_H
#error Build with make, which takes the model from models/
#endif
#include EC_MODEL_H
#if EC_KEYS != 2 && EC_KEYS != 4
#error EC_KEYS must be 2 or 4
#endif
#ifdef __SDCC
#define EC_X __xdata
#define EC_CODE __code
#else
#define EC_X
#define EC_CODE
#endif
#define EC_CONFIG_SIZE 36
#define EC_CONFIRM_TICKS 6000UL
#define EC_STALE_TICKS 40000UL
#define EC_CHARGE_TICKS 48
#define EC_OK 0
#define EC_IRQ 1
#define EC_TIMEOUT 2
#define EC_CHARGE 3
#define EC_OVERFLOW 4
#define EC_SUP_OFFLINE 1
#define EC_SUP_MAINT 2
#define EC_SUP_UNCONFIGURED 4
#define EC_SUP_REARM 8
#define EC_SUP_MEASUREMENT 16
#define EC_SUP_QUEUE 32

typedef struct {
    uint16_t raw, last_raw;
    uint32_t attempts, valid_count, irq_fail, timeout_fail, other_fail;
    uint32_t last_valid, max_gap, candidate_since;
    uint8_t reason, valid, seen, stable, candidate, tracking, released;
} ec_key_t;
extern EC_X uint8_t ec_config[EC_CONFIG_SIZE];
extern EC_X ec_key_t ec_keys[EC_KEYS];
extern EC_X uint8_t ec_physical;
extern EC_X uint16_t ec_generation;
uint16_t ec_u16(const uint8_t EC_X *p);
void ec_put16(uint8_t EC_X *p, uint16_t value);
void ec_put32(uint8_t EC_X *p, uint32_t value);
void ec_defaults(void);
uint8_t ec_validate(const uint8_t EC_X *config);
void ec_compose(uint8_t bindings, kbd_state_t EC_X *state);
void ec_logic_reset(void);
void ec_sample(uint8_t key, uint16_t raw, uint8_t reason, uint32_t now);
uint8_t ec_fresh(uint32_t now);
uint8_t ec_released(uint32_t now);
void ec_chord_step(uint32_t now);
void ec_emit(uint8_t bindings);
void ec_measure_init(void);
uint8_t ec_measure(uint8_t key, uint16_t *raw);
uint8_t ec_nv_read(uint8_t address);
uint8_t ec_nv_write(uint8_t address, uint8_t value);
uint8_t ec_storage_load(void);
uint8_t ec_storage_dirty(void);
void ec_storage_changed(void);
uint8_t ec_storage_save(void); /* 0 failed, 1 unchanged, 2 written. */
extern EC_X uint8_t ec_saved;
void ec_app_init(void);
void ec_app_poll(void);
#endif
