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

#include <string.h>

using namespace Beelzebub::Terminals;

static ElfValidationResult PerformRelocation64(Elf const * elf, uintptr_t offset, uintptr_t A, ElfRelType rtype, uint32_t symInd, uint32_t data)
{
    ptrdiff_t const B = elf->GetLocationDifference();

    union
    {
        uint64_t * u64;
        uint32_t * u32;
        uint16_t * u16;
        uint8_t  * u8;
        uintptr_t uptr;
    } const addr = {reinterpret_cast<uint64_t *>(B + offset)};

    Elf::Symbol sym = elf->GetSymbol(symInd);
    uintptr_t S = sym.Value;
    size_t Z = sym.Size;

    //  Now all the information for processing the relocation has been optained.

    size_t relocSize;

    switch (rtype)
    {
    case R_AMD64_COPY:
        relocSize = Z;  //  Speshul case.
        break;

    case R_AMD64_TLSDESC:
        relocSize = 16;
        break;

    case R_AMD64_64:
    case R_AMD64_GLOB_DAT:
    case R_AMD64_JUMP_SLOT:
    case R_AMD64_RELATIVE:
    case R_AMD64_PC64:
    case R_AMD64_GOTOFF64:
    case R_AMD64_GOT64:
    case R_AMD64_GOTPCREL64:
    case R_AMD64_GOTPC64:
    case R_AMD64_GOTPLT64:
    case R_AMD64_PLTOFF64:
    case R_AMD64_SIZE64:
    case R_AMD64_IRELATIVE:
        relocSize = 8;
        break;

    case R_AMD64_PC32:
    case R_AMD64_GOT32:
    case R_AMD64_PLT32:
    case R_AMD64_GOTPCREL:
    case R_AMD64_32:
    case R_AMD64_32S:
    case R_AMD64_GOTPC32:
    case R_AMD64_SIZE32:
    case R_AMD64_GOTPC32_TLSDESC:
        relocSize = 4;
        break;

    case R_AMD64_16:
    case R_AMD64_PC16:
        relocSize = 2;
        break;

    case R_AMD64_8:
    case R_AMD64_PC8:
        relocSize = 1;
        break;

    case R_AMD64_NONE:
    case R_AMD64_TLSDESC_CALL:
        relocSize = 0;
        break;

    default:
        return ElfValidationResult::UnknownRelocationType;
    }

    //  Now the size of the relocation is known, and invalid/unknown relocations
    //  are reported. Next step is checking whether it is actually possible to
    //  perform the relocation.

    if (rtype != R_AMD64_NONE)
        switch (CheckRangeLoaded(elf, offset, relocSize, RangeLoadOptions::Writable))
        {
        case RangeLoadStatus::CompletelyAbsent:
            return ElfValidationResult::RelocationOutOfLoad;

        case RangeLoadStatus::PartiallyLoaded:
            return ElfValidationResult::RelocationInPartialLoad;

        case RangeLoadStatus::OptionsNotMet:
            return ElfValidationResult::RelocationUnchangeable;

        default: break;
        }

    //  So now it is certain that the relocation can proceed. But before the
    //  actual relocation, the symbol must be looked up in *all* the loaded
    //  executable images...

    //  TODO: Global symbol resolution!

    //  Final step is performing the actual relocation.

    switch (rtype)
    {
    case R_AMD64_NONE: break;

    case R_AMD64_64:        *(addr.u64) = S + A;    break;

    case R_AMD64_COPY: memcpy(addr.u8, reinterpret_cast<void *>(S), Z); break;

    case R_AMD64_GLOB_DAT:
    case R_AMD64_JUMP_SLOT: *(addr.u64) = S;        break;

    case R_AMD64_RELATIVE:  *(addr.u64) = B + A;    break;

    default: return ElfValidationResult::UnsupportedRelocationType;
    }

    return ElfValidationResult::Success;
}
