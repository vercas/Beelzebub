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

#pragma once

#include <beel/enums.kernel.hpp>
#include <beel/structs.kernel.hpp>
#include <synchronization/spinlock_uninterruptible.hpp>
#include <math.h>

namespace Beelzebub { namespace Memory
{
    /**
     * Represents possible statuses of a frame.
     */
    enum class FrameStatus : uint16_t
    {
        Free     =  0,
        Used     =  1,
        Split    =  2,
        Full     =  3,
        Reserved =  4,
    };

    /**
     * Describes a frame of memory.
     */
    struct FrameDescriptor
    {
        /*  Constants  */

        static constexpr FrameDescriptor * const Invalid
            = reinterpret_cast<FrameDescriptor *>(0x81);

        static __forceinline bool IsValid(FrameDescriptor const * const desc)
        {
            return desc != Invalid;
        }

        /*  Fields  */

        //  Number of references to this frame.
        uint32_t ReferenceCount;

        //  Frame status
        FrameStatus Status;

        /*  Constructors  */

        FrameDescriptor(FrameDescriptor const &) = delete;
        FrameDescriptor & operator =(FrameDescriptor const &) = delete;

        inline FrameDescriptor()
            : ReferenceCount( 0)
            , Status(FrameStatus::Free)
        {

        }

        /*  Reference count  */

        __forceinline uint32_t ResetReferenceCount()
        {
            return this->ReferenceCount = 0;
        }

        __forceinline uint32_t AdjustReferenceCount(int32_t diff)
        {
            return this->ReferenceCount += diff;
        }

        /*  Status  */

        __forceinline void Free()
        {
            this->Status = FrameStatus::Free;

            this->ResetReferenceCount();
        }

        __forceinline void Use(uint32_t const refCnt)
        {
            this->Status = FrameStatus::Used;

            __atomic_store_n(&(this->ReferenceCount), refCnt, __ATOMIC_SEQ_CST);
        }

        __forceinline void Reserve()
        {
            this->Status = FrameStatus::Reserved;

            this->ResetReferenceCount();
        }

        __forceinline bool IsSplit()
        {
            return this->Status == FrameStatus::Split || this->Status == FrameStatus::Full;
        }

        /*  Debug  */

    #ifdef __BEELZEBUB__DEBUG
        const char * GetStatusString() const
        {
            switch (this->Status)
            {
                case FrameStatus::Free:
                    return "Free";
                case FrameStatus::Used:
                    return "Used";
                case FrameStatus::Split:
                    return "Split";
                case FrameStatus::Full:
                    return "Full";
                case FrameStatus::Reserved:
                    return "Reserved";

                default:
                    return "UNKNOWN";
            }
        }

        const char GetStatusChar() const
        {
            switch (this->Status)
            {
                case FrameStatus::Free:
                    return 'F';
                case FrameStatus::Used:
                    return 'U';
                case FrameStatus::Split:
                    return 'S';
                case FrameStatus::Full:
                    return 'L';
                case FrameStatus::Reserved:
                    return 'R';

                default:
                    return 'X';
            }
        }

        // __forceinline Terminals::TerminalWriteResult PrintToTerminal(Terminals::TerminalBase * const term)
        // {
        //     return term->WriteFormat("|%c|D@%Xp|R-%X4|A-%X2|I-%X4|"
        //         , this->GetStatusChar(), this
        //         , this->ReferenceCount.Load()
        //         , this->Accesses.Load()
        //         , (uint32_t)this->StackIndex);
        // }
    #endif
    } __packed;

    /**
     * Describes a small frame of memory.
     */
    struct SmallFrameDescriptor : FrameDescriptor
    {
        /*  Constants  */

        static constexpr SmallFrameDescriptor * const Invalid
            = reinterpret_cast<SmallFrameDescriptor *>(0x81);

        static constexpr uint16_t const NullIndex = 0;

        /*  Fields  */

        //  Index of the next small frame in the larger frame's allocation pseudo-stack.
        uint16_t NextIndex;

        /*  Constructors  */

        SmallFrameDescriptor(SmallFrameDescriptor const &) = delete;
        SmallFrameDescriptor & operator =(SmallFrameDescriptor const &) = delete;

        inline SmallFrameDescriptor()
            : FrameDescriptor()
            , NextIndex(NullIndex)
        {

        }

        /*  Debug  */

