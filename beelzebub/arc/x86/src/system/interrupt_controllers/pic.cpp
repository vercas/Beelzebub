/*
    Copyright (c) 2015 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal with the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimers.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimers in the
        documentation and/or other materials provided with the distribution.
      * Neither the names of Alexandru-Mihai Maftei, Vercas, nor the names of
        its contributors may be used to endorse or promote products derived from
        this Software without specific prior written permission.


    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    WITH THE SOFTWARE.

    ---

    You may also find the text of this license in "LICENSE.md", along with a more
    thorough explanation regarding other files.
*/

#include <system/interrupt_controllers/pic.hpp>
#include <system/io_ports.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;

#define PIC1_CMD                    0x20
#define PIC1_DATA                   0x21
#define PIC2_CMD                    0xA0
#define PIC2_DATA                   0xA1
#define PIC_READ_IRR                0x0a    /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR                0x0b    /* OCW3 irq service next CMD read */

/****************
    Pic class
****************/

/*  Statics  */

uint8_t Pic::VectorOffset = 0;
bool Pic::Active = false;
InterruptEnderNode Pic::EnderNode { &Pic::IrqEnder };

/*  Ender  */

void Pic::IrqEnder(InterruptContext const * context, void * cookie, InterruptEndType type)
{
    (void)cookie;

    if (context->IrqVector.Value >= 8)
        Io::Out8(SlaveCommandPort, 0x20);
 
    Io::Out8(MasterCommandPort, 0x20);
}

/*  (De)initialization  */

void Pic::Initialize(uint8_t const vecOff)
{
    //  Obtain masks.
    uint8_t maskM = Io::In8(MasterDataPort);
    uint8_t maskS = Io::In8(SlaveDataPort);

    Disable();

    //  Remove interrupt enders.
    for (int i = 0; i < 16; i++)
    {
        if (InterruptEnderNode::Get(irq_t(i)) == &EnderNode)
            ASSERT(EnderNode.Unregister(irq_t(i)) == IrqEnderUnregisterResult::Success);
    }
 
    //  Initialization sequence for cascade mode.
    Io::Out8(MasterCommandPort, 0x11);
    Io::Out8(SlaveCommandPort, 0x11);

    //  Set vector offsets
    Io::Out8(MasterDataPort, vecOff);
    Io::Out8(SlaveDataPort, vecOff + 8);

    //  Establish master-slave relationship. (this sounds wrong out of context)
    Io::Out8(MasterDataPort, 4);
    Io::Out8(SlaveDataPort, 2);
 
    Io::Out8(MasterDataPort, 0x01);
    Io::Out8(SlaveDataPort, 0x01);

    //  Set interrupt enders.
    for (int i = 0; i < 16; i++)
    {
        assert(InterruptEnderNode::Get(irq_t(i)) == nullptr
            , "IRQ #%u1 already had an ender?! (%Xp)%n"
            , (uint8_t)i, InterruptEnderNode::Get(irq_t(i)));
        
        ASSERT(EnderNode.Register(irq_t(i)) == IrqEnderRegisterResult::Success);
    }
 
    //  Restore masks.
    Io::Out8(MasterDataPort, maskM);
    Io::Out8(SlaveDataPort, maskS);

    VectorOffset = vecOff;
    Active = true;

    MSG("PIC IRR: %X2; ISR: %X2%n", GetIrrs(), GetIsrs());
}

void Pic::Disable()
{
    Active = false;

    //  Just mask all interrupts.
    Io::Out8(MasterDataPort, 0xFF);
    Io::Out8(SlaveDataPort, 0xFF);
}

// /*  Subscription  */

// bool Pic::Subscribe(uint8_t const irq, InterruptHandlerFunction const handler, bool const unmask)
// {
//     assert_or(irq < 16
//         , "IRQ number is out of the range of the PIC: %u1%n"
//         , irq)
//     {
//         return false;
//     }

//     auto vec = Interrupts::Get((uint8_t)(Pic::VectorOffset + irq));

