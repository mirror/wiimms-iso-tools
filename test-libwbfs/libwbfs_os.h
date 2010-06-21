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
