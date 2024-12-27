# SPDX-FileCopyrightText: © 2022 Uri Shaked <uri@wokwi.com>
# SPDX-License-Identifier: MIT

SOURCES = i2c-8bit.chip.c
TARGET  = i2c-8bit.chip.wasm

.PHONY: all
all: $(TARGET) chip.json

$(TARGET): $(SOURCES) wokwi-api.h
	  clang --target=wasm32-unknown-wasi --sysroot ~/wasi-libc/sysroot -nostartfiles -Wl,--import-memory -Wl,--export-table -Wl,--no-entry -Werror -o $(TARGET) $(SOURCES)


.PHONY: test
test:
	  cd test && arduino-cli compile -e -b arduino:avr:uno blink