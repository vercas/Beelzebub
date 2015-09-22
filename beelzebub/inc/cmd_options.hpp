#pragma once

#include <metaprogramming.h>
#include <string.h>
#include <handles.h>

namespace Beelzebub
{
    /**
     *  Possible types of command-line option values.
     */
    enum class CommandLineOptionValueTypes
        : uintptr_t
    {
        BooleanByPresence = 0,
        BooleanExplicit = 1,
        String = 2,
        SignedInteger = 3,
        UnsignedInteger = 4,
        Float = 5,
    };

    /**
     *  Flags indicating options for parsing command-line options.
     */
    enum class CommandLineOptionFlags
        : uintptr_t
    {
        TableFill = 0x0000,
        ReturnFromFunction = 0x0001,
        FlexibleValues = 0x0000,
        LimitedValues = 0x0002,
    };

    ENUMOPS(CommandLineOptionFlags, uintptr_t);

    /**
     *  Represents specification for parsing command line options.
     */
    struct CommandLineOptionSpecification
    {
    public:

        const char * ShortForm;
        const char * LongForm;

        const void * ValueList;
        const size_t ValuesCount;
        const CommandLineOptionValueTypes ValueType;

        const CommandLineOptionFlags Flags;

        Handle ParsingResult;

        union
        {
            char *   StringValue;
            bool     BooleanValue;
            int64_t  SignedIntegerValue;
            uint64_t UnsignedIntegerValue;
        };
    };

    /**
     *  Represents the state of a command-line option parser.
     */
    class CommandLineOptionParserState
    {
    public:

        char * const InputString;
        size_t Length;
        size_t Offset;

        bool Done;
        bool Started;

        __bland __forceinline CommandLineOptionParserState(char * const input)
            : InputString(input)
            , Length(0)
            , Offset(0)
            , Done(false)
            , Started(false)
        {
            //  Nuthin'.
        }
    };

    __bland Handle ParseCommandLineOptions(CommandLineOptionParserState & state);
}
