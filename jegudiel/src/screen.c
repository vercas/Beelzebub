/**
 * THE FOLLOWING LICENSE ONLY APPLIES TO THE FIRST PART OF THE CODE
 *
 * Copyright (c) 2012 by Lukas Heidemann <lukasheidemann@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <screen.h>
#include <stdint.h>
#include <multiboot.h>
#include <ports.h>
#include <font.h>
#include <splash.h>

uint32_t pitch;
uint8_t type, bypp;
uint16_t SCREEN_WIDTH;
uint16_t SCREEN_HEIGHT;
uintptr_t SCREEN_VIDEOMEM_PADDR;
uint32_t frameWidth, frameHeight;
uint16_t curX, curY;

#define vmem ((uint8_t *)SCREEN_VIDEOMEM_PADDR)
#define pixel(x, y) *((uint32_t *)(vmem + (x) * bypp + (y) * pitch))

void screen_write(const char *message, uint16_t x, uint16_t y)
{
	size_t i;
	size_t position = x + y * (pitch / bypp);

	for (i = 0; '\0' != message[i]; ++i)
	{
		if (y == 24)
			SCREEN_VIDEOMEM[position].attributes = SCREEN_ATTRIBUTES_PANIC;

		SCREEN_VIDEOMEM[position++].character = message[i];

#ifdef __JEGUDIEL__SERIAL
		write_serial(COM1, message[i]);
#endif
	}
}

void screen_write_hex(uint64_t number, uint16_t x, uint16_t y)
{
	char string[17];
	string[16] = '\0';

	size_t i;
	for (i = 0; i < 16; ++i)
	{
		uint8_t nibble = (number >> (i * 4)) & 0xF;

		if (nibble >= 10)
			string[15 - i] = 'A' + (nibble - 10);
		else
			string[15 - i] = '0' + nibble;
	}

	screen_write(string, x, y);
}

/**
 * HERE THE FIRST PART END.
 *
 * The rest of the code is under the vLicense.
 */

#define NOCOL  (0x90011337)
#define INVCOL (0x42666616)
//	Immature? Maybe.

#define VBE_BACKGROUND (0xFF262223)
#define VBE_TEXT       (0xFFDFE0E6)
#define VBE_SPLASH     (0xFF121314)
//	A random style of my choosing

void draw_bitmap(const size_t x, const size_t y, const uint32_t colf, const uint32_t colb, const uint8_t * const bmp, const size_t w, const size_t h)
{
	size_t lx, ly, bit;
	size_t line = SCREEN_VIDEOMEM_PADDR + y * pitch + x * bypp;
	size_t byteWidth = w / 8;

	for (ly = 0; ly < h; ++ly)
	{
		size_t ind = ly * w / 8, col = 0;

		for (lx = 0; lx < byteWidth; ++lx)
			for (bit = 0; bit < 8; ++bit)
			{
				if (bmp[ind + lx] & (0x80 >> bit))
				{
					if (colf == INVCOL)
						*((uint32_t *)(col + line)) = ~*((uint32_t *)(col + line));
					else if (colf != NOCOL)
						*((uint32_t *)(col + line)) = colf;
				}
				else if (colb != NOCOL)
					*((uint32_t *)(col + line)) = colb;

				col += bypp;
			}

		line += pitch;
	}
}

/**
 * Initializes the screen.
 **/
void screen_init(void)
{
	curX = curY = 0;

	switch (multiboot_info->framebuffer_type)
	{
		case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED:
			//	wat?
			break;

		case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
			type = 0;
			SCREEN_VIDEOMEM_PADDR = multiboot_info->framebuffer_addr;
			SCREEN_WIDTH = multiboot_info->framebuffer_width;
			SCREEN_HEIGHT = multiboot_info->framebuffer_height;
			pitch = multiboot_info->framebuffer_pitch;
			bypp = (multiboot_info->framebuffer_bpp + 7) / 8;
			//	Rounds up the bpp to bytes per pixel.
			break;

		case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
			type = 1;
			SCREEN_VIDEOMEM_PADDR = multiboot_info->framebuffer_addr;
			frameWidth = multiboot_info->framebuffer_width;
			frameHeight = multiboot_info->framebuffer_height;
			pitch = multiboot_info->framebuffer_pitch;
			bypp = (multiboot_info->framebuffer_bpp + 7) / 8;

			SCREEN_WIDTH = (frameWidth - 2) / FONT_WIDTH;
			SCREEN_HEIGHT = (frameHeight - 2) / FONT_HEIGHT;
			break;
	}

	screen_clear();
}

/**
 * Clears the screen.
 */