//     assert_or(vec.GetEnder() == nullptr || vec.GetEnder() == &(Pic::IrqEnder)
//         , "Interrupt vector #%u1 (IRQ%u1) already has an ender?! (%Xp)%n"
//         , vec, irq, vec.GetEnder())
//     {
//         return false;
//     }

//     vec.SetHandler(handler);

//     vec.SetEnder(&(Pic::IrqEnder));

//     if (unmask)
//     {
//         if (irq >= 8)
//             Io::Out8(Pic::SlaveDataPort , (uint8_t)(Io::In8(Pic::SlaveDataPort ) & ~(1 << (irq - 8))));
//         else
//             Io::Out8(Pic::MasterDataPort, (uint8_t)(Io::In8(Pic::MasterDataPort) & ~(1 <<  irq     )));
//     }

//     return true;
// }

// bool Pic::Unsubscribe(uint8_t const irq, bool const mask)
// {
//     assert_or(irq < 16
//         , "IRQ number is out of the range of the PIC: %u1%n"
//         , irq)
//     {
//         return false;
//     }

//     auto vec = Interrupts::Get((uint8_t)(VectorOffset + irq));

//     assert_or(vec.GetEnder() == &(Pic::IrqEnder)
//         , "Interrupt vector #%u1 (IRQ%u1) already has the wrong ender! (%Xp)%n"
//         , vec, irq, vec.GetEnder())
//     {
//         return false;
//     }

//     if (mask)
//     {
//         if (irq >= 8)
//             Io::Out8( SlaveDataPort, (uint8_t)(Io::In8( SlaveDataPort) | (1 << (irq - 8))));
//         else
//             Io::Out8(MasterDataPort, (uint8_t)(Io::In8(MasterDataPort) | (1 <<  irq     )));
//     }

//     vec.RemoveHandler().SetEnder(nullptr);

//     return true;
// }

// bool Pic::IsSubscribed(uint8_t const irq)
// {
//     assert_or(irq < 16
//         , "IRQ number is out of the range of the PIC: %u1%n"
//         , irq)
//     {
//         return false;
//     }

//     auto vec = Interrupts::Get((uint8_t)(VectorOffset + irq));

//     return vec.GetEnder() == &(Pic::IrqEnder);
// }

/*  Masking  */

bool Pic::SetMasked(uint8_t const irq, bool const masked)
{
    assert_or(irq < 16
        , "IRQ number is out of the range of the PIC: %u1%n"
        , irq)
    {
        return false;
    }

    if (masked)
    {
        if (irq >= 8)
            Io::Out8( SlaveDataPort, (uint8_t)(Io::In8( SlaveDataPort) |  (1 << (irq - 8))));
        else
            Io::Out8(MasterDataPort, (uint8_t)(Io::In8(MasterDataPort) |  (1 <<  irq     )));
    }
    else
    {
        if (irq >= 8)
            Io::Out8( SlaveDataPort, (uint8_t)(Io::In8( SlaveDataPort) & ~(1 << (irq - 8))));
        else
            Io::Out8(MasterDataPort, (uint8_t)(Io::In8(MasterDataPort) & ~(1 <<  irq     )));
    }

    return true;
}

bool Pic::GetMasked(uint8_t const irq)
{
    assert_or(irq < 16
        , "IRQ number is out of the range of the PIC: %u1%n"
        , irq)
    {
        return false;
    }

    if (irq >= 8)
        return 0 != (Io::In8( SlaveDataPort) & (1 << (irq - 8)));
    else
        return 0 != (Io::In8(MasterDataPort) & (1 <<  irq     ));
}


static uint16_t PicGetIrqReg(int ocw3)
{
    Io::Out8(Pic::MasterCommandPort, ocw3);
    Io::Out8(Pic::SlaveCommandPort, ocw3);

    return (Io::In8(Pic::SlaveCommandPort) << 8) | Io::In8(Pic::MasterCommandPort);
}

uint16_t Pic::GetIrrs(void)
{
    return PicGetIrqReg(0x0a);
}

uint16_t Pic::GetIsrs(void)
{
    return PicGetIrqReg(0x0b);
}

