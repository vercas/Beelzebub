/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


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

#define ENUM_HANDLETYPE(ENUMINST) \
    /*  This be an invalid handle. */ \
    ENUMINST(Invalid                  , 0x00U, "INVL") \
    /*  A general result code. */ \
    ENUMINST(Result                   , 0x01U, "RES") \
    /*  A page of memory. */ \
    ENUMINST(Page                     , 0x02U, "PAGE") \
    \
    /*  A unit of execution. */ \
    ENUMINST(Thread                   , 0x10U, "THRD") \
    /*  A unit of isolation. */ \
    ENUMINST(Process                  , 0x11U, "PRCS") \
    /*  A unit of management. */ \
    ENUMINST(Job                      , 0x12U, "JOB") \
    \
    /*  A miscellaneous/anonymous object belonging to the kernel. */ \
    ENUMINST(KernelObject             , 0x20U, "KOBJ") \
    /*  A miscellaneous/anonymous object belonging to a service. */ \
    ENUMINST(ServiceObject            , 0x21U, "SOBJ") \
    /*  A miscellaneous/anonymous object belonging to an application. */ \
    ENUMINST(ApplicationObject        , 0x22U, "AOBJ") \
    \
    /*  A file in the InitRD. */ \
    ENUMINST(InitRdFile               , 0x30U, "irdF") \
    /*  A directory in the InitRD. */ \
    ENUMINST(InitRdDirectory          , 0x31U, "trdD") \
    /*  A kernel module. */ \
    ENUMINST(KernelModule             , 0x32U, "KMOD") \
    \
    /*  An table which associates handles with resources. */ \
    ENUMINST(HandleTable              , 0xF0U, "HTBL") \
    /*  A finite set of handles. */ \
    ENUMINST(MultiHandle              , 0xF1U, "MHND") \

