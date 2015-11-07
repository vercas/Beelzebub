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
            return "Out of mem.";
        case HandleResult::NotFound:
            return "Not Found";
        case HandleResult::IntegrityFailure:
            return "Integrity F";
        case HandleResult::CardinalityViolation:
            return "Cardin Viol";
        case HandleResult::Timeout:
            return "Timeout";
        case HandleResult::UnsupportedOperation:
            return "Unsupp. Op.";
        case HandleResult::NotImplemented:
            return "Not Implem.";

        case HandleResult::ArgumentTemplateInvalid:
            return "Arg. T inv.";
        case HandleResult::ArgumentOutOfRange:
            return "Arg. OOR";
        case HandleResult::ArgumentNull:
            return "Arg. Null";

        case HandleResult::FormatBadSpecifier:
            return "Frm. B Spec";
        case HandleResult::FormatBadArgumentSize:
            return "Frm. B Ar S";

        case HandleResult::PagesOutOfAllocatorRange:
            return "Page OOAR";
        case HandleResult::PageFree:
            return "Page Free";
        case HandleResult::PageCaching:
            return "Page Cached";
        case HandleResult::PageInUse:
            return "Page In Use";
        case HandleResult::PageReserved:
            return "Page Reserv";
        case HandleResult::PageStacked:
            return "Page Stackd";
        case HandleResult::PageNotStacked:
            return "Page N Stkd";

        case HandleResult::PageMapIllegalRange:
            return "Pag rng ill";
        case HandleResult::PageMapped:
            return "Page mapped";
        case HandleResult::PageUnmapped:
            return "Page unmapd";
        case HandleResult::PageUnaligned:
            return "Pag unaligd";

        case HandleResult::ThreadAlreadyLinked:
            return "Thr a. lnkd";

        case HandleResult::CmdOptionUnspecified:
            return "Cmdo unspec";
        case HandleResult::CmdOptionValueTypeInvalid:
            return "Cmdo vT inv";
        case HandleResult::CmdOptionValueNotInTable:
            return "Cmdo v NIT";

        case HandleResult::ObjaPoolsExhausted:
            return "Obja P exh.";

        default:
            return "!!UNKNOWN!!";
    }
}
