/* Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT */
#include "ec.h"
void main(void) {
    kbd_init();
    ec_app_init();
    for(;;) ec_app_poll();
}
