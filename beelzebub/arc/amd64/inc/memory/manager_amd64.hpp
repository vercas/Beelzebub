#pragma once

#include <memory/manager.hpp>
#include <memory/virtual_allocator.hpp>
#include <synchronization/spinlock.hpp>
#include <synchronization/atomic.hpp>
#include <handles.h>
#include <metaprogramming.h>

using namespace Beelzebub::Synchronization;

namespace Beelzebub { namespace Memory
{
    /**
     *  A memory manager for AMD64.
     */
    class MemoryManagerAmd64 : public MemoryManager
    {
    public:

        /*  Statics  */

        static vaddr_t const IsaDmaStart          = 0xFFFF800000000000ULL;
        static vsize_t const IsaDmaLength         = 1ULL << 24;  //  16 MiB
        static vaddr_t const IsaDmaEnd            = IsaDmaStart + IsaDmaLength;

        static vaddr_t const KernelModulesStart   = IsaDmaEnd;
        static vaddr_t const KernelModulesEnd     = 0xFFFF808000000000ULL;
        static vsize_t const KernelModulesLength  = KernelModulesEnd - KernelModulesStart;

        static vaddr_t const PasDescriptorsStart  = KernelModulesEnd;
        static vaddr_t const PasDescriptorsEnd    = 0xFFFF820000000000ULL;
        static vsize_t const PasDescriptorsLength = PasDescriptorsEnd - PasDescriptorsStart;

        static vaddr_t const HandleTablesStart    = PasDescriptorsEnd;
        static vaddr_t const HandleTablesEnd      = 0xFFFF830000000000ULL;
        static vsize_t const HandleTablesLength   = HandleTablesEnd - HandleTablesStart;

        static vaddr_t const KernelHeapStart      = HandleTablesEnd;
        static vaddr_t const KernelHeapEnd        = 0xFFFFFE0000000000ULL;
        static vsize_t const KernelHeapLength     = KernelHeapEnd - KernelHeapStart;
        static vsize_t const KernelHeapPageCount  = KernelHeapLength >> 12;

        static Atomic<vaddr_t> KernelModulesCursor;
        static Spinlock KernelModulesLock;

        static Atomic<vaddr_t> PasDescriptorsCursor;
        static Spinlock PasDescriptorsLock;

        static Spinlock HandleTablesLock;

        static Atomic<vaddr_t> KernelHeapCursor;
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
