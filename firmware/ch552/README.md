<!-- Copyright (c) 2026 QuadState. SPDX-License-Identifier: CC-BY-4.0 -->
# ECmacro 2V firmware (CH552)

Firmware for the ECmacro 2V, two keys in a column.
The schematic and PCB are in [hardware/2V](../../hardware/2V/).

The CH552E runs from the 5 V of USB, on its internal 24 MHz clock.
Each electrode is connected to GND through a 2 MOhm resistor, and the firmware
decides whether a key is pressed from how long its electrode takes to discharge.

| Key | GPIO | Action |
|---|---|---|
| SW1 (top) | P1.4 | Volume up |
| SW2 (bottom) | P1.5 | Volume down |
| SW1 and SW2 together | | Mute |

## Models

What differs between models (number of keys, pins, initial key assignments)
is in one file per model in [models/](models/) ([models/2V.h](models/2V.h) for
the 2V). For another layout, add a file to models/: the Makefile builds every
model it finds there. The common source supports two and four keys; behaviour
that depends on the number of keys, such as muting with both keys of a
two-key model, follows the number of keys.

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
A key is pressed when its raw value is at or above press, and released when it
is at or below release. Adjust them to your electrodes, within
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
