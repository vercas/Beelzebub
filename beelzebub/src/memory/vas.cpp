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
    Vas class
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
        , MemoryAllocationOptions::Free), this->First);
    //  Blank memory region, for allocation.
}

/*  Operations  */

Handle Vas::Allocate(vaddr_t & vaddr, size_t pageCnt
    , MemoryFlags flags, MemoryContent content
    , MemoryAllocationOptions type, bool lock)
{
    if unlikely(this->First == nullptr)
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

    bool runPostCheck = false;

    if (this->ImplementsPreCheck())
        runPostCheck = this->PreCheck(lock, true);

    if likely(lock)
    {
        cookie = System::Interrupts::PushDisable();

        this->Lock.AcquireAsWriter();
    }

    if (runPostCheck)
    {
        res = this->PostCheck();

        if unlikely(!res.IsOkayResult())
            goto end;
    }

    // DEBUG_TERM << " <ALLOC> ";

    if (vaddr == nullvaddr)
    {
        //  Null vaddr means any address is accepted.

        // DEBUG_TERM << " <NULL VADDR> ";

        MemoryRegion * reg = this->First;

        do
        {
            // DEBUG_TERM << *reg;

            if (reg->Content == MemoryContent::Free && reg->GetPageCount() >= effectivePageCnt)
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

            //  Only other possibility is... This free region is either busy or
            //  not big enough!

            reg = reg->Next;
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

        MemoryRegion * reg = this->First;

        do
        {
            // DEBUG_TERM << reg;

            size_t regPageCnt = reg->GetPageCount();

            if (reg->Content != MemoryContent::Free || !rang.IsIn(reg->Range) || effectivePageCnt > regPageCnt)
            {
                reg = reg->Next;

                continue;
            }

            //  So it fits!

            if (regPageCnt == effectivePageCnt)
            {
                //  This region appears to be an exact fit. What a relief, and
                //  coincidence!
                
                // DEBUG_TERM << " <SNUG> ";

                if (reg->Prev == nullptr)
                {
                    //  This is the first region.

                    this->First = reg->Next;
                }
                else
                    reg->Prev->Next = reg->Next;

                if (reg->Next != nullptr)
                    reg->Next->Prev = reg->Prev;

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
                    && newReg->Type == type && newReg->Content == content)
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

                if unlikely(!res.IsOkayResult()) goto end;

                // DEBUG_TERM << *newReg;

                newReg->Prev = reg->Prev;
                newReg->Next = reg;

                goto end;
            }
            else if (rang.End == reg->Range.End)
            {
                //  Or at the end.

                // DEBUG_TERM << " <END> ";

                reg->Range.End = rang.Start;
                //  Free region's end is pulled back.

                MemoryRegion * newReg = this->Tree.Find<vaddr_t>(rang.End);

                if unlikely(0 != (type & MemoryAllocationOptions::UniquenessMask)
                    && newReg != nullptr && newReg->Flags == flags
                    && newReg->Type == type && newReg->Content == content)
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

                if unlikely(!res.IsOkayResult()) goto end;

                // DEBUG_TERM << *newReg;

                newReg->Prev = reg;
                newReg->Next = reg->Next;

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

                if unlikely(!res.IsOkayResult()) goto end;

                res = this->Tree.Insert(MemoryRegion(
                    rang.Start, rang.End, flags, content, type
                ), newBusy);

                if unlikely(!res.IsOkayResult()) goto end;

                newFree->Prev = reg;
                newFree->Next = reg->Next;
                reg->Next = newFree;

                if (newFree->Next != nullptr)
                    newFree->Next->Prev = newFree;
                //  Vital.

                newBusy->Prev = reg;
                newBusy->Next = newFree;

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

Handle Vas::Free(vaddr_t vaddr, size_t size, bool sparse, bool tolerant, bool lock)
{
    if unlikely(this->First == nullptr)
        return HandleResult::ObjectDisposed;

    Handle res = HandleResult::Okay;
    vaddr_t const endAddr = vaddr + size;
    vaddr_t continuation;
    MemoryRegion * reg;

    System::int_cookie_t cookie;

    bool runPostCheck = false;

    if (this->ImplementsPreCheck())
        runPostCheck = this->PreCheck(lock, false);

    if likely(lock)
    {
        cookie = System::Interrupts::PushDisable();

        this->Lock.AcquireAsWriter();
    }

    if (runPostCheck)
    {
        res = this->PostCheck();

        if unlikely(!res.IsOkayResult())
            goto end;
    }

    /*
    There are a few possible cases here:
        1.  The requested area covers more than one descriptor.
            In this case, as much as possible is removed and the function is
            re-entered.
        2.  Area in in the middle of a descriptor (accounting for guards).
            A free descriptor in the middle and a near-copy busy descriptor at
            the end are created.
        3.  Area is just at the beginning of a descriptor (acc. for lower guard).
            Descriptor is shrunk towards the end, and previous descriptor is
            extended if free; otherwise free descriptor is allocated over the
            area.
        4.  Area is just at the end of a desc. (acc. for high guard).
            Descriptor is shrunk towards the start, next descriptor is
            extended if it's free; ditto.
        5.  Area covers entire descriptor (acc. for guards).
            The descriptor is removed and adjacent free descriptors are extended
            or even merged.
     */
    
    continuation = nullvaddr;

    reg = this->Tree.Find<vaddr_t>(vaddr);

    if (endAddr > reg->Range.End)
    {
        if (sparse)
        {
            continuation = reg->Range.End;

            size -= (continuation - vaddr);
        }
        else
        {
            res = HandleResult::ArgumentOutOfRange;

            goto end;
        }
    }

    if (reg->Content != MemoryContent::Free)
    {
        if (0 != (reg->Type & MemoryAllocationOptions::Permanent))
        {
            //  Permanent allocations cannot be freed.

            res = HandleResult::Failed;

            goto end;
        }

        bool const meetsStart = reg->Range.Start == vaddr   || (0 != (reg->Type & MemoryAllocationOptions::GuardLow ) && reg->Range.Start == vaddr   - PageSize);
        bool const meetsEnd   = reg->Range.End   <= endAddr || (0 != (reg->Type & MemoryAllocationOptions::GuardHigh) && reg->Range.End   == endAddr + PageSize);
        //  End is met even if the requested range is beyond this descriptor.

        //  Now there are only four major cases to handle.

        if (meetsStart)
        {
            bool const prevFree = reg->Prev != nullptr && reg->Prev->Content == MemoryContent::Free;
            //  Previous free region is adjacent!

            if (meetsEnd)
            {
                //  So the deallocation spans an entire descriptor (or more), kool.
            
                bool const nextFree = reg->Next != nullptr && reg->Next->Content == MemoryContent::Free;
                //  Next free region is adjacent!

                if (prevFree)
                {
                    vaddr_t newEnd;
                    MemoryRegion * next;

                    if (nextFree)
                    {
                        //  So this descriptor, to be removed entirely, is surrounded
                        //  by free descriptors. This means the deallocated descriptor and
                        //  the next free one are removed and the previous free one is extended
                        //  to cover both.

                        newEnd = reg->Next->Range.End;
                        next = reg->Next->Next;
                        reg = reg->Prev;    //  Variable is repurposed.

                        res = this->Tree.Remove<vaddr_t>(vaddr);
                        if unlikely(!res.IsOkayResult()) goto end;

                        res = this->Tree.Remove<vaddr_t>(newEnd - 1);
                        if unlikely(!res.IsOkayResult()) goto end;
                    }
                    else
                    {
                        //  Next one is not free, but previous one is. Means the
                        //  deallocated descriptor is removed an the previous one
                        //  is extended to cover it.

                        newEnd = reg->Range.End;
                        next = reg->Next;
                        reg = reg->Prev;    //  Variable is repurposed.

                        res = this->Tree.Remove<vaddr_t>(vaddr);
                        if unlikely(!res.IsOkayResult()) goto end;
                    }

                    //  Now there's room to expand the previous descriptor, which is in `reg`.

                    reg->Range.End = newEnd;

                    reg->Next = next;
                    if (next != nullptr)
                        next->Prev = reg;
                    //  Patch linkage.
                }
                else
                {
                    if (nextFree)
                    {
                        //  Next one is free, but previous one is NOT. Next descriptor
                        //  is extended to cover the deallocated one, which is removed.

                        vaddr_t const newStart = reg->Range.Start;
                        MemoryRegion * const prev = reg->Prev;
                        reg = reg->Next;    //  Variable is repurposed.

                        res = this->Tree.Remove<vaddr_t>(vaddr);
                        if unlikely(!res.IsOkayResult()) goto end;

                        //  Now there's room to expand the next descriptor.

                        reg->Range.Start = newStart;

                        reg->Prev = prev;
                        if (prev != nullptr)
                            prev->Next = reg;
                    }
                    else
                    {
                        //  This descriptor is surrounded by non-free ones.
                        //  Therefore, no removal is necessary, just repurposing.

                        reg->Flags = MemoryFlags::Writable | MemoryFlags::Executable;
                        reg->Content = MemoryContent::Free;
                        reg->Type = MemoryAllocationOptions::Free;

                        //  And linkage stays intact.
                    }
                }
            }
            else
            {
                //  The deallocation starts at the beginning of the descriptor
                //  but stops before the end of it.

                vaddr_t const oldStart = reg->Range.Start;

                reg->Range.Start = endAddr;
                reg->Type &= ~MemoryAllocationOptions::GuardLow;
                //  Busy descriptor is shrunk and lower guard is gone, if any.

                if (prevFree)
                {
                    //  Previous descriptor is free means it can be extended to
                    //  cover the deallocated parts.

                    reg->Prev->Range.End = endAddr;

                    //  Linkage is intact.
                }
                else
                {
                    //  Otherwise one ought to be allocated.

                    MemoryRegion * newFree = nullptr;
                    res = this->Tree.Insert(MemoryRegion(oldStart, endAddr
                        , MemoryFlags::Writable | MemoryFlags::Executable
                        , MemoryContent::Free
                        , MemoryAllocationOptions::Free), newFree);

                    if unlikely(!res.IsOkayResult()) goto end;

                    newFree->Prev = reg->Prev;
                    newFree->Next = reg;

                    if (reg->Prev != nullptr)
                        reg->Prev->Next = newFree;

                    reg->Prev = newFree;
                    //  Patch linkage.
                }
            }
        }
        else
        {
            if (meetsEnd)
            {
                //  The deallocation starts after the beginning of the descriptor
                //  but stops at the end of it.

                bool const nextFree = reg->Next != nullptr && reg->Next->Content == MemoryContent::Free;
                //  Next free region is adjacent!

                vaddr_t const oldEnd = reg->Range.End;

                reg->Range.End = vaddr;
                reg->Type &= ~MemoryAllocationOptions::GuardHigh;
                //  Busy descriptor is shrunk and higher guard is gone, if any.

                if (nextFree)
                {
                    //  Next descriptor is free means it can be extended to
                    //  cover the deallocated parts.

                    reg->Prev->Range.Start = vaddr;

                    //  Linkage is intact.
                }
                else
                {
                    //  Otherwise one ought to be allocated.

                    MemoryRegion * newFree = nullptr;
                    res = this->Tree.Insert(MemoryRegion(vaddr, oldEnd
                        , MemoryFlags::Writable | MemoryFlags::Executable
                        , MemoryContent::Free
                        , MemoryAllocationOptions::Free), newFree);

                    if unlikely(!res.IsOkayResult()) goto end;

                    newFree->Prev = reg;
                    newFree->Next = reg->Next;

                    if (reg->Next != nullptr)
                        reg->Next->Prev = newFree;

                    reg->Next = newFree;
                    //  Patch linkage.
                }
            }
            else
            {
                //  The deallocation is within a descriptor and doesn't include
                //  either end. Means that a free region must be created in the
                //  middle & a copy of the descriptor w/o lower guard for the
                //  end. Existing descriptor stays at the beginning.

                vaddr_t const oldEnd = reg->Range.End;
                MemoryAllocationOptions const oldHighGuard = reg->Type & MemoryAllocationOptions::GuardHigh;

                reg->Range.End = vaddr;

                MemoryRegion * newFree = nullptr,  * newBusy = nullptr;

                res = this->Tree.Insert(MemoryRegion(
                    vaddr, endAddr
                    , MemoryFlags::Writable | MemoryFlags::Executable
                    , MemoryContent::Free
                    , MemoryAllocationOptions::Free
                ), newFree);

                if unlikely(!res.IsOkayResult()) goto end;

                res = this->Tree.Insert(MemoryRegion(
                    endAddr, oldEnd, reg->Flags, reg->Content
                    , (reg->Type & ~MemoryAllocationOptions::GuardLow) | oldHighGuard
                ), newBusy);

                if unlikely(!res.IsOkayResult()) goto end;

                newFree->Prev = reg;
                newFree->Next = newBusy;
                newBusy->Prev = newFree;
                newBusy->Next = reg->Next;
                reg->Next = newFree;

                if (newBusy->Next != nullptr)
                    newBusy->Next->Prev = newBusy;
                //  And linkage is patched.
            }
        }
    }
    else if unlikely(!tolerant)
    {
        res = HandleResult::PageFree;

        goto end;
    }

    if (continuation != nullvaddr)
        res = this->Free(continuation, size, true, tolerant, false);

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
    if unlikely(this->First == nullptr)
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

/*  Support  */

Handle Vas::AllocateNode(AvlTree<MemoryRegion>::Node * & node)
{
    return this->Alloc.AllocateObject(node);
}

Handle Vas::RemoveNode(AvlTree<MemoryRegion>::Node * const node)
{
    return this->Alloc.DeallocateObject(node);
}

bool Vas::PreCheck(bool & lock, bool alloc)
{
    return false;
}

Handle Vas::PostCheck()
{
    return HandleResult::Okay;
}

/*************
    OTHERS
*************/

namespace Beelzebub { namespace Utils
{
    template<>
    Handle AvlTree<MemoryRegion>::AllocateNode(AvlTree<MemoryRegion>::Node * & node, void * cookie)
    {
        return (reinterpret_cast<Vas *>(cookie))->AllocateNode(node);
    }

    template<>
    Handle AvlTree<MemoryRegion>::RemoveNode(AvlTree<MemoryRegion>::Node * const node, void * cookie)
    {
        return (reinterpret_cast<Vas *>(cookie))->RemoveNode(node);
    }
}}
