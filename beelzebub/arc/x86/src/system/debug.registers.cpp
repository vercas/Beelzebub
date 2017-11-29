/*
    Copyright (c) 2017 Alexandru-Mihai Maftei. All rights reserved.


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

#include "system/debug.registers.hpp"
#include <beel/sync/smp.lock.hpp>
#include "mailbox.hpp"
#include "cores.hpp"
#include <utils/bitfields.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

struct Dr7
{
    /*  Constructor(s)  */

    inline Dr7() : Value() { }
    inline explicit Dr7(size_t val) : Value( val) { }

    /*  Properties  */

    BITFIELD_FLAG_RW( 8, Le , size_t, this->Value, , const, static)
    BITFIELD_FLAG_RW( 9, Ge , size_t, this->Value, , const, static)
    BITFIELD_FLAG_RW(11, Rtm, size_t, this->Value, , const, static)
    BITFIELD_FLAG_RW(13, Gd , size_t, this->Value, , const, static)

    __forceinline Dr7 & SetProperties(size_t index, bool le, bool ge, BreakpointCondition bc, BreakpointSize bs)
    {
        size_t const mask = (3 << (index * 2)) | (0xF << (16 + (index * 4)));
        size_t const vals = (((le ? 1 : 0) | (ge ? 2 : 0)) << (index * 2))
                          | (((size_t)bc | ((size_t)bs << 2)) << (16 + (index * 4)));

        this->Value = (this->Value & ~mask) | vals;

        return *this;
    }

    __forceinline Dr7 & SetProperties(size_t index, BreakpointProperties const pr)
    {
        return this->SetProperties(index, pr.Local, pr.Global, pr.Condition, pr.Size);
    }

    __forceinline BreakpointProperties GetProperties(size_t index)
    {
        return BreakpointProperties {
            (BreakpointCondition)((this->Value >> (16 + (index * 4))) & 3),
            (BreakpointSize     )((this->Value >> (18 + (index * 4))) & 3),
            ((this->Value >> (1  + (index * 2))) & 1) != 0,
            ((this->Value >>       (index * 2) ) & 1) != 0,
        };
    }

    /*  Fields  */

    size_t Value;
};

static __thread size_t BreakpointCount = 0;
static __thread BreakpointFunction Handlers[4];

static __hot __realign_stack void DebugHandler(INTERRUPT_HANDLER_ARGS_FULL)
{
    size_t dr6;

    asm volatile ( "mov %%dr6, %0 \n\t" : "=rm"(dr6) );

    for (size_t i = 0; i < 4; ++i)
        if (Handlers[i] != nullptr && ((dr6 >> i) & 1) != 0)
        {
            void * bpAddr;

            switch (i)
            {
            case 0:  asm volatile ( "mov %%dr0, %0 \n\t" : "=rm"(bpAddr) ); break;
            case 1:  asm volatile ( "mov %%dr1, %0 \n\t" : "=rm"(bpAddr) ); break;
            case 2:  asm volatile ( "mov %%dr2, %0 \n\t" : "=rm"(bpAddr) ); break;
            default: asm volatile ( "mov %%dr3, %0 \n\t" : "=rm"(bpAddr) ); break;
            }

            Dr7 dr7;

            asm volatile ( "mov %%dr7, %0 \n\t" : "=rm"(dr7.Value) );

            BreakpointProperties bp = dr7.GetProperties(i);
            BreakpointProperties const nbp { bp.Condition, bp.Size, false, false };

            dr7.SetProperties(i, nbp);
            asm volatile ( "mov %0, %%dr7 \n\t" : : "r"(dr7.Value) );
            //  Temporarily disable this specific breakpoint.

            Handlers[i](state, ender, handler, vector, bpAddr, bp);

            dr7.SetProperties(i, bp);
            asm volatile ( "mov %0, %%dr7 \n\t" : : "r"(dr7.Value) );
            //  And restore this breakpoint to what the handler says.

            return;
        }

    FAIL("Debug interrupt triggered for unknown reasons; DR6=%Xs", dr6);
}

static SmpLock InitLock;
static bool Initialized = false;

struct BpAddData
{
    void const * addr;
    BreakpointFunction fnc;
    bool global;
    BreakpointCondition bc;
    BreakpointSize bs;
    size_t additions;
};

struct BpRemData
{
    void const * addr;
    size_t removals;
};

