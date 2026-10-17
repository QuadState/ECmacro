/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT
 * Independent implementation from USB 2.0 ch9, HID 1.11 and WCH CH552 DS.
 * USB ISR only marks disturbance and defers work. INT_BUSY holds transactions
 * until kbd_poll consumes the DMA buffer: no parsing, copying or waits in ISR.
 */
#include "kbd.h"
#include "ch552_regs.h"
#include "descriptors.h"
#include <string.h>

#ifdef __SDCC
volatile __bit kbd_timing_disturbed;
static __xdata __at(0x0000) uint8_t ep0[64];
static __xdata __at(0x0040) uint8_t ep1[64];
#else
volatile uint8_t kbd_timing_disturbed;
static uint8_t ep0[64], ep1[64];
uint32_t kbd_test_clock;
#endif

#define UF_RESET 1
#define UF_TRANSFER 2
#define UF_SUSPEND 4
#define UF_OVERFLOW 16
#define EP_NAK 2
#define EP_STALL 3
#define EP_RX_NAK 8
#define EP_RX_STALL 12
#define EP_RX_TOGGLE 128
#define EP_TX_TOGGLE 64
#define EP_AUTO 16
#define STALL_TICKS 1000000UL
#define DETACH_TICKS 200000UL
enum { CTRL_IDLE, CTRL_IN, CTRL_IN_LAST, CTRL_STATUS_OUT,
       CTRL_OUT, CTRL_STATUS_IN, CTRL_STALL };
enum { ACT_NONE, ACT_ADDRESS, ACT_CONFIG, ACT_FEATURE, ACT_HALT,
       ACT_UNHALT, ACT_IDLE };

static volatile uint16_t timer_high;
static KBD_XDATA kbd_state_t queue[KBD_QUEUE_SIZE], reported;
static KBD_XDATA uint16_t queue_serial[KBD_QUEUE_SIZE];
static KBD_XDATA uint8_t feature_rx[64], feature_tx[64];
static KBD_XDATA uint8_t control_small[9];
static KBD_XDATA uint32_t sent_at, detached_at, idle_at[2];
static KBD_XDATA uint16_t session_id, accepted_id, completed_id, claim_session;
static KBD_XDATA uint8_t head, tail, queued, configured, suspended, halted, syncing;
static KBD_XDATA uint8_t fault, detached, phase, packet_kind, packet_report;
static KBD_XDATA uint8_t idle_duration[2], idle_hold[2];
static KBD_XDATA uint8_t control_state, control_action, control_value, control_report;
static KBD_XDATA uint8_t control_left, control_index, control_zlp, control_feature;
static KBD_XDATA uint8_t feature_state, feature_sent;
static const uint8_t *control_source;

static void clear_flags(uint8_t mask)
{
#ifdef __SDCC
    USB_INT_FG = mask; /* W1C: never read/modify/write flag register. */
#else
    USB_INT_FG &= (uint8_t)~mask;
#endif
}

void kbd_usb_isr(void) KBD_ISR(8)
{
    kbd_timing_disturbed = 1;
    IE_USB = 0;
}

void kbd_timer2_isr(void) KBD_ISR(5)
{
    kbd_timing_disturbed = 1;
    TF2 = 0;
    ++timer_high;
}

uint32_t kbd_now(void)
{
#ifdef __SDCC
    uint8_t saved, lo, hi;
    uint16_t upper;
    saved = EA;
    EA = 0;
    upper = timer_high;
    /* Stable high/low read, including a wrap not serviced by the ISR yet. */
    do { hi = TH2; lo = TL2; } while (hi != TH2);
    if (TF2) {
        ++upper;
        do { hi = TH2; lo = TL2; } while (hi != TH2);
    }
    EA = saved;
    return ((uint32_t)upper << 16) | ((uint16_t)hi << 8) | lo;
#else
    return kbd_test_clock;
#endif
}

static void invalidate(uint8_t mailbox)
{
    UEP1_CTRL = (UEP1_CTRL & EP_TX_TOGGLE) | (halted ? EP_STALL : EP_NAK);
    ++session_id;
    if (!session_id) ++session_id;
    accepted_id = completed_id = 0;
    head = tail = queued = phase = packet_kind = 0;
    syncing = 1;
    if (mailbox) { feature_state = feature_sent = 0; claim_session = 0; }
    else if (feature_state == 2) claim_session = session_id;
    memset(&reported, 0, sizeof(reported));
}

