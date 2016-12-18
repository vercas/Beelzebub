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

#include <beel/handles.h>

namespace Beelzebub { namespace Execution
{
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

    ENUMOPS_LITE(ElfClass)
    ENUM_TO_STRING_DECL(ElfClass, ENUM_ELFCLASS);


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

    ENUMOPS_LITE(ElfDataEncoding)
    ENUM_TO_STRING_DECL(ElfDataEncoding, ENUM_ELFDATAENCODING);


    #define ENUM_ELFOSABI(ENUMINST) \
        ENUMINST(None            ,  0) \
        ENUMINST(HpUx            ,  1) \
        ENUMINST(NetBsd          ,  2) \
        ENUMINST(Gnu             ,  3) \
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

    ENUMOPS_LITE(ElfOsAbi)
    ENUM_TO_STRING_DECL(ElfOsAbi, ENUM_ELFOSABI);


    #define ENUM_ELFFILETYPE(ENUMINST) \
        ENUMINST(None                        , 0     ) \
        ENUMINST(Relocatable                 , 1     ) \
        ENUMINST(Executable                  , 2     ) \
        ENUMINST(Dynamic                     , 3     ) \
        ENUMINST(Core                        , 4     ) \
        ENUMINST(OperatingSystemSpecificLow  , 0xFE00) \
        ENUMINST(OperatingSystemSpecificHigh , 0xFEFF) \
        ENUMINST(ProcessorSpecificLow        , 0xFF00) \
        ENUMINST(ProcessorSpecificHigh       , 0xFFFF) \

    /**
     *  Known object file types.
     */
    enum class ElfFileType : uint16_t
    {
        ENUM_ELFFILETYPE(ENUMINST_VAL)
    };

    ENUMOPS_LITE(ElfFileType)
    ENUM_TO_STRING_DECL(ElfFileType, ENUM_ELFFILETYPE);
    

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

    ENUMOPS_LITE(ElfMachine)
    ENUM_TO_STRING_DECL(ElfMachine, ENUM_ELFMACHINE);


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

    ENUMOPS_LITE(ElfSectionHeaderType)
    ENUM_TO_STRING_DECL(ElfSectionHeaderType, ENUM_ELFSECTIONHEADERTYPE);


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

    ENUMOPS(ElfSectionHeaderFlags_32)
    ENUM_TO_STRING_DECL(ElfSectionHeaderFlags_32, ENUM_ELFSECTIONHEADERFLAGS32);


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

    ENUMOPS(ElfSectionHeaderFlags_64)
    ENUM_TO_STRING_DECL(ElfSectionHeaderFlags_64, ENUM_ELFSECTIONHEADERFLAGS64);


    #define ENUM_ELFPROGRAMHEADERTYPE(ENUMINST) \
        ENUMINST(Null        ,  0) \
        ENUMINST(Load        ,  1) \
        ENUMINST(Dynamic     ,  2) \
        ENUMINST(Interpreter ,  3) \
        ENUMINST(Note        ,  4) \
        ENUMINST(SHLIB       ,  5) \
        ENUMINST(PHDR        ,  6) \
        ENUMINST(Tls         ,  7) \

    /**
     *  Types of ELF segments.
     */
    enum class ElfProgramHeaderType : uint32_t
    {
        ENUM_ELFPROGRAMHEADERTYPE(ENUMINST_VAL)
    };

    ENUMOPS_LITE(ElfProgramHeaderType)
    ENUM_TO_STRING_DECL(ElfProgramHeaderType, ENUM_ELFPROGRAMHEADERTYPE);


    #define ENUM_ELFPROGRAMHEADERFLAGS(ENUMINST) \
        ENUMINST(None       ,  0) \
        ENUMINST(Executable ,  1) \
        ENUMINST(Writable   ,  2) \
        ENUMINST(Readable   ,  4) \

    /**
     *  ELF segment flags.
     */
    enum class ElfProgramHeaderFlags : uint32_t
    {
        ENUM_ELFPROGRAMHEADERFLAGS(ENUMINST_VAL)
    };

    ENUMOPS(ElfProgramHeaderFlags)
    ENUM_TO_STRING_DECL(ElfProgramHeaderFlags, ENUM_ELFPROGRAMHEADERFLAGS);


