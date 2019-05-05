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

#pragma once

#include "execution/process.hpp"
#include <beel/structs.kernel.h>

/*#define DEFINE_THREAD_DATA(type, name) \
    static __section(thread_data) type MCATS(__dummy_var_, __LINE__); \
    type * name = reinterpret_cast<type *>(reinterpret_cast<uint8_t *>(&(MCATS(__dummy_var_, __LINE__))) \
                                         - &thread_data_start + sizeof(Thread));

#define DECLARE_THREAD_DATA(type, name) \
    extern type * name;

#define GET_THREAD_DATA(thr, name) \
    (*reinterpret_cast<decltype(*name)>(reinterpret_cast<uint8_t *>(thr) + reinterpret_cast<size_t>(name)))*/

#define DEFINE_THREAD_DATA(type, name) \
    __section(thread_data) __used type name;

#define DECLARE_THREAD_DATA(type, name) \
    extern type name;

#define GET_THREAD_DATA(thr, name) \
    (*reinterpret_cast<decltype(&name)>(reinterpret_cast<uint8_t *>(thr) \
                                      + reinterpret_cast<size_t>(&name) \
                                      - reinterpret_cast<size_t>(&thread_data_start)))

extern "C" uint8_t thread_data_start;

namespace Beelzebub { namespace Execution
{
    enum class ThreadStatus
    {
        Constructing,
        Active,
    };

    typedef void * (*ThreadEntryPointFunction)(void * const arg);

    /**
     *  A unit of execution.
     */
    class Thread : public Memory::ReferenceCounted<Thread>
                 , public ThreadBase
    {
    public:

        /*  Constructors  */

        inline Thread()
            : ReferenceCounted()
            , ThreadBase()
            , Id(0)
            , Owner(nullptr)
            , Status(ThreadStatus::Constructing)
        {

        }

        Thread(uint16_t id, Process * const owner);

        Thread(Thread const &) = delete;
        Thread(Thread &&) = delete;
        Thread & operator =(Thread const &) = delete;
        Thread & operator =(Thread &&) = delete;

        /*  Destructors  */

        ~Thread();
        void ReleaseMemory();

        /*  Basics  */

        uint16_t const Id;
        Memory::GlobalPointer<Process> Owner;

        /*  Operations  */

        ThreadStatus Status;
        void SetActive();

        char const * Name;
        void SetName(char const * name);

        __hot Handle SwitchTo(Thread * other, GeneralRegisters64 * dest);   //  Implemented in architecture-specific code.
        Handle SwitchToNext(GeneralRegisters64 * dest) { return this->SwitchTo(this->Next, dest); }

        /*  Stack  */

        uintptr_t KernelStackTop, KernelStackBottom, KernelStackPointer;
        void SetKernelStack(uintptr_t top, uintptr_t bottom);

        /*  State  */

        ThreadState State;
        void * ExtendedState;

        /*  Linkage  */

        Thread * Previous;
        Thread * Next;

        Handle IntroduceNext(Thread * const other);

        //  TODO: Eventually implement a proper scheduler and drop the linkage system.

        /*  Parameters  */

        ThreadEntryPointFunction EntryPoint;
    };
}}
