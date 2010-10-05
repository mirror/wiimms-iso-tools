
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
#define TITLE " v" VERSION " r" REVISION " " SYSTEM " - " AUTHOR " - " DATE

///////////////////////////////////////////////////////////////////////////////

typedef enum FileMode { FMODE_WDF, FMODE_WIA, FMODE_CISO, FMODE_WBI } FileMode;
ccp file_mode_name[] = { ".wdf", ".wia", ".ciso", ".wbi", 0 };
enumOFT file_mode_oft[] = { OFT_WDF, OFT_WIA, OFT_CISO, OFT_CISO, 0 };

enumCommands the_cmd	= CMD_PACK;
FileMode file_mode	= FMODE_WDF;
FileMode def_file_mode	= FMODE_WDF;

bool opt_stdout		= false;
ccp  opt_suffix		= 0;
bool opt_chunk		= false;
int  opt_minus1		= 0;
bool opt_preserve	= false;
bool opt_overwrite	= false;
bool opt_keep		= false;

ccp progname0		= 0;
FILE * logout		= 0;

char progname_buf[20];
char default_settings[80];

enumOFT create_oft = OFT_PLAIN;
ccp extract_verb = "extract";
ccp copy_verb	 = "copy   ";

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    help()			///////////////
///////////////////////////////////////////////////////////////////////////////

void print_title ( FILE * f )
{
    static bool done = false;
    if (!done)
    {
	done = true;
	if ( verbose >= 1 && f == stdout )
	    fprintf(f,"\n%s%s\n\n",progname,TITLE);
	else
	    fprintf(f,"*****  %s%s  *****\n",progname,TITLE);
	fflush(f);
    }
}

///////////////////////////////////////////////////////////////////////////////

