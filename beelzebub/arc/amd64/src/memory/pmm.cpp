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

#include <memory/pmm.hpp>
#include <memory/pmm.arc.hpp>
#include <system/cpu.hpp>
#include <kernel.hpp>

#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::System;

/**************************
    Utilitary functions
**************************/

static __hot void SplitLargeFrame(LargeFrameDescriptor * desc, psize_t cnt = LargeFrameDescriptor::SubDescriptorsCount)
{
    auto extra = new (desc->GetExtras()) SplitFrameExtra();

    size_t i = 1;
    uint16_t * lastNextIndex = &(extra->NextFree);

    for (/* nothing */; i <= cnt; ++i)
    {
        *lastNextIndex = i;
        //  Construct the pseudo-stack of free small frames.

        auto sDesc = new (desc->SubDescriptors + i) SmallFrameDescriptor();

        lastNextIndex = &(sDesc->NextIndex);
    }

    *lastNextIndex = SmallFrameDescriptor::NullIndex;

    for (/* nothing */; i <= LargeFrameDescriptor::SubDescriptorsCount; ++i)
        (new (desc->SubDescriptors + i) SmallFrameDescriptor())->Status = FrameStatus::Reserved;

    desc->Status = FrameStatus::Split;
    desc->GetExtras()->FreeCount = cnt;
}

/*******************
    PmmArc class
*******************/

/*  Statics  */

paddr_t PmmArc::TempSpaceLimit = nullpaddr;

FrameAllocationSpace * PmmArc::AllocationSpace = nullptr;
FrameAllocator * PmmArc::MainAllocator = nullptr;

/****************
    Pmm class
****************/

/*  Frame operations  */

paddr_t Pmm::AllocateFrame(FrameSize size, AddressMagnitude magn, uint32_t refCnt)
{
    //  TODO: NUMA selection of some sorts, maybe based on process?

    return PmmArc::MainAllocator->AllocateFrame(size, magn, refCnt);
}

Handle Pmm::FreeFrame(paddr_t addr, bool ignoreRefCnt)
{
    uint32_t dummy;

    return PmmArc::MainAllocator->Mingle(addr, dummy, 0, ignoreRefCnt);
}

Handle Pmm::ReserveRange(paddr_t start, size_t size, bool includeBusy)
{
    return PmmArc::MainAllocator->ReserveRange(start, size, includeBusy);
}

Handle Pmm::AdjustReferenceCount(paddr_t addr, uint32_t & newCnt, int32_t diff)
{
    if unlikely(addr == nullpaddr)
        return HandleResult::ArgumentOutOfRange;

    return PmmArc::MainAllocator->Mingle(addr, newCnt, diff, false);
}

/*******************
    PmmArc class
*******************/

/*  Initialization  */

Handle PmmArc::CreateAllocationSpace(paddr_t start, paddr_t end)
{
    // MSG_("&& Space %XP-%XP (%XS) ", start, end, end - start);

    if ((end - start) < (2 << 21))
    {
        //  Not yet!
        // MSG_("too small &&%n");

        return HandleResult::Failed;
    }

    if (PmmArc::MainAllocator != nullptr)
    {
        //  An allocator exists, therefore the temporary space is used to allocate
        //  the next allocation space.
        // MSG_("preppended &&%n");

        if (reinterpret_cast<paddr_t>(PmmArc::AllocationSpace + 2) > PmmArc::TempSpaceLimit)
        {
            //  So, this new allocation space structure would go beyond the
            //  limit of the temporary space... Not good.

            PmmArc::AllocationSpace = reinterpret_cast<FrameAllocationSpace *>(start);
            PmmArc::TempSpaceLimit = (start += PageSize);
            //  The sensible solution here is to make a new temporary space!
            //  Oh, and push forward the start of this allocation space.
        }
        else
            ++PmmArc::AllocationSpace;

        PmmArc::MainAllocator->AppendAllocationSpace(new (PmmArc::AllocationSpace) FrameAllocationSpace(start, end));

        return HandleResult::Okay;
    }
    else
    {
        //  Some bootstrapping is necessary here. The first page of the first
        //  given space is used to hold all the structures.
        // MSG_("bootstraps &&%n");

        PmmArc::MainAllocator = reinterpret_cast<FrameAllocator *>(start);
        PmmArc::AllocationSpace = reinterpret_cast<FrameAllocationSpace *>(reinterpret_cast<uintptr_t>(PmmArc::MainAllocator + 1));

        PmmArc::TempSpaceLimit = start + PageSize;

        new (PmmArc::MainAllocator) FrameAllocator(new (PmmArc::AllocationSpace) FrameAllocationSpace(start + PageSize, end));

        return HandleResult::Okay;
    }
}

