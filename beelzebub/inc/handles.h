/** Handle Structure:
 *      Bit     63            -  Global Flag (or Fatal if Type = Result)
 *      Bits  8:62 (55 bits)  -  Content / Index
 *      Bits  0: 7 ( 8 bits)  -  Handle Type
 *
 *  Result Handle Structure:
 *      Bit     63            -  Fatal Flag (ignored if primary result = Okay)
 *      Bits 60:62 ( 3 bits)  -  Results Count (value biased by +1)
 *      Bits  8:59 (52 bits)  -  Content:
 *          Bits  8:15 ( 8 bits)  -  Primary Result Value
 *          Bits 16:23 ( 8 bits)  -  Secondary Result Value
 *          Bits 24:31 ( 8 bits)  -  Tertiary Result Value
 *          Bits 32:39 ( 8 bits)  -  Quaternary Result Value
 *          Bits 40:47 ( 8 bits)  -  Quinary Result Value
 *          Bits 48:55 ( 8 bits)  -  Senary Result Value
 *      Bits  0: 7 ( 8 bits)  -  Handle Type (Result = 0x01)
 *  * The result count will not go above 6. A value of 6 means that results were truncated.
 *    A value of 0 means 1 result (the primary), and 5 means all results (primary to senary
 *    included).
 *  
 *  The result is placed in the least significant bits to favor little endian architectures
 *  which can encode integers as various sizes. with this format, a result only needs to be
 *  encoded as a 16-bit constant if it does not contain any extras.
 *
 *  The fields are squeezed into byte boundaries because they are significantly easier to
 *  extract. For platforms that are anal about alignment, the compiler will do all the
 *  masking and shifting. For platforms with lesser alignment requirements (i.e. x86), the
 *  code will be more efficient because the fields can be accessed directly with an address.
 *  (or low/high byte registers; or still do mask-and-shift/shift-and-mask when it has to)
 *  
 *  Moreover, checking for an Okay Result handle now means testing for equality of the lower
 *  word to 0x0001, which is much more efficient than equating to 0xFF00000000000000,
 *  like it had to before... On 32-bit architectures, it even had to compare with two numbers!
 */

#pragma once

#include <metaprogramming.h>

#ifdef __cplusplus

namespace Beelzebub
{
    enum class HandleType : uint8_t
    {
        //  DESCRIPTION                  | // ABBREVIATION

        //  This be an invalid handle.
        Invalid                   = 0x00U, // INVL

        //  A general result code.
        Result                    = 0x01U, // RES

        //  A unit of execution.
        Thread                    = 0x10U, // THRD
        //  A unit of isolation.
        Process                   = 0x11U, // PRCS
        //  A unit of management.
        Job                       = 0x12U, // JOB

        //  A miscellaneous/anonymous object belonging to the kernel.
        KernelObject              = 0x21U, // KOBJ
        //  A miscellaneous/anonymous object belonging to a service.
        ServiceObject             = 0x22U, // SOBJ
        //  A miscellaneous/anonymous object belonging to an application.
        ApplicationObject         = 0x23U, // AOBJ

        //  An table which associates handles with resources.
        HandleTable               = 0xF0U, // HTBL
        //  A finite set of handles.
        MultiHandle               = 0xF1U, // MHND
    };

    enum class HandleResult : uint8_t
    {
        //  DESCRIPTION                  | // SHORT NAME

        //  Saul Goodman!
        Okay                      = 0x00U, // Okay
        //  Not enough memory available to complete an operation.
        OutOfMemory               = 0x01U, // No mem.
        //  The requested operation isn't supported by the object/interface.
        UnsupportedOperation      = 0x09U, // Unsp. Op.
        //  Operation not implemented by the object/interface.
        NotImplemented            = 0x0AU, // Not Impl.

        //  An argument given to a function/method has been given an invalid template argument.
        ArgumentTemplateInvalid   = 0x10U, // Arg. T inv.
        //  An argument given to a function/method is outside of the expected/supported range.
        ArgumentOutOfRange        = 0x11U, // Arg. OOR
        //  An argument given to a function/method shouldn't be null.
        ArgumentNull              = 0x12U, // Arg. Null

