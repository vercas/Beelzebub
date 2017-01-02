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

#ifdef __cplusplus
extern "C" {
#endif

extern __thread int errno;

/*- THE REST IS TAKEN FROM http://src.gnu-darwin.org/src/sys/sys/errno.h.html
 *
 * Copyright (c) 1982, 1986, 1989, 1993
 * Alexandru-Mihai Maftei -AND-
 *  The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)errno.h 8.5 (Berkeley) 1/21/94
 * $FreeBSD: src/sys/sys/errno.h,v 1.28 2005/04/02 12:33:28 das Exp $
 */

#define ENUM_ERRNO(ENUMINST) \
    ENUMINST(EPERM           ,  1 , "Operation not permitted") \
    ENUMINST(ENOENT          ,  2 , "No such file or directory") \
    ENUMINST(ESRCH           ,  3 , "No such process") \
    ENUMINST(EINTR           ,  4 , "Interrupted system call") \
    ENUMINST(EIO             ,  5 , "Input/output error") \
    ENUMINST(ENXIO           ,  6 , "Device not configured") \
    ENUMINST(E2BIG           ,  7 , "Argument list too long") \
    ENUMINST(ENOEXEC         ,  8 , "Exec format error") \
    ENUMINST(EBADF           ,  9 , "Bad file descriptor") \
    ENUMINST(ECHILD          ,  10, "No child processes") \
    ENUMINST(EDEADLK         ,  11, "Resource deadlock avoided") \
    ENUMINST(ENOMEM          ,  12, "Cannot allocate memory") \
    ENUMINST(EACCES          ,  13, "Permission denied") \
    ENUMINST(EFAULT          ,  14, "Bad address") \
    ENUMINST(EBUSY           ,  16, "Device busy") \
    ENUMINST(EEXIST          ,  17, "File exists") \
    ENUMINST(EXDEV           ,  18, "Cross-device link") \
    ENUMINST(ENODEV          ,  19, "Operation not supported by device") \
    ENUMINST(ENOTDIR         ,  20, "Not a directory") \
    ENUMINST(EISDIR          ,  21, "Is a directory") \
    ENUMINST(EINVAL          ,  22, "Invalid argument") \
    ENUMINST(ENFILE          ,  23, "Too many open files in system") \
    ENUMINST(EMFILE          ,  24, "Too many open files") \
    ENUMINST(ENOTTY          ,  25, "Inappropriate ioctl for device") \
    ENUMINST(EFBIG           ,  27, "File too large") \
    ENUMINST(ENOSPC          ,  28, "No space left on device") \
    ENUMINST(ESPIPE          ,  29, "Illegal seek") \
    ENUMINST(EROFS           ,  30, "Read-only filesystem") \
    ENUMINST(EMLINK          ,  31, "Too many links") \
    ENUMINST(EPIPE           ,  32, "Broken pipe") \
    ENUMINST(EDOM            ,  33, "Numerical argument out of domain") \
    ENUMINST(ERANGE          ,  34, "Result too large") \
    ENUMINST(EAGAIN          ,  35, "Resource temporarily unavailable") \
    ENUMINST(ENAMETOOLONG    ,  63, "File name too long") \
    ENUMINST(ENOTEMPTY       ,  66, "Directory not empty") \
    ENUMINST(ENOLCK          ,  77, "No locks available") \
    ENUMINST(ENOSYS          ,  78, "Function not implemented") \
    ENUMINST(EBADMSG         ,  89, "Bad message") \
    ENUMINST(EMULTIHOP       ,  90, "Multihop attempted") \
    ENUMINST(ENOLINK         ,  91, "Link has been severed") \
    ENUMINST(EPROTO          ,  92, "Protocol error") \
    ENUM_ERRNO_NONPOSIX(ENUMINST)

#ifndef _POSIX_SOURCE
#define ENUM_ERRNO_NONPOSIX(ENUMINST) \
    ENUMINST(ENOTBLK         ,  15, "Block device required") \
    ENUMINST(ETXTBSY         ,  26, "Text file busy") \
    /*ENUMINST(EWOULDBLOCK     ,  35, "Operation would block")*/ \
    ENUMINST(EINPROGRESS     ,  36, "Operation now in progress") \
    ENUMINST(EALREADY        ,  37, "Operation already in progress") \
    ENUMINST(ENOTSOCK        ,  38, "Socket operation on non-socket") \
    ENUMINST(EDESTADDRREQ    ,  39, "Destination address required") \
    ENUMINST(EMSGSIZE        ,  40, "Message too long") \
    ENUMINST(EPROTOTYPE      ,  41, "Protocol wrong type for socket") \
    ENUMINST(ENOPROTOOPT     ,  42, "Protocol not available") \
    ENUMINST(EPROTONOSUPPORT ,  43, "Protocol not supported") \
    ENUMINST(ESOCKTNOSUPPORT ,  44, "Socket type not supported") \
    ENUMINST(EOPNOTSUPP      ,  45, "Operation not supported") \
    /*ENUMINST(ENOTSUP         ,  45, "Operation not supported")*/ \
    ENUMINST(EPFNOSUPPORT    ,  46, "Protocol family not supported") \
    ENUMINST(EAFNOSUPPORT    ,  47, "Address family not supported by protocol family") \
    ENUMINST(EADDRINUSE      ,  48, "Address already in use") \
    ENUMINST(EADDRNOTAVAIL   ,  49, "Can't assign requested address") \
    ENUMINST(ENETDOWN        ,  50, "Network is down") \
    ENUMINST(ENETUNREACH     ,  51, "Network is unreachable") \
    ENUMINST(ENETRESET       ,  52, "Network dropped connection on reset") \
    ENUMINST(ECONNABORTED    ,  53, "Software caused connection abort") \
    ENUMINST(ECONNRESET      ,  54, "Connection reset by peer") \
    ENUMINST(ENOBUFS         ,  55, "No buffer space available") \
    ENUMINST(EISCONN         ,  56, "Socket is already connected") \
    ENUMINST(ENOTCONN        ,  57, "Socket is not connected") \
    ENUMINST(ESHUTDOWN       ,  58, "Can't send after socket shutdown") \
    ENUMINST(ETOOMANYREFS    ,  59, "Too many references: can't splice") \
    ENUMINST(ETIMEDOUT       ,  60, "Operation timed out") \
    ENUMINST(ECONNREFUSED    ,  61, "Connection refused") \
    ENUMINST(ELOOP           ,  62, "Too many levels of symbolic links") \
    ENUMINST(EHOSTDOWN       ,  64, "Host is down") \
    ENUMINST(EHOSTUNREACH    ,  65, "No route to host") \
    ENUMINST(EPROCLIM        ,  67, "Too many processes") \
    ENUMINST(EUSERS          ,  68, "Too many users") \
    ENUMINST(EDQUOT          ,  69, "Disc quota exceeded") \
    ENUMINST(ESTALE          ,  70, "Stale NFS file handle") \
    ENUMINST(EREMOTE         ,  71, "Too many levels of remote in path") \
    ENUMINST(EBADRPC         ,  72, "RPC struct is bad") \
    ENUMINST(ERPCMISMATCH    ,  73, "RPC version wrong") \
    ENUMINST(EPROGUNAVAIL    ,  74, "RPC prog. not avail") \
    ENUMINST(EPROGMISMATCH   ,  75, "Program version wrong") \
    ENUMINST(EPROCUNAVAIL    ,  76, "Bad procedure for program") \
    ENUMINST(EFTYPE          ,  79, "Inappropriate file type or format") \
    ENUMINST(EAUTH           ,  80, "Authentication error") \
    ENUMINST(ENEEDAUTH       ,  81, "Need authenticator") \
    ENUMINST(EIDRM           ,  82, "Identifier removed") \
    ENUMINST(ENOMSG          ,  83, "No message of desired type") \
    ENUMINST(EOVERFLOW       ,  84, "Value too large to be stored in data type") \
    ENUMINST(ECANCELED       ,  85, "Operation canceled") \
    ENUMINST(EILSEQ          ,  86, "Illegal byte sequence") \
    ENUMINST(ENOATTR         ,  87, "Attribute not found") \
    ENUMINST(EDOOFUS         ,  88, "Programming error") \
    /*ENUMINST(ELAST           ,  92, "Must be equal largest errno")*/ \

#else
    #define ENUM_ERRNO_NONPOSIX(ENUMINST)    
#endif

enum ERRNO
{
#define ERRNO_VAL(n, v, s) n = v,
    ENUM_ERRNO(ERRNO_VAL)
#undef ERRNO_VAL
    ENOTSUP = EOPNOTSUPP,
};

#ifdef __cplusplus
}
#endif
