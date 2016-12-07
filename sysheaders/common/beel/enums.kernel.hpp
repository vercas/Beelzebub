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
 *  The implementation of the virtual memory manager is architecture-specific.
 */

#pragma once

#include <beel/metaprogramming.h>

namespace Beelzebub
{
    /**
     *  Represents possible page/frame sizes.
     */
    enum class FrameSize : uint8_t
    {
        _4KiB = 1,
        _64KiB = 2,
        _2MiB = 3,
        _4MiB = 4,
        _1GiB = 5,
    };

    ENUMOPS_LITE(FrameSize, uint8_t)

    /**
     *  Represents possible magnitudes of addresses (number of significant bits in their numeric value).
     */
    enum class AddressMagnitude : uint8_t
    {
        _16bit = 1,
        _24bit = 2,
        _32bit = 3,
        _48bit = 4,

        Any = 0xFF,
    };

    ENUMOPS_LITE(AddressMagnitude, uint8_t)

    /**
     *  Options for memory (un)mapping.
     */
    enum class MemoryMapOptions : int
    {
        //  No special actions.
        None                 = 0x00,
        //  No locking will be performed.
        NoLocking            = 0x01,
        //  Frame references will not be changed.
        NoReferenceCounting  = 0x02,
        //  Unmapping is done with precision - large pages are split if necessary.
        PreciseUnmapping     = 0x04,
        //  Unmapping does not cause TLB invalidation.
        NoInvalidation       = 0x08,
        //  If TLB invalidation is enabled, it is not broadcast.
        NoBroadcasting       = 0x10,
    };

    ENUMOPS(MemoryMapOptions, int)

    /**
     *  Represents flags to check memory for.
     */
    enum class MemoryCheckType
    {
        Readable = 0x0,
        Writable = 0x1,
        Free     = 0x2,
        Userland = 0x4,
        Private  = 0x8, //  Means it's owned exclusively by the process in question.
    };

    ENUMOPS(MemoryCheckType)

    /**
     *  Represents characteristics of pages that can be mapped.
     */
    enum class MemoryFlags : uint8_t
    {
        //  No flags.
        None       = 0x0,

        //  Shared by all processes.
        Global     = 0x01,
        //  Accessible by user code.
        Userland   = 0x02,

        //  Writing to the page is allowed.
        Writable   = 0x04,
        //  Executing code from the page is allowed.
        Executable = 0x08,
    };

    ENUMOPS(MemoryFlags, uint8_t)

    /**
     *  Represents possible contents of a memory range.
     */
    #define ENUM_MEMORYCONTENT(ENUMINST) \
        ENUMINST(Generic       , 0x00) /*  Generic contents, nothing remarkable.  */ \
        ENUMINST(Share         , 0x01) /*  Shared with other processes.  */ \
        ENUMINST(ThreadStack   , 0x02) /*  A thread's stack.  */ \
        ENUMINST(Runtime       , 0x03) /*  Part of the runtime.  */ \
        ENUMINST(KernelModule  , 0x04) /*  A kernel module mapped in memory.  */ \
        ENUMINST(CpuDatas      , 0x05) /*  The specific data structures of CPUs.  */ \
        ENUMINST(VasDescriptors, 0x06) /*  Descriptors of a VAS.  */ \
        ENUMINST(BootModule    , 0x80) /*  A module loaded by the bootloader.  */ \
        ENUMINST(VbeFramebuffer, 0x81) /*  The VBE framebuffer.  */ \
        ENUMINST(AcpiTable     , 0x82) /*  An ACPI table.  */ \
        ENUMINST(VmmBootstrap  , 0xFE) /*  VMM bootstrapping data.  */ \
        ENUMINST(Free          , 0xFF) /*  Nothing, waiting to be used.  */ \

    ENUMDECL(MemoryContent, ENUM_MEMORYCONTENT, LITE, uint8_t)

#ifdef __BEELZEBUB_KERNEL
    ENUM_TO_STRING_DECL(MemoryContent, ENUM_MEMORYCONTENT);
#endif

    enum class ExceptionStatus : uintptr_t
    {
        /////////////////////////////////////////////
        // THESE VALUES ARE HARDCODED IN ASSEMBLY! //
        /////////////////////////////////////////////

        Active                  = 0,
        Suspended               = 1,
        Handling                = 2,
        SettingUp               = 3,

        Unknown                 = ~((uintptr_t)0),
    };

    enum class ExceptionType : uintptr_t
    {
        //  This one indicates that there is no exception.
        None                    = 0,
        //  Null pointer dereference.
        NullReference           = 1,
        //  Illegal memory access.
        MemoryAccessViolation   = 2,
        //  Integral division by zero.
        DivideByZero            = 3,
        //  Checked arithmeetic overflow.
        ArithmeticOverflow      = 4,
        //  Invalid instruction encoding or opcode.
        InvalidInstruction      = 5,

        //  A unit test failed.
        UnitTestFailure         = 100,

        //  Haywire.
        Unknown                 = ~((uintptr_t)0),
    };

    enum class MemoryAccessType : uint8_t
    {
        Read            = 0,
        Write           = 1,
        Execute         = 2,

        Unprivileged    = 1 << 6,
        Unaligned       = 1 << 7,
    };

    ENUMOPS(MemoryAccessType)

    enum class MemoryLocationFlags : uint16_t
    {
        None        = 0,

        //  The memory at the given (virtual) address is mapped.
        Present     = 1 << 0,
        //  The memory may be written.
        Writable    = 1 << 1,
        //  The memory may be executed.
        Executable  = 1 << 2,
        //  The memory is global (shared by all processes).
        Global      = 1 << 3,
        //  The memory is accessible by userland.
        Userland    = 1 << 4,
        //  Previous access to this area of memory have occured.
        Accessed    = 1 << 5,
        //  This area of memory has been written to.
        Written     = 1 << 6,
    };

    ENUMOPS(MemoryLocationFlags)
}
