/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


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

#include <crt0.hpp>

using namespace Beelzebub;

/// Global constructors.
__extern __used void _init(void);
/// Global destructors.
__extern __used void _fini(void);

/// Legacy entry point.
__extern __used int main(int argc, char * * argv);

/// Beelzebub entry point, which is preferred.
__extern __used __weak Handle BeelMain();

void * volatile BeelMainAddress = reinterpret_cast<void *>(&BeelMain);

__extern __bland __used void _start(char * args)
{
    //  At this point, all registers should have nice null values, 'xept the ones
    //  used to pass arguments.

    bool legacy = BeelMainAddress == nullptr;

    int argc = -1;
    char * * argv = nullptr;

    Handle hRes = InitializeRuntime(legacy, args, argv, argc);
    //  First, initialize the runtime (this should come from the runtime library)

    if unlikely(!hRes.IsOkayResult())
        return QuitProcess(hRes, -1);
    //  Any failures mean urgent termination.

    _init();
    //  Secondly, invoke global constructors.

    if (legacy)
    {
        int iRes = main(argc, argv);
        //  This is the legacy entry point.

        return QuitProcess(iRes == 0 ? HandleResult::Okay : HandleResult::NonZeroReturnCode
            , iRes);
    }
    else
    {
        //  TODO: Maybe parse argument through a default way?

        hRes = BeelMain();

        return QuitProcess(hRes, 0);
    }
}

__extern void __start(char * input) __alias(_start);