    #define ENUM_ELFDYNAMICENTRYTAG(ENUMINST) \
        ENUMINST(DT_NULL                ,  0) \
        ENUMINST(DT_NEEDED              ,  1) \
        ENUMINST(DT_PLTRELSZ            ,  2) \
        ENUMINST(DT_PLTGOT              ,  3) \
        ENUMINST(DT_HASH                ,  4) \
        ENUMINST(DT_STRTAB              ,  5) \
        ENUMINST(DT_SYMTAB              ,  6) \
        ENUMINST(DT_RELA                ,  7) \
        ENUMINST(DT_RELASZ              ,  8) \
        ENUMINST(DT_RELAENT             ,  9) \
        ENUMINST(DT_STRSZ               , 10) \
        ENUMINST(DT_SYMENT              , 11) \
        ENUMINST(DT_INIT                , 12) \
        ENUMINST(DT_FINI                , 13) \
        ENUMINST(DT_SONAME              , 14) \
        ENUMINST(DT_RPATH               , 15) \
        ENUMINST(DT_SYMBOLIC            , 16) \
        ENUMINST(DT_REL                 , 17) \
        ENUMINST(DT_RELSZ               , 18) \
        ENUMINST(DT_RELENT              , 19) \
        ENUMINST(DT_PLTREL              , 20) \
        ENUMINST(DT_DEBUG               , 21) \
        ENUMINST(DT_TEXTREL             , 22) \
        ENUMINST(DT_JMPREL              , 23) \
        ENUMINST(BindNow                , 24) \
        ENUMINST(InitFunctionsArray     , 25) \
        ENUMINST(FiniFunctionsArray     , 26) \
        ENUMINST(InitArraySize          , 27) \
        ENUMINST(FiniArraySize          , 28) \
        ENUMINST(DT__MAX                , 30) /* Not really a tag value */ \

    /**
     *  ELF DYNAMIC segment entry tags.
     */
    enum ElfDynamicEntryTag : int32_t
    {
        ENUM_ELFDYNAMICENTRYTAG(ENUMINST_VAL)
    };

    ENUMOPS_LITE(ElfDynamicEntryTag)
    ENUM_TO_STRING_DECL(ElfDynamicEntryTag, ENUM_ELFDYNAMICENTRYTAG);


    #define ENUM_ELFRELOCATIONTYPE(ENUMINST) \
        ENUMINST(R_386_NONE             ,  0) \
        ENUMINST(R_386_32               ,  1) \
        ENUMINST(R_386_PC32             ,  2) \
        ENUMINST(R_386_GOT32            ,  3) \
        ENUMINST(R_386_PLT32            ,  4) \
        ENUMINST(R_386_COPY             ,  5) \
        ENUMINST(R_386_GLOB_DAT         ,  6) \
        ENUMINST(R_386_JMP_SLOT         ,  7) \
        ENUMINST(R_386_RELATIVE         ,  8) \
        ENUMINST(R_386_GOTOFF           ,  9) \
        ENUMINST(R_386_GOTPC            , 10) \
        ENUMINST(R_386_32PLT            , 11) \
        ENUMINST(R_AMD64_NONE           ,  0) \
        ENUMINST(R_AMD64_64             ,  1) \
        ENUMINST(R_AMD64_PC32           ,  2) \
        ENUMINST(R_AMD64_GOT32          ,  3) \
        ENUMINST(R_AMD64_PLT32          ,  4) \
        ENUMINST(R_AMD64_COPY           ,  5) \
        ENUMINST(R_AMD64_GLOB_DAT       ,  6) \
        ENUMINST(R_AMD64_JUMP_SLOT      ,  7) \
        ENUMINST(R_AMD64_RELATIVE       ,  8) \
        ENUMINST(R_AMD64_GOTPCREL       ,  9) \
        ENUMINST(R_AMD64_32             , 10) \
        ENUMINST(R_AMD64_32S            , 11) \
        ENUMINST(R_AMD64_16             , 12) \
        ENUMINST(R_AMD64_PC16           , 13) \
        ENUMINST(R_AMD64_8              , 14) \
        ENUMINST(R_AMD64_PC8            , 15) \
        ENUMINST(R_AMD64_DPTMOD64       , 16) \
        ENUMINST(R_AMD64_DTPOFF64       , 17) \
        ENUMINST(R_AMD64_TPOFF64        , 18) \
        ENUMINST(R_AMD64_TLSGD          , 19) \
        ENUMINST(R_AMD64_TLSLD          , 20) \
        ENUMINST(R_AMD64_DTPOFF32       , 21) \
        ENUMINST(R_AMD64_GOTTPOFF       , 22) \
        ENUMINST(R_AMD64_TPOFF32        , 23) \
        ENUMINST(R_AMD64_PC64           , 24) \
        ENUMINST(R_AMD64_GOTOFF64       , 25) \
        ENUMINST(R_AMD64_GOTPC32        , 26) \
        ENUMINST(R_AMD64_GOT64          , 27) \
        ENUMINST(R_AMD64_GOTPCREL64     , 28) \
        ENUMINST(R_AMD64_GOTPC64        , 29) \
        ENUMINST(R_AMD64_GOTPLT64       , 30) \
        ENUMINST(R_AMD64_PLTOFF64       , 31) \
        ENUMINST(R_AMD64_SIZE32         , 32) \
        ENUMINST(R_AMD64_SIZE64         , 33) \
        ENUMINST(R_AMD64_GOTPC32_TLSDESC, 34) \
        ENUMINST(R_AMD64_TLSDESC_CALL   , 35) \
        ENUMINST(R_AMD64_TLSDESC        , 36) \
        ENUMINST(R_AMD64_IRELATIVE      , 37) \
        ENUMINST(R_AMD64__MAX           , 38) \


