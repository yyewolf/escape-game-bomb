// Wokwi Custom Chip - For information and examples see:
// https://link.wokwi.com/custom-chips-alpha
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2022 Uri Shaked / wokwi.com

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

// This is to scan the keypad easily
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

// Common address for the real device.
const int ADDRESS = 0x20;

typedef struct {
  pin_t row_pins[4];
  pin_t col_pins[4];
  uint8_t bitMap[10];

  uint8_t R;
  uint8_t C;
  uint8_t mask;
} chip_state_t;

static bool on_i2c_connect(void *chip, uint32_t address, bool connect);
static uint8_t on_i2c_read(void *chip);
static bool on_i2c_write(void *chip, uint8_t data);
static void on_i2c_disconnect(void *chip);

void chip_init() {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  chip->row_pins[0] = pin_init("R1", INPUT);
  chip->row_pins[1] = pin_init("R2", INPUT);
  chip->row_pins[2] = pin_init("R3", INPUT);
  chip->row_pins[3] = pin_init("R4", INPUT);
  chip->col_pins[0] = pin_init("C1", INPUT);
  chip->col_pins[1] = pin_init("C2", INPUT);
  chip->col_pins[2] = pin_init("C3", INPUT);
  chip->col_pins[3] = pin_init("C4", INPUT);

  setvbuf(stdout, NULL, _IOLBF, 1024);

  const i2c_config_t i2c_config = {
    .address = ADDRESS,
    .scl = pin_init("SCL", INPUT),
    .sda = pin_init("SDA", INPUT),
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
    .disconnect = on_i2c_disconnect,
    .user_data = chip,
  };
  i2c_init(&i2c_config);

  chip->mask = 0;
}

void update(chip_state_t *chip) {
  for (int r=0; r < 4; r++) {
		pin_mode(chip->row_pins[r],INPUT_PULLUP);
	}

	for (int c=0; c<4; c++) {
		for (int r=0; r < 4; r++) {
      bitClear(chip->bitMap[r], c);
    }
  }
	for (int c=0; c<4; c++) {
		pin_mode(chip->col_pins[c],OUTPUT);
		pin_write(chip->col_pins[c], LOW);
		for (int r=0; r < 4; r++) {
      bitWrite(chip->bitMap[r], c, !pin_read(chip->row_pins[r]));
		}
		pin_write(chip->col_pins[c],HIGH);
		pin_mode(chip->col_pins[c],INPUT);
	}

  int R[4] = {0,0,0,0};
  int C[4] = {0,0,0,0};
  
  for (int r=0; r<4; r++) {
		for (int c=0; c<4; c++) {
			int button = bitRead(chip->bitMap[r],c);
      if (button) {
        R[r] = 1;
        C[c] = 1;
      }
		}
	}

  chip->R = (R[0]*14 + R[1]*13 + R[2]*11 + R[3]*7);
  chip->C = (C[0]*14 + C[1]*13 + C[2]*11 + C[3]*7)<<4;
}

bool on_i2c_connect(void *user_data, uint32_t address, bool connect) {
  return true; /* Ack */
}

uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = user_data;
  update(chip);
  if (chip->R == 0 || chip->C == 0) {
    return 0xFF;
  }
  if (chip->mask == 0xF0) {
    return chip->C;
  }
  return chip->R;
}

bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = user_data;
  chip->mask = data;
  return true; // Ack
}

void on_i2c_disconnect(void *user_data) {
  // Do nothing
}
