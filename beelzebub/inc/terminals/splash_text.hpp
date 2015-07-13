/**
 *  This code was produced by Vercas's "Image to Bit Stream" utility.
 * 
 *  The #defines `b` and `b_` are taken from the following file:
 *  https://github.com/klange/toaruos/blob/strawberry-dev/userspace/gui/terminal/terminal-font.h
 *  They ain't much but credit is still due.
 */

#pragma once

#include <stdint.h>

#define SPLASHTEXT_WIDTH  (640)
#define SPLASHTEXT_HEIGHT (150)

#define b(x) ((uint8_t)b_(0 ## x ## uL))
#define b_(x) ((x & 1) | (x >> 2 & 2) | (x >> 4 & 4) | (x >> 6 & 8) | (x >> 8 & 16) | (x >> 10 & 32) | (x >> 12 & 64) | (x >> 14 & 128))

namespace Beelzebub { namespace Terminals
{
    extern const uint16_t SplashTextWidth = 168;
    extern const uint16_t SplashTextHeight = 480;

    extern const uint8_t SplashText[];
}}
