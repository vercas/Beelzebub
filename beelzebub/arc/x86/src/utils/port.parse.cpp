/*
    Copyright (c) 2018 Alexandru-Mihai Maftei. All rights reserved.


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

#include "utils/port.parse.hpp"
#include <beel/utils/conversions.hpp>
#include <string.h>

using namespace Beelzebub;
using namespace Beelzebub::Utils;

Result<PortParseResult, uint16_t> Utils::ParsePort(char const * val)
{
    if (strcasecmp(val, "COM1") == 0) return PortParseResult::COM1;
    if (strcasecmp(val, "COM2") == 0) return PortParseResult::COM2;
    if (strcasecmp(val, "COM3") == 0) return PortParseResult::COM3;
    if (strcasecmp(val, "COM4") == 0) return PortParseResult::COM4;

    if (strcasecmp(val, "COM1Base64") == 0) return PortParseResult::COM1Base64;
    if (strcasecmp(val, "COM2Base64") == 0) return PortParseResult::COM2Base64;
    if (strcasecmp(val, "COM3Base64") == 0) return PortParseResult::COM3Base64;
    if (strcasecmp(val, "COM4Base64") == 0) return PortParseResult::COM4Base64;

    if (strcasecmp(val, "ethernet") == 0) return PortParseResult::Ethernet;

    if (strcasecmp(val, "vbe") == 0) return PortParseResult::Vbe;

    uint16_t port;
    Handle res = FromString(val, port);

    if (res == HandleResult::Okay)
        return port;
    //  Welp, this is a valid port if the parsing succeeded.

    return PortParseResult::Error;
}