/*  Relocation  */

void PmmArc::Remap(FrameAllocator * & alloc, vaddr_t const oldVaddr, vaddr_t const newVaddr)
{
    vaddr_t const allocAddr   = reinterpret_cast<vaddr_t>(alloc);
    vaddr_t const oldVaddrEnd = oldVaddr + PageSize;

    if (allocAddr >= oldVaddr && allocAddr < oldVaddrEnd)
        alloc = reinterpret_cast<FrameAllocator *>((allocAddr - oldVaddr) + newVaddr);

    withLock (alloc->ChainLock)
    {
        vaddr_t const firstAddr   = reinterpret_cast<vaddr_t>(alloc->FirstSpace);
        vaddr_t const lastAddr    = reinterpret_cast<vaddr_t>(alloc->LastSpace);

        if (firstAddr != nullvaddr && firstAddr >= oldVaddr && firstAddr < oldVaddrEnd)
            alloc->FirstSpace = reinterpret_cast<FrameAllocationSpace *>((firstAddr - oldVaddr) + newVaddr);
        if (lastAddr != nullvaddr && lastAddr >= oldVaddr && lastAddr < oldVaddrEnd)
            alloc->LastSpace = reinterpret_cast<FrameAllocationSpace *>((lastAddr - oldVaddr) + newVaddr);

        //  Now makin' sure all the pointers are aligned.

        FrameAllocationSpace * cur = alloc->LastSpace;

        while (cur != nullptr)
        {
            vaddr_t const nextAddr = reinterpret_cast<vaddr_t>(cur->Next);
            vaddr_t const prevAddr = reinterpret_cast<vaddr_t>(cur->Previous);

            if (nextAddr != nullvaddr && nextAddr >= oldVaddr && nextAddr < oldVaddrEnd)
                cur->Next = reinterpret_cast<FrameAllocationSpace *>((nextAddr - oldVaddr) + newVaddr);

            if (prevAddr != nullvaddr && prevAddr >= oldVaddr && prevAddr < oldVaddrEnd)
                cur->Previous = reinterpret_cast<FrameAllocationSpace *>((prevAddr - oldVaddr) + newVaddr);

            cur = cur->Previous;
        }
    }
}

/*********************************
    FrameAllocationSpace class
*********************************/

/*  Constructor(s)  */

