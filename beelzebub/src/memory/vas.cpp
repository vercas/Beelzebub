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
#include <beel/interrupt.state.hpp>

#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Synchronization;
using namespace Beelzebub::Utils;

struct DescriptorCheckResults
{
    bool Accepted, MeetsLeft, MeetsRight, LeftMergeable, RightMergeable;
    Handle FailureResult;
};

struct AnonymousAllocationResults
{
    bool Fits;
};

struct OperationParameters
{
    /*  Constructor(s)  */

    inline OperationParameters(Memory::Vas * vas, vaddr_t vaddr, size_t size
                             , bool sparse, bool tolerant, bool allocation)
        : Vas( vas)
        , StartAddress(vaddr)
        , StartSize(size)
        , Address(vaddr)
        , Size(size)
        , Sparse(sparse)
        , Tolerant(tolerant)
        , Allocation(allocation)
    {

    }

    /*  Operations  */

    //  Checks if the given region fits the criteria for this operation.
    virtual DescriptorCheckResults Check(MemoryRegion const * reg) = 0;

    //  Reassigns its choice of settings to the given region.
    virtual void Repurpose(MemoryRegion * reg) = 0;

    //  Spawns the given region, given its choice of settings.
    virtual Handle Spawn(vaddr_t start, vaddr_t end, MemoryRegion * & reg) = 0;

    //  Meant to change `Address` if the check passes.
    virtual bool CanAllocateAnonymously(MemoryRegion * reg) = 0;

    /*  Fields  */

    Memory::Vas * const Vas;

    vaddr_t const StartAddress;
    size_t const StartSize;

    vaddr_t Address;
    size_t Size;

    bool const Sparse, Tolerant, Allocation;

    /*  Meat  */

