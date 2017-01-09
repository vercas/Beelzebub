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

#define VALLOC_SOURCE

#include "structures.hpp"
#include "locks.hpp"
#include <valloc/interface.hpp>
#include <new>
#include <string.h>

using namespace Valloc;

static __thread ThreadData TD;

Lock GLock {}, PLock {};
Arena * GList = nullptr;

/********************
    Arena Linkage
********************/

static Arena * UnlinkArena(Arena * arena)
{
    if (arena->Prev != nullptr)
        arena->Prev->Next = arena->Next;
    if (arena->Next != nullptr)
        arena->Next->Prev = arena->Prev;

    return arena;
}

static Arena * AddToMine(Arena * const arena)
{
    // Platform::ErrorMessage("Adding arena " VF_PTR " to " VF_PTR, arena, &TD);

    if ((arena->Next = TD.FirstArena) != nullptr)
        arena->Next->Prev = arena;

    TD.FirstArena = arena;

    return arena;
}

static Arena * RemoveFromMine(Arena * const arena)
{
    if (TD.FirstArena == arena)
    {
        if ((TD.FirstArena = arena->Next) != nullptr)
            TD.FirstArena->Prev = nullptr;

        return arena;
    }
    else
        return UnlinkArena(arena);
}

static void RetireArena(Arena * arena)
{
    Platform::ErrorMessage("Retiring arena " VF_PTR " of " VF_PTR, arena, &TD);

    RemoveFromMine(arena);

    arena->Owner = nullptr;
    arena->Prev = nullptr;

    GLock.Acquire();

    if ((arena->Next = GList) != nullptr)
        GList->Prev = arena;

    GList = arena;

    GLock.Release();

    //  And such, the job is done.
}

static Arena * PromoteArena(Arena * arena)
{
    arena->Owner = &TD;

    if (GList == arena)
    {
        if ((GList = arena->Next) != nullptr)
            GList->Prev = nullptr;

        return arena;
    }
    else
        return UnlinkArena(arena);
}

/***************************
    Arena (De)allocation
***************************/

static Arena * AllocateArena()
{
    void * addr = nullptr;
    size_t size = Platform::LargePageSize;

    Platform::AllocateMemory(addr, size);

    if (VALLOC_UNLIKELY(addr == nullptr))
        return nullptr;

    if (VALLOC_UNLIKELY(size < Platform::PageSize))
        Platform::FreeMemory(addr, size);
    //  This is weird... Cannot continue.

    Platform::ErrorMessage("Allocated arena " VF_PTR " for " VF_PTR, addr, &TD);

    return AddToMine(new (addr) Arena(&TD, size));
}

static bool ExtendArena(Arena * arena)
{
    void * end = const_cast<void *>(arena->GetEnd());
    size_t size = Platform::LargePageSize;

    Platform::AllocateMemory(end, size);

    if (VALLOC_UNLIKELY(end == nullptr))
        return false;

    arena->Size += size;
    arena->Free += size;

    if (arena->LastFree != nullptr && arena->LastFree->GetNext() == end)
    {
        //  Easiest case possible - just extend the last chunk.

        arena->LastFree->Size += size;
    }
    else
    {
        //  Add a new free chunk at the end.

        FreeChunk * c = new (end) FreeChunk(arena->LastFree, arena->LastBusy, size, nullptr);

        arena->LastFree = c;

        if (c->PrevFree != nullptr)
            c->PrevFree->NextFree = c;
    }

    Platform::ErrorMessage("Extended arena " VF_PTR " of " VF_PTR, arena, &TD);

    return true;
}

static void DeallocateArena(Arena * const arena)
{
    RemoveFromMine(arena);

    Platform::ErrorMessage("Deallocating arena " VF_PTR " of " VF_PTR, arena, &TD);

    Platform::FreeMemory(arena, arena->Size);
}

/********************
    Chunk Freeing
********************/

static void RelinkFreeChunk(Arena * arena, FreeChunk * c)
{
    if (c->PrevFree != nullptr)
        c->PrevFree->NextFree = c;

    if (c->NextFree != nullptr)
        c->NextFree->PrevFree = c;
    else
        arena->LastFree = c;

    if (c->GetNext() < arena->GetEnd())
        c->GetNext()->Prev = c;
}

