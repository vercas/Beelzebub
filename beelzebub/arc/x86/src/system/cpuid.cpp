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

/**
 *  CPU vendor names and vendor strings may be registered trademarks of
 *  or copyrighted by their respective owners. They do not belong to me,
 *  naturally.
 */

#include <system/cpuid.hpp>
#include <string.h>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Terminals;

#ifdef CPUID_FEATURE
#undef CPUID_FEATURE
#endif

/******************
    CpuId class
******************/

/*  Operations  */

void CpuId::Initialize()
{
    //uint32_t a, b, c, d;

    //  Find the max standard value and vendor string.
    Execute(0x00000000U
        , this->MaxStandardValue
        , this->VendorString.Integers[0]
        , this->VendorString.Integers[2]
        , this->VendorString.Integers[1]);

    //char otherVendorString[13];
    uint32_t dummy;

    //  Find the max extended value and vendor string.
    Execute(0x80000000U
        , this->MaxExtendedValue
        , dummy
        , dummy
        , dummy);

    this->VendorString.Characters[12] = 0;
    //  Just ensuring null-termination.

    //  Find the standard feature flags.
    Execute(0x00000001U, this->VersionInformation, this->FeatureFlagsStandardB
                       , this->FeatureIntegers[1], this->FeatureIntegers[0]);

    //  Find the extended feature flags.
    Execute(0x80000001U, this->ExtendedSignature, this->FeatureFlagsExtendedB
                       , this->FeatureFlagsExtendedC, this->FeatureIntegers[2]);

    if      (memeq(this->VendorString.Characters, "GenuineIntel", 12))
    {
        this->Vendor = CpuVendor::Intel;

        this->InitializeIntel();
    }
    else if (memeq(this->VendorString.Characters, "AuthenticAMD", 12))
    {
        this->Vendor = CpuVendor::Amd;

        this->InitializeAmd();
    }
    else
    {
        this->Vendor = CpuVendor::Unknown;
    }

    if (this->MaxExtendedValue >= 0x80000004U)
    {
        //  First part (16 characters) of the processor name string.
        Execute(0x80000002U
            , this->ProcessorName.Integers[ 0]
            , this->ProcessorName.Integers[ 1]
            , this->ProcessorName.Integers[ 2]
            , this->ProcessorName.Integers[ 3]);

        //  Second part (16 characters) of the processor name string.
        Execute(0x80000003U
            , this->ProcessorName.Integers[ 4]
            , this->ProcessorName.Integers[ 5]
            , this->ProcessorName.Integers[ 6]
            , this->ProcessorName.Integers[ 7]);

        //  Third part (16 characters) of the processor name string.
        Execute(0x80000004U
            , this->ProcessorName.Integers[ 8]
            , this->ProcessorName.Integers[ 9]
            , this->ProcessorName.Integers[10]
            , this->ProcessorName.Integers[11]);
    }

    this->ProcessorName.Characters[48] = 0;
    //  Just makin' sure.
}

void CpuId::InitializeIntel()
{
    //uint32_t a, b, c, d;

    
}

void CpuId::InitializeAmd()
{
    //uint32_t a, b, c, d;

    
}

/*  Info Extraction  */

bool CpuId::CheckFeature(const CpuFeature feature) const
{
    uint32_t val = (uint32_t)feature, varInd = 0, bit = 0;

    FEATUREBITEX(val, varInd, bit);
    //  Extracts the relevant information.

    if (varInd < 3)
        return 0 != (this->FeatureIntegers[varInd] & bit);
    else
        return false;
}

uint8_t CpuId::GetModel() const
{
    uint8_t baseModel  = this->GetBaseModelId()
          , baseFamily = this->GetBaseFamilyId()
          , extdModel  = this->GetExtendedModelId();

    switch (this->Vendor)
    {
    case CpuVendor::Intel:
        if (baseFamily == 0x6 || baseFamily == 0xF)
            return (uint8_t)((extdModel << 4) | baseModel);
        else
            return baseModel;

    case CpuVendor::Amd:
    default:
        if (baseFamily == 0xF)
            return (uint8_t)((extdModel << 4) | baseModel);
        else
            return baseModel;
    }
}

/*  Debug  */

TerminalWriteResult CpuId::PrintToTerminal(TerminalBase * const term) const
{
    TerminalWriteResult tret;
    uint32_t cnt;

    TERMTRY0(term->WriteLine("CPUID:"), tret);

    TERMTRY1(term->WriteFormat("  Vendor String: %s%n", this->VendorString.Characters), tret, cnt);

    TERMTRY1(term->Write("  Vendor: "), tret, cnt);

    switch (this->Vendor)
    {
        case CpuVendor::Intel:
            TERMTRY1(term->WriteLine("Intel Corporation"), tret, cnt);
            break;

        case CpuVendor::Amd:
            TERMTRY1(term->WriteLine("Advanced Micro Devices"), tret, cnt);
            break;

        default:
            TERMTRY1(term->WriteLine("UNKNOWN"), tret, cnt);
            break;
    }

    if (this->MaxExtendedValue >= 0x80000004U)
        TERMTRY1(term->WriteFormat("  Processor Name: %s%n", this->ProcessorName.Characters), tret, cnt);

    TERMTRY1(term->WriteLine("  ----"), tret, cnt);

    TERMTRY1(term->WriteFormat("  Max Standard Value: %X4%n", this->MaxStandardValue), tret, cnt);
    TERMTRY1(term->WriteFormat("  Max Extended Value: %X4%n", this->MaxExtendedValue), tret, cnt);

    TERMTRY1(term->WriteLine("  ---- Version info"), tret, cnt);

    TERMTRY1(term->WriteFormat("  Stepping ID: %X1; Proc. Type: %X1%n"
        , this->GetSteppingId(), this->GetProcessorType()), tret, cnt);
    TERMTRY1(term->WriteFormat("  Ex+Ba Model ID: %X1 %X1; Ex+Ba Fam. ID: %X1 %X1%n"
        , this->GetExtendedModelId(), this->GetBaseModelId()
        , this->GetExtendedFamilyId(), this->GetBaseFamilyId()), tret, cnt);

    TERMTRY1(term->WriteLine("  ---- Features"), tret, cnt);

    TERMTRY1(term->WriteFormat("  Brand Index: %X1%n", this->GetBrandIndex()), tret, cnt);
    TERMTRY1(term->WriteFormat("  Initial APIC ID: %X1%n", this->GetInitialApicId()), tret, cnt);
    TERMTRY1(term->WriteFormat("  Max Logical Processors: %X1%n", this->GetMaxLogicalProcessors()), tret, cnt);
    TERMTRY1(term->WriteFormat("  CLFLUSH Line Size: %Xs%n", this->GetClflushLineSize()), tret, cnt);

    TERMTRY1(term->WriteLine("  ---- Feature flags"), tret, cnt);

    TERMTRY1(term->Write(" "), tret, cnt);

#define PRINTFLAG2(name, prettyName) \
    if (this->CheckFeature(CpuFeature::name)) TERMTRY1(term->Write(" " #prettyName), tret, cnt);

#define CPUID_FEATURE(name, _, __, prettyName) \
    if (this->CheckFeature(CpuFeature::name)) TERMTRY1(term->Write(" " #prettyName), tret, cnt);

#include <system/cpuid_flags.inc>

    return tret;
}