    /**
     *  ELF relocation types.
     */
    enum ElfRelType : uint8_t
    {
        ENUM_ELFRELOCATIONTYPE(ENUMINST_VAL)
    };

    ENUMOPS_LITE(ElfRelType)


    #define ENUM_ELFSYMBOLBINDING(ENUMINST) \
        ENUMINST(Local      , 0x00) \
        ENUMINST(Global     , 0x10) \
        ENUMINST(Weak       , 0x20) \
        ENUMINST(Os1        , 0xA0) \
        ENUMINST(Os2        , 0xB0) \
        ENUMINST(Os3        , 0xC0) \
        ENUMINST(Proc1      , 0xD0) \
        ENUMINST(Proc2      , 0xE0) \
        ENUMINST(Proc3      , 0xF0) \

    /**
     *  ELF symbol bindings.
     */
    enum class ElfSymbolBinding : uint8_t
    {
        ENUM_ELFSYMBOLBINDING(ENUMINST_VAL)
    };

    //  Note: Yes, all the values are actually offset 4 bits left, as they would
    //  appear in the `st_info` field.

    ENUMOPS(ElfSymbolBinding)
    ENUM_TO_STRING_DECL(ElfSymbolBinding, ENUM_ELFSYMBOLBINDING);


    #define ENUM_ELFSYMBOLTYPE(ENUMINST) \
        ENUMINST(None         , 0x0) \
        ENUMINST(Object       , 0x1) \
        ENUMINST(Function     , 0x2) \
        ENUMINST(Section      , 0x3) \
        ENUMINST(File         , 0x4) \
        ENUMINST(Common       , 0x5) \
        ENUMINST(Tls          , 0x6) \
        ENUMINST(Os1          , 0xA) \
        ENUMINST(Os2          , 0xB) \
        ENUMINST(Os3          , 0xC) \
        ENUMINST(SparcRegister, 0xD) \
        ENUMINST(Proc2        , 0xE) \
        ENUMINST(Proc3        , 0xF) \

    /**
     *  ELF symbol bindings.
     */
    enum class ElfSymbolType : uint8_t
    {
        ENUM_ELFSYMBOLTYPE(ENUMINST_VAL)
    };

    ENUMOPS(ElfSymbolType)
    ENUM_TO_STRING_DECL(ElfSymbolType, ENUM_ELFSYMBOLTYPE);


    #define ENUM_ELFSYMBOLVISIBILITY(ENUMINST) \
        ENUMINST(Default      , 0x0) \
        ENUMINST(Internal     , 0x1) \
        ENUMINST(Hidden       , 0x2) \
        ENUMINST(Protected    , 0x3) \
        ENUMINST(Exported     , 0x4) \
        ENUMINST(Singleton    , 0x5) \
        ENUMINST(Eliminate    , 0x6) \

    /**
     *  ELF symbol bindings.
     */
    enum class ElfSymbolVisibility : uint8_t
    {
        ENUM_ELFSYMBOLVISIBILITY(ENUMINST_VAL)
    };