static void IntroduceFreeChunk(Arena * arena, FreeChunk * c)
{
    if (arena->LastFree == nullptr)
    {
        //  Next is busy, and there's no free chunk. Happens.

        c->PrevFree = nullptr;
        c->NextFree = nullptr;

        arena->LastFree = c;
    }
    else
    {
        //  Next is busy, and there's a free chunk.

        FreeChunk * lastFree = arena->LastFree;

        if (lastFree > c)
        {
            //  So the last free chunk is after the current one.

            FreeChunk * free;

            do free = lastFree; while ((lastFree = lastFree->PrevFree) > c);
            //  Find the closest free chunk after the current one.
            //  This also stops at null if (hopefully) pointer comparison is
            //  done unsigned.

            if ((c->PrevFree = free->PrevFree) != nullptr)
                c->PrevFree->NextFree = c;
            //  Patch between current and previous, if any.

            free->PrevFree = c;
            c->NextFree = free;
            //  Patch between current and next, if any.
        }
        else
        {
            //  Last chunk if before this one, meaning this one is now the last.

            arena->LastFree = lastFree->NextFree = c;
            c->PrevFree = lastFree;
        }
    }
}

static size_t FreeThisChunk(Arena * arena, Chunk * c, void const * const arenaEnd, Chunk * & res)
{
    arena->Free += c->Size;

    FreeChunk * const p = c->Prev->AsFree(), * const n = c->GetNext()->AsFree();
    Chunk * const nn = (n < arenaEnd) ? n->GetNext() : nullptr;

    if (p != nullptr && p->IsFree())
    {
        //  Previous is free.

        if (nn != nullptr && n->IsFree())
        {
            //  Surrounded on both sides by free chunks.

            p->Size += c->Size + n->Size;
            //  Must include both.

            if (nn < arenaEnd)
                nn->Prev = p;
            //  This concludes standard chunk linkage.

            if ((p->NextFree = n->NextFree) != nullptr)
                //  If the next also had a next, its previous is my previous!
                p->NextFree->PrevFree = p;
            else
                //  Push back the last free if the next one was the last.
                arena->LastFree = p;
        }
        else
        {
            //  Previous is free, next is not.

            if (nn != nullptr)
                n->Prev = p;
            //  Patch normal chunk linkage. Free chunk linkage stays intact.

            p->Size += c->Size;
        }

        if (arena->LastBusy == c)
            arena->LastBusy = p->Prev->AsBusy();
        //  Previous is free, therefore its previous is busy (or null).

        c = p;
    }
    else
    {
        //  Previous is busy or null.

        c->Flags = ChunkFlags::Free;

        if (nn != nullptr && n->IsFree())
        {
            //  Next is free.
            c->Size += n->Size;

            if (nn < arenaEnd)
                nn->Prev = c;

            if ((c->PrevFree = n->PrevFree) != nullptr)
                c->PrevFree->NextFree = c->AsFree();
            //  Patch linkage with the previous free chunk, if any.

            if ((c->AsFree()->NextFree = n->NextFree) != nullptr)
                c->AsFree()->NextFree->PrevFree = c->AsFree();
                //  Patch linkage with the next free chunk, if any.
            else
                //  Push back the last free if the next one was the last.
                arena->LastFree = c->AsFree();

            if (arena->LastBusy == c)
                arena->LastBusy = c->Prev->AsBusy();
        }
        else
            //  Next is busy.
            IntroduceFreeChunk(arena, c->AsFree());
    }

    res = c;
    return c->Size;
}

static size_t FreeThisChunk(Arena * arena, Chunk * c, void const * const arenaEnd)
{
    Chunk * sink;

    return FreeThisChunk(arena, c, arenaEnd, sink);
}

/*************************
    Garbage Collection
*************************/

enum CollectionResult { NoGarbage, TargetReached, CollectedAll, };

static CollectionResult CollectGarbage(Arena * arena, size_t const target, Chunk * & res)
{
    if (arena->FreeListEmpty())
        return NoGarbage;

    void const * const arenaEnd = arena->GetEnd();
    Chunk * cur = arena->FreeList.Swap(nullptr), * next;

    do
    {
        next = cur->NextInList;

        size_t const freeSize = FreeThisChunk(arena, cur, arenaEnd, res);

        if (VALLOC_LIKELY(target != 0) && freeSize >= target)
        {
            if (VALLOC_LIKELY((cur = next) != nullptr))
            {
                Chunk * last = nullptr;

                do last = cur; while ((cur = cur->NextInList) != nullptr);

                Chunk * top = arena->FreeList.Pointer;

                do last->NextInList = top; while (!arena->FreeList.CAS(top, next));
            }

            return TargetReached;
        }
    } while ((cur = next) != nullptr);

    return CollectedAll;
}

static CollectionResult CollectGarbage(Arena * arena, size_t const target)
{
    Chunk * sink;

    return CollectGarbage(arena, target, sink);
}

/*****************************
    Valloc::AllocateMemory    >-------------------------------------------------
*****************************/

