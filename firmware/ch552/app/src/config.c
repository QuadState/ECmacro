/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT */
#include "ec.h"
#include <string.h>
EC_X uint8_t ec_config[EC_CONFIG_SIZE];
EC_X uint16_t ec_generation;
uint16_t ec_u16(const uint8_t EC_X *p) { return (uint16_t)p[0] | (uint16_t)p[1] << 8; }
void ec_put16(uint8_t EC_X *p, uint16_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
void ec_put32(uint8_t EC_X *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
void ec_defaults(void) {
    uint8_t i;
    /* Fixed startup settings, in 24 MHz Timer0 ticks.
     * Edit these values and rebuild for the electrodes on your own board.
     */
    memset(ec_config,0,EC_CONFIG_SIZE);
    EC_DEFAULT_BINDINGS(ec_config);
    for(i=0;i<EC_KEYS;i++) {
        ec_put16(ec_config+4*i,500);     /* release: raw <= this value */
        ec_put16(ec_config+4*i+2,600);   /* press: raw >= this value */
    }
    ec_config[31]=(1u<<EC_KEYS)-1;       /* enable the populated keys */
    ec_generation=1;
}
static uint8_t binding_valid(const uint8_t EC_X *p) {
    return (!p[1] || (p[1]>=4 && p[1]<=0xdf)) && !(p[2]&0x80) &&
           (!p[2] || (!p[0] && !p[1]));
}
uint8_t ec_validate(const uint8_t EC_X *p) {
    uint8_t i;
    uint16_t release, press;
    if ((p[31] & (uint8_t)~((1u<<EC_KEYS)-1)) || p[33] || p[34] || p[35]) return 0;
    for(i=0;i<4;i++) {
        release=ec_u16(p+4*i); press=ec_u16(p+4*i+2);
#if EC_KEYS == 2
        if(i>=EC_KEYS) {
            if(release || press || p[16+3*i] || p[17+3*i] || p[18+3*i]) return 0;
        } else
#endif
        {
            if ((release || press || (p[31]&(1u<<i))) && (!release || release>=press || press==65535u)) return 0;
            if(!binding_valid(p+16+3*i)) return 0;
        }
    }
#if EC_KEYS == 2
    if(p[32]>100 || !binding_valid(p+28)) return 0;
    if((p[28] || p[29] || p[30]) && !p[32]) return 0;
#else
    if(p[28] || p[29] || p[30] || p[32]) return 0;
#endif
    return 1;
}
void ec_compose(uint8_t bindings, kbd_state_t EC_X *s) {
    uint8_t i,j,key;
    const uint8_t EC_X *p;
    memset(s,0,sizeof(*s));
    for(i=0;i<5;i++) if(bindings&(1u<<i)) {
        p=ec_config+(i==4?28:16+3*i);
        s->modifiers|=p[0]; s->media|=p[2]; key=p[1];
        if(key) for(j=0;j<6;j++) {
            if(s->keys[j]==key) break;
            if(!s->keys[j]) { s->keys[j]=key; break; }
        }
    }
}
