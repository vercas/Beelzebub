#include <arc/cpu.hpp>

void Beelzebub::Cpu::Halt()
{
	asm volatile ("hlt");
}
