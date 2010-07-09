
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <dirent.h>

#include "debug.h"
#include "libwbfs.h"
#include "lib-sf.h"
#include "wbfs-interface.h"
#include "titles.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 SuperFile_t: setup              ///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeSF ( SuperFile_t * sf )
{
    ASSERT(sf);
    memset(sf,0,sizeof(*sf));
    InitializeWH(&sf->wh);
    InitializeFile(&sf->f);
    InitializeMemMap(&sf->modified_list);
    ResetSF(sf,0);
}

///////////////////////////////////////////////////////////////////////////////
// close file & remove all dynamic data

void FreeSF ( SuperFile_t * sf )
{
    ASSERT(sf);

    if (sf->wc)
	free(sf->wc);
    sf->wc = 0;
    sf->wc_size = 0;
    sf->wc_used = 0;

    ResetCISO(&sf->ciso);

    if (sf->wbfs)
    {
	TRACE("#S# close WBFS %s id=%s\n",sf->f.fname,sf->f.id6);
	ResetWBFS(sf->wbfs);
	free(sf->wbfs);
	sf->wbfs = 0;
    }

    if (sf->fst)
    {
	TRACE("#S# close FST %s id=%s\n",sf->f.fname,sf->f.id6);
	ResetFST(sf->fst);
	free(sf->fst);
	sf->fst = 0;
    }

    ResetMemMap(&sf->modified_list);
}

///////////////////////////////////////////////////////////////////////////////

