
/***************************************************************************
 *                    __            __ _ ___________                       *
 *                    \ \          / /| |____   ____|                      *
 *                     \ \        / / | |    | |                           *
 *                      \ \  /\  / /  | |    | |                           *
 *                       \ \/  \/ /   | |    | |                           *
 *                        \  /\  /    | |    | |                           *
 *                         \/  \/     |_|    |_|                           *
 *                                                                         *
 *                           Wiimms ISO Tools                              *
 *                         http://wit.wiimm.de/                            *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project.                                 *
 *   Visit http://wit.wiimm.de/ for project details and sources.           *
 *                                                                         *
 *   Copyright (c) 2009-2011 by Dirk Clemens <wiimm@wiimm.de>              *
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

#ifndef WIT_DEBUG_H
#define WIT_DEBUG_H 1

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  -------------------
//   DEBUG and TRACING
//  -------------------
//
//  If symbol 'DEBUG' or symbol _DEBUG' is defined, then and only then
//  DEBUG and TRACING is enabled.
//
//  There are to function like macros defined:
//
//     TRACE ( const char * format, ... );
//        Print to console only if TRACING is enabled.
//        Ignored when TRACING is disabled.
//        It works like the well known printf() function and include flushing.
//
//      TRACE_IF ( bool condition, const char * format, ... );
//        Like TRACE(), but print only if 'condition' is true.
//
//      TRACELINE
//        Print out current line and source.
//
//      TRACE_SIZEOF ( object_or_type );
//        Print out `sizeof(object_or_type)´
//
//      ASSERT(cond);
//	  If condition is false: print info and exit program.
//
//
//  There are more macros with a preceding 'no' defined:
//
//      noTRACE ( const char * format, ... );
//      noTRACE_IF ( bool condition, const char * format, ... );
//      noTRACELINE ( bool condition, const char * format, ... );
//      noTRACE_SIZEOF ( object_or_type );
//      noASSERT(cond);
//
//      If you add a 'no' before a TRACE-Call it is disabled always.
//      This makes the usage and disabling of multi lines traces very easy.
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdarg.h>

///////////////////////////////////////////////////////////////////////////////

#if defined(RELEASE)
    #undef TESTTRACE
    #undef DEBUG
    #undef _DEBUG
    #undef DEBUG_ASSERT
    #undef _DEBUG_ASSERT
    #undef WAIT_ENABLED
#endif

///////////////////////////////////////////////////////////////////////////////

#if defined(IGNORE_DEBUG)
    #undef DEBUG
    #undef _DEBUG
#endif

///////////////////////////////////////////////////////////////////////////////

#undef TRACE
#undef TRACE_IF
#undef TRACELINE
#undef TRACE_SIZEOF

///////////////////////////////////////////////////////////////////////////////

extern FILE * TRACE_FILE;
void TRACE_FUNC ( const char * format, ...);
void TRACE_ARG_FUNC ( const char * format, va_list arg );
void PRINT_FUNC ( const char * format, ...);
void PRINT_ARG_FUNC ( const char * format, va_list arg );
void WAIT_FUNC ( const char * format, ...);
void WAIT_ARG_FUNC ( const char * format, va_list arg );

///////////////////////////////////////////////////////////////////////////////

#ifdef TESTTRACE

    #undef DEBUG
    #define DEBUG 1

    #undef TEST
    #define TEST 1

    #undef WAIT_ENABLED
    #define WAIT_ENABLED 1

    #undef NEW_FEATURES
    #define NEW_FEATURES 1

    #define TRACE_FUNC printf
    #define PRINT_FUNC printf
    #define WAIT_FUNC  printf

    #define TRACE_ARG_FUNC vprintf
    #define PRINT_ARG_FUNC vprintf
    #define WAIT_ARG_FUNC  vprintf

#endif

///////////////////////////////////////////////////////////////////////////////

#if defined(DEBUG) || defined(_DEBUG)

    #define HAVE_TRACE 1

    #undef DEBUG
    #define DEBUG 1

    #undef DEBUG_ASSERT
    #define DEBUG_ASSERT 1

    #define TRACE(...) TRACE_FUNC(__VA_ARGS__)
    #define TRACE_IF(cond,...) if (cond) TRACE_FUNC(__VA_ARGS__)
    #define TRACELINE TRACE_FUNC("line #%d @ %s\n",__LINE__,__FILE__)
    #define TRACE_SIZEOF(t) TRACE_FUNC("%7zd ==%6zx/hex == sizeof(%s)\n",sizeof(t),sizeof(t),#t)

    #define HEXDUMP(i,a,af,rl,d,c) HexDump(stdout,i,a,af,rl,d,c);
    #define HEXDUMP16(i,a,d,c) HexDump16(stdout,i,a,d,c);
    #define TRACE_HEXDUMP(i,a,af,rl,d,c) HexDump(TRACE_FILE,i,a,af,rl,d,c);
    #define TRACE_HEXDUMP16(i,a,d,c) HexDump16(TRACE_FILE,i,a,d,c);

#else

    #define HAVE_TRACE 0

    #undef DEBUG

    #define TRACE(...)
    #define TRACE_IF(cond,...)
    #define TRACELINE
    #define TRACE_SIZEOF(t)
    #define HEXDUMP(i,a,af,rl,d,c)
    #define HEXDUMP16(a,i,d,c)
    #define TRACE_HEXDUMP(i,a,af,rl,d,c)
    #define TRACE_HEXDUMP16(i,a,d,c)

#endif

///////////////////////////////////////////////////////////////////////////////

#undef ASSERT
#undef ASSERT_MSG

#if defined(DEBUG_ASSERT) || defined(_DEBUG_ASSERT)

    #define HAVE_ASSERT 1

    #undef DEBUG_ASSERT
    #define DEBUG_ASSERT 1

    #define ASSERT(a) if (!(a)) ERROR0(ERR_FATAL,"ASSERTION FAILED !!!\n")
    #define ASSERT_MSG(a,...) if (!(a)) ERROR0(ERR_FATAL,__VA_ARGS__)

#else

    #define HAVE_ASSERT 0

    #undef DEBUG_ASSERT
    #define ASSERT(cond)
    #define ASSERT_MSG(a,...)

#endif

///////////////////////////////////////////////////////////////////////////////

#undef PRINT
#undef PRINT_IF
#undef BINGO

#if defined(DEBUG) && defined(TEST)

    #define HAVE_PRINT 1

    #define PRINT(...) PRINT_FUNC(__VA_ARGS__)
    #define PRINT_IF(cond,...) if (cond) PRINT_FUNC(__VA_ARGS__)
    #define BINGO PRINT_FUNC("BINGO! %s() #%d @ %s\n",__FUNCTION__,__LINE__,__FILE__)

#else

    #define HAVE_PRINT 0

    #define PRINT	TRACE
    #define PRINT_IF	TRACE_IF
    #define BINGO	TRACELINE

#endif

///////////////////////////////////////////////////////////////////////////////

#undef WAIT
#undef WAIT_IF

#if defined(WAIT_ENABLED)

    #define HAVE_WAIT 1

    #define WAIT(...) WAIT_FUNC(__VA_ARGS__)
    #define WAIT_IF(cond,...) if (cond) WAIT_FUNC(__VA_ARGS__)

#else

    #define HAVE_WAIT 0

    #define WAIT(...)
    #define WAIT_IF(cond,...)

#endif

///////////////////////////////////////////////////////////////////////////////

#undef DASSERT
#undef DASSERT_MSG

#if defined(DEBUG) || defined(TEST)

    #define HAVE_DASSERT 1

    #define DASSERT ASSERT
    #define DASSERT_MSG ASSERT_MSG

#else

    #define HAVE_DASSERT 0

    #define DASSERT(cond)
    #define DASSERT_MSG(a,...)

#endif

///////////////////////////////////////////////////////////////////////////////

// prefix 'no' deactivates traces

#undef noTRACE
#undef noTRACE_IF
#undef noTRACELINE
#undef noTRACE_SIZEOF
#undef noPRINT
#undef noPRINT_IF
#undef noWAIT
#undef noWAIT_IF
#undef noASSERT


#ifdef TESTTRACE

    #define noTRACE		TRACE
    #define noTRACE_IF		TRACE_IF
    #define noTRACELINE		TRACELINE
    #define noTRACE_SIZEOF	TRACE_SIZEOF
    #define noPRINT		PRINT
    #define noPRINT_IF		PRINT_IF
    #define noWAIT		WAIT
    #define noWAIT_IF		WAIT_IF
    #define noASSERT		ASSERT
    #define noASSERT_MSG	ASSERT_MSG

#else

    #define noTRACE(...)
    #define noTRACE_IF(cond,...)
    #define noTRACELINE
    #define noTRACE_SIZEOF(t)
    #define noPRINT(...)
    #define noPRINT_IF(...)
    #define noWAIT(...)
    #define noWAIT_IF(...)
    #define noASSERT(cond)
    #define noASSERT_MSG(cond,...)

#endif

///////////////////////////////////////////////////////////////////////////////

#endif // WIT_DEBUG_H
