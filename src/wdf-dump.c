
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

#include "ui-wdf-dump.c"

//
///////////////////////////////////////////////////////////////////////////////

#define NAME "wdf-dump"
#define TITLE NAME " v" VERSION " r" REVISION " " SYSTEM " - " AUTHOR " - " DATE

bool print_chunk_tab = false;
int end_delta = 0;

///////////////////////////////////////////////////////////////////////////////

void help_exit()
{
    PrintHelpCmd(&InfoUI,stdout,0,0,0,0);
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

      RegisterOptionByName(&InfoUI,opt_stat,1,false);

      switch ((enumGetOpt)opt_stat)
      {
	case GO__ERR:		err++; break;

	case GO_VERSION:	version_exit();
	case GO_HELP:
	case GO_XHELP:		help_exit();
	case GO_WIDTH:		err += ScanOptWidth(optarg); break;
	case GO_QUIET:		verbose = verbose > -1 ? -1 : verbose - 1; break;
	case GO_VERBOSE:	verbose = verbose <  0 ?  0 : verbose + 1; break;
	case GO_IO:		ScanIOMode(optarg); break;

	case GO_CHUNK:
	case GO_LONG:		print_chunk_tab = true; break;
      }
    }
 #ifdef DEBUG
    DumpUsedOptions(&InfoUI,TRACE_FILE,11);
 #endif

    return err ? ERR_SYNTAX : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

static int error ( int err, int indent, ccp fname, ccp format, ... )
{
    fprintf(stdout,"%*s!!! %s: ",indent,"",progname);

    va_list arg;
    va_start(arg,format);
    vfprintf(stdout,format,arg);
    va_end(arg);
    fprintf(stdout,"\n\n");

    if (!isatty(fileno(stdout)))
    {
	fprintf(stderr,"%s: ",progname);

	va_start(arg,format);
	vfprintf(stderr,format,arg);
	va_end(arg);
	fprintf(stderr,"\n%s:  - file: %s\n",progname,fname);
    }

    return err;
}

///////////////////////////////////////////////////////////////////////////////

static u64 prev_val = 0;