static void abort_control(void)
{
    control_state = CTRL_IDLE;
    control_action = ACT_NONE;
    control_feature = 0;
    UEP0_CTRL = EP_NAK;
    UEP0_T_LEN = 0;
}

void kbd_cancel(void) { invalidate(0); }

void kbd_reconnect(void)
{
    if (detached) return;
    invalidate(1);
    abort_control();
    configured = suspended = halted = 0;
    USB_CTRL &= (uint8_t)~0x20;
    detached = 1;
    detached_at = kbd_now();
}

uint16_t kbd_session(void) { return session_id; }
uint8_t kbd_ready(void)
{
    return configured && !suspended && !halted && !detached && !syncing;
}

uint8_t kbd_submit(const kbd_state_t *state, kbd_ticket_t *ticket)
{
    uint8_t i, j;
    if (!kbd_ready()) return KBD_OFFLINE;
    if (state->media & 0x80) return KBD_INVALID;
    for (i = 0; i != 6; ++i) {
        if (state->keys[i] && (state->keys[i] < 4 || state->keys[i] > 0xdf))
            return KBD_INVALID;
        for (j = 0; j < i; ++j)
            if (state->keys[i] && state->keys[i] == state->keys[j])
                return KBD_INVALID;
    }
    if (queued == KBD_QUEUE_SIZE) return KBD_FULL;
    /* Ticket serials never alias within a session. Re-arm on exhaustion. */
    if (accepted_id == 65535u) { invalidate(1); return KBD_OFFLINE; }
    memcpy(&queue[tail], state, sizeof(*state));
    queue_serial[tail] = ++accepted_id;
    ticket->session = session_id;
    ticket->serial = accepted_id;
    if (++tail == KBD_QUEUE_SIZE) tail = 0;
    ++queued;
    return KBD_ACCEPTED;
}

uint8_t kbd_status(const kbd_ticket_t *ticket)
{
    if (ticket->session != session_id) return KBD_CANCELED;
    if (!ticket->serial || ticket->serial > accepted_id) return KBD_UNKNOWN;
    return ticket->serial <= completed_id ? KBD_ACKED : KBD_PENDING;
}

void kbd_get_diagnostics(kbd_diag_t *out)
{
    out->session = session_id;
    out->accepted = accepted_id;
    out->completed = completed_id;
    out->queued = queued;
    out->ready = kbd_ready();
    out->suspended = suspended;
    out->fault = fault;
}

uint8_t kbd_feature_take(uint8_t *buffer)
{
    if (feature_state != 1) return 0;
    memcpy(buffer, feature_rx, 64);
    feature_state = 2;
    claim_session = session_id;
    return 1;
}

uint8_t kbd_feature_reply(const uint8_t *buffer)
{
    if (feature_state != 2 || claim_session != session_id ||
        control_feature || buffer[0] != 3 || buffer[1] != 1 ||
        buffer[2] != feature_rx[2] || buffer[3] != feature_rx[3] ||
        buffer[4] != feature_rx[4]) return 0;
    memcpy(feature_tx, buffer, 64);
    feature_state = 3;
    feature_sent = 0;
    return 1;
}

uint8_t kbd_feature_reply_sent(void) { return feature_state == 3 && feature_sent; }

static void stall_control(void)
{
    control_state = CTRL_STALL;
    control_action = ACT_NONE;
    control_feature = 0;
    UEP0_CTRL = EP_RX_STALL | EP_STALL;
}

static void next_control_packet(void)
{
    uint8_t i, count = control_left > 8 ? 8 : control_left;
    for (i = 0; i < count; ++i) ep0[i] = control_source[control_index++];
    control_left -= count;
    if (!count) control_zlp = 0;
    control_state = (control_left || control_zlp) ? CTRL_IN : CTRL_IN_LAST;
    UEP0_T_LEN = count;
    UEP0_CTRL = (UEP0_CTRL & EP_TX_TOGGLE) | EP_RX_TOGGLE;
}