void help_exit ( bool xmode )
{
    printf("%s%s\n",progname,TITLE);

    if (xmode)
    {
	int cmd;
	for ( cmd = 0; cmd < CMD__N; cmd++ )
	    PrintHelpCmd(&InfoUI,stdout,0,cmd, 0, cmd ? 0 : default_settings );
    }
    else
	PrintHelpCmd(&InfoUI,stdout,0,0,"+HELP",default_settings);

    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

void version_exit()
{
    printf("%s%s\n%.*s",
	progname, TITLE,
	(int)strlen(default_settings)-1, default_settings );
    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

static void hint_exit ( enumError stat )
{
    fprintf(stderr,
	    "-> Type '%s -h' (or better '%s -h|less') for more help.\n\n",
	    progname, progname );
    exit(stat);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError OpenOutput
    ( SuperFile_t * fo, SuperFile_t * fi, ccp fname, ccp testmode_msg )
{
    PRINT("OpenOutput(%p,%p,%s,%s)\n",fo,fi,fname,testmode_msg);
    DASSERT(fo);

    if ( verbose > 0 )
	print_title(logout);

    if (!fname)
	fname = opt_dest;

    if (testmode)
    {
	if (testmode_msg)
	    fprintf(logout,"%s: %s\n",progname,testmode_msg);
	fname = 0;
    }

    if (!fname)
	fname = "-";

    fo->f.create_directory = opt_mkdir;
    DASSERT( !fi || IsOpenSF(fi) );
    if ( fi && fi->iod.oft == OFT_UNKNOWN )
    {
	SetupIOD(fi,OFT_PLAIN,OFT_PLAIN);
	fi->file_size = fi->f.st.st_size;
    }

    enumError err = CreateSF(fo,fname,create_oft,IOM_FORCE_STREAM,opt_overwrite,fi);
    if ( !err && opt_split && GetFileMode(fo->f.st.st_mode) == FM_PLAIN )
	err = SetupSplitFile(&fo->f,fo->iod.oft,opt_split_size);

    TRACE("OpenOutput() returns %d\n",err);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CloseInOut ( SuperFile_t * fi, SuperFile_t * fo,
			enumError prev_err, bool remove_source )
{
    TRACE("CloseInOut(%p,%p,%d,%d)\n",fi,fo,prev_err,remove_source);
    DASSERT(fi);

    if (fo)
    {
	const enumError err = ResetSF( fo, opt_preserve ? &fi->f.fatt : 0 );
	if ( prev_err < err )
	     prev_err = err;
    }

    if ( !prev_err && remove_source && !testmode )
	prev_err = RemoveSF(fi);
    else
    {
	const enumError err = ResetSF(fi,0);
	if ( prev_err < err )
	     prev_err = err;
    }
    
    TRACE("CloseInOut() returns %d\n",prev_err);
    return prev_err;
}

///////////////////////////////////////////////////////////////////////////////

static char * RemoveExt ( char *buf, size_t bufsize, ccp * ext_list, ccp fname )
{
    DASSERT( buf );
    DASSERT( bufsize > 10 );
    DASSERT( ext_list );
    DASSERT( fname );

    size_t flen = strlen(fname);
    while ( *ext_list )
    {
	ccp ext = *ext_list++;
	const size_t elen = strlen(ext);
	if ( flen > elen && !strcasecmp(fname+flen-elen,ext) )
	{
	    flen -= elen;
	    if ( flen > bufsize-1 )
		 flen = bufsize-1;
	    StringCopyS(buf,flen+1,fname);
	    return buf;
	}
    }
    return (char*)fname;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  cat helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CatRaw ( SuperFile_t * fi, SuperFile_t * fo,
			ccp out_fname, bool ignore_raw, bool remove_source )
{
    TRACELINE;
    DASSERT(fi);
    DASSERT( fo || out_fname );

    enumError err = ERR_OK;
    SuperFile_t fo_local;

    if ( !ignore_raw )
    {
	if (testmode)
	{
	    if (out_fname)
		fprintf(logout," - WOULD %s file %s -> %s\n",
			copy_verb, fi->f.fname, out_fname );
	    else
		fprintf(logout," - WOULD %s file %s\n",copy_verb,fi->f.fname);
	}
	else
	{
	    if (!fo)
	    {
		if ( verbose > 0 )
		    fprintf(logout," - %s file %s -> %s\n",
				copy_verb, fi->f.fname, out_fname );
		fo = &fo_local;
		InitializeSF(fo);
		err = OpenOutput(fo,fi,out_fname,0);
	    }

	    if (!err)
	    {
		SetupIOD(fi,OFT_PLAIN,OFT_PLAIN);
		err = AppendF(&fi->f,fo,0,fi->f.fatt.size);
	    }
	}
    }

    TRACELINE;
    return CloseInOut( fi, fo == &fo_local ? fo : 0, err, remove_source );
}

///////////////////////////////////////////////////////////////////////////////

enumError CatCISO ( SuperFile_t * fi, CISO_Head_t * ch, SuperFile_t * fo,
			ccp out_fname, bool ignore_raw, bool remove_source )
{
    TRACELINE;
    DASSERT(fi);
    DASSERT(ch);
    DASSERT( fo || out_fname );

    CISO_Info_t ci;
    if (InitializeCISO(&ci,ch))
	return CatRaw(fi,fo,out_fname,ignore_raw,remove_source);

    char out_fname_buf[PATH_MAX];
    if (out_fname)
    {
	ccp ext_list[] = { ".ciso", ".wbi", opt_suffix, 0 };
	out_fname = RemoveExt(out_fname_buf,sizeof(out_fname_buf),ext_list,out_fname);
    }

    if (testmode)
    {
	if (out_fname)
	    fprintf(logout," - WOULD %s CISO %s -> %s\n",
			extract_verb, fi->f.fname, out_fname );
	else
	    fprintf(logout," - WOULD %s CISO %s\n",extract_verb,fi->f.fname);
	ResetSF(fi,0);
	return ERR_OK;
    }

    enumError err = ERR_OK;
    SuperFile_t fo_local;
    if (!fo)
    {
	if ( verbose > 0 )
	    fprintf(logout," - %s CISO %s -> %s\n",extract_verb,fi->f.fname,out_fname);
	fo = &fo_local;
	InitializeSF(fo);
	err = OpenOutput(fo,fi,out_fname,0);
    }

    if (!err)
    {
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
		    AppendZeroSF(fo,zero_count*block_size);
		    zero_count = 0;
		}

		AppendSparseF(&fi->f,fo,read_off,block_size);
		read_off += block_size; 
		blast = bptr;
	    }
	    else
		zero_count++;

	if ( fo == &fo_local )
	{
	    const off_t off = (off_t)block_size * ( blast - ch->map );
	    err = SetSizeSF(fo,off);
	}
    }

    TRACELINE;
    return CloseInOut( fi, fo == &fo_local ? fo : 0, err, remove_source );
}

///////////////////////////////////////////////////////////////////////////////

enumError CatWDF ( ccp fname, SuperFile_t * fo, ccp out_fname,
			bool ignore_raw, bool remove_source )
{
    TRACELINE;
    DASSERT(fname);
    DASSERT( fo || out_fname );

    SuperFile_t fi;
    InitializeSF(&fi);
    enumError err = OpenFile(&fi.f,fname,IOM_IS_IMAGE);
    if (err)
	return err;

    err = SetupReadWDF(&fi);
    if ( err == ERR_NO_WDF )
    {
	fi.f.disable_errors = true;
	CISO_Head_t ch;
	err = ReadAtF(&fi.f,0,&ch,sizeof(ch));
	fi.f.disable_errors = false;
	if (   !err
	    && !memcmp(&ch.magic,"CISO",4)
	    && !*(ccp)&ch.block_size
	    && *ch.map == 1 )
	{
	    return CatCISO(&fi,&ch,fo,out_fname,ignore_raw,remove_source);
	}

	// normal cat
	return CatRaw(&fi,fo,out_fname,ignore_raw,remove_source);
    }

    char out_fname_buf[PATH_MAX];
    if (out_fname)
    {
	ccp ext_list[] = { ".wdf", opt_suffix, 0 };
	out_fname = RemoveExt(out_fname_buf,sizeof(out_fname_buf),ext_list,out_fname);
    }

    if (testmode)
    {
	if (out_fname)
	    fprintf(logout," - WOULD %s WDF  %s -> %s\n",extract_verb,fname,out_fname);
	else
	    fprintf(logout," - WOULD %s WDF  %s\n",extract_verb,fname);
	ResetSF(&fi,0);
	return ERR_OK;
    }

    SuperFile_t fo_local;
    if ( !err && !fo )
    {
	if ( verbose > 0 )
	    fprintf(logout," - %s WDF  %s -> %s\n",extract_verb,fname,out_fname);
	fo = &fo_local;
	InitializeSF(fo);
	err = OpenOutput(fo,0,out_fname,0);
    }

    if (!err)
    {
	int i;
	u64 last_off = 0;
	for ( i=0; i < fi.wc_used; i++ )
	{
	    if ( SIGINT_level > 1 )
	    {
		err = ERR_INTERRUPT;
		break;
	    }

	    WDF_Chunk_t *wc = fi.wc + i;
	    u64 zero_count = wc->file_pos - last_off;
	    if ( zero_count > 0 )
	    {
		TRACE("WDF >> ZERO #%02d @%9llx [%9llx].\n",i,last_off,zero_count);
		last_off += zero_count;
		err = AppendZeroSF(fo,zero_count);
		if (err)
		    return err;
	    }

	    if ( wc->data_size )
	    {
		TRACE("WDF >> COPY #%02d @%9llx [%9llx] read-off=%9llx.\n",
			i, last_off, wc->data_size, wc->data_off );
		last_off += wc->data_size;
		err = AppendF(&fi.f,fo,wc->data_off,wc->data_size);
		if (err)
		    return err;
	    }
	}

	if ( fo == &fo_local )
	    err = SetSizeSF(fo,last_off);
    }

    TRACELINE;
    return CloseInOut( &fi, fo == &fo_local ? fo : 0, err, remove_source );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  cmd cat			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_cat ( bool ignore_raw )
{
    SuperFile_t out;
    InitializeSF(&out);
    enumError err = OpenOutput(&out,0,0,"CONCATENATE files:");
    
    ParamList_t * param;
    for ( param = first_param; !err && param; param = param->next )
	err = CatWDF(param->arg,&out,0,ignore_raw,false);

    SetSizeSF(&out,out.file_size);
    ResetSF(&out,0);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  cmd unpack			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_unpack()
{
    if (opt_stdout)
	return cmd_cat(true);

    if ( verbose > 0 )
	print_title(logout);

    char dest_buf[PATH_MAX];

    ParamList_t * param;
    enumError err = ERR_OK;
    for ( param = first_param; !err && param; param = param->next )
    {
	ccp dest = param->arg;
	if (opt_dest)
	{
	    PathCatPP(dest_buf,sizeof(dest_buf),opt_dest,dest);
	    dest = dest_buf;
	}
	err = CatWDF(param->arg,0,dest,true,!opt_keep);
    }
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  cmd pack			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_pack()
{
    if ( verbose > 0 )
	print_title(logout);

    create_oft = file_mode_oft[file_mode];
    extract_verb = copy_verb = "pack";

    char dest[PATH_MAX];
    ccp suffix = opt_suffix ? opt_suffix : file_mode_name[file_mode];

    ParamList_t * param;
    enumError err = ERR_OK;
    for ( param = first_param; !err && param; param = param->next )
    {
	PathCatPPE(dest,sizeof(dest),opt_dest,param->arg,suffix);
	err = CatWDF(param->arg,0,dest,false,!opt_keep);
    }
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   cmd cmp			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_cmp()
{
    // [2do]
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	printf("CMP %s\n",param->arg);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   cmd dump			///////////////
///////////////////////////////////////////////////////////////////////////////

static u64 prev_val = 0;

static int print_range ( FILE *f, ccp text, u64 end, u64 len )
{
    const int err = end - prev_val != len;
    if (err)
	fprintf(f,"    %-18s: %10llx .. %10llx [%10llx!=%10llx] INVALID!\n", \
		text, prev_val, end-opt_minus1, len, end-prev_val );
    else
	fprintf(f,"    %-18s: %10llx .. %10llx [%10llx]\n", \
		text, prev_val, end-opt_minus1, len );

    prev_val = end;
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError ciso_dump ( FILE *f, CISO_Head_t * ch, File_t *df, ccp fname )
{
    ASSERT(ch);
    ASSERT(df);
    ASSERT(fname);

    if (testmode)
    {
	fprintf(f," - WOULD dump CISO %s\n",fname);
	ResetFile(df,false);
	return ERR_OK;
    }

    fprintf(f,"\nCISO dump of file %s\n\n",fname);

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
    fprintf(f,"  %-18s:         \"%s\" = %02x-%02x-%02x-%02x\n",
		"Magic", wd_print_id(m,4,0), m[0], m[1], m[2], m[3] );

    fprintf(f,"  %-18s: %10x/hex =%11d\n","Used blocks",used_bl,used_bl);
    fprintf(f,"  %-18s: %10x/hex =%11d\n","Max block",max_bl-1,max_bl-1);
    fprintf(f,"  %-18s: %10x/hex =%11d\n","Block size",block_size,block_size);
    fprintf(f,"  %-18s: %10llx/hex =%11lld\n","Real file usage",max_off,max_off);
    fprintf(f,"  %-18s: %10llx/hex =%11lld\n","Real file size",(u64)df->st.st_size,(u64)df->st.st_size);
    fprintf(f,"  %-18s: %10llx/hex =%11lld\n","ISO file usage",iso_file_usage,iso_file_usage);
    fprintf(f,"  %-18s: %10llx/hex =%11lld\n","ISO file size",iso_file_size,iso_file_size);
    putchar('\n');

    //----- print mapping table

    if (opt_chunk)
    {
	fprintf(f,
	    "  Mapping Table:\n\n"
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
		fprintf(f,"%11x      : %9llx .. %9llx : %9llx .. %9llx : %9llx\n",
		    b1, cfa, cfa+siz-opt_minus1, via, via+siz-opt_minus1, siz );
	    else
		fprintf(f,"%8x .. %4x : %9llx .. %9llx : %9llx .. %9llx : %9llx\n",
		    b1, b2-1, cfa, cfa+siz-opt_minus1, via, via+siz-opt_minus1, siz );

	    bcount += b2 - b1;
	}
	putchar('\n');
    }

    ResetFile(df,false);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError wia_dump ( FILE *f, File_t *df, ccp fname )
{
    ASSERT(df);
    ASSERT(fname);

    if (testmode)
    {
	fprintf(f," - WOULD dump WIA %s\n",fname);
	ResetFile(df,false);
	return ERR_OK;
    }

    fprintf(f,"\nWIA dump of file %s\n\n",fname);

    SuperFile_t sf;
    InitializeSF(&sf);
    memcpy(&sf.f,df,sizeof(File_t));
    InitializeFile(df);
    SetupReadWIA(&sf);

    wia_controller_t * wia = sf.wia;
    ASSERT(wia);

    //-------------------------

    fprintf(f,"\n  File header:\n");

    u8 * m = (u8*)wia->fhead.magic;
    fprintf(f,"    %-23s: \"%s\"  %02x %02x %02x %02x\n",
		"Magic",
		wd_print_id(m,4,0), m[0], m[1], m[2], m[3] );
    fprintf(f,"    %-23s: 0x%08x => %s\n",
		"Version",
		wia->fhead.version,
		PrintVersionWIA(0,0,wia->fhead.version) );
    fprintf(f,"    %-23s: 0x%08x => %s\n",
		"Version/compatible",
		wia->fhead.version_compatible,
		PrintVersionWIA(0,0,wia->fhead.version_compatible) );

    if ( wia->fhead.disc_size == sizeof(wia_disc_t) )
	fprintf(f,"    %-23s: %10u = %9x/hex = %s\n",
		"Size of disc section",
		wia->fhead.disc_size, wia->fhead.disc_size,
		wd_print_size(0,0,wia->fhead.disc_size,true) );
    else
	fprintf(f,"    %-23s: %10u = %9x [current: %zu = %zx/hex]\n",
		"Size of disc section",
		wia->fhead.disc_size, wia->fhead.disc_size,
		sizeof(wia_disc_t), sizeof(wia_disc_t) );

    if (wia->fhead.iso_file_size)
    {
	fprintf(f,"    %-23s: %10llu =%10llx/hex = %s\n",
		"ISO image size",
		wia->fhead.iso_file_size, wia->fhead.iso_file_size,
		wd_print_size(0,0,wia->fhead.iso_file_size,true) );
	double percent = 100.0 * wia->fhead.wia_file_size / wia->fhead.iso_file_size;
	fprintf(f,"    %-23s: %10llu =%10llx/hex = %s  %4.*f%%\n",
		"Total file size",
		wia->fhead.wia_file_size, wia->fhead.wia_file_size,
		wd_print_size(0,0,wia->fhead.wia_file_size,true),
		percent <= 9.9 ? 2 : 1, percent );
    }
    else
	fprintf(f,"    %-23s: %10llu =%10llx/hex = %s\n",
		"Total file size",
		wia->fhead.wia_file_size, wia->fhead.wia_file_size,
		wd_print_size(0,0,wia->fhead.wia_file_size,true) );

    //-------------------------

    fprintf(f,"\n  Disc header:\n");

    const wia_disc_t * disc = &wia->disc;

    if (disc->disc_type)
	fprintf(f,"    %-23s: %s, %.64s\n",
		"ID & title",
		wd_print_id(disc->dhead,6,0),
		disc->dhead + WII_TITLE_OFF );
    fprintf(f,"    %-23s: %10u = %s\n",
		"Disc type",
		disc->disc_type, wd_get_disc_type_name(disc->disc_type,"?") );
    fprintf(f,"    %-23s: %10u = %s\n",
		"Compression method",
		disc->compression, wd_get_compression_name(disc->compression,"?") );
    fprintf(f,"    %-23s: %10u\n",
		"Compression level", disc->compr_level);
    fprintf(f,"    %-23s: %10u =%10x/hex = %s\n",
		"Chunk size",
		disc->chunk_size, disc->chunk_size,
		wd_print_size(0,0,disc->chunk_size,true) );

    fprintf(f,"    %-23s: %10u\n",
		" Number of partitions",
		disc->n_part );
    fprintf(f,"    %-23s: %10llu = %9llx/hex = %s\n",
		" Offset of Part.header",
		disc->part_off, disc->part_off,
		wd_print_size(0,0,disc->part_off,true) );
    fprintf(f,"    %-23s: %2d * %5u = %9u\n",
		" Size of Part.header",
		disc->n_part, disc->part_t_size,
		disc->n_part * disc->part_t_size );

    fprintf(f,"    %-23s: %10u\n",
		"Num. of raw data elem.", disc->n_raw_data );
    fprintf(f,"    %-23s: %10llu = %9llx/hex = %s\n",
		"Offset of raw data tab.",
		disc->raw_data_off, disc->raw_data_off,
		wd_print_size(0,0,disc->raw_data_off,true) );
    fprintf(f,"    %-23s: %10u = %9x/hex = %s\n",
		"Size of raw data tab.",
		disc->raw_data_size, disc->raw_data_size,
		wd_print_size(0,0,disc->raw_data_size,true) );

    fprintf(f,"    %-23s: %10u\n",
		" Num. of group elem.", disc->n_groups );
    fprintf(f,"    %-23s: %10llu = %9llx/hex = %s\n",
		" Offset of group tab.",
		disc->group_off, disc->group_off,
		wd_print_size(0,0,disc->group_off,true) );
    fprintf(f,"    %-23s: %10u = %9x/hex = %s\n",
		" Size of group tab.",
		disc->group_size, disc->group_size,
		wd_print_size(0,0,disc->group_size,true) );

    fprintf(f,"    %-23s: %10u = %9x/hex = %s\n",
		"Compressor data length",
		disc->compr_data_len, disc->compr_data_len,
		wd_print_size(0,0,disc->compr_data_len,true) );
    if (disc->compr_data_len)
    {
	fprintf(f,"    %-23s:","Compressor Data");
	int i;
	for ( i = 0; i < disc->compr_data_len; i++ )
	    fprintf(f," %02x",disc->compr_data[i]);
	fputc('\n',f);
    }

    //-------------------------

    if ( long_count > 0 )
    {
	fprintf(f,"\n  ISO memory map:\n");
	wd_print_memmap(stdout,3,&wia->memmap);
	putchar('\n');
    }

    //-------------------------

    fputc('\n',f);
    CloseSF(&sf,0);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError wdf_dump ( FILE *f, ccp fname )
{
    ASSERT(fname);

    File_t df;
    InitializeFile(&df);
    enumError err = OpenFile(&df,fname,IOM_IS_IMAGE);
    if (err)
	return err;

    WDF_Head_t wh;
    const int CISO_MAGIC_SIZE = 4;
    ASSERT( CISO_MAGIC_SIZE < sizeof(wh) );
    err = ReadAtF(&df,0,&wh,CISO_MAGIC_SIZE);
    if (err)
    {
	ResetFile(&df,false);
	return err;
    }

    if (!memcmp(&wh,"CISO",CISO_MAGIC_SIZE))
    {
	CISO_Head_t ch;
	memcpy(&ch,&wh,CISO_MAGIC_SIZE);
	err = ReadAtF(&df,CISO_MAGIC_SIZE,(char*)&ch+CISO_MAGIC_SIZE,sizeof(ch)-CISO_MAGIC_SIZE);
	if (err)
	{
	    ResetFile(&df,false);
	    return err;
	}
	return ciso_dump(f,&ch,&df,fname);
    }

    if (!memcmp(&wh,WIA_MAGIC,WIA_MAGIC_SIZE))
	return wia_dump(f,&df,fname);

    if (testmode)
    {
	fprintf(f," - WOULD dump WDF  %s\n",fname);
	ResetFile(&df,false);
	return ERR_OK;
    }

    fprintf(f,"\nWDF dump of file %s\n",fname);

    err = ReadAtF(&df,CISO_MAGIC_SIZE,(char*)&wh+CISO_MAGIC_SIZE,sizeof(wh)-CISO_MAGIC_SIZE);
    if (err)
    {
	ResetFile(&df,false);
	return err;
    }
    
    ConvertToHostWH(&wh,&wh);
    AnalyzeWH(&df,&wh,false); // needed for splitting support!

    fprintf(f,"\n  Header:\n\n");
    u8 * m = (u8*)wh.magic;
    fprintf(f,"    %-18s: \"%s\"  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
		"Magic",
		wd_print_id(m,8,0),
		m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7] );

    if (memcmp(wh.magic,WDF_MAGIC,WDF_MAGIC_SIZE))
    {
	ResetFile(&df,false);
	return ERROR0(ERR_WDF_INVALID,"Wrong magic: %s\n",fname);
    }

    #undef PRINT32
    #undef PRINT64
    #undef RANGE
    #define PRINT32(a) fprintf(f,"    %-18s: %10x/hex =%11d\n",#a,wh.a,wh.a)
    #define PRINT64(a) fprintf(f,"    %-18s: %10llx/hex =%11lld\n",#a,wh.a,wh.a)

    PRINT32(wdf_version);
 #if WDF2_ENABLED
    PRINT32(wdf_compatible);
    PRINT32(align_factor);
 #endif
    PRINT32(split_file_id);
    PRINT32(split_file_index);
    PRINT32(split_file_num_of);
    PRINT64(file_size);
    fprintf(f,"    %18s: %10llx/hex =%11lld  %4.2f%%\n","- WDF file size ",
		(u64)df.st.st_size, (u64)df.st.st_size,  100.0 * df.st.st_size / wh.file_size );
    PRINT64(data_size);
    PRINT32(chunk_split_file);
    PRINT32(chunk_n);
    PRINT64(chunk_off);

    //--------------------------------------------------

    fprintf(f,"\n  File Parts:\n\n");

    int ec = 0; // error count
    prev_val = 0;

    const int head_size  = GetHeadSizeWDF(wh.wdf_version);
    const int chunk_size = wh.chunk_n*sizeof(WDF_Chunk_t);
    
    ec += print_range(f, "Header",	head_size,			head_size );
    ec += print_range(f, "Data",	wh.chunk_off,			wh.data_size );
    ec += print_range(f, "Chunk-Magic",	wh.chunk_off+WDF_MAGIC_SIZE,	WDF_MAGIC_SIZE );
    ec += print_range(f, "Chunk-Table",	df.st.st_size,			chunk_size );
    fprintf(f,"\n");

    if (ec)
    {
	ResetFile(&df,false);
	return ERROR0(ERR_WDF_INVALID,"Invalid data: %s\n",fname);
    }

    if ( wh.chunk_n > 1000000 )
    {
	ResetFile(&df,false);
	return ERROR0(ERR_INTERNAL,"Too much chunk enties: %s\n",fname);
    }

    //--------------------------------------------------

    char magic[WDF_MAGIC_SIZE];
    err = ReadAtF(&df,wh.chunk_off,magic,sizeof(magic));
    if (err)
    {
	ResetFile(&df,false);
	return ERROR0(ERR_READ_FAILED,"ReadF error: %s\n",fname);
    }

    if (memcmp(magic,WDF_MAGIC,WDF_MAGIC_SIZE))
    {
	ResetFile(&df,false);
	return ERROR0(ERR_WDF_INVALID,"Wrong chunk table magic: %s\n",fname);
    }

    WDF_Chunk_t *w, *wc = malloc(chunk_size);
    if (!wc)
	OUT_OF_MEMORY;

    err = ReadF(&df,wc,chunk_size);
    if (err)
    {
	ResetFile(&df,false);
	free(wc);
	return ERROR0(ERR_READ_FAILED,"ReadF error: %s\n",fname);
    }

    if (opt_chunk)
    {
	fprintf(f,
	    "  Chunk Table:\n\n"
	    "    idx        WDF file offset  data len     virtual ISO offset  hole size\n"
	    "   ------------------------------------------------------------------------\n");

	int idx;
	for ( idx = 0, w = wc; idx < wh.chunk_n; idx++, w++ )
	    ConvertToHostWC(w,w);

	for ( idx = 0, w = wc; idx < wh.chunk_n; idx++, w++ )
	{
	    fprintf(f,"%6d. %10llx..%10llx %9llx %10llx..%10llx  %9llx\n",
		    idx,
		    w->data_off, w->data_off + w->data_size-opt_minus1,
		    w->data_size,
		    w->file_pos, w->file_pos + w->data_size - opt_minus1,
		    idx+1 < wh.chunk_n ? w[1].file_pos - w->file_pos - w->data_size : 0llu );
	}

	putchar('\n');
    }

    ResetFile(&df,false);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError cmd_dump()
{
    SuperFile_t sf;
    InitializeSF(&sf);
    enumError err = OpenOutput(&sf,0,0,"DUMP WDF and CISO data strutures:");
    
    ParamList_t * param;
    for ( param = first_param; !err && param; param = param->next )
	err = wdf_dump(sf.f.fp,param->arg);

    ResetSF(&sf,0);
    return err;
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

      RegisterOptionByName(&InfoUI,opt_stat,1,false);

      switch ((enumGetOpt)opt_stat)
      {
	case GO__ERR:		err++; break;

	case GO_VERSION:	version_exit();
	case GO_HELP:		help_exit(false);
	case GO_XHELP:		help_exit(true);
	case GO_WIDTH:		err += ScanOptWidth(optarg); break;
	case GO_QUIET:		verbose = verbose > -1 ? -1 : verbose - 1; break;
	case GO_VERBOSE:	verbose = verbose <  0 ?  0 : verbose + 1; break;
	case GO_LOGGING:	logging++; break;
	case GO_CHUNK:		opt_chunk = true; break;
	case GO_LONG:		opt_chunk = true; long_count++; break;
	case GO_MINUS1:		opt_minus1 = 1; break;

	case GO_WDF:		file_mode = FMODE_WDF; break;
	case GO_WIA:		file_mode = FMODE_WIA; break;
	case GO_CISO:		file_mode = FMODE_CISO; break;
	case GO_WBI:		file_mode = FMODE_WBI; break;
	case GO_SUFFIX:		opt_suffix = optarg; break;

	case GO_DEST:		opt_dest = optarg; break;
	case GO_DEST2:		opt_dest = optarg; opt_mkdir = true; break;
	case GO_STDOUT:		opt_stdout = true; break;
	case GO_KEEP:		opt_keep = true; break;
	case GO_OVERWRITE:	opt_overwrite = true; break;
	case GO_PRESERVE:	opt_preserve = true; break;

	case GO_SPLIT:		opt_split++; break;
	case GO_SPLIT_SIZE:	err += ScanOptSplitSize(optarg); break;
	case GO_CHUNK_MODE:	err += ScanChunkMode(optarg); break;
	case GO_CHUNK_SIZE:	err += ScanChunkSize(optarg); break;
	case GO_MAX_CHUNKS:	err += ScanMaxChunks(optarg); break;
	case GO_COMPRESSION:	err += ScanOptCompression(optarg); break;
	case GO_MEM:		err += ScanOptMem(optarg,true); break;

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
///////////////			  CheckCommand()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CheckCommand ( int argc, char ** argv )
{
    argc -= optind;
    argv += optind;

    TRACE("CheckCommand(%d,) optind=%d\n",argc+optind,optind);
    
    if ( argc > 0 && **argv == '+' )
    {
	int cmd_stat;
	const CommandTab_t * cmd_ct = ScanCommand(&cmd_stat,*argv,CommandTab);
	if (!cmd_ct)
	{
	    if ( cmd_stat > 0 )
		ERROR0(ERR_SYNTAX,"Command abbreviation is ambiguous: %s\n",*argv);
	    else
		ERROR0(ERR_SYNTAX,"Unknown command: %s\n",*argv);
	    hint_exit(ERR_SYNTAX);
	}

	TRACE("COMMAND FOUND: #%lld = %s\n",cmd_ct->id,cmd_ct->name1);

	enumError err = VerifySpecificOptions(&InfoUI,cmd_ct);
	if (err)
	    hint_exit(err);

	the_cmd = cmd_ct->id;
	argc--;
	argv++;
    }

    while ( argc-- > 0 )
	AtFileHelper(*argv++,false,true,AddParam);

    enumError err = ERR_OK;
    switch (the_cmd)
    {
	case CMD_VERSION:	version_exit();
	case CMD_HELP:		PrintHelp(&InfoUI,stdout,0,"+HELP",default_settings); break;

	case CMD_PACK:		err = cmd_pack(); break;
	case CMD_UNPACK:	err = cmd_unpack(); break;
	case CMD_CAT:		err = cmd_cat(false); break;
	case CMD_CMP:		err = cmd_cmp(); break;
	case CMD_DUMP:		err = cmd_dump(); break;

	// no default case defined
	//	=> compiler checks the existence of all enum values

	case CMD__NONE:
	case CMD__N:
	    help_exit(false);
    }

    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    main()			///////////////
///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    logout = stdout;
    SetupLib(argc,argv,NAME,PROG_WDF);
    opt_chunk_mode = CHUNK_MODE_32KIB;
    progname0 = progname;

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
	    the_cmd = CMD_DUMP;
	else if ( strstr(iobuf,"cmp") || strstr(iobuf,"diff") )
	    the_cmd = CMD_CMP;
	else if ( strstr(iobuf,"cat") )
	    the_cmd = CMD_CAT;
	else if ( !memcmp(iobuf,"un",2) )
	    the_cmd = CMD_UNPACK;

	if (strstr(iobuf,"wia"))
	    file_mode = def_file_mode = FMODE_WIA;
	else if (strstr(iobuf,"ciso"))
	    file_mode = def_file_mode = FMODE_CISO;
	else if (strstr(iobuf,"wbi"))
	    file_mode = def_file_mode = FMODE_WBI;
    }

    ccp fmname = file_mode_name[def_file_mode] + 1;
    switch(the_cmd)
    {
	case CMD_DUMP:
	    snprintf(progname_buf,sizeof(progname_buf), "%s-dump", fmname );
	    break;

	case CMD_CMP:
	    snprintf(progname_buf,sizeof(progname_buf), "%s-cmp", fmname );
	    break;

	case CMD_CAT:
	    snprintf(progname_buf,sizeof(progname_buf), "%s-cat", fmname );
	    break;

	case CMD_UNPACK:
	    snprintf(progname_buf,sizeof(progname_buf), "un%s", fmname );
	    break;

	default:
	    snprintf(progname_buf,sizeof(progname_buf), "%s", fmname );
	    break;
    }
    progname = progname_buf;

    snprintf( default_settings, sizeof(default_settings),
		"Default settings for '%s': %s --%s\n\n",
		progname0, CommandInfo[the_cmd].name1, fmname );

    //----- process arguments

    if ( argc < 2 )
    {
	printf("\n%s%s\nVisit %s%s for more info.\n\n",progname,TITLE,URI_HOME,NAME);
	hint_exit(ERR_OK);
    }

    if (CheckOptions(argc,argv))
	hint_exit(ERR_SYNTAX);

    if ( opt_stdout )
	logout = stderr;

    //----- cmd selector

    int err = CheckCommand(argc,argv);
    if (err)
	return err;


    //----- terminate

    if (SIGINT_level)
	ERROR0(ERR_INTERRUPT,"Program interrupted by user.");
    return err > max_error ? err : max_error;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

