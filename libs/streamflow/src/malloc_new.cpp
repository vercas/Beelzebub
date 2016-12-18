/*
 * Copyright (C) 2007  Scott Schneider, Christos Antonopoulos
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
    Copyright (c) 2016 Alexandru-Mihai Maftei. All rights reserved.


    Developed by: Alexandru-Mihai Maftei
    aka Vercas
    http://vercas.com | https://github.com/vercas/Beelzebub

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    ---

    You may also find the licensing information for the rest of the code in
    "LICENSE.md", along with a more thorough explanation.
*/

#include <new>
#include <beel/metaprogramming.h>
#include <stdlib.h>

void* operator new(std::size_t size)
#ifdef __BEELZEBUB__IN_KERNEL
    noexcept
#else
    throw(std::bad_alloc)
#endif
{
	return malloc(size);
}

void* operator new[](std::size_t size)
#ifdef __BEELZEBUB__IN_KERNEL
    noexcept
#else
    throw(std::bad_alloc)
#endif
{
    return malloc(size);
}

void operator delete(void * ptr) noexcept
{
	free(ptr);
}

void operator delete[](void * ptr) noexcept
{
	free(ptr);
}

#if __cplusplus >= 201402L
void operator delete(void * ptr, std::size_t size) noexcept
{
    (void)size;

    free(ptr);
}

void operator delete[](void * ptr, std::size_t size) noexcept
{
    (void)size;

    free(ptr);
}
#endif

void * operator new(std::size_t size, std::nothrow_t const &) noexcept
{
    return malloc(size);
}

void * operator new[](std::size_t size, std::nothrow_t const &) noexcept
{
    return malloc(size);
}