    ENUMOPS(ElfSymbolVisibility)
    ENUM_TO_STRING_DECL(ElfSymbolVisibility, ENUM_ELFSYMBOLVISIBILITY);


    #define ENUM_ELFSECTIONINDEXES(ENUMINST) \
        ENUMINST(SHN_UNDEF    , 0x0000) \
        ENUMINST(SHN_LORESERVE, 0xFF00) \
        ENUMINST(SHN_LOPROC   , 0xFF00) \
        ENUMINST(SHN_HIPROC   , 0xFF1F) \
        ENUMINST(SHN_LOOS     , 0xFF20) \
        ENUMINST(SHN_HIOS     , 0xFF3F) \
        ENUMINST(SHN_ABS      , 0xFFF1) \
        ENUMINST(SHN_COMMON   , 0xFFF2) \
        ENUMINST(SHN_XINDEX   , 0xFFFF) \
        ENUMINST(SHN_HIRESERVE, 0xFFFF) \

    /**
     *  Special ELF section indexes, with equally special meaning.
     */
    enum ElfSectionIndexes : uint16_t
    {
        ENUM_ELFSECTIONINDEXES(ENUMINST_VAL)
    };

    ENUMOPS_LITE(ElfSectionIndexes)


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
        uint32_t                  Name; /*  0 -  3 */
        ElfSectionHeaderType      Type; /*  4 -  7 */
        ElfSectionHeaderFlags_32 Flags; /*  8 - 11 */
        uint32_t               Address; /* 12 - 15 */
        uint32_t                Offset; /* 16 - 19 */
        uint32_t                  Size; /* 20 - 23 */
        uint32_t                  Link; /* 24 - 27 */
        uint32_t                  Info; /* 28 - 31 */
        uint32_t      AddressAlignment; /* 32 - 35 */
        uint32_t             EntrySize; /* 36 - 39 */
    } __packed;

    /**
     *  Represents the header of a section in a 64-bit ELF.
     */
    struct ElfSectionHeader_64
    {
        uint32_t                  Name; /*  0 -  3 */
        ElfSectionHeaderType      Type; /*  4 -  7 */
        ElfSectionHeaderFlags_64 Flags; /*  8 - 15 */
        uint64_t               Address; /* 16 - 23 */
        uint64_t                Offset; /* 24 - 31 */
        uint64_t                  Size; /* 32 - 39 */
        uint32_t                  Link; /* 40 - 43 */
        uint32_t                  Info; /* 44 - 47 */
        uint64_t      AddressAlignment; /* 48 - 55 */
        uint64_t             EntrySize; /* 56 - 63 */
    } __packed;


    /**
     *  Represents the header of a segment in a 32-bit ELF.
     */
    struct ElfProgramHeader_32
    {
        ElfProgramHeaderType   Type; /*  0 -  3 */
        uint32_t             Offset; /*  4 -  7 */
        uint32_t              VAddr; /*  8 - 11 */
        uint32_t              PAddr; /* 12 - 15 */
        uint32_t              PSize; /* 16 - 19 */
        uint32_t              VSize; /* 20 - 23 */
        ElfProgramHeaderFlags Flags; /* 24 - 27 */
        uint32_t          Alignment; /* 28 - 31 */
    } __packed;

    /**
     *  Represents the header of a segment in a 64-bit ELF.
     */
    struct ElfProgramHeader_64
    {
        ElfProgramHeaderType   Type; /*  0 -  3 */
        ElfProgramHeaderFlags Flags; /*  4 -  7 */
        uint64_t             Offset; /*  8 - 15 */
        uint64_t              VAddr; /* 16 - 23 */
        uint64_t              PAddr; /* 24 - 31 */
        uint64_t              PSize; /* 32 - 39 */
        uint64_t              VSize; /* 40 - 47 */
        uint64_t          Alignment; /* 48 - 55 */
    } __packed;


    /**
     *  Represents an entry in the DYNAMIC program header of a 32-bit ELF.
     */
    struct ElfDynamicEntry_32
    {
        ElfDynamicEntryTag Tag; /*  0 -  3 */
        uint32_t         Value; /*  4 -  7 */
    } __packed;

    /**
     *  Represents an entry in the DYNAMIC program header of a 64-bit ELF.
     */
    struct ElfDynamicEntry_64
    {
        ElfDynamicEntryTag Tag; /*  0 -  4 */
        uint32_t    TagPadding; /*  4 -  7 */
        uint64_t         Value; /*  8 - 15 */
    } __packed;


