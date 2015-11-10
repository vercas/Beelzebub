#include <system/interrupts.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;

/***********************
    Interrupts class
***********************/

/*  Statics  */

Idt Interrupts::Table;
IdtRegister Interrupts::Register {0xFFF, &Interrupts::Table};
