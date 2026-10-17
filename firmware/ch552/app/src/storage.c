/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT
 * Two independent byte-rewritable records at DataFlash offsets 0 and 64.
 * Layout: ER,format,board,generationLE,config36,crcLE,commit A5 (45 bytes).
 * The inactive commit is invalidated FIRST; new commit written LAST.
 */
#include "ec.h"
EC_X uint8_t ec_saved;
static EC_X uint8_t saved_slot, dirty=1;
static EC_X uint16_t saved_generation;
static uint16_t crc_byte(uint16_t crc, uint8_t value) {
    uint8_t i;
    crc^=(uint16_t)value<<8;
    for(i=0;i<8;i++) crc=(crc&0x8000)?(crc<<1)^0x1021:crc<<1;
    return crc;
}
static uint16_t record_generation(uint8_t base) {
    return (uint16_t)ec_nv_read(base+4) | (uint16_t)ec_nv_read(base+5)<<8;
}
static uint8_t record_valid(uint8_t base) {
    uint8_t i;
    uint16_t crc=0xffff;
    if(ec_nv_read(base+44)!=0xa5 || ec_nv_read(base)!='E' || ec_nv_read(base+1)!='R' ||
       ec_nv_read(base+2)!=1 || ec_nv_read(base+3)!=EC_KEYS) return 0;
    for(i=0;i<42;i++) crc=crc_byte(crc,ec_nv_read(base+i));
    return (uint8_t)crc==ec_nv_read(base+42) && (uint8_t)(crc>>8)==ec_nv_read(base+43);
}
static void read_config(uint8_t base) {
    uint8_t i;
    for(i=0;i<EC_CONFIG_SIZE;i++) ec_config[i]=ec_nv_read(base+6+i);
}
uint8_t ec_storage_load(void) {
    uint8_t first=record_valid(0), second=record_valid(64);
    uint16_t a=0,b=0;
    ec_saved=0; dirty=1;
    if(!first && !second) return 0;
    if(first) a=record_generation(0);
    if(second) b=record_generation(64);
    saved_slot=(!first || (second && (int16_t)(b-a)>0))?64:0;
    read_config(saved_slot);
    if(!ec_validate(ec_config)) {
        /* A CRC-correct but semantically invalid newest record is never activated. */
        saved_slot^=64;
        if(!(saved_slot?second:first)) return 0;
        read_config(saved_slot);
        if(!ec_validate(ec_config)) return 0;
    }
    saved_generation=record_generation(saved_slot);
    ec_saved=1; dirty=0;
    return 1;
}
uint8_t ec_storage_dirty(void) { return dirty; }
void ec_storage_changed(void) {
    uint8_t i;
    dirty=1;
    if(!ec_saved) return;
    for(i=0;i<EC_CONFIG_SIZE;i++) if(ec_config[i]!=ec_nv_read(saved_slot+6+i)) return;
    dirty=0;
}
uint8_t ec_storage_save(void) {
    uint8_t base,i,value;
    uint16_t crc=0xffff, generation=saved_generation+1;
#ifdef __SDCC
    uint16_t session=kbd_session();
#endif
    if(!ec_validate(ec_config)) return 0;
    if(!ec_storage_dirty()) return 1;
    base=ec_saved?(saved_slot^64):0;
    if(!ec_nv_write(base+44,0)) return 0;
    for(i=0;i<42;i++) {
        if(i==0) value='E';
        else if(i==1) value='R';
        else if(i==2) value=1;
        else if(i==3) value=EC_KEYS;
        else if(i==4) value=(uint8_t)generation;
        else if(i==5) value=(uint8_t)(generation>>8);
        else value=ec_config[i-6];
        crc=crc_byte(crc,value);
        if(!ec_nv_write(base+i,value)) return 0;
        /* A byte write pauses scanning; service USB between independent bytes. */
#ifdef __SDCC
        kbd_poll();
        if(session!=kbd_session()) return 0;
#endif
    }
    if(!ec_nv_write(base+42,(uint8_t)crc) || !ec_nv_write(base+43,(uint8_t)(crc>>8)) ||
       !ec_nv_write(base+44,0xa5) || !record_valid(base)) return 0;
    saved_slot=base; saved_generation=generation; ec_saved=1; dirty=0;
    return 2;
}