    #ifdef __BEELZEBUB__DEBUG
        // __forceinline Terminals::TerminalWriteResult PrintToTerminal(Terminals::TerminalBase * const term)
        // {
        //     return term->WriteFormat("|%c|D@%Xp|R-%X4|A-%X2|I-%X4|"
        //         , this->GetStatusChar(), this
        //         , this->ReferenceCount.Load()
        //         , this->Accesses.Load()
        //         , (uint32_t)this->StackIndex);
        // }
    #endif
    } __packed;

    /**
     * Contains extra fields required by a large frame which is split into several
     * smaller frames.
     */
    struct SplitFrameExtra
    {
        /*  Constants  */

        static constexpr uint32_t const NullIndex = 0xFFFFFFFF;

        /*  Constructors  */

        inline SplitFrameExtra()
            : NextFree(SmallFrameDescriptor::NullIndex)
            , FreeCount(0)
            , PrevIndex(NullIndex)
        {

        }

        /*  Fields  */

        uint16_t NextFree;
        uint16_t FreeCount;

        uint32_t PrevIndex;
    };

    /**
     * Describes a large frame of memory.
     */
    struct LargeFrameDescriptor : FrameDescriptor
    {
        /*  Constants  */

        static constexpr LargeFrameDescriptor * const Invalid
            = reinterpret_cast<LargeFrameDescriptor *>(0x81);

        static constexpr uint32_t const NullIndex = SplitFrameExtra::NullIndex;

        static constexpr uint16_t const SubDescriptorsCount = 511;

        /*  Fields & Properties  */

        uint16_t Padding1;

        //  Index of the next large frame in the allocation pseudo-stack.
        uint32_t NextIndex;

        uint32_t Padding2;

        SmallFrameDescriptor * SubDescriptors;

        __forceinline SplitFrameExtra * GetExtras()
        {
            return reinterpret_cast<SplitFrameExtra *>(this->SubDescriptors);
        }

        /*  Constructors  */

        LargeFrameDescriptor(LargeFrameDescriptor const &) = delete;
        LargeFrameDescriptor & operator =(LargeFrameDescriptor const &) = delete;

        inline LargeFrameDescriptor()
            : FrameDescriptor()
            , NextIndex(NullIndex)
            , SubDescriptors(nullptr)
        {

        }

        /*  Debug  */

    #ifdef __BEELZEBUB__DEBUG
        // __forceinline Terminals::TerminalWriteResult PrintToTerminal(Terminals::TerminalBase * const term)
        // {
        //     return term->WriteFormat("|%c|D@%Xp|R-%X4|A-%X2|I-%X4|"
        //         , this->GetStatusChar(), this
        //         , this->ReferenceCount.Load()
        //         , this->Accesses.Load()
        //         , (uint32_t)this->StackIndex);
        // }
    #endif
    } __packed;

    static_assert(sizeof(FrameDescriptor) == 6, "Frame descriptor size mismatch.");
    static_assert(sizeof(SmallFrameDescriptor) == 8, "Small frame descriptor size mismatch.");
    static_assert(sizeof(SplitFrameExtra) <= sizeof(SmallFrameDescriptor), "Split frame extras should not be larger than a small frame descriptor.");
    static_assert(512 * sizeof(SmallFrameDescriptor) <= PageSize, "512 small frame descriptors should fit in a small page.");
    static_assert(sizeof(LargeFrameDescriptor) == 16 + sizeof(void *), "Large frame descriptor size mismatch.");

    /**
     * Manages a region of memory in which frames can be allocated.
     */
    class FrameAllocationSpace
    {
    public:

        /*  Proeprties  */

    #define PROP(type, name)                                 \
    private:                                                 \
        type name;                                           \
    public:                                                  \
        __forceinline type MCATS(Get, name)() const          \
        {                                                    \
            return this->name;                               \
        }
    #define CNST(type, name)                                 \
    public:                                                  \
        type const name;                                     \
        __forceinline type MCATS(Get, name)() const          \
        {                                                    \
            return this->name;                               \
        }

        PROP(paddr_t, MemoryStart)          //  Start of the allocation space.
        PROP(paddr_t, MemoryEnd)            //  End of the allocation space.
        PROP(psize_t, Size)                 //  Total number of bytes in the allocation space.

        PROP(paddr_t, AllocationStart)      //  Start of space which can be freely allocated.
        PROP(paddr_t, AllocationEnd)        //  End of space which can be freely allocated.
        PROP(psize_t, AllocableSize)        //  Total number of bytes which can be allocated.

