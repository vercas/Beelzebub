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

/**
 *  The implementation of the physical memory manager is largely architecture-
 *  specific.
 */

#pragma once

#include <beel/handles.h>
#include <beel/enums.kernel.hpp>

namespace Beelzebub { namespace Memory
{
    /**
     *  The physical memory manager.
     */
    class Pmm
    {
    public:
        /*  Statics  */

        static void * const InvalidDescriptor;

        /*  Frame operations  */

        static __hot __noinline paddr_t AllocateFrame(void * & desc, FrameSize size = FrameSize::_4KiB, AddressMagnitude magn = AddressMagnitude::Any);

        static __hot __forceinline paddr_t AllocateFrame(FrameSize size = FrameSize::_4KiB, AddressMagnitude magn = AddressMagnitude::Any)
        {
            void * sink;

            return AllocateFrame(sink, size, magn);
        }

        static __hot __forceinline paddr_t AllocateFrame(void * & desc, AddressMagnitude magn, FrameSize size = FrameSize::_4KiB)
        { return AllocateFrame(desc, size, magn); }

        static __hot __forceinline paddr_t AllocateFrame(AddressMagnitude magn, FrameSize size = FrameSize::_4KiB)
        { return AllocateFrame(size, magn); }

        static __hot __noinline Handle FreeFrame(paddr_t addr, bool ignoreRefCnt = true);
        static __cold __noinline Handle ReserveRange(paddr_t start, size_t size, bool includeBusy = false);

        static __hot __noinline Handle AdjustReferenceCount(paddr_t & addr, void * & desc, uint32_t & newCnt, uint32_t diff);

        static __forceinline __hot Handle AdjustReferenceCount(paddr_t & addr, void * & desc, uint32_t diff)
        {
            uint32_t dummy;

            return AdjustReferenceCount(addr, desc, dummy, diff);
        }

        static __forceinline __hot Handle AdjustReferenceCount(paddr_t addr, uint32_t & newCnt, uint32_t diff)
        {
            void * dummy = nullptr;

            return AdjustReferenceCount(addr, dummy, newCnt, diff);
        }

        static __forceinline __hot Handle AdjustReferenceCount(void * desc, uint32_t & newCnt, uint32_t diff)
        {
            paddr_t dummy = nullpaddr;

            return AdjustReferenceCount(dummy, desc, newCnt, diff);
        }

        static __forceinline __hot Handle AdjustReferenceCount(paddr_t addr, uint32_t diff)
        {
            void * dummy1 = nullptr;
            uint32_t dummy2;

            return AdjustReferenceCount(addr, dummy1, dummy2, diff);
        }

        static __forceinline __hot Handle AdjustReferenceCount(void * desc, uint32_t diff)
        {
            paddr_t dummy1 = nullpaddr;
            uint32_t dummy2;

            return AdjustReferenceCount(dummy1, desc, dummy2, diff);
        }
    };
}}
