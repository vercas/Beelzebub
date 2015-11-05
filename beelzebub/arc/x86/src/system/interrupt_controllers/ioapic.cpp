#include <system/interrupt_controllers/ioapic.hpp>
#include <system/interrupt_controllers/lapic.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;

/******************
    Ioaic class
******************/

/*  Statics  */

size_t Ioapic::Count;
Ioapic Ioapic::All[Ioapic::Limit];

/*  Ender  */

void Ioapic::IrqEnder(INTERRUPT_ENDER_ARGS)
{
    Lapic::EndOfInterrupt();
}
