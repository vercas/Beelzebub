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

#include <execution/elf.helpers.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;

static inline RangeLoadStatus CheckRangeLoaded(Elf const * elf, uint64_t rStart, uint64_t rSize, RangeLoadOptions opts = RangeLoadOptions::None)
{
    return elf->CheckRangeLoaded64(rStart, rSize, opts);
}

#include <execution/elf64.arc.inc>

/****************
    ELF class
****************/

/*  Methods  */

ElfValidationResult Elf::ValidateParseDt64(ElfDynamicEntry_64 const * dts)
{
    ASSUME(this->IsElf64());

    ElfDynamicEntry_64 const * dtCursor = dts;
    size_t offset = 0, dtCount = 0;

    size_t tagCounters[(size_t)(ElfDynamicEntryTag::DT__MAX)] = {0};

    do
    {
        if likely((size_t)(dtCursor->Tag) < (size_t)(ElfDynamicEntryTag::DT__MAX))
            ++tagCounters[(size_t)(dtCursor->Tag)];
        //  Count all the tags that are relevant. Also, the conversion is there
        //  because the tag's underlying type is signed. Brilliant, IKR...

        switch (dtCursor->Tag)
        {
            //  RELA relocations

            DT_CASE_P(DT_RELA, this->RELA_64, ElfRelaEntry_64 const *)
            DT_CASE_C(DT_RELASZ, this->RELA_Size, size_t)

            case DT_RELAENT:
                if unlikely(dtCursor->Value != sizeof(ElfRelaEntry_64))
                    return ElfValidationResult::StructureSizeMismatch;
                break;

            //  REL relocations
            
            DT_CASE_P(DT_REL, this->REL_64, ElfRelEntry_64 const *)
            DT_CASE_C(DT_RELSZ, this->REL_Size, size_t)

            case DT_RELENT:
                if unlikely(dtCursor->Value != sizeof(ElfRelEntry_64))
                    return ElfValidationResult::StructureSizeMismatch;
                break;

            //  PLT relocations

            DT_CASE_P(DT_JMPREL, this->PLT_RELA_64, ElfRelaEntry_64 const *)
            DT_CASE_C(DT_PLTRELSZ, this->PLT_REL_Size, size_t)

            case DT_PLTREL:
                this->PLT_REL_Type = (ElfDynamicEntryTag)(dtCursor->Value);

                switch ((ElfDynamicEntryTag)(dtCursor->Value))
                {
                case DT_REL:
                case DT_RELA:
                    break;

                default:
                    return ElfValidationResult::DtInvalidPltRelocationType;
                }

                break;

            DT_CASE_P(DT_PLTGOT, this->PLT_GOT, uintptr_t)

            //  DYNSYM

            DT_CASE_P(DT_SYMTAB, this->DYNSYM_64, ElfSymbol_64 *)

            case DT_SYMENT:
                if unlikely(dtCursor->Value != sizeof(ElfSymbol_64))
                    return ElfValidationResult::StructureSizeMismatch;
                break;

            //  STRTAB

            DT_CASE_P(DT_STRTAB, this->STRTAB, char const *)
            DT_CASE_C(DT_STRSZ, this->STRTAB_Size, size_t)

            //  HASH

            DT_CASE_P(DT_HASH, this->HASH, ElfHashTable const *)

            //  Initializer and finalizer

            DT_CASE_P(DT_INIT, this->INIT, ActionFunction0)
            DT_CASE_P(DT_FINI, this->FINI, ActionFunction0)

            default: break;
        }

        ++dtCursor;
        ++dtCount;
        offset += sizeof(ElfDynamicEntry_64);

        if unlikely(offset > this->DT_Size - sizeof(ElfDynamicEntry_64))
            return ElfValidationResult::DtEntryOutOfBounds;
        //  This is done after the increment because the null entry must be
        //  valid and within the segment as well.
    } while (dtCursor->Tag != DT_NULL);

    this->DT_Count = dtCount;

    //  Okay, so a few entries have been gathered. Now, a census must be performed.

    DT_UNIQUE(DT_SYMTAB, DT_SYMENT, DT_HASH, DT_STRTAB, DT_STRSZ);
    //  Yes, these 5 are mandatory for all ELF files with a dynamic segment.

    DT_IMPLY(DT_REL, DT_RELSZ, DT_RELENT) return ElfValidationResult::DtEntryMultiplicate;
    DT_IMPLY(DT_RELA, DT_RELASZ, DT_RELAENT) return ElfValidationResult::DtEntryMultiplicate;

    DT_IMPLY(DT_JMPREL, DT_PLTRELSZ, DT_PLTREL) return ElfValidationResult::DtEntryMultiplicate;
    //  DT_PLTGOT is not mandatory.

    if unlikely(DT_CNT(DT_INIT) > 1) return ElfValidationResult::DtEntryMultiplicate;
    if unlikely(DT_CNT(DT_FINI) > 1) return ElfValidationResult::DtEntryMultiplicate;

    //  Now to check the boundaries.

    CHECK_LOADED(reinterpret_cast<uintptr_t>(this->STRTAB), this->STRTAB_Size);
    //  First byte should be 0. Checked after loading.

    CHECK_LOADED(reinterpret_cast<uintptr_t>(this->HASH), sizeof(ElfHashTable));
    //  The full hashtable isn't checked here. It will be checked after it's loaded.

    CHECK_LOADED(reinterpret_cast<uintptr_t>(this->DYNSYM_64), sizeof(ElfSymbol_64));
    //  The hashtable needs to be accessed to get the actual length of the dynamic
    //  symbol table. But it needs at least the undefined symbol, and it needs to
    //  be within a loadable segment, so this minimal check is still performed here.

    if (DT_CNT(DT_REL) == 1)
        CHECK_LOADED(reinterpret_cast<uintptr_t>(this->REL_64), this->REL_Size);

    if (DT_CNT(DT_RELA) == 1)
        CHECK_LOADED(reinterpret_cast<uintptr_t>(this->RELA_64), this->RELA_Size);

    if (DT_CNT(DT_JMPREL) == 1)
        CHECK_LOADED(reinterpret_cast<uintptr_t>(this->PLT_REL_64), this->PLT_REL_Size);

    if (DT_CNT(DT_INIT) == 1)
        CHECK_LOADED(reinterpret_cast<uintptr_t>(this->INIT), 1, Executable);
    //  Needs at least 1 byte into a executable segment.

    if (DT_CNT(DT_FINI) == 1) //  Ditto.
        CHECK_LOADED(reinterpret_cast<uintptr_t>(this->FINI), 1, Executable);

    //  If not for the macros, that would've been about two hundred lines of code.

    return ElfValidationResult::Success;
}