        //  An unknown format specifier was encountered in the format string.
        FormatBadSpecifier        = 0x20U, // Frm. BSpc.
        //  The size given for a format argument is invalid.
        FormatBadArgumentSize     = 0x21U, // Frm. BAS

        //  An operation was attempted on a range of pages that
        //  aren't covered by the page allocator.
        PagesOutOfAllocatorRange  = 0x30U, // Pag OOAR
        //  An invalid operation was attempted on a reserved page.
        PageReserved              = 0x31U, // Pag Res.
        //  An invalid operation was attempted on a free page.
        PageFree                  = 0x32U, // Pag Free
        //  An invalid operation was attempted on a used page.
        PageInUse                 = 0x33U, // Pag Used
        //  An invalid operation was attempted on a caching page.
        PageCaching               = 0x34U, // Pag Cach.
        //  A page cannot be pushed to the stack because it is already on the stack.
        PageStacked               = 0x35U, // Pag Stkd
        //  A page cannot be popped off the stack because it is not on the stack.
        PageNotStacked            = 0x36U, // Pag N Stkd

        //  A/The target page is in an illegal range.
        PageMapIllegalRange       = 0x40U, // Pag rng ill
        //  A/The target page is (already) mapped.
        PageMapped                = 0x41U, // Pag mapped
        //  A/The target page is (already) unmapped.
        PageUnmapped              = 0x42U, // Pag unmp.
        //  A given page is unaligned.
        PageUnaligned             = 0x43U, // Pag unal.

        //  A thread is already linked.
        ThreadAlreadyLinked       = 0x50U, // Thr a. lnk.

        //  A command-line option was not specified.
        CmdOptionUnspecified      = 0x60U, // Cmdo n spc.
        //  A command-line option's value seems to be of the wrong type.
        CmdOptionValueTypeInvalid = 0x61U, // Cmdo vT inv
        //  A command-line option's value is not found in the given table.
        CmdOptionValueNotInTable  = 0x62U, // Cmdo v nit.

        //  An object could not be allocated due to pool exhaustion.
        ObjaPoolsExhausted        = 0x70U, // Obja P exh.
    };

    struct Handle
    {
        /*  Statics  */

        static const uint64_t NullValue                 = 0x0000000000000000ULL;
        static const uint16_t OkayResultWord            = (uint16_t)(((uint16_t)HandleResult::Okay << 8) | (uint16_t)HandleType::Result);

        static const uint64_t TypeBits                  = 0x00000000000000FFULL;
        //static const uint32_t TypeOffset                = 56;

        static const uint64_t IndexBits                 = 0x7FFFFFFFFFFFFF00ULL;
        static const size_t   IndexOffset               = 8;

        static const uint64_t GlobalFatalBit            = 0x8000000000000000ULL;
        static const size_t   GlobalOffset              = 63;
        static const size_t   GlobalFatalByteIndex      = 7;
        static const size_t   GlobalFatalByteBit        = 0x80;

        static const uint64_t ResultCountBits           = 0x7000000000000000ULL;
        static const size_t   ResultCountOffset         = 60;
        static const uint64_t ResultCountUnit           = 0x1000000000000000ULL;
        static const size_t   ResultCountByteIndex      = 7;
        static const size_t   ResultCountByteBits       = 0x70;
        static const size_t   ResultCountByteOffset     = 4;
        static const uint64_t ResultCountByteUnit       = 0x10;

        static const uint64_t ResultsBits               = 0x00FFFFFFFFFFFF00ULL;
        static const uint64_t ResultsPreshiftBits       = 0x0000FFFFFFFFFF00ULL;
        static const uint64_t ResultsShiftOffset        = 8;

