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

#include <execution/elf.hpp>
#include <math.h>

#define DT_CASE(type, var) \
    case Beelzebub::Execution::ElfDynamicEntryTag::type: \
        var = dtCursor->Value; \
        break;
#define DT_CASE_C(type, var, vType) \
    case Beelzebub::Execution::ElfDynamicEntryTag::type: \
        var = (vType)(dtCursor->Value); \
        break;
#define DT_CASE_P(type, var, vType) \
    case Beelzebub::Execution::ElfDynamicEntryTag::type: \
        var = reinterpret_cast<vType>(/*this->Start +*/ dtCursor->Value); \
        break;

//  Number of occurrences of a specific tag.
#define DT_CNT(type) tagCounters[(size_t)(Beelzebub::Execution::ElfDynamicEntryTag::type)]

//  Makes sure the given tag is found on a unique entry.
#define DT_UNIQUE1(type) do \
{ \
    if (DT_CNT(type) == 0) \
        return Beelzebub::Execution::ElfValidationResult::DtEntryMissing; \
    else if (DT_CNT(type) > 1) \
        return Beelzebub::Execution::ElfValidationResult::DtEntryMultiplicate; \
} while (false)

#define DT_UNIQUE2(t1, t2) do { DT_UNIQUE1(t1); DT_UNIQUE1(t2); } while (false)
#define DT_UNIQUE3(t1, t2, t3) do { DT_UNIQUE1(t1); DT_UNIQUE2(t2, t3); } while (false)
#define DT_UNIQUE4(t1, t2, t3, t4) do { DT_UNIQUE2(t1, t4); DT_UNIQUE2(t2, t3); } while (false)
#define DT_UNIQUE5(t1, t2, t3, t4, t5) do { DT_UNIQUE2(t1, t4); DT_UNIQUE3(t2, t3, t5); } while (false)
#define DT_UNIQUE6(t1, t2, t3, t4, t5, t6) do { DT_UNIQUE3(t1, t4, t6); DT_UNIQUE3(t2, t3, t5); } while (false)

#define DT_UNIQUE(...) GET_MACRO6(__VA_ARGS__, DT_UNIQUE6, DT_UNIQUE5, DT_UNIQUE4, DT_UNIQUE3, DT_UNIQUE2, DT_UNIQUE1)(__VA_ARGS__)

//  Makes sure that the presence of a single entry of the first type also implies
//  the existence of a unique entry of the second type. Otherwise, the following
//  block/expression is executed when multiplicates are found.
#define DT_IMPLY1(type1, type2) if (DT_CNT(type1) == 1) DT_UNIQUE(type2); else if (DT_CNT(type1) > 1)

#define DT_IMPLY2(t1, t2, t3) if (DT_CNT(t1) == 1) { DT_UNIQUE(t2); DT_UNIQUE(t3); } else if (DT_CNT(t1) > 1)
#define DT_IMPLY3(t1, t2, t3, t4) if (DT_CNT(t1) == 1) { DT_UNIQUE(t2); DT_UNIQUE(t3); DT_UNIQUE(t4); } else if (DT_CNT(t1) > 1)

#define DT_IMPLY(arg1, ...) GET_MACRO3(__VA_ARGS__, DT_IMPLY3, DT_IMPLY2, DT_IMPLY1)(arg1, __VA_ARGS__)

#define CHECK_LOADED_2(rStart, rSize) do { \
    switch (CheckRangeLoaded(this, rStart, rSize)) \
    { \
    case RangeLoadStatus::CompletelyAbsent: \
        return ElfValidationResult::DtEntryAddressOutOfBounds; \
    case RangeLoadStatus::PartiallyLoaded: \
        return ElfValidationResult::DtEntryAddressInPartialLoad; \
    default: break; \
    } \
} while (false)

#define CHECK_LOADED_3(rStart, rSize, opt) do { \
    switch (CheckRangeLoaded(this, rStart, rSize, RangeLoadOptions::opt)) \
    { \
    case RangeLoadStatus::CompletelyAbsent: \
        return ElfValidationResult::DtEntryAddressOutOfBounds; \
    case RangeLoadStatus::PartiallyLoaded: \
        return ElfValidationResult::DtEntryAddressInPartialLoad; \
    case RangeLoadStatus::OptionsNotMet: \
        return ElfValidationResult::DtEntryWrongSegmentFlags; \
    default: break; \
    } \
} while (false)

#define CHECK_LOADED(arg1, ...) GET_MACRO2(__VA_ARGS__, CHECK_LOADED_3, CHECK_LOADED_2)(arg1, __VA_ARGS__)
