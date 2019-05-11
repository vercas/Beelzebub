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

#include <beel/enums.kernel.h>
#include <beel/terminals/base.hpp>

using namespace Beelzebub;

__ENUM_TO_STRING_IMPL(FrameSize, __ENUM_FRAMESIZE)
__ENUM_TO_STRING_IMPL(MemoryContent, __ENUM_MEMORYCONTENT)
__ENUM_TO_STRING_IMPL(ExceptionType, __ENUM_EXCEPTIONTYPE)
__ENUM_TO_STRING_IMPL(SchedulerStatus, __ENUM_SCHEDULERSTATUS)

bool Beelzebub::MemoryContentsMergeable(MemoryContent a, MemoryContent b)
{
    if (a == b)
    {
        switch (a)
        {
        case MemoryContent::Generic:
        case MemoryContent::CpuDatas:
        case MemoryContent::VmmBootstrap:
        case MemoryContent::Free:
            //  Descriptors with these contents can be merged safely.
            return true;

        default:
            //  All the others have an important unique identity.
            return false;
        }
    }

    return false;
}
