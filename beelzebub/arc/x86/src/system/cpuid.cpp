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
        , this->VendorString.Integers[1]
        , this->VendorString.Integers[2]);

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

    if (this->CheckFeature(CpuFeature::FPU)) TERMTRY1(term->Write(" FPU"), tret, cnt);
    if (this->CheckFeature(CpuFeature::VME)) TERMTRY1(term->Write(" VME"), tret, cnt);
    if (this->CheckFeature(CpuFeature::DE)) TERMTRY1(term->Write(" DE"), tret, cnt);
    if (this->CheckFeature(CpuFeature::PSE)) TERMTRY1(term->Write(" PSE"), tret, cnt);
    if (this->CheckFeature(CpuFeature::TSC)) TERMTRY1(term->Write(" TSC"), tret, cnt);
    if (this->CheckFeature(CpuFeature::MSR)) TERMTRY1(term->Write(" MSR"), tret, cnt);
    if (this->CheckFeature(CpuFeature::PAE)) TERMTRY1(term->Write(" PAE"), tret, cnt);
    if (this->CheckFeature(CpuFeature::MCE)) TERMTRY1(term->Write(" MCE"), tret, cnt);
    if (this->CheckFeature(CpuFeature::CX8)) TERMTRY1(term->Write(" CX8"), tret, cnt);
    if (this->CheckFeature(CpuFeature::APIC)) TERMTRY1(term->Write(" APIC"), tret, cnt);
    if (this->CheckFeature(CpuFeature::SEP)) TERMTRY1(term->Write(" SEP"), tret, cnt);
    if (this->CheckFeature(CpuFeature::MTRR)) TERMTRY1(term->Write(" MTRR"), tret, cnt);
    if (this->CheckFeature(CpuFeature::PGE)) TERMTRY1(term->Write(" PGE"), tret, cnt);
    if (this->CheckFeature(CpuFeature::MCA)) TERMTRY1(term->Write(" MCA"), tret, cnt);
    if (this->CheckFeature(CpuFeature::CMOV)) TERMTRY1(term->Write(" CMOV"), tret, cnt);
    if (this->CheckFeature(CpuFeature::PAT)) TERMTRY1(term->Write(" PAT"), tret, cnt);
    if (this->CheckFeature(CpuFeature::PSE36)) TERMTRY1(term->Write(" PSE36"), tret, cnt);
    if (this->CheckFeature(CpuFeature::PSN)) TERMTRY1(term->Write(" PSN"), tret, cnt);
    if (this->CheckFeature(CpuFeature::CLFSH)) TERMTRY1(term->Write(" CLFSH"), tret, cnt);
    if (this->CheckFeature(CpuFeature::DS)) TERMTRY1(term->Write(" DS"), tret, cnt);
    if (this->CheckFeature(CpuFeature::ACPI_Thermal_Monitor)) TERMTRY1(term->Write(" ACPI_Thermal_Monitor"), tret, cnt);
    if (this->CheckFeature(CpuFeature::MMX)) TERMTRY1(term->Write(" MMX"), tret, cnt);
    if (this->CheckFeature(CpuFeature::FXSR)) TERMTRY1(term->Write(" FXSR"), tret, cnt);
    if (this->CheckFeature(CpuFeature::SSE)) TERMTRY1(term->Write(" SSE"), tret, cnt);
    if (this->CheckFeature(CpuFeature::SSE2)) TERMTRY1(term->Write(" SSE2"), tret, cnt);
    if (this->CheckFeature(CpuFeature::SS)) TERMTRY1(term->Write(" SS"), tret, cnt);
    if (this->CheckFeature(CpuFeature::HTT)) TERMTRY1(term->Write(" HTT"), tret, cnt);
    if (this->CheckFeature(CpuFeature::TM)) TERMTRY1(term->Write(" TM"), tret, cnt);
    if (this->CheckFeature(CpuFeature::PBE)) TERMTRY1(term->Write(" PBE"), tret, cnt);
    if (this->CheckFeature(CpuFeature::SSE3)) TERMTRY1(term->Write(" SSE3"), tret, cnt);
    if (this->CheckFeature(CpuFeature::PCLMULQDQ)) TERMTRY1(term->Write(" PCLMULQDQ"), tret, cnt);
    if (this->CheckFeature(CpuFeature::MONITOR)) TERMTRY1(term->Write(" MONITOR"), tret, cnt);
    if (this->CheckFeature(CpuFeature::DS_CPL)) TERMTRY1(term->Write(" DS_CPL"), tret, cnt);
    if (this->CheckFeature(CpuFeature::VMX)) TERMTRY1(term->Write(" VMX"), tret, cnt);
    if (this->CheckFeature(CpuFeature::EST)) TERMTRY1(term->Write(" EST"), tret, cnt);
    if (this->CheckFeature(CpuFeature::TM2)) TERMTRY1(term->Write(" TM2"), tret, cnt);
    if (this->CheckFeature(CpuFeature::SSSE3)) TERMTRY1(term->Write(" SSSE3"), tret, cnt);
    if (this->CheckFeature(CpuFeature::CNXT_ID)) TERMTRY1(term->Write(" CNXT_ID"), tret, cnt);
    if (this->CheckFeature(CpuFeature::FMA)) TERMTRY1(term->Write(" FMA"), tret, cnt);
    if (this->CheckFeature(CpuFeature::CMPXCHG16B)) TERMTRY1(term->Write(" CMPXCHG16B"), tret, cnt);
    if (this->CheckFeature(CpuFeature::xTPR_Update_Control)) TERMTRY1(term->Write(" xTPR_Update_Control"), tret, cnt);
    if (this->CheckFeature(CpuFeature::PDCM)) TERMTRY1(term->Write(" PDCM"), tret, cnt);
    if (this->CheckFeature(CpuFeature::PCID)) TERMTRY1(term->Write(" PCID"), tret, cnt);
    if (this->CheckFeature(CpuFeature::SSE41)) TERMTRY1(term->Write(" SSE41"), tret, cnt);
    if (this->CheckFeature(CpuFeature::SSE42)) TERMTRY1(term->Write(" SSE42"), tret, cnt);
    if (this->CheckFeature(CpuFeature::POPCNT)) TERMTRY1(term->Write(" POPCNT"), tret, cnt);
    if (this->CheckFeature(CpuFeature::AES)) TERMTRY1(term->Write(" AES"), tret, cnt);
    if (this->CheckFeature(CpuFeature::SMEP)) TERMTRY1(term->Write(" SMEP"), tret, cnt);
    if (this->CheckFeature(CpuFeature::SMAP)) TERMTRY1(term->Write(" SMAP"), tret, cnt);
    if (this->CheckFeature(CpuFeature::NX)) TERMTRY1(term->Write(" NX"), tret, cnt);
    if (this->CheckFeature(CpuFeature::SyscallSysret)) TERMTRY1(term->Write(" SyscallSysret"), tret, cnt);
    if (this->CheckFeature(CpuFeature::Page1GB)) TERMTRY1(term->Write(" Page1GB"), tret, cnt);
    if (this->CheckFeature(CpuFeature::RDTSP)) TERMTRY1(term->Write(" RDTSP"), tret, cnt);
    if (this->CheckFeature(CpuFeature::LM)) TERMTRY1(term->Write(" LM"), tret, cnt);

    return tret;
}
