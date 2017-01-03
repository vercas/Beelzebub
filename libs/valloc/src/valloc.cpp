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

Lock GLock;
Arena * GList;

//  Eh.

static Arena * AddToMine(Arena * const arena)
{
    if ((arena->Next = TD.FirstArena) != nullptr)
        arena->Next->Prev = arena;

    TD.FirstArena = arena;

    return arena;
}

static Arena * RemoveFromMine(Arena * const arena)
{
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

    return AddToMine(new (addr) Arena(&TD, size));
}

static void DeallocateArena(Arena * const arena)
{
    RemoveFromMine(arena);

    Platform::FreeMemory(arena, arena->Size);
}

static void RetireArena(Arena * arena)
{
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

    for (;;)
    {
        c = c->GetNext();

        if (c < arenaEnd && !c->IsFree())
            c->PrevFree = free;
        else
            break;

        //  Will stop at the first free chunk.
    }
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

            PatchPrevFree(n, p, arenaEnd);
        }
        else
        {
            //  Previous is free, next is not.

            p->Size += c->Size;

            PatchPrevFree(c, p, arenaEnd);
            //  c can be used as a starting point here because its size is
            //  still valid.
        }

        c = p;
    }
    else
    {
        c->Flags = ChunkFlags::Free;

        if (n < arenaEnd && n->IsFree())
            c->Size += n->Size;

        PatchPrevFree(c, c, arenaEnd);
    }

    arena->Free += c->Size;

    if (arena->LastFree < c)
        arena->LastFree = c;

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
            if (c->Size < roundSize)
                continue;
            else if (c->Size == roundSize)
            {
                //  Needs to be entirely re-purposed.

                c->Flags = ChunkFlags::None;

                PatchPrevFree(c, c->PrevFree, arena->GetEnd());

                arena->LastFree = c->PrevFree;
                //  Okey dokey?
            }
            else
            {
                //  Will fit more.

                c->Size -= roundSize;
                //  Reduce.

                c = new (c->GetNext()) Chunk(arena, roundSize, false, c);
                //  Create a new used chunk at the end of this free chunk.
            }

            if ((arena->Free -= roundSize) == 0)
                if (CollectGarbage(arena, roundSize) == NoGarbage)
                    RetireArena(arena);
            //  It's full. Will be retired if it contains no garbage.

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

    if (VALLOC_UNLIKELY(!c->IsBusy()))
    {
        Platform::ErrorMessage("Attempted to free a non-busy chunk: " VF_PTR, ptr);

        if (crash)
            VALLOC_ABORT();
        else
            return;
    }

    Arena * const arena = c->Owner;
    ThreadData * owner = arena->Owner;

    if (VALLOC_LIKELY(owner == &TD))
    {
    free_my_own:
        //  Easiest case, freeing a chunk in an arena owned by this thread.

        FreeChunk(arena, c, arena->GetEnd());

        if (arena->IsEmpty())
            DeallocateArena(arena);
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

            Chunk * top = arena->FreeList.Pointer;

            do c->NextInList = top; while (!arena->FreeList.CAS(top, c));
        }
        else
        {
            //  It's global, it needs synchronization. It might even change owner in
            //  the meantime.

            if (locked)
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