#define ENUM_HANDLERESULT(ENUMINST) \
    /*  Saul Goodman! */ \
    ENUMINST(Okay                     , 0x00U, "Okay") \
    /*  Not enough memory available to complete an operation. */ \
    ENUMINST(OutOfMemory              , 0x01U, "No mem.") \
    /*  An object that was looked for was not found. */ \
    ENUMINST(NotFound                 , 0x02U, "Not Found") \
    /*  An object or data set failed an integrity check */ \
    ENUMINST(IntegrityFailure         , 0x03U, "Intg Fail") \
    /*  Too many or not enough instances of an object/entity are ecnountered. */ \
    ENUMINST(CardinalityViolation     , 0x04U, "Card Viol") \
    /*  An operation is taking too long to complete and has timed out. */ \
    ENUMINST(Timeout                  , 0x05U, "Timeout") \
    /*  An object is misaligned. */ \
    ENUMINST(AlignmentFailure         , 0x08U, "Unaligned") \
    /*  The requested operation isn't supported by the object/interface. */ \
    ENUMINST(UnsupportedOperation     , 0x09U, "Unsp. Op.") \
    /*  Operation not implemented by the object/interface. */ \
    ENUMINST(NotImplemented           , 0x0AU, "Not Impl.") \
    /*  An operation was attempted on an object that has been disposed. */ \
    ENUMINST(ObjectDisposed           , 0x0BU, "Obj Disp.") \
    /*  An operation failed. */ \
    ENUMINST(Failed                   , 0x0FU, "Failed") \
    \
    /*  An argument given to a function/method has been given an invalid template argument. */ \
    ENUMINST(ArgumentTemplateInvalid  , 0x10U, "Arg. T inv.") \
    /*  An argument given to a function/method is outside of the expected/supported range. */ \
    ENUMINST(ArgumentOutOfRange       , 0x11U, "Arg. OOR") \
    /*  An argument given to a function/method shouldn't be null. */ \
    ENUMINST(ArgumentNull             , 0x12U, "Arg. Null") \
    /*  A given handle is invalid (or of the wrong type). */ \
    ENUMINST(HandleInvalid            , 0x1EU, "Handle inv.") \
    /*  A selected syscall is invalid. */ \
    ENUMINST(SyscallSelectionInvalid  , 0x1FU, "Sysc S inv.") \
    \
    /*  An unknown format specifier was encountered in the format string. */ \
    ENUMINST(FormatBadSpecifier       , 0x20U, "Frm. BSpc.") \
    /*  The size given for a format argument is invalid. */ \
    ENUMINST(FormatBadArgumentSize    , 0x21U, "Frm. BAS") \
    \
    /*  An operation was attempted on a range of pages that */ \
    /*  aren't covered by the page allocator. */ \
    ENUMINST(PagesOutOfAllocatorRange , 0x30U, "Pag OOAR") \
    /*  An invalid operation was attempted on a reserved page. */ \
    ENUMINST(PageReserved             , 0x31U, "Pag Res.") \
    /*  An invalid operation was attempted on a free page. */ \
    ENUMINST(PageFree                 , 0x32U, "Pag Free") \
    /*  An invalid operation was attempted on a used page. */ \
    ENUMINST(PageInUse                , 0x33U, "Pag Used") \
    /*  An invalid operation was attempted on a caching page. */ \
    ENUMINST(PageCaching              , 0x34U, "Pag Cach.") \
    /*  A page cannot be pushed to the stack because it is already on the stack. */ \
    ENUMINST(PageStacked              , 0x35U, "Pag Stkd") \
    /*  A page cannot be popped off the stack because it is not on the stack. */ \
    ENUMINST(PageNotStacked           , 0x36U, "Pag N Stkd") \
    \
    /*  A/The target page is in an illegal range. */ \
    ENUMINST(PageMapIllegalRange      , 0x40U, "Pag rng ill") \
    /*  A/The target page is (already) mapped. */ \
    ENUMINST(PageMapped               , 0x41U, "Pag mapped") \
    /*  A/The target page is (already) unmapped. */ \
    ENUMINST(PageUnmapped             , 0x42U, "Pag unmp.") \
    /*  The page cannot be allocated on demand. */ \
    ENUMINST(PageUndemandable         , 0x43U, "Pag undem") \
    /*  The page hit is a guard page. */ \
    ENUMINST(PageGuard                , 0x44U, "Pag guard") \
    \
    /*  A thread is already linked. */ \
    ENUMINST(ThreadAlreadyLinked      , 0x50U, "Thr a. lnk.") \
    \
    /*  A command-line option was not specified. */ \
    ENUMINST(CmdOptionsMalformatted   , 0x60U, "Cmdo malfrm") \
    /*  A command-line option's value seems to be of the wrong type. */ \
    ENUMINST(CmdOptionValueTypeInvalid, 0x61U, "Cmdo vT inv") \
    /*  A command-line option's value is not found in the given table. */ \
    ENUMINST(CmdOptionValueNotInTable , 0x62U, "Cmdo v nit.") \
    \
    /*  An object could not be allocated due to pool exhaustion. */ \
    ENUMINST(ObjaPoolsExhausted       , 0x70U, "Obja P exh.") \
    /*  An attempt was made to free an object which is already free. */ \
    ENUMINST(ObjaAlreadyFree          , 0x71U, "Obja A Free") \
    /*  An object allocator has reached maximum capacity. */ \
    ENUMINST(ObjaMaximumCapacity      , 0x72U, "Obja M cap.") \
    \
    /*  An executable image failed to load. */ \
    ENUMINST(ImageLoadingFailure      , 0x80U, "Img ld fail") \
    /*  An executable image failed to relocate. */ \
    ENUMINST(ImageRelocationFailure   , 0x81U, "Img rl fail") \
    /*  The runtime library seems to not match the kernel. */ \
    ENUMINST(RuntimeMismatch          , 0x82U, "RT mismatch") \
    \
    /*  A process/thread/task (unit of work) requested immediate termination. */ \
    ENUMINST(ImmediateTermination     , 0xFDU, "Imm. termin") \
    /*  An assertion was failed. */ \
    ENUMINST(AssertionFailure         , 0xFEU, "Assret fail") \
    /*  A process exited with a non-zero return code. */ \
    ENUMINST(NonZeroReturnCode        , 0xFFU, "Non-0 ret c") \