FrameAllocationSpace::FrameAllocationSpace(paddr_t phys_start, paddr_t phys_end)
    : MemoryStart( phys_start)
    , MemoryEnd(phys_end)
    , Size(phys_end - phys_start)
    , ReservedSize(0)
    , Map(reinterpret_cast<LargeFrameDescriptor *>(phys_start))
    , LargeLocker()
    , SplitLocker()
    , LargeFree(LargeFrameDescriptor::NullIndex)
    , SplitFree(LargeFrameDescriptor::NullIndex)
    , Next(nullptr)
    , Previous(nullptr)
{
    paddr_t const algn_end = RoundDown(phys_end, 2 << 20);
    //  Round down the end to a two-megabyte address.

    paddr_t ctrl_end = phys_start, alloc_start = algn_end;
    size_t frameCount = 0;

    //  Now, progressively attempt to push forward the end of the control structures
    //  and pull back the beginning of the allocation area in steps of decrementing
    //  powers of 2. Thus, after 32 iterations, the resulting positions allow
    //  ~almost~ as much of the space as possible to be used. This could be tuned
    //  later to attempt to place the control structures at the end too.

    for (psize_t i = 1UL << 31; i > 0; i >>= 1)
    {
        paddr_t new_ctrl_end = ctrl_end + i * sizeof(LargeFrameDescriptor)
            , new_alloc_start = alloc_start - (i << 21);

        if (new_alloc_start > alloc_start)
            continue;
        //  Underflow.

        if (new_ctrl_end <= new_alloc_start)
        {
            //  This fits!

            frameCount += i;
            ctrl_end = new_ctrl_end;
            alloc_start = new_alloc_start;

            if unlikely(new_ctrl_end == new_alloc_start)
                break;
            //  If this is a precise fit, no need to loop further.
        }
    }

    ASSERTX(frameCount > 0
        , "Failed to squeeze any frames in allocation space.")
        (phys_start)(phys_end)XEND;

    ASSERTX(alloc_start + (frameCount << 21) == algn_end
        , "Sanity failure: %XP + %Xs != %XP??"
        , alloc_start, frameCount << 21, algn_end)XEND;
    //  Just checking sanity.

    this->AllocationStart = alloc_start;
    this->AllocationEnd   = algn_end;
    this->AllocableSize   = algn_end - alloc_start;

    this->ControlAreaSize = frameCount * sizeof(LargeFrameDescriptor);
    this->FreeSize        = this->AllocableSize;

    this->LargeFrameCount = frameCount;

    uint32_t * lastNextIndex = &(this->LargeFree);

    for (size_t i = 0; i < frameCount; ++i)
    {
        *lastNextIndex = i;
        //  Construct the pseudo-stack of free large frames.

        auto desc = new (this->Map + i) LargeFrameDescriptor();
        desc->SubDescriptors = reinterpret_cast<SmallFrameDescriptor *>(alloc_start + (i << 21));

        lastNextIndex = &(desc->NextIndex);
    }

    *lastNextIndex = LargeFrameDescriptor::NullIndex;

    //  So, it's almost ready. There may be a stray frame at the end that can be
    //  used partially. Enough space for another descriptor is also necessary.

    if unlikely(alloc_start - ctrl_end >= sizeof(LargeFrameDescriptor) && phys_end - algn_end >= (3 * PageSize))
    {
        this->AllocationEnd = phys_end;
        this->AllocableSize = phys_end - alloc_start;

        this->ControlAreaSize += sizeof(LargeFrameDescriptor);
        this->FreeSize        += phys_end - algn_end - PageSize;

        SplitLargeFrame(new (this->Map + frameCount) LargeFrameDescriptor(), (phys_end - algn_end) / PageSize - 2);

        this->SplitFree = frameCount;

        ++this->LargeFrameCount;
    }
}

/*  Frame manipulation  */

