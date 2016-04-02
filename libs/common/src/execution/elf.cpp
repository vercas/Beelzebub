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

#include <execution/elf.hpp>
#include <terminals/base.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

/*  Constants  */

uint32_t Beelzebub::Execution::ElfMagicNumber = 0x464C457F;

/*  Enum-to-string  */

ENUM_TO_STRING_EX2(ElfClass, ENUM_ELFCLASS, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfDataEncoding, ENUM_ELFDATAENCODING, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfOsAbi, ENUM_ELFOSABI, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfFileType, ENUM_ELFFILETYPE, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfMachine, ENUM_ELFMACHINE, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfSectionHeaderType, ENUM_ELFSECTIONHEADERTYPE, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfSectionHeaderFlags_32, ENUM_ELFSECTIONHEADERFLAGS32, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfSectionHeaderFlags_64, ENUM_ELFSECTIONHEADERFLAGS64, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfProgramHeaderType, ENUM_ELFPROGRAMHEADERTYPE, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfProgramHeaderFlags, ENUM_ELFPROGRAMHEADERFLAGS, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfDynamicEntryTag, ENUM_ELFDYNAMICENTRYTAG, Beelzebub::Execution)

/*  Now to implement some << operator magic.  */

namespace Beelzebub { namespace Terminals
{
    /*  First, the enums  */

    #define SPAWN_ENUM(eName) \
    template<> \
    TerminalBase & operator << <eName>(TerminalBase & term, eName const value) \
    { \
        return term << (__underlying_type(eName))(value) << " (" << EnumToString(value) << ")"; \
    }

    SPAWN_ENUM(ElfClass)
    SPAWN_ENUM(ElfDataEncoding)
    SPAWN_ENUM(ElfOsAbi)
    SPAWN_ENUM(ElfFileType)
    SPAWN_ENUM(ElfMachine)

    SPAWN_ENUM(ElfProgramHeaderType)

    SPAWN_ENUM(ElfDynamicEntryTag)

    template<>
    TerminalBase & operator << <ElfProgramHeaderFlags>(TerminalBase & term, ElfProgramHeaderFlags const value)
    {
        term.WriteHex32((uint32_t)value);
        term.Write(" (");
        term.Write(0 != (value & ElfProgramHeaderFlags::Readable  ) ? "R" : " ");
        term.Write(0 != (value & ElfProgramHeaderFlags::Writable  ) ? "W" : " ");
        term.Write(0 != (value & ElfProgramHeaderFlags::Executable) ? "E" : " ");
        term.Write(")");

        return term;
    }

    /*  Then, the structs  */

    template<>
    TerminalBase & operator << <ElfHeader1>(TerminalBase & term, ElfHeader1 const value)
    {
        term.WriteFormat("%s\"%S\" (%B)"
            , "[ELF Header 1 | Magic "
            , (size_t)4, &(value.Identification.MagicNumber)
            , value.Identification.MagicNumber == ElfMagicNumber);

        return term << "; Class " << value.Identification.Class
            << "; D. Encoding " << value.Identification.DataEncoding
            << "; Version " << value.Identification.Version
            << "; OS ABI " << value.Identification.OsAbi
            << "; ABI Version " << value.Identification.AbiVersion
            << "; Type " << value.Type
            << "; Machine " << value.Machine
            << "; Version " << value.Version
            << "]";
    }

    template<>
    TerminalBase & operator << <ElfHeader2_32>(TerminalBase & term, ElfHeader2_32 const value)
    {
        return term << "[ELF Header 2, ELF32 | Entry Point " << value.EntryPoint
            << "; PHTO " << value.ProgramHeaderTableOffset
            << "; SHTO " << value.SectionHeaderTableOffset
            << "]";
    }

    template<>
    TerminalBase & operator << <ElfHeader2_64>(TerminalBase & term, ElfHeader2_64 const value)
    {
        return term << "[ELF Header 2, ELF64 | Entry Point " << value.EntryPoint
            << "; PHTO " << value.ProgramHeaderTableOffset
            << "; SHTO " << value.SectionHeaderTableOffset
            << "]";
    }

    template<>
    TerminalBase & operator << <ElfHeader3>(TerminalBase & term, ElfHeader3 const value)
    {
        term.Write("[ELF Header 3 | Flags ");
        term.WriteHex32(value.Flags);
        term.Write("; Header Size ");
        term.WriteHex32(value.HeaderSize);
        term.Write(" (");
        term.WriteUIntD(value.HeaderSize);

        return term << "); PHTE Size " << value.ProgramHeaderTableEntrySize
            << "; PHTE Count " << value.ProgramHeaderTableEntryCount
            << "; SHTE Size " << value.SectionHeaderTableEntrySize
            << "; SHTE Count " << value.SectionHeaderTableEntryCount
            << "; SNST Index " << value.SectionNameStringTableIndex
            << "]";
    }

    template<>
    TerminalBase & operator << <ElfProgramHeader_32>(TerminalBase & term, ElfProgramHeader_32 const value)
    {
        term << "[ELF Program Header, ELF32 | Flags " << value.Flags
            << "; Offset ";
        term.WriteHex32(value.Offset);

        term.Write("; VAddr ");
        term.WriteHex32(value.VAddr);
        term.Write("; PAddr ");
        term.WriteHex32(value.PAddr);
        term.Write("; VSize ");
        term.WriteHex32(value.VSize);
        term.Write("; PSize ");
        term.WriteHex32(value.PSize);
        term.Write("; Alignment ");
        term.WriteHex32(value.Alignment);

        return term << "; Type " << value.Type << "]";
    }

    template<>
    TerminalBase & operator << <ElfProgramHeader_64>(TerminalBase & term, ElfProgramHeader_64 const value)
    {
        term << "[ELF Program Header, ELF64 | Flags " << value.Flags
            << "; Offset ";
        term.WriteHex64(value.Offset);

        term.Write("; VAddr ");
        term.WriteHex64(value.VAddr);
        term.Write("; PAddr ");
        term.WriteHex64(value.PAddr);
        term.Write("; VSize ");
        term.WriteHex64(value.VSize);
        term.Write("; PSize ");
        term.WriteHex64(value.PSize);
        term.Write("; Alignment ");
        term.WriteHex64(value.Alignment);

        return term << "; Type " << value.Type << "]";
    }

    template<>
    TerminalBase & operator << <ElfDynamicEntry>(TerminalBase & term, ElfDynamicEntry const value)
    {
        term.Write("[ELF Dynamic Entry | Value ");
        term.WriteHex64(value.Value);

        return term << "; Tag " << value.Tag << "]";
    }
}}