static void control_read(const uint8_t *data, uint8_t length, uint16_t wanted)
{
    control_source = data;
    control_left = wanted < length ? (uint8_t)wanted : length;
    control_index = 0;
    control_zlp = length < wanted && !(length & 7);
    UEP0_CTRL = EP_RX_TOGGLE | EP_TX_TOGGLE;
    if (!wanted) {
        /* USB 2.0 8.5.3: no data stage always has an IN status stage. */
        control_state = CTRL_STATUS_IN;
        UEP0_T_LEN = 0;
        UEP0_CTRL = EP_RX_NAK | EP_TX_TOGGLE;
    } else next_control_packet();
}

static void status_in(void)
{
    control_state = CTRL_STATUS_IN;
    UEP0_T_LEN = 0;
    /* ACK a repeated final OUT if its handshake was lost. Keep the next
     * expected RX toggle so control_out discards the duplicate data. */
    UEP0_CTRL = (UEP0_CTRL & EP_RX_TOGGLE) | EP_TX_TOGGLE;
}

static void setup_control(void)
{
    uint8_t type, request, low, high, index, index_high;
    uint16_t length;
    /* Read SETUP before any routine overwrites the shared endpoint DMA. */
    type = ep0[0]; request = ep0[1]; low = ep0[2]; high = ep0[3];
    index = ep0[4]; index_high = ep0[5];
    length = ep0[6] | ((uint16_t)ep0[7] << 8);
    abort_control();
    if (USB_RX_LEN != 8) { stall_control(); return; }
    if (request == 6 && (type == 0x80 || type == 0x81)) {
        if (type == 0x80 && high != 3 && (index || index_high)) goto invalid;
        if (type == 0x81 && (index || index_high || low)) goto invalid;
        if (type == 0x80 && high == 1 && !low)
            control_read(device_descriptor, sizeof(device_descriptor), length);
        else if (type == 0x80 && high == 2 && !low)
            control_read(configuration_descriptor, sizeof(configuration_descriptor), length);
        else if (type == 0x80 && high == 3) {
            if (low == 0) control_read(language_descriptor, sizeof(language_descriptor), length);
            else if (index == 9 && index_high == 4 && low == 1)
                control_read(manufacturer_descriptor, sizeof(manufacturer_descriptor), length);
            else if (index == 9 && index_high == 4 && low == 2)
                control_read(product_descriptor, sizeof(product_descriptor), length);
            else goto invalid;
        } else if (type == 0x81 && high == 0x21)
            control_read(configuration_descriptor + 18, 9, length);
        else if (type == 0x81 && high == 0x22)
            control_read(report_descriptor, sizeof(report_descriptor), length);
        else goto invalid;
        return;
    }
    if (index_high) goto invalid;
    if (type == 0 && request == 5 && !high && low < 128 && !index && !length && !configured) {
        control_action = ACT_ADDRESS; control_value = low; status_in(); return;
    }
    if (type == 0 && request == 9 && !high && low <= 1 && !index && !length) {
        control_action = ACT_CONFIG; control_value = low; status_in(); return;
    }
    if (type == 0x80 && request == 8 && !high && !low && !index && length == 1) {
        control_small[0] = configured; control_read(control_small, 1, length); return;
    }
    if (request == 0 && !high && !low && length == 2) {
        if ((type == 0x80 && !index) || (type == 0x81 && !index && configured) ||
            (type == 0x82 && (index == 0 || index == 0x80 || (index == 0x81 && configured)))) {
            control_small[0] = type == 0x82 && index == 0x81 ? halted : 0;
            control_small[1] = 0; control_read(control_small, 2, length); return;
        }
    }
    if (configured && type == 0x02 && (request == 1 || request == 3) &&
        !high && !low && index == 0x81 && !length) {
        control_action = request == 1 ? ACT_UNHALT : ACT_HALT; status_in(); return;
    }
    if (configured && !index) {
        if (type == 0x81 && request == 10 && !low && !high && length == 1) {
            control_small[0] = 0; control_read(control_small,1,length); return;
        }
        if (type == 0x01 && request == 11 && !low && !high && !length) {
            control_action = ACT_CONFIG; control_value = 1; status_in(); return;
        }
        if (type == 0xa1 && request == 1) { /* HID GET_REPORT */
            if (high == 3 && low == 3) {
                control_feature = 1;
                control_read(feature_tx,64,length); return;
            }
            if (high == 1 && (low == 1 || low == 2)) {
                control_small[0] = low;
                if (low == 1) {
                    control_small[1] = reported.modifiers; control_small[2] = 0;
                    memcpy(control_small + 3, reported.keys, 6);
                } else control_small[1] = reported.media;
                control_read(control_small,low == 1 ? 9 : 2,length); return;
            }
        }
        if (type == 0x21 && request == 9 && high == 3 && low == 3 &&
            length == 64 && feature_state != 1 && feature_state != 2) {
            control_index = 0; control_state = CTRL_OUT;
            UEP0_CTRL = EP_RX_TOGGLE | EP_NAK; return;
        }
        if (type == 0xa1 && request == 2 && !high && (low == 1 || low == 2) && length == 1) {
            control_small[0] = idle_duration[low - 1];
            control_read(control_small,1,length); return;
        }
        if (type == 0x21 && request == 10 && low <= 2 && !length) {
            control_action = ACT_IDLE; control_report = low; control_value = high;
            status_in(); return;
        }
    }
invalid:
    stall_control();
}