paddr_t FrameAllocationSpace::AllocateFrame(FrameSize size, uint32_t refCnt)
{
    switch (size)
    {
    case FrameSize::_64KiB: //  TODO: Use 4-KiB frames to provide this.
    case FrameSize::_4MiB:  //  TODO: Use 2-MiB frames to provide this.
    case FrameSize::_1GiB:
        FAIL("A request was made for a frame size which is not supported by this architecture.");

        return nullpaddr;

    default:
        break;
    }

    // MSG_("** AllocateFrame %s **%n", size == FrameSize::_4KiB ? "4 KiB" : "2 MiB");

    uint32_t lIndex = LargeFrameDescriptor::NullIndex;
    uint16_t sIndex = SmallFrameDescriptor::NullIndex;
    LargeFrameDescriptor * lDesc = nullptr;
    paddr_t paddr = nullpaddr;

    if (size == FrameSize::_4KiB)
    {
        withLock (this->SplitLocker)
        {
            //  No large frame was pre-selected.

            lIndex = this->SplitFree;

            if (lIndex == LargeFrameDescriptor::NullIndex)
                goto grab_large_page;

            // MSG_("   Split free: %u4%n", lIndex);

            //  Reaching this point means a non-full split frame exists!

            lDesc = this->Map + lIndex;
            sIndex = lDesc->GetExtras()->NextFree;
            SmallFrameDescriptor * const sDesc = lDesc->SubDescriptors + sIndex;

            assert_or(sIndex != SmallFrameDescriptor::NullIndex
                , "Invalid split frame state!")
            {
                //  This should *not* happen, ever.

                goto split_frame_full;
            }

            // MSG_("   Small frame index: %u2 (-> %u2)%n", sIndex, sDesc->NextIndex);

            sDesc->Use(refCnt);

            lDesc->GetExtras()->NextFree = sDesc->NextIndex;
            lDesc->GetExtras()->FreeCount -= 1;

            if unlikely(sDesc->NextIndex == SmallFrameDescriptor::NullIndex)
            {
                //  This was the last small frame in the split frame.

            split_frame_full:
                lDesc->Status = FrameStatus::Full;

                uint32_t next = lDesc->NextIndex;
                this->SplitFree = next;

                if likely(next != LargeFrameDescriptor::NullIndex)
                    this->Map[next].GetExtras()->PrevIndex = LargeFrameDescriptor::NullIndex;
                //  No more previous frame for the next frame.

                // MSG_("   Split frame depleted; Next: %u4%n", this->SplitFree);

                onRelease if (sIndex == SmallFrameDescriptor::NullIndex)
                    goto grab_large_page;
                //  In release mode, this rather odd situation is tolerated.
            }
        }

        // MSG_("   Returning address %XP **%n", this->AllocationStart + (lIndex << 21) + (sIndex << 12));

        return this->AllocationStart + (lIndex << 21) + (sIndex << 12);
    }

grab_large_page:
    withLock (this->LargeLocker)
    {
        lIndex = this->LargeFree;

        if (lIndex == LargeFrameDescriptor::NullIndex)
            return nullpaddr;

        //  Reaching this point means a non-full split frame exists!

        lDesc = this->Map + lIndex;

        this->LargeFree = lDesc->NextIndex;
        paddr = this->AllocationStart + (lIndex << 21);

        if (size == FrameSize::_2MiB)
        {
            lDesc->Use(refCnt);

            return paddr;
        }
    }

    //  At this point, `lIndex`, `paddr` and `lDesc` are valid.

    SplitLargeFrame(lDesc);

    sIndex = lDesc->GetExtras()->NextFree;
    SmallFrameDescriptor * const sDesc = lDesc->SubDescriptors + sIndex;

    sDesc->Use(refCnt);

    lDesc->GetExtras()->NextFree = sDesc->NextIndex;
    lDesc->GetExtras()->FreeCount -= 1;

    withLock (this->SplitLocker)
    {
        //  Gotta put this frame on top of the stack.

        uint32_t next = this->SplitFree;

        lDesc->NextIndex = next;
        this->SplitFree = lIndex;

        if likely(next != LargeFrameDescriptor::NullIndex)
            this->Map[next].GetExtras()->PrevIndex = lIndex;
    }

    return this->AllocationStart + (lIndex << 21) + (sIndex << 12);
}