    __hot Handle Execute(bool lock)
    {
        Memory::Vas * vas = this->Vas;

        if unlikely(vas->First == nullptr)
            return HandleResult::ObjectDisposed;

        Handle res = HandleResult::Okay;
        vaddr_t const vaddr = this->Address;
        size_t const size = this->Size;
        vaddr_t const endAddr = vaddr + size;

        MemoryRegion * reg;
        vaddr_t continuation;
        DescriptorCheckResults dcr;

        InterruptState cookie;

        bool runPostCheck = false;

        if (vas->ImplementsPreCheck())
            runPostCheck = vas->PreCheck(lock, this->Allocation);

        if likely(lock)
        {
            cookie = InterruptState::Disable();

            vas->Lock.AcquireAsWriter();
        }

        if (runPostCheck)
        {
            res = vas->PostCheck();

            if unlikely(!res.IsOkayResult())
                goto end;
        }

        if unlikely(this->Allocation && vaddr == nullvaddr)
        {
            //  Null vaddr on allocation means any address is accepted.

            reg = vas->First;

            do
            {
                if (reg->Content == MemoryContent::Free && this->CanAllocateAnonymously(reg))
                {
                    //  This free region fits the criteria for anonymous allocation.

                    res = this->Execute(false);

                    goto end;
                }

                //  If not, move on.

                reg = reg->Next;
            } while (reg != nullptr);

            //  Reaching this point means there is no space to spare!

            res = HandleResult::OutOfMemory;

            goto end;
        }

        /*
        There are a few possible cases here:
            1.  The requested area covers more than one descriptor.
                In this case, as much as possible is handled and the function is
                re-entered.
            2.  Area is in the middle of a descriptor.
                A desired descriptor in the middle and a near-copy of the original
                descriptor at the end are created.
            3.  Area is just at the beginning of a descriptor.
                Descriptor is shrunk towards the end, and previous descriptor is
                extended if possible; otherwise desired descriptor is allocated over
                the area.
            4.  Area is just at the end of a desc.
                Descriptor is shrunk towards the start, next descriptor is
                extended if it matches; ditto.
            5.  Area covers entire descriptor.
                The descriptor is removed and adjacent matching descriptors are merged.

            + 4 more cases due to mergeability.

            Total: 9 possibilities.
         */
        
        continuation = nullvaddr;

        reg = vas->Tree.Find<vaddr_t>(vaddr);

        if unlikely(reg == nullptr)
        {
            res = HandleResult::ArgumentOutOfRange;

            goto end;
        }

        if (vas->LastSearched == reg)
            vas->LastSearched = nullptr;

        if (endAddr > reg->Range.End)
        {
            if (this->Sparse)
                continuation = reg->Range.End;
            else
            {
                res = HandleResult::ArgumentOutOfRange;

                goto end;
            }
        }

        dcr = this->Check(reg);

        if (dcr.Accepted)
        {
            if (0 != (reg->Type & MemoryAllocationOptions::Permanent))
            {
                //  Permanent allocations cannot be manipulated.

                res = HandleResult::Failed;

                goto end;
            }

            //  Now there are only four major cases to handle.

            if (dcr.MeetsLeft)
            {
                if (dcr.MeetsRight)
                {
                    //  So the operation spans an entire descriptor (or more), kool.
                
                    if (dcr.LeftMergeable)
                    {
                        vaddr_t newEnd;
                        MemoryRegion * next;

                        if (dcr.RightMergeable)
                        {
                            //  So this descriptor, to be operated on entirely, is surrounded
                            //  by mergeable descriptors. This means the operated descriptor and
                            //  the right mergeable one are removed and the previous mergeable one
                            //  is extended to cover both.

                            newEnd = reg->Next->Range.End;
                            next = reg->Next->Next;

                            res = vas->Tree.Remove<vaddr_t>(newEnd - 1);
                            if unlikely(!res.IsOkayResult()) goto end;
                        }
                        else
                        {
                            //  Right one is not mergeable, but left one is.
                            //  Means the operated descriptor is removed an the
                            //  left one is extended to cover it.

                            newEnd = reg->Range.End;
                            next = reg->Next;
                        }

                        reg = reg->Prev;    //  Variable is repurposed.

                        res = vas->Tree.Remove<vaddr_t>(vaddr);
                        if unlikely(!res.IsOkayResult()) goto end;
                        //  This removes old `reg`.

                        //  Now there's room to expand the previous descriptor, which is in `reg`.

                        reg->Range.End = newEnd;

                        reg->Next = next;
                        if (next != nullptr)
                            next->Prev = reg;
                        //  Patch linkage.
                    }
                    else
                    {
                        if (dcr.RightMergeable)
                        {
                            //  Right one is mergeable, but left one is NOT. Right descriptor
                            //  is extended to cover the operated one, which is removed.

                            vaddr_t const newStart = reg->Range.Start;
                            MemoryRegion * const prev = reg->Prev;
                            reg = reg->Next;    //  Variable is repurposed.

                            res = vas->Tree.Remove<vaddr_t>(vaddr);
                            if unlikely(!res.IsOkayResult()) goto end;

                            //  Now there's room to expand the next descriptor.

                            reg->Range.Start = newStart;

                            reg->Prev = prev;
                            if (prev != nullptr)
                                prev->Next = reg;
                            else
                                vas->First = reg;
                        }
                        else
                        {
                            //  This descriptor is surrounded by non-mergeable ones.
                            //  Therefore, no removal is necessary, just repurposing.

                            this->Repurpose(reg);

                            //  And linkage stays intact.
                        }
                    }
                }
                else
                {
                    //  The operation starts at the beginning of the descriptor
                    //  but stops before the end of it.

                    vaddr_t const oldStart = reg->Range.Start;

                    reg->Range.Start = endAddr;
                    reg->Type &= ~MemoryAllocationOptions::GuardLow;
                    //  Chosen descriptor is shrunk and lower guard is gone, if any.

                    if (dcr.LeftMergeable)
                    {
                        //  Left descriptor is mergeable means it can be extended to
                        //  cover the operated parts.

                        reg->Prev->Range.End = endAddr;

                        //  Linkage is intact.
                    }
                    else
                    {
                        //  Otherwise one ought to be spawned.

                        MemoryRegion * newReg = nullptr;
                        res = this->Spawn(oldStart, endAddr, newReg);

                        if unlikely(!res.IsOkayResult()) goto end;

                        newReg->Prev = reg->Prev;
                        newReg->Next = reg;

                        if (reg->Prev != nullptr)
                            reg->Prev->Next = newReg;
                        else
                            vas->First = newReg;

                        reg->Prev = newReg;
                        //  Patch linkage.
                    }
                }
            }
            else
            {
                if (dcr.MeetsRight)
                {
                    //  The operation starts after the beginning of the descriptor
                    //  but stops at the end of it.

                    vaddr_t const oldEnd = reg->Range.End;

                    reg->Range.End = vaddr;
                    reg->Type &= ~MemoryAllocationOptions::GuardHigh;
                    //  Busy descriptor is shrunk and higher guard is gone, if any.

                    if (dcr.RightMergeable)
                    {
                        //  Next descriptor is mergeable means it can be extended to
                        //  cover the operated parts.

                        reg->Next->Range.Start = vaddr;

                        //  Linkage is intact.
                    }
                    else
                    {
                        //  Otherwise one ought to be allocated.

                        MemoryRegion * newReg = nullptr;
                        res = this->Spawn(vaddr, oldEnd, newReg);

                        if unlikely(!res.IsOkayResult()) goto end;

                        newReg->Prev = reg;
                        newReg->Next = reg->Next;

                        if (reg->Next != nullptr)
                            reg->Next->Prev = newReg;

                        reg->Next = newReg;
                        //  Patch linkage.
                    }
                }
                else
                {
                    //  The operation is within a descriptor and doesn't include
                    //  either end. Means that a desired region must be created in the
                    //  middle & a copy of the descriptor w/o lower guard for the
                    //  end. Existing descriptor stays at the beginning.

                    vaddr_t const oldEnd = reg->Range.End;
                    MemoryAllocationOptions const oldHighGuard = reg->Type & MemoryAllocationOptions::GuardHigh;

                    reg->Range.End = vaddr;

                    MemoryRegion * newMidReg = nullptr, * newEndReg = nullptr;

                    res = this->Spawn(vaddr, endAddr, newMidReg);

                    if unlikely(!res.IsOkayResult()) goto end;

                    res = vas->Tree.Insert(MemoryRegion(
                        endAddr, oldEnd, reg->Flags, reg->Content
                        , (reg->Type & ~MemoryAllocationOptions::GuardLow) | oldHighGuard
                    ), newEndReg);

                    if unlikely(!res.IsOkayResult()) goto end;

                    newMidReg->Prev = reg;
                    newMidReg->Next = newEndReg;
                    newEndReg->Prev = newMidReg;
                    newEndReg->Next = reg->Next;
                    reg->Next = newMidReg;

                    if (newEndReg->Next != nullptr)
                        newEndReg->Next->Prev = newEndReg;
                    //  And linkage is patched.
                }
            }
        }
        else if unlikely(!this->Tolerant)
        {
            res = dcr.FailureResult;

            goto end;
        }

        if (continuation != nullvaddr)
        {
            this->Address = continuation;
            this->Size = size - (continuation - vaddr);

            res = this->Execute(false);
        }

    end:
        if likely(lock)
        {
            vas->Lock.ReleaseAsWriter();

            cookie.Restore();
        }

        return res;
    }
};

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
        , MemoryFlags::None
        , MemoryContent::Free
        , MemoryAllocationOptions::None), this->First);
    //  Blank memory region, for allocation.
}

