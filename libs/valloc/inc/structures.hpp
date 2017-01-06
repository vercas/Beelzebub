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

#pragma once

#include <valloc/platform.hpp>

namespace Valloc
{
    struct Arena;
    struct Chunk;
    struct FreeChunk;
    struct BusyChunk;
    struct ThreadData;

    /***************
        Pointers
    ***************/

    template<typename T>
    struct AtomicPointer
    {
        T * Pointer;

        inline explicit AtomicPointer(T * const p)
            : Pointer( p)
        {
            //  Eh.
        }

        inline T * Swap(T * const des)
        {
            return Platform::Swap<T *>(&(this->Pointer), des);
        }

        inline bool CAS(T * & exp, T * const des)
        {
            return Platform::CAS<T *>(&(this->Pointer), exp, des);
        }
    };

    template<typename T>
    struct GenerationalPointer
    {
        T * Pointer;
        uintptr_t Generation;

        inline explicit GenerationalPointer(T * const p)
            : Pointer( p)
            , Generation(0)
        {
            //  Eh.
        }

        inline T * Swap(T * const des)
        {
            return Platform::Swap<T *>(&(this->Pointer), des);
        }

        inline bool CAS(T * & exp, T * const des)
        {
            return Platform::CAS<T *>(&(this->Pointer), exp, des);
        }

        inline bool CAS(T * & exp, T * const des, uintptr_t & gen)
        {
            return Platform::CAS2<GenerationalPointer<T>, T *, uintptr_t>(this, exp, gen, des, gen + 1);
        }
    };

    /**************
        Headers
    **************/

    enum class ChunkFlags : size_t
    {
        None = 0,
        Free = 1,
        Queued = 2,
    };

    VALLOC_ENUMOPS_FULL(ChunkFlags, size_t)

    struct Chunk
    {
        static inline Chunk * FromContents(void const * const ptr)
        {
            return (Chunk *)PointerSub(ptr, sizeof(Chunk));
        }

        VALLOC_ANONYMOUS union
        {
            Arena * Owner;
            Chunk * NextInList;
            FreeChunk * PrevFree;
        };

        Chunk * Prev;
        size_t Size;
        ChunkFlags Flags;

        inline explicit Chunk(Arena * const o)
            : Owner( o)
            , Prev(nullptr)
            , Size(0)
            , Flags(ChunkFlags::Free)
        {

        }

        inline Chunk(Arena * const o, Chunk * const p, size_t const s, ChunkFlags const f)
            : Owner( o)
            , Prev(p)
            , Size(s)
            , Flags(f)
        {

        }

        inline Chunk(FreeChunk * const pf, Chunk * const p, size_t const s, ChunkFlags const f)
            : PrevFree( pf)
            , Prev(p)
            , Size(s)
            , Flags(f)
        {

        }

        inline Chunk * GetNext() const
        {
            return const_cast<Chunk *>(PointerAdd(this, this->Size));
        }

        inline void * GetContents() const
        {
            return (void *)PointerAdd(this, sizeof(Chunk));
        }

        inline bool IsBusy() const { return this->Flags == ChunkFlags::None; }
        inline bool IsFree() const { return this->Flags == ChunkFlags::Free; }

        inline void Print(PrintFunction f, bool indent = false) const
        {
            return f(VF_STR "[Chunk " VF_PTR "<-" VF_PTR "->" VF_PTR " " VF_STR "; " VF_PTR "]"
                , indent ? "\t" : ""
                , this, this->Prev, this->GetNext()
                , this->IsBusy() ? "B" : this->IsFree() ? "F" : "Q"
                , this->Owner);
        }

        inline FreeChunk * AsFree()
        {
            return reinterpret_cast<FreeChunk *>(this);
        }
    };

    struct FreeChunk : public Chunk
    {
        FreeChunk * NextFree;

        inline FreeChunk(FreeChunk * const pf, Chunk * const p, size_t const s, FreeChunk * const nf)
            : Chunk( pf, p, s, ChunkFlags::Free)
            , NextFree(nf)
        {

        }

        inline void Print(PrintFunction f, bool indent = false) const
        {
            return f(VF_STR "[Chunk " VF_PTR "<-" VF_PTR "->" VF_PTR " " VF_STR "; " VF_PTR "; " VF_PTR "]"
                , indent ? "\t" : ""
                , this, this->Prev, this->GetNext()
                , this->IsBusy() ? "B" : this->IsFree() ? "F" : "Q"
                , this->Owner
                , this->NextFree);
        }
    };

