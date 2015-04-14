#include <arc/cpu.hpp>
#include <kernel.hpp>

using namespace Beelzebub;

void Beelzebub::Main()
{
    while (true) if (Cpu::CanHalt) Cpu::Halt();
}

void Beelzebub::Secondary()
{
    while (true) if (Cpu::CanHalt) Cpu::Halt();
}