/*  Operations  */

Handle Vas::Allocate(vaddr_t & vaddr, size_t size
    , MemoryFlags flags, MemoryContent content
    , MemoryAllocationOptions type, bool lock)
{
    if unlikely(this->First == nullptr)
        return HandleResult::ObjectDisposed;

    Handle res;

    size_t const lowOffset  = 0 != (type & MemoryAllocationOptions::GuardLow ) ? PageSize : 0;
    size_t const highOffset = 0 != (type & MemoryAllocationOptions::GuardHigh) ? PageSize : 0;

    vaddr_t const effectiveAddress = vaddr == nullvaddr ? nullvaddr : (vaddr - lowOffset);
    size_t const effectiveSize = size + lowOffset + highOffset;

    struct AllocateOperation : public OperationParameters
    {
        /*  Constructor(s)  */

        inline AllocateOperation(Memory::Vas * vas, vaddr_t vaddr, size_t size
                               , MemoryFlags flags, MemoryContent content, MemoryAllocationOptions type)
            : OperationParameters(vas, vaddr, size, false, false, true)
            , Flags(flags)
            , Content(content)
            , Type(type)
        {

        }

        /*  Operations  */

        virtual DescriptorCheckResults Check(MemoryRegion const * reg) override
        {
            DescriptorCheckResults res;
            res.FailureResult = HandleResult::PageFree;

            if ((res.Accepted = (reg->Content == MemoryContent::Free)))
            {
                res.MeetsLeft  = reg->Range.Start == this->Address;
                res.MeetsRight = reg->Range.End   == this->Address + this->Size;

                MemoryRegion const * const prev = reg->Prev, * const next = reg->Next;

                res.LeftMergeable  = res.MeetsLeft
                    && prev != nullptr
                    && 0 == (prev->Type & MemoryAllocationOptions::GuardHigh)   //  Previous region must not have a high guard.
                    && 0 == (this->Type & MemoryAllocationOptions::GuardLow)    //  Requested one must not have a low guard.
                    && prev->Flags == this->Flags                               //  Flags and content must match.
                    && MemoryContentsMergeable(prev->Content, this->Content)
                    && (prev->Type & ~MemoryAllocationOptions::GuardFull) == (this->Type & ~MemoryAllocationOptions::GuardFull);

                res.RightMergeable = res.MeetsRight
                    && next != nullptr
                    && 0 == (next->Type & MemoryAllocationOptions::GuardLow)
                    && 0 == (this->Type & MemoryAllocationOptions::GuardHigh)
                    && next->Flags == this->Flags
                    && MemoryContentsMergeable(next->Content, this->Content)
                    && (next->Type & ~MemoryAllocationOptions::GuardFull) == (this->Type & ~MemoryAllocationOptions::GuardFull);
            }

            return res;
        }

        virtual void Repurpose(MemoryRegion * reg) override
        {
            reg->Flags = this->Flags;
            reg->Content = this->Content;
            reg->Type = this->Type;
        }

        virtual Handle Spawn(vaddr_t start, vaddr_t end, MemoryRegion * & reg) override
        {
            return this->Vas->Tree.Insert(MemoryRegion(start, end
                , this->Flags
                , this->Content
                , this->Type), reg);
        }

        virtual bool CanAllocateAnonymously(MemoryRegion * reg) override
        {
            //  TODO: Alignment rules.

            bool res = reg->GetSize() >= this->StartSize;

            if (res)
                this->Address = reg->Range.End - this->StartSize;

            return res;
        }

        /*  Fields  */

        MemoryFlags Flags;
        MemoryContent Content;
        MemoryAllocationOptions Type;
    } manip(this, effectiveAddress, effectiveSize, flags, content, type);

    res = manip.Execute(lock);

    vaddr = manip.Address + lowOffset;

    return res;
}

