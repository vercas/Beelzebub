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

/*
    VERY IMPORTANT NOTE:
    RingBufferConcurrent::WaitBeginPop doesn't attempt to serialize with other
    consumers when they're all waiting for data to be produced.
    Implementing this requires significant (complex, slow) work from all the
    consumers, and a counter...
 */

#pragma once

#include <beel/metaprogramming.h>

namespace Beelzebub { namespace Utils
{
    class RingBufferConcurrent
    {
    public:
        /*  Subtypes  */

        struct PopCookie
        {
            friend class RingBufferConcurrent;

            /*  Properties  */

            inline uint8_t * GetEnd() const { return this->Array + this->Count; }
            inline bool IsValid() const { return this->Queue != nullptr; }
            inline bool IsInvalid() const { return this->Queue == nullptr; }

        private:
            /*  Constructor  */

            inline PopCookie(RingBufferConcurrent * queue, uint8_t * arr, size_t cnt, size_t tail)
                : Queue( queue)
                , Array(arr)
                , Count(cnt)
                , Tail(tail)
            { }

        public:
            /*  Destructor  */

            inline ~PopCookie()
            {
                if likely(this->Queue != nullptr)
                    this->Queue->EndPop(this);
            }

            /*  Fields  */

            RingBufferConcurrent * const Queue;
            uint8_t * const Array;
            size_t const Count;
            size_t const Tail;
        };

        friend struct PopCookie;

    private:
        /*  Statics  */

        static PopCookie const InvalidCookie;

    public:
        /*  Constructors  */

        inline RingBufferConcurrent(uint8_t * buf, size_t cap)
            : TailHard(0), HeadHard(0), TailSoft(0), HeadSoft(0)
            , Buffer(buf), Capacity(cap)
        {

        }

        /*  Properties  */

        inline bool IsEmpty() const
        {
            size_t const ts = __atomic_load_n(&(this->TailSoft), __ATOMIC_RELAXED);
            size_t const hs = __atomic_load_n(&(this->HeadSoft), __ATOMIC_RELAXED);
            //  The specified memory orders force the order of the two memory loads.
            //  This just makes it more likely to get an accurate negative result
            //  under contention.

            return ts == hs;
        }

        /*  Operations  */

        size_t TryPush(uint8_t const * elems, size_t count, size_t min = 1);
        void Push(uint8_t const * elems, size_t count);

        PopCookie TryBeginPop(size_t max, size_t min = 1);
        PopCookie BeginPop(size_t max, size_t min = 1);
        PopCookie WaitBeginPop(size_t max, size_t min = 1);

    private:
        void EndPop(PopCookie const * cookie);

        /*  Fields  */

        size_t TailHard, HeadHard, TailSoft, HeadSoft;

        uint8_t * const Buffer;
        size_t const Capacity;
    };

    // template<typename T, int16_t Size>
    // class RingBufferConcurrent
    // {
    //     /*  Subtypes  */

    //     union RbcState
    //     {
    //         int64_t Value64;
            
    //         struct
    //         {
    //             int16_t TailHard, HeadHard, TailSoft, HeadSoft;
    //         };
    //     };

    // public:
    //     enum AttemptTypes
    //     {
    //         SoftAttempt, MediumAttempt, HardAttempt, BlockingAttempt,
    //     };

    //     struct PopCookie
    //     {
    //         T * const Array;
    //         int16_t const Count;

    //         inline T * GetEnd() { return this->Array + this->Count; }

    //     private:
    //         /*  Constructor  */

    //         PopCookie(T * arr, int16_t cnt) : Array(arr), Count(cnt) { }

    //         friend class RingBufferConcurrent;
    //     };

    //     /*  Constructors  */

    //     RingBufferConcurrent()
    //         : State(), Elements()
    //     {

    //     }

    //     /*  Properties  */

    //     inline bool IsEmpty() const
    //     {
    //         RbcState const st { __atomic_load_n(&(this->State.Value64), __ATOMIC_RELAXED) };
    //         //  This obtains the hard tail and hard head.

    //         return st.TailHard == st.HeadHard;
    //         //  If they are equal, then the buffer is empty.
    //     }

    //     inline bool IsFull() const
    //     {
    //         RbcState const st { __atomic_load_n(&(this->State.Value64), __ATOMIC_RELAXED) };

    //         if (st.TailHard == st.HeadHard - 1)
    //             return true;
    //         //  Hard tail is exactly one element behind the hard head? Means it's empty.

    //         if (st.HeadHard == Size - 1 && st.TailHard == 0)
    //             return true;
    //         //  Same condition, but this one accounts for the position at the end of the buffer.

