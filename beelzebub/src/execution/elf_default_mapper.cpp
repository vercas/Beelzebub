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

#include <execution/elf_default_mapper.hpp>
#include <execution/elf.hpp>
#include <memory/vmm.hpp>

#include <kernel.hpp>
#include <entry.h>
#include <system/cpu.hpp>

#include <string.h>
#include <math.h>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::Memory;
using namespace Beelzebub::System;

bool Execution::MapSegment64(uintptr_t loc, uintptr_t img, ElfProgramHeader_64 const & phdr, void * data)
{
    vaddr_t const segVaddr    = loc + RoundDown(phdr.VAddr, PageSize);
    vaddr_t const segVaddrEnd = loc + RoundUp  (phdr.VAddr + phdr.VSize, PageSize);

    if (segVaddrEnd <= segVaddr || segVaddr < Vmm::UserlandStart || segVaddrEnd > Vmm::KernelEnd)
        return false;
    //  So it starts before the userland or ends after the kernel... Not good.

    if (segVaddrEnd > Vmm::UserlandEnd)
    {
        if (segVaddr < Vmm::KernelStart)
            return false;
        //  It seems to start before the kernel, but ends after userland...
        //  bad.

        //  So it starts after the kernel, ends before the kernel end.

        if (data != nullptr)
        {
            auto dmo = reinterpret_cast<DefaultMappingOptions *>(data);

            if (!dmo->AllowInKernel)
                return false;
            //  Explicitly disallowed in the kernel.
        }
        else
            return false;
        //  Also implicitly disallowed for security.
    }

    vaddr_t vaddr = segVaddr;
    //  Will be used for rollback.

    MemoryFlags pageFlags = MemoryFlags::Userland;

    if (0 != (phdr.Flags & ElfProgramHeaderFlags::Executable))
        pageFlags |= MemoryFlags::Executable;

    if (0 != (phdr.Flags & ElfProgramHeaderFlags::Writable))
        pageFlags |= MemoryFlags::Writable;

    if (0 != (pageFlags & MemoryFlags::Writable) || phdr.VSize != phdr.PSize)
    {
        pageFlags |= MemoryFlags::Writable;

        for (/* nothing */; vaddr < segVaddrEnd; vaddr += PageSize)
        {
            PageDescriptor * desc = nullptr;
            paddr_t const paddr = Cpu::GetData()->DomainDescriptor->PhysicalAllocator->AllocatePage(desc);

            assert_or(paddr != nullpaddr && desc != nullptr
                , "Unable to allocate a physical page for mapping writable ELF segment %Xp!"
                , &phdr)
            {
                goto rollbackMapping;
            }

            Handle res = Vmm::MapPage(Cpu::GetProcess(), vaddr, paddr, pageFlags, desc);

            assert_or(res.IsOkayResult()
                , "Failed to map page at %Xp (%XP) for mapping writable ELF segment %Xp: %H."
                , vaddr, paddr, &phdr, res)
            {
                Cpu::GetData()->DomainDescriptor->PhysicalAllocator->FreePageAtAddress(paddr);
                //  First free the page that was allocated for this vaddr. :(

                goto rollbackMapping;
            }
        }

        memcpy(reinterpret_cast<void *>(loc + phdr.VAddr )
            ,  reinterpret_cast<void *>(img + phdr.Offset), phdr.PSize);

        if (phdr.VSize > phdr.PSize)
            memset(reinterpret_cast<void *>(loc + phdr.VAddr + phdr.PSize)
                , 0, phdr.VSize - phdr.PSize);
    }
    else
    {
        vaddr_t imgVaddr = img + RoundDown(phdr.Offset, PageSize);

        for (/* nothing */; vaddr < segVaddrEnd; vaddr += PageSize, imgVaddr += PageSize)
        {
            paddr_t paddr;

            Handle res = Vmm::Translate(Cpu::GetProcess(), imgVaddr, paddr);

            assert_or(res.IsOkayResult() && paddr != nullpaddr
                , "Failed to retrieve physical address at %Xp for mapping non-writable ELF segment %Xp: %H."
                , vaddr, &phdr, res)
            {
                goto rollbackMapping;
            }

            res = Vmm::MapPage(Cpu::GetProcess(), vaddr, paddr, pageFlags);

            assert_or(res.IsOkayResult()
                , "Failed to map page at %Xp (%XP) for mapping non-writable ELF segment %Xp: %H."
                , vaddr, paddr, &phdr, res)
            {
                goto rollbackMapping;
            }
        }
    }

    return true;

rollbackMapping:

    //  It starts with a decrement because vaddr points to a page that failed
    //  to map.

    do
    {
        vaddr -= PageSize;

        Handle res = Vmm::UnmapPage(Cpu::GetProcess(), vaddr);

        ASSERT(res.IsOkayResult()
            , "Failed to unmap page at %Xp for unrolling ELF segment %Xp: %H."
            , vaddr, &phdr, res);
    } while (vaddr > segVaddr);

    return false;
}

bool Execution::UnmapSegment64(uintptr_t loc, ElfProgramHeader_64 const & phdr, void * data)
{
    vaddr_t const segVaddr    = loc + RoundDown(phdr.VAddr, PageSize);
    vaddr_t const segVaddrEnd = loc + RoundUp  (phdr.VAddr + phdr.VSize, PageSize);

    if (segVaddrEnd <= segVaddr || segVaddr < Vmm::UserlandStart || segVaddrEnd > Vmm::KernelEnd)
        return false;
    //  So it starts before the userland or ends after the kernel... Not good.

    if (segVaddrEnd > Vmm::UserlandEnd)
    {
        if (segVaddr < Vmm::KernelStart)
            return false;
        //  It seems to start before the kernel, but ends after userland...
        //  bad.

        //  So it starts after the kernel, ends before the kernel end.

        if (data != nullptr)
        {
            auto dmo = reinterpret_cast<DefaultMappingOptions *>(data);

            if (!dmo->AllowInKernel)
                return false;
            //  Explicitly disallowed in the kernel.
        }
        else
            return false;
        //  Also implicitly disallowed for security.
    }

    vaddr_t vaddr = segVaddrEnd;

    //  It starts with a decrement because vaddr points to a page that is outside
    //  of the actual segment.

    do
    {
        vaddr -= PageSize;

        Handle res = Vmm::UnmapPage(Cpu::GetProcess(), vaddr);

        ASSERT(res.IsOkayResult()
            , "Failed to unmap page at %Xp for unrolling ELF segment %Xp: %H."
            , vaddr, &phdr, res);
    } while (vaddr > segVaddr);

    return true;
}
