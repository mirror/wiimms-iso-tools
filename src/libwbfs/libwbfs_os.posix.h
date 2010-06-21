
/***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project.                                 *
 *   Visit http://wit.wiimm.de/ for project details and sources.           *
 *                                                                         *
 *   Copyright (c) 2009 Kwiirk                                             *
 *   Copyright (c) 2009-2010 by Dirk Clemens <wiimm@wiimm.de>              *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   See file gpl-2.0.txt or http://www.gnu.org/licenses/gpl-2.0.txt       *
 *                                                                         *
 ***************************************************************************/
       
////////////////////////////////////////////////////////////////////////
/////   This is a tmeplate file for 'libwbfs_os.h'                 /////
/////   Move it to your project directory and rename it.           /////
/////   Edit this file and customise it for your project and os.   /////
////////////////////////////////////////////////////////////////////////

#ifndef LIBWBFS_OS_H
#define LIBWBFS_OS_H

// system includes
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

// unsigned integer types
typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

// signed integer types
typedef signed char		s8;
typedef signed short		s16;
typedef signed int		s32;
typedef signed long long	s64;

// other types
typedef const char *		ccp;

// error messages
typedef enum enumError
{
	ERR_OK,
	ERR_WARNING,
	ERR_WDISC_NOT_FOUND,
	ERR_FATAL

} enumError;

// debugging helpers
#define TRACE(...)
#define noTRACE(...)
#define ASSERT(cond)
#define DASSERT(cond)
#define TRACE_HEXDUMP(...)
#define TRACE_HEXDUMP16(...)

// SHA1 not supported
#define SHA1 wbfs_sha1_fake

// error messages
#define wbfs_warning(...) wbfs_print_error(__FUNCTION__,__FILE__,__LINE__,0,__VA_ARGS__)
#define wbfs_error(...)   wbfs_print_error(__FUNCTION__,__FILE__,__LINE__,1,__VA_ARGS__)
#define wbfs_fatal(...)   wbfs_print_error(__FUNCTION__,__FILE__,__LINE__,2,__VA_ARGS__)
#define OUT_OF_MEMORY wbfs_fatal("Out of memory!")

// alloc and free memory space
#define wbfs_malloc(x) malloc(x)
#define wbfs_free(x) free(x)

// alloc and free memory space suitable for disk io
#define wbfs_ioalloc(x) malloc(x)
#define wbfs_iofree(x) free(x)

// endianess functions
#define wbfs_ntohl(x) ntohl(x)
#define wbfs_ntohs(x) ntohs(x)
#define wbfs_htonl(x) htonl(x)
#define wbfs_htons(x) htons(x)

// memory functions
#define wbfs_memcmp(x,y,z) memcmp(x,y,z)
#define wbfs_memcpy(x,y,z) memcpy(x,y,z)
#define wbfs_memset(x,y,z) memset(x,y,z)

// time function
#define wbfs_time() ((u64)time(0))

#endif // LIBWBFS_OS_H
