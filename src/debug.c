
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#undef TRACE_ARG_FUNC
#undef TRACE_FUNC

FILE * TRACE_FILE = 0;

unsigned GetTimerMSec();

void TRACE_ARG_FUNC ( const char * format, va_list arg )
{
    if (!TRACE_FILE)
    {
	TRACE_FILE = fopen("_trace.tmp","wb");
	if (!TRACE_FILE)
	    TRACE_FILE = stderr;
    }

    unsigned msec = GetTimerMSec();
    fprintf(TRACE_FILE,"%4d.%03d  ",msec/1000,msec%1000);
    vfprintf(TRACE_FILE,format,arg);
    fflush(TRACE_FILE);
}

void TRACE_FUNC ( const char * format, ... )
{
    va_list arg;
    va_start(arg,format);
    TRACE_ARG_FUNC(format,arg);
    va_end(arg);
}

