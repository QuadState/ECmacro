/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT
 * WCH CH552 datasheet V1.8 sections 6.4-6.5: byte rewrite in DataFlash only.
 */
#include "ec.h"
#include "ch552_regs.h"
uint8_t ec_nv_read(uint8_t address) {
    uint8_t value, enabled=EA;
    if(address>=128) return 0xff;
    EA=0;
    ROM_ADDR_H=0xc0; ROM_ADDR_L=(uint8_t)(address<<1);
    ROM_CTRL=0x8e;
    value=(ROM_STATUS&2)?0xff:ROM_DATA_L;
    EA=enabled;
    return value;
}
uint8_t ec_nv_write(uint8_t address, uint8_t value) {
    uint8_t ok, enabled=EA;
    if(address>=128) return 0;
    EA=0;
    SAFE_MOD=0x55; SAFE_MOD=0xaa;
    GLOBAL_CFG=(GLOBAL_CFG&0xf3)|0x04; /* Never enable CODE_WE. */
    SAFE_MOD=0;
    ROM_ADDR_H=0xc0; ROM_ADDR_L=(uint8_t)(address<<1); ROM_DATA_L=value;
    ROM_CTRL=0x9a;
    ok=((ROM_STATUS&0x42)==0x40);
    SAFE_MOD=0x55; SAFE_MOD=0xaa;
    GLOBAL_CFG&=0xf3; /* Close BOTH write gates on every exit. */
    SAFE_MOD=0;
    EA=enabled;
    return ok && ec_nv_read(address)==value;
}