    struct BusyChunk : public Chunk
    {
        inline BusyChunk(Arena * const o, Chunk * const p, size_t const s)
            : Chunk( o, p, s, ChunkFlags::None)
        {

        }
    };

#ifdef VALLOC_CAN_ALIGN
    #define ARENA_SIZE (sizeof(Arena))
#elif defined(VALLOC_SIZES_NONCONST)
    //  Yep, will be computed every tiem. :c
    #define ARENA_SIZE (RoundUp(sizeof(Arena), Platform::CacheLineSize))
#else
    #define ARENA_SIZE (RoundUp(sizeof(Arena), VALLOC_CACHE_LINE_SIZE))
#endif

    struct Arena
    {
        Arena * Prev, * Next;
        size_t Size, Free;
        ThreadData * Owner;
        FreeChunk * LastFree;

#ifdef VALLOC_CACHE_LINE_SIZE
        uintptr_t Padding0[(VALLOC_CACHE_LINE_SIZE / sizeof(void *)) - 6];

        //  If possible, push the free list onto the next cache line.
        //  This allows accesses to it to not interfere with accesses to the
        //  rest of the data.
#endif

        AtomicPointer<Chunk> FreeList;

        inline explicit Arena(ThreadData * const o, size_t const s)
            : Prev( nullptr), Next(nullptr)
            , Size(s), Free(s - ARENA_SIZE)
            , Owner(o)
            , LastFree(reinterpret_cast<FreeChunk *>(PointerAdd(this, ARENA_SIZE)))
#ifdef VALLOC_CACHE_LINE_SIZE
            , Padding0()
#endif
            , FreeList(nullptr)
        {

        }

        inline Chunk * GetFirstChunk() const
        {
            return const_cast<Chunk *>(reinterpret_cast<Chunk const *>(PointerAdd(this, ARENA_SIZE)));
        }

        inline void const * GetEnd() const
        {
            return reinterpret_cast<void const *>(reinterpret_cast<uintptr_t>(this) + this->Size);
        }

        inline bool IsEmpty() const { return this->Free == this->Size - ARENA_SIZE; };

        inline bool FreeListEmpty() const { return this->FreeList.Pointer == nullptr; }

        inline void PopFromList()
        {
            if (this->Prev != nullptr)
                this->Prev->Next = this->Next;
            if (this->Next != nullptr)
                this->Next->Prev = this->Prev;
        }

        inline void Print(PrintFunction f) const
        {
            return f("[Arena " VF_PTR "->" VF_PTR "]", this, this->GetEnd());
        }

        inline void Dump(PrintFunction f) const
        {
            f("[Arena " VF_PTR "->" VF_PTR "; " VF_PTR " & " VF_PTR "; " VF_PTR " & " VF_PTR "]"
                , this, this->GetEnd()
                , this->Size, this->Free
                , this->LastFree, this->Owner);

            for (Chunk const * c = this->GetFirstChunk(); c < this->GetEnd(); c = c->GetNext())
                if (c->Flags == ChunkFlags::Free)
                    reinterpret_cast<FreeChunk const *>(c)->Print(f, true);
                else
                    c->Print(f, true);
        }
    }
#if defined(VALLOC_CACHE_LINE_SIZE) && defined(VALLOC_CAN_ALIGN)
    VALLOC_ALIGNED(VALLOC_CACHE_LINE_SIZE)
#endif
    ;

// #ifdef VALLOC_POINTER_48BIT
//     static_assert(sizeof(Chunk) == 2 * sizeof(void *), "Struct size mismatch.");
//     static_assert(sizeof(size_t) == sizeof(uint32_t), "Standard type size mismatch.");
// #else
    static_assert(sizeof(Chunk) == 4 * sizeof(void *), "Struct size mismatch.");
// #endif
    
#ifdef VALLOC_CACHE_LINE_SIZE
    static_assert(ARENA_SIZE % VALLOC_CACHE_LINE_SIZE == 0, "Arena header should be congruent to cache line size.");

    static_assert(sizeof(Chunk) < VALLOC_CACHE_LINE_SIZE, "Chunk header exceeds cache line size.");
#endif

    struct ThreadData
    {
        Arena * FirstArena = nullptr;
    };
}
