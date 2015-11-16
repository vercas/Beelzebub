#include <system/exceptions.hpp>
#include <memory/manager_amd64.hpp>
#include <system/cpu.hpp>
#include <kernel.hpp>
#include <entry.h>
#include <system/serial_ports.hpp>

#include <_print/paging.hpp>
#include <_print/isr.hpp>

#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Memory;

#if   defined(__BEELZEBUB__ARCH_AMD64)
#define INSTRUCTION_POINTER (state->RIP)
#elif defined(__BEELZEBUB__ARCH_IA32)
#define INSTRUCTION_POINTER (state->EIP)
#endif

/**
 *  Interrupt handler for miscellaneous interrupts that do not represent an exception.
 */
void Beelzebub::System::MiscellaneousInterruptHandler(INTERRUPT_HANDLER_ARGS)
{
    ASSERT(false
        , "<<MISC INT @ %Xp (%Xs) - vector %u1/%X1>>"
        , INSTRUCTION_POINTER, state->ErrorCode
        , vector, vector);
}

/**
 *  Interrupt handler for division by 0.
 */
void Beelzebub::System::DivideErrorHandler(INTERRUPT_HANDLER_ARGS)
{
    ASSERT(false
        , "<<DIVIDE ERROR @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for checked arithmetic overflows.
 */
void Beelzebub::System::OverflowHandler(INTERRUPT_HANDLER_ARGS)
{
    ASSERT(false
        , "<<OVERFLOW @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for checked exceeded array bounds.
 */
void Beelzebub::System::BoundRangeExceededHandler(INTERRUPT_HANDLER_ARGS)
{
    ASSERT(false
        , "<<BOUNDS EXCEEDED @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for invalid opcode exceptions.
 */
void Beelzebub::System::InvalidOpcodeHandler(INTERRUPT_HANDLER_ARGS)
{
    ASSERT(false
        , "<<INVALID OPCODE @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for double faults.
 */
void Beelzebub::System::DoubleFaultHandler(INTERRUPT_HANDLER_ARGS)
{
    ASSERT(false
        , "<<DOUBLE FAULT @ %Xp (%Xs)>>", INSTRUCTION_POINTER, state->ErrorCode);
    asm volatile ("cli \n\t");
    while (true) { asm volatile ("hlt \n\t"); }
}

/**
 *  Interrupt handler for invalid TSS exceptions.
 */
void Beelzebub::System::InvalidTssHandler(INTERRUPT_HANDLER_ARGS)
{
    ASSERT(false
        , "<<INVALID TSS @ %Xp (%Xs)>>", INSTRUCTION_POINTER, state->ErrorCode);
}

/**
 *  Interrupt handler for invalid segment descriptors.
 */
void Beelzebub::System::SegmentNotPresentHandler(INTERRUPT_HANDLER_ARGS)
{
    uint16_t ES = 0xFFFF, FS = 0xFFFF, GS = 0xFFFF;
    //  Used for retrieving the registers.

    asm volatile ( "mov %%es, %0 \n\t"
                   "mov %%fs, %1 \n\t"
                   "mov %%gs, %2 \n\t"
                 : "=r"(ES), "=r"(FS), "=r"(GS));

    ASSERT(false
        , "<<SEGMENT NOT PRESENT @ %Xp (%Xs): CS%X2 DS%X2 SS%X2 ES%X2 FS%X2 GS%X2>>"
        , INSTRUCTION_POINTER, state->ErrorCode
        , (uint16_t)state->CS, (uint16_t)state->DS, (uint16_t)state->SS, ES, FS, GS);
}

/**
 *  Interrupt handler for invalid stack segmrnt exception.
 */
void Beelzebub::System::StackSegmentFaultHandler(INTERRUPT_HANDLER_ARGS)
{
    ASSERT(false
        , "<<STACK SEGMENT FAULT @ %Xp (%Xs): SS%X2>>"
        , INSTRUCTION_POINTER, state->ErrorCode, (uint16_t)state->SS);
}

/**
 *  Interrupt handler for general protection exceptions.
 */
void Beelzebub::System::GeneralProtectionHandler(INTERRUPT_HANDLER_ARGS)
{
    MSG("<< GP FAULT! >>%n");

    PrintToDebugTerminal(state);

    uintptr_t stackPtr = state->RSP;
    uintptr_t const stackEnd = RoundUp(stackPtr, PageSize);

    if ((stackPtr & (sizeof(size_t) - 1)) != 0)
    {
        msg("Stack pointer was not a multiple of %us! (%Xp)%n"
            , sizeof(size_t), stackPtr);

        stackPtr &= ~((uintptr_t)(sizeof(size_t) - 1));
    }

    bool odd;
    for (odd = true; stackPtr < stackEnd; stackPtr += sizeof(size_t), odd = !odd)
    {
        msg("%X2|%Xs|%s"
            , (uint16_t)(stackPtr - state->RSP)
            , *((size_t const *)stackPtr)
            , odd ? "\t" : "\r\n");
    }

    if (odd) msg("%n");

    ASSERT(false
        , "%n<<GENERAL PROTECTION FAULT @ %Xp (%Xs)>>"
        , INSTRUCTION_POINTER, state->ErrorCode);
}

/**
 *  Interrupt handler for page faults.
 */
void Beelzebub::System::PageFaultHandler(INTERRUPT_HANDLER_ARGS)
{
    vaddr_t CR2 = (vaddr_t)Cpu::GetCr2();

    const bool present = 0 != (state->ErrorCode & 1);
    const bool write = 0 != (state->ErrorCode & 2);
    const bool user = 0 != (state->ErrorCode & 4);
    const bool reserved = 0 != (state->ErrorCode & 8);
    const bool instruction = 0 != (state->ErrorCode & 16);

#if   defined(__BEELZEBUB__ARCH_AMD64)
    const uint16_t ind1 = VirtualAllocationSpace::GetPml1Index(CR2)
                 , ind2 = VirtualAllocationSpace::GetPml2Index(CR2)
                 , ind3 = VirtualAllocationSpace::GetPml3Index(CR2)
                 , ind4 = VirtualAllocationSpace::GetPml4Index(CR2);
#elif defined(__BEELZEBUB__ARCH_IA32PAE)
    const uint16_t ind1 = VirtualAllocationSpace::GetPml1Index(CR2)
                 , ind2 = VirtualAllocationSpace::GetPml2Index(CR2)
                 , ind3 = VirtualAllocationSpace::GetPml3Index(CR2);
#elif defined(__BEELZEBUB__ARCH_IA32)
    const uint16_t ind1 = VirtualAllocationSpace::GetPml1Index(CR2)
                 , ind2 = VirtualAllocationSpace::GetPml2Index(CR2);
#endif

    char status[6] = "     ";

    if (present)     status[0] = 'P';
    if (write)       status[1] = 'W'; else status[1] = 'r';
    if (user)        status[2] = 'U'; else status[2] = 's';
    if (reserved)    status[3] = '0';
    if (instruction) status[4] = 'I'; else status[4] = 'd';

    MSG("%n<<PAGE FAULT @ %Xp (%s|%X1); CR2: %Xp | "
        , INSTRUCTION_POINTER, status, (uint8_t)state->ErrorCode, CR2);

#if   defined(__BEELZEBUB__ARCH_AMD64)
    MSG("%u2:%u2:%u2:%u2 | ", ind4, ind3, ind2, ind1);
#elif defined(__BEELZEBUB__ARCH_IA32PAE)
    MSG("%u2:%u2:%u2 | ", ind3, ind2, ind1);
#elif defined(__BEELZEBUB__ARCH_IA32)
    MSG("%u2:%u2 | ", ind2, ind1);
#endif

#if   defined(__BEELZEBUB__ARCH_AMD64)
    if (CR2 >= VirtualAllocationSpace::FractalStart && CR2 <= VirtualAllocationSpace::FractalEnd)
    {
        vaddr_t vaddr = (CR2 - VirtualAllocationSpace::LocalPml1Base) << 9;

        if (0 != (vaddr & 0x0000800000000000ULL))
            vaddr |= 0xFFFF000000000000ULL;

        uint16_t vind1 = VirtualAllocationSpace::GetPml1Index(CR2)
               , vind2 = VirtualAllocationSpace::GetPml2Index(CR2)
               , vind3 = VirtualAllocationSpace::GetPml3Index(CR2)
               , vind4 = VirtualAllocationSpace::GetPml4Index(CR2);

        MSG("Adr: %Xp - %u2:%u2:%u2:%u2 | ", vaddr, vind4, vind3, vind2, vind1);
    }
#endif

    Pml1Entry * e = nullptr;

    Handle res = BootstrapMemoryManager.Vas->GetEntry(RoundDown(CR2, PageSize), e, true);

    if (e != nullptr)
    {
        PrintToTerminal(Beelzebub::Debug::DebugTerminal, *e);

        MSG(" >>");
    }
    else
        MSG("%H >>", res);

    ASSERT(false, "<< ^ EXCEPTION ^ >>");
}
