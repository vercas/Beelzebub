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

#pragma once

#include <beel/terminals/base.hpp>

//  FORMAT:
//   0: 7 - Bit index
//   8:31 - Value index

#define FEATUREBIT(varInd, bitInd) (((varInd) << 8U) | ((bitInd) & 0xFFU))
#define FEATUREBITEX(val, varInd, bit) do { \
    varInd = val >> 8U;                     \
    bit = 1U << (val & 0xFFU);              \
} while (false)

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

        static __forceinline void Execute(uint32_t in1, uint32_t & a, uint32_t & b, uint32_t & c, uint32_t & d)
        {
            asm volatile ( "cpuid"
                         : "=a" (a), "=b" (b), "=c" (c), "=d" (d)
                         : "a" (in1));
        }

        static __forceinline void Execute(uint32_t in1, uint32_t in2, uint32_t & a, uint32_t & b, uint32_t & c, uint32_t & d)
        {
            asm volatile ( "cpuid"
                         : "=a" (a), "=b" (b), "=c" (c), "=d" (d)
                         : "a" (in1), "c" (in2));
        }

        /*  Cosntructor(s)  */

        CpuId() = default;
        CpuId(CpuId const&) = default;

        /*  Operations  */

        __cold void Initialize();

        __cold void InitializeIntel();
        __cold void InitializeAmd();

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

        __hot bool CheckFeature(const CpuFeature feature) const;

        __forceinline uint8_t GetSteppingId() const
        {
            return (uint8_t)( this->VersionInformation & 0x0000000F       );
        }

        __forceinline uint8_t GetBaseModelId() const
        {
            return (uint8_t)((this->VersionInformation & 0x000000F0) >> 4);
        }

        __forceinline uint8_t GetBaseFamilyId() const
        {
            return (uint8_t)((this->VersionInformation & 0x00000F00) >> 8);
        }

        __forceinline uint8_t GetProcessorType() const
        {
            return (uint8_t)((this->VersionInformation & 0x00003000) >> 12);
        }

        __forceinline uint8_t GetExtendedModelId() const
        {
            return (uint8_t)((this->VersionInformation & 0x000F0000) >> 16);
        }

        __forceinline uint8_t GetExtendedFamilyId() const
        {
            return (uint8_t)((this->VersionInformation & 0x0FF00000) >> 20);
        }

        uint8_t GetModel() const;

        __forceinline uint8_t GetFamily() const
        {
            uint8_t baseFamily = this->GetBaseFamilyId();

            if (baseFamily == 0x0F)
                return (uint8_t)((this->GetExtendedFamilyId() << 4) | 0xF);
            else
                return baseFamily;
        }


        __forceinline uint8_t GetBrandIndex() const
        {
            return (uint8_t)( this->FeatureFlagsStandardB & 0x000000FF       );
        }

        __forceinline size_t GetClflushLineSize() const
        {
            return (size_t )((this->FeatureFlagsStandardB & 0x0000FF00) >> 5);
            //  The value needs to be multiplied by 8 (shifted left by 3).
        }

        __forceinline uint8_t GetMaxLogicalProcessors() const
        {
            return (uint8_t)((this->FeatureFlagsStandardB & 0x00FF0000) >> 16);
        }

        __forceinline uint8_t GetInitialApicId() const
        {
            return (uint8_t)((this->FeatureFlagsStandardB & 0xFF000000) >> 24);
        }

        /*  Debug  */

        __cold Terminals::TerminalWriteResult PrintToTerminal(Terminals::TerminalBase * const term) const;
    };
}}
