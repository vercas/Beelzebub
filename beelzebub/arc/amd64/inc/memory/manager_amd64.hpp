#pragma once

#include <memory/manager.hpp>
#include <memory/virtual_allocator.hpp>
#include <synchronization/spinlock.hpp>
#include <handles.h>
#include <metaprogramming.h>

namespace Beelzebub { namespace Memory
{
    /**
     *  A memory manager for AMD64.
     */
    class MemoryManagerAmd64 : public MemoryManager
    {
    public:

        /*  Statics  */

        static const vaddr_t IsaDmaStart          = 0xFFFF800000000000ULL;
        static const vsize_t IsaDmaLength         = 1ULL << 24;  //  16 MiB
        static const vaddr_t IsaDmaEnd            = IsaDmaStart + IsaDmaLength;

        static const vaddr_t KernelModulesStart   = IsaDmaEnd;
        static const vaddr_t KernelModulesEnd     = 0xFFFF808000000000ULL;
        static const vsize_t KernelModulesLength  = KernelModulesEnd - KernelModulesStart;

        static const vaddr_t PasDescriptorsStart  = KernelModulesEnd;
        static const vaddr_t PasDescriptorsEnd    = 0xFFFF820000000000ULL;
        static const vsize_t PasDescriptorsLength = PasDescriptorsEnd - PasDescriptorsStart;

        static const vaddr_t HandleTablesStart    = PasDescriptorsEnd;
        static const vaddr_t HandleTablesEnd      = 0xFFFF830000000000ULL;
        static const vsize_t HandleTablesLength   = HandleTablesEnd - HandleTablesStart;

        static const vaddr_t KernelHeapStart      = HandleTablesEnd;
        static const vaddr_t KernelHeapEnd        = 0xFFFFFE0000000000ULL;
        static const vsize_t KernelHeapLength     = KernelHeapEnd - KernelHeapStart;
        static const vsize_t KernelHeapPageCount  = KernelHeapLength >> 12;

        static volatile vaddr_t KernelModulesCursor;
        static Spinlock KernelModulesLock;

        static volatile vaddr_t PasDescriptorsCursor;
        static Spinlock PasDescriptorsLock;

        static Spinlock HandleTablesLock;

        static volatile vaddr_t KernelHeapCursor;
        static volatile size_t KernelHeapLockCount;
        static Spinlock KernelHeapMasterLock;

        static Spinlock KernelBinariesLock;

        /*  Constructors  */

        MemoryManagerAmd64() = default;
        MemoryManagerAmd64(MemoryManagerAmd64 const &) = delete;
        MemoryManagerAmd64 & operator =(const MemoryManagerAmd64 &) = delete;

        __bland __forceinline MemoryManagerAmd64(VirtualAllocationSpace * const vas)
            : Vas(vas)
            , UserLock()
            , UserHeapCursor(1 << 12)
        {

        }

        /*  CPU data mapping  */

        __bland vaddr_t GetCpuDataLocation();

        /*  Components  */

        VirtualAllocationSpace * Vas;

        /*  Locks  */

        Spinlock UserLock;

        /*  Cursors  */

        volatile vaddr_t UserHeapCursor;
    };
}}