static void set_idle(uint8_t index)
{
    /* HID 1.11 7.2.4: retain the current period when fewer than 4 ms remain.
     * A retained duration is always nonzero; zero means no deferred period.
     * GET_IDLE reports the new setting; the next report finishes the old one. */
    if (!idle_hold[index] && idle_duration[index] &&
        (uint32_t)(kbd_now() - idle_at[index]) > (uint32_t)(idle_duration[index] - 1)*8000UL)
        idle_hold[index] = idle_duration[index];
    idle_duration[index] = control_value;
}

static void commit_control(void)
{
    uint8_t action = control_action;
    control_action = ACT_NONE;
    if (action == ACT_ADDRESS) USB_DEV_AD = control_value;
    else if (action == ACT_CONFIG) {
        invalidate(1); configured = control_value; halted = 0;
        UEP1_CTRL = EP_NAK;
        idle_duration[0] = idle_duration[1] = 0;
        idle_hold[0] = idle_hold[1] = 0;
    } else if (action == ACT_FEATURE) {
        memset(feature_tx,0,64);
        memcpy(feature_tx,feature_rx,5);
        feature_tx[5] = 1; /* BUSY echo of this exact command. */
        feature_state = 1; feature_sent = 0;
    } else if (action == ACT_HALT || action == ACT_UNHALT) {
        invalidate(1); halted = action == ACT_HALT;
        UEP1_CTRL = halted ? EP_STALL : EP_NAK; /* Clear resets DATA0. */
        if (halted) fault = KBD_FAULT_HALTED;
    } else if (action == ACT_IDLE) {
        if (control_report != 2) set_idle(0);
        if (control_report != 1) set_idle(1);
    }
}

static void control_in(void)
{
    UEP0_CTRL ^= EP_TX_TOGGLE;
    if (control_state == CTRL_IN) next_control_packet();
    else if (control_state == CTRL_IN_LAST) {
        control_state = CTRL_STATUS_OUT;
        UEP0_CTRL = EP_RX_TOGGLE | EP_NAK;
    } else if (control_state == CTRL_STATUS_IN) {
        commit_control(); abort_control();
    }
}

static void control_out(void)
{
    uint8_t i;
    if (!(USB_INT_ST & 0x40)) return; /* Retransmitted DATA packet. */
    if (control_state == CTRL_OUT) {
        if (USB_RX_LEN != 8 || control_index > 56) { stall_control(); return; }
        for (i = 0; i != 8; ++i) feature_rx[control_index++] = ep0[i];
        UEP0_CTRL ^= EP_RX_TOGGLE;
        if (control_index == 64) {
            if (feature_rx[0] != 3 || feature_rx[1] != 1 ||
                feature_rx[5] || feature_rx[6] > 56 || feature_rx[7]) {
                stall_control(); return;
            }
            for (i = 8 + feature_rx[6]; i != 64; ++i)
                if (feature_rx[i]) { stall_control(); return; }
            control_action = ACT_FEATURE; status_in();
        }
    } else if ((control_state == CTRL_STATUS_OUT || control_state == CTRL_IN ||
                control_state == CTRL_IN_LAST) && USB_RX_LEN == 0) {
        /* A status OUT can also abort a read before its final armed packet.
         * Require observed completion of every IN for application handoff.
         * Lost final ACK is indistinguishable here: complete EP0 normally,
         * but conservatively withhold reply_sent (and therefore ISP entry).
         */
        if (control_state == CTRL_STATUS_OUT && control_feature &&
            control_index == 64 && feature_state == 3)
            feature_sent = 1;
        abort_control();
    } else stall_control();
}

