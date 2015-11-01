#include <utils/checksum.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

uint8_t Beelzebub::Utils::Checksum8(void const * const start, size_t const length)
{
    uint8_t const * ptr = (uint8_t const *)start;
    uint8_t sum = 0;

    for (size_t i = 0; i < length; ++i)
        sum += ptr[i];

    return sum;
}
