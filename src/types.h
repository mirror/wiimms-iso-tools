
#ifndef WWT_TYPES_H
#define WWT_TYPES_H 1

#include <types.h>
#include <limits.h>

///////////////////////////////////////////////////////////////////////////////

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

typedef signed char		s8;
typedef signed short		s16;
typedef signed int		s32;
typedef signed long long	s64;

///////////////////////////////////////////////////////////////////////////////

typedef unsigned char		uchar;
typedef unsigned int		uint;
typedef unsigned long		ulong;

typedef const char *		ccp;

typedef enum bool { false, true } __attribute__ ((packed)) bool;

///////////////////////////////////////////////////////////////////////////////

#endif // WWT_TYPES_H

