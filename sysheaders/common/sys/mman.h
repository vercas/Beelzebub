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

#ifdef __BEELZEBUB__SOURCE_CXX
extern "C" {
#endif

typedef __SIZE_TYPE__ size_t;
typedef unsigned long long off_t;

enum PROT
{
    PROT_READ = 1,
    PROT_WRITE = 2,
    PROT_EXEC = 4,
    PROT_NONE = 0,
};

enum MAP
{
    MAP_SHARED = 1,
    MAP_PRIVATE = 0,
    MAP_FIXED = 2,
    MAP_ANON = 4,   //  Seriously? It's deprecated.
    MAP_ANONYMOUS = 4,

    MAP_HUGETLB = 0x100,
    MAP_HUGE_2MB = 0x100,
};

enum MREMAP
{
    MREMAP_MAYMOVE = 1,
    MREMAP_FIXED = 2,
};

#define MAP_FAILED ((void *)(unsigned long)(long)(-1))

void * mmap(void * addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void * addr, size_t length);
void * mremap(void * old_address, size_t old_size, size_t new_size, int flags, ...);

#ifdef __BEELZEBUB__SOURCE_CXX
}
#endif
