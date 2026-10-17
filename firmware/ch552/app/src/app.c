/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT */
#include "ec.h"
#include <string.h>
static EC_X uint8_t feature[64];
static EC_X kbd_state_t desired, next_state;
static EC_X kbd_diag_t transport;
static EC_X kbd_ticket_t ticket;
static EC_X uint8_t maintenance, rearm, suppression, next_key, reply_pending, pending_isp, isp_armed, app_fault;
static EC_X uint16_t session;
static EC_X uint32_t isp_nonce, isp_armed_at;

static void reset_inputs(void) {
    ec_logic_reset(); memset(&desired,0,sizeof(desired)); rearm=1;
}
static void cancel_inputs(void) {
    kbd_cancel(); session=kbd_session(); reset_inputs(); isp_armed=0;
}
void ec_emit(uint8_t bindings) {
    uint8_t result;
    if(suppression) return;
    ec_compose(bindings,&next_state);
    if(!memcmp(&desired,&next_state,sizeof(desired))) return;
    result=kbd_submit(&next_state,&ticket);
    if(result==KBD_ACCEPTED) memcpy(&desired,&next_state,sizeof(desired));
    else {
        cancel_inputs(); app_fault=EC_SUP_QUEUE; suppression|=EC_SUP_QUEUE|EC_SUP_REARM;
    }
}
static uint8_t outputs_zero(void) {
    kbd_get_diagnostics(&transport);
    return transport.ready && !transport.queued && !desired.modifiers && !desired.media && !desired.keys[0];
}
static uint8_t request_valid(void) {
    uint8_t i;
    if(feature[0]!=3 || feature[1]!=1 || feature[5] || feature[6]>56 || feature[7]) return 0;
    for(i=8+feature[6];i<64;i++) if(feature[i]) return 0;
    return 1;
}
static void info(uint8_t EC_X *p) {
    p[0]='E';p[1]='R';p[2]='C';p[3]='2';p[4]=EC_KEYS;p[5]=1;
    p[6]=ec_saved;p[7]=ec_storage_dirty();
    ec_put16(p+8,ec_generation);ec_put16(p+10,session);
    ec_put32(p+12,24000000UL);ec_put32(p+16,2000000UL);
    ec_put16(p+20,3000);ec_put16(p+22,20000);ec_put16(p+24,EC_CHARGE_TICKS);
    p[26]=0;p[27]=0;
}
static void key_stats(uint8_t key, uint8_t EC_X *p, uint32_t now) {
    ec_key_t EC_X *s=&ec_keys[key];
    p[0]=key;p[1]=s->valid;p[2]=s->reason;p[3]=s->stable;
    ec_put16(p+4,s->raw);ec_put16(p+6,s->last_raw);
    ec_put32(p+8,s->attempts);ec_put32(p+12,s->valid_count);
    ec_put32(p+16,s->irq_fail);ec_put32(p+20,s->timeout_fail);ec_put32(p+24,s->other_fail);
    ec_put32(p+28,s->seen?now-s->last_valid:0xffffffffUL);
    ec_put32(p+32,s->max_gap);
}
static void output_stats(uint8_t EC_X *p) {
    uint8_t i;
    kbd_get_diagnostics(&transport);
    ec_put16(p,transport.session);ec_put16(p+2,transport.accepted);ec_put16(p+4,transport.completed);
    p[6]=transport.queued;p[7]=transport.ready;p[8]=transport.suspended;p[9]=transport.fault;
    p[10]=suppression;p[11]=ec_physical;p[12]=desired.modifiers;
    for(i=0;i<6;i++) p[13+i]=desired.keys[i];
    p[19]=desired.media;
}
static void command(uint32_t now) {
    uint8_t EC_X *p=feature+8;
    uint8_t length=feature[6], out=0, status=0, i,result;
    /* A different command must not acknowledge an unread ISP_EXEC response. */
    pending_isp=0;
    if(!request_valid()) status=2;
    else switch(feature[2]) {
    case 0:
        if(length) {status=2;break;}
        info(p);out=28;break;
    case 1:
        if(length) {status=2;break;}
        memcpy(p,ec_config,EC_CONFIG_SIZE);out=EC_CONFIG_SIZE;break;
    case 2:
        if(length!=38 || !ec_validate(p+2)) {status=2;break;}
        if(ec_u16(p)!=ec_generation) {status=3;break;}
        memcpy(ec_config,p+2,EC_CONFIG_SIZE);ec_generation++;
        ec_storage_changed();cancel_inputs();
        ec_put16(p,ec_generation);out=2;break;
    case 3:
        if(length!=4 || memcmp(p,"SAVE",4)) {status=2;break;}
        if(!maintenance || !outputs_zero()) {status=3;break;}
        result=ec_storage_save();
        reset_inputs();
        if(!result) {status=4;break;}
        p[0]=(result==2);ec_put16(p+1,ec_generation);out=3;break;
    case 4:
        if(length!=1 || p[0]>=EC_KEYS) {status=2;break;}
        key_stats(p[0],p,now);out=36;break;
    case 5:
        if(length) {status=2;break;}
        output_stats(p);out=20;break;
    case 6:
        if(length!=1 || p[0]>1) {status=2;break;}
        maintenance=p[0];cancel_inputs();
        isp_armed=0;p[0]=maintenance;out=1;break;
    case 7:
        if(length!=4 || memcmp(p,"ARM!",4)) {status=2;break;}
        if(!maintenance || !outputs_zero()) {status=3;break;}
        isp_nonce=now^((uint32_t)session<<16)^0x45524332UL;
        isp_armed_at=now;isp_armed=1;
        ec_put32(p,isp_nonce);out=4;break;
    case 8:
        if(length!=8 || memcmp(p+4,"ISP!",4)) {status=2;break;}
        if(!maintenance || !outputs_zero() || !isp_armed ||
           now-isp_armed_at>=6000000UL || ec_u16(p)!=(uint16_t)isp_nonce ||
           ec_u16(p+2)!=(uint16_t)(isp_nonce>>16)) {status=3;break;}
        isp_armed=0;pending_isp=1;break;
    default: status=2;break;
    }
    feature[5]=status;feature[6]=out;feature[7]=0;
    for(i=8+out;i<64;i++) feature[i]=0;
    reply_pending=!kbd_feature_reply(feature);
}
void ec_app_init(void) {
    /* Use the source values on every startup. */
    ec_defaults();
    ec_measure_init();session=kbd_session();reset_inputs();
    suppression=EC_SUP_REARM|EC_SUP_OFFLINE;
}
void ec_app_poll(void) {
    uint16_t raw;
    uint8_t reason;
    uint32_t now;
    kbd_poll();
    if(session!=kbd_session()) {
        session=kbd_session();reset_inputs();reply_pending=0;pending_isp=0;isp_armed=0;
    }
    if(reply_pending && kbd_feature_reply(feature)) reply_pending=0;
    now=kbd_now();
    if((isp_armed || pending_isp) && now-isp_armed_at>=6000000UL) {isp_armed=0;pending_isp=0;}
    if(pending_isp && !reply_pending && kbd_feature_reply_sent()) {pending_isp=0;kbd_isp();}
    reason=ec_measure(next_key,&raw);now=kbd_now();
    /* Check the old timestamps before a recovered sample can hide a stale gap. */
    if(!ec_fresh(now) && !rearm) {cancel_inputs();app_fault=EC_SUP_MEASUREMENT;}
    ec_sample(next_key,raw,reason,now);
    next_key++;if(next_key==EC_KEYS) next_key=0;
    suppression=0;
    if(!kbd_ready()) suppression|=EC_SUP_OFFLINE;
    if(maintenance) suppression|=EC_SUP_MAINT;
    if(!ec_config[31]) suppression|=EC_SUP_UNCONFIGURED;
    if(!ec_fresh(now)) suppression|=EC_SUP_MEASUREMENT;
    if(!suppression && rearm && ec_released(now)) {rearm=0;app_fault=0;}
    if(rearm) suppression|=EC_SUP_REARM|app_fault;
    if(!suppression) ec_chord_step(now);
    if(!reply_pending && kbd_feature_take(feature)) command(now);
}
