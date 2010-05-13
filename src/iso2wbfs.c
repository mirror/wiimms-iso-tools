/***************************************************************************
 *                                                                         *
 *   Copyright (c) 2009-2010 by Dirk Clemens <develop@cle-mens.de>         *
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

///////////////////////////////////////////////////////////////////////////////

#define NAME "iso2wbfs"
#define TITLE NAME " v" VERSION " r" REVISION " " SYSTEM " - " AUTHOR " - " DATE

int  testmode		= 0;
ccp  dest		= 0;
bool opt_mkdir		= false;
int  opt_preserve	= 0;
int  opt_split		= 1;
u64  opt_split_size	= 0;
bool overwrite		= false;

//
///////////////////////////////////////////////////////////////////////////////

enum // const for long options without a short brothers
{
	GETOPT_BASE	= 0x1000,
	GETOPT_PSEL,
	GETOPT_RAW,
	GETOPT_IO,
};

char short_opt[] = "hVqvPtd:D:pzZ:o";
struct option long_opt[] =
{
	{ "help",	0, 0, 'h' },
	{ "version",	0, 0, 'V' },
	{ "quiet",	0, 0, 'q' },
	{ "verbose",	0, 0, 'v' },
	{ "progress",	0, 0, 'P' },
	{ "test",	0, 0, 't' },
	{ "psel",	1, 0, GETOPT_PSEL },
	{ "raw",	0, 0, GETOPT_RAW },
	{ "dest",	1, 0, 'd' },
	{ "DEST",	1, 0, 'D' },
	{ "preserve",	0, 0, 'p' },
	{ "split",	0, 0, 'z' },
	{ "split-size",	1, 0, 'Z' },
	 { "splitsize",	1, 0, 'Z' },
	{ "overwrite",	0, 0, 'o' },

	{ "io",		1, 0, GETOPT_IO },	// [2do] hidden option for tests

	{0,0,0,0}
};

///////////////////////////////////////////////////////////////////////////////

static const char help_text[] =
    "\n"
    TITLE "\n"
    "This tool converts files into WBFS files and split at 2 gb.\n"
    "\n"
    "Syntax: " NAME " [option]... files...\n"
    "\n"
    "Options:\n"
    "\n"
    "   -h --help            Print this help and exit.\n"
    "   -V --version         Print program name and version and exit.\n"
    "   -q --quiet           Be quiet   -> print only error messages.\n"
    "   -v --verbose         Be verbose -> program name and processed files.\n"
    "   -P --progress        Print progress counter independent of verbose level.\n"
    "   -t --test            Run in test mode, don't create a WBFS file.\n"
    "\n"
    "      --psel  p-type    Partition selector: (no-)data|update|channel all(=def) raw.\n"
    "      --raw             Short cut for --psel=raw.\n"
    "   -d --dest  path      Define a destination directory/file.\n"
    "   -D --DEST  path      Like --dest, but create directory path automatically.\n"
    "   -p --preserve        Preserve file times (atime+mtime).\n"
//  "   -z --split           Enable output file splitting, default split size = 4 gb.\n"
    "   -Z --split-size size Enable output file splitting and set split size.\n"
    "   -o --overwrite       Overwrite already existing files.\n"
 #ifdef TEST // [test]
    "\n"
    "      --io flags        IO mode (0=open or 1=fopen) &1=WBFS &2=IMAGE &4=other.\n"
 #endif
    "\n";

///////////////////////////////////////////////////////////////////////////////

void help_exit()
{
    fputs(help_text,stdout);
    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

void version_exit()
{
    printf("%s\n",TITLE);
    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

void hint_exit ( int stat )
{
    fprintf(stderr,
	    "-> Type '%s -h' or %s --help for more help.\n\n",
	    progname, progname );
    exit(stat);
}

///////////////////////////////////////////////////////////////////////////////

enumError CheckOptions ( int argc, char ** argv )
{
    int err = 0;
    for(;;)
    {
	const int opt_stat = getopt_long(argc,argv,short_opt,long_opt,0);
	if ( opt_stat == -1 )
	    break;

	noTRACE("CHECK OPTION %02x\n",opt_stat);
	switch (opt_stat)
	{
	  case '?': err++; break;
	  case 'V': version_exit();
	  case 'h': help_exit();
	  case 'q': verbose = verbose > -1 ? -1 : verbose - 1; break;
	  case 'v': verbose = verbose <  0 ?  0 : verbose + 1; break;
	  case 'P': progress++; break;
	  case 't': testmode++; break;
	  case 'd': dest = optarg; break;
	  case 'D': dest = optarg; opt_mkdir = true; break;
	  case 'p': opt_preserve++; break;
   	  case 'z': opt_split++; break;
	  case 'o': overwrite = true; break;

	  case GETOPT_PSEL:
	    {
		const int new_psel = ScanPartitionSelector(optarg);
		if ( new_psel == -1 )
		    err++;
		else
		    partition_selector = new_psel;
	    }
	    break;

	  case GETOPT_RAW:
	    partition_selector = WHOLE_DISC;
	    break;

	  case 'Z':
	    if (ScanSizeOptU64(&opt_split_size,optarg,GiB,0,
				"split-size",MIN_SPLIT_SIZE,0,DEF_SPLIT_FACTOR_ISO,0,true))
		hint_exit(ERR_SYNTAX);
	    opt_split++;
	    break;

	  case GETOPT_IO:
	    {
		const enumIOMode new_io = strtol(optarg,0,0); // [2do] error handling
		opt_iomode = new_io & IOM__IS_MASK;
		if ( verbose > 0 || opt_iomode != new_io )
		    printf("IO mode set to %#0x.\n",opt_iomode);
		opt_iomode |= IOM_FORCE_STREAM;
	    }
	    break;

	  default:
	    ERROR0(ERR_INTERNAL,"Internal error: unhandled option: '%c'\n",opt_stat);
	    ASSERT(0); // line never reached
	    break;
	}
    }
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
