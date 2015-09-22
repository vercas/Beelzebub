#include <cmd_options.hpp>
#include <debug.hpp>

using namespace Beelzebub;

Handle Beelzebub::ParseCommandLineOptions(CommandLineOptionParserState & state)
{
    if (!state.Started)
    {
        state.Length = strlen(state.InputString);

        state.Started = true;

        msg("Length of \"%s\" is: %us.%n", state.InputString, state.Length);
    }

    return HandleResult::Okay;
}
