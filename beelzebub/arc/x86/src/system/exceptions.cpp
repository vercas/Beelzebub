/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#include "system/exceptions.hpp"
#include "memory/vmm.hpp"
#include "memory/vmm.arc.hpp"
#include "system/cpu.hpp"
#include "system/fpu.hpp"
#include "kernel.hpp"
#include "cores.hpp"
#include "entry.h"
#include "system/serial_ports.hpp"
#include "execution/extended_states.hpp"

#include "_print/paging.hpp"
#include "_print/isr.hpp"

#include "utils/stack_walk.hpp"

#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;

#pragma GCC diagnostic ignored "-Wunused-parameter"

#if   defined(__BEELZEBUB__ARCH_AMD64)
#define INSTRUCTION_POINTER (state->RIP)
#elif defined(__BEELZEBUB__ARCH_IA32)
#define INSTRUCTION_POINTER (state->EIP)
#endif

static Synchronization::SmpLock PrintLock {};

/**
 *  Interrupt handler for miscellaneous interrupts that do not represent an exception.
 */
void System::MiscellaneousInterruptHandler(INTERRUPT_HANDLER_ARGS)
{
    FAIL("<<MISC INT @ %Xp (%Xs) - vector %u1/%X1>>"
        , INSTRUCTION_POINTER, state->ErrorCode
        , vector, vector);
}

/**
 *  Interrupt handler for division by 0.
 */
void System::DivideErrorHandler(INTERRUPT_HANDLER_ARGS)
{
    CpuData * cpuData = CpuDataSetUp ? Cpu::GetData() : nullptr;

    Synchronization::LockGuard<Synchronization::SmpLock > lg {PrintLock};

    if (cpuData != nullptr)
        MSG("<< DIVIDE ERROR! core %us >>%n", cpuData->Index);
    else
        MSG("<< DIVIDE ERROR! >>%n");

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

    FAIL("<<DIVIDE ERROR @ %Xp>>", INSTRUCTION_POINTER);
}

