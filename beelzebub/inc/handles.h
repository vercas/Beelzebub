#pragma once

#include <metaprogramming.h>

namespace Beelzebub
{
    enum class HandleType : uint64_t
    {
        //  This be an invalid handle.
        Invalid              = 0x0000000000000000U,

        //  A miscellaneous/anonymous object belonging to the kernel.
        KernelObject         = 0x0001000000000000U,
        //  A miscellaneous/anonymous object belonging to a service.
        ServiceObject        = 0x0002000000000000U,
        //  A miscellaneous/anonymous object belonging to an application.
        ApplicationObject    = 0x0003000000000000U,

        //  A unit of execution.
        Thread               = 0x0010000000000000U,
        //  A unit of isolation.
        Process              = 0x0011000000000000U,
        //  A unit of management.
        Job                  = 0x0012000000000000U,

        //  A virtual address space. ("linear" by x86 terminology)
        VirtualAddressSpace  = 0x0020000000000000U,

        //  An table which associates handles with resources.
        HandleTable          = 0xFFF0000000000000U,
        //  A finite set of handles.
        MultiHandle          = 0xFFF1000000000000U,

        //  A general result code.
        Result               = 0xFFFF000000000000U,
    };

    enum class HandleResult : uint64_t
    {
        //  Saul Goodman!
        Okay                  = 0x0000000000000000U,
        //  The requested operation isn't supported by the object/interface.
        UnsupportedOperation  = 0x0000000000000001U,
        //  Operation not implemented by the object/interface.
        NotImplemented        = 0x0000000000000002U,

        //  An argument given to a function/method is outside of the expected/supported range.
        ArgumentOutOfRange    = 0x0000000000000010U,
        //  An argument given to a function/method shouldn't be null.
        ArgumentNull          = 0x0000000000000011U,

        //  An unknown format specifier was encountered in the format string.
        FormatBadSpecifier    = 0x0000000000000020U,
        //  The size given for a format argument is invalid.
        FormatBadArgumentSize = 0x0000000000000021U,

        //  An operation was attempted on a range of pages that aren't free.
        PagesNotFree          = 0x0000000000000030U,
        //  An operation was attempted on a range of pages that aren't free or caching.
        PagesNotFreeOrCaching = 0x0000000000000031U,
        //  An invalid operation was attempted on a reserved page.
        PageReserved          = 0x0000000000000032U,
        //  An invalid operation was attempted on a free page.
        PageFree              = 0x0000000000000032U,
    };

    struct Handle
    {
        /*  Statics  */

        static const uint64_t NullValue    = 0xFFFFFFFFFFFFFFFFULL;
        //static const Handle Null           = {NullValue};

        static const uint64_t TypeBits     = 0xFFFF000000000000ULL;
        //static const uint32_t TypeOffset   = 48;

        static const uint64_t IndexBits    = 0x00007FFFFFFFFFFFULL;

        static const uint64_t GlobalFatalBit    = 0x0000800000000000ULL;
        static const uint32_t GlobalOffset = 47;

        /*  Constructor(s)  */

        Handle() = default;
        Handle(Handle const&) = default;

        //  Arbitrary-type handle.
        __bland __forceinline Handle(const HandleType type
                                   , const uint64_t index)
            : Value((uint64_t)type
                  | (index & IndexBits))
        {
            //  Type is converted to 64-bit and offset.
            //  This conversion will leave no unwanted bits.
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

        /*  Result  */

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

        /*  Execution flow-specific functions  */

        __bland __forceinline uint64_t GetThreadId() const
        {
            return (this->IsType(HandleType::Thread))
                ? (this->Value & IndexBits)
                : ~0ULL;
        }

        __bland __forceinline uint64_t GetProcessId() const
        {
            return (this->IsType(HandleType::Process))
                ? (this->Value & IndexBits)
                : ~0ULL;
        }

        __bland __forceinline uint64_t GetJobId() const
        {
            return (this->IsType(HandleType::Job))
                ? (this->Value & IndexBits)
                : ~0ULL;
        }

        /*  Field(s)  */

    private:

        uint64_t Value;

    } __attribute__((packed));
    //  So GCC thinks that Handle isn't POD enough unless I pack it. GG.
}

//  TODO: C version.
