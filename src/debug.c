
/***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project.                                 *
 *   Visit http://wit.wiimm.de/ for project details and sources.           *
 *                                                                         *
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

#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 trace functions		///////////////
///////////////////////////////////////////////////////////////////////////////

FILE * TRACE_FILE = 0;

unsigned GetTimerMSec();

///////////////////////////////////////////////////////////////////////////////

static void trace_helper ( int print_stderr, const char * format, va_list arg )
{
    if (!TRACE_FILE)
    {
	TRACE_FILE = fopen("_trace.tmp","wb");
	if (!TRACE_FILE)
	    TRACE_FILE = stderr;
    }

    unsigned msec = GetTimerMSec();

    if ( print_stderr || TRACE_FILE == stderr )
    {
	fflush(stdout);
	fprintf(stderr,"%4d.%03d  ",msec/1000,msec%1000);
	vfprintf(stderr,format,arg);
	fflush(stderr);
    }

    if ( TRACE_FILE != stderr )
    {
	fprintf(TRACE_FILE,"%4d.%03d  ",msec/1000,msec%1000);
	vfprintf(TRACE_FILE,format,arg);
	fflush(TRACE_FILE);
    }
}

///////////////////////////////////////////////////////////////////////////////

#undef TRACE_ARG_FUNC

void TRACE_ARG_FUNC ( const char * format, va_list arg )
{
    trace_helper(0,format,arg);
}

///////////////////////////////////////////////////////////////////////////////

#undef TRACE_FUNC

void TRACE_FUNC ( const char * format, ... )
{
    va_list arg;
    va_start(arg,format);
    trace_helper(0,format,arg);
    va_end(arg);
}

///////////////////////////////////////////////////////////////////////////////

#undef PRINT_ARG_FUNC

void PRINT_ARG_FUNC ( const char * format, va_list arg )
{
    trace_helper(1,format,arg);
}

///////////////////////////////////////////////////////////////////////////////

#undef PRINT_FUNC

void PRINT_FUNC ( const char * format, ... )
{
    va_list arg;
    va_start(arg,format);
    trace_helper(1,format,arg);
    va_end(arg);
}

///////////////////////////////////////////////////////////////////////////////

#undef WAIT_ARG_FUNC

void WAIT_ARG_FUNC ( const char * format, va_list arg )
{
    if ( format && *format )
	trace_helper(1,format,arg);
    PRINT_FUNC(">>>>>> PRESS RETURN: ");
    getchar();
}

///////////////////////////////////////////////////////////////////////////////

#undef WAIT_FUNC

void WAIT_FUNC ( const char * format, ... )
{
    va_list arg;
    va_start(arg,format);
    WAIT_ARG_FUNC(format,arg);
    va_end(arg);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////
