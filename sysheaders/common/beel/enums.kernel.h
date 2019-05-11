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

#ifndef __ASSEMBLER__

/**
 *  Represents possible page/frame sizes.
 */
#define __ENUM_FRAMESIZE(ENUMINST) \
    ENUMINST(_4KiB , 1) \
    ENUMINST(_64KiB, 2) \
    ENUMINST(_2MiB , 3) \
    ENUMINST(_4MiB , 4) \
    ENUMINST(_1GiB , 5)

__PUB_ENUM(FrameSize, __ENUM_FRAMESIZE, LITE, uint8_t)

/**
 *  Represents possible magnitudes of addresses (number of significant bits in their numeric value).
 */
#define __ENUM_ADDRESSMAGNITUDE(ENUMINST) \
    ENUMINST(_16bit, 1   ) \
    ENUMINST(_24bit, 2   ) \
    ENUMINST(_32bit, 3   ) \
    ENUMINST(_48bit, 4   ) \
    ENUMINST(Any   , 0xFF)

__PUB_ENUM(AddressMagnitude, __ENUM_ADDRESSMAGNITUDE, LITE, uint8_t)

/**
 *  Options for memory (un)mapping.
 */
#define __ENUM_MEMORYMAPOPTIONS(ENUMINST) \
    /* No special actions. */ \
    ENUMINST(None                , 0x00) \
    /* No locking will be performed. */ \
    ENUMINST(NoLocking           , 0x01) \
    /* Frame references will not be changed. */ \
    ENUMINST(NoReferenceCounting , 0x02) \
    /* Unmapping is done with precision - large pages are split if necessary. */ \
    ENUMINST(PreciseUnmapping    , 0x04) \
    /* Unmapping does not cause TLB invalidation. */ \
    ENUMINST(NoInvalidation      , 0x08) \
    /* If TLB invalidation is enabled, it is not broadcast. */ \
    ENUMINST(NoBroadcasting      , 0x10)

__PUB_ENUM(MemoryMapOptions, __ENUM_MEMORYMAPOPTIONS, FULL)

/**
 *  Represents flags to check memory for.
 */
#define __ENUM_MEMORYCHECKTYPE(ENUMINST) \
    ENUMINST(Readable, 0x0) \
    ENUMINST(Writable, 0x1) \
    ENUMINST(Free    , 0x2) \
    ENUMINST(Userland, 0x4) \
    /* Means it's owned exclusively by the process in question. */ \
    ENUMINST(Private , 0x8)

__PUB_ENUM(MemoryCheckType, __ENUM_MEMORYCHECKTYPE, FULL)

/**
 *  Represents characteristics of pages that can be mapped.
 */
#define __ENUM_MEMORYFLAGS(ENUMINST) \
    ENUMINST(None      , 0x00) /* No flags. */ \
    ENUMINST(Global    , 0x01) /* Shared by all processes. */ \
    ENUMINST(Userland  , 0x02) /* Accessible by user code. */ \
    ENUMINST(Writable  , 0x04) /* Writing to the page is allowed. */ \
    ENUMINST(Executable, 0x08) /* Executing code from the page is allowed. */

__PUB_ENUM(MemoryFlags, __ENUM_MEMORYFLAGS, FULL, uint8_t)

/**
 *  Represents possible contents of a memory range.
 */
#define __ENUM_MEMORYCONTENT(ENUMINST) \
    ENUMINST(Generic       , 0x00) /*  Generic contents, nothing remarkable.  */ \
    ENUMINST(Share         , 0x01) /*  Shared with other processes.  */ \
    ENUMINST(ThreadStack   , 0x02) /*  A thread's stack.  */ \
    ENUMINST(Runtime       , 0x03) /*  Part of the runtime.  */ \
    ENUMINST(KernelModule  , 0x04) /*  A kernel module mapped in memory.  */ \
    ENUMINST(CpuDatas      , 0x05) /*  The specific data structures of CPUs.  */ \
    ENUMINST(VasDescriptors, 0x06) /*  Descriptors of a VAS.  */ \
    ENUMINST(HandleTable   , 0x07) /*  (Part of) The kernel's handle table.  */ \
    ENUMINST(HandleMap     , 0x08) /*  (Part of) A process's handle map.  */ \
    ENUMINST(ProcessList   , 0x10) /*  A list of processes, typically used by the scheduler.  */ \
    ENUMINST(ThreadList    , 0x11) /*  A list of threads, typically used by the scheduler.  */ \
    ENUMINST(BootModule    , 0x80) /*  A module loaded by the bootloader.  */ \
    ENUMINST(VbeFramebuffer, 0x81) /*  The VBE framebuffer.  */ \
    ENUMINST(AcpiTable     , 0x82) /*  An ACPI table.  */ \
    ENUMINST(VmmBootstrap  , 0xFE) /*  VMM bootstrapping data.  */ \
    ENUMINST(Free          , 0xFF) /*  Nothing, waiting to be used.  */

