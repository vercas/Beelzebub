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

#pragma once

#include <metaprogramming.h>
#include <handles.h>

namespace Beelzebub { namespace Execution {
    /*  Constants  */

    extern uint32_t ElfMagicNumber;

    /*  Enumerations  */

    #define ENUM_ELFCLASS(ENUMINST) \
        ENUMINST(None  , 0) \
        ENUMINST(Elf32 , 1) \
        ENUMINST(Elf64 , 2) \

    /**
     *  Known classes for ELF identification.
     */
    enum class ElfClass : uint8_t
    {
        ENUM_ELFCLASS(ENUMINST_VAL)
    };

    ENUMOPS(ElfClass, uint8_t)

    inline ENUM_TO_STRING(ElfClass, ENUM_ELFCLASS)

    #define ENUM_ELFDATAENCODING(ENUMINST) \
        ENUMINST(None         , 0) \
        ENUMINST(LittleEndian , 1) \
        ENUMINST(BigEndian    , 2) \

    /**
     *  Known data encoding rules for ELF identification.
     */
    enum class ElfDataEncoding : uint8_t
    {
        ENUM_ELFDATAENCODING(ENUMINST_VAL)
    };

    ENUMOPS(ElfDataEncoding, uint8_t)

    #define ENUM_ELFOSABI(ENUMINST) \
        ENUMINST(None            ,  0) \
        ENUMINST(HpUx            ,  1) \
        ENUMINST(NetBsd          ,  2) \
        ENUMINST(Gnu             ,  3) \
        ENUMINST(Linux           ,  3) \
        ENUMINST(Solaris         ,  6) \
        ENUMINST(Aix             ,  7) \
        ENUMINST(Irix            ,  8) \
        ENUMINST(FreeBsd         ,  9) \
        ENUMINST(CompaqTru64     , 10) \
        ENUMINST(NovellModesto   , 11) \
        ENUMINST(OpenBsd         , 12) \
        ENUMINST(OpenVms         , 13) \
        ENUMINST(HpNonStopKernel , 14) \
        ENUMINST(AmigaResearchOs , 15) \
        ENUMINST(FenixOs         , 16) \
        ENUMINST(NuxiCloudAbi    , 17) \
        ENUMINST(OpenVos         , 18) \
        ENUMINST(Beelzebub       , 39) \
        // 'B' ^ 'e' = 0x42 ^ 0x65 = 0x27 = 39

    /**
     *  Known OS ABIs for ELF identification.
     */
    enum class ElfOsAbi : uint8_t
    {
        ENUM_ELFOSABI(ENUMINST_VAL)
    };

    ENUMOPS(ElfOsAbi, uint8_t)

    #define ENUM_ELFFILETYPE(ENUMINST) \
        ENUMINST(None                        , 0     ) \
        ENUMINST(Relocatable                 , 1     ) \
        ENUMINST(Executable                  , 2     ) \
        ENUMINST(Dynamic                     , 3     ) \
        ENUMINST(Core                        , 4     ) \
        ENUMINST(OperatingSystemSpecificLow  , 0xFE00) \
        ENUMINST(OperatingSystemSpecificHigh , 0xFE00) \
        ENUMINST(ProcessorSpecificLow        , 0xFF00) \
        ENUMINST(ProcessorSpecificHigh       , 0xFFFF) \

    /**
     *  Known object file types.
     */
    enum class ElfFileType : uint16_t
    {
        ENUM_ELFFILETYPE(ENUMINST_VAL)
    };

    ENUMOPS(ElfFileType, uint16_t)
    