        PROP(psize_t, ControlAreaSize)      //  Number of bytes used for control structures (large frame descriptors).
        PROP(psize_t, FreeSize)             //  Number of bytes in unallocated pages.
        PROP(psize_t, ReservedSize)         //  Number of bytes in reserved pages.

        PROP(psize_t, LargeFrameCount)      //  Total number of large frames in this space.

    public:

        /*  Constructors    */

        FrameAllocationSpace(paddr_t phys_start, paddr_t phys_end);

        FrameAllocationSpace(FrameAllocationSpace const &) = delete;
        FrameAllocationSpace & operator =(FrameAllocationSpace const &) = delete;

        /*  Frame manipulation  */

        __hot paddr_t AllocateFrame(Handle & desc, FrameSize size, uint32_t refCnt);

        __hot Handle Mingle(paddr_t addr, uint32_t & newCnt, int32_t diff, bool ignoreRefCnt);
        __cold Handle ReserveRange(paddr_t start, psize_t size, bool includeBusy);

        inline bool ContainsRange(paddr_t start, psize_t size) const
        {
            return ( start         >= this->AllocationStart)
                && ((start + size) <= this->AllocationEnd);
        }

        /*  Miscellaneous  */

        __hot LargeFrameDescriptor * GetDescriptor(paddr_t paddr);

        /*  Fields  */

        LargeFrameDescriptor * Map;
        //  Pointers to the allocation map within the space.

        Synchronization::SpinlockUninterruptible<> LargeLocker;
        Synchronization::SpinlockUninterruptible<> SplitLocker;

        uint32_t LargeFree;
        uint32_t SplitFree;

        FrameAllocationSpace * Next;
        FrameAllocationSpace * Previous;

        /*  Debug  */

    #ifdef __BEELZEBUB__DEBUG
        // __cold Terminals::TerminalWriteResult PrintStackToTerminal(Terminals::TerminalBase * const term, bool const details);
    #endif
    };

    /**
     *  Manages allocation of memory pages using a linked list of
     *  frame allocation spaces.
     */
    class FrameAllocator
    {
    public:

        /*  Constructors  */

        inline FrameAllocator()
            : ChainLock()
            , FirstSpace(nullptr)
            , LastSpace(nullptr)
        {

        }

        inline FrameAllocator(FrameAllocationSpace * first)
            : ChainLock()
            , FirstSpace(first)
            , LastSpace(first)
        {
            
        }

        FrameAllocator(FrameAllocator const &) = delete;
        FrameAllocator & operator =(FrameAllocator const &) = delete;

        /*  Page Manipulation  */

        __hot paddr_t AllocateFrame(Handle & desc, FrameSize size, AddressMagnitude magn, uint32_t refCnt);

        __hot Handle Mingle(paddr_t addr, uint32_t & newCnt, int32_t diff, bool ignoreRefCnt);
        __cold Handle ReserveRange(paddr_t start, psize_t size, bool includeBusy);

        bool ContainsRange(paddr_t start, psize_t size);
        __hot FrameAllocationSpace * GetSpace(paddr_t paddr);
        __hot LargeFrameDescriptor * GetDescriptor(paddr_t paddr);

        /*  Synchronization  */

        //  Used for mutual exclusion over the linking pointers of the
        //  allocation spaces.
        Synchronization::SpinlockUninterruptible<> ChainLock;

        /*  Space Chaining  */

        FrameAllocationSpace * FirstSpace;
        FrameAllocationSpace * LastSpace;

        __cold void PreppendAllocationSpace(FrameAllocationSpace * space);
        __cold void AppendAllocationSpace(FrameAllocationSpace * space);

        __cold void RemapLinks(vaddr_t oldAddr, vaddr_t newAddr);
    };

    /**
     *  The architecture-specific aspects of the physical memory manager.
     */
    class PmmArc
    {
    public:
        /*  Statics  */

        static paddr_t TempSpaceLimit;

        static FrameAllocationSpace * AllocationSpace;
        static FrameAllocator * MainAllocator;

        /*  Initialization  */

        static __cold Handle CreateAllocationSpace(paddr_t start, paddr_t end);

        /*  Relocation  */

        static __cold void Remap(FrameAllocator * & alloc, vaddr_t const oldVaddr, vaddr_t const newVaddr);
    };
}}
