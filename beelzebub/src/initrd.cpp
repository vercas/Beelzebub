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

#include "initrd.hpp"
#include <beel/utils/tar.hpp>

#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

static TarHeader const * TarStart = nullptr;
static void * TarEnd = nullptr;

typedef HandlePointer<TarHeader, HandleType::InitRdFile,      9> InitRdFileHandle;
typedef HandlePointer<TarHeader, HandleType::InitRdDirectory, 9> InitRdDirectoryHandle;

/*******************
    InitRd class
*******************/

/*  Statics  */

bool InitRd::Loaded = false;

/*  Methods  */

Handle InitRd::Initialize(vaddr_t vaddr, vsize_t size)
{
    TarStart = reinterpret_cast<TarHeader const *>(vaddr.Pointer);
    TarEnd = const_cast<void *>((vaddr + size).Pointer);

    for (TarHeader const * cursor = TarStart; cursor != nullptr; cursor = cursor->GetNext())
    {
        if (vaddr_t(cursor + 1) > vaddr + size)
            return HandleResult::IntegrityFailure;
        //  It appears the tape archive is malformed.

        if (reinterpret_cast<uintptr_t>(cursor) % sizeof(TarHeader) != 0)
            return HandleResult::AlignmentFailure;
        //  All TAR headers must be aligned in the InitRD.
    }

    Loaded = true;

    return HandleResult::Okay;
}

/*  Items  */

Handle InitRd::FindItem(char const * name)
{
    if (TarStart == nullptr || TarEnd == nullptr)
        return HandleResult::UnsupportedOperation;

    for (TarHeader const * cursor = TarStart; cursor != nullptr; cursor = cursor->GetNext())
    {
        if (cursor->CompareName(name) == 0)
        {
            if likely(cursor->TypeFlag == TarHeaderType::File)
                return InitRdFileHandle(cursor, cursor->GetChecksum()).ToHandle(true);
            else if (cursor->TypeFlag == TarHeaderType::Directory)
                return InitRdDirectoryHandle(cursor, cursor->GetChecksum()).ToHandle(true);
            //  Only files and directories are supported.

            return HandleResult::UnsupportedOperation;
        }
    }

    return HandleResult::NotFound;
}

FileBoundaries InitRd::GetFileBoundaries(Handle file)
{
    if unlikely(!file.IsType(HandleType::InitRdFile))
        return { nullvaddr, vsize_t(0), vsize_t(0) };

    InitRdFileHandle hptr = file;
    TarHeader const * thdr = hptr.GetPointer();

    if (hptr.GetData() != (thdr->GetChecksum() & InitRdFileHandle::DataMask))
        return { nullvaddr, vsize_t(0), vsize_t(0) };

    vsize_t const size { thdr->GetSize() };

    return { vaddr_t(thdr + 1), size, RoundUp(size, SizeOf<TarHeader>) };
}
