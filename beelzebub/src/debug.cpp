#include <debug.hpp>
#include <system/cpu.hpp>

using namespace Beelzebub;
using namespace Beelzebub::System;
using namespace Beelzebub::Synchronization;

Spinlock Beelzebub::Debug::MsgSpinlock;
TerminalBase * Beelzebub::Debug::DebugTerminal;

namespace Beelzebub { namespace Debug
{
    //  Although the 'CatchFire' functions will brick the CPU,
    //  I still feel obliged to make them... Efficient...

    void CatchFire(const char * const file
                 , const size_t line
                 , const char * const msg)
    {
        if (DebugTerminal != nullptr
         && DebugTerminal->Descriptor->Capabilities.CanOutput)
        {
            (&MsgSpinlock)->Acquire();

            DebugTerminal->WriteLine("");
            DebugTerminal->Write("CAUGHT FIRE at line ");
            DebugTerminal->WriteUIntD(line);
            DebugTerminal->Write(" of \"");
            DebugTerminal->Write(file);

            if (msg == nullptr)
                DebugTerminal->WriteLine("\".");
            else
            {
                DebugTerminal->WriteLine("\":");
                DebugTerminal->WriteLine(msg);
            }

            (&MsgSpinlock)->Release();
        }

        Cpu::DisableInterrupts();

        //  Allow the CPU to rest.
        while (true) if (Cpu::CanHalt) Cpu::Halt();

        __unreachable_code;
    }

    void CatchFire(const char * const file
                 , const size_t line
                 , const char * const fmt, va_list args)
    {
        if (DebugTerminal != nullptr
         && DebugTerminal->Descriptor->Capabilities.CanOutput)
        {
            (&MsgSpinlock)->Acquire();

            DebugTerminal->WriteLine("");
            DebugTerminal->Write("CAUGHT FIRE at line ");
            DebugTerminal->WriteUIntD(line);
            DebugTerminal->Write(" of \"");
            DebugTerminal->Write(file);

            if (fmt == nullptr)
                DebugTerminal->WriteLine("\".");
            else
            {
                DebugTerminal->WriteLine("\":");
                DebugTerminal->Write(fmt, args);
            }

            (&MsgSpinlock)->Release();
        }

        Cpu::DisableInterrupts();

        //  Allow the CPU to rest... IN PEACE! :OOOOooOO00OOoO
        while (true) if (Cpu::CanHalt) Cpu::Halt();

        __unreachable_code;
    }

    void CatchFireFormat(const char * const file
                       , const size_t line
                       , const char * const fmt, ...)
    {
        va_list args;

        va_start(args, fmt);

        CatchFire(file, line, fmt, args);
        //  That function will never return.

        va_end(args);
    }

    void Assert(const bool condition
              , const char * const file
              , const size_t line
              , const char * const msg)
    {
        if unlikely(!condition)
            CatchFire(file, line, msg);
    }

    void Assert(const bool condition
               , const char * const file
               , const size_t line
               , const char * const msg, va_list args)
    {
        if unlikely(!condition)
            CatchFire(file, line, msg, args);
    }

    void AssertFormat(const bool condition
                    , const char * const file
                    , const size_t line
                    , const char * const fmt, ...)
    {
        if unlikely(!condition)
        {
            va_list args;

            va_start(args, fmt);

            CatchFire(file, line, fmt, args);
            //  That function will never return either.

            va_end(args);
        }

        //  No reason to mess with the varargs otherwise.
    }
}}