    /**
     *  Represents the type of a relocation entry's info in a 64-bit ELF.
     */
    union ElfRelEntryInfoType_64
    {
        uint32_t         Value; /*  0 -  3 */
        uint8_t       Bytes[4]; /*  0 -  3 */

        inline constexpr ElfRelEntryInfoType_64(uint32_t data, ElfRelType type)
            : Value((uint8_t)type | (data << 8)) { }

        inline uint32_t   GetData() const { return this->Value >> 8;             }
        inline ElfRelType GetType() const { return (ElfRelType)(this->Bytes[0]); }
    } __packed;


    /**
     *  Represents the info of a relocation entry in a 32-bit ELF.
     */
    union ElfRelEntryInfo_32
    {
        uint32_t         Value; /*  0 -  3 */
        uint8_t       Bytes[4]; /*  0 -  3 */

        inline constexpr ElfRelEntryInfo_32(uint32_t symbol, ElfRelType type)
            : Value((uint8_t)type | (symbol << 8)) { }

        inline uint32_t GetSymbol() const { return this->Value >> 8;             }
        inline ElfRelType GetType() const { return (ElfRelType)(this->Bytes[0]); }
    } __packed;

    /**
     *  Represents the info of a relocation entry in a 64-bit ELF.
     */
    struct ElfRelEntryInfo_64
    {
        ElfRelEntryInfoType_64 Type; /*  0 -  3 */
        uint32_t             Symbol; /*  4 -  7 */

        inline constexpr ElfRelEntryInfo_64(uint32_t symbol, ElfRelType type, uint32_t data)
            : Type({data, type}), Symbol(symbol) { }

        inline uint32_t GetSymbol() const { return this->Symbol;         }
        inline ElfRelType GetType() const { return this->Type.GetType(); }
        inline uint32_t   GetData() const { return this->Type.GetData(); }
    } __packed;


    /**
     *  Represents a Rel entry in a 32-bit ELF.
     */
    struct ElfRelEntry_32
    {
        uint32_t         Offset; /*  0 -  3 */
        ElfRelEntryInfo_32 Info; /*  4 -  7 */
    } __packed;

    /**
     *  Represents a Rel entry in a 64-bit ELF.
     */
    struct ElfRelEntry_64
    {
        uint64_t         Offset; /*  0 -  7 */
        ElfRelEntryInfo_64 Info; /*  8 - 15 */
    } __packed;


    /**
     *  Represents a Rela entry in a 32-bit ELF.
     */
    struct ElfRelaEntry_32 : public ElfRelEntry_32
    {
        int32_t          Append; /*  8 - 11 */
    } __packed;

    /**
     *  Represents a Rela entry in a 64-bit ELF.
     */
    struct ElfRelaEntry_64 : public ElfRelEntry_64
    {
        int64_t          Append; /* 16 - 23 */
    } __packed;


    /**
     *  Represents the info of a symbol in an ELF.
     */
    union ElfSymbolInfo
    {
        uint8_t            Value; /* byte 0     */
        ElfSymbolType       Type; /* bits 0 - 3 */
        ElfSymbolBinding Binding; /* bits 4 - 7 */

        inline constexpr ElfSymbolInfo(ElfSymbolBinding binding, ElfSymbolType type)
            : Value(((uint8_t)type & 0xF) | ((uint8_t)binding & 0xF0)) { }

        inline ElfSymbolType    GetType()    const { return this->Type    & 0xF;  }
        inline ElfSymbolBinding GetBinding() const { return this->Binding & 0xF0; }
    } __packed;



    /**
     *  Represents the st_other field of a symbol in an ELF.
     */
    union ElfSymbolOther
    {
        uint8_t                  Value; /* byte 0     */
        ElfSymbolVisibility Visibility; /* bits 0 - ? */

        //  NOTE: Documentation found here:
        //  https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-79797.html#chapter6-tbl-21
        //  ... says the mask for visiblity should be 0x3, but meh.

        inline constexpr ElfSymbolOther(ElfSymbolVisibility visibility)
            : Value((uint8_t)visibility & 0x07) { }

        inline ElfSymbolVisibility GetVisibility() const { return this->Visibility & 0x7; }
    } __packed;