ElfValidationResult Elf::ValidateParseElf64(Elf::SegmentValidatorFunc segval, void * valdata)
{
    ASSUME(this->IsElf64());

    if unlikely(this->GetH3()->ProgramHeaderTableEntrySize != sizeof(ElfProgramHeader_64))
        return ElfValidationResult::StructureSizeMismatch;

    auto phdrs_offset = this->GetH2_64()->ProgramHeaderTableOffset;
    uint32_t phdr_count = this->GetH3()->ProgramHeaderTableEntryCount;

    if unlikely(phdrs_offset > this->Size - sizeof(ElfProgramHeader_64))
        return ElfValidationResult::SegmentHeadersOutOfBounds;
    //  This counts on the unsigned-ness of size_t. Also makes sure there's room
    //  for at least one header.

    if unlikely(phdrs_offset + phdr_count * sizeof(ElfProgramHeader_64) > this->Size)
        return ElfValidationResult::SegmentHeadersOutOfBounds;
    //  This check is done after, separately, to account for an offset that would
    //  overflow, but wouldn't appear so together with the count.
    //  Since the segment count is 16-bit, this cannot overflow enough to be yet
    //  within the boundaries of the image.

    auto phdrs = this->GetPhdrs_64();
    size_t loadableCount = 0;

    ElfDynamicEntry_64 const * dts = nullptr;

    for (size_t i = 0; i < phdr_count; ++i)
    {
        auto & phdr = phdrs[i];

        if unlikely(phdr.Offset >= this->Size)
            return ElfValidationResult::SegmentOutOfBounds;

        if unlikely(phdr.Offset + phdr.PSize > this->Size)
            return ElfValidationResult::SegmentOutOfBounds;
        //  These two account for double overflow as well.

        if unlikely((phdr.VAddr % PageSize) != (phdr.Offset % PageSize))
            return ElfValidationResult::SegmentNonCongruent;
        //  This is required by the ABI.

        if (phdr.Type == ElfProgramHeaderType::Load)
        {
            //  Load segments' addresses need to be checked!

            if unlikely(segval != nullptr && !segval((uintptr_t)(phdr.VAddr), (size_t)(phdr.VSize), phdr.Flags, valdata))
                return ElfValidationResult::SegmentRangeInvalid;

            //  And now an overlap check, with the previous segments, which passed
            //  all the other checks.

            for (size_t j = 0; j < i; ++j)
            {
                if (phdrs[j].Type != ElfProgramHeaderType::Load)
                    continue;

                auto intersection = IntersectMemoryRanges({phdr.VAddr, phdr.VSize}, {phdrs[j].VAddr, phdrs[j].VSize});

                if unlikely(intersection.Size != 0)
                    return ElfValidationResult::SegmentsOverlap;
            }

            //  Now see if this segment's virtual address is the (new) base, or end.

            this->BaseAddress = Minimum(phdr.VAddr, this->BaseAddress);
            this->EndAddress = Maximum(phdr.VAddr + phdr.VSize, this->EndAddress);

            ++loadableCount;
        }
        else if (phdr.Type == ElfProgramHeaderType::Dynamic)
        {
            if unlikely(this->DT_64 != nullptr)
                return ElfValidationResult::DynamicSegmentMultiplicate;

            dts = reinterpret_cast<ElfDynamicEntry_64 const *>(this->Start + phdr.Offset);

            this->DT_64 = reinterpret_cast<ElfDynamicEntry_64 const *>(phdr.VAddr);
            this->DT_Size = (size_t)(phdr.VSize);
        }
        else if (phdr.Type == ElfProgramHeaderType::Tls)
        {
            if unlikely(this->TLS_64 != nullptr)
                return ElfValidationResult::DynamicSegmentMultiplicate;

            this->TLS_64 = &phdr;
        }
    }

    //  Now, all the non-load segments need to be checked. They ought to fit within
    //  a load segment. Exception makes the TLS segment.

    for (size_t i = 0; i < phdr_count; ++i)
    {
        auto nonload = phdrs[i];

        if (nonload.Type == ElfProgramHeaderType::Load || nonload.Type == ElfProgramHeaderType::Tls)
            continue;

        //  Note: Since a section can only be within one segment, there's no
        //  need to merge adjacent segments or something like that to catch
        //  non-load segments spanning more than one load segment.
        //  If that happens, though, send a bag of coal to the developers of
        //  your linker.

        switch (this->CheckRangeLoaded64(nonload.VAddr, nonload.VSize))
        {
        case RangeLoadStatus::CompletelyAbsent:
            return ElfValidationResult::SegmentOutOfLoad;

        case RangeLoadStatus::PartiallyLoaded:
            return ElfValidationResult::SegmentInPartialLoad;

        default: break;
        }
    }

    //  Headers appear to be fine so far. Next step is checking the dynamic tags,
    //  if any.

    if (dts != nullptr)
    {
        ElfValidationResult res = this->ValidateParseDt64(dts);

        if (res != ElfValidationResult::Success)
            return res;
    }

    if likely((this->Loadable = (loadableCount > 0)))
        return ElfValidationResult::Success;
    else
        return ElfValidationResult::Unloadable;
}

