#include <arc/entry.h>

#include <jegudiel.h>
#include <arc/isr.h>
#include <arc/keyboard.h>
#include <arc/screen.h>
#include <ui.h>

#include <arc/lapic.h>
#include <arc/screen.h>

#include <memory.h>

#include <kernel.hpp>

static __bland void fault_gp(isr_state_t * state)
{
    // ..? :(
}

void kmain_bsp()
{
    for (size_t i = 0; i < 256; ++i)
    {
        isr_handlers[i] = (uintptr_t)&fault_gp;
    }

    //isr_handlers[KEYBOARD_IRQ_VECTOR] = (uintptr_t)&keyboard_handler;

    InitializeMemory(JG_INFO_MMAP, JG_INFO_ROOT->mmap_count, JG_INFO_ROOT->free_paddr);
    
    asm volatile ("sti");

    Beelzebub::Main();
}

void kmain_ap()
{
    asm volatile ("sti");

    Beelzebub::Secondary();
}
