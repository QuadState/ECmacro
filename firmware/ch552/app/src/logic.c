/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT */
#include "ec.h"
EC_X ec_key_t ec_keys[EC_KEYS];
EC_X uint8_t ec_physical;
#if EC_KEYS == 2
static EC_X uint8_t chord_mode, pending;
static EC_X uint32_t chord_start;
#endif
void ec_logic_reset(void) {
    uint8_t i;
    ec_physical=0;
    for(i=0;i<EC_KEYS;i++) {
        ec_keys[i].stable=0; ec_keys[i].tracking=0; ec_keys[i].released=0; ec_keys[i].seen=0;
    }
#if EC_KEYS == 2
    chord_mode=0; pending=0;
#endif
}
void ec_sample(uint8_t key, uint16_t raw, uint8_t reason, uint32_t now) {
    ec_key_t EC_X *s=&ec_keys[key];
    uint8_t target;
    uint32_t gap;
    s->attempts++; s->raw=raw; s->reason=reason; s->valid=(reason==EC_OK);
    if(reason) {
        if(reason==EC_IRQ) s->irq_fail++;
        else if(reason==EC_TIMEOUT) s->timeout_fail++;
        else s->other_fail++;
        /* An isolated ISR failure has no value to feed the debounce filter.
         * The next valid observation checks elapsed time and continuity. */
        return;
    }
    if(s->seen) {
        gap=now-s->last_valid;
        if(gap>s->max_gap) s->max_gap=gap;
        if(gap>=EC_STALE_TICKS) { s->tracking=0; s->released=0; }
    }
    s->seen=1; s->last_valid=now; s->last_raw=raw; s->valid_count++;
    if(!(ec_config[31]&(1u<<key))) return;
    target=s->stable;
    if(raw>=ec_u16(ec_config+4*key+2)) target=1;
    if(raw<=ec_u16(ec_config+4*key)) target=0;
    else {
        s->released=0;
        /* A band sample cannot start release confirmation for rearming. */
        if(!s->stable && !target) {s->tracking=0;return;}
    }
    if(!s->tracking || target!=s->candidate) {
        s->candidate=target; s->candidate_since=now; s->tracking=1;
    } else if(now-s->candidate_since>=EC_CONFIRM_TICKS) {
        s->stable=target;
        if(!target && raw<=ec_u16(ec_config+4*key)) s->released=1;
    }
    if(s->stable) ec_physical|=(uint8_t)(1u<<key);
    else ec_physical&=(uint8_t)~(1u<<key);
}
uint8_t ec_fresh(uint32_t now) {
    uint8_t i;
    for(i=0;i<EC_KEYS;i++) if(ec_config[31]&(1u<<i)) {
        if(!ec_keys[i].seen || now-ec_keys[i].last_valid>=EC_STALE_TICKS) return 0;
    }
    return 1;
}
uint8_t ec_released(uint32_t now) {
    uint8_t i;
    if(ec_physical || !ec_fresh(now)) return 0;
    for(i=0;i<EC_KEYS;i++) if((ec_config[31]&(1u<<i)) && !ec_keys[i].released) return 0;
    return 1;
}
void ec_chord_step(uint32_t now) {
#if EC_KEYS == 2
    uint8_t mask=ec_physical;
    if(!(ec_config[28]|ec_config[29]|ec_config[30]) || ec_config[31]!=3) { ec_emit(mask); return; }
    if(chord_mode==4) { if(!mask) chord_mode=0; return; }
    if(chord_mode==3) {
        if(mask!=3) { ec_emit(0); chord_mode=mask?4:0; }
        return;
    }
    if(!chord_mode) {
        if(!mask) return;
        if(mask==3) { chord_mode=3; ec_emit(16); return; }
        pending=mask; chord_start=now; chord_mode=1;
    }
    if(chord_mode==1) {
        if(now-chord_start<(uint32_t)ec_config[32]*2000UL && mask==3) {
            chord_mode=3; ec_emit(16); return;
        }
        if((mask&pending) && now-chord_start<(uint32_t)ec_config[32]*2000UL) return;
        /* A confirmed tap released inside the chord window still produces both edges. */
        ec_emit(pending); chord_mode=2;
    }
    ec_emit(mask);
    if(!mask) chord_mode=0;
#else
    (void)now;
    ec_emit(ec_physical);
#endif
}
