#include <math.h>
#include <memory/paging.hpp>
#include <terminals/vga.hpp>
#include <screen.h>
#include <jegudiel.h>

extern "C" void __cxa_pure_virtual()
{
    //	NUTHIN
    // I should panic here.
}

extern "C" void doCppStuff()
{
	u64 * mem = (u64 *)RoundUp(JG_INFO_ROOT->free_paddr, Beelzebub::Memory::Paging::PageSize);

	for (size_t i = 0; i < 120; ++i)
    {
        short row = i / 5, col = i % 5;
        short x = col * 16;

        screen_write_hex(mem[i], x, row);
	}
}