__PUB_ENUM(MemoryContent, __ENUM_MEMORYCONTENT, LITE, uint8_t)


/////////////////////////////////////////////
// THESE VALUES ARE HARDCODED IN ASSEMBLY! //
/////////////////////////////////////////////
/**
 *  Possible status of an exception context.
 */
#define __ENUM_EXCEPTIONCONTEXTSTATUS(ENUMINST) \
    ENUMINST(Active   , 0) /* All good. */ \
    ENUMINST(Suspended, 1) /* Temporarily suspended. */ \
    ENUMINST(Handling , 2) /* In the process of handling an exception. */ \
    ENUMINST(SettingUp, 3) /* Context is being set up. */ \
    ENUMINST(Unknown  , ~((uintptr_t)0))

__PUB_ENUM(ExceptionContextStatus, __ENUM_EXCEPTIONCONTEXTSTATUS, LITE, uintptr_t)

/**
 *  Known exception types.
 */
#define __ENUM_EXCEPTIONTYPE(ENUMINST) \
    ENUMINST(None                   , 0x00) /*  This one indicates that there is no exception. */ \
    ENUMINST(NullReference          , 0x01) /*  Null pointer dereference.  */ \
    ENUMINST(MemoryAccessViolation  , 0x02) /*  Illegal memory access.  */ \
    ENUMINST(DivideByZero           , 0x03) /*  Integral division by zero.  */ \
    ENUMINST(ArithmeticOverflow     , 0x04) /*  Checked arithmetic overflow.  */ \
    ENUMINST(InvalidInstruction     , 0x05) /*  Invalid instruction encoding or opcode.  */ \
    ENUMINST(UnitTestFailure        , 0xFF) /*  A unit test failed.  */ \
    ENUMINST(Unknown                , (0UL - 1UL)) /*  Haywire!  */

__PUB_ENUM(ExceptionType, __ENUM_EXCEPTIONTYPE, LITE, uintptr_t)

/**
 *  
 */
#define __ENUM_MEMORYACCESSTYPE(ENUMINST) \
    ENUMINST(Read        , 0) \
    ENUMINST(Write       , 1) \
    ENUMINST(Execute     , 2) \
    ENUMINST(Unprivileged, 1 << 6) \
    ENUMINST(Unaligned   , 1 << 7)

__PUB_ENUM(MemoryAccessType, __ENUM_MEMORYACCESSTYPE, FULL, uint8_t)

/**
 *  Represents possible flags of a memory location
 */
#define __ENUM_MEMORYLOCATIONFLAGS(ENUMINST) \
    ENUMINST(None          , 0x00) \
    ENUMINST(Present       , 0x01) /*  The memory at the given (virtual) address is mapped.  */ \
    ENUMINST(Writable      , 0x02) /*  The memory may be written.  */ \
    ENUMINST(Executable    , 0x04) /*  The memory may be executed.  */ \
    ENUMINST(Global        , 0x08) /*  The memory is global (shared by all processes).  */ \
    ENUMINST(Userland      , 0x10) /*  The memory is accessible by userland.  */ \

__PUB_ENUM(MemoryLocationFlags, __ENUM_MEMORYLOCATIONFLAGS, FULL, uint16_t)

/**
 *  Possible status of a thread in regards to scheduling.
 */
#define __ENUM_SCHEDULERSTATUS(ENUMINST) \
    ENUMINST(Unscheduled , 0) /* Thread is not currently participating in scheduling. */ \
    ENUMINST(Queued      , 1) /* Queued for scheduling but not yet executing or due to be executed. */ \
    ENUMINST(Executing   , 2) /* Currently being executed, or will execute shortly. */ \
    ENUMINST(Blocked     , 3) /* Thread is waiting for an event. */ \

__PUB_ENUM(SchedulerStatus, __ENUM_SCHEDULERSTATUS, LITE)

#endif

#ifdef __BEELZEBUB__SOURCE_CXX
namespace Beelzebub
{
    __ENUM_TO_STRING_DECL(ExceptionType);

#ifdef __BEELZEBUB_KERNEL
    __ENUM_TO_STRING_DECL(MemoryContent);

    bool MemoryContentsMergeable(MemoryContent a, MemoryContent b);

    __ENUM_TO_STRING_DECL(FrameSize);
#endif

    __ENUM_TO_STRING_DECL(SchedulerStatus);
}
#endif
