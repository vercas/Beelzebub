#pragma once

#include <arc/memory/paging.hpp>
#include <arc/system/registers.hpp>
#include <metaprogramming.h>

#define REGFUNC1(regl, regu, type)                                   \
static __bland __forceinline type MCATS2(Get, regu)()                \
{                                                                    \
	type ret;                                                        \
                                                                     \
	asm volatile ( "mov %%" #regl ", %0\n\t"                         \
	             : "=a"(ret) );                                      \
                                                                     \
	return ret;                                                      \
}                                                                    \
static __bland __forceinline void MCATS2(Set, regu)(const type val)  \
{                                                                    \
	asm volatile ( "mov %0, %%" #regl "\n\t"                         \
	             : : "r"(val) );                                     \
}

#define REGFUNC2(regl, regu, type, type2)                            \
static __bland __forceinline type2 MCATS2(Get, regu)()               \
{                                                                    \
	type ret;                                                        \
                                                                     \
	asm volatile ( "mov %%" #regl ", %0\n\t"                         \
	             : "=a"(ret) );                                      \
                                                                     \
	return type2(ret);                                               \
}                                                                    \
static __bland __forceinline void MCATS2(Set, regu)(const type val)  \
{                                                                    \
	asm volatile ( "mov %0, %%" #regl "\n\t"                         \
	             : : "r"(val) );                                     \
}                                                                    \
static __bland __forceinline void MCATS2(Set, regu)(const type2 val) \
{                                                                    \
	type innerVal = val.Value;                                       \
                                                                     \
	asm volatile ( "mov %0, %%" #regl "\n\t"                         \
	             : : "r"(innerVal) );                                \
}

namespace Beelzebub { namespace System
{
	class Cpu
	{
	public:

		/*  Control  */

		static const bool CanHalt = true;

		static __bland __forceinline void Halt()
		{
			asm volatile ("hlt");
		}

		/*  Registers  */

		REGFUNC2(cr0, Cr0, size_t, Beelzebub::System::Cr0)
		REGFUNC1(cr2, Cr2, void *)
		REGFUNC2(cr3, Cr3, size_t, Beelzebub::System::Cr3)
		REGFUNC1(cr4, Cr4, size_t)

		/*  Interrupts  */

		static __bland __forceinline bool InterruptsEnabled()
		{
			size_t flags;

			asm volatile ( "pushf\n\t"
			               "pop %0"
			               : "=r"(flags) );

			return (flags & (size_t)(1 << 9)) != 0;
		}

		static __bland __forceinline void EnableInterrupts()
		{
			asm volatile ("sti");
		}

		static __bland __forceinline void DisableInterrupts()
		{
			asm volatile ("cli");
		}

		static __bland __forceinline void LIDT(const uintptr_t base
		                                     , const uint16_t size)
		{
			struct
			{
				uint16_t length;
				uintptr_t base;
			} __attribute__((packed)) IDTR;

			IDTR.length = size;
			IDTR.base = base;

			asm ( "lidt (%0)" : : "p"(&IDTR) );
		}

		/*  Far memory ops  */

		static __bland __forceinline uint32_t FarGet32(const uint16_t sel
		                                             , const uintptr_t off)
		{
			uint32_t ret;
			asm ( "push %%fs\n\t"
			      "mov  %1, %%fs\n\t"
			      "mov  %%fs:(%2), %0\n\t"
			      "pop  %%fs"
			      : "=r"(ret) : "g"(sel), "r"(off) );
			return ret;
		}

		/*  Port I/O  */

		static __bland __forceinline void Out8(const uint16_t port
		                                     , const uint8_t value)
		{
			asm volatile ("outb %1, %0" :: "dN" (port), "a" (value));
		}

		static __bland __forceinline void Out16(const uint16_t port
		                                      , const uint16_t value)
		{
			asm volatile ("outw %1, %0" :: "dN" (port), "a" (value));
		}

		static __bland __forceinline void Out32(const uint16_t port
		                                      , const uint32_t value)
		{
			asm volatile ("outl %1, %0" :: "dN" (port), "a" (value));
		}

		static __bland __forceinline uint8_t In8(uint16_t port)
		{
			uint8_t value;
			asm volatile ("inb %1, %0" : "=a" (value) : "dN" (port));
			return value;
		}

		static __bland __forceinline uint16_t In16(uint16_t port)
		{
			uint16_t value;
			asm volatile ("inw %1, %0" : "=a" (value) : "dN" (port));
			return value;
		}

		static __bland __forceinline uint32_t In32(uint16_t port)
		{
			uint32_t value;
			asm volatile ("inl %1, %0" : "=a" (value) : "dN" (port));
			return value;
		}
	};
}}
