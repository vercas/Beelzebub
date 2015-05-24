#pragma once

#include <metaprogramming.h>

namespace Beelzebub
{
    enum class HandleType : uint64_t
    {
        //  DESCRIPTION                           | // ABBREVIATION

        //  This be an invalid handle.
        Invalid              = 0x0000000000000000U, // INVL

        //  A miscellaneous/anonymous object belonging to the kernel.
        KernelObject         = 0x0001000000000000U, // KOBJ
        //  A miscellaneous/anonymous object belonging to a service.
        ServiceObject        = 0x0002000000000000U, // SOBJ
        //  A miscellaneous/anonymous object belonging to an application.
        ApplicationObject    = 0x0003000000000000U, // AOBJ

        //  A unit of execution.
        Thread               = 0x0010000000000000U, // THRD
        //  A unit of isolation.
        Process              = 0x0011000000000000U, // PRCS
        //  A unit of management.
        Job                  = 0x0012000000000000U, // JOB

        //  A virtual address space. ("linear" by x86 terminology)
        VirtualAddressSpace  = 0x0020000000000000U, // VASP

        //  An table which associates handles with resources.
        HandleTable          = 0xFFF0000000000000U, // HTBL
        //  A finite set of handles.
        MultiHandle          = 0xFFF1000000000000U, // MHND

        //  A general result code.
        Result               = 0xFFFF000000000000U, // RES
    };

    enum class HandleResult : uint64_t
    {
        //  DESCRIPTION                                | // SHORT NAME

        //  Saul Goodman!
        Okay                      = 0x0000000000000000U, // Okay
        //  Not enough memory available to complete an operation.
        OutOfMemory               = 0x0000000000000001U, // No mem.
        //  The requested operation isn't supported by the object/interface.
        UnsupportedOperation      = 0x0000000000000009U, // Unsp. Op.
        //  Operation not implemented by the object/interface.
        NotImplemented            = 0x000000000000000AU, // Not Impl.

        //  An argument given to a function/method is outside of the expected/supported range.
        ArgumentOutOfRange        = 0x0000000000000010U, // Arg. OOR
        //  An argument given to a function/method shouldn't be null.
        ArgumentNull              = 0x0000000000000011U, // Arg. Null

        //  An unknown format specifier was encountered in the format string.
        FormatBadSpecifier        = 0x0000000000000020U, // Frm. BSpc.
        //  The size given for a format argument is invalid.
        FormatBadArgumentSize     = 0x0000000000000021U, // Frm. BAS

        //  An operation was attempted on a range of pages that
        //  aren't covered by the page allocator.
        PagesOutOfAllocatorRange  = 0x0000000000000030U, // Pag OOAR
        //  An invalid operation was attempted on a reserved page.
        PageReserved              = 0x0000000000000031U, // Pag Res.
        //  An invalid operation was attempted on a free page.
        PageFree                  = 0x0000000000000032U, // Pag Free
        //  An invalid operation was attempted on a used page.
        PageInUse                 = 0x0000000000000033U, // Pag Used
        //  An invalid operation was attempted on a caching page.
        PageCaching               = 0x0000000000000034U, // Pag Cach.
        //  A page cannot be pushed to the stack because it is already on the stack.
        PageStacked               = 0x0000000000000035U, // Pag Stkd
        //  A page cannot be popped off the stack because it is not on the stack.
        PageNotStacked            = 0x0000000000000036U, // Pag N Stkd

        //  A/The target page is in an illegal range.
        PageMapIllegalRange       = 0x0000000000000040U, // Pag rng ill
        //  A/The target page is (already) mapped.
        PageMapped                = 0x0000000000000041U, // Pag mapped
        //  A/The target page is (already) unmapped.
        PageUnmapped              = 0x0000000000000042U, // Pag unmp.
        //  A given page is unaligned.
        PageUnaligned             = 0x0000000000000042U, // Pag unal.
    };

    struct Handle
    {
        /*  Statics  */

        static const uint64_t NullValue      = 0x0000000000000000ULL;
        //static const Handle Null             = {NullValue};

        static const uint64_t TypeBits       = 0xFFFF000000000000ULL;
        //static const uint32_t TypeOffset     = 48;

        static const uint64_t IndexBits      = 0x00007FFFFFFFFFFFULL;

        static const uint64_t GlobalFatalBit = 0x0000800000000000ULL;
        static const uint32_t GlobalOffset   = 47;

        /*  Constructor(s)  */

        Handle() = default;
        Handle(Handle const&) = default;

