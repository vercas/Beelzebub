/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


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

#include <system/fpu.hpp>
#include <system/cpu.hpp>
#include <system/xcrs.hpp>
#include <entry.h>

#include <debug.hpp>

#ifdef __BEELZEBUB__ARCH_AMD64
    #define SAVE_SUFFIX "64"
#else
    #define SAVE_SUFFIX ""
#endif

using namespace Beelzebub;
using namespace Beelzebub::System;

/****************
    Fpu class
****************/

/*  Statics  */

bool Fpu::Available = false;
bool Fpu::Sse = false;
bool Fpu::Avx = false;
bool Fpu::Xsave = false;

size_t Fpu::StateSize = 0;
size_t Fpu::StateAlignment = 0;

XsaveRfbm Fpu::Xcr0 {};

/*  Initialization  */

void Fpu::InitializeMain()
{
    Fpu::Available = BootstrapCpuid.CheckFeature(CpuFeature::FPU);
    Fpu::Sse = BootstrapCpuid.CheckFeature(CpuFeature::SSE);
    Fpu::Avx = BootstrapCpuid.CheckFeature(CpuFeature::AVX);
    Fpu::Xsave = BootstrapCpuid.CheckFeature(CpuFeature::XSAVE);

    if (Fpu::Available)
    {
        Fpu::StateSize = sizeof(FxsaveMap);
        Fpu::StateAlignment = __alignof(FxsaveMap);

        if (Fpu::Avx)
        {
            ASSERT(Fpu::Xsave, "XSAVE should be available with AVX!");

            uint32_t w, x, y, z;
            CpuId::Execute(0xDU, 0U, w, x, y, z);

            ASSERT(y >= Fpu::StateSize
                , "New FPU state size (%u4) shouldn't be smaller than the first"
                  " one (%us)."
                , y, Fpu::StateSize);

            Fpu::StateSize = y;
        }
        
        if (!Fpu::Avx)
            Fpu::Xsave = false;
        //  Disable XSAVE usage... It's unnecessary.
    }
    else
    {
        Fpu::StateSize = 0;
        Fpu::StateAlignment = 0;
    }

    // msg("** FPU%b SSE%b AVX%b XSAVE%b; SS=%us SA=%us **%n"
    //     , Fpu::Available, Fpu::Sse, Fpu::Avx, Fpu::Xsave
    //     , Fpu::StateSize, Fpu::StateAlignment);

    Fpu::InitializeSecondary();
}

    void Fpu::InitializeSecondary()
    {
        Cpu::SetCr4(Cpu::GetCr4().SetOsfxsr(Fpu::Sse).SetOsxsave(Fpu::Xsave));

        if (Fpu::Xsave)
        {
            Fpu::Xcr0 = Fpu::Xcr0.SetX87(Fpu::Available).SetSse(Fpu::Sse).SetAvx(Fpu::Avx);

            Xcrs::Write(0, Fpu::Xcr0.Low, Fpu::Xcr0.High);
        }

        if (Fpu::Available)
        {
            asm volatile ("fninit");
        }
    }

/*  Operations  */

void Fpu::SaveState(void * state)
{
    if (Fpu::Xsave)
    {
        asm volatile (  "xsave" SAVE_SUFFIX " %[ptr] \n\t"
                     :
                     : [ptr]"m"(*((char *)state))
                     , "a"(Fpu::Xcr0.Low), "d"(Fpu::Xcr0.High) );
    }
    else
        asm volatile ( "fxsave" SAVE_SUFFIX " %[ptr] \n\t" : : [ptr]"m"(*((char *)state)) );
}

void Fpu::LoadState(void * state)
{
    if (Fpu::Xsave)
    {
        asm volatile (  "xrstor" SAVE_SUFFIX " %[ptr] \n\t"
                     :
                     : [ptr]"m"(*((char *)state))
                     , "a"(Fpu::Xcr0.Low), "d"(Fpu::Xcr0.High) );
    }
    else
        asm volatile ( "fxrstor" SAVE_SUFFIX " %[ptr] \n\t" : : [ptr]"m"(*((char *)state)) );
}
