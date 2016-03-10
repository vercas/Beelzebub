#include <syscalls.h>

using namespace Beelzebub;

__extern __used int _start()
{
beginning:

    uintptr_t volatile someAddress = 0x30000;
    uintptr_t volatile someEnd = someAddress + someAddress;
    //  Yay.

    *reinterpret_cast<uint64_t *>(someAddress) = 0;

    for (size_t i = 0; i < 1000; ++i)
    {
        ++(*reinterpret_cast<uint64_t *>(someAddress));

        for (uint8_t * ptr = reinterpret_cast<uint8_t *>(someAddress + 8); (uintptr_t)ptr < someEnd; ++ptr)
            ++(*ptr);

        PerformSyscall(SyscallSelection::DebugPrint, const_cast<char *>("SYSCALL TEST LEL"), 0, 0, 0, 0);
    }

    double volatile d1 = -1, d2 = 0, d3 = 1;

    double volatile d4 = d1 + d3;

    PerformSyscall(SyscallSelection::DebugPrint, const_cast<char *>("BLERGH\r\n"), 0, 0, 0, 0);
    PerformSyscall(SyscallSelection::DebugPrint, const_cast<char *>("BLAH\r\n"), 0, 0, 0, 0);
    PerformSyscall(SyscallSelection::DebugPrint, const_cast<char *>("BLERGH\r\n"), 0, 0, 0, 0);
    PerformSyscall(SyscallSelection::DebugPrint, const_cast<char *>("BLAH\r\n"), 0, 0, 0, 0);
    PerformSyscall(SyscallSelection::DebugPrint, const_cast<char *>("BLERGH\r\n"), 0, 0, 0, 0);
    PerformSyscall(SyscallSelection::DebugPrint, const_cast<char *>("BLAH\r\n"), 0, 0, 0, 0);

    goto beginning;

    return 0;
}