    //         return false;
    //     }

    //     /*  Operations  */

    //     template<AttemptTypes AT>
    //     bool Push(T const * elems, int16_t count)
    //     {
    //         if (count >= Size)
    //             return false;
    //         //  The buffer cannot accomodate so much.

    //         if (count <= 0)
    //             return true;
    //         //  Shouldn't happen in practice.

    //         RbcState oldSt { __atomic_load_n(&(this->State.Value64), __ATOMIC_ACQUIRE) };

    //         do
    //         {
    //             int16_t const newHS = (oldSt.HeadSoft + count) % Size;

    //             if (oldSt.TailHard <= oldSt.HeadHard)
    //             {
    //                 //  Hard tail is behind hard head, meaning that the contents of the ring buffer do not wrap past the end
    //                 //  of the linear buffer.

    //                 if (oldSt.TailHard <= newHS && newHS <= oldSt.HeadHard)
    //                     return false;
    //                 //  The new soft head falls between the hard tail and the hard head.
    //             }
    //             else
    //             {
    //                 //  Hard tail is ahead of hard head, meaning that the contents of the ring buffer DO wrap past the end
    //                 //  of the linear buffer. So he failure condition is flipped.

    //                 if (newHS >= oldSt.TailHard || newHS <= oldSt.HeadHard)
    //                     return false
    //                 //  The new soft head falls beyond the hard tail or before the hard head.
    //             }

    //             RbcState newSt = oldSt;
    //             newSt.HeadSoft = newHS;

    //             if (!__atomic_compare_exchange_n(&(this->State.Value64), &oldSt, newSt, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
    //             {
    //                 //  The state changed in the meantime. Too bad there's no way to tell this operation to ignore the soft tail. Insert le frowny face.

    //                 if (AT == SoftAttempt)
    //                     return false;
    //             }

    //             break;
    //             //  The operation succeeded, move on.
    //         } while (true);

    //         if (oldSt.HeadSoft + count <= Size)
    //         {
    //             //  No wrap, means one `memcpy`.
                
    //             memcpy(this->Elements + oldSt.HeadSoft, elems, count * sizeof T);
    //         }
    //         else
    //         {
    //             //  Wraps, so two copies are required - one at the end, and one at the start.
                
    //             int16_t const sliceSize = Size - oldSt.HeadSoft;

    //             memcpy(this->Elements + oldSt.HeadSoft, elems, sliceSize * sizeof T);
    //             memcpy(this->Elements, elems + sliceSize, (count - sliceSize) * sizeof T);
    //         }

    //         while (this->State.HeadHard != oldSt.HeadSoft) DO_NOTHING();
    //         //  Wait for the hard head to advance to the soft head that this operation started on.
    //         //  This means all previous memory copies have finished.
            
    //         __atomic_store_n(&(this->State.HeadHard), newSt.HeadSoft, __ATOMIC_RELAXED);

    //         return true;
    //     }

    //     PopCookie TryBeginPop(int16_t max)
    //     {
    //         if (max <= 0)
    //             return {nullptr, 0};

    //         RbcState oldSt { __atomic_load_n(&(this->State.Value64), __ATOMIC_ACQUIRE) };
    //         int16_t count;

    //         if (oldSt.TailSoft == oldSt.HeadHard)
    //         {
    //             //  No contents.
                
    //             return {nullptr, 0};
    //         }
    //         else if (oldSt.TailSoft < oldSt.HeadHard)
    //         {
    //             //  No wrapping.
                
    //             count = oldSt.HeadHard - oldSt.TailSoft;
    //         }
    //         else
    //         {
    //             //  Yes wrapping.
                
    //             count = Size - oldSt.TailSoft;
    //         }

    //         if (count > max) count = max;

    //         RbcState newSt = oldSt;
    //         newSt.TailSoft = (newSt.TailSoft + count) % Size;

    //         if (!__atomic_compare_exchange_n(&(this->State.Value64), &oldSt, newSt, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
    //             return {nullptr, 0};
    //         //  This should retry if the soft tail remained the same and the hard head didn't move back.

    //         return {this->Elements + oldSt.TailSoft, count};
    //     }

    //     void EndPop(PopCookie const & cookie)
    //     {
    //         int16_t const cookieTail = cookie.Array - this->Elements;

    //         while (this->State.TailHard != cookieTail) DO_NOTHING();

    //         this->State.TailHard = cookieTail + cookie.Count;
    //     }

    // private:
    //     /*  Fields  */

    //     RbcState State;

    //     T Elements[Size];
    // };
}}