ElfValidationResult Elf::LoadAndValidate64(Elf::SegmentMapper64Func segmap, Elf::SegmentUnmapper64Func segunmap, Elf::SymbolResolverFunc symres, void * lddata) const
{
    if unlikely(!this->Loadable)
        return ElfValidationResult::Unloadable;

    ASSUME(this->IsElf64());

    ElfValidationResult res = ElfValidationResult::LoadFailure;

    //  Step 1 is trying to map all the segments.

    auto phdr_count = this->GetH3()->ProgramHeaderTableEntryCount;
    auto phdrs = this->GetPhdrs_64();

    size_t i = 0;

    for (/* nothing */; i < phdr_count; ++i)
    {
        auto phdr = phdrs[i];

        if (phdr.Type != ElfProgramHeaderType::Load)
            continue;

        if unlikely(!segmap(this->GetLocationDifference(), this->Start, phdr, lddata))
        {
            //  Okay, mapping this load segment failed. Time to roll everything
            //  back. :(
            //  Also, the mapping function should take care of partially-mapped
            //  segments.

            goto rollbackMapping;
        }
    }

    //  Reaching this point means all load segments were mapped successfully.

    goto skipRollback;

rollbackMapping:

    //  Here all the mapped segments need to be unmapped. The unmapping
    //  function will need to be tolerant.

    while (i-- > 0)
    {
        auto phdr = phdrs[i];

        if (phdr.Type != ElfProgramHeaderType::Load)
            continue;

        segunmap(this->GetLocationDifference(), phdr, lddata);
        //  This could fail, but there's nothing to do if it does...
    }

    return res;

skipRollback:
    
    //  So, next step is performing relocations, if any.

    if (this->REL_64 != nullptr)
        for (size_t i = 0, offset = this->REL_Size; offset > 0; ++i, offset -= sizeof(ElfRelEntry_64))
        {
            ElfRelEntry_64 const & rel = this->REL_64[i];

            res = PerformRelocation64(this, rel.Offset, 0, rel.Info.GetType(), rel.Info.GetSymbol(), rel.Info.GetData(), symres, lddata);

            if (res != ElfValidationResult::Success)
                goto rollbackMapping;
        }

    if (this->RELA_64 != nullptr)
        for (size_t i = 0, offset = this->RELA_Size; offset > 0; ++i, offset -= sizeof(ElfRelaEntry_64))
        {
            ElfRelaEntry_64 const & rel = this->RELA_64[i];

            res = PerformRelocation64(this, rel.Offset, rel.Append, rel.Info.GetType(), rel.Info.GetSymbol(), rel.Info.GetData(), symres, lddata);

            if (res != ElfValidationResult::Success)
                goto rollbackMapping;
        }

    if (this->PLT_REL_64 != nullptr)
    {
        if (this->PLT_REL_Type == DT_REL)
            for (size_t i = 0, offset = this->PLT_REL_Size; offset > 0; ++i, offset -= sizeof(ElfRelEntry_64))
            {
                ElfRelEntry_64 const & rel = this->PLT_REL_64[i];

                res = PerformRelocation64(this, rel.Offset, 0, rel.Info.GetType(), rel.Info.GetSymbol(), rel.Info.GetData(), symres, lddata);

                if (res != ElfValidationResult::Success)
                    goto rollbackMapping;
            }
        else
            for (size_t i = 0, offset = this->PLT_REL_Size; offset > 0; ++i, offset -= sizeof(ElfRelaEntry_64))
            {
                ElfRelaEntry_64 const & rel = this->PLT_RELA_64[i];

                res = PerformRelocation64(this, rel.Offset, rel.Append, rel.Info.GetType(), rel.Info.GetSymbol(), rel.Info.GetData(), symres, lddata);

                if (res != ElfValidationResult::Success)
                    goto rollbackMapping;
            }
    }

    return ElfValidationResult::Success;
}

