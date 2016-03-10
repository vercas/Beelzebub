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

#ifdef __BEELZEBUB__TEST_EXCP

#include <tests/exceptions.hpp>
#include <exceptions.hpp>
#include <system/cpu.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;

__startup void TestNullDereference(uintptr_t volatile * const testPtr)
{
    uintptr_t volatile * volatile testNullPtr = reinterpret_cast<uintptr_t volatile *>(testPtr - testPtr);

    __try
    {
        uintptr_t thisShouldFail = *testNullPtr;

        ASSERT(false,
            "(( Test value that should've failed: %Xp ))%n",
            thisShouldFail);
    }
    __catch (x)
    {
        ASSERT(x->Type == ExceptionType::NullReference
            , "Exception type should be %up, not %up!"
            , ExceptionType::NullReference, x->Type);
    }
}

void TestExceptions()
{
    uintptr_t volatile * volatile testPtr = reinterpret_cast<uintptr_t volatile *>(0x1234567);
    
    for (size_t i = 0; i < 100; ++i)
        __try
        {
            uintptr_t thisShouldFail = *testPtr;

            ASSERT(false,
                "(( Test value that should've failed: %Xp ))%n",
                thisShouldFail);
        }
        __catch (x)
        {
            ASSERT(x->Type == ExceptionType::MemoryAccessViolation
                , "(Iteration %us) Exception type should be %up, not %up!"
                , i, ExceptionType::MemoryAccessViolation, x->Type);

            ASSERT(x->MemoryAccessViolation.Address == testPtr
                , "(Iteration %us) Memory violation address should be %Xp, not %Xp!"
                , i, testPtr, x->MemoryAccessViolation.Address);

            ASSERT(x->MemoryAccessViolation.PhysicalAddress == nullpaddr
                , "(Iteration %us) Memory violation physical address should be null, not %Xp!"
                , i, x->MemoryAccessViolation.PhysicalAddress);

            ASSERT(x->MemoryAccessViolation.PageFlags == MemoryLocationFlags::None
                , "(Iteration %us) Memory violation page flags should be %X2, not %X2!"
                , i, MemoryLocationFlags::None, x->MemoryAccessViolation.PageFlags);

            ASSERT(x->MemoryAccessViolation.AccessType == MemoryAccessType::Read
                , "(Iteration %us) Memory violation access type should be %X1, not %X1!"
                , i, MemoryAccessType::Read, x->MemoryAccessViolation.AccessType);
        }

    TestNullDereference(testPtr);
}

#endif
