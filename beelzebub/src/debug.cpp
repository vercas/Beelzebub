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

#include <debug.hpp>
#include "memory/vmm.hpp"
#include "cores.hpp"
#include <beel/interrupt.state.hpp>

#ifdef __BEELZEBUB_SETTINGS_KRNDYNALLOC_VALLOC
#include <valloc/interface.hpp>
#endif

using namespace Beelzebub;
using namespace Beelzebub::Debug;
using namespace Beelzebub::System;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Terminals;

static SmpLock DataDumpLock;

static __cold void Killer(void * cookie)
{
    (void)cookie;

    DEBUG_TERM_ << "Core " << Cpu::GetData()->Index << " was ordered to catch fire." << EndLine;

#ifdef __BEELZEBUB_SETTINGS_KRNDYNALLOC_VALLOC
    withLock (DataDumpLock)
    {
        DEBUG_TERM << "-------------------- vAlloc on core " << Cpu::GetData()->Index << " --------------------" << EndLine;

        Valloc::DumpMyState();
    }
#endif

    //  Allow the CPU to rest. Interrupts are already disabled.
    while (true) if (CpuInstructions::CanHalt) CpuInstructions::Halt();
}

static __cold __noreturn void Die()
{
    Interrupts::Disable();

    DEBUG_TERM_ << "Core " << Cpu::GetData()->Index << " is setting the system on fire." << EndLine;

#if defined(__BEELZEBUB_SETTINGS_SMP)
    if unlikely(Cores::IsReady())
    {
        ALLOCATE_MAIL_BROADCAST(mail, &Killer);
        mail.Post(false);
    }
#endif

#ifdef __BEELZEBUB_SETTINGS_KRNDYNALLOC_VALLOC
    withLock (DataDumpLock)
    {
        DEBUG_TERM << "-------------------- vAlloc on core " << Cpu::GetData()->Index << " --------------------" << EndLine;
        
        Valloc::DumpMyState();
    }
#endif

    withLock (DataDumpLock)
    {
        DEBUG_TERM   << "-------------------- KVAS --------------------" << EndLine
                     << &(Memory::Vmm::KVas);
    }

    //  Allow the CPU to rest.
    while (true) if (CpuInstructions::CanHalt) CpuInstructions::Halt();
}

SmpLockUni Debug::MsgSpinlock;

//  Although the 'CatchFire' functions will brick the CPU,
//  I still feel obliged to make them... Efficient...

void Debug::CatchFire(char const * const file
                    , size_t const line
                    , char const * const cond
                    , char const * const msg)
{
    if (DebugTerminal != nullptr && DebugTerminal->Capabilities->CanOutput)
        withLock (MsgSpinlock)
        {
            *DebugTerminal << "Caugth fire at:" << Terminals::EndLine
                << '\t' << file << ": " << line << Terminals::EndLine;

            if (cond != nullptr)
                *DebugTerminal << "Expression:" << Terminals::EndLine
                    << '\t' << cond << Terminals::EndLine;

            if (msg != nullptr)
                *DebugTerminal << "Message:" << Terminals::EndLine
                    << '\t' << msg << Terminals::EndLine;
        }

    Die();
}

void Debug::CatchFireV(char const * const file
                     , size_t const line
                     , char const * const cond
                     , char const * const fmt, va_list args)
{
    if (DebugTerminal != nullptr && DebugTerminal->Capabilities->CanOutput)
        withLock (MsgSpinlock)
        {
            *DebugTerminal << "Caugth fire at:" << Terminals::EndLine
                << '\t' << file << ": " << line << Terminals::EndLine;

            if (cond != nullptr)
                *DebugTerminal << "Expression:" << Terminals::EndLine
                    << '\t' << cond << Terminals::EndLine;

            if (fmt != nullptr)
            {
                *DebugTerminal << "Message:" << Terminals::EndLine
                    << '\t';

                DebugTerminal->Write(fmt, args);

                DebugTerminal->WriteLine();
            }
        }

    Die();
}

/*************************
    AssertHelper class
*************************/

/*  State  */

bool AssertHelper::RealityCheck()
{
    if (this->State++ == 0)
        return true;

    Die();

    return false;
}