Elf::Symbol Elf::GetSymbol64(uint32_t index) const
{
    ASSUME(this->IsElf64());

    if unlikely(!this->Loadable)
        return {};

    if unlikely(this->NewLocation == 0 && this->H1->Type == ElfFileType::Dynamic)
        return {};

    if unlikely(this->HASH == nullptr || this->DYNSYM_64 == nullptr || this->STRTAB == nullptr)
        return {};

    if unlikely(index >= this->HASH->ChainCount)
        return {};

    ElfSymbol_64 const & sym = this->DYNSYM_64[index];

    Elf::Symbol ret = {
        this->STRTAB + sym.Name,
        (uintptr_t)(sym.Value),
        (size_t)(sym.Size),
        sym.Info.GetType(),
        sym.Info.GetBinding(),
        sym.Other.GetVisibility(),
        true,
        sym.SectionIndex != SHN_UNDEF,
    };

    if (ret.Defined && sym.SectionIndex != SHN_ABS)
    {
        // if (ret.Binding != ElfSymbolBinding::Weak)
            ret.Value += this->GetLocationDifference();
    }

    return ret;
}

RangeLoadStatus Elf::CheckRangeLoaded64(uint64_t rStart, uint64_t rSize, RangeLoadOptions opts) const
{
    ASSUME(this->IsElf64());

    auto phdr_count = this->GetH3()->ProgramHeaderTableEntryCount;
    auto phdrs = this->GetPhdrs_64();

    for (size_t i = 0; i < phdr_count; ++i)
    {
        auto load = phdrs[i];

        if (load.Type != ElfProgramHeaderType::Load)
            continue;

        //  Note: It is assumed that the given range can only be within one
        //  single load segment.

        auto intersection = IntersectMemoryRanges({(uintptr_t)rStart, (size_t)rSize}, {load.VAddr, load.VSize});

        if likely(intersection.Size == 0)
            continue;
        //  Not this segment.

        if likely(intersection.Start == rStart && intersection.Size == rSize)
        {
            switch (opts)
            {
            case RangeLoadOptions::Writable:
                if likely((load.Flags & ElfProgramHeaderFlags::Writable) != 0)
                    return RangeLoadStatus::FullyLoaded;
                else
                    return RangeLoadStatus::OptionsNotMet;
            case RangeLoadOptions::Executable:
                if likely((load.Flags & ElfProgramHeaderFlags::Executable) != 0)
                    return RangeLoadStatus::FullyLoaded;
                else
                    return RangeLoadStatus::OptionsNotMet;
            default:
                //  No speshul options.
                return RangeLoadStatus::FullyLoaded;
            }
        }
        //  This is it!

        //  If this point is reached, the given range fits into this load segment
        //  only partially. As load segments don't overlap in virtual memory,
        //  and the given range cannot sit in more than one segment, this isn't
        //  tolerated.

        return RangeLoadStatus::PartiallyLoaded;
    }

    return RangeLoadStatus::CompletelyAbsent;
}