    /**
     *  Represents a symbol in a 32-bit ELF.
     */
    struct ElfSymbol_32
    {
        uint32_t           Name; /*  0 -  3 */
        uint32_t          Value; /*  4 -  7 */
        uint32_t           Size; /*  8 - 11 */
        ElfSymbolInfo      Info; /* 12      */
        ElfSymbolOther    Other; /* 13      */
        uint16_t   SectionIndex; /* 14 - 15 */
    } __packed;

    /**
     *  Represents a symbol in a 64-bit ELF.
     */
    struct ElfSymbol_64
    {
        uint32_t           Name; /*  0 -  3 */
        ElfSymbolInfo      Info; /*  4      */
        ElfSymbolOther    Other; /*  5      */
        uint16_t   SectionIndex; /*  6 -  7 */
        uint64_t          Value; /*  8 - 15 */
        uint64_t           Size; /* 16 - 23 */
    } __packed;


    /**
     *  Represents the header of the hashtable in an ELF.
     */
    struct ElfHashTable
    {
        typedef uint32_t BucketType;
        typedef uint32_t ChainType;

        uint32_t    BucketCount; /*  0 -  3 */
        uint32_t     ChainCount; /*  4 -  7 */

        inline size_t GetTotalSize() const { return sizeof(*this) + this->BucketCount * sizeof(BucketType) + this->ChainCount * sizeof(ChainType); }

        inline uint32_t const * GetBuckets() const { return reinterpret_cast<uint32_t const *>(this) + 2; }
        inline uint32_t const * GetChains () const { return reinterpret_cast<uint32_t const *>(this) + 2 + this->BucketCount; }

        inline uint32_t GetBucket(size_t const ind) const { return this->GetBuckets()[ind]; }
        inline uint32_t GetChain (size_t const ind) const { return this->GetChains()[ind]; }
    } __packed;


    /*  ELF class  */


    #define ENUM_ELFVALIDATIONRESULT(ENUMINST) \
        ENUMINST(Success                    ,  0) \
        ENUMINST(InvalidClass               ,  1) \
        ENUMINST(SegmentRangeInvalid        ,  2) \
        ENUMINST(HeaderRejected             ,  3) \
        ENUMINST(StructureSizeMismatch      ,  4) \
        ENUMINST(SegmentHeadersOutOfBounds  ,  5) \
        ENUMINST(SegmentOutOfBounds         ,  6) \
        ENUMINST(SegmentNonCongruent        ,  7) \
        ENUMINST(SegmentsOverlap            ,  8) \
        ENUMINST(SegmentOutOfLoad           ,  9) \
        ENUMINST(SegmentInPartialLoad       , 10) \
        ENUMINST(DtEntryOutOfBounds         , 11) \
        ENUMINST(DtEntryMissing             , 12) \
        ENUMINST(DtEntryMultiplicate        , 13) \
        ENUMINST(DtEntryAddressOutOfBounds  , 14) \
        ENUMINST(DtEntryAddressInPartialLoad, 15) \
        ENUMINST(DtInvalidPltRelocationType , 16) \
        ENUMINST(DtEntryWrongSegmentFlags   , 17) \
        ENUMINST(Unloadable                 , 18) \
        ENUMINST(Unrelocatable              , 19) \
        ENUMINST(LoadFailure                , 20) \
        ENUMINST(DynamicSegmentMultiplicate , 21) \
        ENUMINST(Unlocated                  , 22) \
        ENUMINST(UnknownRelocationType      , 23) \
        ENUMINST(UnsupportedRelocationType  , 24) \
        ENUMINST(RelocationOutOfLoad        , 25) \
        ENUMINST(RelocationInPartialLoad    , 26) \
        ENUMINST(RelocationUnchangeable     , 27) \
        ENUMINST(WrongMagicNumber           , 28) \
        ENUMINST(WrongEndianness            , 29) \

    /**
     *  Results of ELF validation.
     */
    enum class ElfValidationResult : uint8_t
    {
        ENUM_ELFVALIDATIONRESULT(ENUMINST_VAL)
    };

    ENUMOPS_LITE(ElfValidationResult)
    ENUM_TO_STRING_DECL(ElfValidationResult, ENUM_ELFVALIDATIONRESULT);


    enum class RangeLoadStatus
    {
        FullyLoaded = 0,
        CompletelyAbsent = 1,
        PartiallyLoaded = 2,
        OptionsNotMet = 3,
    };

