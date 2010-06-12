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
#include "types.h"
#include "lib-sf.h"
#include "lib-std.h"

#include "ui-wdf-cat.c"

//
///////////////////////////////////////////////////////////////////////////////

#define NAME "wdf-cat"
#define TITLE NAME " v" VERSION " r" REVISION " " SYSTEM " - " AUTHOR " - " DATE

///////////////////////////////////////////////////////////////////////////////

void help_exit()
{
    PrintHelpCmd(&InfoUI,stdout,0,0,0);
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
	case GO_IO:		ScanIOMode(optarg); break;
      }
    }
 #ifdef DEBUG
    DumpUsedOptions(&InfoUI,TRACE_FILE,11);
 #endif

    return err ? ERR_SYNTAX : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError do_cat ( File_t * f, off_t src_off, u64 size64 )
{
    while ( size64 > 0 )
    {
	if ( SIGINT_level > 1 )
	    return ERR_INTERRUPT;

	u32 size = sizeof(iobuf);
	if ( size > size64 )
	    size = (u32)size64;

	int stat = ReadAtF(f,src_off,iobuf,size);
	if (stat)
	    return stat;

	size_t wstat = fwrite(iobuf,1,size,stdout);
	if ( wstat != size )
	    return ERR_WRITE_FAILED;

	src_off += size;
	size64  -= size;
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError do_zero ( u64 size64 )
{
    while ( size64 > 0 )
    {
	if ( SIGINT_level > 1 )
	    return ERR_INTERRUPT;

	u32 size = sizeof(zerobuf);
	if ( size > size64 )
	    size = (u32)size64;

	size_t wstat = fwrite(zerobuf,1,size,stdout);
	if ( wstat != size )
	    return ERR_WRITE_FAILED;

	size64 -= size;
    }
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// ciso

enumError ciso_cat ( CISO_Head_t * ch, SuperFile_t * sf )
{
    ASSERT(ch);
    ASSERT(sf);

    CISO_Info_t ci;
    if (InitializeCISO(&ci,ch))
    {
	TRACE("raw copy %llu bytes.\n",(u64)sf->f.st.st_size);
	const enumError stat = do_cat(&sf->f,0,sf->f.st.st_size);
	CloseSF(sf,0);
	return stat;
    }

    const u64 block_size = ci.block_size;
    ResetCISO(&ci);

    u32 zero_count = 0;
    off_t read_off = CISO_HEAD_SIZE;
    const u8 *bptr = ch->map, *blast = ch->map;
    const u8 *bend = bptr + CISO_MAP_SIZE;
    while ( bptr < bend )
	if (*bptr++)
	{
	    if (zero_count)
	    {
		do_zero(zero_count*block_size);
		zero_count = 0;
	    }

	    do_cat(&sf->f,read_off,block_size);
	    read_off += block_size; 
	    blast = bptr;
	}
	else
	    zero_count++;

    const off_t off = (off_t)block_size * ( blast - ch->map );
    const off_t min = (off_t)WII_SECTORS_SINGLE_LAYER * WII_SECTOR_SIZE;
    if ( off < min )
	do_zero(min-off);

    CloseSF(sf,0);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// wdf

enumError wdf_cat ( ccp fname )
{
    ASSERT(fname);

    SuperFile_t sf;
    InitializeSF(&sf);
    enumError stat = OpenFile(&sf.f,fname,IOM_IS_IMAGE);
    if (stat)
	return stat;

    stat = SetupReadWDF(&sf);
    if ( stat == ERR_NO_WDF )
    {
	CISO_Head_t ch;
	stat = ReadAtF(&sf.f,0,&ch,sizeof(ch));
	if (   !stat
	    && !memcmp(&ch.magic,"CISO",4)
	    && !*(ccp)&ch.block_size
	    && *ch.map == 1 )
	{
	    return ciso_cat(&ch,&sf);
	}

	// normal cat

	TRACE("raw copy %llu bytes.\n",(u64)sf.f.st.st_size);
	stat = do_cat(&sf.f,0,sf.f.st.st_size);
	CloseSF(&sf,0);
	return stat;
    }

    if (stat)
    {
	CloseSF(&sf,0);
	return stat;
    }

    int i;
    u64 last_off = 0;
    for ( i=0; i < sf.wc_used; i++ )
    {
	if ( SIGINT_level > 1 )
	{
	    stat = ERR_INTERRUPT;
	    break;
	}

	WDF_Chunk_t *wc = sf.wc + i;
	long long zero_count = wc->file_pos - last_off;
	if ( zero_count > 0 )
	{
	    TRACE("zero #%02d @%9llx [%9llx].\n",i,last_off,zero_count);
	    last_off += zero_count;
	    stat = do_zero(zero_count);
	    if (stat)
		return stat;
	}

	if ( wc->data_size )
	{
	    TRACE("copy #%02d @%9llx [%9llx] read-off=%9llx.\n",
		    i, last_off, wc->data_size, wc->data_off );
	    last_off += wc->data_size;
	    stat = do_cat(&sf.f,wc->data_off,wc->data_size);
	    if (stat)
		return stat;
	}
    }

    CloseSF(&sf,0);
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    SetupLib(argc,argv,NAME,PROG_WDF_CAT);

    //----- process arguments

    if ( argc < 2 )
    {
	printf("\n%s\nVisit %s%s for more info.\n\n",TITLE,URI_HOME,NAME);
	hint_exit(ERR_OK);
    }

    if (CheckOptions(argc,argv))
	hint_exit(ERR_SYNTAX);

    int i, max_err = ERR_OK;
    for ( i = optind; i < argc && !SIGINT_level; i++ )
    {
	const int last_err = wdf_cat(argv[i]);
	if ( max_err < last_err )
	    max_err = last_err;
    }

    if (SIGINT_level)
	ERROR0(ERR_INTERRUPT,"Program interrupted by user.");
    return max_err > max_error ? max_err : max_error;
}

///////////////////////////////////////////////////////////////////////////////