static int print_range ( ccp text, u64 end, u64 len )
{
    const int err = end - prev_val != len;
    if (err)
	printf("    %-18s: %10llx .. %10llx [%10llx!=%10llx] INVALID!\n", \
		text, prev_val, end-end_delta, len, end-prev_val );
    else
	printf("    %-18s: %10llx .. %10llx [%10llx]\n", \
		text, prev_val, end-end_delta, len );

    prev_val = end;
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// ciso

enumError ciso_dump ( CISO_Head_t * ch, File_t *f, ccp fname )
{
    ASSERT(ch);
    ASSERT(f);
    ASSERT(fname);

    printf("\nCISO dump of file %s\n\n",fname);

    //----- collect data

    // block size is stored in untypical little endian
    u8 * bs = (u8*)&ch->block_size;
    const u32 block_size = ((( bs[3] << 8 ) | bs[2] ) << 8 | bs[1] ) << 8 | bs[0];

    int used_bl = 0;
    const u8 *bptr, *bend = ch->map + CISO_MAP_SIZE, *bmax = ch->map;
    for( bptr = ch->map; bptr < bend; bptr++ )
	if (*bptr)
	{
	    bmax = bptr;
	    used_bl++;
	}
    const int max_bl		= bmax - ch->map + 1;
    const u64 max_off		= (u64)used_bl * block_size + CISO_HEAD_SIZE;
    const u64 min_file_size	= (u64)WII_SECTORS_SINGLE_LAYER * WII_SECTOR_SIZE;
    const u64 iso_file_usage	= (u64)max_bl * block_size;
    const u64 iso_file_size	= iso_file_usage > min_file_size
				? iso_file_usage : min_file_size;

    //----- print header
    
    u8 * m = (u8*)ch->magic;
    printf("  %-18s:         \"%c%c%c%c\" = %02x-%02x-%02x-%02x\n",
		"Magic",
		m[0]>=' ' && m[0]<0x7f ? m[0] : '.',
		m[1]>=' ' && m[1]<0x7f ? m[1] : '.',
		m[2]>=' ' && m[2]<0x7f ? m[2] : '.',
		m[3]>=' ' && m[3]<0x7f ? m[3] : '.',
		m[0], m[1], m[2], m[3] );

    printf("  %-18s: %10x/hex =%11d\n","Used blocks",used_bl,used_bl);
    printf("  %-18s: %10x/hex =%11d\n","Max block",max_bl-1,max_bl-1);
    printf("  %-18s: %10x/hex =%11d\n","Block size",block_size,block_size);
    printf("  %-18s: %10llx/hex =%11lld\n","Real file usage",max_off,max_off);
    printf("  %-18s: %10llx/hex =%11lld\n","Real file size",(u64)f->st.st_size,(u64)f->st.st_size);
    printf("  %-18s: %10llx/hex =%11lld\n","ISO file usage",iso_file_usage,iso_file_usage);
    printf("  %-18s: %10llx/hex =%11lld\n","ISO file size",iso_file_size,iso_file_size);
    putchar('\n');

    //----- print mapping table

    if (print_chunk_tab)
    {
	printf("  Mapping Table:\n\n"
	       "       blocks    :       CISO file offset :     virtual ISO offset :      size\n"
	       "   ----------------------------------------------------------------------------\n");

	u32 bcount = 0;
	u64 bs64 = block_size;
	for( bptr = ch->map; ; )
	{
	    // find first block used
	    while ( bptr < bend && !*bptr )
		bptr++;
	    const int b1 = bptr - ch->map;

	    // find last block used
	    while ( bptr < bend && *bptr )
		bptr++;
	    const int b2 = bptr - ch->map;

	    if ( b1 >= b2 )
		break;

	    const u64 cfa = bcount * bs64 + CISO_HEAD_SIZE;
	    const u64 via = b1 * bs64;
	    const u64 siz = (b2-b1) * bs64;

	    if ( b1 == b2-1 )
		printf("%11x      : %9llx .. %9llx : %9llx .. %9llx : %9llx\n",
		    b1, cfa, cfa+siz, via, via+siz, siz );
	    else
		printf("%8x .. %4x : %9llx .. %9llx : %9llx .. %9llx : %9llx\n",
		    b1, b2-1, cfa, cfa+siz, via, via+siz, siz );

	    bcount += b2 - b1;
	}
	putchar('\n');
    }

    ResetFile(f,false);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// wdf

enumError wdf_dump ( ccp fname )
{
    ASSERT(fname);

    File_t f;
    InitializeFile(&f);
    enumError err = OpenFile(&f,fname,IOM_IS_IMAGE);
    if (err)
	return err;

    WDF_Head_t wh;
    const int CISO_MAGIC_SIZE = 4;
    ASSERT( CISO_MAGIC_SIZE < sizeof(wh) );
    err = ReadAtF(&f,0,&wh,CISO_MAGIC_SIZE);
    if (err)
    {
	ResetFile(&f,false);
	return err;
    }

    if (!memcmp(&wh,"CISO",CISO_MAGIC_SIZE))
    {
	CISO_Head_t ch;
	memcpy(&ch,&wh,CISO_MAGIC_SIZE);
	err = ReadAtF(&f,CISO_MAGIC_SIZE,(char*)&ch+CISO_MAGIC_SIZE,sizeof(ch)-CISO_MAGIC_SIZE);
	if (err)
	{
	    ResetFile(&f,false);
	    return err;
	}
	return ciso_dump(&ch,&f,fname);
    }

    printf("\nWDF dump of file %s\n",fname);

    err = ReadAtF(&f,CISO_MAGIC_SIZE,(char*)&wh+CISO_MAGIC_SIZE,sizeof(wh)-CISO_MAGIC_SIZE);
    if (err)
    {
	ResetFile(&f,false);
	return err;
    }
    
    ConvertToHostWH(&wh,&wh);
    AnalyzeWH(&f,&wh,false); // needed for splitting support!

    printf("\n  Header:\n\n");
    u8 * m = (u8*)wh.magic;
    printf("    %-18s: \"%c%c%c%c%c%c%c%c\"  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
		"Magic",
		m[0]>=' ' && m[0]<0x7f ? m[0] : '.',
		m[1]>=' ' && m[1]<0x7f ? m[1] : '.',
		m[2]>=' ' && m[2]<0x7f ? m[2] : '.',
		m[3]>=' ' && m[3]<0x7f ? m[3] : '.',
		m[4]>=' ' && m[4]<0x7f ? m[4] : '.',
		m[5]>=' ' && m[5]<0x7f ? m[5] : '.',
		m[6]>=' ' && m[6]<0x7f ? m[6] : '.',
		m[7]>=' ' && m[7]<0x7f ? m[7] : '.',
		m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7] );

    if (memcmp(wh.magic,WDF_MAGIC,WDF_MAGIC_SIZE))
    {
	ResetFile(&f,false);
	return error(ERR_WDF_INVALID,2,fname,"Wrong magic");
    }

    const int head_size = AdjustHeaderWDF(&wh);

    #undef PRINT32
    #undef PRINT64
    #undef RANGE
    #define PRINT32(a) printf("    %-18s: %10x/hex =%11d\n",#a,wh.a,wh.a)
    #define PRINT64(a) printf("    %-18s: %10llx/hex =%11lld\n",#a,wh.a,wh.a)

 #if WDF2_ENABLED
    PRINT32(wdf_version);
    PRINT32(wdf_compatible);
    PRINT32(wdf_head_size);
    PRINT32(align_factor);
 #else
    printf("    %-18s: %10x/hex =%11d%s\n",
		"wdf_version", wh.wdf_version, wh.wdf_version,
		 head_size == WDF_VERSION1_SIZE ? "" : "+" );
    printf("    %-18s: %10x/hex =%11d\n"," - header size",head_size,head_size);
 #endif

    PRINT32(split_file_id);
    PRINT32(split_file_index);
    PRINT32(split_file_num_of);
    PRINT64(file_size);
    printf("    %-18s: %10llx/hex =%11lld  %4.2f%%\n",
		" - WDF file size ",
		(u64)f.st.st_size, (u64)f.st.st_size,  100.0 * f.st.st_size / wh.file_size );
    PRINT64(data_size);
    PRINT32(chunk_split_file);
    PRINT32(chunk_n);
    PRINT64(chunk_off);

    //--------------------------------------------------

    printf("\n  File Parts:\n\n");

    int ec = 0; // error count
    prev_val = 0;

    const int chunk_size = wh.chunk_n*sizeof(WDF_Chunk_t);
    
    ec += print_range( "Header",	head_size,			head_size );
    ec += print_range( "Data",		wh.chunk_off,			wh.data_size );
    ec += print_range( "Chunk-Magic",	wh.chunk_off+WDF_MAGIC_SIZE,	WDF_MAGIC_SIZE );
    ec += print_range( "Chunk-Table",	f.st.st_size,			chunk_size );
    printf("\n");

    if (ec)
    {
	ResetFile(&f,false);
	return error(ERR_WDF_INVALID,2,fname,"Invalid data");
    }

    if ( wh.chunk_n > 1000000 )
    {
	ResetFile(&f,false);
	return error(ERR_INTERNAL,2,fname,"Too much chunk enties");
    }

    //--------------------------------------------------

    char magic[WDF_MAGIC_SIZE];
    err = ReadAtF(&f,wh.chunk_off,magic,sizeof(magic));
    if (err)
    {
	ResetFile(&f,false);
	return error(ERR_READ_FAILED,2,fname,"ReadF error");
    }

    if (memcmp(magic,WDF_MAGIC,WDF_MAGIC_SIZE))
    {
	ResetFile(&f,false);
	return error(ERR_WDF_INVALID,2,fname,"Wrong chunk table magic");
    }

    WDF_Chunk_t *w, *wc = malloc(chunk_size);
    if (!wc)
	OUT_OF_MEMORY;

    err = ReadF(&f,wc,chunk_size);
    if (err)
    {
	ResetFile(&f,false);
	free(wc);
	return error(ERR_READ_FAILED,2,fname,"ReadF error");
    }

    if (print_chunk_tab)
    {
	printf("  Chunk Table:\n\n"
	       "    idx        WDF file offset  data len     virtual ISO offset  hole size\n"
	       "   ------------------------------------------------------------------------\n");

	int idx;
	for ( idx = 0, w = wc; idx < wh.chunk_n; idx++, w++ )
	    ConvertToHostWC(w,w);

	for ( idx = 0, w = wc; idx < wh.chunk_n; idx++, w++ )
	{
	    printf("%6d. %10llx..%10llx %9llx %10llx..%10llx  %9llx\n",
		    idx,
		    w->data_off, w->data_off + w->data_size-end_delta,
		    w->data_size,
		    w->file_pos, w->file_pos + w->data_size - end_delta,
		    idx+1 < wh.chunk_n ? w[1].file_pos - w->file_pos - w->data_size : 0llu );
	}

	putchar('\n');
    }

    ResetFile(&f,false);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    SetupLib(argc,argv,NAME,PROG_WDF_DUMP);

    //----- process arguments

    if ( argc < 2 )
    {
	printf("\n%s\nVisit %s%s for more info.\n\n",TITLE,URI_HOME,NAME);
	hint_exit(ERR_OK);
    }

    if (CheckOptions(argc,argv))
	hint_exit(ERR_SYNTAX);

    if ( verbose >= 0 )
	printf("\n%s\n\n",TITLE);

    int i, max_err = ERR_OK;
    for ( i = optind; i < argc && !SIGINT_level; i++ )
    {
	const int last_err = wdf_dump(argv[i]);
	if ( max_err < last_err )
	    max_err = last_err;
    }

    CloseAll();

    if (SIGINT_level)
	ERROR0(ERR_INTERRUPT,"Program interrupted by user.");
    return max_err > max_error ? max_err : max_error;
}

///////////////////////////////////////////////////////////////////////////////
