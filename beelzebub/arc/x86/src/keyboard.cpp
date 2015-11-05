#include <keyboard.hpp>

#include <system/cpu.hpp>   //  Only used for task switching right now...
#include <system/io_ports.hpp>
#include <system/timers/pit.hpp>

#include <kernel.hpp>
#include <debug.hpp>
#include <_print/isr.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::System;
using namespace Beelzebub::System::Timers;

bool keyboard_escaped = false;
volatile int breakpointEscaped = 0;

void keyboard_init(void)
{
    while (Io::In8(0x64) & 0x1)
    {
        Io::In8(0x60);
    }

    keyboard_send_command(0xF4);
}

void keyboard_send_command(uint8_t cmd)
{
    while (0 != (Io::In8(0x64) & 0x2))
    {
        //  Await.
    }

    Io::Out8(0x60, cmd);
}

void keyboard_handler(INTERRUPT_HANDLER_ARGS)
{
    uint8_t code = Io::In8(0x60);
    Io::Out8(0x61, Io::In8(0x61));

    if (KEYBOARD_CODE_ESCAPED == code)
    {
        keyboard_escaped = true;
    }
    else if (keyboard_escaped)
    {
        keyboard_escaped = false;

        switch (code)
        {
        case KEYBOARD_CODE_LEFT:
            {
                PitCommand pitCmd {};
                pitCmd.SetAccessMode(PitAccessMode::LowHigh);
                Pit::SendCommand(pitCmd);
                
                uint32_t pitFreq = 50;
                Pit::SetFrequency(pitFreq);

                //  This is merely a test of the PIT.
            }

            break;

        case KEYBOARD_CODE_RIGHT:
            breakpointEscaped = 0;

            break;

        case KEYBOARD_CODE_UP:
            Thread * const activeThread = Cpu::GetActiveThread();

            if (activeThread != nullptr)
            {
                activeThread->State = *state;

                msg("PRE-SWITCH ");
                PrintToDebugTerminal(state);
                msg("%n");

                msg("(( AT=%Xp; N=%Xp; P=%Xp; BST=%B ))%n", activeThread, activeThread->Next, activeThread->Previous, activeThread == &BootstrapThread);

                activeThread->SwitchToNext(state);

                msg("%nPOST-SWITCH ");
                PrintToDebugTerminal(state);
                msg("%n");
            }

            break;

        /*case KEYBOARD_CODE_DOWN:
            ui_scroll_down();
            break;//*/
        }
    }

    END_OF_INTERRUPT();
}