static void input_ack(void)
{
    uint8_t i;
    UEP1_CTRL = (UEP1_CTRL ^ EP_TX_TOGGLE) | EP_NAK;
    if (!packet_kind) return;
    idle_at[packet_report - 1] = kbd_now();
    idle_hold[packet_report - 1] = 0;
    if (packet_report == 1) {
        reported.modifiers = ep1[1];
        for (i = 0; i != 6; ++i) reported.keys[i] = ep1[i+3];
    } else reported.media = ep1[1];
    if (packet_kind == 1 || packet_kind == 2) {
        if (!phase) phase = 1;
        else {
            phase = 0;
            if (packet_kind == 1) syncing = 0;
            else {
                completed_id = queue_serial[head];
                if (++head == KBD_QUEUE_SIZE) head = 0;
                --queued;
            }
        }
    }
    packet_kind = 0;
}

static void schedule_input(void)
{
    uint8_t i, duration;
    const kbd_state_t *state;
    uint32_t now = kbd_now();
    if (!configured || suspended || halted || detached) return;
    if (packet_kind) {
        if ((uint32_t)(now - sent_at) >= STALL_TICKS) {
            fault = KBD_FAULT_STALL; kbd_reconnect();
        }
        return;
    }
    if (!(USB_MIS_ST & 0x20) || (USB_INT_FG & UF_TRANSFER)) return;
    if (syncing) {
        packet_kind = 1; packet_report = phase + 1; state = &reported;
    } else if (queued) {
        packet_kind = 2; packet_report = phase + 1; state = &queue[head];
    } else {
        state = &reported;
        for (i = 0; i != 2; ++i) {
            duration = idle_hold[i] ? idle_hold[i] : idle_duration[i];
            if (duration && (uint32_t)(now - idle_at[i]) >= (uint32_t)duration*8000UL) {
                packet_report = i + 1;
                break;
            }
        }
        if (i == 2) return;
        packet_kind = 3;
    }
    ep1[0] = packet_report;
    if (packet_report == 1) {
        ep1[1] = state->modifiers; ep1[2] = 0;
        for (i = 0; i != 6; ++i) ep1[i+3] = state->keys[i];
        UEP1_T_LEN = 9;
    } else { ep1[1] = state->media; UEP1_T_LEN = 2; }
    sent_at = now;
    UEP1_CTRL &= (uint8_t)~3;
}

static void sleep_if_suspended(void)
{
#ifdef __SDCC
    uint8_t saved;
    if (!suspended || detached) return;
    saved = EA;
    EA = 0;
    /* WCH DS 7.1/7.2: USB wake is independent of interrupt servicing.
     * Arm before the final flag/line-state check. Caller has finished
     * measurements and released all charged GPIOs before calling poll.
     * Timer2 pauses in sleep: this clock is not wall time across suspend.
     */
    SAFE_MOD = 0x55; SAFE_MOD = 0xaa; WAKE_CTRL = 0x80; SAFE_MOD = 0;
    if ((USB_MIS_ST & 4) && !(USB_INT_FG & 0x17)) PCON |= 2;
    SAFE_MOD = 0x55; SAFE_MOD = 0xaa; WAKE_CTRL = 0; SAFE_MOD = 0;
    EA = saved;
    kbd_timing_disturbed = 1;
#endif
}

