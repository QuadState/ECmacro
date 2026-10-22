<!-- Copyright (c) 2026 QuadState. SPDX-License-Identifier: CC-BY-4.0 -->
# CH552 firmware

Firmware for the ECmacro models with a CH552E.
It charges each key's electrode, lets it discharge, and times the discharge
with a 24 MHz timer to decide whether the key is pressed or released.

## Models

What differs between models (number of keys, pins, initial key assignments)
is in one file per model in [models/](models/). The Makefile builds every
model there. The common source supports models with two and four keys.

Assignments in [models/2V.h](models/2V.h):

| Key | Pin | Action |
|---|---|---|
| SW1 | P1.4 | Volume up |
| SW2 | P1.5 | Volume down |
| SW1 and SW2 together | | Mute |

## Build

Uses [SDCC](https://sdcc.sourceforge.net/) 4.5.0 and GNU make.

```sh
sudo apt install sdcc make
make
```

This makes `build/ECmacro-2V.bin`.
The BIN is kept within 13,312 bytes (0x3400), clear of the ISP area from 0x3800.

## Thresholds

The thresholds are set in `ec_defaults()` in [app/src/config.c](app/src/config.c).
Every key starts with release 500 and press 600, in 24 MHz timer ticks.
A key is pressed when its measured value is at or above press, and released
when it is at or below release. Adjust them to your electrodes, within
`0 < release < press < 65535`, and build again.

## Flashing

Uses [wchisp](https://github.com/ch32-rs/wchisp).

1. Connect the BOOT pads TP1 (D+) and TP2 (to V33 through 10 kOhm) on the PCB,
   and plug in USB while they are connected.
2. Once the board shows up as WCH ISP, disconnect the BOOT pads.
3. Write the firmware:

```sh
wchisp flash build/ECmacro-2V.bin
```

## License

- `app/`, `usb/`, `models/`, `Makefile`: MIT ([text](../../LICENSES/MIT.txt))
- This README: CC BY 4.0 ([text](../../LICENSES/CC-BY-4.0.txt))

The SDCC runtime and wchisp keep their own licenses.
