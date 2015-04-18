/**
 * This code was produced by Vercas's "Image to Bit Stream" utility.
 * 
 * The #defines `b` and `b_` are taken from the following file:
 * https://github.com/klange/toaruos/blob/strawberry-dev/userspace/gui/terminal/terminal-font.h
 * They ain't much but credit is still due.
 *
 * The image reproduced in this code is still property of its respective owner.
 * Use the code as you would use the image. The author of the tool takes
 * no responsibility for the actions of the users.
 */

#pragma once

#include <stdint.h>

#define SPLASH_WIDTH  (168)
#define SPLASH_HEIGHT (480)

#define b(x) ((uint8_t)b_(0 ## x ## uL))
#define b_(x) ((x & 1) | (x >> 2 & 2) | (x >> 4 & 4) | (x >> 6 & 8) | (x >> 8 & 16) | (x >> 10 & 32) | (x >> 12 & 64) | (x >> 14 & 128))

extern const __attribute__((section(".graphics"))) uint8_t splash[];
