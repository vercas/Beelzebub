/*
    Copyright (c) 2017 Alexandru-Mihai Maftei. All rights reserved.


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

    ---

    SCOPE_EXIT macro inspired by the work of Andrei Alexandrescu.
*/

#include <beel/metaprogramming.h>

namespace Beelzebub { namespace Utils
{
    /// <summary>Guards a scope with a specified function.</summary>
    template<typename TFunc>
    struct ScopeGuard
    {
        /*  Constructor(s)  */

        __forceinline ScopeGuard(TFunc const func) : Guard(func) { }

        ScopeGuard(ScopeGuard const &) = delete;
        ScopeGuard(ScopeGuard && other) = default;
        ScopeGuard & operator =(ScopeGuard const &) = delete;
        ScopeGuard & operator =(ScopeGuard &&) = default;

        /*  Destructor  */

        __forceinline ~ScopeGuard()
        {
            this->Guard();
        }

    private:
        /*  Field(s)  */

        TFunc const Guard;
    };

    enum class __Dummy1 { };

    template<typename TFunc>
    __forceinline ScopeGuard<TFunc> operator +(__Dummy1, TFunc const func)
    {
        return ScopeGuard<TFunc>(func);
    }

    #define SCOPE_EXIT auto ANONVAR(__scope_exit) \
        = __Dummy1() + [&]() __forceinline -> void 
}}
