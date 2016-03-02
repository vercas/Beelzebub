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

#pragma once

#include <system/registers_x86.hpp>
#include <memory/paging.hpp>
#include <handles.h>

namespace Beelzebub { namespace System
{
    /**
     * Represents the contents of the CR3 register.
     */
    struct Cr3
    {
        /*  Bit structure with PCID disabled:
         *       0 -   2 : Ignored
         *       3       : PWT (page-level write-through)
         *       4       : PCD (page-level cache disable)
         *       5 -  11 : Ignored
         *      12 - M-1 : Physical address of PML4 table; 4-KiB aligned.
         *       M -  63 : Reserved (must be 0)
         *
         *  Bit structure with PCID enabled:
         *       0 -  11 : PCID
         *      12 - M-1 : Physical address of PML4 table; 4-KiB aligned.
         *       M -  63 : Reserved (must be 0)
         */

        /*  Properties  */

        BITFIELD_DEFAULT_1W( 3, Pwt)
        BITFIELD_DEFAULT_1W( 4, Pcd)

        static uint64_t const AddressBits   = 0x000FFFFFFFFFF000ULL;
        static uint64_t const PcidBits      = 0x0000000000000FFFULL;

        /*  Constructors  */

        /**
         *  Creates a new CR3 structure from the given raw value.
         */
        inline Cr3(uint64_t const val)
            : Value(val)
        {
            
        }

        /**
         *  Creates a new CR3 structure for use with PCID disabled.
         */
        __forceinline Cr3(paddr_t const paddr, bool const PWT, bool const PCD)
            : Value(((uint64_t)paddr & AddressBits)
                  | (PWT ? PwtBit : 0)
                  | (PCD ? PcdBit : 0))
        {
            
        }

        /**
         *  Creates a new CR3 structure for use with PCID enabled.
         */
        __forceinline Cr3(paddr_t const paddr, Handle const process)
            : Value(((uint64_t)paddr    & AddressBits)
                  | (process.GetIndex() & PcidBits   ))
        {
            //  TODO: Check whether the handle is correct or not.
        }

        /*  Properties  */

        /**
         *  Gets the physical address of the PML4 table.
         */
        __forceinline Memory::Pml4 * GetPml4Ptr() const
        {
            return (Memory::Pml4 *)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the PML4 table.
         */
        __forceinline void SetPml4Ptr(Memory::Pml4 const * const paddr)
        {
            this->Value = ((uint64_t)paddr       &  AddressBits)
                        | (          this->Value & ~AddressBits);
        }

        /**
         *  Gets the physical address of the PML4 table.
         */
        __forceinline paddr_t GetAddress() const
        {
            return (paddr_t)(this->Value & AddressBits);
        }
        /**
         *  Sets the physical address of the PML4 table.
         */
        __forceinline void SetAddress(paddr_t const paddr)
        {
            this->Value = ((uint64_t)paddr       &  AddressBits)
                        | (          this->Value & ~AddressBits);
        }

        /*  PCID  */

        /**
         *  Gets the process ID according to the PCID value.
         */
        __forceinline Handle GetProcess() const
        {
            //  TODO: Construct proper handles here!

            return Handle(HandleType::Process, this->Value & PcidBits);
        }
        /**
         *  Sets the PCID value from the given process.
         */
        __forceinline void SetProcess(Handle const process)
        {
            //  TODO: Check whether the handle is correct or not.

            this->Value = (process.GetIndex() &  PcidBits)
                        | (       this->Value & ~PcidBits);
        }

        /**
         *  Gets the PCID value.
         */
        __forceinline uint64_t GetPcid() const
        {
            //  TODO: Construct proper handles here!

            return this->Value & PcidBits;
        }
        /**
         *  Sets the PCID value.
         */
        __forceinline void SetPcid(uint64_t const pcid)
        {
            //  TODO: Check whether the handle is correct or not.

            this->Value = (pcid        &  PcidBits)
                        | (this->Value & ~PcidBits);
        }

        /*  Field(s)  */

    //private:

        uint64_t Value;
    };
}}
