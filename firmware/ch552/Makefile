# Copyright (c) 2026 QuadState. SPDX-License-Identifier: MIT
# Builds the firmware of every model in models/ with SDCC and GNU make:
# models/<name>.h gives build/ECmacro-<name>.bin. One model: make MODELS=<name>
SDCC ?= sdcc
SDAR ?= sdar
MAKEBIN ?= makebin
BUILD ?= build
APP := app
LIB := usb
MODELS ?= $(sort $(basename $(notdir $(wildcard models/*.h))))
ifeq ($(strip $(MODELS)),)
$(error No model in models/)
endif
SOURCES := $(wildcard $(APP)/src/*.c)
HEADERS := $(APP)/include/ec.h $(LIB)/include/kbd.h $(LIB)/include/ch552_regs.h Makefile
APPFLAGS := -mmcs51 --model-small --std-sdcc11 --opt-code-size -I$(APP)/include -I$(LIB)/include -Imodels
LIBFLAGS := -mmcs51 --model-small --std-c11 --opt-code-size -I$(LIB)/include
LINKFLAGS := -mmcs51 --model-small --xram-loc 0x0080 --xram-size 0x0380 --iram-size 256 --code-size 0x3400
.DELETE_ON_ERROR:
.PHONY: all clean
all: $(foreach m,$(MODELS),$(BUILD)/ECmacro-$(m).bin)

$(BUILD)/kbd.rel: $(LIB)/src/kbd.c $(LIB)/src/descriptors.h $(LIB)/include/kbd.h $(LIB)/include/ch552_regs.h Makefile
	@mkdir -p $(@D)
	$(SDCC) $(LIBFLAGS) -c $< -o $@
$(BUILD)/libch552kbd.lib: $(BUILD)/kbd.rel
	$(SDAR) -rc $@ $<

# One model: the app sources compiled with its header, then linked.
define model_rules
$(BUILD)/$(1)/%.rel: $(APP)/src/%.c models/$(1).h $(HEADERS)
	@mkdir -p $$(@D)
	$$(SDCC) $$(APPFLAGS) '-DEC_MODEL_H="$(1).h"' -c $$< -o $$@
$(BUILD)/ECmacro-$(1).ihx: $(patsubst $(APP)/src/%.c,$(BUILD)/$(1)/%.rel,$(SOURCES)) $(BUILD)/libch552kbd.lib
	$$(SDCC) $$(LINKFLAGS) $$^ -o $$@
endef
$(foreach m,$(MODELS),$(eval $(call model_rules,$(m))))

# Fill unused bytes with FF to the fixed safe image limit. ISP starts at 0x3800.
$(BUILD)/%.bin: $(BUILD)/%.ihx
	$(MAKEBIN) -s 13312 $< $@
	@test "$$(wc -c < "$@")" -eq 13312
clean:
	rm -rf -- "$(BUILD)"
