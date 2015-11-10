#include <system/interrupts.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;

/***********************
    Interrupts class
***********************/

/*  Statics  */

Idt Interrupts::Table;
