#ifndef LIBWBFS_OS_H
#define LIBWBFS_OS_H

// system includes
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// project dependent includes
#include "types.h"
#include "debug.h"
#include "lib-error.h"

// error messages
#define wbfs_fatal(...) PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_FATAL,__VA_ARGS__)
#define wbfs_error(...) PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_WBFS,__VA_ARGS__)
#define wbfs_warning(...) PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_WARNING,__VA_ARGS__)

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