#ifdef __cplusplus

namespace Beelzebub
{
    enum class HandleType : uint8_t
    {
        ENUM_HANDLETYPE(ENUMINST_VAL)
    };

    enum class HandleResult : uint8_t
    {
        ENUM_HANDLERESULT(ENUMINST_VAL)
    };

    struct Handle
    {
        /*  Statics  */

        static constexpr uint64_t const NullValue                 = 0x0000000000000000ULL;
        static constexpr uint16_t const OkayResultWord            = (uint16_t)(((uint16_t)HandleResult::Okay << 8) | (uint16_t)HandleType::Result);

        static constexpr uint64_t const TypeBits                  = 0x00000000000000FFULL;
        //static constexpr uint32_t const TypeOffset                = 56;

        static constexpr uint64_t const IndexBits                 = 0x7FFFFFFFFFFFFF00ULL;
        static constexpr   size_t const IndexOffset               = 8;

        static constexpr uint64_t const GlobalFatalBit            = 0x8000000000000000ULL;
        static constexpr   size_t const GlobalOffset              = 63;
        static constexpr   size_t const GlobalFatalByteIndex      = 7;
        static constexpr   size_t const GlobalFatalByteBit        = 0x80;

        static constexpr uint64_t const ResultCountBits           = 0x7000000000000000ULL;
        static constexpr   size_t const ResultCountOffset         = 60;
        static constexpr uint64_t const ResultCountUnit           = 0x1000000000000000ULL;
        static constexpr   size_t const ResultCountByteIndex      = 7;
        static constexpr   size_t const ResultCountByteBits       = 0x70;
        static constexpr   size_t const ResultCountByteOffset     = 4;
        static constexpr uint64_t const ResultCountByteUnit       = 0x10;

        static constexpr uint64_t const ResultsBits               = 0x00FFFFFFFFFFFF00ULL;
        static constexpr uint64_t const ResultsPreshiftBits       = 0x0000FFFFFFFFFF00ULL;
        static constexpr uint64_t const ResultsShiftOffset        = 8;

        static constexpr   size_t const ResultPrimaryByteIndex    = 1;
        static constexpr   size_t const ResultPrimaryOffset       = 8;
        static constexpr   size_t const ResultSecondaryByteIndex  = 2;
        static constexpr   size_t const ResultSecondaryOffset     = 16;
        static constexpr   size_t const ResultTertiaryByteIndex   = 3;
        static constexpr   size_t const ResultTertiaryOffset      = 24;
        static constexpr   size_t const ResultQuaternaryByteIndex = 4;
        static constexpr   size_t const ResultQuaternaryOffset    = 32;
        static constexpr   size_t const ResultQuinaryByteIndex    = 5;
        static constexpr   size_t const ResultQuinaryOffset       = 40;
        static constexpr   size_t const ResultSenaryByteIndex     = 6;
        static constexpr   size_t const ResultSenaryOffset        = 48;

        /*  Constructor(s)  */

        Handle() = default;
        Handle(Handle const&) = default;

        //  Arbitrary-type handle.
        inline constexpr Handle(HandleType const type
                                , uint64_t const index
                                ,     bool const global = false)
            : Value((uint64_t)type
                  | ((index << IndexOffset) & IndexBits)
                  | (global ? GlobalFatalBit : 0))
        {
            
        }

        //  Result handle.
        inline constexpr Handle(HandleResult const res)
            : Value((uint64_t)( (uint16_t)HandleType::Result
                             | ((uint16_t)res << ResultPrimaryOffset)))
        {
            
        }

