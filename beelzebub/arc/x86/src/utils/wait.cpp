#include <utils/wait.hpp>
#include <system/timers/pit.hpp>
#include <system/cpu_instructions.hpp>
#include <math.h>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::System::Timers;
using namespace Beelzebub::Utils;

void Utils::Wait(uint64_t const microseconds)
{
    size_t difference = RoundUp(microseconds, 10000) / 10000;
    //  Round up to the length of a timer tick in microseconds, then get the
    //  number of ticks.

    size_t volatile counterStart = Pit::Counter;

    int_cookie_t const cookie = Interrupts::PushEnable();

    do
    {
        CpuInstructions::Halt();
    } while (Pit::Counter.Load() - counterStart < difference);
    //  Yes, the CPU can be halted currently, because only the BSP uses these.

    Interrupts::RestoreState(cookie);
}

bool Utils::Wait(uint64_t const microseconds, PredicateFunction0 const pred)
{
    if (pred())
        return true;
    //  Eh, just checkin'?

    size_t difference = RoundUp(microseconds, 10000) / 10000;
    //  Round up to the length of a timer tick in microseconds, then get the
    //  number of ticks.

    size_t volatile counterStart = Pit::Counter;

    int_cookie_t const cookie = Interrupts::PushEnable();

    do
    {
        CpuInstructions::Halt();

        if (pred())
            return true;
    } while (Pit::Counter.Load() - counterStart < difference);

    Interrupts::RestoreState(cookie);

    return pred();
}
