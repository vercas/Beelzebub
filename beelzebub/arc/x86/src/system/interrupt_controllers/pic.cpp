#include <system/interrupt_controllers/pic.hpp>
#include <system/io_ports.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::System::InterruptControllers;

/****************
    Pic class
****************/

/*  Statics  */

uint8_t Pic::VectorOffset = 0;
bool Pic::Active = false;

/*  Ender  */

void Pic::IrqEnder(INTERRUPT_ENDER_ARGS)
{
    if (vector >= VectorOffset + 8)
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
    for (size_t i = 0; i < 16; i++)
    {
        uint8_t vec = (uint8_t)(VectorOffset + i);

        if (InterruptEnders[vec] == &(Pic::IrqEnder))
            InterruptEnders[vec] = nullptr;
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
    for (size_t i = 0; i < 16; i++)
    {
        uint8_t vec = (uint8_t)(vecOff + i);

        assert(InterruptEnders[vec] == nullptr
            , "Interrupt vector #%u1 already had an ender?! (%Xp)%n"
            , vec, InterruptEnders[vec]);
        
        InterruptEnders[vec] = &(Pic::IrqEnder);
    }
 
    //  Restore masks.
    Io::Out8(MasterDataPort, maskM);
    Io::Out8(SlaveDataPort, maskS);

    VectorOffset = vecOff;
    Active = true;
}

void Pic::Disable()
{
    Active = false;

    //  Just mask all interrupts.
    Io::Out8(MasterDataPort, 0xFF);
    Io::Out8(SlaveDataPort, 0xFF);
}

/*  Subscription  */

bool Pic::Subscribe(uint8_t const irq, InterruptHandlerFunction const handler, bool const unmask)
{
    assert_or(irq < 16
        , "IRQ number is out of the range of the PIC: %u1%n"
        , irq)
    {
        return false;
    }

    uint8_t vec = (uint8_t)(VectorOffset + irq);

    assert_or(InterruptEnders[vec] == nullptr || InterruptEnders[vec] == &(Pic::IrqEnder)
        , "Interrupt vector #%u1 (IRQ%u1) already has an ender?! (%Xp)%n"
        , vec, irq, InterruptEnders[vec])
    {
        return false;
    }

    InterruptHandlers[vec] = handler;
    InterruptEnders[vec] = &(Pic::IrqEnder);

    if (unmask)
    {
        if (irq >= 8)
            Io::Out8( SlaveDataPort, (uint8_t)(Io::In8( SlaveDataPort) & ~(1 << (irq - 8))));
        else
            Io::Out8(MasterDataPort, (uint8_t)(Io::In8(MasterDataPort) & ~(1 <<  irq     )));
    }

    return true;
}

bool Pic::Unsubscribe(uint8_t const irq, bool const mask)
{
    assert_or(irq < 16
        , "IRQ number is out of the range of the PIC: %u1%n"
        , irq)
    {
        return false;
    }

    uint8_t vec = (uint8_t)(VectorOffset + irq);

    assert_or(InterruptEnders[vec] == &(Pic::IrqEnder)
        , "Interrupt vector #%u1 (IRQ%u1) already has the wrong ender! (%Xp)%n"
        , vec, irq, InterruptEnders[vec])
    {
        return false;
    }

    if (mask)
    {
        if (irq >= 8)
            Io::Out8( SlaveDataPort, (uint8_t)(Io::In8( SlaveDataPort) | (1 << (irq - 8))));
        else
            Io::Out8(MasterDataPort, (uint8_t)(Io::In8(MasterDataPort) | (1 <<  irq     )));
    }

    InterruptHandlers[vec] = nullptr;
    InterruptEnders[vec] = nullptr;

    return true;
}

bool Pic::IsSubscribed(uint8_t const irq)
{
    assert_or(irq < 16
        , "IRQ number is out of the range of the PIC: %u1%n"
        , irq)
    {
        return false;
    }

    uint8_t vec = (uint8_t)(VectorOffset + irq);

    return InterruptEnders[vec] == &(Pic::IrqEnder);
}

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
