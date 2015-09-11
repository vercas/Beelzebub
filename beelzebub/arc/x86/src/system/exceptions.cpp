#include <system/exceptions.hpp>
#include <system/cpu.hpp>
#include <memory\manager_amd64.hpp>

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
void Beelzebub::System::MiscellaneousInterruptHandler(IsrState * const state)
{
    assert(false
        , "<<MISC INT @ %Xp (%Xs) - vector %u1>>", INSTRUCTION_POINTER, state->ErrorCode, state->Vector);
}

/**
 *  Interrupt handler for division by 0.
 */
void Beelzebub::System::DivideErrorHandler(IsrState * const state)
{
    assert(false
        , "<<DIVIDE ERROR @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for checked arithmetic overflows.
 */
void Beelzebub::System::OverflowHandler(IsrState * const state)
{
    assert(false
        , "<<OVERFLOW @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for checked exceeded array bounds.
 */
void Beelzebub::System::BoundRangeExceededHandler(IsrState * const state)
{
    assert(false
        , "<<BOUNDS EXCEEDED @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for invalid opcode exceptions.
 */
void Beelzebub::System::InvalidOpcodeHandler(IsrState * const state)
{
    assert(false
        , "<<INVALID OPCODE @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for double faults.
 */
void Beelzebub::System::DoubleFaultHandler(IsrState * const state)
{
    assert(false
        , "<<DOUBLE FAULT @ %Xp (%Xs)>>", INSTRUCTION_POINTER, state->ErrorCode);
    asm volatile ("cli \n\t");
    while (true) { asm volatile ("hlt \n\t"); }
}

/**
 *  Interrupt handler for invalid TSS exceptions.
 */
void Beelzebub::System::InvalidTssHandler(IsrState * const state)
{
    assert(false
        , "<<INVALID TSS @ %Xp (%Xs)>>", INSTRUCTION_POINTER, state->ErrorCode);
}

/**
 *  Interrupt handler for invalid segment descriptors.
 */
void Beelzebub::System::SegmentNotPresentHandler(IsrState * const state)
{
    uint16_t ES = 0xFFFF, FS = 0xFFFF, GS = 0xFFFF;
    //  Used for retrieving the registers.

    asm volatile ( "mov %%es, %0 \n\t"
                   "mov %%fs, %1 \n\t"
                   "mov %%gs, %2 \n\t"
                 : "=r"(ES), "=r"(FS), "=r"(GS));

    assert(false
        , "<<SEGMENT NOT PRESENT @ %Xp (%Xs): CS%X2 DS%X2 SS%X2 ES%X2 FS%X2 GS%X2>>"
        , INSTRUCTION_POINTER, state->ErrorCode
        , (uint16_t)state->CS, (uint16_t)state->DS, (uint16_t)state->SS, ES, FS, GS);
}

/**
 *  Interrupt handler for invalid stack segmrnt exception.
 */
void Beelzebub::System::StackSegmentFaultHandler(IsrState * const state)
{
    assert(false
        , "<<STACK SEGMENT FAULT @ %Xp (%Xs): SS%X2>>"
        , INSTRUCTION_POINTER, state->ErrorCode, (uint16_t)state->SS);
}

/**
 *  Interrupt handler for general protection exceptions.
 */
void Beelzebub::System::GeneralProtectionHandler(IsrState * const state)
{
    assert(false
        , "<<GENERAL PROTECTION FAULT @ %Xp (%Xs)>>"
        , INSTRUCTION_POINTER, state->ErrorCode);
}

/**
 *  Interrupt handler for page faults.
 */
void Beelzebub::System::PageFaultHandler(IsrState * const state)
{
    vaddr_t CR2 = (vaddr_t)Cpu::GetCr2();

    const bool present = 0 != (state->ErrorCode & 1);
    const bool write = 0 != (state->ErrorCode & 2);
    const bool user = 0 != (state->ErrorCode & 4);
    const bool reserved = 0 != (state->ErrorCode & 8);
    const bool instruction = 0 != (state->ErrorCode & 16);

    char status[6] = "     ";

    if (present)     status[0] = 'P';
    if (write)       status[1] = 'W'; else status[1] = 'r';
    if (user)        status[2] = 'U'; else status[2] = 's';
    if (reserved)    status[3] = '0';
    if (instruction) status[4] = 'I'; else status[4] = 'd';

    Pml1Entry * e = nullptr;

    Handle res = ((MemoryManagerAmd64 *)BootstrapMemoryManager)->Vas->GetEntry(CR2 & ~((vaddr_t)0xFFF), e, true);

    if (e != nullptr)
    {
        msg("<<PAGE FAULT @ %Xp (%s|%Xs); CR2: %Xp | "
            , INSTRUCTION_POINTER, status, state->ErrorCode, CR2);

        e->PrintToTerminal(Beelzebub::Debug::DebugTerminal);

        msg(" >>");
    }
    else
        msg("<<PAGE FAULT @ %Xp (%s|%Xs); CR2: %Xp | %H >>"
            , INSTRUCTION_POINTER, status, state->ErrorCode, CR2, res);

    assert(false, "<< ^ EXCEPTION ^ >>");
}