void * Valloc::AllocateMemory(size_t size)
{
    size_t const roundSize = RoundUp(size + sizeof(Chunk), Platform::CacheLineSize);

    Arena * arena;

    if (VALLOC_UNLIKELY((arena = TD.FirstArena) == nullptr))
        goto with_new_arena;

    do
    {
        VALLOC_ASSERT_MSG(reinterpret_cast<uintptr_t>(arena) % Platform::PageSize == 0
            , "Misaligned arena..? " VF_PTR, arena);

        Chunk * c;

        if (VALLOC_UNLIKELY(arena->Free < roundSize))
            goto no_space;

    try_chunks:
        for (c = arena->LastFree; c != nullptr; c = c->PrevFree)
        {
            VALLOC_ASSERT_MSG(c->IsFree()
                , "Chunk " VF_PTR " from " VF_PTR " should be free.", c, arena);

        try_specific_chunk:
            if (VALLOC_UNLIKELY(c->Size < roundSize))
                continue;
            else if (c->Size == roundSize)
            {
                //  Needs to be entirely re-purposed.

                c->Flags = ChunkFlags::None;

                if (c->PrevFree != nullptr)
                    c->PrevFree->NextFree = c->AsFree()->NextFree;

                if (c->AsFree()->NextFree != nullptr)
                    c->AsFree()->NextFree->PrevFree = c->PrevFree;
                else
                    arena->LastFree = c->PrevFree;
                //  Okey dokey?

                if (arena->LastBusy < c)
                    arena->LastBusy = c->AsBusy();

                c->Owner = arena;
            }
            else
            {
                //  Will fit more.

                c->Size -= roundSize;
                //  Reduce.

                c = new (c->GetNext()) BusyChunk(arena, c, roundSize);
                //  Create a new used chunk at the end of this free chunk.

                if (c->GetNext() < arena->GetEnd())
                    c->GetNext()->Prev = c;
                    //  The next one cannot be free, so this new one cannot be the last busy one.
                else
                    arena->LastBusy = c->AsBusy();
            }

            if ((arena->Free -= roundSize) == 0)
                if (CollectGarbage(arena, roundSize) == NoGarbage)
                    RetireArena(arena);
            //  It's full. Will be retired if it contains no garbage.

            VALLOC_ASSERT_MSG(c->Owner != nullptr
                , "About to return chunk " VF_PTR " from " VF_PTR " BY " VF_PTR " has a null owner."
                , c, arena, &TD);

            return c->GetContents();
        }

    no_space:
        if (CollectGarbage(arena, roundSize, c) == TargetReached)
            goto try_specific_chunk;
        //  If the target was reached through collection, proceed.

        if (ExtendArena(arena))
            goto try_chunks;

        //  Still won't fit. Maybe this needs retiring? TODO: Some heuristic?
    } while ((arena = arena->Next) != nullptr);

with_new_arena:
    arena = AllocateArena();

    if (VALLOC_UNLIKELY(arena == nullptr))
        return nullptr;

    Chunk * const c = arena->LastBusy = new (arena->LastFree) BusyChunk(arena, nullptr, roundSize);
    arena->LastFree = new (c->GetNext()) FreeChunk(nullptr, c, arena->Free -= roundSize, nullptr);

    //  Normally, allocations are served at the end of free chunks.
    //  This means that the chunks after it don't need their PrevFree updated.
    //  When an arena is freshly-allocated, it's served from the beginning, to
    //  avoid too many on-demand allocations. Will save a page at best, but still
    //  worth it.

    VALLOC_ASSERT_MSG(c->Owner != nullptr
        , "About to return chunk " VF_PTR " from " VF_PTR " BY " VF_PTR " has a null owner."
        , c, arena, &TD);

    return c->GetContents();
}

/*******************************
    Valloc::DeallocateMemory    >-----------------------------------------------
*******************************/