        //  Result handle, optionally fatal.
        inline constexpr Handle(HandleResult const res, bool const fatal)
            : Value((uint64_t)( (uint16_t)HandleType::Result
                             | ((uint16_t)res << ResultPrimaryOffset))
                  | (fatal ? GlobalFatalBit : 0))
        {
            //  Why not a default parameter? Because this overload needs to work
            //  with 64-bit numbers, while the other needs only 16-bit.
        }

    private:
        //  Use is discouraged, but meh.
        inline constexpr Handle(uint64_t val) : Value( val) { }

    public:
        /*  Type  */

        inline HandleType GetType() const
        {
            return (HandleType)(this->Bytes[0]);
        }

        inline bool IsType(HandleType const type) const
        {
            return this->Bytes[0] == (uint8_t)type;
        }

        inline bool IsGlobal() const
        {
            HandleType type = this->GetType();

            return 0 != (this->Bytes[GlobalFatalByteIndex] & GlobalFatalByteBit)
                && type != HandleType::Result
                && type != HandleType::Invalid;
        }

        inline bool IsValid() const
        {
            return !this->IsType(HandleType::Invalid);
        }

        inline bool IsLiterally(uint64_t const val) const
        {
            return this->Value == val;
        }

        /*  Generic  */

        inline uint64_t GetIndex() const
        {
            return (this->IsType(HandleType::Result) || this->IsType(HandleType::Invalid))
                ? ~0ULL
                : ((this->Value & IndexBits) >> IndexOffset);
        }

        inline uint64_t GetIndex(HandleType const type) const
        {
            return this->IsType(type)
                ? ((this->Value & IndexBits) >> IndexOffset)
                : ~0ULL;
        }

        /*  Page  */

        inline void * GetPage() const
        {
            if (this->IsType(HandleType::Page))
            {
                uint64_t ptr = (this->Value & IndexBits) >> IndexOffset;

                EXTEND_POINTER(ptr);

                return reinterpret_cast<void *>(ptr);
            }
            else
                return nullptr;
        }

        /*  Result  */

        inline HandleResult GetResult() const
        {
            return (HandleResult)(this->Bytes[ResultPrimaryByteIndex]);
        }

        inline size_t GetResultCount() const
        {
            return (size_t)((this->Bytes[ResultCountByteIndex] & ResultCountByteBits) >> ResultCountByteOffset);
        }

        inline Handle WithResultCount(size_t const count) const
        {
            uint64_t const countBits = ((uint64_t)count << ResultCountOffset) & ResultCountBits;

            return {(this->Value & ~ResultCountBits) | countBits};
        }

        inline Handle WithPreppendedResult(HandleResult const res) const
        {
            if (!this->IsType(HandleType::Result))
                return Handle();

            size_t const resultCount = this->GetResultCount();

            uint64_t nonResultBits;

            if (resultCount < 6)
                nonResultBits = (((uint64_t)resultCount + 1) << ResultCountOffset) | (this->Value & (TypeBits | GlobalFatalBit));
                //  The result count shouldn't make the result bits overflow.
            else
                nonResultBits = this->Value & (TypeBits | GlobalFatalBit | ResultCountBits);
                //  Preserve all.

            return {nonResultBits | ((this->Value & ResultsPreshiftBits) << ResultsShiftOffset) | (((uint64_t)res) << ResultPrimaryOffset)};

            //  Whoever misused this method by passing an invalid result value (i.e. > 255) in the first parameter's register/address will only
            //  affect themselves. Don't be a smartbutt.
        }

        inline bool IsResult(HandleResult const res) const
        {
            return this->Bytes[ResultPrimaryByteIndex] == (uint8_t)res && this->GetType() == HandleType::Result;
        }

        inline bool IsFatalResult() const
        {
            return 0 != (this->Bytes[GlobalFatalByteIndex] & GlobalFatalByteBit)
                && this->GetType() == HandleType::Result;
        }

        inline bool IsGlobalOrFatal() const
        {
            return 0 != (this->Bytes[GlobalFatalByteIndex] & GlobalFatalByteBit);
        }

