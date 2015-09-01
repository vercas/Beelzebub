#pragma once

#include <stdint.h>

#define COM1	((uint16_t)0x3F8)

/**
 * Sends a byte to an output <port>.
 *
 * @param port port to send the byte to
 * @param value the byte to send
 */
inline __attribute__((always_inline)) void outb(const uint16_t port, uint8_t value)
{
	asm volatile ("outb %1, %0" :: "dN" (port), "a" (value));
}

/**
 * Reads a byte from an input <port>.
 *
 * @param port the port to read from
 * @return the byte read from the port
 */
inline __attribute__((always_inline)) uint8_t inb(const uint16_t port)
{
	uint8_t value;
    asm volatile ("inb %1, %0" : "=a" (value) : "dN" (port));
    return value;
}

void init_serial(const uint16_t port);

char read_serial(const uint16_t port);
void write_serial(const uint16_t port, const char a);

void write_serial_str(const uint16_t port, const char * const a);

void write_serial_ud(const uint16_t port, const uint64_t x);
void write_serial_uh(const uint16_t port, const uint64_t x, const size_t d);
