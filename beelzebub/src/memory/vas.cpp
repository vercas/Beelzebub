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

#include <memory/vas.hpp>
#include <system/interrupts.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Utils;

/****************
    VAS class
****************/

/*  Constructors  */

Handle Vas::Initialize(vaddr_t start, vaddr_t end
    , AcquirePoolFunc acquirer, EnlargePoolFunc enlarger, ReleasePoolFunc releaser
    , PoolReleaseOptions const releaseOptions
    , size_t const quota)
{
    new (&(this->Alloc)) ObjectAllocator(
        sizeof(*(this->Tree.Root)), __alignof(*(this->Tree.Root)),
        acquirer, enlarger, releaser, releaseOptions, SIZE_MAX, quota);

    return this->Tree.Insert(MemoryRegion(start, end
        , MemoryFlags::Writable | MemoryFlags::Executable
        , MemoryContent::Free
        , MemoryAllocationOptions::Free), this->FirstFree);
    //  Blank memory region, for allocation.
}

/*  Operations  */

Handle Vas::Allocate(vaddr_t & vaddr, size_t pageCnt
    , MemoryFlags flags, MemoryContent content
    , MemoryAllocationOptions type, bool lock)
{
    if unlikely(this->FirstFree == nullptr)
        return HandleResult::ObjectDisposed;

    Handle res = HandleResult::Okay;

    size_t lowOffset = 0, highOffset = 0;
    size_t effectivePageCnt = pageCnt;

    if (0 != (type & MemoryAllocationOptions::GuardLow))
    {
        lowOffset = PageSize;
        ++effectivePageCnt;
    }

    if (0 != (type & MemoryAllocationOptions::GuardHigh))
    {
        highOffset = PageSize;
        ++effectivePageCnt;
    }

    System::int_cookie_t cookie;
    //  When locking, the code shan't be interrupted!

    if likely(lock)
    {
        cookie = System::Interrupts::PushDisable();

        this->Lock.AcquireAsWriter();
    }

    // DEBUG_TERM << " <ALLOC> ";

    if (vaddr == nullvaddr)
    {
        //  Null vaddr means any address is accepted.

        // DEBUG_TERM << " <NULL VADDR> ";

        MemoryRegion * reg = this->FirstFree;

        do
        {
            // DEBUG_TERM << *reg;

            if (reg->GetPageCount() >= effectivePageCnt)
            {
                //  Oh yush, this region contains more than enough pages!

                // DEBUG_TERM << " <BIGGER> ";

                //  TODO: Implement (K)ASLR here.
                //  For now, it allocates at the end of the space, so it doesn't
                //  slow down future allocations in any meaningful way.

                vaddr = reg->Range.End - effectivePageCnt * PageSize + lowOffset;
                //  New region begins where the free one ends, after shrinking.

                res = this->Allocate(vaddr, pageCnt, flags, content, type, false);

                goto end;
            }

            //  Only other possibility is... This free region isn't big enough!

            reg = reg->NextFree;
        } while (reg != nullptr);

        //  Reaching this point means there is no space to spare!

        res = HandleResult::OutOfMemory;
    }
    else
    {
        //  A non-null vaddr means a specific address is required. Guard would
        //  go before the requested address.

        // DEBUG_TERM << " <VADDR " << (void *)vaddr << "> ";

        MemoryRange rang {vaddr - lowOffset, vaddr + pageCnt * PageSize + highOffset};
        //  This will be the exact range of the allocation.

        MemoryRegion * reg = this->FirstFree;

        do
        {
            // DEBUG_TERM << reg;

            size_t regPageCnt = reg->GetPageCount();

            if (!rang.IsIn(reg->Range) || effectivePageCnt > regPageCnt)
            {
                reg = reg->NextFree;

                continue;
            }

            //  So it fits!

            if (regPageCnt == effectivePageCnt)
            {
                //  This region appears to be an exact fit. What a relief, and
                //  coincidence!
                
                // DEBUG_TERM << " <SNUG> ";

                if (reg->PrevFree == nullptr)
                {
                    //  This is the first region.

                    this->FirstFree = reg->NextFree;
                }
                else
                    reg->PrevFree->NextFree = reg->NextFree;

                if (reg->NextFree != nullptr)
                    reg->NextFree->PrevFree = reg->PrevFree;

                //  Now linkage is patched, and the region can be merrily
                //  converted!

                reg->Flags = flags;
                reg->Type = type;

                vaddr = reg->Range.Start;

                goto end;
                //  Done!
            }

            //  Okay, so it's not a perfect fit... Meh. Split.
            //  There are 3 possibilities: the desired range is at the start of
            //  the region, at the end, or in the middle...

            if (rang.Start == reg->Range.Start)
            {
                //  So it sits at the very start.

                // DEBUG_TERM << " <START> ";

                reg->Range.Start = rang.End;
                //  Free region's start is pushed forward.

                MemoryRegion * newReg = this->Tree.Find<vaddr_t>(rang.Start - 1);

                if unlikely(0 != (type & MemoryAllocationOptions::UniquenessMask)
                    && newReg != nullptr && newReg->Flags == flags
                    && newReg->Type == type)
                {
                    //  The region must be a perfect match and have no uniqueness
                    //  features in order to "merge".

                    newReg->Range.End = rang.End;
                    //  Existing region's end is pushed forward to cover the
                    //  requested region.

                    goto end;
                }

                res = this->Tree.Insert(MemoryRegion(
                    rang, flags, content, type
                ), newReg);

                if unlikely(!res.IsOkayResult())
                    goto end;

                // DEBUG_TERM << *newReg;

                newReg->PrevFree = reg->PrevFree;
                newReg->NextFree = reg;

                goto end;
            }
            else if (rang.End == reg->Range.End)
            {
                //  Or at the end.

                // DEBUG_TERM << " <END> ";

                reg->Range.End = rang.Start;
                //  Free region's end is pulled back.

                MemoryRegion * newReg = this->Tree.Find<vaddr_t>(rang.End + 1);

                if unlikely(0 != (type & MemoryAllocationOptions::UniquenessMask)
                    && newReg != nullptr && newReg->Flags == flags
                    && newReg->Type == type)
                {
                    //  The region must be a perfect match and have no uniqueness
                    //  features in order to "merge".

                    newReg->Range.Start = rang.Start;
                    //  Existing region's start is pulled back to cover the
                    //  requested region.

                    goto end;
                }

                res = this->Tree.Insert(MemoryRegion(
                    rang, flags, content, type
                ), newReg);

                if unlikely(!res.IsOkayResult())
                    goto end;

                // DEBUG_TERM << *newReg;

                newReg->PrevFree = reg;
                newReg->NextFree = reg->NextFree;

                goto end;
            }
            else
            {
                //  Well, it's in the middle.

                // DEBUG_TERM << " <MID> ";

                vaddr_t const oldEnd = reg->Range.End;
                reg->Range.End = rang.Start;
                //  Pulls the end of the free region to become the left free region.

                MemoryRegion * newFree = nullptr,  * newBusy = nullptr;

                res = this->Tree.Insert(MemoryRegion(
                    rang.End, oldEnd
                    , MemoryFlags::Writable | MemoryFlags::Executable
                    , MemoryContent::Free
                    , MemoryAllocationOptions::Free
                ), newFree);

                if unlikely(!res.IsOkayResult())
                    goto end;

                res = this->Tree.Insert(MemoryRegion(
                    rang.Start, rang.End, flags, content, type
                ), newBusy);

                if unlikely(!res.IsOkayResult())
                    goto end;

                newFree->PrevFree = reg;
                newFree->NextFree = reg->NextFree;
                reg->NextFree = newFree;

                if (newFree->NextFree != nullptr)
                    newFree->NextFree->PrevFree = newFree;
                //  Vital.

                newBusy->PrevFree = reg;
                newBusy->NextFree = newFree;

                // DEBUG_TERM << "{" << reg << newBusy << " " << newFree << "}";

                goto end;
            }
        } while (reg != nullptr);

        //  Reaching this point means the requested region is taken!

        res = HandleResult::OutOfMemory;
    }

end:
    if likely(lock)
    {
        this->Lock.ReleaseAsWriter();

        System::Interrupts::RestoreState(cookie);
    }

    return res;
}

