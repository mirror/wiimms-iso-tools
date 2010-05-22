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

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#include "debug.h"
#include "version.h"
#include "wiidisc.h"
#include "types.h"
#include "lib-sf.h"
#include "lib-std.h"
#include "wbfs-interface.h"

#include "ui-iso2wbfs.c"

//
///////////////////////////////////////////////////////////////////////////////

#define NAME "iso2wbfs"
#define TITLE NAME " v" VERSION " r" REVISION " " SYSTEM " - " AUTHOR " - " DATE

int  testmode		= 0;
ccp  dest		= 0;
bool opt_mkdir		= false;
int  opt_preserve	= 0;
bool overwrite		= false;

///////////////////////////////////////////////////////////////////////////////

void help_exit()
{
    PrintHelpCmd(&InfoUI,stdout,0,0);
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
	case GO_PROGRESS:	progress++; break;
	case GO_IO:		ScanIOMode(optarg); break;

	case GO_TEST:		testmode++; break;

	case GO_DEST:		dest = optarg; break;
	case GO_DEST2:		dest = optarg; opt_mkdir = true; break;
	case GO_PRESERVE:	opt_preserve++; break;
	case GO_SPLIT:		opt_split++; break;
	case GO_SPLIT_SIZE:	err += ScanSplitSize(optarg); break;
	case GO_OVERWRITE:	overwrite = true; break;

	case GO_PSEL:
	    {
		const int new_psel = ScanPartitionSelector(optarg);
		if ( new_psel == -1 )
		    err++;
		else
		    partition_selector = new_psel;
	    }
	    break;

	case GO_RAW:
	    partition_selector = WHOLE_DISC;
	    break;
      }
    }
 #ifdef DEBUG
    DumpUsedOptions(&InfoUI,TRACE_FILE,11);
 #endif

    return err ? ERR_SYNTAX : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError iso2wbfs ( ccp fname )
{
    SuperFile_t fi, fo;
    InitializeSF(&fi);
    InitializeSF(&fo);

    enumError err = OpenSF(&fi,fname,true,false);
    if (err)
	return err;
    const bool raw_mode = partition_selector == WHOLE_DISC || !fi.f.id6[0];

    ccp outname = fi.f.id6[0] ? fi.f.id6 : fi.f.outname ? fi.f.outname : fname;
    GenImageFileName(&fo.f,dest,outname,OFT_WBFS);
    outname = fo.f.rename ? fo.f.rename : fo.f.fname;
    fo.f.create_directory = opt_mkdir;

    if (testmode)
    {
	fprintf( fo.f.is_stdfile ? stderr : stdout,
			"%s: WOULD %s %s -> %s\n",
			progname, raw_mode ? "COPY: " : "SCRUB:", fname, outname );
	ResetSF(&fo,0);
	return ERR_OK;
    }

    if ( verbose >= 0 )
	fprintf( fo.f.is_stdfile ? stderr : stdout,
			"* %s: %s %s -> %s\n",
			progname, raw_mode ? "COPY " : "SCRUB", fname, outname );

    err = CreateFile(&fo.f,0,IOM_IS_IMAGE,overwrite);
    if (err)
	goto abort;

    if (opt_split)
	SetupSplitFile(&fo.f,OFT_WBFS,opt_split_size);

    fo.file_size = fi.file_size;
    err = SetupWriteSF(&fo,OFT_WBFS);
    if (err)
	goto abort;

    fo.indent		= 5;
    fo.show_progress	= verbose > 1 || progress;
    fo.show_summary	= verbose > 0 || progress;
    fo.show_msec	= verbose > 2;

    err =  CopySF(&fi,&fo, raw_mode ? WHOLE_DISC : partition_selector );
    if (err)
	goto abort;

    err = ResetSF( &fo, opt_preserve ? &fi.f.fatt : 0 );
    if (err)
	goto abort;

    ResetSF(&fi,0);
    return ERR_OK;

 abort:
    ResetSF(&fi,0);
    RemoveSF(&fo);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    SetupLib(argc,argv,NAME,PROG_ISO2WBFS);
    opt_split = 1;

    //----- process arguments

    if ( argc < 2 )
    {
	printf("\n%s\nVisit %s%s for more info.\n\n",TITLE,URI_HOME,NAME);
	hint_exit(ERR_OK);
    }

    if (CheckOptions(argc,argv))
	hint_exit(ERR_SYNTAX);

    if ( verbose > 0 )
	printf("\n%s\n\n",TITLE);

    int i, max_err = ERR_OK;
    for ( i = optind; i < argc && !SIGINT_level; i++ )
    {
	const int last_err = iso2wbfs(argv[i]);
	if ( max_err < last_err )
	    max_err = last_err;
    }

    if (SIGINT_level)
	ERROR0(ERR_INTERRUPT,"Program interrupted by user.");
    return max_err > max_error ? max_err : max_error;
}

///////////////////////////////////////////////////////////////////////////////
