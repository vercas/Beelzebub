#include <system/exceptions.hpp>
#include <system/cpu.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;

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
    assert(INSTRUCTION_POINTER < KERNEL_CODE_SEPARATOR
        , "<<MISC INT @ %Xp: v%u1, ec%X8>>", INSTRUCTION_POINTER, state->Vector, state->ErrorCode);
}

/**
 *  Interrupt handler for division by 0.
 */
void Beelzebub::System::DivideErrorHandler(IsrState * const state)
{
    assert(INSTRUCTION_POINTER < KERNEL_CODE_SEPARATOR
        , "<<DIVIDE ERROR @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for checked arithmetic overflows.
 */
void Beelzebub::System::OverflowHandler(IsrState * const state)
{
    assert(INSTRUCTION_POINTER < KERNEL_CODE_SEPARATOR
        , "<<OVERFLOW @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for checked exceeded array bounds.
 */
void Beelzebub::System::BoundRangeExceededHandler(IsrState * const state)
{
    assert(INSTRUCTION_POINTER < KERNEL_CODE_SEPARATOR
        , "<<BOUNDS EXCEEDED @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for invalid opcode exceptions.
 */
void Beelzebub::System::InvalidOpcodeHandler(IsrState * const state)
{
    assert(INSTRUCTION_POINTER < KERNEL_CODE_SEPARATOR
        , "<<INVALID OPCODE @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for double faults.
 */
void Beelzebub::System::DoubleFaultHandler(IsrState * const state)
{
    assert(INSTRUCTION_POINTER < KERNEL_CODE_SEPARATOR
        , "<<DOUBLE FAULT @ %Xp (%Xs)>>", INSTRUCTION_POINTER, state->ErrorCode);
}

/**
 *  Interrupt handler for invalid TSS exceptions.
 */
void Beelzebub::System::InvalidTssHandler(IsrState * const state)
{
    assert(INSTRUCTION_POINTER < KERNEL_CODE_SEPARATOR
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

    assert(INSTRUCTION_POINTER < KERNEL_CODE_SEPARATOR
        , "<<SEGMENT NOT PRESENT @ %Xp (%Xs): CS%X2 DS%X2 SS%X2 ES%X2 FS%X2 GS%X2>>"
        , INSTRUCTION_POINTER, state->ErrorCode
        , (uint16_t)state->CS, (uint16_t)state->DS, (uint16_t)state->SS, ES, FS, GS);
}

/**
 *  Interrupt handler for invalid stack segmrnt exception.
 */
void Beelzebub::System::StackSegmentFaultHandler(IsrState * const state)
{
    assert(INSTRUCTION_POINTER < KERNEL_CODE_SEPARATOR
        , "<<STACK SEGMENT FAULT @ %Xp (%Xs): SS%X2>>"
        , INSTRUCTION_POINTER, state->ErrorCode, (uint16_t)state->SS);
}

/**
 *  Interrupt handler for general protection exceptions.
 */
void Beelzebub::System::GeneralProtectionHandler(IsrState * const state)
{
    assert(INSTRUCTION_POINTER < KERNEL_CODE_SEPARATOR
        , "<<GENERAL PROTECTION FAULT @ %Xp (%Xs)>>"
        , INSTRUCTION_POINTER, state->ErrorCode);
}

/**
 *  Interrupt handler for page faults.
 */
void Beelzebub::System::PageFaultHandler(IsrState * const state)
{
    void * const CR2 = Cpu::GetCr2();

    assert(INSTRUCTION_POINTER < KERNEL_CODE_SEPARATOR
        , "<<PAGE FAULT @ %Xp (%Xs); CR2: %Xp>>"
        , INSTRUCTION_POINTER, state->ErrorCode, CR2);
}