void Valloc::DeallocateMemory(void * ptr, bool crash)
{
    Chunk * const c = Chunk::FromContents(ptr);

    VALLOC_ASSERT_MSG(reinterpret_cast<uintptr_t>(ptr) % Platform::CacheLineSize == sizeof(Chunk)
            , "Misaligned pointer: " VF_PTR, ptr);

    if (VALLOC_UNLIKELY(!c->IsBusy()))
    {
        Platform::ErrorMessage("Attempted to free a " VF_STR " chunk: " VF_PTR
            , c->IsBusy() ? "busy" : c->IsFree() ? "free" : "queued"
            , c);

        if (crash)
            VALLOC_ABORT();
        else
            return;
    }

    Arena * const arena = c->Owner;

    VALLOC_ASSERT_MSG(arena != nullptr, "Chunk " VF_PTR " has a null owner; it is " VF_STR
        , c
        , c->IsBusy() ? "busy" : c->IsFree() ? "free" : "queued");

    ThreadData * owner = arena->Owner;

    if (VALLOC_LIKELY(owner == &TD))
    {
        // Platform::ErrorMessage("Freeing " VF_PTR " from arena " VF_PTR " BY " VF_PTR
        //     , c, arena, owner);

    free_my_own:
        //  Easiest case, freeing a chunk in an arena owned by this thread.

        FreeThisChunk(arena, c, arena->GetEnd());

        // arena->Dump(Platform::ErrorMessage);

        if (arena->IsEmpty())
            DeallocateArena(arena);
    }
    else
    {
        // Platform::ErrorMessage("Freeing " VF_PTR " from arena " VF_PTR " of " VF_PTR " BY " VF_PTR
        //     , c, arena, owner, &TD);

        bool locked = false;

    retry:

        if (VALLOC_LIKELY(owner != nullptr))
        {
            if (VALLOC_UNLIKELY(locked))
                GLock.Release();

            c->Flags = ChunkFlags::Queued;

            Chunk * top = arena->FreeList.Pointer;

            do c->NextInList = top; while (!arena->FreeList.CAS(top, c));
        }
        else
        {
            if (VALLOC_LIKELY(locked))
            {
                PromoteArena(arena);

                GLock.Release();

                AddToMine(arena);
                arena->Prev = nullptr;

                CollectGarbage(arena, 0);

                goto free_my_own;
            }
            else
            {
                GLock.Acquire();
                locked = true;

                owner = arena->Owner;

                goto retry;
            }
        }
    }
}

/************************************
    Valloc::AllocateAlignedMemory    >------------------------------------------
************************************/

void * Valloc::AllocateAlignedMemory(size_t size, size_t mul, size_t off)
{

}

/*******************************
    Valloc::ResizeAllocation    >-----------------------------------------------
*******************************/

