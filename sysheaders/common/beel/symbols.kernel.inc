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

/*
KeRNel SYMbol GLoBal FuNCtion (no namespace)
KeRNel SYMbol GLoBal VARiable
KeRNel SYMbol NaMeSpaced FuNCtion (in Beelzebub::)
KeRNel SYMbol NaMeSpaced VARiable

KRN_SYM_(GLB|NMS)_FNC(function name, attributes, return type, argument 1, argument 2, ...)
KRN_SYM_(GLB|NMS)_VAR(variable name, attributes, variable type)
 */

/******************************************************************************\
\*  String operations, very basic and necessary.                              */

KRN_SYM_GLB_FNC(memeq       , , bool        , void const * src1    , void const * src2  , size_t len)
KRN_SYM_GLB_FNC(memcmp      , , comp_t      , void const * src1    , void const * src2  , size_t len)
KRN_SYM_GLB_FNC(memchr      , , void       *, void const * src     , int          val   , size_t len)
KRN_SYM_GLB_FNC(memcpy      , , void       *, void       * dst     , void const * src   , size_t len)
KRN_SYM_GLB_FNC(memmove     , , void       *, void       * dst     , void const * src   , size_t len)
KRN_SYM_GLB_FNC(mempcpy     , , void       *, void       * dst     , void const * src   , size_t len)
KRN_SYM_GLB_FNC(mempmove    , , void       *, void       * dst     , void const * src   , size_t len)
KRN_SYM_GLB_FNC(memset      , , void       *, void       * dst     , int  const   val   , size_t len)
KRN_SYM_GLB_FNC(mempset     , , void       *, void       * dst     , int  const   val   , size_t len)
KRN_SYM_GLB_FNC(memset16    , , void       *, void       * dst     , int  const   val   , size_t cnt)
KRN_SYM_GLB_FNC(mempset16   , , void       *, void       * dst     , int  const   val   , size_t cnt)
KRN_SYM_GLB_FNC(memset32    , , void       *, void       * dst     , long const   val   , size_t cnt)
KRN_SYM_GLB_FNC(mempset32   , , void       *, void       * dst     , long const   val   , size_t cnt)
KRN_SYM_GLB_FNC(strlen      , , size_t      , char const * str)
KRN_SYM_GLB_FNC(strnlen     , , size_t      , char const * str     , size_t       len)
KRN_SYM_GLB_FNC(strnlenex   , , size_t      , char const * str     , size_t       len   , bool * reached)
KRN_SYM_GLB_FNC(strcat      , , char       *, char       * dst     , char const * src)
KRN_SYM_GLB_FNC(strncat     , , char       *, char       * dst     , char const * src   , size_t len)
KRN_SYM_GLB_FNC(strcpy      , , char       *, char       * dst     , char const * src)
KRN_SYM_GLB_FNC(strncpy     , , char       *, char       * dst     , char const * src   , size_t len)
KRN_SYM_GLB_FNC(strpbrk     , , char       *, char const * haystack, char const * needle)
KRN_SYM_GLB_FNC(strnpbrk    , , char       *, char const * haystack, char const * needle, size_t len)
KRN_SYM_GLB_FNC(strcmp      , , comp_t      , char const * src1    , char const * src2)
KRN_SYM_GLB_FNC(strncmp     , , comp_t      , char const * src1    , char const * src2  , size_t len)
KRN_SYM_GLB_FNC(strcasecmp  , , comp_t      , char const * src1    , char const * src2)
KRN_SYM_GLB_FNC(strcasencmp , , comp_t      , char const * src1    , char const * src2  , size_t len)
KRN_SYM_GLB_FNC(strstr      , , char const *, char const * haystack, char const * needle)
KRN_SYM_GLB_FNC(strstrex    , , char const *, char const * haystack, char const * needle, char const * seps)
KRN_SYM_GLB_FNC(strcasestrex, , char const *, char const * haystack, char const * needle, char const * seps)

/******************************************************************************\
\*  Basic system information and state                                        */

KRN_SYM_NMS_FNC(GetCpuData           ,                , Beelzebub::CpuData const *, void)
KRN_SYM_NMS_FNC(GetActiveProcess     ,                , Beelzebub::Process const *, void)
KRN_SYM_NMS_FNC(GetActiveThread      ,                , Beelzebub::Thread  const *, void)

/******************************************************************************\
\*  Exceptions                                                                */

KRN_SYM_NMS_FNC(GetException         ,                , Beelzebub::Exception *         , void)
KRN_SYM_NMS_FNC(GetExceptionContext  ,                , Beelzebub::ExceptionContext * *, void)
KRN_SYM_NMS_FNC(EnterExceptionContext, __returns_twice, bool                           , Beelzebub::ExceptionContext * context)
KRN_SYM_NMS_FNC(LeaveExceptionContext,                , void                           , void)
KRN_SYM_NMS_FNC(ThrowException       , __noreturn     , void                           , void)
