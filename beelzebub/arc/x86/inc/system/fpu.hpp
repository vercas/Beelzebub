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

#pragma once

#include <utils/bitfields.hpp>

namespace Beelzebub { namespace System
{
    /**
     *  Represents a floating point register used by the x87 FPU.
     */
    struct FpuRegister
    {
        uint64_t Lower;
        uint16_t Higher;
    } __packed;

    /**
     *  Represents an x87 FPU register as represented in the FXSAVE area.
     */
    struct PaddedFpuRegister
    {
        FpuRegister Value;
        uint16_t Reserved[3];
    } __packed;

    static_assert(sizeof(FpuRegister) == 10, "Struct size mismatch.");
    static_assert(sizeof(PaddedFpuRegister) == 16, "Struct size mismatch.");

    struct DoubleQuads
    {
        uint64_t Quads[2];
    } __packed;

    struct QuadDoubles
    {
        uint32_t Doubles[4];
    } __packed;

    struct OctoWords
    {
        uint16_t Words[8];
    } __packed;

    /**
     *  Represents an extended multi-media register used by SSE.
     */
    union XmmRegister
    {
        DoubleQuads Double;
        QuadDoubles Quad;
        OctoWords Octo;
        uint8_t Bytes[16];
    };

    static_assert(sizeof(DoubleQuads) == 16, "Struct size mismatch.");
    static_assert(sizeof(QuadDoubles) == 16, "Struct size mismatch.");
    static_assert(sizeof(OctoWords) == 16, "Struct size mismatch.");
    static_assert(sizeof(XmmRegister) == 16, "Union size mismatch.");

    /**
     *  Represents the map of the memory area as used by the FXSAVE instruction,
     *  and the XSAVE legacy area.
     */
    struct FxsaveMap
    {
        uint8_t Header[32];
        PaddedFpuRegister FpuRegisters[8];
        XmmRegister SseRegisters[16];
        uint8_t Reserved[48];
        uint8_t Available[48];
    } __packed __aligned(64);

    static_assert(sizeof(FxsaveMap) == 512, "Struct size mismatch.");

    /**
     *  Represents the XSAVE header added to the end of the legacy area by the
     *  XSAVE instruction.
     */
    struct XsaveHeader
    {
        uint64_t StateComponentBitmap[2];
        uint8_t Reserved[48];
    } __packed;

    static_assert(sizeof(XsaveHeader) == 64, "Struct size mismatch.");

    struct XsaveRfbm
    {
        /*  Bit structure for the XSAVE RFBM:
         *       0       : x87 State
         *       1       : SSE State
         *       2       : AVX State
         *       3 -   4 : MPX State
         *          3    : BNDREGS State
         *          4    : BNDCSR State
         *       5 -   7 : AVX-512 State
         *          5    : Opmask State
         *          6    : ZMM High 256 State
         *          7    : High 16 ZMM State
         *       8       : PT State
         *       9       : PKRU State
         *      10 -  63 : Reserved
         */

        /*  Properties  */

        BITFIELD_DEFAULT_1W(0, X87);
        BITFIELD_DEFAULT_1W(1, Sse);
        BITFIELD_DEFAULT_1W(2, Avx);
        BITFIELD_DEFAULT_1W(3, MpxBndregs);
        BITFIELD_DEFAULT_1W(4, MpxBndcsr);
        BITFIELD_DEFAULT_1W(5, Avx512Opmask);
        BITFIELD_DEFAULT_1W(6, Avx512ZmmHigh256);
        BITFIELD_DEFAULT_1W(7, Avx512High16Zmm);
        BITFIELD_DEFAULT_1W(8, Pt);
        BITFIELD_DEFAULT_1W(9, Pkru);

        /*  Constructors  */

        /**
         *  Creates an empty XSAVE RFBM structure.
         */
        inline XsaveRfbm() : Value( 0ULL ) { }

        /*  Fields  */

        union
        {
            uint64_t Value;

            struct
            {
                uint32_t Low;
                uint32_t High;
            };
        };
    };

    /**
     *  Contains functions for interacting with the x87 FPU.
     */
    class Fpu
    {
        /*  Statics  */

        static bool Initialized, Sse, Avx;

        /*  Operations  */

        void Initialize();
    };
}}
