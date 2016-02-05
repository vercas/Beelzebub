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

#include <handles.h>
#include <synchronization/atomic.hpp>

namespace Beelzebub { namespace Execution
{
    class Image;
    //  Forward declaration for the sake of the callback.

    typedef Handle (*ImageUnloadCallback)(Image * const img);

    enum class ImageType
    {
        Invalid = 0x00,
        None    = 0x01,
        Unknown = 0x02,

        Elf32   = 0x10,
        Elf64   = 0x11,

        Pe32    = 0x20,
        Pe64    = 0x21,
    };

    enum class ImageRole
    {
        Invalid     = 0x00,
        None        = 0x01,
        Auto        = 0x02,

        //  Userland
        Executable  = 0x10,
        Library     = 0x11,

        //  System/kernel
        Service     = 0x20,
        Driver      = 0x21,
        Kernel      = 0x22,
    };

    class Image
    {
        //  Note:
        //  Bit 0 (LSB of this->ReferenceCount) is used by the object allocator
        //  as the busy bit.

    public:
        /*  Constructor(s)  */

        inline Image()
            : ReferenceCount( 0)
            , Name(nullptr)
            , Type(ImageType::Invalid)
            , Role(ImageRole::Invalid)
            , Start(nullptr)
            , End(nullptr)
            , Size(0)
            , UnloadCallback(nullptr)
        {

        }

        Image(Image const &) = delete;
        Image & operator =(Image const &) = delete;

        /*  Reference Count  */

    private:
        Synchronization::Atomic<size_t> ReferenceCount;

    public:
        inline size_t IncrementReferenceCount()
        {
            return (this->ReferenceCount += 2) >> 1;
        }

        Handle DecrementReferenceCount(size_t & newCount);

        inline Handle DecrementReferenceCount()
        {
            size_t dummy;

            return this->DecrementReferenceCount(dummy);
        }

        inline void SetReferenceCount(size_t const count)
        {
            this->ReferenceCount = (count << 1) | (this->ReferenceCount.Load() & 1);
        }

        /*  Operations  */

        void Parse();

        /*  Fields  */

        char const * Name;

        ImageType Type;
        ImageRole Role;

        uint8_t * Start;
        uint8_t * End;
        size_t Size;

        /*  Others  */

        ImageUnloadCallback UnloadCallback;
    };

    class Images
    {
        /*  Statics  */

        /// Maximum number of images that can be loaded.
        static constexpr size_t const Limit = 1 << 16;

        /*  Constructor(s)  */

    protected:
        Images() = default;

    public:
        Images(Images const &) = delete;
        Images & operator =(Images const &) = delete;

        /*  Initialization  */

        static __cold void Initialize();

        /*  (Un)load  */

        static Handle Load(char const * const name, ImageRole const role
            , uint8_t * const imgStart, size_t const size, Image * & img
            , ImageUnloadCallback ucbk = nullptr);
        
        static Handle Unload(Image * const img);

        /*  Retrieval  */

        static Image * FindByName(char const * const name);
    };
}}
