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
    struct ThreadData;

    /***************
        Pointers
    ***************/

#ifdef VALLOC_POINTER_48BIT

//     union SplitQword
//     {
//         uint64_t Whole;
//         uint16_t Words[4];

//         inline SplitQword(uint64_t const w, uint16_t const s)
//             : Whole(w << 16)
//         { (void)s; }

//         inline SplitQword(uint16_t const s, uint64_t const w)
//             : Whole(w & 0x0000FFFFFFFFFFFF)
//         { (void)s; }

//         inline uint64_t GetTop48()    const { return this->Whole >> 16; }
//         inline uint64_t GetBottom48() const { return this->Whole & 0x0000FFFFFFFFFFFF; }

//         inline SplitQword operator ^(SplitQword const other) const { return {this->Whole ^ other->Whole}; }
//     };

//     template<typename T>
//     struct TopPointer
//     {
//         SplitQword Inner;

//         inline TopPointer(T * const val)
//             : Inner(reinterpret_cast<uint64_t>(val), 0)
//         { }

//         inline T * GetPointer() const { return reinterpret_cast<T *>(this->Inner.GetTop48()); }

//         inline TopPointer operator ^(TopPointer const other) { return this->Inner ^ other.Inner; }
//     }

//     template<typename T>
//     struct BottomPointer
//     {
//         SplitQword Inner;

//         inline TopPointer(T * const val)
//             : Inner(0, reinterpret_cast<uint64_t>(val))
//         { }

//         inline T * GetPointer() const { return reinterpret_cast<T *>(this->Inner.GetBottom48()); }

//         inline BottomPointer operator ^(BottomPointer const other) { return this->Inner ^ other.Inner; }
//     }

#endif

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
        };

        Chunk * PrevFree;
        size_t Size;
        ChunkFlags Flags;

        inline explicit Chunk(Arena * const o)
            : Owner( o)
            , PrevFree(nullptr)
            , Size(0)
            , Flags(ChunkFlags::Free)
        {

        }

        inline Chunk(Arena * const o, size_t const s, bool const f, Chunk * const p = nullptr)
            : Owner( o)
            , PrevFree(p)
            , Size(s)
            , Flags(f ? ChunkFlags::Free : ChunkFlags::None)
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
            return f(VF_STR "[Chunk " VF_PTR "->" VF_PTR " " VF_STR "; " VF_PTR "]"
                , indent ? "\t" : ""
                , this, this->GetNext()
                , this->IsBusy() ? "B" : this->IsFree() ? "F" : "Q"
                , this->PrevFree);
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
        GenerationalPointer<Chunk> FreeList;
        size_t Size, Free;
        Arena * Prev, * Next;
        ThreadData * Owner;
        Chunk * LastFree;

        inline explicit Arena(ThreadData * const o, size_t const s)
            : FreeList( nullptr)
            , Size(s), Free(s - ARENA_SIZE)
            , Prev(nullptr), Next(nullptr)
            , Owner(o)
            , LastFree(reinterpret_cast<Chunk *>(PointerAdd(this, ARENA_SIZE)))
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
