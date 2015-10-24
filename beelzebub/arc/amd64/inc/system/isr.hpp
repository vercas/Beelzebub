#pragma once

#include <metaprogramming.h>

namespace Beelzebub { namespace System
{
    /**
     *  The state of the system before an interrupt was raised, which
     *  that can be used and manipulated by the ISR.
     */
    struct IsrState
    {
    public:
        /*  Field(s)  */

        uint64_t DS;

        uint64_t R15;
        uint64_t R14;
        uint64_t R13;
        uint64_t R12;
        uint64_t R11;
        uint64_t R10;
        uint64_t R9;
        uint64_t R8;
        uint64_t RBP;
        uint64_t RDI;
        uint64_t RSI;
        uint64_t RDX;
        uint64_t RCX;
        uint64_t RBX;
        uint64_t RAX;

        uint64_t Vector;
        uint64_t ErrorCode;

        uint64_t RIP;
        uint64_t CS;
        uint64_t RFLAGS;
        uint64_t RSP;
        uint64_t SS;
    } __packed;
}}

typedef void (*IsrHandlerFunction)(Beelzebub::System::IsrState * const state);

#define ISR_COUNT 256

/**
*  Array of pointers to all interrupt gates.
*/
__extern uint64_t IsrGates[ISR_COUNT];

/**
*  Array of higher-level interrupt handlers.
*/
__extern IsrHandlerFunction IsrHandlers[ISR_COUNT];