Handle FrameAllocationSpace::Mingle(paddr_t addr, uint32_t & newCnt, int32_t diff, bool ignoreRefCnt)
{
    if (addr < this->AllocationStart || addr >= this->AllocationEnd)
        return HandleResult::PagesOutOfAllocatorRange;
    //  Easy-peasy.

    uint32_t lIndex = (uint32_t)((addr - this->AllocationStart) >> 21UL);
    LargeFrameDescriptor * lDesc = this->Map + lIndex;
    uint16_t sIndex = (uint16_t)((addr & 0x1FF000) >> 12);
    SmallFrameDescriptor * sDesc = lDesc->SubDescriptors + sIndex;

    auto test = [&newCnt, ignoreRefCnt, diff](FrameDescriptor * desc)
    {
        // MSG_("&& TEST %i4, %B &&%n", diff, ignoreRefCnt);

        if (diff == 0)
            return ignoreRefCnt || desc->ReferenceCount <= 1;
        else
            return (newCnt = desc->AdjustReferenceCount(diff)) == 0;
    };

    //  This only serves as a good starting point under conditions of low
    //  contention over this individual page.

    switch (lDesc->Status)
    {
    case FrameStatus::Free:
        return HandleResult::PageFree;

    case FrameStatus::Used:
        break;
        //  Will move onto the next phase, which is under a lock.

    case FrameStatus::Split:
    case FrameStatus::Full:
        goto do_small_frame;

    case FrameStatus::Reserved:
        return HandleResult::PageReserved;

    default:
        assert_or(false
            , "Unknown status for large frame @%XP: %u2"
            , addr, lDesc->Status)
        {
            return HandleResult::IntegrityFailure;
        }
    }

do_large_frame:
    //  A large frame is quite easy to deal with. They are chained in a
    //  singly-linked list. This is done under a lock to be sure.

    withLock (this->LargeLocker)
    {
        //  Yes, technically, since the previous checks, it could've changed
        //  from used to any other status.

        switch (lDesc->Status)
        {
        case FrameStatus::Free:
            return HandleResult::PageFree;

        case FrameStatus::Used:
            if likely(test(lDesc))
            {
                //  Both 0 and 1 are valid values for the reference count.
                lDesc->Free();

                lDesc->NextIndex = this->LargeFree;
                this->LargeFree = lIndex;

                return HandleResult::Okay;
            }

            //  No? Bad.
            return diff == 0 ? HandleResult::PageInUse : HandleResult::Okay;

        case FrameStatus::Split:
        case FrameStatus::Full:
            break;
            //  Simply leave the switch statement and continue execution.

        case FrameStatus::Reserved:
            return HandleResult::PageReserved;

        default:
            assert_or(false
                , "Unknown status for large frame @%XP: %u2"
                , addr, lDesc->Status)
            {
                return HandleResult::IntegrityFailure;
            }
        }
    }

do_small_frame:
    //  Reaching this point means the large frame is split, therefore the given
    //  address refers to a small frame.

    withLock (this->SplitLocker)
    {
        //  Yes, the check needs to be performed again. It COULD have changed
        //  since the previous check.

        switch (lDesc->Status)
        {
        case FrameStatus::Free:
            return HandleResult::PageFree;

        case FrameStatus::Used:
            goto do_large_frame;

        case FrameStatus::Split:
        case FrameStatus::Full:
            break;
            //  Simply leave the switch statement and continue execution.

        case FrameStatus::Reserved:
            return HandleResult::PageReserved;

        default:
            assert_or(false
                , "Unknown status for large frame @%XP: %u2"
                , addr, lDesc->Status)
            {
                return HandleResult::IntegrityFailure;
            }
        }

        if unlikely(sIndex == 0)
            return HandleResult::PageReserved;
        //  The first index is always reserved. No exceptions. Also, this is
        //  done under the lock because the large frame has to actually be split
        //  for this failure condition.

        switch (sDesc->Status)
        {
        case FrameStatus::Free:
            return HandleResult::PageFree;

        case FrameStatus::Used:
            if likely(test(sDesc))
            {
                sDesc->Free();

                sDesc->NextIndex = lDesc->GetExtras()->NextFree;
                lDesc->GetExtras()->NextFree = sIndex;
                uint16_t subDescCnt = lDesc->GetExtras()->FreeCount += 1;

                if unlikely(lDesc->Status == FrameStatus::Full)
                {
                    //  Split frame used to be full, but not anymore. So it can
                    //  be added to the stack of non-full split frames.

                    lDesc->Status = FrameStatus::Split;

                    uint32_t next = this->SplitFree;

                    lDesc->NextIndex = next;
                    this->SplitFree = lIndex;

                    if likely(next != LargeFrameDescriptor::NullIndex)
                        this->Map[next].GetExtras()->PrevIndex = lIndex;

                    lDesc->GetExtras()->PrevIndex = LargeFrameDescriptor::NullIndex;
                }
                else if unlikely(subDescCnt == LargeFrameDescriptor::SubDescriptorsCount)
                {
                    //  All small frames within the split frame are free, so it
                    //  can be freed completely.

                    uint32_t next = lDesc->NextIndex, prev = lDesc->GetExtras()->PrevIndex;

                    if (next != LargeFrameDescriptor::NullIndex)
                        this->Map[next].GetExtras()->PrevIndex = prev;
                    if (prev != LargeFrameDescriptor::NullIndex)
                        this->Map[prev].NextIndex = next;

                    if (this->SplitFree == lIndex)
                        this->SplitFree = next;

                    goto reclaim_large_frame;
                }

                return HandleResult::Okay;
            }

            return diff == 0 ? HandleResult::PageInUse : HandleResult::Okay;

        case FrameStatus::Split:
        case FrameStatus::Full:
            assert_or(false
                , "Invalid status for small frame @%XP: %u2."
                , addr, sDesc->Status)
            {
                break;
            }

        case FrameStatus::Reserved:
            return HandleResult::PageReserved;

        default:
            assert_or(false
                , "Unknown status for small frame @%XP: %u2"
                , addr, lDesc->Status)
            {
                return HandleResult::IntegrityFailure;
            }
        }
    }

    //  Should never be reached.
    return HandleResult::IntegrityFailure;

reclaim_large_frame:
    lDesc->Free();

    withLock (this->LargeLocker)
    {
        lDesc->NextIndex = this->LargeFree;
        this->LargeFree = lIndex;
    }

    return HandleResult::Okay;
}

