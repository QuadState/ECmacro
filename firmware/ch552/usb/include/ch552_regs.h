/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT
 * Register facts independently transcribed from WCH CH552 datasheet V1F,
 * chapters 4, 8, 9, 12 and 16. This is not a vendor SDK header.
 */
#ifndef CH552_REGS_H
#define CH552_REGS_H
#include <stdint.h>
#ifdef __SDCC
#define CH_SFR(name, address) __sfr __at(address) name
#define CH_BIT(name, address) __sbit __at(address) name
#else
#define CH_SFR(name, address) extern volatile uint8_t name
#define CH_BIT(name, address) extern volatile uint8_t name
#endif
CH_SFR(ROM_ADDR_L,0x84); CH_SFR(ROM_ADDR_H,0x85);
CH_SFR(ROM_CTRL,0x86); CH_SFR(ROM_STATUS,0x86);
CH_SFR(PCON,0x87); CH_SFR(TCON,0x88); CH_SFR(TMOD,0x89);
CH_SFR(TL0,0x8a); CH_SFR(TL1,0x8b); CH_SFR(TH0,0x8c); CH_SFR(TH1,0x8d);
CH_SFR(ROM_DATA_L,0x8e); CH_SFR(ROM_DATA_H,0x8f);
CH_SFR(P1,0x90); CH_SFR(USB_C_CTRL,0x91);
CH_SFR(P1_MOD_OC,0x92); CH_SFR(P1_DIR_PU,0x93);
CH_SFR(P3_MOD_OC,0x96); CH_SFR(P3_DIR_PU,0x97);
CH_SFR(SCON,0x98); CH_SFR(ADC_CFG,0x9a); CH_SFR(PWM_CTRL,0x9d);
CH_SFR(SAFE_MOD,0xa1); CH_SFR(XBUS_AUX,0xa2);
CH_SFR(IE,0xa8); CH_SFR(WAKE_CTRL,0xa9); CH_SFR(P3,0xb0); CH_SFR(GLOBAL_CFG,0xb1);
CH_SFR(IP,0xb8); CH_SFR(CLOCK_CFG,0xb9);
CH_SFR(SCON1,0xc0); CH_SFR(TKEY_CTRL,0xc3); CH_SFR(PIN_FUNC,0xc6);
CH_SFR(T2CON,0xc8); CH_SFR(T2MOD,0xc9);
CH_SFR(RCAP2L,0xca); CH_SFR(RCAP2H,0xcb); CH_SFR(TL2,0xcc); CH_SFR(TH2,0xcd);
CH_SFR(UDEV_CTRL,0xd1); CH_SFR(UEP1_CTRL,0xd2); CH_SFR(UEP1_T_LEN,0xd3);
CH_SFR(USB_INT_FG,0xd8); CH_SFR(USB_INT_ST,0xd9); CH_SFR(USB_MIS_ST,0xda);
CH_SFR(USB_RX_LEN,0xdb); CH_SFR(UEP0_CTRL,0xdc); CH_SFR(UEP0_T_LEN,0xdd);
CH_SFR(USB_INT_EN,0xe1); CH_SFR(USB_CTRL,0xe2); CH_SFR(USB_DEV_AD,0xe3);
CH_SFR(IE_EX,0xe8); CH_SFR(IP_EX,0xe9); CH_SFR(UEP4_1_MOD,0xea); CH_SFR(UEP2_3_MOD,0xeb);
CH_SFR(UEP0_DMA_L,0xec); CH_SFR(UEP0_DMA_H,0xed);
CH_SFR(UEP1_DMA_L,0xee); CH_SFR(UEP1_DMA_H,0xef);
CH_BIT(TR0,0x8c); CH_BIT(TF0,0x8d); CH_BIT(TR1,0x8e); CH_BIT(TF1,0x8f);
CH_BIT(ET0,0xa9); CH_BIT(ET1,0xab); CH_BIT(ET2,0xad); CH_BIT(EA,0xaf);
CH_BIT(TR2,0xca); CH_BIT(TF2,0xcf); CH_BIT(IE_USB,0xea);
#endif