        static const size_t   ResultPrimaryByteIndex    = 1;
        static const size_t   ResultPrimaryOffset       = 8;
        static const size_t   ResultSecondaryByteIndex  = 2;
        static const size_t   ResultSecondaryOffset     = 16;
        static const size_t   ResultTertiaryByteIndex   = 3;
        static const size_t   ResultTertiaryOffset      = 24;
        static const size_t   ResultQuaternaryByteIndex = 4;
        static const size_t   ResultQuaternaryOffset    = 32;
        static const size_t   ResultQuinaryByteIndex    = 5;
        static const size_t   ResultQuinaryOffset       = 40;
        static const size_t   ResultSenaryByteIndex     = 6;
        static const size_t   ResultSenaryOffset        = 48;

        /*  Constructor(s)  */

        Handle() = default;
        Handle(Handle const&) = default;

        //  Arbitrary-type handle.
        __bland __forceinline constexpr Handle(const HandleType type
                                             , const uint64_t index
                                             , const bool global = false)
            : Value((uint64_t)type
                  | ((index & IndexBits) << IndexOffset)
                  | (global ? GlobalFatalBit : 0))
        {
            
        }

        //  Result handle.
        __bland __forceinline constexpr Handle(const HandleResult res)
            : Value((uint64_t)( (uint16_t)HandleType::Result
                             | ((uint16_t)res << ResultPrimaryOffset)))
        {
            
        }

        //  Result handle, optionally fatal.
        __bland __forceinline constexpr Handle(const HandleResult res
                                             , const bool fatal)
            : Value((uint64_t)( (uint16_t)HandleType::Result
                             | ((uint16_t)res << ResultPrimaryOffset))
                  | (fatal ? GlobalFatalBit : 0))
        {
            //  Why not a default parameter? Because this overload needs to work
            //  with 64-bit numbers, while the other needs only 16-bit.
        }

        /*  Type  */

        __bland __forceinline HandleType GetType() const
        {
            return (HandleType)(this->Bytes[0]);
        }

        __bland __forceinline bool IsType(const HandleType type) const
        {
            return this->Bytes[0] == (uint8_t)type;
        }

        __bland __forceinline bool IsGlobal() const
        {
            HandleType type = this->GetType();

            return 0 != (this->Bytes[GlobalFatalByteIndex] & GlobalFatalByteBit)
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
            return (HandleResult)(this->Bytes[ResultPrimaryByteIndex]);
        }

        __bland __forceinline size_t GetResultCount() const
        {
            return (size_t)((this->Bytes[ResultCountByteIndex] & ResultCountByteBits) >> ResultCountByteOffset);
        }

        __bland Handle WithResultCount(const size_t count) const;
        __bland Handle WithPreppendedResult(const HandleResult res) const;

        __bland __forceinline bool IsResult(const HandleResult res) const
        {
            return this->Bytes[ResultPrimaryByteIndex] == (uint8_t)res && this->GetType() == HandleType::Result;
        }

        __bland __forceinline bool IsFatalResult() const
        {
            return 0 != (this->Bytes[GlobalFatalByteIndex] & GlobalFatalByteBit)
                && this->GetType() == HandleType::Result;
        }

        __bland __forceinline bool IsGlobalOrFatal() const
        {
            return 0 != (this->Bytes[GlobalFatalByteIndex] & GlobalFatalByteBit);
        }

        __bland __forceinline bool IsOkayResult() const
        {
            return this->Words[0] == OkayResultWord;
            //  Other bits are irrelevant.
        }

        /*  Field(s)  */

    private:

        union
        {
            uint64_t Value;
            uint32_t Dwords[2];
            uint16_t Words[4];
            uint8_t  Bytes[8];
        };

        //  Use is discouraged, but meh.
        __bland __forceinline constexpr Handle(uint64_t val)
            : Value( val)
        {

        }

    public:

        /*  Printing  */

        __bland __noinline const char * const GetTypeString() const;
        __bland __noinline const char * const GetResultString() const;

    } __packed;
    //  So GCC thinks that Handle isn't POD enough unless I pack it. GG.

    const Handle Null = Handle();
}

#else
    
    typedef struct Handle_t { uint64_t Value; } Handle;
    //  Eh... Good enough.

#endif
