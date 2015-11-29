#include <stdint.h>

extern "C" __attribute__((__used__)) int _start()
{
    uintptr_t volatile someAddress = 0x30000;
    uintptr_t volatile someEnd = someAddress + someAddress;
    //  Yay.

    *reinterpret_cast<uint64_t *>(someAddress) = 0;

    do
    {
        ++(*reinterpret_cast<uint64_t *>(someAddress));

        for (uint8_t * ptr = reinterpret_cast<uint8_t *>(someAddress + 8); (uintptr_t)ptr < someEnd; ++ptr)
            ++(*ptr);

    } while (true);

    return 0;
}
