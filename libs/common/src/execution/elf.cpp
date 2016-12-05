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

#include <string.h>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

/*  Constants  */

uint32_t Execution::ElfMagicNumber = 0x464C457F;

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
ENUM_TO_STRING_EX2(ElfSymbolBinding, ENUM_ELFSYMBOLBINDING, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfSymbolType, ENUM_ELFSYMBOLTYPE, Beelzebub::Execution)
ENUM_TO_STRING_EX2(ElfSymbolVisibility, ENUM_ELFSYMBOLVISIBILITY, Beelzebub::Execution)

ENUM_TO_STRING_EX2(ElfValidationResult, ENUM_ELFVALIDATIONRESULT, Beelzebub::Execution)

#include <beel/terminals/base.hpp>

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

    SPAWN_ENUM(ElfSymbolBinding)
    SPAWN_ENUM(ElfSymbolType)
    SPAWN_ENUM(ElfSymbolVisibility)

    SPAWN_ENUM(ElfValidationResult)

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
        term << "[ELF32 Program Header | Flags " << value.Flags
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
        term << "[ELF64 Program Header | Flags " << value.Flags
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
    TerminalBase & operator << <ElfDynamicEntry_32>(TerminalBase & term, ElfDynamicEntry_32 const value)
    {
        term.Write("[ELF32 Dynamic Entry | Value ");
        term.WriteHex32(value.Value);

        return term << "; Tag " << value.Tag << "]";
    }

    template<>
    TerminalBase & operator << <ElfDynamicEntry_64>(TerminalBase & term, ElfDynamicEntry_64 const value)
    {
        term.Write("[ELF64 Dynamic Entry | Value ");
        term.WriteHex64(value.Value);

        return term << "; Tag " << value.Tag << "]";
    }

    template<>
    TerminalBase & operator << <ElfRelEntryInfo_32>(TerminalBase & term, ElfRelEntryInfo_32 const value)
    {
        term.Write("[ELF32 Rela Info | Symbol ");
        term.WriteHex24(value.GetSymbol());

        return term << "; Type " << (uint8_t)(value.GetType()) << "]";
    }

    template<>
    TerminalBase & operator << <ElfRelEntryInfo_64>(TerminalBase & term, ElfRelEntryInfo_64 const value)
    {
        term.Write("[ELF64 Rela Info | Symbol ");
        term.WriteHex32(value.GetSymbol());
        term.Write("; Data ");
        term.WriteHex24(value.GetData());

        return term << "; Type " << (uint8_t)(value.GetType()) << "]";
    }

    template<>
    TerminalBase & operator << <ElfRelaEntry_32>(TerminalBase & term, ElfRelaEntry_32 const value)
    {
        term.Write("[ELF32 Rela Entry | Offset ");
        term.WriteHex32(value.Offset);
        term.Write("; Append ");
        term.WriteHex32(value.Append);

        return term << "; Info " << value.Info << "]";
    }

    template<>
    TerminalBase & operator << <ElfRelaEntry_64>(TerminalBase & term, ElfRelaEntry_64 const value)
    {
        term.Write("[ELF64 Rela Entry | Offset ");
        term.WriteHex64(value.Offset);
        term.Write("; Append ");
        term.WriteHex64(value.Append);

        return term << "; " << value.Info << "]";
    }

    template<>
    TerminalBase & operator << <ElfSymbol_32>(TerminalBase & term, ElfSymbol_32 const value)
    {
        term.Write("[ELF32 Symbol | Value ");
        term.WriteHex32(value.Value);
        term.Write("; Size ");
        term.WriteUIntD(value.Size);
        term.Write("; S Index ");
        term.WriteHex16(value.SectionIndex);

        return term << "; Type " << value.Info.GetType()
                    << "; Binding " << value.Info.GetBinding()
                    << "; Visibility " << value.Other.GetVisibility()
                    << "]";
    }

    template<>
    TerminalBase & operator << <ElfSymbol_64>(TerminalBase & term, ElfSymbol_64 const value)
    {
        term.Write("[ELF64 Symbol | Value ");
        term.WriteHex64(value.Value);
        term.Write("; Size ");
        term.WriteUIntD(value.Size);
        term.Write("; S Index ");
        term.WriteHex16(value.SectionIndex);

        return term << "; Type " << value.Info.GetType()
                    << "; Binding " << value.Info.GetBinding()
                    << "; Visibility " << value.Other.GetVisibility()
                    << "]";
    }

    template<>
    TerminalBase & operator << <Elf::Symbol>(TerminalBase & term, Elf::Symbol const value)
    {
        if (value.Exists)
        {
            term.Write("[ELF Symbol | Value ");
            term.WriteHex64(value.Value);
            term.Write("; Size ");
            term.WriteUIntD(value.Size);

            return term << "; Name " << (value.Name == nullptr ? "% NULL %" : value.Name)
                        << "; Type " << value.Type
                        << "; Binding " << value.Binding
                        << "; Visibility " << value.Visibility
                        << "]";
        }

        return term << "[ELF Symbol | NON-EXISTENT ]";
    }

    template<>
    TerminalBase & operator << <Elf>(TerminalBase & term, Elf const value)
    {
        if (value.IsElf32())
            return term << *(value.H1) << EndLine
                        << *(value.GetH2_32()) << EndLine
                        << *(value.GetH3()) << EndLine;
        else
            return term << *(value.H1) << EndLine
                        << *(value.GetH2_64()) << EndLine
                        << *(value.GetH3()) << EndLine;
    }
}}

