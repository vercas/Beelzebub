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

#include <keyboard.hpp>

#include <system/cpu.hpp>   //  Only used for task switching right now...
#include <system/io_ports.hpp>
#include <system/timers/pit.hpp>

#include <kernel.hpp>
#include <debug.hpp>

using namespace Beelzebub;
using namespace Beelzebub::Execution;
using namespace Beelzebub::System;
using namespace Beelzebub::System::Timers;

bool keyboard_escaped = false;

int volatile breakpointEscaped = 0;
int volatile * volatile breakpointEscapedAux = nullptr;

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
            /*{
                PitCommand pitCmd {};
                pitCmd.SetAccessMode(PitAccessMode::LowHigh);
                Pit::SendCommand(pitCmd);
                
                uint32_t pitFreq = 50;
                Pit::SetFrequency(pitFreq);

                //  This is merely a test of the PIT.
            }*/

            break;

        case KEYBOARD_CODE_RIGHT:
            breakpointEscaped = 0;

            if (breakpointEscapedAux != nullptr)
                *breakpointEscapedAux = 0;

            break;

        case KEYBOARD_CODE_UP:
            Thread * const activeThread = Cpu::GetThread();

            if (activeThread != nullptr)
            {
                // activeThread->State = *state;

                //msg("PRE-SWITCH ");
                //PrintToDebugTerminal(state);
                //msg("%n");

                /*msg("(( AT=%Xp; N=%Xp; P=%Xp; BST=%B ))%n"
                    , activeThread, activeThread->Next, activeThread->Previous
                    , activeThread == &BootstrapThread);//*/

                activeThread->SwitchToNext(state);

                //msg("%nPOST-SWITCH ");
                //PrintToDebugTerminal(state);
                //msg("%n");
            }

            break;

        /*case KEYBOARD_CODE_DOWN:
            ui_scroll_down();
            break;//*/
        }
    }
}
