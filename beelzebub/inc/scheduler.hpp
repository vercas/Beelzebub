/*
    Copyright (c) 2018 Alexandru-Mihai Maftei. All rights reserved.


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

/*  Note that the implementation of this header is architecture-specific.  */

#pragma once

#include "execution/thread.hpp"
#include "kernel.hpp"

namespace Beelzebub
{
    /**
     *  Scheduler assistance!
     */
    class Scheduler
    {
    public:
        static constexpr size_t const MaximumCPUs = 512;
        static constexpr int const PriorityLevels = 32;

        static bool Postpone;

        struct AffinityMask
        {
            static constexpr size_t const Size = (MaximumCPUs + sizeof(size_t) * 8 - 1) / (sizeof(size_t) * 8);

            inline bool IsAllZero() const
            {
                size_t val = 0;

                for (size_t i = 0; i < Size; ++i)
                    val |= this->Bitmap[i];

                return val == 0;
            }

            inline bool IsAllOne() const
            {
                size_t val = ~(size_t)0UL;

                for (size_t i = 0; i < Size; ++i)
                    val &= this->Bitmap[i];

                return val == ~(size_t)0UL;
            }

            inline bool operator [](size_t const index) const
            {
                size_t const indexWord = (index & ~(sizeof(size_t) * 8UL - 1UL)) >> (sizeof(size_t) == 4 ? 5 : 6);
                size_t const indexBitMask = 1UL << (index & (sizeof(size_t) * 8UL - 1UL));

                return 0 != (this->Bitmap[indexWord] & indexBitMask);
            }

            inline void SetBit(size_t const index)
            {
                size_t const indexWord = (index & ~(sizeof(size_t) * 8UL - 1UL)) >> (sizeof(size_t) == 4 ? 5 : 6);
                size_t const indexBitMask = 1UL << (index & (sizeof(size_t) * 8UL - 1UL));

                this->Bitmap[indexWord] |= indexBitMask;
            }

            size_t Bitmap[Size];
        };

        static AffinityMask const & AllCpusMask;

    protected:
        /*  Constructor(s)  */

        Scheduler() = default;

    public:
        Scheduler(Scheduler const &) = delete;
        Scheduler & operator =(Scheduler const &) = delete;

        /*  Initialization  */

        static __startup void Initialize(MainParameters * params);

    private:
        static __startup void InitializeIdleThread(MainParameters * params);

    public:
        static void Engage();

        static void Enroll(Execution::Thread * thread);

        /*  Properties  */

        static SchedulerStatus GetStatus(Execution::Thread * thread);
        static void SetAffinity(Execution::Thread * thread, AffinityMask const & affinity);
    };
}