void System::BreakpointHandler(INTERRUPT_HANDLER_ARGS)
{
    FAIL("<<BREAKPOINT @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for checked arithmetic overflows.
 */
void System::OverflowHandler(INTERRUPT_HANDLER_ARGS)
{
    FAIL("<<OVERFLOW @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for checked exceeded array bounds.
 */
void System::BoundRangeExceededHandler(INTERRUPT_HANDLER_ARGS)
{
    FAIL("<<BOUNDS EXCEEDED @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for invalid opcode exceptions.
 */
void System::InvalidOpcodeHandler(INTERRUPT_HANDLER_ARGS)
{
    FAIL("<<INVALID OPCODE @ %Xp>>", INSTRUCTION_POINTER);
}

/**
 *  Interrupt handler for device not available exception.
 */
void System::NoMathCoprocessorHandler(INTERRUPT_HANDLER_ARGS)
{
    ASSERTX(CpuDataSetUp
        , "CPU data should be set up prior to using the FPU.")XEND;

    Thread * activeThread = Cpu::GetThread();

    ASSERTX(activeThread != nullptr
        , "There should be an active thread when trying to use the FPU.")XEND;

    CpuData * cpuData = Cpu::GetData();
    
    if likely(cpuData->LastExtendedStateThread != activeThread)
    {
        if likely(activeThread->ExtendedState == nullptr)
        {
            //  No extended state means we allocate one.

            Handle res = ExtendedStates::AllocateNew(activeThread->ExtendedState);

            ASSERTX(res.IsOkayResult(), "Failed to allocate extended thread state.")
                (res)XEND;
            //  TODO: Crash the thread gracefully.
        }

        // Cpu::SetCr0(Cpu::GetCr0().SetTaskSwitched(true));
        CpuInstructions::Clts();

        Fpu::LoadState(activeThread->ExtendedState);
    }
    else
    {
        //  So the same thread is trying to use the FPU state as last time.
        //  In this case, no need to reload.

        CpuInstructions::Clts();
        //  Just let it do its job.
    }
}

/**
 *  Interrupt handler for double faults.
 */
void System::DoubleFaultHandler(INTERRUPT_HANDLER_ARGS)
{
    FAIL("<<DOUBLE FAULT @ %Xp (%Xs)>>", INSTRUCTION_POINTER, state->ErrorCode);
    asm volatile ("cli \n\t");
    while (true) { asm volatile ("hlt \n\t"); }
}

/**
 *  Interrupt handler for invalid TSS exceptions.
 */
void System::InvalidTssHandler(INTERRUPT_HANDLER_ARGS)
{
    FAIL("<<INVALID TSS @ %Xp (%Xs)>>", INSTRUCTION_POINTER, state->ErrorCode);
}

/**
 *  Interrupt handler for invalid segment descriptors.
 */
void System::SegmentNotPresentHandler(INTERRUPT_HANDLER_ARGS)
{
    uint16_t ES = 0xFFFF, FS = 0xFFFF, GS = 0xFFFF;
    //  Used for retrieving the registers.

    asm volatile ( "mov %%es, %0 \n\t"
                   "mov %%fs, %1 \n\t"
                   "mov %%gs, %2 \n\t"
                 : "=r"(ES), "=r"(FS), "=r"(GS));

    FAIL("<<SEGMENT NOT PRESENT @ %Xp (%Xs): CS%X2 DS%X2 SS%X2 ES%X2 FS%X2 GS%X2>>"
        , INSTRUCTION_POINTER, state->ErrorCode
        , (uint16_t)state->CS, (uint16_t)state->DS, (uint16_t)state->SS, ES, FS, GS);
}

/**
 *  Interrupt handler for invalid stack segmrnt exception.
 */
void System::StackSegmentFaultHandler(INTERRUPT_HANDLER_ARGS)
{
    FAIL("<<STACK SEGMENT FAULT @ %Xp (%Xs): SS%X2>>"
        , INSTRUCTION_POINTER, state->ErrorCode, (uint16_t)state->SS);
}

/**
 *  Interrupt handler for general protection exceptions.
 */
void System::GeneralProtectionHandler(INTERRUPT_HANDLER_ARGS_FULL)
{
    CpuData * cpuData = CpuDataSetUp ? Cpu::GetData() : nullptr;

    withLock (Debug::MsgSpinlock)
    {
        if (cpuData != nullptr)
            MSG("<< GP FAULT! core %us >>%n", cpuData->Index);
        else
            MSG("<< GP FAULT! >>%n");

        PrintToDebugTerminal(state);

        uintptr_t stackPtr = state->RSP;
        uintptr_t const stackEnd = RoundUp(stackPtr, PageSize);

        if ((stackPtr & (sizeof(size_t) - 1)) != 0)
        {
            MSG("Stack pointer was not a multiple of %us! (%Xp)%n"
                , sizeof(size_t), stackPtr);

            stackPtr &= ~((uintptr_t)(sizeof(size_t) - 1));
        }

        bool odd;
        for (odd = true; stackPtr < stackEnd; stackPtr += sizeof(size_t), odd = !odd)
        {
            MSG("%X2|%Xs|%s"
                , (uint16_t)(stackPtr - state->RSP)
                , *((size_t const *)stackPtr)
                , odd ? "\t" : "\r\n");
        }

        if (odd) MSG("%n");

        Utils::StackFrame stackFrame;

        if (stackFrame.LoadFirst(state->RSP, state->RBP, state->RIP))
        {
            do
            {
                MSG("[Func %Xp; Stack top %Xp + %us]%n"
                    , stackFrame.Function, stackFrame.Top, stackFrame.Size);

            } while (stackFrame.LoadNext());
        }
    }

    FAIL("General protection fault @ %Xp (%Xs)"
        , INSTRUCTION_POINTER, state->ErrorCode);
}

/**
 *  Interrupt handler for page faults.
 */
void System::PageFaultHandler(INTERRUPT_HANDLER_ARGS_FULL)
{
    vaddr_t CR2 = (vaddr_t)Cpu::GetCr2();
    auto pff = (PageFaultFlags)(state->ErrorCode);

    Cores::AssertCoreRegistration();

    // MSG_("! Page fault at %Xp: %X1; RIP %Xp !", CR2, pff, INSTRUCTION_POINTER);

    // CpuData * cpuData = CpuDataSetUp ? Cpu::GetData() : nullptr;

    // if (cpuData)
    // {
    //     if (cpuData->Index > 0)
    //     {
    //         ASSERTX(CR2 >= Vmm::KernelStart)(CR2)XEND;
    //     }
    // }

    Handle res {};

    res = Vmm::HandlePageFault(nullptr, CR2, pff);

    if likely(res.IsOkayResult())
        return;
    //  All's good!

    bool const present = 0 != (state->ErrorCode & 1);
    bool const write = 0 != (state->ErrorCode & 2);
    bool const user = 0 != (state->ErrorCode & 4);
    bool const reserved = 0 != (state->ErrorCode & 8);
    bool const instruction = 0 != (state->ErrorCode & 16);

#if   defined(__BEELZEBUB__ARCH_AMD64)
    uint16_t const ind1 = VmmArc::GetPml1Index(CR2)
                 , ind2 = VmmArc::GetPml2Index(CR2)
                 , ind3 = VmmArc::GetPml3Index(CR2)
                 , ind4 = VmmArc::GetPml4Index(CR2);
#elif defined(__BEELZEBUB__ARCH_IA32PAE)
    uint16_t const ind1 = VmmArc::GetPml1Index(CR2)
                 , ind2 = VmmArc::GetPml2Index(CR2)
                 , ind3 = VmmArc::GetPml3Index(CR2);
#elif defined(__BEELZEBUB__ARCH_IA32)
    uint16_t const ind1 = VmmArc::GetPml1Index(CR2)
                 , ind2 = VmmArc::GetPml2Index(CR2);
#endif

    Pml1Entry * e = nullptr;

    Thread * activeThread = CpuDataSetUp ? Cpu::GetThread() : nullptr;
    ExceptionContext * context = (activeThread == nullptr) ? nullptr : activeThread->ExceptionContext;

    if (context == nullptr)
    {
    justFail:
        char status[6] = "     ";

        if (present)     status[0] = 'P';
        if (write)       status[1] = 'W'; else status[1] = 'r';
        if (user)        status[2] = 'U'; else status[2] = 's';
        if (reserved)    status[3] = '0';
        if (instruction) status[4] = 'I'; else status[4] = 'd';

        withLock (PrintLock)
        {
            MSG("%n<< PAGE FAULT @ %Xp ("
                , INSTRUCTION_POINTER);

            switch (state->ErrorCode)
            {
            case 0:
                MSG("kernel attempted to read from absent page");
                break;

            case 2:
                MSG("kernel attempted write to absent page");
                break;

            case 3:
                MSG("kernel could not write to present page");
                break;

            case 4:
                MSG("userland attempted to read from absent page");
                break;

            case 5:
                MSG("userland attempted to read from kernel page");
                break;

            case 6:
                MSG("userland attempted to write to absent page");
                break;

            case 7:
                MSG("userland could not write to present page");
                break;

            case 8:
                MSG("reserved bit set in paging table entry");
                break;

            case 16:
                MSG("kernel attempted to execute from unmapped page");
                break;

            case 17:
                MSG("kernel attempted to execute from non-executable page");
                break;

            case 20:
                MSG("userland attempted to execute from unmapped page");
                break;

            case 21:
                MSG("userland attempted to execute from kernel/non-executable page");
                break;

            default:
                MSG("%s", status);
                break;
            }

            MSG("|%X1); CR2: %Xp | "
                , (uint8_t)state->ErrorCode, CR2);

    #if   defined(__BEELZEBUB__ARCH_AMD64)
            MSG("%u2:%u2:%u2:%u2 | ", ind4, ind3, ind2, ind1);
    #elif defined(__BEELZEBUB__ARCH_IA32PAE)
            MSG("%u2:%u2:%u2 | ", ind3, ind2, ind1);
    #elif defined(__BEELZEBUB__ARCH_IA32)
            MSG("%u2:%u2 | ", ind2, ind1);
    #endif

    #if   defined(__BEELZEBUB__ARCH_AMD64)
            if (CR2 >= VmmArc::FractalStart && CR2 <= VmmArc::FractalEnd)
            {
                vaddr_t vaddr = (CR2 - VmmArc::LocalPml1Base) << 9;

                if (0 != (vaddr & 0x0000800000000000ULL))
                    vaddr |= 0xFFFF000000000000ULL;

                uint16_t vind1 = VmmArc::GetPml1Index(CR2)
                       , vind2 = VmmArc::GetPml2Index(CR2)
                       , vind3 = VmmArc::GetPml3Index(CR2)
                       , vind4 = VmmArc::GetPml4Index(CR2);

                MSG("Adr: %Xp - %u2:%u2:%u2:%u2 | ", vaddr, vind4, vind3, vind2, vind1);
            }
    #endif

            if (e != nullptr)
            {
                PrintToTerminal(Beelzebub::Debug::DebugTerminal, *e);

                MSG(" >>%n");
            }
            else
                MSG("%H >>%n", res);

            MSG("<< AT@%Xp >>%n", activeThread);

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
            for (odd = false; stackPtr < stackEnd; stackPtr += sizeof(size_t), odd = !odd)
            {
                msg("%X2|%Xp|%Xs|%s"
                    , (uint16_t)(stackPtr - state->RSP)
                    , stackPtr
                    , *((size_t const *)stackPtr)
                    , odd ? "\r\n" : "\t");
            }

            if (odd) msg("%n");

            Utils::StackFrame stackFrame;

            if (stackFrame.LoadFirst(state->RSP, state->RBP, state->RIP))
            {
                do
                {
                    msg("[Func %Xp; Stack top %Xp + %us]%n"
                        , stackFrame.Function, stackFrame.Top, stackFrame.Size);

                } while (stackFrame.LoadNext());
            }
        }

        Beelzebub::Debug::CatchFireFormat(__FILE__, __LINE__, nullptr, nullptr);
    }
    else
    {
        //  First, find the right context.

        while (context->Status != ExceptionStatus::Active)
            if (context->Previous != nullptr)
                context = context->Previous;
            else
                goto justFail;
        //  This will find the first ready context.

        //  Now, to prepare exception delivery!

        state->RSP = context->RSP;
        state->RIP = context->SwapPointer;
        //  Returns where told! Sorta like a task switch.

        state->RDI = reinterpret_cast<uint64_t>(context);
        //  Required by the swapper.

        //  And finally, the exception itself.

        activeThread->Exception.InstructionPointer = state->RIP;
        activeThread->Exception.StackPointer = state->RSP;

        if (CR2 == 0)
        {
            activeThread->Exception.Type = ExceptionType::NullReference;
        }
        else
        {
            activeThread->Exception.Type = ExceptionType::MemoryAccessViolation;

            activeThread->Exception.MemoryAccessViolation.PageFlags = MemoryLocationFlags::None;
            //  Makes sure it's all zeros.

            if (e == nullptr)
                activeThread->Exception.MemoryAccessViolation.PhysicalAddress = nullpaddr;
            else
            {
                activeThread->Exception.MemoryAccessViolation.PhysicalAddress = e->GetAddress();

                if (present)
                    activeThread->Exception.MemoryAccessViolation.PageFlags |= MemoryLocationFlags::Present;

                if (e->GetWritable())
                    activeThread->Exception.MemoryAccessViolation.PageFlags |= MemoryLocationFlags::Writable;
                if (!e->GetXd()) //  Note the negation.
                    activeThread->Exception.MemoryAccessViolation.PageFlags |= MemoryLocationFlags::Executable;
                if (e->GetGlobal())
                    activeThread->Exception.MemoryAccessViolation.PageFlags |= MemoryLocationFlags::Global;
                if (e->GetUserland())
                    activeThread->Exception.MemoryAccessViolation.PageFlags |= MemoryLocationFlags::Userland;
                // if (e->GetAccessed())
                //     activeThread->Exception.MemoryAccessViolation.PageFlags |= MemoryLocationFlags::Accessed;
                // if (e->GetDirty())
                //     activeThread->Exception.MemoryAccessViolation.PageFlags |= MemoryLocationFlags::Written;
            }

            if (instruction)
                activeThread->Exception.MemoryAccessViolation.AccessType = MemoryAccessType::Execute;
            else if (write)
                activeThread->Exception.MemoryAccessViolation.AccessType = MemoryAccessType::Write;
            else
                activeThread->Exception.MemoryAccessViolation.AccessType = MemoryAccessType::Read;

            activeThread->Exception.MemoryAccessViolation.Address = reinterpret_cast<void *>(CR2);
        }
    }
}
