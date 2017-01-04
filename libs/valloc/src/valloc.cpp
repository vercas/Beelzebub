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

using namespace Valloc;

static __thread ThreadData TD;

Lock GLock {};
Arena * GList = nullptr;

//  Eh.

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
    // Platform::ErrorMessage("Removing arena " VF_PTR " from " VF_PTR, arena, &TD);

    arena->PopFromList();
    if (TD.FirstArena == arena)
        TD.FirstArena = arena->Next;
    //  Patch linkage of owned arenas.

    return arena;
}

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

    // Platform::ErrorMessage("Allocated arena " VF_PTR " for " VF_PTR, addr, &TD);

    return AddToMine(new (addr) Arena(&TD, size));
}

static void DeallocateArena(Arena * const arena)
{
    RemoveFromMine(arena);

    Platform::FreeMemory(arena, arena->Size);
}

static void RetireArena(Arena * arena)
{
    // Platform::ErrorMessage("Retiring arena " VF_PTR " of " VF_PTR, arena, &TD);

    arena->Owner = nullptr;

    RemoveFromMine(arena);

    arena->Prev = nullptr;

    GLock.Acquire();

    arena->Next = GList;
    GList = arena;

    GLock.Release();

    //  And such, the job is done.
}

//  Vital note: This skips the first chunk; starts from the next.
static void PatchPrevFree(Chunk * c, Chunk * const free, void const * const arenaEnd)
{
    // Platform::ErrorMessage("Patch! " VF_PTR " " VF_PTR " " VF_PTR, c, free, arenaEnd);

again:
    c = c->GetNext();

    if (c < arenaEnd)
    {
        c->PrevFree = free;

        if (!c->IsFree())
            goto again;
        //  Will stop after the first free chunk.
    }

    //  Or once it reaches the end of the arena.
}

static size_t FreeChunk(Arena * arena, Chunk * c, void const * const arenaEnd)
{
    Chunk * const p = c->PrevFree, * const n = c->GetNext();

    if (p != nullptr && p->GetNext() == c)
    {
        if (n < arenaEnd && n->IsFree())
        {
            //  Surrounded on both sides by free chunks.

            p->Size += c->Size + n->Size;
            //  Must include both.

            PatchPrevFree(p, p, arenaEnd);

            if (arena->LastFree == n)
                arena->LastFree = p;
            //  Push back the last free if the next one was the last.
        }
        else
        {
            //  Previous is free, next is not.

            p->Size += c->Size;

            //  No need to patch linkage, because everything already points to
            //  the previous free chunk. No need to check the last free either.
        }

        c = p;
    }
    else
    {
        c->Owner = arena;
        c->Flags = ChunkFlags::Free;

        if (n < arenaEnd && n->IsFree())
        {
            c->Size += n->Size;

            if (arena->LastFree == n)
                arena->LastFree = c;
        }
        else if (arena->LastFree < c)
            arena->LastFree = c;

        PatchPrevFree(c, c, arenaEnd);
    }

    arena->Free += c->Size;

    return c->Size;
}

enum CollectionResult { NoGarbage, TargetReached, CollectedAll, };