void * Valloc::ResizeAllocation(void * ptr, size_t size, bool crash)
{
    Chunk * const c = Chunk::FromContents(ptr);

    VALLOC_ASSERT_MSG(reinterpret_cast<uintptr_t>(ptr) % Platform::CacheLineSize == sizeof(Chunk)
            , "Misaligned pointer: " VF_PTR, ptr);

    size_t const roundSize = RoundUp(size + sizeof(Chunk), Platform::CacheLineSize);
    ssize_t const sizeDiff = (ssize_t)(roundSize) - (ssize_t)(c->Size);

    if (sizeDiff == 0)
        return ptr;

    if (VALLOC_UNLIKELY(!c->IsBusy()))
    {
        Platform::ErrorMessage("Attempted to resize a " VF_STR " chunk: " VF_PTR
            , c->IsBusy() ? "busy" : c->IsFree() ? "free" : "queued"
            , c);

        if (crash)
            VALLOC_ABORT();
        else
            return nullptr;
    }

    Arena * const arena = c->Owner;
    ThreadData * owner = arena->Owner;

    if (VALLOC_LIKELY(owner == &TD))
    {
    resize_my_own:
        //  Hardest case, resizing a chunk in an arena owned by this thread.

        Chunk * next = c->GetNext();
        void const * const arenaEnd = arena->GetEnd();

        if (next >= arenaEnd)
            goto allocate_new;

        if (VALLOC_LIKELY(sizeDiff > 0))
        {
            //  Enlarging?

            if (VALLOC_LIKELY(!next->IsFree() || next->Size < (size_t)sizeDiff))
                goto allocate_new;
            //  Next one's not free? Not gonna bother checking the previous...
            //  Next one's too small? Ditto.

            c->Size = roundSize;

            arena->Free -= sizeDiff;

            if unlikely((ssize_t)(next->Size) == sizeDiff)
            {
                if (next->PrevFree != nullptr)
                {
                    if ((next->PrevFree->NextFree = next->AsFree()->NextFree) != nullptr)
                        next->AsFree()->NextFree->PrevFree = next->PrevFree;
                    else
                        arena->LastFree = next->PrevFree;
                }
                else if (next->AsFree()->NextFree != nullptr)
                    next->AsFree()->NextFree->PrevFree = nullptr;

                if (next->GetNext() < arenaEnd)
                    next->GetNext()->Prev = c;
            }
            else
                //  Reclaim the rest of the space.
                RelinkFreeChunk(arena, new (c->GetNext()) FreeChunk(next->PrevFree, c, next->Size - sizeDiff, next->AsFree()->NextFree));

            //  I think that's all..?
        }
        else
        {
            //  Shrinking? Two possibilities here.

            c->Size = roundSize;

            arena->Free -= sizeDiff;

            if (next->IsFree())
                //  Push back the next free chunk.
                RelinkFreeChunk(arena, new (c->GetNext()) FreeChunk(next->PrevFree, c, next->Size - sizeDiff, next->AsFree()->NextFree));
            else
            {
                FreeChunk * const prev = c->Prev->AsFree();

                if (prev->IsFree())
                {
                    //  Previous one is free, means linkage can be patched more efficiently.

                    next = new (c->GetNext()) FreeChunk(prev, c, (size_t)(-sizeDiff), prev->NextFree);

                    prev->NextFree = next->AsFree();

                    if (next->AsFree()->NextFree != nullptr)
                        next->AsFree()->NextFree->PrevFree = next->AsFree();
                    else
                        arena->LastFree = next->AsFree();
                }
                else
                    //  Or properly introduce the free chunk, possibly more slowly...
                    IntroduceFreeChunk(arena, new (c->GetNext()) FreeChunk(nullptr, c, (size_t)(-sizeDiff), nullptr));
            }
        }

        return ptr;

    allocate_new:
        void * const other = Valloc::AllocateMemory(size);
        //  Allocate a new one locally.  

        if (VALLOC_UNLIKELY(other == nullptr))
            return nullptr;

        memcpy(other, ptr, Minimum(size, c->Size - sizeof(Chunk)));
        //  Transfer the needed data.

        FreeThisChunk(arena, c, arena->GetEnd());

        if (arena->IsEmpty())
            DeallocateArena(arena);
        //  The new location may have been allocated in a different arena.

        return other;
    }
    else
    {
        bool locked = false;

    retry:
        //  It's important to note, that after the lock is acquired, the possible
        //  new owner cannot be this thread.

        if (VALLOC_LIKELY(owner != nullptr))
        {
            //  It's owned by another thread, so the chunk must simply be queued for
            //  freeing.

            if (VALLOC_UNLIKELY(locked))
                GLock.Release();
            //  If the lock is taken, and this arena was taken by another thread,
            //  the lock is released and queuing proceeds normally.

            void * const other = Valloc::AllocateMemory(size);
            //  Allocate a new one locally.

            if (VALLOC_UNLIKELY(other == nullptr))
                return nullptr;

            memcpy(other, ptr, Minimum(size, c->Size - sizeof(Chunk)));
            //  Transfer the needed data.

            c->Flags = ChunkFlags::Queued;

            Chunk * top = arena->FreeList.Pointer;

            do c->NextInList = top; while (!arena->FreeList.CAS(top, c));

            //  Queue up the old one for deleteion.

            return other;
        }
        else
        {
            //  It's global, it needs synchronization. It might even change owner in
            //  the meantime.

            if (VALLOC_LIKELY(locked))
            {
                //  It has already been locked and has no owner.

                PromoteArena(arena);

                GLock.Release();

                //  Now the arena is not in the global list anymore, and is owned
                //  by the current thread.

                AddToMine(arena);
                arena->Prev = nullptr;
                //  Now it's in this thread's list, officially.

                CollectGarbage(arena, 0);
                //  Collect all the garbage that sneaked in.

                goto resize_my_own;
            }
            else
            {
                //  Not locked: The lock needs to be acquired first.

                GLock.Acquire();
                locked = true;

                owner = arena->Owner;
                //  Get the new owner, which cannot change now.

                goto retry;
            }
        }
    }
}

/*******************************
    Valloc::CollectMyGarbage    >-----------------------------------------------
*******************************/

void Valloc::CollectMyGarbage()
{
    Arena * arena, * next;

    if (VALLOC_UNLIKELY((arena = TD.FirstArena) == nullptr))
        return;

    do
    {
        VALLOC_ASSERT_MSG(reinterpret_cast<uintptr_t>(arena) % Platform::PageSize == 0
            , "Misaligned arena..? " VF_PTR, arena);

        auto res = CollectGarbage(arena, 0);

        switch (res)
        {
        case NoGarbage:
            Platform::ErrorMessage("No garbage in " VF_PTR " of " VF_PTR, arena, &TD);
            break;
        case CollectedAll:
            Platform::ErrorMessage("Collected all from " VF_PTR " of " VF_PTR, arena, &TD);
            break;
        case TargetReached:
            Platform::ErrorMessage("WTF? - " VF_PTR " of " VF_PTR, arena, &TD);
            break;
        }

        next = arena->Next;

        if (arena->IsEmpty())
            DeallocateArena(arena);
    } while ((arena = next) != nullptr);
}
