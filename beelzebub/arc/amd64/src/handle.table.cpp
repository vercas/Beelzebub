/*
    Copyright (c) 2018 Alexandru-Mihai Maftei. All rights reserved.


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

#include "handle.table.arc.hpp"
#include "memory/vmm.hpp"

using namespace Beelzebub;
using namespace Beelzebub::Memory;

__thread handle_t LocalFreeIndex = HandleTable::InvalidHandle;
__thread size_t LocalFreeCount = 0;

/***************************
    HandleTableArc class
***************************/

/*  Statics  */

HandleTableEntryArc * HandleTableArc::Table;
handle_t HandleTableArc::GlobalFreeIndex;
handle_t HandleTableArc::Maximum;
handle_t HandleTableArc::Cursor {0};

handle_t HandleTableArc::FreeListThreshold { 100 };
handle_t HandleTableArc::FreeListRemovalCount { 90 };

/************************
    HandleTable class
************************/

/*  Initialization  */

HandleTableInitializationResult HandleTable::Initialize(handle_t limit)
{
    vsize_t const size = RoundUp(limit.Value * SizeOf<HandleTableEntryArc>, PageSize);
    vaddr_t vaddr = nullvaddr;

    //  TODO: Read parameters for FreeList(Threshold|RemovalCount)

    Handle res = Vmm::AllocatePages(nullptr
        , size
        , MemoryAllocationOptions::AllocateOnDemand | MemoryAllocationOptions::VirtualKernelHeap
        , MemoryFlags::Global | MemoryFlags::Writable
        , MemoryContent::HandleTable
        , vaddr);

    if (res != HandleResult::Okay)
        return HandleTableInitializationResult::OutOfMemory;

    HandleTableArc::Table = reinterpret_cast<HandleTableEntryArc *>((void *)vaddr);
    HandleTableArc::Maximum = limit;

    return HandleTableInitializationResult::Success;
}

/*  Operation  */

Result<HandleAllocationResult, handle_t> HandleTable::Allocate(uint16_t pcid)
{
    if unlikely(pcid == 0xFFFF)
        return HandleAllocationResult::InvalidPcid;
    //  TODO: Type-safe PCID, remove magic number.

    handle_t res;

    if ((res = LocalFreeIndex) != InvalidHandle)
    {
        LocalFreeIndex = HandleTableArc::Table[res.Value].LocalIndex;
        --LocalFreeCount;

        goto return_entry;
    }

    do
    {
        res = handle_t(__atomic_load_n(&HandleTableArc::GlobalFreeIndex.Value, __ATOMIC_ACQUIRE));

        if (res == InvalidHandle)
            break;

        handle_t next = HandleTableArc::Table[res.Value].LocalIndex;

        if (__atomic_compare_exchange_n(&HandleTableArc::GlobalFreeIndex.Value, &next.Value, res.Value, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED))
            goto return_entry;

        /*  Note: This is indeed vulnerable to the ABA problem, when the `next` value of the stack top entry changes
            after it has been read. This is extremely unlikely to occur in practice because of the use of local free
            lists. This entry would have to have been popped from the stack, deallocated, triggered a garbage
            collection, and end up in the right spot to make it global stack top again.
        */
    } while (true);

    //  Reaching this point means the cursor needs to be increased.

    if (HandleTableArc::Cursor.Value >= HandleTableArc::Maximum.Value)
        return HandleAllocationResult::TableFull;

    res = handle_t(__atomic_fetch_add(&HandleTableArc::Cursor.Value, 1, __ATOMIC_SEQ_CST));

    if (res.Value >= HandleTableArc::Maximum.Value)
        return HandleAllocationResult::TableFull;
    //  Keep in mind, this can fail even now. We cannot overallocate.

return_entry:
    new (HandleTableArc::Table + res.Value) HandleTableEntryArc(0, pcid, InvalidHandle);
    //  This **might** page fault if the cursor was incremented before.
    //  TODO: Catch an exception here, perhaps?

    return res;
}

bool HandleTable::Deallocate(handle_t ind)
{
    if unlikely(ind.Value < HandleTableArc::Cursor.Value)
        return false;

    HandleTableEntryArc * entry = HandleTableArc::Table + ind.Value;

    if unlikely(entry->ProcessId == 0xFFFF)
        return false;
    //  TODO: Magic number, begone!

    //  TODO: Mark invalidation, somehow, in the process maps.

    entry->ReferenceCount -= 1;
    entry->ProcessId = 0xFFFF;
    entry->LocalIndex = LocalFreeIndex;

    LocalFreeIndex = ind;

    if unlikely(++LocalFreeCount >= HandleTableArc::FreeListThreshold.Value)
    {
        size_t i;

        for (i = 1; entry->LocalIndex != InvalidHandle && i < HandleTableArc::FreeListRemovalCount.Value; ++i)
            entry = HandleTableArc::Table + entry->LocalIndex.Value;

        LocalFreeIndex = entry->LocalIndex;
        LocalFreeCount -= i;

        do
        {
            entry->LocalIndex = handle_t(__atomic_load_n(&HandleTableArc::GlobalFreeIndex.Value, __ATOMIC_ACQUIRE));
        } while(!__atomic_compare_exchange_n(&HandleTableArc::GlobalFreeIndex.Value, &entry->LocalIndex, ind, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
        //  Splices some of the current core's free list entries into the global free list.
    }

    return true;
}

Result<HandleGetResult, HandleTableEntry> HandleTable::Get(handle_t ind)
{
    HandleTableEntryArc const e = HandleTableArc::Table[ind.Value];

    if unlikely(e.ProcessId == 0xFFFF)
        return HandleGetResult::Unallocated;

    return e;
}

