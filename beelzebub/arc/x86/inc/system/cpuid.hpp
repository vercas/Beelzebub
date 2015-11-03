/**
 *  CPU vendor names and vendor strings may be registered trademarks of
 *  or copyrighted by their respective owners. They do not belong to me,
 *  naturally.
 */

#pragma once

#include <terminals/base.hpp>
#include <metaprogramming.h>

//  FORMAT:
//   0: 7 - Bit index
//   8:31 - Value index

#define FEATUREBIT(varInd, bitInd) (((varInd) << 8U) | ((bitInd) & 0xFFU))
#define FEATUREBITEX(val, varInd, bit) do { \
    varInd = val >> 8U;                     \
    bit = 1U << (val & 0xFFU);              \
} while (false)

using namespace Beelzebub::Terminals;

namespace Beelzebub { namespace System
{
    /**
     *  Known/supported CPU vendors
     */
    enum class CpuVendor
    {
        //  The vendor is not yet known.
        Unknown = 0,
        //  Inventor of the x86 sieries.
        Intel = 1,
        //  Creator of the AMD64 specification.
        Amd = 2,
    };

    /**
     *  Known/supported CPU features
     */
    enum class CpuFeature : uint32_t
    {
#define CPUID_FEATURE(name, regInd, bitInd, _) name = FEATUREBIT(regInd, bitInd),

#include <system/cpuid_flags.inc>
    };

    /**
     *  Represents the identity of a CPU.
     */
    class CpuId
    {
    public:

        /*  Statics  */

        static __bland __forceinline void Execute(uint32_t in1, uint32_t & a, uint32_t & b, uint32_t & c, uint32_t & d)
        {
            asm volatile ( "cpuid"
                         : "=a" (a), "=b" (b), "=c" (c), "=d" (d)
                         : "a" (in1));
        }

        static __bland __forceinline void Execute(uint32_t in1, uint32_t in2, uint32_t & a, uint32_t & b, uint32_t & c, uint32_t & d)
        {
            asm volatile ( "cpuid"
                         : "=a" (a), "=b" (b), "=c" (c), "=d" (d)
                         : "a" (in1), "c" (in2));
        }

        /*  Cosntructor(s)  */

        CpuId() = default;
        CpuId(CpuId const&) = default;

        /*  Operations  */

        __cold __bland void Initialize();

        __cold __bland void InitializeIntel();
        __cold __bland void InitializeAmd();

        /*  Fields  */

        uint32_t MaxStandardValue;
        uint32_t MaxExtendedValue;

        union
        {
            char Characters[13];
            uint32_t Integers[3];
        } VendorString;

        union
        {
            char Characters[49];
            uint32_t Integers[12];
        } ProcessorName;

        //char VendorString[13], ProcessorName[49];
        CpuVendor Vendor;

        uint32_t VersionInformation, FeatureFlagsStandardB;
        uint32_t ExtendedSignature, FeatureFlagsExtendedB, FeatureFlagsExtendedC;
        uint32_t FeatureIntegers[3];

        /*  Info extraction  */

        __hot __bland bool CheckFeature(const CpuFeature feature) const;

        __bland __forceinline uint8_t GetSteppingId() const
        {
            return (uint8_t)( this->VersionInformation & 0x0000000F       );
        }

        __bland __forceinline uint8_t GetBaseModelId() const
        {
            return (uint8_t)((this->VersionInformation & 0x000000F0) >> 4);
        }

        __bland __forceinline uint8_t GetBaseFamilyId() const
        {
            return (uint8_t)((this->VersionInformation & 0x00000F00) >> 8);
        }

        __bland __forceinline uint8_t GetProcessorType() const
        {
            return (uint8_t)((this->VersionInformation & 0x00003000) >> 12);
        }

        __bland __forceinline uint8_t GetExtendedModelId() const
        {
            return (uint8_t)((this->VersionInformation & 0x000F0000) >> 16);
        }

        __bland __forceinline uint8_t GetExtendedFamilyId() const
        {
            return (uint8_t)((this->VersionInformation & 0x0FF00000) >> 20);
        }

        __bland uint8_t GetModel() const;

        __bland __forceinline uint8_t GetFamily() const
        {
            uint8_t baseFamily = this->GetBaseFamilyId();

            if (baseFamily == 0x0F)
                return (uint8_t)((this->GetExtendedFamilyId() << 4) | 0xF);
            else
                return baseFamily;
        }


        __bland __forceinline uint8_t GetBrandIndex() const
        {
            return (uint8_t)( this->FeatureFlagsStandardB & 0x000000FF       );
        }

        __bland __forceinline size_t GetClflushLineSize() const
        {
            return (size_t )((this->FeatureFlagsStandardB & 0x0000FF00) >> 5);
            //  The value needs to be multiplied by 8 (shifted left by 3).
        }

        __bland __forceinline uint8_t GetMaxLogicalProcessors() const
        {
            return (uint8_t)((this->FeatureFlagsStandardB & 0x00FF0000) >> 16);
        }

        __bland __forceinline uint8_t GetInitialApicId() const
        {
            return (uint8_t)((this->FeatureFlagsStandardB & 0xFF000000) >> 24);
        }

        /*  Debug  */

        __cold __bland TerminalWriteResult PrintToTerminal(TerminalBase * const term) const;
    };
}}