        inline bool IsOkayResult() const
        {
            return this->Words[0] == OkayResultWord;
            //  Other bits are irrelevant.
        }

        /*  Operator(s)  */

        inline bool operator ==(Handle const other) { return this->Value == other.Value; }
        inline bool operator !=(Handle const other) { return this->Value != other.Value; }

        inline bool operator ==(HandleResult const res) { return this->IsResult(res); }
        inline bool operator !=(HandleResult const res) { return !this->IsResult(res); }

        /*  Field(s)  */

    private:

        union
        {
            uint64_t Value;
            uint32_t Dwords[2];
            uint16_t Words[4];
            uint8_t  Bytes[8];
        };

    public:

        /*  Printing  */

        __noinline char const * const GetTypeString() const;
        __noinline char const * const GetResultString() const;

    } __packed;
    //  So GCC thinks that Handle isn't POD enough unless I pack it. GG.

    template<typename T, HandleType HTYPE, size_t OFFSET = 0>
    union HandlePointer
    {
        /*  Masks and Offsets  */

        static constexpr uint64_t const PointerMask = 0x0000FFFFFFFFFFFF;
        static constexpr uint64_t const DataMask = ((1ULL << (7UL + OFFSET)) - 1) & 0x007FFFFFFFFFFFFFU;
        static constexpr uint64_t const DataOffset = 48UL - OFFSET;

        /*  Constructors  */

        HandlePointer() = default;

        inline HandlePointer(Handle const h) : Value(h.GetIndex(HTYPE)) { }

        inline HandlePointer(T const * const ptr, uint64_t const data = 0)
            : Value((((uint64_t)reinterpret_cast<uintptr_t>(ptr) & PointerMask) >> OFFSET)
                | ((data & DataMask) << DataOffset))
        {
            //  Simple, eh?
        }

        inline Handle ToHandle(bool const glob = false)
        {
            return Handle(HTYPE, this->Value, glob);
        }

        /*  Properties  */

        inline void SetPointer(T const * const ptr)
        {
            uint64_t const ptr64 = ((uint64_t)reinterpret_cast<uintptr_t>(ptr) & PointerMask) >> OFFSET;
            
            this->Value = (this->Value & (DataMask << DataOffset)) | ptr64;
        }

        inline void SetData(uint64_t const data)
        {
            this->Value = (this->Value & (PointerMask >> OFFSET)) | ((data & DataMask) << DataOffset);
        }

        inline T * GetPointer() const
        {
            uint64_t ptr64 = (this->Value & (PointerMask >> OFFSET)) << OFFSET;

            EXTEND_POINTER(ptr64);

            return reinterpret_cast<T *>(ptr64);
        }

        inline HandlePointer const & GetPointer(T * & val) const
        {
            uint64_t ptr64 = (this->Value & (PointerMask >> OFFSET)) << OFFSET;

            EXTEND_POINTER(ptr64);

            val = reinterpret_cast<T *>(ptr64);

            return *this;
        }

        inline uint64_t GetData() const
        {
            return (this->Value >> DataOffset) & DataMask;
        }

        inline HandlePointer const & GetData(uint64_t & val) const
        {
            val = (this->Value >> DataOffset) & DataMask;

            return *this;
        }

        /*  Fields  */

        uint64_t Value;
    };
}

#else
    
    enum HANDLE_TYPE
    {
        ENUM_HANDLETYPE(ENUMINST_VAL)
    };

    enum HANDLE_RESULT
    {
        ENUM_HANDLERESULT(ENUMINST_VAL)
    };

    typedef union Handle
    {
        uint64_t Value;
        uint32_t Dwords[2];
        uint16_t Words[4];
        uint8_t  Bytes[8];
    } Handle;

    //  Eh... Good enough?

#endif

#ifdef __cplusplus
typedef Beelzebub::Handle handle_t;
#else
typedef union Handle handle_t;
#endif
