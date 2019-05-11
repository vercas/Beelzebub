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

#pragma once

#include <beel/timing.hpp>
#include <beel/handles.h>

namespace Beelzebub
{
    typedef void (* TimedFunctionVoid)(void * cookie);

    template<typename TCookie>
    using TimedFunction = void (*)(TCookie *);

    /**
     *  <summary>Represents an abstract system timer.</summary>
     */
    class Timer
    {
    public:
        /*  Statics  */

        static uint64_t Frequency;
        static size_t TicksPerMicrosecond;

        static constexpr uint_fast16_t const Count = 32;

    protected:
        /*  Constructor(s)  */

        Timer() = default;

    public:
        Timer(Timer const &) = delete;
        Timer & operator =(Timer const &) = delete;

        /*  Initialization  */

        static __startup void Initialize();

        /*  Operation  */

        static bool Enqueue(TimeSpanLite delay, TimedFunctionVoid func, void * cookie = nullptr);

        template<typename TCookie>
        static bool Enqueue(TimeSpanLite delay, TimedFunction<TCookie> func, TCookie * cookie)
        {
            return Enqueue(delay, reinterpret_cast<TimedFunctionVoid>(func), cookie);
        }
    };
}
