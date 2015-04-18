#pragma once

#include <stdint.h>

/**
 * The #defines `b` and `b_` are taken from the following file:
 * https://github.com/klange/toaruos/blob/strawberry-dev/userspace/gui/terminal/terminal-font.h
 * They ain't much but credit is still due.
 **/

#define b(x) ((uint8_t)b_(0 ## x ## uL))
#define b_(x) ((x & 1) | (x >> 2 & 2) | (x >> 4 & 4) | (x >> 6 & 8) | (x >> 8 & 16) | (x >> 10 & 32) | (x >> 12 & 64) | (x >> 14 & 128))

#define FONT_WIDTH   8
#define FONT_HEIGHT 14

#define FONT_MIN    33
#define FONT_MAX   126

extern const __attribute__((section(".graphics"))) uint8_t font[][FONT_HEIGHT];