enumError CloseSF ( SuperFile_t * sf, FileAttrib_t * set_time_ref )
{
    ASSERT(sf);
    enumError err = ERR_OK;

    if (sf->disc)
    {
	wd_close_disc(sf->disc);
	sf->disc = 0;
    }

    if ( sf->f.is_writing && sf->min_file_size )
    {
	err = SetMinSizeSF(sf,sf->min_file_size);
	sf->min_file_size = 0;
    }

    if ( !err && sf->f.is_writing && !sf->f.is_reading )
    {
	if (sf->wc)
	    err = TermWriteWDF(sf);

	if (sf->ciso.map)
	    err = TermWriteCISO(sf);

	if (sf->wbfs)
	{
	    CloseWDisc(sf->wbfs);
	    err = TruncateWBFS(sf->wbfs);
	}
    }

    if ( err != ERR_OK )
	CloseFile(&sf->f,true);
    else
    {
	err = CloseFile(&sf->f,false);
	if (set_time_ref)
	    SetFileTime(&sf->f,set_time_ref);
    }

    FreeSF(sf);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError ResetSF ( SuperFile_t * sf, FileAttrib_t * set_time_ref )
{
    ASSERT(sf);

    // free dynamic memory
    enumError err = CloseSF(sf,set_time_ref);
    ResetFile(&sf->f,false);
    sf->indent = NormalizeIndent(sf->indent);

    // reset data
    SetupIOD(sf,OFT_UNKNOWN,OFT_UNKNOWN);
    InitializeWH(&sf->wh);
    ResetCISO(&sf->ciso);
    sf->max_virt_off = 0;

    // reset timer
    sf->progress_start_time = sf->progress_last_calc_time = GetTimerMSec();
    sf->progress_last_view_sec = 0;
    sf->progress_sum_time = 0;
    sf->progress_time_divisor = 0;
    sf->progress_max_wd = 0;
 #if defined(DEBUG) || defined(TEST)
    sf->show_msec = true;
 #else
    sf->show_msec = false;
 #endif
    
    sf->allow_fst = allow_fst;

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError RemoveSF ( SuperFile_t * sf )
{
    ASSERT(sf);
    TRACE("RemoveSF(%p)\n",sf);

    if (sf->wbfs)
    {
	if ( sf->wbfs->used_discs > 1 )
	{
	    TRACE(" - remove wdisc %s\n",sf->f.id6);
	    RemoveWDisc(sf->wbfs,sf->f.id6,0);
	    return ResetSF(sf,0);
	}
    }

    TRACE(" - remove file %s\n",sf->f.fname);
    enumError err = CloseSF(sf,0);
    CloseFile(&sf->f,true);
    ResetSF(sf,0);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

bool IsOpenSF ( const SuperFile_t * sf )
{
    return sf && ( sf->fst || IsOpenF(&sf->f) );
}

///////////////////////////////////////////////////////////////////////////////

enumOFT SetupIOD ( SuperFile_t * sf, enumOFT force, enumOFT def )
{
    ASSERT(sf);
    sf->iod.oft = CalcOFT(force,sf->f.fname,0,def);
    TRACE("SetupIOD(%p,%u,%u) OFT := %u\n",sf,force,def,sf->iod.oft);

    switch (sf->iod.oft)
    {
	case OFT_PLAIN:
	    sf->iod.read_func		= ReadISO;
	    sf->iod.write_func		= WriteISO;
	    sf->iod.write_sparse_func	= WriteSparseISO;
	    sf->iod.write_zero_func	= WriteZeroISO;
	    break;

	case OFT_WDF:
	    sf->iod.read_func		= ReadWDF;
	    sf->iod.write_func		= WriteWDF;
	    sf->iod.write_sparse_func	= WriteSparseWDF;
	    sf->iod.write_zero_func	= WriteZeroWDF;
	    break;

	case OFT_CISO:
	    sf->iod.read_func		= ReadCISO;
	    sf->iod.write_func		= WriteCISO;
	    sf->iod.write_sparse_func	= WriteSparseCISO;
	    sf->iod.write_zero_func	= WriteZeroCISO;
	    break;

	case OFT_WBFS:
	    sf->iod.read_func		= ReadWBFS;
	    sf->iod.write_func		= WriteWBFS;
	    sf->iod.write_sparse_func	= WriteWBFS;		// no sparse support
	    sf->iod.write_zero_func	= WriteZeroWBFS;
	    break;

	case OFT_FST:
	    sf->iod.read_func		= ReadFST;
	    sf->iod.write_func		= WriteSwitchSF;	// not supported
	    sf->iod.write_sparse_func	= WriteSparseSwitchSF;	// not supported
	    sf->iod.write_zero_func	= WriteZeroSwitchSF;	// not supported
	    break;

	default:
	    sf->iod.read_func		= ReadSwitchSF;
	    sf->iod.write_func		= WriteSwitchSF;
	    sf->iod.write_sparse_func	= WriteSparseSwitchSF;
	    sf->iod.write_zero_func	= WriteZeroSwitchSF;
	    break;
    }
    sf->std_read_func = sf->iod.read_func;

    return sf->iod.oft;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////	                       Setup SF                ////////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupReadSF ( SuperFile_t * sf )
{
    ASSERT(sf);
    TRACE("SetupReadSF(%p) fd=%d is-r=%d is-w=%d\n",
	sf, sf->f.fd, sf->f.is_reading, sf->f.is_writing );

    if ( !sf || !sf->f.is_reading )
	return ERROR0(ERR_INTERNAL,0);

    SetupIOD(sf,OFT_PLAIN,OFT_PLAIN);
    if ( sf->f.ftype == FT_UNKNOWN )
	AnalyzeFT(&sf->f);

    if ( sf->allow_fst && sf->f.ftype & FT_ID_FST )
	return SetupReadFST(sf);

    if ( !IsOpenSF(sf) )
    {
	if (!sf->f.disable_errors)
	    ERROR0( ERR_CANT_OPEN, "Can't open file: %s\n", sf->f.fname );
	return ERR_CANT_OPEN;
    }

    if ( sf->f.ftype & FT_A_WDF )
	return SetupReadWDF(sf);

    if ( sf->f.ftype & FT_A_CISO )
	return SetupReadCISO(sf);

    if ( sf->f.ftype & FT_ID_WBFS && sf->f.id6[0] )
	return SetupReadWBFS(sf);

    sf->file_size = sf->f.st.st_size;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError SetupReadISO ( SuperFile_t * sf )
{
    ASSERT(sf);
    TRACE("SetupReadISO(%p)\n",sf);

    enumError err = SetupReadSF(sf);
    if ( err || sf->f.ftype & FT_A_ISO )
	return err;

    if (!sf->f.disable_errors)
	PrintErrorFT(&sf->f,FT_A_ISO);
    return ERR_WRONG_FILE_TYPE;
}

///////////////////////////////////////////////////////////////////////////////

enumError SetupReadWBFS ( SuperFile_t * sf )
{
    ASSERT(sf);
    TRACE("SetupReadWBFS(%p)\n",sf);

    if ( !sf || !sf->f.is_reading || sf->wbfs )
	return ERROR0(ERR_INTERNAL,0);

    WBFS_t * wbfs = malloc(sizeof(*wbfs));
    if (!wbfs)
	OUT_OF_MEMORY;
    InitializeWBFS(wbfs);
    enumError err = SetupWBFS(wbfs,sf,true,0,false);
    if (err)
	goto abort;

    err = OpenWDiscID6(wbfs,sf->f.id6);
    if (err)
	goto abort;

    //----- calc file size

    ASSERT(wbfs->disc);
    ASSERT(wbfs->disc->header);
    u16 * wlba_tab = wbfs->disc->header->wlba_table;
    ASSERT(wlba_tab);

    wbfs_t *w = wbfs->wbfs;
    ASSERT(w);

    int bl;
    for ( bl = w->n_wbfs_sec_per_disc-1; bl > 0; bl-- )
	if (ntohs(wlba_tab[bl]))
	{
	    bl++;
	    break;
	}
    sf->file_size = (off_t)bl * w->wbfs_sec_sz;
    const off_t single_size = WII_SECTORS_SINGLE_LAYER * (off_t)WII_SECTOR_SIZE;
    if ( sf->file_size < single_size )
	sf->file_size = single_size;

    //---- store data

    TRACE("WBFS %s/%s opened.\n",sf->f.fname,sf->f.id6);
    sf->wbfs = wbfs;

    wd_header_t * dh = (wd_header_t*)wbfs->disc->header;
    snprintf(iobuf,sizeof(iobuf),"%s [%s]",
		GetTitle(sf->f.id6,(ccp)dh->game_title), sf->f.id6 );
    FreeString(sf->f.outname);
    sf->f.outname = strdup(iobuf);
    SetupIOD(sf,OFT_WBFS,OFT_WBFS);
    SetupISOModifier(sf);
    return ERR_OK;

 abort:
    ResetWBFS(wbfs);
    free(wbfs);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenSF
(
	SuperFile_t * sf,
	ccp fname,
	bool allow_non_iso,
	bool open_modify
)
{
    ASSERT(sf);
    CloseSF(sf,0);
    TRACE("#S# OpenSF(%p,%s,%d,%d)\n",sf,fname,allow_non_iso,open_modify);

    const bool disable_errors = sf->f.disable_errors;
    sf->f.disable_errors = true;
    if (open_modify)
	OpenFileModify(&sf->f,fname,IOM_IS_IMAGE);
    else
	OpenFile(&sf->f,fname,IOM_IS_IMAGE);
    sf->f.disable_errors = disable_errors;

    DefineCachedAreaISO(&sf->f,true);

    return allow_non_iso
		? SetupReadSF(sf)
		: SetupReadISO(sf);
}

///////////////////////////////////////////////////////////////////////////////

enumError CreateSF
(
	SuperFile_t * sf,
	ccp fname,
	enumOFT oft,
	enumIOMode iomode,
	int overwrite
)
{
    ASSERT(sf);
    CloseSF(sf,0);
    TRACE("#S# CreateSF(%p,%s,%x)\n",sf,fname,oft);

    const enumError err = CreateFile(&sf->f,fname,iomode,overwrite);
    return err ? err : SetupWriteSF(sf,oft);
}

///////////////////////////////////////////////////////////////////////////////

enumError SetupWriteSF ( SuperFile_t * sf, enumOFT oft )
{
    ASSERT(sf);
    TRACE("SetupWriteSF(%p)\n",sf);

    if ( !sf || sf->f.is_reading || !sf->f.is_writing )
	return ERROR0(ERR_INTERNAL,0);

    SetupIOD(sf,oft,OFT__DEFAULT);
    switch(sf->iod.oft)
    {
	case OFT_PLAIN:
	    return ERR_OK;

	case OFT_WDF:
	    return SetupWriteWDF(sf);

	case OFT_CISO:
	    return SetupWriteCISO(sf);

	case OFT_WBFS:
	    return SetupWriteWBFS(sf);

	default:
	    return ERROR0(ERR_INTERNAL,"Unknown output file format: %s\n",sf->iod.oft);
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError SetupWriteWBFS ( SuperFile_t * sf )
{
    ASSERT(sf);
    TRACE("SetupWriteWBFS(%p)\n",sf);

    if ( !sf || sf->f.is_reading || !sf->f.is_writing || sf->wbfs )
	return ERROR0(ERR_INTERNAL,0);

    WBFS_t * wbfs =  malloc(sizeof(*wbfs));
    if (!wbfs)
	OUT_OF_MEMORY;
    InitializeWBFS(wbfs);
    enumError err = CreateGrowingWBFS(wbfs,sf,(off_t)10*GiB,sf->f.sector_size);
    sf->wbfs = wbfs;
    SetupIOD(sf,OFT_WBFS,OFT_WBFS);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

wd_disc_t * OpenDiscSF
(
	SuperFile_t * sf,	// valid pointer
	bool load_part_data,	// true: load partition data
	bool print_err		// true: print error message if open fails
)
{
    ASSERT(sf);
    if ( !sf->disc && IsOpenSF(sf) && sf->f.is_reading )
	sf->disc = wd_open_disc(WrapperReadSF,sf,sf->file_size,0);
	
    if (!sf->disc)
    {
	if (print_err)
	    ERROR0(ERR_WDISC_NOT_FOUND,"Can't open Wii disc: %s\n",sf->f.fname);
    }
    else if (load_part_data)
	wd_load_all_part(sf->disc,false,false);

    return sf->disc;
}

///////////////////////////////////////////////////////////////////////////////

void SubstFileNameSF
(
    SuperFile_t	* fo,		// output file
    SuperFile_t	* fi,		// input file
    ccp		dest_arg	// arg to parse for escapes
)
{
    ASSERT(fo);
    ASSERT(fi);

    char fname[PATH_MAX*2];
    const int conv_count
	= SubstFileNameBuf(fname,sizeof(fname),fi,dest_arg,fo->f.fname,fo->iod.oft);
    if ( conv_count > 0 )
	fo->f.create_directory = true;
    SetFileName(&fo->f,fname,true);
}

///////////////////////////////////////////////////////////////////////////////

int SubstFileNameBuf
(
    char	* fname_buf,	// output buf
    size_t	fname_size,	// size of output buf
    SuperFile_t	* fi,		// input file -> id6, fname
    ccp		fname,		// pure filename. if NULL: extract from 'fi'
    ccp		dest_arg,	// arg to parse for escapes
    enumOFT	oft		// output file type
)
{
    ASSERT(fi);

    ccp disc_name = 0;
    char buf[HD_SECTOR_SIZE];
    if (fi->f.id6[0])
    {
	if (fi->disc)
	    disc_name = (ccp)fi->disc->dhead.game_title;
	else
	{
	    const bool disable_errors = fi->f.disable_errors;
	    fi->f.disable_errors = true;
	    if (!ReadSF(fi,0,&buf,sizeof(buf)))
		disc_name = (ccp)((wd_header_t*)buf)->game_title;
	    fi->f.disable_errors = disable_errors;
	}
    }

    return SubstFileName ( fname_buf, fname_size,
			fi->f.id6, disc_name, fi->f.path ? fi->f.path : fi->f.fname,
			fname, dest_arg, oft );
}

///////////////////////////////////////////////////////////////////////////////

int SubstFileName
(
    char	* fname_buf,	// output buf
    size_t	fname_size,	// size of output buf
    ccp		id6,		// id6
    ccp		disc_name,	// name of disc
    ccp		src_file,	// full path to source file
    ccp		fname,		// pure filename, no path. If NULL: extract from 'src_file'
    ccp		dest_arg,	// arg to parse for escapes
    enumOFT	oft		// output file type
)
{
    DASSERT(fname_buf);
    DASSERT(fname_size>1);

    if ( !src_file || !*src_file )
	src_file = "./";
	
    if (!fname)
    {
	fname = src_file;
	ccp temp = strrchr(src_file,'/');
	if (temp)
	    fname = temp+1;
    }

    char src_path[PATH_MAX];
    StringCopyS(src_path,sizeof(src_path),src_file);
    char * temp = strrchr(src_path,'/');
    if (temp)
	*temp = 0;
    else
	*src_path = 0;

    if (!disc_name)
	disc_name = id6;
    ccp title = GetTitle(id6,disc_name);

    char x_buf[1000];
    snprintf(x_buf,sizeof(x_buf),"%s [%s]%s",
		title, id6, oft_ext[oft] );

    char y_buf[1000];
    snprintf(y_buf,sizeof(y_buf),"%s [%s]",
		title, id6 );

    char plus_buf[20];
    ccp plus_name;
    if ( oft == OFT_WBFS )
    {
	snprintf(plus_buf,sizeof(plus_buf),"%s%s",
	    id6, oft_ext[oft] );
	plus_name = plus_buf;
    }
    else
	plus_name = x_buf;

    SubstString_t subst_tab[] =
    {
	{ 'i', 'I', 0, id6 },
	{ 'n', 'N', 0, disc_name },
	{ 't', 'T', 0, title },
	{ 'e', 'E', 0, oft_ext[oft]+1 },
	{ 'p', 'P', 1, src_path },
	{ 'f', 'F', 1, fname },
	{ 'x', 'X', 0, x_buf },
	{ 'y', 'Y', 0, y_buf },
	{ '+', '+', 0, plus_name },
	{0,0,0,0}
    };

    int conv_count;
    SubstString(fname_buf,fname_size,subst_tab,dest_arg,&conv_count);
    return conv_count;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sparse helper			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError SparseHelperByte
	( SuperFile_t * sf, off_t off, const void * buf, size_t count,
	  WriteFunc write_func, size_t min_chunk_size )
{
    ASSERT(sf);
    ASSERT(write_func);
    ASSERT( min_chunk_size >= sizeof(WDF_Hole_t) );

    TRACE(TRACE_RDWR_FORMAT, "#SH# SparseHelperB()",
	GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    ccp ptr = buf;
    ccp end = ptr + count;

    // skip start hole
    while ( ptr < end && !*ptr )
	ptr++;

    while ( ptr < end )
    {
	ccp data_beg = ptr;
	ccp data_end = data_beg;

	while ( ptr < end )
	{
	    // find end of data
	    while ( ptr < end && *ptr )
		ptr++;
	    data_end = ptr;

	    // skip holes
	    while ( ptr < end && !*ptr )
		ptr++;

	    // accept only holes >= min_chunk_size
	    if ( (ccp)ptr - (ccp)data_end >= min_chunk_size )
		break;
	}

	const size_t wlen = (ccp)data_end - (ccp)data_beg;
	if (wlen)
	{
	    const off_t  woff = off + ( (ccp)data_beg - (ccp)buf );
	    const enumError err = write_func(sf,woff,data_beg,wlen);
	    if (err)
		return err;
	}
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError SparseHelper
	( SuperFile_t * sf, off_t off, const void * buf, size_t count,
	  WriteFunc write_func, size_t min_chunk_size )
{
    ASSERT(sf);
    ASSERT(write_func);

    TRACE(TRACE_RDWR_FORMAT, "#SH# SparseHelper()",
	GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );
    TRACE(" -> write_func = %p, min_chunk_size = %zu\n",write_func, min_chunk_size );

    //----- disable sparse check for already existing file areas

    if ( off < sf->max_virt_off )
    {
	const off_t max_overlap = sf->max_virt_off - off;
	const size_t overlap = count < max_overlap ? count : (size_t) max_overlap;
	const enumError err = write_func(sf,off,buf,overlap);
	count -= overlap;
	if ( err || !count )
	    return err;
	off += overlap;
	buf = (char*)buf + overlap;
    }


    //----- check minimal size

    if ( min_chunk_size < sizeof(WDF_Hole_t) )
	min_chunk_size = sizeof(WDF_Hole_t);
    if ( count < 50 || count < min_chunk_size )
	return SparseHelperByte( sf, off, buf, count, write_func, min_chunk_size );


    //----- check if buf is well aligend
    
    DASSERT( Count1Bits32(sizeof(WDF_Hole_t)) == 1 );
    const size_t align_mask = sizeof(WDF_Hole_t) - 1;

    ccp start = buf;
    const size_t start_align = (size_t)start & align_mask;
    if ( start_align )
    {
	const size_t wr_size =  sizeof(WDF_Hole_t) - start_align;
	const enumError err
	    = SparseHelperByte( sf, off, start, count, write_func, min_chunk_size );
	if (err)
	    return err;
	start += wr_size;
	off   -= wr_size;
	count -= wr_size;
    }


    //----- check aligned data

    WDF_Hole_t * ptr = (WDF_Hole_t*) start;
    WDF_Hole_t * end = (WDF_Hole_t*)( start + ( count & ~align_mask ) );

    // skip start hole
    while ( ptr < end && !*ptr )
	ptr++;

    while ( ptr < end )
    {
	WDF_Hole_t * data_beg = ptr;
	WDF_Hole_t * data_end = data_beg;

	while ( ptr < end )
	{
	    // find end of data
	    while ( ptr < end && *ptr )
		ptr++;
	    data_end = ptr;

	    // skip holes
	    while ( ptr < end && !*ptr )
		ptr++;

	    // accept only holes >= min_chunk_size
	    if ( (ccp)ptr - (ccp)data_end >= min_chunk_size )
		break;
	}

	const size_t wlen = (ccp)data_end - (ccp)data_beg;
	if (wlen)
	{
	    const off_t woff = off + ( (ccp)data_beg - start );
	    const enumError err = write_func(sf,woff,data_beg,wlen);
	    if (err)
		return err;
	}
    }


    //----- check remaining bytes and return

    const size_t remaining_len = count & align_mask;
    if ( remaining_len )
    {
	ASSERT( remaining_len < sizeof(WDF_Hole_t) );
	const off_t woff = off + ( (ccp)end - start );
	const enumError err
	    = SparseHelperByte ( sf, woff, end, remaining_len,
				write_func, min_chunk_size );
	if (err)
	    return err;
    }


    //----- all done

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		 standard read and write wrappers	///////////////
///////////////////////////////////////////////////////////////////////////////

enumError ReadZero
	( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    // dummy read function: fill with zero
    memset(buf,0,count);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadSF
	( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    ASSERT(sf);
    ASSERT(sf->iod.read_func);
    return sf->iod.read_func(sf,off,buf,count);
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSF
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    ASSERT(sf);
    ASSERT(sf->iod.write_func);
    return sf->iod.write_func(sf,off,buf,count);
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSparseSF
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    ASSERT(sf);
    ASSERT(sf->iod.write_sparse_func);
    return sf->iod.write_sparse_func(sf,off,buf,count);
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroSF ( SuperFile_t * sf, off_t off, size_t count )
{
    ASSERT(sf);
    ASSERT(sf->iod.write_zero_func);
    return sf->iod.write_zero_func(sf,off,count);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError ReadSwitchSF
	( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    ASSERT(sf);
    DASSERT(0); // should never called
    switch(sf->iod.oft)
    {
	case OFT_WDF:	return ReadWDF(sf,off,buf,count);
	case OFT_CISO:	return ReadCISO(sf,off,buf,count);
	case OFT_WBFS:	return ReadWBFS(sf,off,buf,count);
	case OFT_FST:	return ReadFST(sf,off,buf,count);
	default:	return ReadISO(sf,off,buf,count);
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSwitchSF
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    ASSERT(sf);
    DASSERT(0); // should never called
    switch(sf->iod.oft)
    {
	case OFT_PLAIN:	return WriteISO(sf,off,buf,count);
	case OFT_WDF:	return WriteWDF(sf,off,buf,count);
	case OFT_CISO:	return WriteCISO(sf,off,buf,count);
	case OFT_WBFS:	return WriteWBFS(sf,off,buf,count);
	default:	return ERROR0(ERR_INTERNAL,0);
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSparseSwitchSF
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    ASSERT(sf);
    DASSERT(0); // should never called
    switch(sf->iod.oft)
    {
	case OFT_PLAIN:	return WriteSparseISO(sf,off,buf,count);
	case OFT_WDF:	return WriteSparseWDF(sf,off,buf,count);
	case OFT_CISO:	return WriteSparseCISO(sf,off,buf,count);
	case OFT_WBFS:	return WriteWBFS(sf,off,buf,count); // no sparse support
	default:	return ERROR0(ERR_INTERNAL,0);
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroSwitchSF ( SuperFile_t * sf, off_t off, size_t count )
{
    ASSERT(sf);
    DASSERT(0); // should never called
    switch(sf->iod.oft)
    {
	case OFT_PLAIN:	return WriteZeroISO(sf,off,count);
	case OFT_WDF:	return WriteZeroWDF(sf,off,count);
	case OFT_CISO:	return WriteZeroCISO(sf,off,count);
	case OFT_WBFS:	return WriteZeroWBFS(sf,off,count);
	default:	return ERROR0(ERR_INTERNAL,0);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError ReadISO
	( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    ASSERT(sf);
    const enumError err = ReadAtF(&sf->f,off,buf,count);

    off += count;
    DASSERT_MSG( err || sf->f.cur_off == off, "%llx : %llx\n",sf->f.cur_off,off);
    if ( sf->max_virt_off < off )
	 sf->max_virt_off = off;
    if ( sf->file_size < off )
	 sf->file_size = off;
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteISO
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    ASSERT(sf);
    const enumError err = WriteAtF(&sf->f,off,buf,count);

    off += count;
    DASSERT_MSG( err || sf->f.cur_off == off, "%llx : %llx\n",sf->f.cur_off,off);
    if ( sf->max_virt_off < off )
	 sf->max_virt_off = off;
    if ( sf->file_size < off )
	 sf->file_size = off;
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSparseISO
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    return SparseHelper(sf,off,buf,count,WriteISO,0);
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroISO ( SuperFile_t * sf, off_t off, size_t size )
{
    // [2do] [zero] optimization

    while ( size > 0 )
    {
	const size_t size1 = size < sizeof(zerobuf) ? size : sizeof(zerobuf);
	const enumError err = WriteISO(sf,off,zerobuf,size1);
	if (err)
	    return err;
	off  += size1;
	size -= size1;
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    set file size		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetSizeSF ( SuperFile_t * sf, off_t off )
{
    DASSERT(sf);
    TRACE("SetSizeSF(%p,%llx) wbfs=%p wc=%p\n",sf,(u64)off,sf->wbfs,sf->wc);

    sf->file_size = off;
    switch (sf->iod.oft)
    {
	case OFT_PLAIN:	return SetSizeF(&sf->f,off);
	case OFT_WDF:	return ERR_OK;
	case OFT_CISO:	return ERR_OK;
	case OFT_WBFS:	return ERR_OK;
	default:	return ERROR0(ERR_INTERNAL,0);
    };
}

///////////////////////////////////////////////////////////////////////////////

enumError SetMinSizeSF ( SuperFile_t * sf, off_t off )
{
    DASSERT(sf);
    TRACE("SetMinSizeSF(%p,%llx) wbfs=%p wc=%p fsize=%llx\n",
		sf, (u64)off, sf->wbfs, sf->wc, (u64)sf->file_size );

    return sf->file_size < off ? SetSizeSF(sf,off) : ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError MarkMinSizeSF ( SuperFile_t * sf, off_t off )
{
    DASSERT(sf);
    TRACE("MarkMinSizeSF(%p,%llx) wbfs=%p wc=%p min_fsize=%llx\n",
		sf, (u64)off, sf->wbfs, sf->wc, (u64)sf->min_file_size );

    if ( sf->f.seek_allowed )
	return SetMinSizeSF(sf,off);

    if ( sf->min_file_size < off )
	 sf->min_file_size = off;
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    ReadWBFS() & WriteWBFS()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError ReadWBFS ( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    TRACE(TRACE_RDWR_FORMAT, "#B# ReadWBFS()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    ASSERT(sf->wbfs);
    ASSERT(sf->wbfs->disc);
    ASSERT(sf->wbfs->disc->header);
    u16 * wlba_tab = sf->wbfs->disc->header->wlba_table;
    ASSERT(wlba_tab);

    wbfs_t * w = sf->wbfs->wbfs;
    ASSERT(w);

    u32 bl = (u32)( off / w->wbfs_sec_sz );
    u32 bl_off = (u32)( off - (u64)bl * w->wbfs_sec_sz );

    while ( count > 0 )
    {
	u32 max_count = w->wbfs_sec_sz - bl_off;
	ASSERT( max_count > 0 );
	if  ( max_count > count )
	    max_count = count;

	const u32 wlba = bl < w->n_wbfs_sec_per_disc ? ntohs(wlba_tab[bl]) : 0;

	TRACE(">> BL=%d[%d], BL-OFF=%x, count=%x/%zx\n",
		bl, wlba, bl_off, max_count, count );

	if (wlba)
	{
	    enumError err = ReadAtF( &sf->f,
				     (off_t)w->wbfs_sec_sz * wlba + bl_off,
				     buf, max_count);
	    if (err)
		return err;
	}
	else
	    memset(buf,0,max_count);

	bl++;
	bl_off = 0;
	count -= max_count;
	buf = (char*)buf + max_count;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteWBFS
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    TRACE(TRACE_RDWR_FORMAT, "#B# WriteWBFS()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    ASSERT(sf->wbfs);
    ASSERT(sf->wbfs->disc);
    ASSERT(sf->wbfs->disc->header);
    u16 * wlba_tab = sf->wbfs->disc->header->wlba_table;
    ASSERT(wlba_tab);

    wbfs_t * w = sf->wbfs->wbfs;
    ASSERT(w);

    u32 bl = (u32)( off / w->wbfs_sec_sz );
    u32 bl_off = (u32)( off - (u64)bl * w->wbfs_sec_sz );

    while ( count > 0 )
    {
	u32 max_count = w->wbfs_sec_sz - bl_off;
	ASSERT( max_count > 0 );
	if  ( max_count > count )
	    max_count = count;

	const u32 wlba = bl < w->n_wbfs_sec_per_disc ? ntohs(wlba_tab[bl]) : 0;
	const off_t off = (off_t)w->wbfs_sec_sz * wlba + bl_off;

	TRACE(">> BL=%d[%d], BL-OFF=%x, count=%x/%zx\n",
		bl, wlba, bl_off, max_count, count );

	if (!wlba)
	{
	    sf->f.last_error = ERR_WRITE_FAILED;
	    if (  sf->f.max_error < ERR_WRITE_FAILED )
		 sf->f.max_error = ERR_WRITE_FAILED;
	    if (!sf->f.disable_errors)
		ERROR0( ERR_WRITE_FAILED, 
			"Can't write to non existing WBFS block [%c=%d,%llx]: %s\n",
			GetFT(&sf->f), GetFD(&sf->f), off, sf->f.fname );
	    return ERR_WRITE_FAILED;
	}
    
	enumError err = WriteAtF( &sf->f,
				 (off_t)w->wbfs_sec_sz * wlba + bl_off,
				 buf, max_count);
	if (err)
	    return err;

	bl++;
	bl_off = 0;
	count -= max_count;
	buf = (char*)buf + max_count;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroWBFS ( SuperFile_t * sf, off_t off, size_t size )
{
    // [2do] [zero] optimization

    while ( size > 0 )
    {
	const size_t size1 = size < sizeof(zerobuf) ? size : sizeof(zerobuf);
	const enumError err = WriteWBFS(sf,off,zerobuf,size1);
	if (err)
	    return err;
	off  += size1;
	size -= size1;
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		SuperFile: read + write wrapper		///////////////
///////////////////////////////////////////////////////////////////////////////

int WrapperReadSF ( void * p_sf, u32 offset, u32 count, void * iobuf )
{
    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    SuperFile_t * sf = (SuperFile_t *)p_sf;
    DASSERT(sf);
    DASSERT(sf->iod.read_func);

    return sf->iod.read_func( sf, (off_t)offset << 2, iobuf, count );
}

///////////////////////////////////////////////////////////////////////////////

int WrapperWriteSF ( void * p_sf, u32 lba, u32 count, void * iobuf )
{
    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    SuperFile_t * sf = (SuperFile_t *)p_sf;
    DASSERT(sf);
    DASSERT(sf->iod.write_func);

    return sf->iod.write_func(
		sf,
		(off_t)lba * WII_SECTOR_SIZE,
		iobuf,
		count * WII_SECTOR_SIZE );
}

///////////////////////////////////////////////////////////////////////////////

int WrapperWriteSparseSF ( void * p_sf, u32 lba, u32 count, void * iobuf )
{
    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    SuperFile_t * sf = (SuperFile_t *)p_sf;
    DASSERT(sf);
    DASSERT(sf->iod.write_sparse_func);

    return sf->iod.write_sparse_func(
		sf,
		(off_t)lba * WII_SECTOR_SIZE,
		iobuf,
		count * WII_SECTOR_SIZE );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                SuperFile_t: progress            ///////////////
///////////////////////////////////////////////////////////////////////////////
// progress and statistics

void CopyProgressSF ( SuperFile_t * dest, SuperFile_t * src )
{
    ASSERT(dest);
    ASSERT(src);

    dest->indent			= src->indent;
    dest->show_progress			= src->show_progress;
    dest->show_summary			= src->show_summary;
    dest->show_msec			= src->show_msec;
    dest->progress_start_time		= src->progress_start_time;
    dest->progress_last_view_sec	= src->progress_last_view_sec;
    dest->progress_last_calc_time	= src->progress_last_calc_time;
    dest->progress_sum_time		= src->progress_sum_time;
    dest->progress_time_divisor		= src->progress_time_divisor;
    dest->progress_max_wd		= src->progress_max_wd;
    dest->progress_verb			= src->progress_verb;
}

///////////////////////////////////////////////////////////////////////////////


void PrintProgressSF ( u64 p_done, u64 p_total, void * param )
{
    SuperFile_t * sf = (SuperFile_t*) param;
    TRACE("PrintProgressSF(%llu,%llu), sf=%p, fd=%d, sh-prog=%d\n",
		p_done, p_total, sf,
		sf ? GetFD(&sf->f) : -2,
		sf ? sf->show_progress : -1 );

    if ( !sf || !sf->show_progress || !p_done || !p_total )
	return;

    if ( p_total > TiB )
    {
	// avoid integer overflow
	p_done  >>= 15;
	p_total >>= 15;
    }

    const u32 now		= GetTimerMSec();
    const u32 elapsed		= now - sf->progress_start_time;
    const u32 view_sec		= elapsed / 1000;
    const u32 rate		= 10000 * p_done / p_total;
    const u32 percent		= rate / 100;
    const u64 total		= sf->f.bytes_read + sf->f.bytes_written;

    TRACE(" - sec=%d->%d, rate=%d, total=%llu=%llu+%llu\n",
		sf->progress_last_view_sec, view_sec, rate,
		total, sf->f.bytes_read, sf->f.bytes_written );

    if ( sf->progress_last_view_sec != view_sec && rate > 0 && total > 0 )
    {
	sf->progress_last_view_sec	= view_sec;
	sf->progress_sum_time		+= ( now - sf->progress_last_calc_time ) * view_sec;
	sf->progress_time_divisor	+= view_sec;
	sf->progress_last_calc_time	= now;

	const u32 elapsed	= now - sf->progress_start_time;
	const u32 eta		= ( view_sec * sf->progress_sum_time * (10000-rate) )
				/ ( sf->progress_time_divisor * rate );

	char buf1[50], buf2[50];
	ccp time1 = PrintMSec(buf1,sizeof(buf1),elapsed,sf->show_msec);
	ccp time2 = PrintMSec(buf2,sizeof(buf2),eta,false);

	TRACE("PROGRESS: now=%7u quot=%8llu/%-5llu perc=%3u ela=%6u eta=%6u [%s,%s]\n",
		now, sf->progress_sum_time, sf->progress_time_divisor,
		percent, elapsed, eta, time1, time2 );

	if ( !sf->progress_verb || !*sf->progress_verb )
	    sf->progress_verb = "copied";

	int wd;
	if ( percent < 10 && view_sec < 10 )
	    printf("%*s%3d%% %s in %s (%3.1f MiB/sec)  %n\r",
		sf->indent,"", percent, sf->progress_verb, time1,
		(double)total * 1000 / MiB / elapsed, &wd );
	else
	    printf("%*s%3d%% %s in %s (%3.1f MiB/sec) -> ETA %s   %n\r",
		sf->indent,"", percent, sf->progress_verb, time1,
		(double)total * 1000 / MiB / elapsed, time2, &wd );

	if ( sf->progress_max_wd < wd )
	    sf->progress_max_wd = wd;
	fflush(stdout);
    }
}

///////////////////////////////////////////////////////////////////////////////
// missing void* support

void PrintSummarySF ( SuperFile_t * sf )
{
    if ( !sf || !sf->show_progress && !sf->show_summary )
	return;

    char buf[200];
    buf[0] = 0;

    if (sf->show_summary)
    {
	const u32 elapsed = GetTimerMSec() - sf->progress_start_time;
	char timbuf[50];
	ccp tim = PrintMSec(timbuf,sizeof(timbuf),elapsed,sf->show_msec);

	u64 total = sf->f.bytes_read + sf->f.bytes_written;
	if (total)
	{
	    if ( !sf->progress_verb || !*sf->progress_verb )
		sf->progress_verb = "copied";

	    snprintf(buf,sizeof(buf),"%*s%4llu MiB %s in %s, %4.1f MiB/sec",
		sf->indent,"", (total+MiB/2)/MiB, sf->progress_verb,
		tim, (double)total * 1000 / MiB / elapsed );
	}
	else
	    snprintf(buf,sizeof(buf),"%*sFinished in %s",
		sf->indent,"", tim );
    }

    if (sf->show_progress)
	printf("%-*s\n",sf->progress_max_wd,buf);
    else
	printf("%s\n",buf);
    fflush(stdout);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     AnalyzeFT()                 ///////////////
///////////////////////////////////////////////////////////////////////////////

enumFileType AnalyzeFT ( File_t * f )
{
    ASSERT(f);

    TRACE("AnalyzeFT(%p) fd=%d, split=%d\n",f,GetFD(f),IsSplittedF(f));

    if (!IsOpenF(f))
    {
	TRACELINE;
	f->ftype = FT_UNKNOWN;

	if ( !f->is_reading )
	    return f->ftype;

	ccp name = strrchr(f->fname,'/');
	if (!name)
	    return f->ftype;

	enum { IS_NONE, IS_ID6, IS_INDEX, IS_SLOT } mode = IS_NONE;
	char id6[7];

	ulong idx = 0;
	name++;
	if ( strlen(name) == 6 )
	{
	    TRACELINE;
	    // maybe an id6
	    int i;
	    for ( i = 0; i < 6; i++ )
		id6[i] = toupper((int)name[i]); // cygwin needs the '(int)'
	    id6[6] = 0;
	    if (CheckID6(id6,false,false))
		mode = IS_ID6;
	}

	if ( mode == IS_NONE )
	{
	    TRACELINE;
	    const bool is_slot = *name == '#';
	    ccp start = name + is_slot;
	    char * end;
	    idx = strtoul(start,&end,10);
	    if ( end > start && !*end )
		mode = is_slot ? IS_SLOT : IS_INDEX;
	    else
		return f->ftype;
	}

	TRACELINE;
	ASSERT( mode != IS_NONE );

	// try open ..
	char fname[PATH_MAX];
	int max = name - f->fname;
	if ( max > sizeof(fname) )
	    max = sizeof(fname);
	memcpy(fname,f->fname,max);
	fname[max-1] = 0;

	SuperFile_t sf;
	InitializeSF(&sf);
	sf.f.disable_errors = true;
	if (OpenFileModify(&sf.f,fname,IOM_IS_WBFS))
	    return f->ftype;

	AnalyzeFT(&sf.f);

	bool ok = false;
	WDiscInfo_t wdisk;
	InitializeWDiscInfo(&wdisk);

	if ( sf.f.ftype & FT_ID_WBFS )
	{
	    TRACE(" - f2=WBFS\n");
	    WBFS_t wbfs;
	    InitializeWBFS(&wbfs);
	    if (!SetupWBFS(&wbfs,&sf,false,0,false))
	    {
		switch(mode)
		{
		    case IS_ID6:
			ok = !FindWDiscInfo(&wbfs,&wdisk,id6);
			break;

		    case IS_INDEX:
			if (!GetWDiscInfo(&wbfs,&wdisk,idx))
			{
			    memcpy(id6,wdisk.id6,6);
			    ok = true;
			}
			break;

		    case IS_SLOT:
			if (!GetWDiscInfoBySlot(&wbfs,&wdisk,idx))
			{
			    memcpy(id6,wdisk.id6,6);
			    ok = true;
			}
			break;

		    default: // suppress warning "IS_NONE not handled in switch"
			break;
		}
	    }
	    TRACELINE;
	    ResetWBFS(&wbfs);
	}

	if (ok)
	{
	    TRACE(" - WBFS/%s found\n",id6);
	    sf.f.disable_errors = f->disable_errors;
	    ASSERT(!sf.f.path);
	    sf.f.path = sf.f.fname;
	    sf.f.fname = f->fname;
	    f->fname = 0;
	    CopyFileAttribDiscInfo(&sf.f.fatt,&wdisk);

	    ResetFile(f,false);
	    memcpy(f,&sf.f,sizeof(*f));
	    CopyPatchedDiscId(f->id6,id6);
	    memset(&sf.f,0,sizeof(sf.f));
	    TRACE(" - WBFS/fname = %s\n",f->fname);
	    TRACE(" - WBFS/path  = %s\n",f->path);
	    TRACE(" - WBFS/id6   = %s\n",f->id6);
	    f->ftype |= FT_A_ISO|FT_A_WDISC;
	}
	else
	    ResetSF(&sf,0);
	return f->ftype;
    }

    enumFileType ft = 0;

    if (S_ISREG(f->st.st_mode))
	ft |= FT_A_REGFILE;

    if (S_ISBLK(f->st.st_mode))
	ft |= FT_A_BLOCKDEV;

    if (S_ISCHR(f->st.st_mode))
	ft |= FT_A_CHARDEV;

    if (f->seek_allowed)
	ft |= FT_A_SEEKABLE;

    if (S_ISDIR(f->st.st_mode))
	return f->ftype = IsFST(f->fname,f->id6);

    if (f->is_writing)
	ft |= FT_A_WRITING;

    if (!f->is_reading)
	return f->ftype;

    //----- now we must analyze the file contents

    // disable warnings
    const bool disable_errors = f->disable_errors;
    f->disable_errors = true;

    // read file header
    char buf1[FILE_PRELOAD_SIZE];
    char buf2[FILE_PRELOAD_SIZE];
    TRACELINE;
    enumError err = ReadAtF(f,0,&buf1,sizeof(buf1));
    if (err)
    {
	TRACELINE;
	ft |= FT_ID_OTHER;
    }
    else
    {
     #ifdef DEBUG
	{
	  ccp ptr = buf1;
	  int i;
	  for ( i = 0; i < 4; i++, ptr+=8 )
	  {
	    TRACE("ANALYZE/HEAD: \"%c%c%c%c%c%c%c%c\"  "
			"%02x %02x %02x %02x  %02x %02x %02x %02x\n",
		ptr[0]>=' ' && ptr[0]<0x7f ? ptr[0] : '.',
		ptr[1]>=' ' && ptr[1]<0x7f ? ptr[1] : '.',
		ptr[2]>=' ' && ptr[2]<0x7f ? ptr[2] : '.',
		ptr[3]>=' ' && ptr[3]<0x7f ? ptr[3] : '.',
		ptr[4]>=' ' && ptr[4]<0x7f ? ptr[4] : '.',
		ptr[5]>=' ' && ptr[5]<0x7f ? ptr[5] : '.',
		ptr[6]>=' ' && ptr[6]<0x7f ? ptr[6] : '.',
		ptr[7]>=' ' && ptr[7]<0x7f ? ptr[7] : '.',
		(u8)ptr[0], (u8)ptr[1], (u8)ptr[2], (u8)ptr[3],
		(u8)ptr[4], (u8)ptr[5], (u8)ptr[6], (u8)ptr[7] );
	  }
	}
     #endif // DEBUG

	ccp data_ptr = buf1;

	WDF_Head_t wh;
	memcpy(&wh,buf1,sizeof(wh));
	ConvertToHostWH(&wh,&wh);

	err = AnalyzeWH(f,&wh,false);
	if ( err != ERR_NO_WDF )
	    ft |= FT_A_WDF;

	if (!err)
	{
	    TRACE(" - WDF found -> load first chunk\n");
	    ft |= FT_A_WDF;
	    data_ptr += sizeof(WDF_Head_t);
	    if ( f->seek_allowed
		&& !ReadAtF(f,wh.chunk_off,&buf2,WDF_MAGIC_SIZE+sizeof(WDF_Chunk_t))
		&& !memcmp(buf2,WDF_MAGIC,WDF_MAGIC_SIZE) )
	    {
		TRACE(" - WDF chunk loaded\n");
		WDF_Chunk_t *wc = (WDF_Chunk_t*)(buf2+WDF_MAGIC_SIZE);
		ConvertToHostWC(wc,wc);
		if ( wh.split_file_index == wc->split_file_index
		    && wc->data_size >= 6 )
		{
		    // save param before clear buffer
		    const off_t off = wc->data_off;
		    const u32 ldlen = wc->data_size < sizeof(buf2)
				    ? wc->data_size : sizeof(buf2);
		    memset(buf2,0,sizeof(buf2));
		    if (!ReadAtF(f,off,&buf2,ldlen))
		    {
			TRACE(" - %d bytes of first chunk loaded\n",ldlen);
			data_ptr = buf2;
		    }
		}
	    }
	}

	if (!memcmp(data_ptr,"CISO",4))
	{
	    CISO_Head_t ch;
	    CISO_Info_t ci;
	    InitializeCISO(&ci,0);
	    if ( !ReadAtF(f,0,&ch,sizeof(ch)) && !SetupCISO(&ci,&ch) )
	    {
		TRACE(" - CISO found -> load part of first block\n");
		ft |= FT_A_CISO;
		if (!ReadAtF(f,sizeof(CISO_Head_t),&buf2,sizeof(buf2)))
		    data_ptr = buf2;
	    }
	    ResetCISO(&ci);
	}

	TRACE("ISO ID6+MAGIC:  \"%c%c%c%c%c%c\"    %02x %02x %02x %02x %02x %02x / %08x\n",
		data_ptr[0]>=' ' && data_ptr[0]<0x7f ? data_ptr[0] : '.',
		data_ptr[1]>=' ' && data_ptr[1]<0x7f ? data_ptr[1] : '.',
		data_ptr[2]>=' ' && data_ptr[2]<0x7f ? data_ptr[2] : '.',
		data_ptr[3]>=' ' && data_ptr[3]<0x7f ? data_ptr[3] : '.',
		data_ptr[4]>=' ' && data_ptr[4]<0x7f ? data_ptr[4] : '.',
		data_ptr[5]>=' ' && data_ptr[5]<0x7f ? data_ptr[5] : '.',
		(u8)data_ptr[0], (u8)data_ptr[1], (u8)data_ptr[2],
		(u8)data_ptr[3], (u8)data_ptr[4], (u8)data_ptr[5],
		*(u32*)(data_ptr + WII_MAGIC_OFF) );

	const enumFileType mt = AnalyzeMemFT(data_ptr,f->st.st_size);
	switch (mt)
	{
	    case FT_ID_WBFS:
		{
		    WBFS_t wbfs;
		    InitializeWBFS(&wbfs);
		    if (!OpenWBFS(&wbfs,f->fname,false,0))
		    {
			ft |= FT_ID_WBFS;
			if ( wbfs.used_discs == 1 )
			{
			    WDiscInfo_t dinfo;
			    InitializeWDiscInfo(&dinfo);
			    if (!GetWDiscInfo(&wbfs,&dinfo,0))
				CopyPatchedDiscId(f->id6,dinfo.id6);
			    ResetWDiscInfo(&dinfo);
			}
		    }
		    ResetWBFS(&wbfs);
		}
		break;

	    case FT_ID_ISO:
		ft |= FT_ID_ISO|FT_A_ISO;
		if ( !(ft&FT_A_WDF) && !f->seek_allowed )
		    DefineCachedAreaISO(f,false);

		CopyPatchedDiscId(f->id6,data_ptr);
		if ( f->st.st_size < ISO_SPLIT_DETECT_SIZE )
		    SetupSplitFile(f,OFT_PLAIN,0);
		break;

	    case FT_ID_HEAD_BIN:
	    case FT_ID_BOOT_BIN:
		ft |= mt;
		CopyPatchedDiscId(f->id6,data_ptr);
		break;

	    default:
		ft |= mt;
	}
    }
    TRACE("ANALYZE FILETYPE: %04x [%s]\n",ft,f->id6);

    // restore warnings
    f->disable_errors = disable_errors;

    return f->ftype = ft;
}

///////////////////////////////////////////////////////////////////////////////

enumFileType AnalyzeMemFT ( const void * preload_buf, off_t file_size )
{
    // make some quick tests for different file formats

    ccp data = preload_buf;
    TRACE("AnalyzeMemFT(,%llx) id=%08x magic=%08x\n",
		(u64)file_size, be32(data), be32(data+WII_MAGIC_OFF) );

    //----- test BOOT.BIN or ISO

    if ( CheckID6(data,false,false) && be32(data+WII_MAGIC_OFF) == WII_MAGIC )
    {
	if ( file_size == WII_BOOT_SIZE )
	    return FT_ID_BOOT_BIN;
	if ( file_size == sizeof(wd_header_t) || file_size == WBFS_INODE_INFO_OFF )
	    return FT_ID_HEAD_BIN;
	if ( !file_size || file_size >= WII_GOOD_UPDATE_PART_OFF )
	    return FT_ID_ISO;
    }


    //----- test WBFS

    if (!memcmp(data,"WBFS",4))
	return FT_ID_WBFS;

    
    int i;
    const u32 check_size = HD_SECTOR_SIZE < file_size ? HD_SECTOR_SIZE : file_size;


    //----- test *.DOL

    const dol_header_t * dol = preload_buf;
    bool ok = true;
    u32 last_off = DOL_HEADER_SIZE;
    for ( i = 0; ok && i < DOL_N_SECTIONS; i++ )
    {
	u32 off  = ntohl(dol->sect_off[i]);
	u32 size = ntohl(dol->sect_size[i]);
	//printf(" %8x %8x %8x %08x\n",last_off,off,size,ntohl(dol->sect_addr[i]));
	if (size)
	{
	    if ( off < last_off )
	    {
		ok = false;
		break;
	    }
	    last_off = off+size;
	}
    }
    if ( ok && last_off > DOL_HEADER_SIZE && last_off <= file_size )
	return FT_ID_DOL;


    //----- test FST.BIN

    static u8 fst_magic[] = { 1,0,0,0, 0,0,0,0, 0 };
    if (!memcmp(data,fst_magic,sizeof(fst_magic)))
    {
	const u32 n_files = be32(data+8);
	const u32 max_name_off = (u32)file_size - n_files * sizeof(wd_fst_item_t);
	u32 max = be32(data+8);
	if ( max_name_off < file_size )
	{
	    if ( max > check_size/sizeof(wd_fst_item_t) )
		 max = check_size/sizeof(wd_fst_item_t);

	    const wd_fst_item_t * fst = preload_buf;
	    for ( i = 0; i < max; i++, fst++ )
	    {
		if ( (ntohl(fst->name_off)&0xffffff) >= max_name_off )
		    break;
		if ( fst->is_dir && ntohl(fst->size) > n_files )
		    break;
	    }
	    if ( i == max )
		return FT_ID_FST_BIN;
	}
    }


    //----- test TMD.BIN

    if ( file_size >= sizeof(wd_tmd_t) )
    {
	const wd_tmd_t * tmd = preload_buf;
	const int n = ntohs(tmd->n_content);
	if ( file_size == sizeof(wd_tmd_t) + n * sizeof(wd_tmd_content_t)
	    && CheckID4(tmd->title_id+4,true,false) )
	{
	    return FT_ID_TMD_BIN;
	}
    }


    //----- test TICKET.BIN

    if ( file_size == sizeof(wd_ticket_t) )
    {
	const wd_ticket_t * tik = preload_buf;
	if ( CheckID4(tik->title_id+4,true,false) )
	    return FT_ID_TIK_BIN;
    }


    //----- fall back to iso

    if ( CheckID6(data,false,false) && be32(data+WII_MAGIC_OFF) == WII_MAGIC )
	return FT_ID_ISO;


    return FT_ID_OTHER;
}

///////////////////////////////////////////////////////////////////////////////

enumError XPrintErrorFT ( XPARM File_t * f, enumFileType err_mask )
{
    ASSERT(f);
    if ( f->ftype == FT_UNKNOWN )
	AnalyzeFT(f);

    enumError stat = ERR_OK;
    enumFileType  and_mask = err_mask &  f->ftype;
    enumFileType nand_mask = err_mask & ~f->ftype;
    TRACE("PRINT FILETYPE ERROR: ft=%04x mask=%04x -> %04x,%04x\n",
		f->ftype, err_mask, and_mask, nand_mask );

    if ( ( f->split_f ? f->split_f[0]->fd : f->fd ) == -1 )
	stat = PrintError( XERROR0, ERR_READ_FAILED,
		"File is not open: %s\n", f->fname );

    else if ( and_mask & FT_ID_DIR )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Is a directory: %s\n", f->fname );

    else if ( nand_mask & FT_A_SEEKABLE )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Wrong file type (not seekable): %s\n", f->fname );

    else if ( nand_mask & (FT_A_REGFILE|FT_A_BLOCKDEV|FT_A_CHARDEV) )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Wrong file type: %s\n", f->fname );

    else if ( nand_mask & FT_ID_WBFS )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Not a WBFS: %s\n", f->fname );

    else if ( nand_mask & FT_ID_ISO || nand_mask & FT_A_ISO )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Not a ISO image: %s\n", f->fname );

    else if ( nand_mask & FT_A_WDF )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Not a WDF: %s\n", f->fname );

    else if ( nand_mask & FT_A_CISO )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Not a CISO: %s\n", f->fname );

    f->last_error = stat;
    if ( f->max_error < stat )
	f->max_error = stat;

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

ccp GetNameFT ( enumFileType ftype, int ignore )
{
    switch ( ftype & FT__ID_MASK )
    {
	case FT_UNKNOWN:
	    return ignore ? 0 : "NO-FILE";

	case FT_ID_DIR:
	    return ignore > 1 ? 0 : "DIR";
	    break;

	case FT_ID_FST:
	    return ftype & FT_A_PART_DIR
			? "FST/PART"
			: "FST";
	    break;

	case FT_ID_WBFS:
	    return ftype & FT_A_WDISC
			? "WBFS/ISO"
			: ignore > 1
				? 0
				: ftype & FT_A_WDF
					? "WDF+WBFS"
					: "WBFS";

	case FT_ID_ISO:
	    return ftype & FT_A_WDF
			? "WDF+ISO"
			: ftype & FT_A_CISO
				? "CISO"
				: "ISO";
	    break;

	case FT_ID_DOL:
	    return ignore > 1
			? 0
			: ftype & FT_A_WDF
				? "WDF+DOL"
				: "DOL";
	    break;

	case FT_ID_TIK_BIN:
	    return ignore > 1
			? 0
			: ftype & FT_A_WDF
				? "WDF+TIK"
				: "TIK.BIN";
	    break;

	case FT_ID_TMD_BIN:
	    return ignore > 1
			? 0
			: ftype & FT_A_WDF
				? "WDF+TMD"
				: "TMD.BIN";
	    break;

	case FT_ID_HEAD_BIN:
	    return ignore > 1
			? 0
			: ftype & FT_A_WDF
				? "WDF+HEAD"
				: "HEAD.BIN";
	    break;

	case FT_ID_BOOT_BIN:
	    return ignore > 1
			? 0
			: ftype & FT_A_WDF
				? "WDF+BOOT"
				: "BOOT.BIN";
	    break;

	case FT_ID_FST_BIN:
	    return ignore > 1
			? 0
			: ftype & FT_A_WDF
				? "W+FST.B"
				: "FST.BIN";
	    break;

	default:
	    return ignore > 1
			? 0
			: ftype & FT_A_WDF
				? "WDF"
				: ftype & FT_A_CISO
					? "C-OTHER"
					: "OTHER";
    }
}

///////////////////////////////////////////////////////////////////////////////

enumOFT GetOFT ( SuperFile_t * sf )
{
    ASSERT(sf);
    return sf->wbfs
		? OFT_WBFS
		: sf->wc
			? OFT_WDF
			: sf->ciso.map
				? OFT_CISO
				: OFT_PLAIN;
}

///////////////////////////////////////////////////////////////////////////////

u32 CountUsedIsoBlocksSF ( SuperFile_t * sf, wd_select_t psel )
{
    ASSERT(sf);

    u32 count = 0;
    if ( psel & WD_SEL_WHOLE_DISC )
	count = sf->file_size * WII_SECTORS_PER_MIB / MiB;
    else
    {
	wd_disc_t *disc = OpenDiscSF(sf,true,true);
	if (disc)
	    count = wd_count_used_disc_blocks_sel(disc,1,psel);
    }
    return count;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    copy functions               ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CopySF ( SuperFile_t * in, SuperFile_t * out, wd_select_t psel )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("---\n");
    TRACE("+++ CopySF(%d->%d,%llx) +++\n",GetFD(&in->f),GetFD(&out->f),(u64)psel);

    if ( out->iod.oft == OFT_WBFS )
	return CopyToWBFS(in,out,psel);

    if ( ! (psel & WD_SEL_WHOLE_DISC ) )
    {
	wd_disc_t * disc = OpenDiscSF(in,true,false);
	if (disc)
	{
	    MarkMinSizeSF(out,in->file_size);
	    wd_filter_usage_table_sel(disc,wdisc_usage_tab,psel);

	    if ( out->iod.oft == OFT_WDF )
	    {
		// write an empty disc header -> makes renaming easier
		enumError err = WriteSF(out,0,zerobuf,WII_TITLE_OFF+WII_TITLE_SIZE);
		if (err)
		    return err;
	    }

	    // encryption && decryption support
	    u8 last_id = 0;
	    aes_key_t akey;
	    bool check_encryption = ( encoding & ENCODE_M_CRYPT ) != 0 && !hook_enabled;
	    bool encrypt = 0, decrypt = 0;
	    ASSERT( sizeof(iobuf) >= 2 * WII_SECTOR_SIZE );
	    char * rdbuf = iobuf; // setup: read_buffer == write_buffer

	    int idx;
	    u64 pr_done = 0, pr_total = 0;
	    if ( out->show_progress )
	    {
		for ( idx = 0; idx < sizeof(wdisc_usage_tab); idx++ )
		    if (wdisc_usage_tab[idx])
			pr_total++;
		PrintProgressSF(0,pr_total,out);
	    }

	    for ( idx = 0; idx < sizeof(wdisc_usage_tab); idx++ )
	    {
		const u8 cur_id = wdisc_usage_tab[idx];
		if (cur_id)
		{
		    if ( SIGINT_level > 1 )
			return ERR_INTERRUPT;
    
		    if ( check_encryption && last_id != cur_id )
		    {
			last_id = cur_id;
			encrypt = decrypt = false;
			rdbuf = iobuf;

			if ( cur_id & WD_USAGE_F_CRYPT )
			{
			    wd_part_t * part = wd_get_part_by_usage(disc,cur_id,0);
			    if ( part && part->is_valid )
			    {
				DASSERT(part->is_loaded);
				encrypt = encoding & ENCODE_ENCRYPT && !part->is_encrypted;
				decrypt = encoding & ENCODE_DECRYPT &&  part->is_encrypted;
				TRACE("PART #%u: ENCRYPT=%d, DECRPYT=%d\n",
					cur_id-2, encrypt, decrypt );
				if ( encrypt || decrypt )
				{
				    rdbuf = iobuf + WII_SECTOR_SIZE;
				    wd_aes_set_key(&akey,part->key);
				}
			    }
			}
		    }

		    off_t off = (off_t)WII_SECTOR_SIZE * idx;
		    enumError err = ReadSF(in,off,rdbuf,WII_SECTOR_SIZE);
		    if (err)
			return err;

		    if ( idx == WII_PTAB_SECTOR && disc->patch_ptab_recommended )
			wd_patch_ptab(disc,iobuf+WII_PTAB_REF_OFF-off,false);

		    if (encrypt)
			EncryptSectors(&akey,rdbuf,iobuf,1);
		    else if (decrypt)
			DecryptSectors(&akey,rdbuf,iobuf,1);

		    err = WriteSparseSF(out,off,iobuf,WII_SECTOR_SIZE);
		    if (err)
			return err;

		    if ( out->show_progress )
			PrintProgressSF(++pr_done,pr_total,out);

		}
	    }
	    if ( out->show_progress || out->show_summary )
		PrintSummarySF(out);
	    return ERR_OK;
	}
    }

    switch (in->iod.oft)
    {
	case OFT_WDF:	return CopyWDF(in,out);
	case OFT_WBFS:	return CopyWBFSDisc(in,out);
	default:	return CopyRaw(in,out);
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyRaw ( SuperFile_t * in, SuperFile_t * out )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("---\n");
    TRACE("+++ CopyRaw(%d,%d) +++\n",GetFD(&in->f),GetFD(&out->f));

    const int align_mask = sizeof(u32)-1;
    ASSERT( (align_mask & align_mask+1) == 0 );

    off_t copy_size = in->file_size;
    off_t off       = 0;

    MarkMinSizeSF(out,copy_size);

    if ( out->show_progress )
	PrintProgressSF(0,in->file_size,out);

    while ( copy_size > 0 )
    {
	if ( SIGINT_level > 1 )
	    return ERR_INTERRUPT;

	u32 size = sizeof(iobuf) < copy_size ? sizeof(iobuf) : (u32)copy_size;
	enumError err = ReadSF(in,off,iobuf,size);
	if (err)
	    return err;

	err = WriteSparseSF(out,off,iobuf,size);
	if (err)
	    return err;

	copy_size -= size;
	off       += size;
	if ( out->show_progress )
	    PrintProgressSF(off,in->file_size,out);
    }

    if ( out->show_progress || out->show_summary )
	PrintSummarySF(out);

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyRawData
	( SuperFile_t * in, SuperFile_t * out, off_t off, off_t copy_size )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("+++ CopyRawData(%d,%d,%llx,%llx) +++\n",
		GetFD(&in->f), GetFD(&out->f), (u64)off, (u64)copy_size );

    while ( copy_size > 0 )
    {
	const u32 size = sizeof(iobuf) < copy_size ? sizeof(iobuf) : (u32)copy_size;
	enumError err = ReadSF(in,off,iobuf,size);
	if (err)
	    return err;

	err = WriteSF(out,off,iobuf,size);
	if (err)
	    return err;

	copy_size -= size;
	off       += size;
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyWDF ( SuperFile_t * in, SuperFile_t * out )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("---\n");
    TRACE("+++ CopyWDF(%d,%d) +++\n",GetFD(&in->f),GetFD(&out->f));

    if ( !in->wc )
	return ERROR0(ERR_INTERNAL,0);

    enumError err = MarkMinSizeSF(out,in->file_size);
    if (err)
	return err;

    u64 pr_done = 0, pr_total = 0;
    if ( out->show_progress )
    {
	int i;
	for ( i = 0; i < in->wc_used; i++ )
	    pr_total += in->wc[i].data_size;
	PrintProgressSF(0,pr_total,out);
    }

    int i;
    for ( i = 0; i < in->wc_used; i++ )
    {
	WDF_Chunk_t *wc = in->wc + i;
	if ( wc->data_size )
	{
	    u64 dest_off = wc->file_pos;
	    u64 src_off  = wc->data_off;
	    u64 size64   = wc->data_size;

	    while ( size64 > 0 )
	    {
		if ( SIGINT_level > 1 )
		    return ERR_INTERRUPT;

		u32 size = sizeof(iobuf);
		if ( size > size64 )
		    size = (u32)size64;

		TRACE("cp #%02d %09llx .. %07x .. %09llx\n",i,src_off,size,dest_off);

		enumError err = ReadAtF(&in->f,src_off,iobuf,size);
		if (err)
		    return err;

		err = WriteSF(out,dest_off,iobuf,size);
		if (err)
		    return err;

		dest_off	+= size;
		src_off		+= size;
		size64		-= size;

		if ( out->show_progress )
		{
		    pr_done += size;
		    PrintProgressSF(pr_done,pr_total,out);
		}
	    }
	}
    }

    if ( out->show_progress || out->show_summary )
	PrintSummarySF(out);

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyWBFSDisc ( SuperFile_t * in, SuperFile_t * out )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("---\n");
    TRACE("+++ CopyWBFSDisc(%d,%d) +++\n",GetFD(&in->f),GetFD(&out->f));

    if ( !in->wbfs )
	return ERROR0(ERR_INTERNAL,0);

    ASSERT(in->wbfs->disc);
    ASSERT(in->wbfs->disc->header);
    u16 * wlba_tab = in->wbfs->disc->header->wlba_table;
    ASSERT(wlba_tab);

    wbfs_t * w = in->wbfs->wbfs;
    ASSERT(w);

    char * copybuf;
    if ( w->wbfs_sec_sz <= sizeof(iobuf) )
	 copybuf = iobuf;
    else
    {
	copybuf = malloc(w->wbfs_sec_sz);
	if (!copybuf)
	    OUT_OF_MEMORY;
    }

    MarkMinSizeSF(out,in->file_size);
    enumError err = ERR_OK;

    u64 pr_done = 0, pr_total = 0;
    if ( out->show_progress )
    {
	int bl;
	for ( bl = 0; bl < w->n_wbfs_sec_per_disc; bl++ )
	    if (ntohs(wlba_tab[bl]))
		pr_total++;
	PrintProgressSF(0,pr_total,out);
    }

    int bl;
    for ( bl = 0; bl < w->n_wbfs_sec_per_disc; bl++ )
    {
	const u32 wlba = ntohs(wlba_tab[bl]);
	if (wlba)
	{
	    err = ReadAtF( &in->f, (off_t)w->wbfs_sec_sz * wlba,
				copybuf, w->wbfs_sec_sz );
	    if (err)
		goto abort;

	    err = WriteSF( out, (off_t)w->wbfs_sec_sz * bl,
				copybuf, w->wbfs_sec_sz );
	    if (err)
		goto abort;

	    if ( out->show_progress )
		PrintProgressSF(++pr_done,pr_total,out);
	}
    }

    if ( out->show_progress || out->show_summary )
	PrintSummarySF(out);

 abort:
    if ( copybuf != iobuf )
	free(copybuf);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyToWBFS ( SuperFile_t * in, SuperFile_t * out, wd_select_t psel )
{
    TRACE("---\n");
    TRACE("+++ CopyToWBFS(%d,%d,%llx) +++\n",in->f.fd,out->f.fd,(u64)psel);

    if ( !out->wbfs )
	return ERROR0(ERR_INTERNAL,0);

    in->indent		= out->indent;
    in->show_progress	= out->show_progress;
    in->show_summary	= out->show_summary;
    in->show_msec	= out->show_msec;

    if ( AddWDisc(out->wbfs,in,psel) > ERR_WARNING )
	return ERROR0(ERR_WBFS,"Error while creating disc [%s] @%s\n",
			out->f.id6, out->f.fname );

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError AppendF
	( File_t * in, SuperFile_t * out, off_t in_off, size_t count )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("AppendF(%d,%d,%llx,%zx) +++\n",
		GetFD(in), GetFD(&out->f), (u64)in_off, count );

    while ( count > 0 )
    {
	const u32 size = sizeof(iobuf) < count ? sizeof(iobuf) : (u32)count;
	enumError err = ReadAtF(in,in_off,iobuf,size);
	TRACE_HEXDUMP16(3,in_off,iobuf,size<0x10?size:0x10);
	if (err)
	    return err;

	err = WriteSF(out,out->max_virt_off,iobuf,size);
	if (err)
	    return err;

	count  -= size;
	in_off += size;
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError AppendSparseF
	( File_t * in, SuperFile_t * out, off_t in_off, size_t count )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("AppendSparseF(%d,%d,%llx,%zx) +++\n",
		GetFD(in), GetFD(&out->f), (u64)in_off, count );

    while ( count > 0 )
    {
	const u32 size = sizeof(iobuf) < count ? sizeof(iobuf) : (u32)count;
	enumError err = ReadAtF(in,in_off,iobuf,size);
	TRACE_HEXDUMP16(3,in_off,iobuf,size<0x10?size:0x10);
	if (err)
	    return err;

	err = WriteSparseSF(out,out->max_virt_off,iobuf,size);
	if (err)
	    return err;

	count  -= size;
	in_off += size;
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError AppendSF
	( SuperFile_t * in, SuperFile_t * out, off_t in_off, size_t count )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("AppendSF(%d,%d,%llx,%zx) +++\n",
		GetFD(&in->f), GetFD(&out->f), (u64)in_off, count );

    while ( count > 0 )
    {
	const u32 size = sizeof(iobuf) < count ? sizeof(iobuf) : (u32)count;
	enumError err = ReadSF(in,in_off,iobuf,size);
	TRACE_HEXDUMP16(3,in_off,iobuf,size<0x10?size:0x10);
	if (err)
	    return err;

	err = WriteSF(out,out->max_virt_off,iobuf,size);
	if (err)
	    return err;

	count  -= size;
	in_off += size;
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError AppendZeroSF ( SuperFile_t * out, off_t count )
{
    ASSERT(out);
    TRACE("AppendZeroSF(%d,%llx) +++\n",GetFD(&out->f),(u64)count);

    return WriteSF(out,out->max_virt_off+count,0,0);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    diff functions               ///////////////
///////////////////////////////////////////////////////////////////////////////

static void PrintDiff ( off_t off, ccp iobuf1, ccp iobuf2, size_t iosize )
{
    while ( iosize > 0 )
    {
	if ( *iobuf1 != *iobuf2 )
	{
	    const size_t bl = off/WII_SECTOR_SIZE;
	    printf("! differ at ISO block #%06zu, off=%#09llx     \n", bl, (u64)off );
	    off_t old = off;
	    off = (bl+1) * (off_t)WII_SECTOR_SIZE;
	    const size_t skip = off - old;
	    if ( skip >= iosize )
		break;
	    iobuf1 += skip;
	    iobuf2 += skip;
	    iosize -= skip;
	}
	else
	{
	    off++;
	    iobuf1++;
	    iobuf2++;
	    iosize--;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError DiffSF
(
	SuperFile_t	* f1,
	SuperFile_t	* f2,
	int		long_count,
	wd_select_t	psel
)
{
    ASSERT(f1);
    ASSERT(f2);
    DASSERT(IsOpenSF(f1));
    DASSERT(IsOpenSF(f2));
    DASSERT(f1->f.is_reading);
    DASSERT(f2->f.is_reading);

    f1->progress_verb = f2->progress_verb = "compared";

    if ( psel & WD_SEL_WHOLE_DISC )
	return DiffRawSF(f1,f2,long_count);

    wd_disc_t * disc1 = OpenDiscSF(f1,true,true);
    wd_disc_t * disc2 = OpenDiscSF(f2,true,true);
    if ( !disc1 || !disc2 )
	return ERR_WDISC_NOT_FOUND;

    wd_filter_usage_table_sel(disc1,wdisc_usage_tab, psel);
    wd_filter_usage_table_sel(disc2,wdisc_usage_tab2,psel);
    
    int idx;
    u64 pr_done = 0, pr_total = 0;
    bool differ = false;

    const bool have_mod_list = f1->modified_list.used || f2->modified_list.used;

    for ( idx = 0; idx < sizeof(wdisc_usage_tab); idx++ )
    {
	if (   wd_usage_class_tab[wdisc_usage_tab [idx]]
	    != wd_usage_class_tab[wdisc_usage_tab2[idx]] )
	{
	    differ = true;
	    if (!long_count)
		goto abort;
	    wdisc_usage_tab[idx] = 1; // mark for future
	}

	if ( wdisc_usage_tab[idx] )
	{
	    pr_total++;

	    if (have_mod_list)
	    {
		// mark blocks for delayed comparing
		wdisc_usage_tab2[idx] = 0;
		off_t off = (off_t)WII_SECTOR_SIZE * idx;
		if (   FindMemMap(&f1->modified_list,off,WII_SECTOR_SIZE)
		    || FindMemMap(&f2->modified_list,off,WII_SECTOR_SIZE) )
		{
		    TRACE("DIFF BLOCK #%u (off=%llx) delayed.\n",idx,(u64)off);
		    wdisc_usage_tab[idx] = 0;
		    wdisc_usage_tab2[idx] = 1;
		}
	    }
	}
    }

    if ( f2->show_progress )
	PrintProgressSF(0,pr_total,f2);

    ASSERT( sizeof(iobuf) >= 2*WII_SECTOR_SIZE );
    char *iobuf1 = iobuf, *iobuf2 = iobuf + WII_SECTOR_SIZE;

    int run = 0;
    for(;;)
    {
	TRACE("** DIFF LOOP, run=%d, have_mod_list=%d\n",run,have_mod_list);
	for ( idx = 0; idx < sizeof(wdisc_usage_tab); idx++ )
	{
	    if ( SIGINT_level > 1 )
		return ERR_INTERRUPT;

	    if (wdisc_usage_tab[idx])
	    {
		off_t off = (off_t)WII_SECTOR_SIZE * idx;
		TRACE(" - DIFF BLOCK #%u (off=%llx).\n",idx,(u64)off);
		enumError err = ReadSF(f1,off,iobuf1,WII_SECTOR_SIZE);
		if (err)
		    return err;

		err = ReadSF(f2,off,iobuf2,WII_SECTOR_SIZE);
		if (err)
		    return err;

		if ( idx == WII_PTAB_SECTOR )
		{
		    wd_patch_ptab(disc1,iobuf1+WII_PTAB_REF_OFF-off,true);
		    wd_patch_ptab(disc2,iobuf2+WII_PTAB_REF_OFF-off,true);
		}

		if (memcmp(iobuf1,iobuf2,WII_SECTOR_SIZE))
		{
		    differ = true;
		    if (long_count)
			PrintDiff(off,iobuf1,iobuf2,WII_SECTOR_SIZE);
		    if ( long_count < 2 )
			break;
		}

		if ( f2->show_progress )
		    PrintProgressSF(++pr_done,pr_total,f2);
	    }
	}
	if ( !have_mod_list || run++ )
	    break;
	    
	memcpy(wdisc_usage_tab,wdisc_usage_tab2,sizeof(wdisc_usage_tab));
	UpdateSignatureFST(f1->fst); // NULL allowed
	UpdateSignatureFST(f2->fst); // NULL allowed
    }

    if ( f2->show_progress || f2->show_summary )
	PrintSummarySF(f2);

 abort:

    return differ ? ERR_DIFFER : ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError DiffRawSF
(
	SuperFile_t * f1,
	SuperFile_t * f2,
	int long_count
)
{
    ASSERT(f1);
    ASSERT(f2);
    ASSERT(IsOpenSF(f1));
    ASSERT(IsOpenSF(f2));
    ASSERT(f1->f.is_reading);
    ASSERT(f2->f.is_reading);

    if ( f1->iod.oft == OFT_WBFS || f2->iod.oft == OFT_WBFS )
    {
	if ( f1->iod.oft == OFT_WBFS )
	    f2->f.read_behind_eof = 2;
	if ( f2->iod.oft == OFT_WBFS )
	    f1->f.read_behind_eof = 2;
    }
    else if ( f1->file_size != f2->file_size )
	return ERR_DIFFER;

    const size_t io_size = sizeof(iobuf)/2;
    ASSERT( (io_size&511) == 0 );
    char *iobuf1 = iobuf, *iobuf2 = iobuf + io_size;

    off_t off = 0;
    off_t total_size = f1->file_size;
    if ( total_size < f2->file_size )
	 total_size = f2->file_size;
    off_t size = total_size;

    if ( f2->show_progress )
	PrintProgressSF(0,total_size,f2);

    bool differ = false;
    while ( size > 0 )
    {
	if (SIGINT_level>1)
	    return ERR_INTERRUPT;

	const size_t cmp_size = io_size < size ? io_size : (size_t)size;

	enumError err;
	err = ReadSF(f1,off,iobuf1,cmp_size);
	if (err)
	    return err;
	err = ReadSF(f2,off,iobuf2,cmp_size);
	if (err)
	    return err;

	if (memcmp(iobuf1,iobuf2,cmp_size))
	{
	    differ = true;
	    if (long_count)
		PrintDiff(off,iobuf1,iobuf2,cmp_size);
	    if ( long_count < 2 )
		break;
	}

	off  += cmp_size;
	size -= cmp_size;

	if ( f2->show_progress )
	    PrintProgressSF(off,total_size,f2);
    }

    if ( f2->show_progress || f2->show_summary )
	PrintSummarySF(f2);

    return differ ? ERR_DIFFER : ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError DiffFilesSF
(
	SuperFile_t	* f1,
	SuperFile_t	* f2,
	int		long_count,
	FilePattern_t	*pat,
	wd_select_t	psel,
	wd_ipm_t	pmode
)
{
    ASSERT(f1);
    ASSERT(f2);
    DASSERT(IsOpenSF(f1));
    DASSERT(IsOpenSF(f2));
    DASSERT(f1->f.is_reading);
    DASSERT(f2->f.is_reading);
    ASSERT(pat);


    //----- setup fst

    wd_disc_t * disc1 = OpenDiscSF(f1,true,true);
    wd_disc_t * disc2 = OpenDiscSF(f2,true,true);
    if ( !disc1 || !disc2 )
	return ERR_WDISC_NOT_FOUND;

    wd_select(disc1,psel);
    wd_select(disc2,psel);
    
    WiiFst_t fst1, fst2;
    InitializeFST(&fst1);
    InitializeFST(&fst2);

    CollectFST(&fst1,disc1,GetDefaultFilePattern(),false,pmode,false);
    CollectFST(&fst2,disc2,GetDefaultFilePattern(),false,pmode,false);

    SortFST(&fst1,SORT_NAME,SORT_NAME);
    SortFST(&fst2,SORT_NAME,SORT_NAME);


    //----- define variables

    int differ = 0;
    enumError err = ERR_OK;

    const int BUF_SIZE = sizeof(iobuf) / 2;
    u8 * buf1 = (u8*)iobuf;
    u8 * buf2 = buf1 + BUF_SIZE;

    u64 done_count = 0, total_count = 0;
    f1->progress_verb = f2->progress_verb = "compared";


    //----- first find solo partions

    int ip1, ip2;
    for ( ip1 = 0; ip1 < fst1.part_used; ip1++ )
    {
	WiiFstPart_t * p1 = fst1.part + ip1;
	
	// find a partition with equal type in fst2

	WiiFstPart_t * p2 = 0;
	for ( ip2 = 0; ip2 < fst2.part_used; ip2++ )
	{
	    WiiFstPart_t * p = fst2.part + ip2;
	    if ( !p->done && p->part_type == p1->part_type )
	    {
		p2 = p;
		p2->done++;
		break;
	    }
	}

	if (!p2)
	{
	    p1->done++;
	    differ++;
	    if (verbose<0)
		goto abort;
	    printf("Only in ISO #1:   Partition #%u, type=%x %s\n",
		ip1, p1->part_type, wd_get_part_name(p1->part_type,"") );
	    continue;
	}

	ASSERT(p1);
	ASSERT(p2);
	total_count += p1->file_used + p2->file_used;
    }

    for ( ip2 = 0; ip2 < fst2.part_used; ip2++ )
    {
	WiiFstPart_t * p2 = fst2.part + ip2;
	if (!p2->done)
	{
	    differ++;
	    if (verbose<0)
		goto abort;
	    printf("Only in ISO #2:   Partition #%u, type=%x %s\n",
		ip2, p2->part_type, wd_get_part_name(p2->part_type,"") );
	}
	// reset done flag
	p2->done = 0;
    }


    //----- compare files

    if ( f2->show_progress )
	PrintProgressSF(0,total_count,f2);

    for ( ip1 = 0; ip1 < fst1.part_used; ip1++ )
    {
	WiiFstPart_t * p1 = fst1.part + ip1;
	if (p1->done++)
	    continue;
	
	// find a partition with equal type in fst2

	WiiFstPart_t * p2 = 0;
	for ( ip2 = 0; ip2 < fst2.part_used; ip2++ )
	{
	    WiiFstPart_t * p = fst2.part + ip2;
	    if ( !p->done && p->part_type == p1->part_type )
	    {
		p2 = p;
		break;
	    }
	}
	if (!p2)
	    continue;
	p2->done++;

	//----- compare files

	ASSERT(p1);
	ASSERT(p2);

	const WiiFstFile_t *file1 = p1->file;
	const WiiFstFile_t *end1  = file1 + p1->file_used;
	const WiiFstFile_t *file2 = p2->file;
	const WiiFstFile_t *end2  = file2 + p2->file_used;

	static char only_in[] = "Only in ISO #%u:   %s%s\n";

	while ( file1 < end1 && file2 < end2 )
	{
	    if (SIGINT_level>1)
		return ERR_INTERRUPT;

	    if ( f2->show_progress )
		PrintProgressSF(done_count,total_count,f2);

	    int stat = strcmp(file1->path,file2->path);
	    if ( stat < 0 )
	    {	
		differ++;
		if (verbose<0)
		    goto abort;
		printf(only_in,1,p1->path,file1->path);
		file1++;
		done_count++;
		continue;
	    }

	    if ( stat > 0 )
	    {	
		differ++;
		if (verbose<0)
		    goto abort;
		printf(only_in,2,p2->path,file2->path);
		file2++;
		done_count++;
		continue;
	    }

	    if ( file1->icm != file2->icm )
	    {
		// this should never happen
		differ++;
		if (verbose<0)
		    goto abort;
		printf("File types differ: %s%s\n", p1->path, file1->path );
	    }
	    else if ( file1->icm > WD_ICM_DIRECTORY )
	    {
		if ( file1->size != file2->size )
		{
		    differ++;
		    if (verbose<0)
			goto abort;
		    if ( file1->size < file2->size )
			printf("File size differ: %s%s -> %u+%u=%u\n", p1->path, file1->path,
				    file1->size, file2->size - file1->size, file2->size );
		    else
			printf("File size differ: %s%s -> %u-%u=%u\n", p1->path, file1->path,
				    file1->size, file1->size - file2->size, file2->size );
		}
		else
		{
		    u32 off4 = 0;
		    u32 size = file1->size;

		    while ( size > 0 )
		    {
			const u32 read_size = size < BUF_SIZE ? size : BUF_SIZE;
			if (   ReadFileFST(p1,file1,off4,buf1,read_size)
			    || ReadFileFST(p2,file2,off4,buf2,read_size) )
			{
			    err = ERR_READ_FAILED;
			    goto abort;
			}

			if (memcmp(buf1,buf2,read_size))
			{
			    differ++;
			    if (verbose<0)
				goto abort;
			    printf("Files differ:     %s%s\n", p1->path, file1->path );
			    break;
			}

			size -= read_size;
			off4 += read_size>>2;
		    }
		}
	    }

	    file1++;
	    file2++;
	    done_count += 2;
	}

	while ( file1 < end1 )
	{
	    differ++;
	    if (verbose<0)
		goto abort;
	    printf(only_in,1,p1->path,file1->path);
	    file1++;
	}

	while ( file2 < end2 )
	{
	    differ++;
	    if (verbose<0)
		goto abort;
	    printf(only_in,2,p2->path,file2->path);
	    file2++;
	}
    }

abort: 

    if ( done_count && ( f2->show_progress || f2->show_summary ) )
	PrintSummarySF(f2);

    ResetFST(&fst1);
    ResetFST(&fst2);

    return err ? err : differ ? ERR_DIFFER : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  source iterator                ///////////////
///////////////////////////////////////////////////////////////////////////////

static StringField_t dir_done_list;
static StringField_t file_done_list;

//-----------------------------------------------------------------------------

void InitializeIterator ( Iterator_t * it )
{
    ASSERT(it);
    memset(it,0,sizeof(*it));
    InitializeStringField(&it->source_list);
}

//-----------------------------------------------------------------------------

void ResetIterator ( Iterator_t * it )
{
    ASSERT(it);
    ResetStringField(&it->source_list);
    memset(it,0,sizeof(*it));
}

//-----------------------------------------------------------------------------

static enumError SourceIteratorHelper
	( Iterator_t * it, ccp path, bool collect_fnames )
{
    ASSERT(it);
    ASSERT( it->func || collect_fnames );
    TRACE("SourceIteratorHelper(%p,%s,%d) depth=%d/%d\n",
		it, path, collect_fnames, it->depth, it->max_depth );

    if ( collect_fnames && path && *path == '-' && !path[1] )
    {
	TRACE(" - ADD STDIN\n");
	InsertStringField(&it->source_list,path,false);
	return ERR_OK;
    }

 #ifdef __CYGWIN__
    char goodpath[PATH_MAX];
    NormalizeFilenameCygwin(goodpath,sizeof(goodpath),path);
    path = goodpath;
 #endif

    char buf[PATH_MAX+10];

    SuperFile_t sf;
    InitializeSF(&sf);
    enumError err = ERR_OK;

    //----- directory part

    if ( !stat(path,&sf.f.st) && S_ISDIR(sf.f.st.st_mode) )
    {
	if ( it->act_fst >= ACT_ALLOW )
	{
	    enumFileType ftype = IsFST(path,0);
	    if ( ftype & FT_ID_FST )
	    {
		sf.f.ftype = ftype;
		sf.allow_fst = it->act_fst >= ACT_EXPAND && !collect_fnames;
		goto check_file;
	    }
	}

	if ( it->depth >= it->max_depth )
	{
	    ResetSF(&sf,0);
	    return ERR_OK;
	}

	ccp real_path = realpath(path,buf);
	if (!real_path)
	    real_path = path;

	if (InsertStringField(&dir_done_list,real_path,false))
	{
	    it->num_of_dirs++;
	    DIR * dir = opendir(path);
	    if (dir)
	    {
		char * bufend = buf+sizeof(buf);
		char * dest = StringCopyE(buf,bufend-1,path);
		if ( dest > buf && dest[-1] != '/' )
		    *dest++ = '/';

		it->depth++;

		const enumAction act_non_exist = it->act_non_exist;
		const enumAction act_non_iso   = it->act_non_iso;
		it->act_non_exist = it->act_non_iso = ACT_IGNORE;

		while ( err == ERR_OK && !SIGINT_level )
		{
		    struct dirent * dent = readdir(dir);
		    if (!dent)
			break;
		    ccp n = dent->d_name;
		    if ( n[0] != '.' )
		    {
			StringCopyE(dest,bufend,dent->d_name);
			err = SourceIteratorHelper(it,buf,collect_fnames);
		    }
		}
		closedir(dir);

		it->act_non_exist = act_non_exist;
		it->act_non_iso   = act_non_iso;
		it->depth--;
	    }
	}
	ResetSF(&sf,0);
	return err ? err : SIGINT_level ? ERR_INTERRUPT : ERR_OK;
    }

    //----- file part
 
 check_file:
    sf.f.disable_errors = it->act_non_exist != ACT_WARN;
    err = OpenSF(&sf,path,it->act_non_iso||it->act_wbfs>=ACT_ALLOW,it->open_modify);
    if ( err != ERR_OK && err != ERR_CANT_OPEN )
    {
    	ResetSF(&sf,0);
	return ERR_OK;
    }
    sf.f.disable_errors = false;

    ccp real_path = realpath( sf.f.path ? sf.f.path : sf.f.fname, buf );
    if (!real_path)
	real_path = path;
    it->real_path = real_path;

    if ( !IsOpenSF(&sf) )
    {
	if ( it->act_non_exist >= ACT_ALLOW )
	{
	    it->num_of_files++;
	    if (collect_fnames)
	    {
		InsertStringField(&it->source_list,sf.f.fname,false);
		err = ERR_OK;
	    }
	    else
		err = it->func(&sf,it);
	    ResetSF(&sf,0);
	    return err;
	}
	if ( err != ERR_CANT_OPEN && it->act_non_exist == ACT_WARN )
	    ERROR0(ERR_CANT_OPEN, "Can't open file X: %s\n",path);
	goto abort;
    }
    else if ( it->act_open < ACT_ALLOW
		&& sf.f.st.st_dev == it->open_dev
		&& sf.f.st.st_ino == it->open_ino )
    {
	if ( it->act_open == ACT_WARN )
	    printf(" - Ignore input=output: %s\n",path);
	ResetSF(&sf,0);
	return ERR_OK;
    }
    err = ERR_OK;

    if ( sf.f.ftype & FT_A_WDISC )
    {
	ccp slash = strrchr(path,'/');
	if (slash)
	{
	    StringCat2S(buf,sizeof(buf),it->real_path,slash);
	    real_path = buf;
	}
    }

    if ( it->act_wbfs >= ACT_EXPAND
	&& ( sf.f.ftype & (FT_ID_WBFS|FT_A_WDISC) ) == FT_ID_WBFS )
    {
	it->num_of_files++;
	WBFS_t wbfs;
	InitializeWBFS(&wbfs);
	if (!SetupWBFS(&wbfs,&sf,false,0,false))
	{
	    const int max_disc = wbfs.wbfs->max_disc;
	    int fw, slot;
	    for ( slot = max_disc-1; slot >= 0
			&& !wbfs.wbfs->head->disc_table[slot]; slot-- )
		;

	 #if 0 // [obsolete]
	    if (!slot)
	    {
		// only slot #0 is used!
		if ( collect_fnames )
		{
		    InsertStringField(&it->source_list,sf.f.fname,false);
		    err = ERR_OK;
		}
		else
		    err = it->func(&sf,it);
		ResetWBFS(&wbfs);
		ResetSF(&sf,0);
		return err;
	    }
	 #endif

	    char fbuf[PATH_MAX+10];
	    snprintf(fbuf,sizeof(fbuf),"%u%n",slot,&fw);
	    char * dest = StringCopyS(fbuf,sizeof(fbuf)-10,path);
	    *dest++ = '/';

	    if ( collect_fnames )
	    {
		// fast scan of wbfs
		WDiscInfo_t wdisk;
		InitializeWDiscInfo(&wdisk);
		for ( slot = 0; slot < max_disc; slot++ )
		{
		    if ( !GetWDiscInfoBySlot(&wbfs,&wdisk,slot)
			&& !IsExcluded(wdisk.id6) )
		    {
			snprintf(dest,fbuf+sizeof(fbuf)-dest,"#%0*u",fw,slot);
			InsertStringField(&it->source_list,fbuf,false);
		    }
		}
		ResetWDiscInfo(&wdisk);
		ResetWBFS(&wbfs);
		ResetSF(&sf,0);
		return ERR_OK;
	    }

	    // make a copy of the disc table because we want close the wbfs
	    u8 discbuf[512];
	    u8 * disc_table = max_disc <= sizeof(discbuf)
				? discbuf
				: malloc(sizeof(discbuf));
	    memcpy(disc_table,wbfs.wbfs->head->disc_table,max_disc);

	    ResetWBFS(&wbfs);
	    ResetSF(&sf,0);

	    for ( slot = 0; slot < max_disc && err == ERR_OK; slot++ )
	    {
		if (disc_table[slot])
		{
		    snprintf(dest,fbuf+sizeof(fbuf)-dest,"#%0*u",fw,slot);
		    err = SourceIteratorHelper(it,fbuf,collect_fnames);
		}
	    }

	    if ( disc_table != discbuf )
		free(disc_table);
	}
	else
	    ResetSF(&sf,0);
	return err ? err : SIGINT_level ? ERR_INTERRUPT : ERR_OK;
    }

    if ( sf.f.ftype & FT__SPC_MASK )
    {
	const enumAction action = it->act_non_iso > it->act_known
				? it->act_non_iso : it->act_known;
	if ( action < ACT_ALLOW ) 
	{
	    if ( action == ACT_WARN )
		PrintErrorFT(&sf.f,FT_A_ISO);
	    goto abort;
	}
    }
    else  if (!sf.f.id6[0])
    {
	const enumAction action = sf.f.ftype & FT_ID_WBFS
				? it->act_wbfs : it->act_non_iso;
	if ( action < ACT_ALLOW ) 
	{
	    if ( action == ACT_WARN )
		PrintErrorFT(&sf.f,FT_A_ISO);
	    goto abort;
	}
    }

    it->num_of_files++;
    if ( InsertStringField(&file_done_list,real_path,false)
	&& ( !sf.f.id6[0] || !IsExcluded(sf.f.id6) ))
    {
	if (collect_fnames)
	{
	    InsertStringField(&it->source_list,sf.f.fname,false);
	    err = ERR_OK;
	}
	else
	    err = it->func(&sf,it);
    }

 abort:
    ResetSF(&sf,0);
    return err ? err : SIGINT_level ? ERR_INTERRUPT : ERR_OK;
}

//-----------------------------------------------------------------------------

static enumError SourceIteratorStarter
	( Iterator_t * it, ccp path, bool collect_fnames )
{
    ASSERT(it);
    const u32 num_of_files = it->num_of_files;
    const u32 num_of_dirs  = it->num_of_dirs;

    const enumError err =SourceIteratorHelper(it,path,collect_fnames);
    
    if ( it->num_of_dirs > num_of_dirs && it->num_of_files == num_of_files )
	it->num_empty_dirs++;

    return err;
}

//-----------------------------------------------------------------------------

enumError SourceIterator
(
	Iterator_t * it,
	int warning_mode,
	bool current_dir_is_default,
	bool collect_fnames
)
{
    ASSERT(it);
    ASSERT( it->func || collect_fnames );
    TRACE("SourceIterator(%p) func=%p, act=%d,%d,%d\n",
		it, it->func,
		it->act_non_exist, it->act_non_iso, it->act_wbfs );

    if ( current_dir_is_default && !source_list.used && !recurse_list.used )
	AppendStringField(&source_list,".",false);

    if ( it->act_wbfs < it->act_non_iso )
	it->act_wbfs = it->act_non_iso;

    InitializeStringField(&dir_done_list);
    InitializeStringField(&file_done_list);

    ccp *ptr, *end;
    enumError err = ERR_OK;

    it->depth = 0;
    it->max_depth = opt_recurse_depth;
    for ( ptr = recurse_list.field, end = ptr + recurse_list.used;
		err == ERR_OK && !SIGINT_level && ptr < end; ptr++ )
	err = SourceIteratorStarter(it,*ptr,collect_fnames);

    it->depth = 0;
    it->max_depth = 1;
    for ( ptr = source_list.field, end = ptr + source_list.used;
		err == ERR_OK && !SIGINT_level && ptr < end; ptr++ )
	err = SourceIteratorStarter(it,*ptr,collect_fnames);

    ResetStringField(&dir_done_list);
    ResetStringField(&file_done_list);

    return warning_mode > 0
		? SourceIteratorWarning(it,err,warning_mode==1)
		: err;
}

//-----------------------------------------------------------------------------

enumError SourceIteratorCollected ( Iterator_t * it, int warning_mode )
{
    ASSERT(it);
    ASSERT(it->func);
    TRACE("SourceIteratorCollected(%p) count=%d\n",it,it->source_list.used);

    ResetStringField(&file_done_list);

    enumError err = ERR_OK;
    int idx;
    for ( idx = 0; idx < it->source_list.used && !SIGINT_level; idx++ )
    {
	it->source_index = idx;
	err = SourceIteratorStarter(it,it->source_list.field[idx],false);
    }

    return warning_mode > 0
		? SourceIteratorWarning(it,err,warning_mode==1)
		: err;
}

//-----------------------------------------------------------------------------

enumError SourceIteratorWarning ( Iterator_t * it, enumError max_err, bool silent )
{
    if ( it->num_empty_dirs && max_err < ERR_NO_SOURCE_FOUND )
    {
	if (silent)
	    max_err = ERR_NO_SOURCE_FOUND;
	else if ( it->num_empty_dirs > 1 )
	    max_err = ERROR0(ERR_NO_SOURCE_FOUND,
			"%u directories without valid source files found.\n",
			it->num_empty_dirs);
	else
	    max_err = ERROR0(ERR_NO_SOURCE_FOUND,
			"A directory without valid source files found.\n");
    }

    if ( !it->num_of_files && max_err < ERR_NOTHING_TO_DO )
    {
	if (silent)
	    max_err = ERR_NOTHING_TO_DO;
	else
	    max_err = ERROR0(ERR_NOTHING_TO_DO,"No valid source file found.\n");
    }

    return max_err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