    #define ENUM_ELFMACHINE(ENUMINST) \
        ENUMINST(None          ,   0) \
        ENUMINST(We32100       ,   1) \
        ENUMINST(Sparc         ,   2) \
        ENUMINST(Intel80386    ,   3) \
        ENUMINST(Motorola68000 ,   4) \
        ENUMINST(Motorola88000 ,   5) \
        ENUMINST(IntelMcu      ,   6) \
        ENUMINST(Intel80860    ,   7) \
        ENUMINST(MipsRs3000    ,   8) \
        ENUMINST(IbmSystem370  ,   9) \
        ENUMINST(MipsRs3000Le  ,  10) \
        ENUMINST(HpPaRisc      ,  15) \
        ENUMINST(FujitsuVpp500 ,  17) \
        ENUMINST(Sparc32Plus   ,  18) \
        ENUMINST(Intel80960    ,  19) \
        ENUMINST(Arm32         ,  40) \
        ENUMINST(SparcV9       ,  43) \
        ENUMINST(Ia64          ,  50) \
        ENUMINST(Amd64         ,  62) \
        ENUMINST(HuAny         ,  81) \
        ENUMINST(Arm64         , 183) \
        ENUMINST(NvidiaCuda    , 190) \
        ENUMINST(AmdGpu        , 224) \

    /**
     *  Known architectures supported by ELF files.
     */
    enum class ElfMachine : uint16_t
    {
        ENUM_ELFMACHINE(ENUMINST_VAL)
    };

    ENUMOPS(ElfMachine, uint16_t)

    #define ENUM_ELFSECTIONHEADERTYPE(ENUMINST) \
        ENUMINST(Null                       ,  0) \
        ENUMINST(ProgramBits                ,  1) \
        ENUMINST(StaticSymbolTable          ,  2) \
        ENUMINST(StringTable                ,  3) \
        ENUMINST(RelocationEntriesAppends   ,  4) \
        ENUMINST(HashTable                  ,  5) \
        ENUMINST(Dynamic                    ,  6) \
        ENUMINST(Note                       ,  7) \
        ENUMINST(NoBits                     ,  8) \
        ENUMINST(RelocationEntries          ,  9) \
        ENUMINST(SHLIB                      , 10) \
        ENUMINST(DynamicSymbolTable         , 11) \
        ENUMINST(InitializationFunctions    , 14) \
        ENUMINST(TerminationFunctions       , 15) \
        ENUMINST(PreinitializationFunctions , 16) \
        ENUMINST(SectionGroup               , 17) \
        ENUMINST(SymbolTableIndexes         , 18) \

    /**
     *  Types of ELF sections.
     */
    enum class ElfSectionHeaderType : uint32_t
    {
        ENUM_ELFSECTIONHEADERTYPE(ENUMINST_VAL)
    };

    ENUMOPS(ElfSectionHeaderType, uint32_t)

    #define ENUM_ELFSECTIONHEADERFLAGS32(ENUMINST) \
        ENUMINST(Writable           , 0x001) \
        ENUMINST(Allocated          , 0x002) \
        ENUMINST(Executable         , 0x004) \
        ENUMINST(Merge              , 0x010) \
        ENUMINST(Strings            , 0x020) \
        ENUMINST(InfoLink           , 0x040) \
        ENUMINST(LinkOrdering       , 0x080) \
        ENUMINST(OsNonconforming    , 0x100) \
        ENUMINST(GroupMember        , 0x200) \
        ENUMINST(ThreadLocalStorage , 0x400) \
        ENUMINST(Compressed         , 0x800) \

    /**
     *  Section flags for 32-bit ELF.
     */
    enum class ElfSectionHeaderFlags_32 : uint32_t
    {
        ENUM_ELFSECTIONHEADERFLAGS32(ENUMINST_VAL)
    };

    ENUMOPS(ElfSectionHeaderFlags_32, uint32_t)

    #define ENUM_ELFSECTIONHEADERFLAGS64(ENUMINST) \
        ENUMINST(Writable           , 0x001) \
        ENUMINST(Allocated          , 0x002) \
        ENUMINST(Executable         , 0x004) \
        ENUMINST(Merge              , 0x010) \
        ENUMINST(Strings            , 0x020) \
        ENUMINST(InfoLink           , 0x040) \
        ENUMINST(LinkOrdering       , 0x080) \
        ENUMINST(OsNonconforming    , 0x100) \
        ENUMINST(GroupMember        , 0x200) \
        ENUMINST(ThreadLocalStorage , 0x400) \
        ENUMINST(Compressed         , 0x800) \

