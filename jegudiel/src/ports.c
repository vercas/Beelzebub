#include <ports.h>
#include <stdint.h>

// void outb(const uint16_t port, uint8_t value)
// {
//     asm volatile ("outb %1, %0" :: "dN" (port), "a" (value));
// }

// uint8_t inb(const uint16_t port)
// {
//     uint8_t value;
//     asm volatile ("inb %1, %0" : "=a" (value) : "dN" (port));
//     return value;
// }

void init_serial(const uint16_t port)
{
	outb(port + 1, 0x00);    // Disable all interrupts
	outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outb(port + 1, 0x00);    //                  (hi byte)
	outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int serial_received(const uint16_t port)
{
   return inb(port + 5) & 1;
}
 
char read_serial(const uint16_t port)
{
   while (!serial_received(port));
 
   return inb(port);
}

int is_transmit_empty(const uint16_t port)
{
	return inb(port + 5) & 0x20;
}

void write_serial(const uint16_t port, const char a)
{
	while (!is_transmit_empty(port));

	outb(port, a);
}

void write_serial_str(const uint16_t port, const char * const a)
{
	size_t i = 0;

	while (a[i] != 0)
		write_serial(port, a[i++]);
}

void write_serial_ud(const uint16_t port, const uint64_t n)
{
	char str[21];
	str[20] = 0;

	size_t i;
	uint64_t x = n;

	for (i = 0; x > 0 && i < 20; ++i)
	{
		uint8_t digit = x % 10;

		str[19 - i] = '0' + digit;

		x /= 10;
	}

	//write_serial_str(port, "I WAS SUPPOSED TO WRITE A DECIMAL INTEGER!");

	write_serial_str(port, str + 20 - i);
}

void write_serial_uh(const uint16_t port, const uint64_t x, const size_t d)
{
	int32_t i;

	for (i = (int32_t)d - 1; i >= 0; --i)
	{
		uint8_t nib = (x >> (i * 4)) & 0x0F;

		write_serial(port, (nib > 9 ? '7' : '0') + nib);
		//	'7' + 10 = 'A' in ASCII, dawg.
	}
}