void screen_clear(void)
{
	if (type == 0)
	{
		uint16_t x, y;
		for (y = 0; y < SCREEN_HEIGHT; ++y)
		for (x = 0; x < SCREEN_WIDTH; ++x)
		{
			vmem[x * bypp + y * pitch    ] = ' ';
			vmem[x * bypp + y * pitch + 1] = SCREEN_ATTRIBUTES;
		}
	}
	else if (type == 1)
	{
		uint16_t x, y;
		for (y = 0; y < frameHeight; ++y)
		for (x = 0; x < frameWidth; ++x)
		{
			*((uint32_t *)(vmem + x * bypp + y * pitch)) = VBE_BACKGROUND;
		}

		draw_bitmap(frameWidth  - SPLASH_WIDTH,
			        frameHeight - SPLASH_HEIGHT,
			        VBE_SPLASH, NOCOL,
			        splash,
			        SPLASH_WIDTH, SPLASH_HEIGHT);
	}

#ifdef __JEGUDIEL__SERIAL
	write_serial_str(COM1, "--- screen clear ---\r\n");
#endif
}

size_t write_vga(const char * const s)
{
	size_t i, pos, x = curX, y = curY;

	//	SPECIAL STRING
	if (s[0] == '-' && s[1] == '-' && s[2] == '-' && s[3] == '-' && s[4] == 0)
	{
		size_t ret = SCREEN_WIDTH - x;

#ifdef __JEGUDIEL__SERIAL
		for (; x < SCREEN_WIDTH; x++)
			write_serial(COM1, vmem[x * bypp + y * pitch] = '-');

		curX = 0; ++curY;

		write_serial_str(COM1, "\r\n");
#endif

		return ret;
	}

	for (i = 0; '\0' != s[i]; ++i)
	{
		char c = s[i];
		pos = x * bypp + y * pitch;

		if (c == '\r')
			x = 0;
		else if (c == '\n')
			++y;
		else if (c == '\t')
			x = (x / 8 + 1) * 8;
		else if (c == '\b')
		{
			if (x > 0)
				--x;
		}
		else
		{
			if (x == SCREEN_WIDTH)
			{
				x = 0; y++;
			}

			if (y == 24)
				vmem[pos + 1] = SCREEN_ATTRIBUTES_PANIC;
			else
				vmem[pos + 1] = SCREEN_ATTRIBUTES;

			vmem[pos] = c;

			x++;
		}

#ifdef __JEGUDIEL__SERIAL
		write_serial(COM1, c);
#endif
	}

	curX = x; curY = y;

	return i;
}

size_t write_vbe(const char * const s)
{
	size_t i, x = curX, y = curY;

	if (s[0] == '-' && s[1] == '-' && s[2] == '-' && s[3] == '-' && s[4] == 0)
	{
		size_t ret = SCREEN_WIDTH - x, w = 1 + x * FONT_WIDTH;
		size_t y2 = 1 + y * FONT_HEIGHT + FONT_HEIGHT / 2;

		for (; w < frameWidth - 1; ++w)
			pixel(w, y2) = VBE_TEXT;

#ifdef __JEGUDIEL__SERIAL
		for (; x < SCREEN_WIDTH; x++)
			write_serial(COM1, '-');

		write_serial_str(COM1, "\r\n");
#endif

		curX = 0; ++curY;

		return ret;
	}

	for (i = 0; '\0' != s[i]; ++i)
	{
		char c = s[i];

		if (c == '\r')
			x = 0;
		else if (c == '\n')
			++y;
		else if (c == '\t')
			x = (x / 8 + 1) * 8;
		else if (c == '\b')
		{
			if (x > 0)
				--x;
		}
		else if (c == ' ')
			++x;
		else if (c >= FONT_MIN && c <= FONT_MAX)
		{
			if (x == SCREEN_WIDTH)
			{     x = 0; y++;    }

			draw_bitmap(1 + x * FONT_WIDTH,
				        1 + y * FONT_HEIGHT,
				        VBE_TEXT, NOCOL,
				        font[c - FONT_MIN],
				        FONT_WIDTH, FONT_HEIGHT);

			++x;
		}

#ifdef __JEGUDIEL__SERIAL
		write_serial(COM1, c);
#endif
	}

	curX = x; curY = y;

	return i;
}

size_t puts(const char * const s)
{
	if (type == 0)
		return write_vga(s);
	else if (type == 1)
		return write_vbe(s);

	return ~((size_t)0);
}

size_t puthexs(const uint64_t number)
{
	char string[17];
	string[16] = '\0';

	size_t i;
	for (i = 0; i < 16; ++i)
	{
		uint8_t nibble = (number >> (i * 4)) & 0xF;

		if (nibble >= 10)
			string[15 - i] = 'A' + (nibble - 10);
		else
			string[15 - i] = '0' + nibble;
	}

	return puts(string);
}