Handle Vas::Free(vaddr_t vaddr, size_t size, bool sparse, bool tolerant, bool lock)
{
    struct FreeOperation : public OperationParameters
    {
        /*  Constructor(s)  */

        inline FreeOperation(Memory::Vas * vas, vaddr_t vaddr, size_t size
                                    , bool sparse, bool tolerant)
            : OperationParameters(vas, vaddr, size, sparse, tolerant, false)
        {

        }

        /*  Operations  */

        virtual DescriptorCheckResults Check(MemoryRegion const * reg) override
        {
            DescriptorCheckResults res;
            res.FailureResult = HandleResult::PageFree;

            if ((res.Accepted = (reg->Content != MemoryContent::Free)))
            {
                res.MeetsLeft  = reg->Range.Start == this->Address              || (0 != (reg->Type & MemoryAllocationOptions::GuardLow ) && reg->Range.Start == this->Address              - PageSize);
                res.MeetsRight = reg->Range.End   <= this->Address + this->Size || (0 != (reg->Type & MemoryAllocationOptions::GuardHigh) && reg->Range.End   == this->Address + this->Size + PageSize);
                //  End is met even if the requested range is beyond this descriptor.

                res.LeftMergeable  = res.MeetsLeft  && reg->Prev != nullptr && reg->Prev->Content == MemoryContent::Free;
                res.RightMergeable = res.MeetsRight && reg->Next != nullptr && reg->Next->Content == MemoryContent::Free;
            }

            return res;
        }

        virtual void Repurpose(MemoryRegion * reg) override
        {
            reg->Flags = MemoryFlags::None;
            reg->Content = MemoryContent::Free;
            reg->Type = MemoryAllocationOptions::None;
        }

        virtual Handle Spawn(vaddr_t start, vaddr_t end, MemoryRegion * & reg) override
        {
            return this->Vas->Tree.Insert(MemoryRegion(start, end
                , MemoryFlags::None
                , MemoryContent::Free
                , MemoryAllocationOptions::None), reg);
        }

        virtual bool CanAllocateAnonymously(MemoryRegion * reg) override
        {
            (void)reg;

            FAIL("VAS free operation should not consider the possibility of anonymous allocation.");
        }
    } manip(this, vaddr, size, sparse, tolerant);

    return manip.Execute(lock);
}

Handle Vas::Modify(vaddr_t vaddr, size_t pageCnt
    , MemoryFlags flags, bool lock)
{
    (void)vaddr;
    (void)pageCnt;
    (void)flags;

    if unlikely(this->First == nullptr)
        return HandleResult::ObjectDisposed;

    Handle res = HandleResult::Okay;

    InterruptState cookie;

    if likely(lock)
    {
        cookie = InterruptState::Disable();

        this->Lock.AcquireAsWriter();
    }

    //  So, this is gonna suck a bit. There are... Lots of options.

//end:
    if likely(lock)
    {
        this->Lock.ReleaseAsWriter();

        cookie.Restore();
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
    (void)lock;
    (void)alloc;

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