        //  Arbitrary-type handle.
        __bland __forceinline Handle(const HandleType type
                                   , const uint64_t index)
            : Value((uint64_t)type
                  | (index & IndexBits))
        {
            
        }

        //  Result handle.
        __bland __forceinline Handle(const HandleResult res)
            : Value((uint64_t)HandleType::Result
                  | ((uint64_t)res & IndexBits))
        {
            
        }

        /*  Type  */

        __bland __forceinline HandleType GetType() const
        {
            return (HandleType)(this->Value & TypeBits);
        }

        __bland __forceinline bool IsType(const HandleType type) const
        {
            return (HandleType)(this->Value & TypeBits) == type;
        }

        __bland __forceinline bool IsGlobal() const
        {
            HandleType type = this->GetType();

            return 0 != (this->Value & GlobalFatalBit)
                && type != HandleType::Result
                && type != HandleType::Invalid;
        }

        __bland __forceinline bool IsValid() const
        {
            return !this->IsType(HandleType::Invalid);
        }

        /*  Generic  */

        __bland __forceinline uint64_t GetIndex() const
        {
            return (this->IsType(HandleType::Result) || this->IsType(HandleType::Invalid))
                ? ~0ULL
                : (this->Value & IndexBits);
        }

        /*  Result  */

        __bland __forceinline HandleResult GetResult() const
        {
            return (HandleResult)(this->Value & IndexBits);
        }

        __bland __forceinline bool IsResult(const HandleResult res) const
        {
            return this->Value == ((uint64_t)HandleType::Result | (uint64_t)res);
        }

        __bland __forceinline bool IsFatalResult() const
        {
            return 0 != (this->Value & GlobalFatalBit)
                && this->IsType(HandleType::Result);
        }

        __bland __forceinline bool IsOkayResult() const
        {
            //return this->IsType(HandleType::Result)
            //    && (HandleResult)(this->Value & IndexBits) == HandleResult::Okay;

            return this->Value == ((uint64_t)HandleType::Result | (uint64_t)HandleResult::Okay);
        }

        /*  Field(s)  */

    private:

        uint64_t Value;

    public:

        /*  Printing  */

        __bland __forceinline const char * const GetTypeString() const
        {
            switch (this->GetType())
            {
                case HandleType::Invalid:
                    return "INVL";

                case HandleType::KernelObject:
                    return "KOBJ";
                case HandleType::ServiceObject:
                    return "SOBJ";
                case HandleType::ApplicationObject:
                    return "AOBJ";

                case HandleType::Thread:
                    return "THRD";
                case HandleType::Process:
                    return "PROC";
                case HandleType::Job:
                    return "JOB ";

                case HandleType::VirtualAddressSpace:
                    return "VASP";

                case HandleType::HandleTable:
                    return "HTBL";
                case HandleType::MultiHandle:
                    return "MHND";

                case HandleType::Result:
                    return "RES ";

                default:
                    return "UNKN";
            }
        }

        __bland __forceinline const char * const GetResultString() const
        {
            if (!this->IsType(HandleType::Result))
                return nullptr;

            switch (this->GetResult())
            {
                case HandleResult::Okay:
                    return "Okay";
                case HandleResult::OutOfMemory:
                    return "No mem.";
                case HandleResult::UnsupportedOperation:
                    return "Uns. Op.";
                case HandleResult::NotImplemented:
                    return "Not Impl.";

                case HandleResult::ArgumentOutOfRange:
                    return "Arg. OOR";
                case HandleResult::ArgumentNull:
                    return "Arg. Null";

                case HandleResult::FormatBadSpecifier:
                    return "Frm. BSpc";
                case HandleResult::FormatBadArgumentSize:
                    return "Frm. BAS ";

                case HandleResult::PagesOutOfAllocatorRange:
                    return "Pag OOAR";
                case HandleResult::PageFree:
                    return "Pag Free";
                case HandleResult::PageCaching:
                    return "Pag Cach.";
                case HandleResult::PageInUse:
                    return "Pag Used";
                case HandleResult::PageReserved:
                    return "Pag Res.";
                case HandleResult::PageStacked:
                    return "Pag Stkd";
                case HandleResult::PageNotStacked:
                    return "Pag N Stkd";

                case HandleResult::PageMapIllegalRange:
                    return "Pag rng ill";
                case HandleResult::PageMapped:
                    return "Pag mapped";
                case HandleResult::PageUnmapped:
                    return "Pag unmp.";

                default:
                    return "UNKNOWN";
            }
        }

    } __attribute__((packed));
    //  So GCC thinks that Handle isn't POD enough unless I pack it. GG.
}

//  TODO: C version.