    /**
     *  Section flags for 64-bit ELF.
     */
    enum class ElfSectionHeaderFlags_64 : uint64_t
    {
        ENUM_ELFSECTIONHEADERFLAGS64(ENUMINST_VAL)
    };

    ENUMOPS(ElfSectionHeaderFlags_64, uint64_t)

    /*  Structures  */

    /**
     *  Represents the information that mark the file as an object file
     *  and provide machine-independent data used to decode the contents.
     */
    struct ElfIdentification
    {
        uint32_t        MagicNumber;  /*  0 -  3 */

        ElfClass        Class;        /*  4      */
        ElfDataEncoding DataEncoding; /*  5      */

        uint8_t         Version;      /*  6      */

        ElfOsAbi        OsAbi;        /*  7      */
        uint8_t         AbiVersion;   /*  8      */
    }  __packed;
    //  Should be 9 bytes long.

    /**
     *  Represents the common initial header of an ELF file.
     */
    struct ElfHeader1
    {
        ElfIdentification Identification;                /*  0 -  8 */
        uint8_t Padding[16 - sizeof(ElfIdentification)]; /*  9 - 15 */
        //  Padding should be 7 bytes.

        ElfFileType       Type;                          /* 16 - 17 */
        ElfMachine        Machine;                       /* 18 - 19 */
        uint32_t          Version;                       /* 20 - 23 */
    } __packed;

    /**
     *  Represents the header section specific to the 32-bit ELF class.
     */
    struct ElfHeader2_32
    {
        uint32_t EntryPoint;               /* 24 - 27 */
        uint32_t ProgramHeaderTableOffset; /* 28 - 31 */
        uint32_t SectionHeaderTableOffset; /* 31 - 35 */
    } __packed;

    /**
     *  Represents the header section specific to the 64-bit ELF class.
     */
    struct ElfHeader2_64
    {
        uint64_t EntryPoint;               /* 24 - 31 */
        uint64_t ProgramHeaderTableOffset; /* 32 - 39 */
        uint64_t SectionHeaderTableOffset; /* 40 - 47 */
    } __packed;

    /**
     *  Represents the common final header of an ELF file.
     */
    struct ElfHeader3
    {                                          /*  32-bit |  64-bit */
        uint32_t Flags;                        /* 36 - 39 | 48 - 51 */
        uint16_t HeaderSize;                   /* 40 - 41 | 52 - 53 */
        uint16_t ProgramHeaderTableEntrySize;  /* 42 - 43 | 54 - 55 */
        uint16_t ProgramHeaderTableEntryCount; /* 44 - 45 | 56 - 57 */
        uint16_t SectionHeaderTableEntrySize;  /* 46 - 47 | 58 - 59 */
        uint16_t SectionHeaderTableEntryCount; /* 48 - 49 | 60 - 61 */
        uint16_t SectionNameStringTableIndex;  /* 50 - 51 | 62 - 63 */
    } __packed;

    /**
     *  Represents the header of a section in a 32-bit ELF.
     */
    struct ElfSectionHeader_32
    {
        uint32_t Name;                  /*  0 -  3 */
        ElfSectionHeaderType Type;      /*  4 -  7 */
        ElfSectionHeaderFlags_32 Flags; /*  8 - 11 */
        uint32_t Address;               /* 12 - 15 */
        uint32_t Offset;                /* 16 - 19 */
        uint32_t Size;                  /* 20 - 23 */
        uint32_t Link;                  /* 24 - 27 */
        uint32_t Info;                  /* 28 - 31 */
        uint32_t AddressAlignment;      /* 32 - 35 */
        uint32_t EntrySize;             /* 36 - 39 */
    } __packed;

    /**
     *  Represents the header of a section in a 64-bit ELF.
     */
    struct ElfSectionHeader_64
    {
        uint32_t Name;
        ElfSectionHeaderType Type;
        ElfSectionHeaderFlags_64 Flags;
        uint64_t Address;
        uint64_t Offset;
        uint64_t Size;
        uint32_t Link;
        uint32_t Info;
        uint64_t AddressAlignment;
        uint64_t EntrySize;
    } __packed;
}}
