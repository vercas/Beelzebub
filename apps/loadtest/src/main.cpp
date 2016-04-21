#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Terminals;

int main(int, char * *);

static __used void * const main_ptr = (void *)(&main);

int main(int argc, char * * argv)
{
beginning:

    uintptr_t volatile someAddress = 0x300000000000;
    uintptr_t volatile someEnd = someAddress + 0x30000;
    //  Yay.

    *reinterpret_cast<uint64_t *>(someAddress) = 0;

    for (size_t i = 0; i < 1000; ++i)
    {
        ++(*reinterpret_cast<uint64_t *>(someAddress));

        for (uint8_t * ptr = reinterpret_cast<uint8_t *>(someAddress + 8); (uintptr_t)ptr < someEnd; ++ptr)
            ++(*ptr);

        DEBUG_TERM << "Syscall test from userland app!" << EndLine;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

    double volatile d1 = -1, d2 = 0, d3 = 1;

    double volatile d4 = d1 + d3;

#pragma GCC diagnostic pop

    DEBUG_TERM  << "BLERGH" << EndLine << "BLAH" << EndLine
                << "BLERGH" << EndLine << "BLAH" << EndLine
                << "BLERGH" << EndLine << "BLAH" << EndLine;

    DEBUG_TERM << "My 'main' is at " << main_ptr << EndLine;

    goto beginning;

    return 0;
}
