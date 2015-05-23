#include <arc/memory/virtual_allocator.hpp>
#include <arc/system/cpuid.hpp>
#include <arc/system/cpu.hpp>
#include <debug.hpp>
#include <string.h>
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;
using namespace Beelzebub::Debug;
using namespace Beelzebub::Memory;
using namespace Beelzebub::Memory::Paging;
using namespace Beelzebub::System;

/************************************
    VirtualAllocationSpace struct
************************************/

/*  Cached Feature Flags  */

bool VirtualAllocationSpace::Page1GB;
bool VirtualAllocationSpace::NX;

/*  Constructors    */

VirtualAllocationSpace::VirtualAllocationSpace(PageAllocator * const allocator)
    : Allocator( allocator )
    , FreePagesCount(0)
    , MappedPagesCount(0)
{

}

/*  Main Operations  */

Handle VirtualAllocationSpace::Bootstrap()
{
    //  The bootstrap function prepares the FIRST virtual allocation space
    //  for use. 'Tis necessary because of identity-mapping and the lack of
    //  a proper kernel space.

    VirtualAllocationSpace::Page1GB = BootstrapProcessorId.CheckFeature(CpuFeature::Page1GB);
    VirtualAllocationSpace::NX = false;// BootstrapProcessorId.CheckFeature(CpuFeature::NX);

    if (NX)
        Cpu::EnableNxBit();

    paddr_t pml4_paddr = this->Allocator->AllocatePage();

    Pml4 & pml4 = *((Pml4 *)pml4_paddr);
    //  Cheap.

    memset((void *)pml4_paddr, 0, 4096);
    //  Clear it all out!

    Cr3 cr3 = Cpu::GetCr3();
    Pml4 & currentPml4 = *cr3.GetPml4Ptr();

    //msg("CURRENT PML4 ADDR: %XP ", cr3.GetAddress());
    //msg("NEW ONE: %XP ", pml4_paddr);

    pml4[(uint16_t)511] = currentPml4[(uint16_t)511];

    for (uint16_t i = 256; i < VirtualAllocationSpace::FractalIndex; ++i)
    {
        paddr_t pml3_paddr = this->Allocator->AllocatePage();

        memset((void *)pml3_paddr, 0, 4096);
        //  Clear again.

        pml4[i] = Pml4Entry(pml3_paddr, true, true, true, false);
    }

    pml4[VirtualAllocationSpace::FractalIndex] = Pml4Entry(pml4_paddr, true, true, false, NX);

    this->Pml4Address = pml4_paddr;

    return Handle(HandleResult::Okay);
}

Handle VirtualAllocationSpace::Activate()
{
    Cr3 newVal = Cr3(this->Pml4Address, false, false);

    //msg("NEW CR3: %X8 ", newVal.Value);

    Cpu::SetCr3(newVal);

    return Handle(HandleResult::Okay);
}