Handle FrameAllocationSpace::ReserveRange(paddr_t start, psize_t size, bool includeBusy)
{
    if (start < this->AllocationStart || (start + size) >= this->AllocationEnd)
        return HandleResult::PagesOutOfAllocatorRange;

    //  TODO
}

/***************************
    FrameAllocator class
***************************/

/*  Page Manipulation  */

paddr_t FrameAllocator::AllocateFrame(FrameSize size, AddressMagnitude magn, uint32_t refCnt)
{
    paddr_t ret = nullpaddr;
    FrameAllocationSpace * space;

    if (magn == AddressMagnitude::Any || magn == AddressMagnitude::_48bit)
    {
        //  Any space is alright.

        space = this->LastSpace;

        while (space != nullptr)
        {
            ret = space->AllocateFrame(size, refCnt);

            if (ret != nullpaddr)
                return ret;

            space = space->Previous;
        }
    }
    else if (magn == AddressMagnitude::_32bit)
    {
        space = this->LastSpace;

        while (space != nullptr)
        {
            if (space->GetAllocationEnd() <= (1ULL << 32))
            {
                //  The condition checks that the allocation space ends
                //  at a 32-bit address. (all the other addresses are less,
                //  thus have to be 32-bit if the end is)

                ret = space->AllocateFrame(size, refCnt);

                if (ret != nullpaddr)
                    return ret;
            }

            space = space->Previous;
        }
    }
    else
    {
        //  TODO: 24-bit and 16-bit addresses, maybeh?

        FAIL("Unable to serve frames of address magnitude %s."
            , (magn == AddressMagnitude::_24bit) ? "24-bit" : "16-bit");
    }

    return nullpaddr;
}

Handle FrameAllocator::Mingle(paddr_t addr, uint32_t & newCnt, int32_t diff, bool ignoreRefCnt)
{
    Handle res;
    FrameAllocationSpace * space = this->FirstSpace;

    while (space != nullptr)
    {
        res = space->Mingle(addr, newCnt, diff, ignoreRefCnt);

        if (!res.IsResult(HandleResult::PagesOutOfAllocatorRange))
            return res;

        space = space->Next;
    }

    return HandleResult::PagesOutOfAllocatorRange;
}

Handle FrameAllocator::ReserveRange(paddr_t start, psize_t size, bool includeBusy)
{
    Handle res;
    FrameAllocationSpace * space = this->FirstSpace;

    while (space != nullptr)
    {
        res = space->ReserveRange(start, size, includeBusy);

        if (!res.IsResult(HandleResult::PagesOutOfAllocatorRange))
            return res;

        space = space->Next;
    }

    return HandleResult::PagesOutOfAllocatorRange;
}

bool FrameAllocator::ContainsRange(paddr_t start, psize_t size)
{
    FrameAllocationSpace const * space = this->FirstSpace;

    while (space != nullptr)
    {
        if (space->ContainsRange(start, size))
            return true;

        space = space->Next;
    }

    return false;
}

FrameAllocationSpace * FrameAllocator::GetSpace(paddr_t paddr)
{
    FrameAllocationSpace * space = this->FirstSpace;

    while (space != nullptr)
    {
        if (space->ContainsRange(paddr, 1))
            return space;

        space = space->Next;
    }

    return nullptr;
}

/*  Space Chaining  */

void FrameAllocator::PreppendAllocationSpace(FrameAllocationSpace * space)
{
    withLock (this->ChainLock)
    {
        this->FirstSpace->Previous = space;
        space->Next = this->FirstSpace;
        this->FirstSpace = space;
    }
}

void FrameAllocator::AppendAllocationSpace(FrameAllocationSpace * space)
{
    withLock (this->ChainLock)
    {
        this->LastSpace->Next = space;
        space->Previous = this->LastSpace;
        this->LastSpace = space;
    }
}