static bool AddBpInternal(BpAddData * bad)
{
    Dr7 dr7;

    asm volatile ( "mov %%dr7, %0 \n\t" : "=r"(dr7.Value) );

    for (size_t i = 0; i < 4; ++i)
    {
        BreakpointProperties bp = dr7.GetProperties(i);

        if (bp.IsEnabled())
            continue;

        if (bad->global)
            bp.Global = true;
        else
            bp.Local = true;

        bp.Condition = bad->bc;
        bp.Size = bad->bs;

        dr7.SetProperties(i, bp);

        switch (i)
        {
        case 0:  asm volatile ( "mov %0, %%dr0 \n\t" : : "r"(bad->addr) ); break;
        case 1:  asm volatile ( "mov %0, %%dr1 \n\t" : : "r"(bad->addr) ); break;
        case 2:  asm volatile ( "mov %0, %%dr2 \n\t" : : "r"(bad->addr) ); break;
        default: asm volatile ( "mov %0, %%dr3 \n\t" : : "r"(bad->addr) ); break;
        }

        Handlers[i] = bad->fnc;

        // MSG_("Core %us added breakpoint #%us at %Xp.%n", Cpu::GetData()->Index, i, bad->addr);

        asm volatile ( "mov %0, %%dr7 \n\t" : : "r"(dr7.Value) );

        ++BreakpointCount;
        ++bad->additions;
        return true;
    }

    return false;
}

static bool RemBpInternal(BpRemData * brd)
{
    if (BreakpointCount == 0)
        return false;

    Dr7 dr7;

    asm volatile ( "mov %%dr7, %0 \n\t" : "=r"(dr7.Value) );

    for (size_t i = 0; i < 4; ++i)
    {
        BreakpointProperties bp = dr7.GetProperties(i);

        if (!bp.IsEnabled())
            continue;

        void * bpAddr;

        switch (i)
        {
        case 0:  asm volatile ( "mov %%dr0, %0 \n\t" : "=r"(bpAddr) ); break;
        case 1:  asm volatile ( "mov %%dr1, %0 \n\t" : "=r"(bpAddr) ); break;
        case 2:  asm volatile ( "mov %%dr2, %0 \n\t" : "=r"(bpAddr) ); break;
        default: asm volatile ( "mov %%dr3, %0 \n\t" : "=r"(bpAddr) ); break;
        }

        if (brd->addr != bpAddr)
            continue;

        bp.Global = false;
        bp.Local = false;
        //  The rest of the properties don't really matter.

        dr7.SetProperties(i, bp);

        Handlers[i] = nullptr;

        // MSG_("Core %us removed breakpoint #%us at %Xp.%n", Cpu::GetData()->Index, i, brd->addr);

        asm volatile ( "mov %0, %%dr7 \n\t" : : "r"(dr7.Value) );

        --BreakpointCount;
        ++brd->removals;
        return true;
    }

    return false;
}

static void AddBpExternal(void * cookie)
{
    AddBpInternal((BpAddData *)cookie);
}

static void RemBpExternal(void * cookie)
{
    RemBpInternal((BpRemData *)cookie);
}

/***************************
    DebugRegisters class
***************************/

/*  Initialization  */

void DebugRegisters::Initialize()
{
    if (InitLock.TryAcquire())
    {
        if (Initialized)
            return;

        Initialized = true;

        InitLock.Release();
    }
    else
        return;

    Interrupts::Get(KnownExceptionVectors::Debug).SetHandler(&DebugHandler);
}

/*  Operation  */

bool DebugRegisters::AddBreakpoint(void const * addr, size_t size, bool global
    , BreakpointCondition bc, BreakpointFunction fnc)
{
    if (BreakpointCount == 4)
        return false;

    BreakpointSize bs;

    switch (size)
    {
    case 1: bs = BreakpointSize::One; break;
    case 2: bs = BreakpointSize::Two; break;
    case 4: bs = BreakpointSize::Four; break;
    case 8: bs = BreakpointSize::Eight; break;
    default: return false;
    }

    BpAddData bad { addr, fnc, global, bc, bs, 0 };

    ALLOCATE_MAIL_BROADCAST(m1, &AddBpExternal, &bad);
    m1.SetAwait(true).SetNonMaskable(true).Post(&AddBpExternal, &bad);

    return bad.additions == Cores::GetCount();
}

bool DebugRegisters::RemoveBreakpoint(void const * addr)
{
    if (BreakpointCount == 0)
        return false;

    BpRemData brd { addr, 0 };

    ALLOCATE_MAIL_BROADCAST(m1, &RemBpExternal, &brd);
    m1.SetAwait(true).SetNonMaskable(true).Post(&RemBpExternal, &brd);

    return brd.removals == Cores::GetCount();
}

size_t DebugRegisters::GetBreakpointCount()
{
    return BreakpointCount;
}
