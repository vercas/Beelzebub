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

#include <execution/thread.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

/******************
    Thread class
*******************/

/*  Constructors  */

Thread::Thread(uint16_t id, Process * const owner)
    : ReferenceCounted()
    , ThreadBase()
    , Id(id)
    , Owner(owner)
    , Status(ThreadStatus::Constructing)
    , Name(nullptr)
    , KernelStackTop()
    , KernelStackBottom()
    , KernelStackPointer()
    , State()
    , ExtendedState(nullptr)
    , Previous(nullptr)
    , Next(nullptr)
    , EntryPoint()
{

}

/*  Destructors  */

Thread::~Thread()
{
    assert(this->Owner == nullptr);
}

void Thread::ReleaseMemory()
{
    if (this->Owner != nullptr)
        this->Owner = nullptr;
    else
        assert(this->Status == ThreadStatus::Constructing);
}

/*  Operations  */

void Thread::SetActive()
{
    assert(this->Status == ThreadStatus::Constructing);
    assert(this->Id != 0);
    assert(this->KernelStackTop != 0);
    assert(this->KernelStackBottom != 0);
    assert(this->Owner != nullptr);

    // this->PreSetActive();
    this->Status = ThreadStatus::Active;
}

void Thread::SetName(char const * name)
{
    assert(this->Status == ThreadStatus::Constructing);
    assert(this->Name == nullptr);
    assert(name != nullptr);

    this->Name = name;
}

/*  Stack  */

void Thread::SetKernelStack(uintptr_t top, uintptr_t bottom)
{
    assert(this->Status == ThreadStatus::Constructing);
    assert(this->KernelStackTop == 0);
    assert(this->KernelStackBottom == 0);
    assert(top != 0);
    assert(bottom != 0);
    assert(top % 16 == 0);      //  Stack alignment.
    assert(bottom % 16 == 0);   //  Stack alignment.

    this->KernelStackTop = top;
    this->KernelStackBottom = bottom;
}

/*  Linkage  */

Handle Thread::IntroduceNext(Thread * const other)
{
    Thread * oldNext = this->Next;

    Thread * current = oldNext;

    do
    {
        if (current == other)
            return HandleResult::ThreadAlreadyLinked;

        current = current->Next;
    }
    while (current != oldNext);

    this->Next = other;
    oldNext->Previous = other;

    other->Previous = this;
    other->Next = oldNext;

    return HandleResult::Okay;
}
