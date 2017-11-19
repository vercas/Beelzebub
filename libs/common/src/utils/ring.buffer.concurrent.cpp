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

#include <beel/utils/ring.buffer.concurrent.hpp>
#include <string.h>
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

/*********************************
    RingBufferConcurrent class
*********************************/

typedef RingBufferConcurrent::PopCookie PopCookie;

/*  Statics  */

PopCookie const RingBufferConcurrent::InvalidCookie { nullptr, nullptr, 0, 0 };

/*  Operations  */

size_t RingBufferConcurrent::TryPush(uint8_t const * elems, size_t count, size_t min)
{
    if (count == 0)
        return 0;
    if (min > count)
        min = count;

    size_t const myTH = __atomic_load_n(&(this->TailHard), __ATOMIC_RELAXED),
                 myHS = __atomic_load_n(&(this->HeadSoft), __ATOMIC_RELAXED);

    if (myTH + this->Capacity < myHS + min)
        return 0;
    //  Not enough space left in the buffer to write the minimum amount of data.
    //  This arithmetic avoids unerflow that would occur when too much data is queued.

    size_t myHH = __atomic_load_n(&(this->HeadHard), __ATOMIC_RELAXED);
    //  This is acquired a tiny bit later so other producers get a few more cycles
    //  to finish their work. Non-const because of the CAS below.

    if (myHS != myHH)
        return 0;
    //  The hard head and the soft head need to be identical, otherwise this producer would
    //  have to block until the hard head becomes equal to the soft headed it started with.

    size_t const newHS = Minimum(myTH + this->Capacity, myHS + count);

    if (!__atomic_compare_exchange_n(&(this->HeadSoft), &myHH, newHS, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return 0;

    size_t const cpycnt = newHS - myHS;

    memcpy(this->Buffer + (myHH % this->Capacity), elems, cpycnt);

    __atomic_store_n(&(this->HeadHard), newHS, __ATOMIC_RELEASE);

    return cpycnt;
}

void RingBufferConcurrent::Push(uint8_t const * elems, size_t count)
{
    if (count == 0)
        return;
    //  Shouldn't happen in practice.

    size_t const myHS = __atomic_fetch_add(&(this->HeadSoft), count, __ATOMIC_RELAXED);
    size_t start = myHS % this->Capacity, hss = myHS;
    //  hss = head soft soft..?

    while (count > 0)
    {
        size_t bufEnd = this->Capacity + __atomic_load_n(&(this->TailHard), __ATOMIC_RELAXED);

        while (hss >= bufEnd)
        {
            //  Wait until there are some bytes available in the buffer.
            DO_NOTHING();

            bufEnd = this->Capacity + __atomic_load_n(&(this->TailHard), __ATOMIC_RELAXED);
        }

        size_t const cpycnt = Minimum(bufEnd - hss, count, this->Capacity - start);
        memcpy(this->Buffer + start, elems, cpycnt);

        elems += cpycnt; count -= cpycnt; start += cpycnt; hss += cpycnt;
        //  This is... less than ideal.

        if (start >= this->Capacity)
            start = 0;
        //  Compiles to `cmov`.

        if (this->HeadHard >= myHS)
            this->HeadHard = hss;
        //  Ditto.
    }

    while (this->HeadHard < myHS) DO_NOTHING();
    //  Wait for previous producers to signal that their data has been written
    //  integrally.

    __atomic_store_n(&(this->HeadHard), myHS, __ATOMIC_RELAXED);
    //  Let other producers know that this one is done.
}

PopCookie RingBufferConcurrent::TryBeginPop(size_t max, size_t min)
{
    if (min > this->Capacity)
        return InvalidCookie;
    //  Cannot return more data than the buffer can contain.

    size_t const myHH = __atomic_load_n(&(this->HeadHard), __ATOMIC_RELAXED),
                 myTH = __atomic_load_n(&(this->TailHard), __ATOMIC_RELAXED);
    size_t myTS = __atomic_load_n(&(this->TailSoft), __ATOMIC_RELAXED);

    if (myHH == myTS || myTH != myTS)
        return InvalidCookie;
    //  All the enqueued data is due to be consumed, nothing else to do. Or...
    //  There's another consumer working with data - this one would have to wait
    //  when ending the pop.

    size_t const relTS = myTS % this->Capacity;

    size_t const bufEnd = Minimum(myHH, myTH + this->Capacity - relTS);
    size_t cpycnt = bufEnd - myTS;

    if (cpycnt < min)
        return InvalidCookie;
    else if (cpycnt > max)
        cpycnt = max;

    if (!__atomic_compare_exchange_n(&(this->TailSoft), &myTS, myTS + cpycnt, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return InvalidCookie;
    //  Well, this phaild.

    return { this, this->Buffer + relTS, cpycnt, myTS };
}

PopCookie RingBufferConcurrent::BeginPop(size_t max, size_t min)
{
    if (min > this->Capacity)
        return InvalidCookie;

    size_t const myHH = __atomic_load_n(&(this->HeadHard), __ATOMIC_RELAXED),
                 myTH = __atomic_load_n(&(this->TailHard), __ATOMIC_RELAXED);
    size_t myTS = __atomic_load_n(&(this->TailSoft), __ATOMIC_RELAXED);

    if (myHH == myTS)
        return InvalidCookie;

    size_t const bufEnd = Minimum(myHH, myTH + this->Capacity);
    size_t cpycnt = bufEnd - myTS;

    if (cpycnt < min)
        return InvalidCookie;
    else if (cpycnt > max)
        cpycnt = max;

    if (!__atomic_compare_exchange_n(&(this->TailSoft), &myTS, myTS + cpycnt, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return InvalidCookie;

    return { this, this->Buffer + (myTS % this->Capacity), cpycnt, myTS };
}

PopCookie RingBufferConcurrent::WaitBeginPop(size_t max, size_t min)
{
    if (min > this->Capacity)
        return InvalidCookie;

    size_t cpycnt, myTS = __atomic_load_n(&(this->TailSoft), __ATOMIC_RELAXED);

loop:
    do
    {
        size_t const myHH = __atomic_load_n(&(this->HeadHard), __ATOMIC_RELAXED),
                     myTH = __atomic_load_n(&(this->TailHard), __ATOMIC_RELAXED);

        if (myHH == myTS)
            goto loop;
        //  This function guarantees it will return data, at some point.

        size_t const bufEnd = Minimum(myHH, myTH + this->Capacity);
        cpycnt = bufEnd - myTS;

        if (cpycnt < min)
            goto loop;  //  Respect the parameters.
        else if (cpycnt > max)
            cpycnt = max;
    }
    while (!__atomic_compare_exchange_n(&(this->TailSoft), &myTS, myTS + cpycnt, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));

    return { this, this->Buffer + (myTS % this->Capacity), cpycnt, myTS };
}

void RingBufferConcurrent::EndPop(PopCookie const * cookie)
{
    while (__atomic_load_n(&(this->TailHard), __ATOMIC_RELAXED) != cookie->Tail)
        DO_NOTHING();

    __atomic_store_n(&(this->TailHard), cookie->Tail + cookie->Count, __ATOMIC_RELEASE);
}
