#pragma once

#include <metaprogramming.h>
#include <handles.h>

namespace Beelzebub { namespace Execution {
    /*  Constants  */

    extern ElfMagicNumber;

    /*  Enumerations  */

    /**
     *  Known classes for ELF identification.
     */
    enum class ElfClass : uint8_t
    {
        None  = 0,
        Elf32 = 1,
        Elf64 = 2,
    };

    ENUMOPS(ElfClass, uint8_t)

    /**
     *  Known data encoding rules for ELF identification.
     */
    enum class ElfDataEncoding : uint8_t
    {
        None         = 0,
        LittleEndian = 1,
        BigEndian    = 2,
    };

    ENUMOPS(ElfDataEncoding, uint8_t)

    /**
     *  Known OS ABIs for ELF identification.
     */
    enum class ElfOsAbi : uint8_t
    {
        None            =  0,
        HpUx            =  1,
        NetBsd          =  2,
        Gnu             =  3,
        Linux           =  3,

        Solaris         =  6,
        Aix             =  7,
        Irix            =  8,
        FreeBsd         =  9,
        CompaqTru64     = 10,
        NovellModesto   = 11,
        OpenBsd         = 12,
        OpenVms         = 13,
        HpNonStopKernel = 14,
        AmigaResearchOs = 15,
        FenixOs         = 16,
        NuxiCloudAbi    = 17,
        OpenVos         = 18,

        Beelzebub       = 39, // 'B' ^ 'e' = 0x42 ^ 0x65 = 0x27 = 39
    };

    ENUMOPS(ElfOsAbi, uint8_t)

    /**
     *  Known object file types.
     */
    enum class ElfFileType : uint16_t
    {
        None                        = 0,
        Relocatable                 = 1,
        Executable                  = 2,
        Dynamic                     = 3,
        Core                        = 4,

        OperatingSystemSpecificLow  = 0xFE00,
        OperatingSystemSpecificHigh = 0xFE00,

        ProcessorSpecificLow        = 0xFF00,
        ProcessorSpecificHigh       = 0xFFFF,
    };

    ENUMOPS(ElfFileType, uint16_t)

    /**
     *  Known object file types.
     */
    enum class ElfMachine : uint16_t
    {
        None          =   0,
        We32100       =   1,
        Sparc         =   2,
        Intel80386    =   3,
        Motorola68000 =   4,
        Motorola88000 =   5,
        IntelMcu      =   6,
        Intel80860    =   7,
        MipsRs3000    =   8,
        IbmSystem370  =   9,
        MipsRs3000Le  =  10,

        HpPaRisc      =  15,

        FujitsuVpp500 =  17,
        Sparc32Plus   =  18,
        Intel80960    =  19,

        Arm32         =  40,
        SparcV9       =  43,

        Ia64          =  50,

        Amd64         =  62,

        HuAny         =  81,

        Arm64         = 183,

        NvidiaCuda    = 190,

        AmdGpu        = 224,
    };

    ENUMOPS(ElfMachine, uint16_t)

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
}}
