#include <handles.h>

using namespace Beelzebub;

/********************
    Handle struct
********************/

/*  Results  */

Handle Handle::WithResultCount(const size_t count) const
{
    const uint64_t countBits = ((uint64_t)count << ResultCountOffset) & ResultCountBits;

    return {(this->Value & ~ResultCountBits) | countBits};
}

Handle Handle::WithPreppendedResult(const HandleResult res) const
{
    if (!this->IsType(HandleType::Result))
        return Null;

    const size_t resultCount = this->GetResultCount();

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

/*  Printing  */

const char * const Handle::GetTypeString() const
{
    switch (this->GetType())
    {
        case HandleType::Invalid:
            return "INVL";

        case HandleType::Result:
            return "RES ";

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

        case HandleType::HandleTable:
            return "HTBL";
        case HandleType::MultiHandle:
            return "MHND";

        default:
            return "UNKN";
    }
}

const char * const Handle::GetResultString() const
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
        case HandleResult::PageUnaligned:
            return "Pag unalig.";

        case HandleResult::ThreadAlreadyLinked:
            return "Thr a. lnk.";

        case HandleResult::CmdOptionUnspecified:
            return "Cmdo n spc.";
        case HandleResult::CmdOptionValueTypeInvalid:
            return "Cmdo vT inv";
        case HandleResult::CmdOptionValueNotInTable:
            return "Cmdo v nit.";

        case HandleResult::ObjaPoolsExhausted:
            return "Obja P exh.";

        default:
            return "UNKNOWN";
    }
}
