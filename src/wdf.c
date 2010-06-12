/***************************************************************************
 *                                                                         *
 *   Copyright (c) 2009-2010 by Dirk Clemens <wiimm@wiimm.de>              *
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

#define _GNU_SOURCE 1

//#include <sys/types.h>
//#include <sys/stat.h>

//#include <fcntl.h>
//#include <unistd.h>
//#include <getopt.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stdarg.h>
//#include <limits.h>
#include <ctype.h>

#include "debug.h"
#include "version.h"
#include "types.h"
#include "lib-std.h"
#include "lib-sf.h"

#include "ui-wdf.c"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    variables			///////////////
///////////////////////////////////////////////////////////////////////////////

#define NAME "wdf"
#define TITLE NAME " v" VERSION " r" REVISION " " SYSTEM " - " AUTHOR " - " DATE

///////////////////////////////////////////////////////////////////////////////

typedef enum JobMode
{
	JMODE_PACK,
	JMODE_UNPACK,
	JMODE_CAT,
	JMODE_CMP,
	JMODE_DUMP
} JobMode;

ccp job_mode_name[] = { "pack", "unpack", "cat", "cmp", "dump", 0 };

typedef enum FileMode { FMODE_WDF, FMODE_CISO, FMODE_WBI } FileMode;
ccp file_mode_name[] = { "wdf", "ciso", "wbi", 0 };

JobMode  job_mode	= JMODE_PACK;
JobMode  def_job_mode	= JMODE_PACK;
FileMode file_mode	= FMODE_WDF;
FileMode def_file_mode	= FMODE_WDF;

bool opt_stdout		= false;
ccp  suffix		= 0;
bool opt_chunk		= false;
int  long_count		= 0;
ccp  opt_dest		= 0;
bool opt_mkdir		= false;
bool opt_preserve	= false;
bool opt_overwrite	= false;
bool opt_keep		= false;
int  testmode		= 0;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    help()			///////////////
///////////////////////////////////////////////////////////////////////////////

void help_exit()
{
    char buf[100];
    snprintf( buf, sizeof(buf),
		"Default options for '%s': --%s --%s\n\n",
		progname,
		job_mode_name[def_job_mode],
		file_mode_name[def_file_mode] );

    PrintHelpCmd(&InfoUI,stdout,0,0,buf);
    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

void version_exit()
{
    printf("%s\n",TITLE);
    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

void hint_exit ( enumError stat )
{
    fprintf(stderr,
	    "-> Type '%s -h' (or better '%s -h|less') for more help.\n\n",
	    progname, progname );
    exit(stat);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			CheckOptions()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CheckOptions ( int argc, char ** argv )
{
    int err = 0;
    for(;;)
    {
      const int opt_stat = getopt_long(argc,argv,OptionShort,OptionLong,0);
      if ( opt_stat == -1 )
	break;

      RegisterOption(&InfoUI,opt_stat,1,false);

      switch ((enumGetOpt)opt_stat)
      {
	case GO__ERR:		err++; break;

	case GO_VERSION:	version_exit();
	case GO_HELP:
	case GO_XHELP:		help_exit();
	case GO_QUIET:		verbose = verbose > -1 ? -1 : verbose - 1; break;
	case GO_VERBOSE:	verbose = verbose <  0 ?  0 : verbose + 1; break;
	case GO_CHUNK:		opt_chunk = true; break;
	case GO_LONG:		opt_chunk = true; long_count++; break;
	case GO_IO:		ScanIOMode(optarg); break;

	case GO_PACK:		job_mode = JMODE_PACK; break;
	case GO_UNPACK:		job_mode = JMODE_UNPACK; break;
	case GO_CAT:		job_mode = JMODE_CAT; break;
	case GO_CMP:		job_mode = JMODE_CMP; break;
	case GO_DUMP:		job_mode = JMODE_DUMP; break;
	case GO_STDOUT:		opt_stdout = true; break;

	case GO_WDF:		file_mode = FMODE_WDF; break;
	case GO_CISO:		file_mode = FMODE_CISO; break;
	case GO_WBI:		file_mode = FMODE_WBI; break;
	case GO_SUFFIX:		suffix = optarg; break;

	case GO_DEST:		opt_dest = optarg; break;
	case GO_DEST2:		opt_dest = optarg; opt_mkdir = true; break;
	case GO_SPLIT:		opt_split++; break;
	case GO_SPLIT_SIZE:	err += ScanSplitSize(optarg); break;
	case GO_CHUNK_MODE:	err += ScanChunkMode(optarg); break;
	case GO_CHUNK_SIZE:	err += ScanChunkSize(optarg); break;
	case GO_MAX_CHUNKS:	err += ScanMaxChunks(optarg); break;
	case GO_PRESERVE:	opt_preserve = true; break;
	case GO_OVERWRITE:	opt_preserve = true; break;
	case GO_KEEP:		opt_keep = true; break;

	case GO_TEST:		testmode++; break;
      }
    }
 #ifdef DEBUG
    DumpUsedOptions(&InfoUI,TRACE_FILE,11);
 #endif

    return err ? ERR_SYNTAX : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  job pack			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError job_pack ( int argc, char ** argv )
{
    // [2do]
    while ( argc-- > 0 )
	printf("pack %s\n",*argv++);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  job unpack			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError job_unpack ( int argc, char ** argv )
{
    // [2do]
    while ( argc-- > 0 )
	printf("unpack %s\n",*argv++);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  job cat			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError job_cat ( int argc, char ** argv )
{
    // [2do]
    while ( argc-- > 0 )
	printf("cat %s\n",*argv++);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   job cmp			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError job_cmp ( int argc, char ** argv )
{
    // [2do]
    while ( argc-- > 0 )
	printf("cmp %s\n",*argv++);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   job dump			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError job_dump ( int argc, char ** argv )
{
    // [2do]
    while ( argc-- > 0 )
	printf("dump %s\n",*argv++);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    main()			///////////////
///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    SetupLib(argc,argv,NAME,PROG_WDF);
    opt_chunk_mode = CHUNK_MODE_POW2;

    //----- setup default options

    if (progname)
    {
	ccp src = progname;
	char *dest = iobuf, *end = iobuf + sizeof(iobuf) - 1;
	while ( *src && dest < end )
	    *dest++ = tolower((int)*src++);
	*dest = 0;
	TRACE("PROGNAME/LOWER: %s\n",iobuf);

	if ( strstr(iobuf,"dump") )
	    job_mode = def_job_mode = JMODE_DUMP;
	else if ( strstr(iobuf,"cmp") || strstr(iobuf,"diff") )
	    job_mode = def_job_mode = JMODE_CMP;
	else if ( strstr(iobuf,"cat") )
	    job_mode = def_job_mode = JMODE_CAT;
	else if ( !memcmp(iobuf,"un",2) )
	    job_mode = def_job_mode = JMODE_UNPACK;

	if (strstr(iobuf,"ciso"))
	    file_mode = def_file_mode = FMODE_CISO;
	else if (strstr(iobuf,"wbi"))
	    file_mode = def_file_mode = FMODE_WBI;
    }


    //----- process arguments

    if ( argc < 2 )
    {
	printf("\n%s\nVisit %s%s for more info.\n\n",TITLE,URI_HOME,NAME);
	hint_exit(ERR_OK);
    }

    if (CheckOptions(argc,argv))
	hint_exit(ERR_SYNTAX);


    //----- job selector

    argc -= optind;
    argv += optind;
    int err = ERR_FATAL;
    switch (job_mode)
    {
	case JMODE_PACK:	err = job_pack(argc,argv); break;
	case JMODE_UNPACK:	err = job_unpack(argc,argv); break;
	case JMODE_CAT:		err = job_cat(argc,argv); break;
	case JMODE_CMP:		err = job_cmp(argc,argv); break;
	case JMODE_DUMP:	err = job_dump(argc,argv); break;
    }
    

    //----- terminate

    if (SIGINT_level)
	ERROR0(ERR_INTERRUPT,"Program interrupted by user.");
    return err > max_error ? err : max_error;
}

///////////////////////////////////////////////////////////////////////////////
