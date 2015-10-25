#pragma once

#include <memory/paging.hpp>
#include <_print/print_template.hpp>

PRINT_FUNCS(Beelzebub::Memory::Pml4 const &)
PRINT_FUNCS(Beelzebub::Memory::Pml3 const &)
PRINT_FUNCS(Beelzebub::Memory::Pml2 const &)
PRINT_FUNCS(Beelzebub::Memory::Pml1 const &)

PRINT_FUNCS_CONST(Beelzebub::Memory::Pml4Entry)
PRINT_FUNCS_CONST(Beelzebub::Memory::Pml3Entry)
PRINT_FUNCS_CONST(Beelzebub::Memory::Pml2Entry)
PRINT_FUNCS_CONST(Beelzebub::Memory::Pml1Entry)