void kbd_poll(void)
{
    uint8_t flags = USB_INT_FG;
    if (flags & UF_RESET) {
        invalidate(1); abort_control();
        configured = suspended = halted = 0;
        USB_DEV_AD = 0; UEP1_CTRL = EP_NAK;
        idle_duration[0] = idle_duration[1] = 0;
        idle_hold[0] = idle_hold[1] = 0;
        clear_flags(UF_RESET | UF_TRANSFER | UF_SUSPEND | UF_OVERFLOW);
    } else if (flags & UF_OVERFLOW) {
        fault = KBD_FAULT_OVERFLOW;
        kbd_reconnect(); clear_flags(UF_OVERFLOW | UF_TRANSFER);
    } else {
        if (flags & UF_SUSPEND) {
            /* Preserve transactions already ACKed before suspend: USB 2.0
             * 9.1.1.6 retains address/configuration, and 9.4.6 commits the
             * address after status. Feature commands are still canceled. */
            if (flags & UF_TRANSFER) {
                if ((USB_INT_ST & 0x3f) == 0x21)
                    UEP1_CTRL ^= EP_TX_TOGGLE;
                else if ((USB_INT_ST & 0x3f) == 0x20 &&
                         control_state == CTRL_STATUS_IN && control_action != ACT_FEATURE)
                    commit_control();
            }
            invalidate(1); abort_control();
            suspended = !!(USB_MIS_ST & 4);
            clear_flags(UF_SUSPEND | UF_TRANSFER);
        } else if (flags & UF_TRANSFER) {
            switch (USB_INT_ST & 0x3f) {
            case 0x30: setup_control(); break;
            case 0x20: control_in(); break;
            case 0x00: control_out(); break;
            case 0x21: input_ack(); break;
            default: break;
            }
            clear_flags(UF_TRANSFER);
        }
    }
    if (detached && (uint32_t)(kbd_now() - detached_at) >= DETACH_TICKS) {
        detached = 0; USB_CTRL |= 0x20;
    }
    schedule_input();
    IE_USB = 1;
    sleep_if_suspended();
}

void kbd_init(void)
{
    EA = 0; IE = 0; IE_EX = 0;
    SAFE_MOD = 0x55; SAFE_MOD = 0xaa; CLOCK_CFG = 0x86; SAFE_MOD = 0;
    T2CON = 0; T2MOD &= (uint8_t)~0x4f;
    TL2 = TH2 = RCAP2L = RCAP2H = 0; timer_high = 0;
    IP = 0; IP_EX = 0; /* Both tiny ISRs use register bank1, no nesting. */
    TF2 = 0; TR2 = 1; ET2 = 1;
    USB_CTRL = 0x06;
    USB_CTRL = 0; /* Release SIE reset before programming its registers. */
    UEP0_DMA_L = 0; UEP0_DMA_H = 0;
    UEP1_DMA_L = 0x40; UEP1_DMA_H = 0;
    UEP4_1_MOD = 0x40; UEP2_3_MOD = 0;
    session_id = 0; configured = suspended = halted = detached = fault = 0;
    idle_duration[0] = idle_duration[1] = 0;
    idle_hold[0] = idle_hold[1] = 0;
    invalidate(1); abort_control();
    memset(feature_tx,0,64); feature_tx[0] = 3; feature_tx[1] = 1; feature_tx[5] = 3;
    USB_DEV_AD = 0;
    UDEV_CTRL = 0x81;
    USB_CTRL = 0x29; /* Pullup, DMA and automatic busy until transfer consumed. */
    clear_flags(0x1f); USB_INT_EN = 0x17;
    IE_USB = 1; EA = 1;
}

void kbd_isp(void)
{
#ifdef __SDCC
    uint8_t periods;
    /* Deliberate terminal operation. Detach >=100 ms before official BOOT
     * entry; hardware Timer2 overflow bounds each wait, independent of IRQs. */
    EA = 0; IE = 0; IE_EX = 0; USB_INT_EN = 0;
    USB_CTRL = 0x06; UDEV_CTRL = 0;
    TCON = 0; SCON = SCON1 = 0; TKEY_CTRL = PWM_CTRL = ADC_CFG = 0;
    T2CON = 0; T2MOD = 0; RCAP2L = RCAP2H = TL2 = TH2 = 0;
    TR2 = 1;
    for (periods = 0; periods != 4; ++periods) {
        while (!TF2) { }
        TF2 = 0;
    }
    TR2 = 0;
    /* WCH CH55X ISP note: enter factory BOOT at 0x3800. No Flash/config writes. */
    __asm
        ljmp 0x3800
    __endasm;
#endif
}