static CollectionResult CollectGarbage(Arena * arena, size_t const target)
{
    if (arena->FreeListEmpty())
        return NoGarbage;

    void const * const arenaEnd = arena->GetEnd();
    Chunk * first = arena->FreeList.Swap(nullptr);
    Chunk * cur = first;

    Platform::ErrorMessage("Collecting garbage for " VF_PTR ": " VF_PTR
        , arena, first);

    // arena->Dump(Platform::ErrorMessage);

    do
    {
        size_t const freeSize = FreeChunk(arena, cur, arenaEnd);

        if (VALLOC_LIKELY(target != 0) && freeSize >= target)
        {
            if (VALLOC_LIKELY((cur = first = cur->NextInList) != nullptr))
            {
                Chunk * last = nullptr;

                do last = cur; while ((cur = cur->NextInList) != nullptr);

                Chunk * top = arena->FreeList.Pointer;

                do last->NextInList = top; while (!arena->FreeList.CAS(top, first));
            }

            return TargetReached;
        }
    } while ((cur = cur->NextInList) != nullptr);

    return CollectedAll;
}

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

        if (VALLOC_UNLIKELY(arena->Free < roundSize))
        {
            //  Won't fit. Maybe there's garbage to collect?

            switch (CollectGarbage(arena, roundSize))
            {
            case TargetReached:
                //  If the target is reached, it's time to try the chunks.
                goto try_chunks;

            case CollectedAll: continue;
            default: break;
            }

            //  Still won't fit. Maybe this needs retiring? TODO: Some heuristic?
            //  TODO: Attempt to enlarge?

            continue;
        }

    try_chunks:
        Chunk * c = arena->LastFree;

        do
        {
            VALLOC_ASSERT_MSG(c->IsFree()
                , "Chunk " VF_PTR " from " VF_PTR " should be free.", c, arena);

            if (c->Size < roundSize)
                continue;
            else if (c->Size == roundSize)
            {
                //  Needs to be entirely re-purposed.

                c->Flags = ChunkFlags::None;

                PatchPrevFree(c, c->PrevFree, arena->GetEnd());

                if (arena->LastFree == c)
                    arena->LastFree = c->PrevFree;
                //  Okey dokey?

                // Platform::ErrorMessage("Repurposed " VF_PTR " (" VF_PTR ") from arena " VF_PTR " of " VF_PTR
                //     , c, (uintptr_t)(c->Size), arena, &TD);
            }
            else
            {
                //  Will fit more.

                c->Size -= roundSize;
                //  Reduce.

                c = new (c->GetNext()) Chunk(arena, roundSize, false, c);
                //  Create a new used chunk at the end of this free chunk.

                // Platform::ErrorMessage("Sliced " VF_PTR " from " VF_PTR " (" VF_PTR ") in arena " VF_PTR " of " VF_PTR
                //     , c, c->PrevFree, (uintptr_t)(c->PrevFree->Size), arena, &TD);
            }

            if ((arena->Free -= roundSize) == 0)
                if (CollectGarbage(arena, roundSize) == NoGarbage)
                    RetireArena(arena);
            //  It's full. Will be retired if it contains no garbage.

            // Platform::ErrorMessage("Allocated " VF_PTR " from arena " VF_PTR " of " VF_PTR
            //     , c, arena, &TD);

            // arena->Dump(Platform::ErrorMessage);

            return c->GetContents();
        } while ((c = c->PrevFree) != nullptr);

        if (CollectGarbage(arena, roundSize) == TargetReached)
            goto try_chunks;
    } while ((arena = arena->Next) != nullptr);

with_new_arena:
    arena = AllocateArena();

    if (VALLOC_UNLIKELY(arena == nullptr))
        return nullptr;

    Chunk * const c = new (arena->LastFree) Chunk(arena, roundSize, false);
    arena->LastFree = new (c->GetNext()) Chunk(arena, arena->Free - roundSize, true);

    //  Normally, allocations are served at the end of free chunks.
    //  This means that the chunks after it don't need their PrevFree updated.
    //  When an arena is freshly-allocated, it's served from the beginning, to
    //  avoid too many on-demand allocations. Will save a page at best, but still
    //  worth it.

    // c->Print(Platform::ErrorMessage);
    // c->GetNext()->Print(Platform::ErrorMessage);
    // arena->Dump(Platform::ErrorMessage);

    return c->GetContents();
}

void * Valloc::AllocateAlignedMemory(size_t size, size_t mul, size_t off)
{

}

void * Valloc::ResizeAllocation(void * ptr, size_t size)
{

}

void Valloc::DeallocateMemory(void * ptr, bool crash)
{
    (void)crash;

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
    ThreadData * owner = arena->Owner;

    if (VALLOC_LIKELY(owner == &TD))
    {
        // Platform::ErrorMessage("Freeing " VF_PTR " from arena " VF_PTR " BY " VF_PTR
        //     , c, arena, owner);

    free_my_own:
        //  Easiest case, freeing a chunk in an arena owned by this thread.

        FreeChunk(arena, c, arena->GetEnd());

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

            c->Flags = ChunkFlags::Queued;

            Chunk * top = arena->FreeList.Pointer;

            do c->NextInList = top; while (!arena->FreeList.CAS(top, c));
        }
        else
        {
            //  It's global, it needs synchronization. It might even change owner in
            //  the meantime.

            if (VALLOC_LIKELY(locked))
            {
                //  It has already been locked and has no owner.

                arena->PopFromList();
                if (GList == arena)
                    GList = arena->Next;
                //  Patch linkage of global arenas.

                arena->Owner = &TD;

                GLock.Release();

                //  Now the arena is not in the global list anymore, and is owned
                //  by the current thread.

                AddToMine(arena);
                arena->Prev = nullptr;
                //  Now it's in this thread's list, officially.

                CollectGarbage(arena, 0);
                //  Collect all the garbage that sneaked in.

                goto free_my_own;
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

void Valloc::CollectMyGarbage()
{
    Arena * arena;

    if (VALLOC_UNLIKELY((arena = TD.FirstArena) == nullptr))
        return;

    do
    {
        VALLOC_ASSERT_MSG(reinterpret_cast<uintptr_t>(arena) % Platform::PageSize == 0
            , "Misaligned arena..? " VF_PTR, arena);

        CollectGarbage(arena, 0);
    } while ((arena = arena->Next) != nullptr);
}