Handle Vas::Modify(vaddr_t vaddr, size_t pageCnt
    , MemoryFlags flags, bool lock)
{
    if unlikely(this->FirstFree == nullptr)
        return HandleResult::ObjectDisposed;

    Handle res = HandleResult::Okay;

    System::int_cookie_t cookie;

    if likely(lock)
    {
        cookie = System::Interrupts::PushDisable();

        this->Lock.AcquireAsWriter();
    }

    //  So, this is gonna suck a bit. There are... Lots of options.

end:
    if likely(lock)
    {
        this->Lock.ReleaseAsWriter();

        System::Interrupts::RestoreState(cookie);
    }

    return res;
}

MemoryRegion * Vas::FindRegion(vaddr_t vaddr)
{
    return this->Tree.Find<vaddr_t>(vaddr);
}

/*************
    OTHERS
*************/

namespace Beelzebub { namespace Utils
{
    template<>
    Handle AvlTree<MemoryRegion>::AllocateNode(AvlTree<MemoryRegion>::Node * & node, void * cookie)
    {
        return (reinterpret_cast<ObjectAllocator *>(cookie))->AllocateObject(node);
    }

    template<>
    Handle AvlTree<MemoryRegion>::RemoveNode(AvlTree<MemoryRegion>::Node * const node, void * cookie)
    {
        return (reinterpret_cast<ObjectAllocator *>(cookie))->DeallocateObject(node);
    }
}}