/****************
    ELF class
****************/

/*  Statics  */

uint32_t Elf::Hash(char const * name)
{
    uint32_t h = 0, g;
 
    while (*name)
    {
        h = (h << 4) + *(reinterpret_cast<uint8_t const *>(name++));

        if ((g = h & 0xF0000000))
            h ^= g >> 24;

        h &= ~g;
    }

    return h;
}

/*  Constructors  */

Elf::Elf(void const * addr, size_t size)
    : Size( size), H1(reinterpret_cast<ElfHeader1 const *>(addr))
    , BaseAddress(~((uintptr_t)0)), EndAddress(0), NewLocation(0)
    , Symbolic(false), TextRelocation(false), Loadable(false)
    , DT_32(nullptr), DT_Size(0), DT_Count(0)
    , REL_32(nullptr), REL_Size(0), RELA_32(nullptr), RELA_Size(0)
    , PLT_REL_32(nullptr), PLT_REL_Size(0), PLT_REL_Type(DT_NULL), PLT_GOT(0)
    , DYNSYM_32(nullptr), DYNSYM_Count(0)
    , STRTAB(nullptr), STRTAB_Size(0)
    , HASH(nullptr), INIT(nullptr), FINI(nullptr)
{

}

/*  Methods  */

ElfValidationResult Elf::ValidateAndParse(Elf::HeaderValidatorFunc headerval, Elf::SegmentValidatorFunc segval, void * valdata)
{
    if unlikely(headerval != nullptr && !headerval(this->H1, valdata))
        return ElfValidationResult::HeaderRejected;

    if (this->H1->Identification.MagicNumber != ElfMagicNumber)
        return ElfValidationResult::WrongMagicNumber;

    if (this->H1->Identification.DataEncoding != ElfDataEncoding::LittleEndian)
        return ElfValidationResult::WrongEndianness;

    switch (this->H1->Identification.Class)
    {
    case ElfClass::Elf32:
        return this->ValidateParseElf32(segval, valdata);
    case ElfClass::Elf64:
        return this->ValidateParseElf64(segval, valdata);
    default:
        return ElfValidationResult::InvalidClass;
    }
}

ElfValidationResult Elf::Relocate(uintptr_t newAddress)
{
    if (!this->Loadable)
        return ElfValidationResult::Unloadable;

    if (this->H1->Type != ElfFileType::Dynamic)
        return ElfValidationResult::Unrelocatable;
    //  Only DYN-type ELF files can be relocated. Even PIE are of type DYN.

    ptrdiff_t const diff = newAddress - this->BaseAddress;

#define RELOCATE(var) \
    if (this->var != (decltype(this->var))(nullptr)) \
        this->var = reinterpret_cast<decltype(this->var)>(reinterpret_cast<uintptr_t>(this->var) + diff);
    //  This macro should be m'kay.

    RELOCATE(DT_32);
    RELOCATE(REL_32);
    RELOCATE(RELA_32);
    RELOCATE(PLT_REL_32);
    RELOCATE(PLT_GOT);
    RELOCATE(DYNSYM_32);
    RELOCATE(STRTAB);
    RELOCATE(HASH);
    RELOCATE(INIT);
    RELOCATE(FINI);

    this->NewLocation = newAddress;

    return ElfValidationResult::Success;
}

Elf::Symbol Elf::GetSymbol(uint32_t index) const
{
    if unlikely(!this->Loadable)
        return {0};

    switch (this->H1->Identification.Class)
    {
    case ElfClass::Elf32:
        return this->GetSymbol32(index);
    case ElfClass::Elf64:
        return this->GetSymbol64(index);
    default:
        return {0};
    }
}

Elf::Symbol Elf::GetSymbol(char const * name) const
{
    if unlikely(!this->Loadable)
        return {0};

    if unlikely(this->NewLocation == 0 && this->H1->Type == ElfFileType::Dynamic)
        return {0};

    if unlikely(this->HASH == nullptr || this->DYNSYM_64 == nullptr || this->STRTAB == nullptr)
        return {0};

    //  Step one is obtaining the hash.

    uint32_t hash = Hash(name) % this->HASH->BucketCount;

    //  Step two is walking the hash chain...

    unsigned id = this->HASH->GetBucket(hash);

    do
    {
        Symbol res = this->GetSymbol64(id);

        if (strcmp(res.Name, name) == 0)
            return res;
        //  Found, apparently.
    } while((id = this->HASH->GetChain(id)) != 0 && id != this->HASH->GetBucket(hash));

    return {0};
    //  Not found. :c
}
