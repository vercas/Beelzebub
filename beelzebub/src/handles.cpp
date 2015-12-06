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

#include <handles.h>

using namespace Beelzebub;

/********************
    Handle struct
********************/

/*  Printing  */

ENUM_TO_STRING_EX1(HandleType, , Handle::GetTypeString() const, this->GetType(), ENUM_HANDLETYPE)

// char const * const Handle::GetTypeString() const
// {
//     switch (this->GetType())
//     {
//         case HandleType::Invalid:
//             return "INVL";

//         case HandleType::Result:
//             return "RES ";

//         case HandleType::KernelObject:
//             return "KOBJ";
//         case HandleType::ServiceObject:
//             return "SOBJ";
//         case HandleType::ApplicationObject:
//             return "AOBJ";

//         case HandleType::Thread:
//             return "THRD";
//         case HandleType::Process:
//             return "PROC";
//         case HandleType::Job:
//             return "JOB ";

//         case HandleType::HandleTable:
//             return "HTBL";
//         case HandleType::MultiHandle:
//             return "MHND";

//         default:
//             return "UNKN";
//     }
// }

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
        case HandleResult::ObjectDisposed:
            return "Obj Disp.";

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
        case HandleResult::ObjaAlreadyFree:
            return "Obja A free";
        case HandleResult::ObjaMaximumCapacity:
            return "Obja M cap.";

        default:
            return "!!UNKNOWN!!";
    }
}