    enum class RangeLoadOptions
    {
        None = 0,
        Writable = 1,
        Executable = 2,
    };


    /**
     *  Represents the header of the hashtable in an ELF.
     */
    class Elf
    {
    public:
        /*  Types  */

        typedef bool (* SegmentValidatorFunc)(uintptr_t addr, size_t size, ElfProgramHeaderFlags flags, void * data);
        typedef bool (* HeaderValidatorFunc )(ElfHeader1 const * header, void * data);

        typedef bool (* SegmentMapper32Func  )(uintptr_t loc, uintptr_t img, ElfProgramHeader_32 const & phdr, void * data);
        typedef bool (* SegmentUnmapper32Func)(uintptr_t loc, ElfProgramHeader_32 const & phdr, void * data);

        typedef bool (* SegmentMapper64Func  )(uintptr_t loc, uintptr_t img, ElfProgramHeader_64 const & phdr, void * data);
        typedef bool (* SegmentUnmapper64Func)(uintptr_t loc, ElfProgramHeader_64 const & phdr, void * data);

        /**
         *  Represents the header of the hashtable in an ELF.
         */
        struct Symbol
        {
            char const * Name;
            uintptr_t Value;
            size_t Size;
            ElfSymbolType Type;
            ElfSymbolBinding Binding;
            ElfSymbolVisibility Visibility;
            bool Exists, Defined;
        };

        typedef Symbol (* SymbolResolverFunc)(char const * name, void * data);

        /*  Statics  */

        static uint32_t Hash(char const * name);

        /*  Constructors  */

        Elf(void const * addr, size_t size);

        inline Elf(uintptr_t addr, size_t size) : Elf(reinterpret_cast<void const *>(addr), size) { }

        Elf() = default;

        /*  Methods  */

    private:
        ElfValidationResult ValidateParseElf32(SegmentValidatorFunc segval, void * valdata);
        ElfValidationResult ValidateParseElf64(SegmentValidatorFunc segval, void * valdata);

        ElfValidationResult ValidateParseDt32(ElfDynamicEntry_32 const * dts);
        ElfValidationResult ValidateParseDt64(ElfDynamicEntry_64 const * dts);

        Symbol GetSymbol32(uint32_t index) const;
        Symbol GetSymbol64(uint32_t index) const;

    public:
        ElfValidationResult ValidateAndParse(HeaderValidatorFunc headerval, SegmentValidatorFunc segval, void * valdata);
        ElfValidationResult Relocate(uintptr_t newAddress);

        ElfValidationResult LoadAndValidate32(SegmentMapper32Func segmap, SegmentUnmapper32Func segunmap, SymbolResolverFunc symres, void * lddata) const;
        ElfValidationResult LoadAndValidate64(SegmentMapper64Func segmap, SegmentUnmapper64Func segunmap, SymbolResolverFunc symres, void * lddata) const;

        Symbol GetSymbol(uint32_t index) const;
        Symbol GetSymbol(char const * name) const;

        RangeLoadStatus CheckRangeLoaded32(uint32_t rStart, uint32_t rSize, RangeLoadOptions opts = RangeLoadOptions::None) const;
        RangeLoadStatus CheckRangeLoaded64(uint64_t rStart, uint64_t rSize, RangeLoadOptions opts = RangeLoadOptions::None) const;

        /*  Properties  */

        inline bool IsElf32() const { return this->H1->Identification.Class == ElfClass::Elf32; }
        inline bool IsElf64() const { return this->H1->Identification.Class == ElfClass::Elf64; }

        inline ElfHeader2_32 const * GetH2_32() const
        {
            return this->IsElf32()
                ? reinterpret_cast<ElfHeader2_32 const *>(this->H1 + 1)
                : nullptr;
        }

        inline ElfHeader2_64 const * GetH2_64() const
        {
            return this->IsElf64()
                ? reinterpret_cast<ElfHeader2_64 const *>(this->H1 + 1)
                : nullptr;
        }

        inline ElfHeader3 const * GetH3() const
        {
            if (this->IsElf32())
                return reinterpret_cast<ElfHeader3 const *>(this->Start + sizeof(ElfHeader1) + sizeof(ElfHeader2_32));
            else if (this->IsElf64())
                return reinterpret_cast<ElfHeader3 const *>(this->Start + sizeof(ElfHeader1) + sizeof(ElfHeader2_64));
            else
                return nullptr;
        }

