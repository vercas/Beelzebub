#pragma once

#include <metaprogramming.h>

/**
 * The #defines `b` and `b_` are taken from the following file:
 * https://github.com/klange/toaruos/blob/strawberry-dev/userspace/gui/terminal/terminal-font.h
 * They ain't much but credit is still due.
 **/

#define b(x) ((uint8_t)b_(0 ## x ## uL))
#define b_(x) ((x & 1) | (x >> 2 & 2) | (x >> 4 & 4) | (x >> 6 & 8) | (x >> 8 & 16) | (x >> 10 & 32) | (x >> 12 & 64) | (x >> 14 & 128))

namespace Beelzebub { namespace Terminals
{
	extern const uint8_t FontWidth = 8;
	extern const uint8_t FontHeight = 14;

	extern const uint8_t FontMin = 33;
	extern const uint8_t FontMax = 126;

	extern const uint8_t FontUnderlineHeight = FontHeight - 2;

	extern const uint8_t Font[][FontHeight];
}}