        inline ElfProgramHeader_32 const * GetPhdrs_32() const
        {
            if (this->IsElf32())
            {
                auto H2 = reinterpret_cast<ElfHeader2_32 const *>(this->H1 + 1);

                return reinterpret_cast<ElfProgramHeader_32 const *>(this->Start + H2->ProgramHeaderTableOffset);
            }
            else
                return nullptr;
        }

        inline ElfProgramHeader_64 const * GetPhdrs_64() const
        {
            if (this->IsElf64())
            {
                auto H2 = reinterpret_cast<ElfHeader2_64 const *>(this->H1 + 1);

                return reinterpret_cast<ElfProgramHeader_64 const *>(this->Start + H2->ProgramHeaderTableOffset);
            }
            else
                return nullptr;
        }

        inline size_t GetSizeInMemory() const
        {
            return this->EndAddress - this->BaseAddress;
        }

        inline ptrdiff_t GetLocationDifference() const
        {
            if (this->H1->Type == ElfFileType::Dynamic)
                return this->NewLocation - this->BaseAddress;
            else
                return 0;
        }

        inline uintptr_t GetEntryPoint() const
        {
            if (this->IsElf32())
                return this->GetLocationDifference() + reinterpret_cast<ElfHeader2_32 const *>(this->H1 + 1)->EntryPoint;
            else
                return this->GetLocationDifference() + reinterpret_cast<ElfHeader2_64 const *>(this->H1 + 1)->EntryPoint;
        }

        /*  Fields  */

        union { size_t Size; uint64_t Dummy0; };

        union
        {
            ElfHeader1 const * H1;
            uintptr_t Start;
            uint64_t Dummy1;
        };

        union { uintptr_t BaseAddress; uint64_t Dummy2; };

        union { uintptr_t EndAddress; uint64_t Dummy3; };

        union { uintptr_t NewLocation; uint64_t Dummy4; };

        bool Symbolic, TextRelocation, Loadable;

        //  Dynamic section/segment.

        union
        {
            ElfDynamicEntry_32 const * DT_32;
            ElfDynamicEntry_64 const * DT_64;
            uint64_t Dummy5;
        };

        union { size_t DT_Size; uint64_t Dummy6; };

        union { size_t DT_Count; uint64_t Dummy7; };

        //  REL relocations

        union
        {
            ElfRelEntry_32 const * REL_32;
            ElfRelEntry_64 const * REL_64;
            uint64_t Dummy8;
        };

        union { size_t REL_Size; uint64_t Dummy9; };

        //  RELA relocations

        union
        {
            ElfRelaEntry_32 const * RELA_32;
            ElfRelaEntry_64 const * RELA_64;
            uint64_t Dummy10;
        };

        union { size_t RELA_Size; uint64_t Dummy11; };

        //  PLT relocations

        union
        {
            ElfRelEntry_32 const * PLT_REL_32;
            ElfRelEntry_64 const * PLT_REL_64;
            ElfRelaEntry_32 const * PLT_RELA_32;
            ElfRelaEntry_64 const * PLT_RELA_64;
            uint64_t Dummy12;
        };

        union { size_t PLT_REL_Size; uint64_t Dummy13; };
        ElfDynamicEntryTag PLT_REL_Type;
        union { uintptr_t PLT_GOT; uint64_t Dummy14; };

        //  Dynamic symbol table

        union
        {
            ElfSymbol_32 * DYNSYM_32;
            ElfSymbol_64 * DYNSYM_64;
            uint64_t Dummy15;
        };

        union { size_t DYNSYM_Count; uint64_t Dummy16; };

        //  String table (for dynamic symbols)

        union { char const * STRTAB; uint64_t Dummy17; };
        union { size_t STRTAB_Size; uint64_t Dummy18; };

        //  Hash table (also for dynamic symbols)

        union { ElfHashTable const * HASH; uint64_t Dummy19; };

        //  Initializers and finalizers

        union { ActionFunction0 INIT; uint64_t Dummy20; };
        union { ActionFunction0 FINI; uint64_t Dummy21; };

        //  TLS segment.

        union
        {
            ElfProgramHeader_32 const * TLS_32;
            ElfProgramHeader_64 const * TLS_64;
            uint64_t Dummy22;
        };
    };
}}
