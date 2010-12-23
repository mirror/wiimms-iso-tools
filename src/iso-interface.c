
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
#include <errno.h>
#include <ctype.h>

#include "debug.h"
#include "iso-interface.h"
#include "wbfs-interface.h"
#include "titles.h"
#include "match-pattern.h"
#include "dirent.h"
#include "crypt.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 dump helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static void dump_data
	( FILE * f, int indent, off_t base, u32 off4, off_t size, ccp text )
{
    const u64 off = (off_t)off4 << 2;
    const u64 end = off + size;
    fprintf(f,"%*s"
	"    %-5s %9llx .. %9llx -> %9llx .. %9llx, size:%10llx/hex =%11llu\n",
	indent,"", text, off, end,
	(u64)base+off, (u64)base+end, (u64)size, (u64)size );
}

//-----------------------------------------------------------------------------

static void dump_size
(
    FILE		* f,		// output stream
    int			indent,		// indent
    ccp			title,		// header
    u64			size,		// size to print
    u64			percent_base1,	// >0: print percent value based on 'percent_base'
    u64			percent_base2	// >0: print percent value based on 'percent_base'
)
{
    fprintf(f,"%*s%-15s%11llx/hex =%11llu",
	indent,"", title, size, size );

    if ( size >= 10240 )
	fprintf(f," = %s",wd_print_size_1024(0,0,size,true));

    if ( percent_base1 > 0 )
    {
	double percent = size * 100.0 / percent_base1;
	if ( percent >= 9.95 )
	    fprintf(f,", %4.1f%%",percent);
	else
	    fprintf(f,", %4.2f%%",percent);
    }

    if ( percent_base2 > 0 )
    {
	double percent = size * 100.0 / percent_base2;
	if ( percent >= 9.95 )
	    fprintf(f,", %4.1f%%",percent);
	else
	    fprintf(f,", %4.2f%%",percent);
    }
    
    fputc('\n',f);
}

//-----------------------------------------------------------------------------

static int dump_header
(
    FILE		* f,		// output stream
    int			indent,		// indent
    SuperFile_t		* sf,		// file to dump
    ccp			id6,		// NULL or id6
    ccp			real_path,	// NULL or pointer to real path
    ccp			user_head,	// NULL or user defined field
    ccp			user_parm	// NULL or parameter for 'user_head'
)
{
    ASSERT(f);
    ASSERT(sf);
    indent = NormalizeIndent(indent);

    fprintf(f,"\n%*sDump of file %s\n\n",indent,"",sf->f.fname);
    indent += 2;
    if ( real_path && *real_path && strcmp(sf->f.fname,real_path) )
	fprintf(f,"%*sReal path:         %s\n",indent,"",real_path);

    const u64 virt_size = sf->file_size;
    const u64 real_size = sf->f.st.st_size;
    if ( sf->iod.oft != OFT_UNKNOWN && sf->disc2 )
    {
	if ( sf->iod.oft == OFT_PLAIN )
	    dump_size(f,indent,"ISO file size:",real_size,0,0);
	else
	    dump_size(f,indent,"Virtual size:",virt_size,0,0);

	const u64 scrubbed_size = wd_count_used_disc_blocks(sf->disc2,1,0)
				* (u64)WII_SECTOR_SIZE;
	dump_size(f,indent,"Scrubbed size:",scrubbed_size,virt_size,0);

	if ( sf->iod.oft != OFT_PLAIN )
	{
	    char buf[50];
	    snprintf(buf,sizeof(buf),"%s file size:",oft_info[sf->iod.oft].name);
	    dump_size(f,indent,buf,real_size,virt_size,scrubbed_size);
	}
    }
    else
	dump_size(f,indent,"File size:",real_size,0,0);

    if ( user_head && user_parm )
	fprintf(f,"%*s%-19s%s\n",
		indent,"", user_head, user_parm );
	
    char info[30] = {0};
    if (sf->wia)
    {
	wia_disc_t * disc = &sf->wia->disc;
	snprintf( info, sizeof(info), " (v%s,%s)",
		PrintVersionWIA(0,0,sf->wia->fhead.version),
		wd_print_compression(0,0,disc->compression,
				disc->compr_level,disc->chunk_size,2) );
    }

    if (!id6)
	id6 = sf->f.id6;
    if (*id6)
	fprintf(f,"%*sID & file type:    %s, %s%s\n",
		indent,"", wd_print_id(id6,6,0), GetNameFT(sf->f.ftype,0), info );
    else
	fprintf(f,"%*sFile type:         %s%s\n",
		indent,"", GetNameFT(sf->f.ftype,0), info );

    return indent;
}

//-----------------------------------------------------------------------------

static void dump_sig_type
(
    FILE		* f,		// output stream
    int			indent,		// indent
    u32			sig_type,	// signature or public key type
    cert_stat_t		sig_status,	// not NULL: cert status of signature
    bool		is_pubkey	// ture: nit sig but public key
)
{
    const int size = is_pubkey
			? cert_get_pubkey_size(sig_type)
			: cert_get_signature_size(sig_type);

    fprintf(f,"%*s%-17s%11x/hex =%11u  [%s, size=%u%s%s]\n",
		indent, "",
		is_pubkey ? "Public key type:" : "Signature type:",
		sig_type, sig_type,
		cert_get_signature_name(sig_type|0x10000,"?"),
		size,
		sig_status ? ", " : "",
		sig_status ? cert_get_status_name(sig_status,"?") : "" );

 #if 0
    if (sig_status)
	fprintf(f,"%*s%-19s%s\n",
		indent, "",
		is_pubkey ? "Public key status:" : "Signature status:",
		cert_get_status_message(sig_status,"?") );
 #endif
}

//-----------------------------------------------------------------------------

static void dump_hex
(
    FILE		* f,		// output file
    const void		* p_data,	// pointer to data
    size_t		dsize,		// size of data, must be <100
    size_t		asc_indent,	// if >0: print ":...:" with entered indent
    ccp			tail		// print this test and end of line (eg. LF)
)
{
    DASSERT(f);
    const u8 * data = p_data;
    DASSERT(p_data);
    char buf[100];
    ASSERT(dsize < sizeof(buf) );

    int count = 0, fw = 0;
    while ( dsize-- > 0 )
    {
	if ( count && !(count&3) )
	{
	    fputc(' ',f);
	    fw++;
	}
	buf[count++] = *data >= ' ' && *data < 0x7f ? *data : '.';
	fprintf(f,"%02x",*data++);
	fw += 2;
    }

    if ( asc_indent > 0 )
    {
	buf[count] = 0;
	fprintf(f,"%*s= '%s'%s",(int)(asc_indent-fw), "", buf, tail ? tail : "" );
    }
    else if (tail)
	fputs(tail,f);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_ISO()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void dump_gc_part
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    wd_disc_t		* disc,		// disc pointer
    ShowMode		show_mode	// what should be printed
)
{
    DASSERT(f);
    DASSERT(disc);

    if ( !( disc->disc_attrib & WD_DA_GC_MULTIBOOT ) )
	return;

    //----- partitions

    if ( show_mode & SHOW_P_INFO && disc->n_part )
    {
	wd_part_t * part = disc->part;
	DASSERT(part);

	int i;
	u32 np = disc->n_part;
	for ( i = 0; i < np; i++, part++ )
	{
	    if ( !part->is_valid || !part->is_enabled )
		continue;

	    fprintf(f,"\n%*sGameCube partition #%d, offset 0x%llx, size %s:\n",
		indent,"", part->index,
		(u64)part->part_off4 << 2,
		wd_print_size_1024(0,0,part->part_size,false));

	    if (part->is_overlay)
		fprintf(f,"%*s  Partition overlays other partitions.\n", indent,"" );

	    fprintf(f,"%*s  Partition type:  %s\n",
			indent, "",
			wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO) );
	    fprintf(f,"%*s  boot.bin, ID:    %s\n",
			indent, "", wd_print_id(&part->boot,6,0));
	    fprintf(f,"%*s  boot.bin, title: %.64s\n",
			indent, "", part->boot.dhead.disc_title);

	    ccp title = GetTitle(&part->boot.dhead.disc_id,0);
	    if (title)
		fprintf(f,"%*s  Title DB:        %s\n", indent, "", title );

	    fprintf(f,"%*s  Region:        %7u [%s]\n",indent,"",
			part->region, GetRegionName(part->region,"?") );

	    fprintf(f,"%*s  Directories:   %7u\n",indent,"",part->fst_dir_count);
	    fprintf(f,"%*s  Files:         %7u\n",indent,"",part->fst_file_count);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

static void dump_wii_part
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    wd_disc_t		* disc,		// disc pointer
    ShowMode		show_mode	// what should be printed
)
{
    DASSERT(f);
    DASSERT(disc);

    int i;
    u32 nt = disc->n_ptab;
    u32 np = disc->n_part;
 
    char pname[100];
   
    if ( show_mode & SHOW_P_TAB && wd_disc_has_ptab(disc) )
    {
	fprintf(f,"\n%*s%d partition table%s with %d partition%s%s:\n\n"
	    "%*s   tab.idx   n(part)       offset(part.tab) .. end(p.tab)\n"
	    "%*s  --------------------------------------------------------\n",
		indent,"", nt, nt == 1 ? "" : "s",
		np, np == 1 ? "" : "s",
		disc->patch_ptab_recommended ? " (patching recommended)" : "",
		indent,"", indent,"" );

	wd_ptab_info_t * pc = disc->ptab_info;
	for ( i = 0; i < WII_MAX_PTAB; i++, pc++ )
	{
	    const u32 np = ntohl(pc->n_part);
	    if (np)
	    {
		const u32 off4 = ntohl(pc->off4);
		const u64 off = (u64)off4 << 2;
		fprintf(f,"%*s%7d %8d %11x*4 = %10llx .. %10llx\n",
		    indent,"", i, np, off4, off,
		    off + np * sizeof(wd_ptab_entry_t) );
	    }
	}

	//---------------

	fprintf(f,"\n%*s%d partition%s:\n\n"
	    "%*s   index      type      offset .. end offset   size/hex =   size/dec =  MiB  status\n"
	    "%*s  ------------------------------------------------------------------------------------\n",
	    indent,"", np, np == 1 ? "" : "s", indent,"", indent,"" );

	wd_part_t * part = disc->part;
	DASSERT(part);
	for ( i = 0; i < np; i++, part++ )
	{
	    wd_print_part_name(pname,sizeof(pname),part->part_type,WD_PNAME_COLUMN_9);

	    const u64 off  = (u64)part->part_off4 << 2;
	    ccp status = !part->is_enabled
				? "disabled"
				: part->is_encrypted
					? "-"
					: "decrypted";
	    
	    if (part->is_valid)
	    {
		const u64 size = part->part_size;
		fprintf(f,"%*s%5d.%-2d %s %11llx ..%11llx %10llx =%11llu =%5llu  %s\n",
		    indent,"", part->ptab_index, part->ptab_part_index,
		    pname, off, off + size, size, size, (size+MiB/2)/MiB, status );
	    }
	    else
		fprintf(f,"%*s%5d.%-2d %s %11llx         "
			  "** INVALID PARTITION **               invalid\n",
		    indent,"", part->ptab_index, part->ptab_part_index,
		    pname, off );
	}
    }


    //----- partitions

    if ( show_mode & SHOW__PART && disc->n_part && wd_disc_has_ptab(disc) )
    {
      wd_part_t * part = disc->part;
      DASSERT(part);
      for ( i = 0; i < np; i++, part++ )
      {
	if ( !part->is_valid || !part->is_enabled )
	    continue;

	const wd_part_header_t * ph = &part->ph;

	wd_print_part_name(pname,sizeof(pname),part->part_type,WD_PNAME_NUM_INFO);
	fprintf(f,"\n%*sPartition table #%d, partition #%d, type %s:\n",
		indent,"", part->ptab_index, part->ptab_part_index, pname );

	if ( show_mode & SHOW_P_INFO )
	{
	    if (part->is_overlay)
		fprintf(f,"%*s  Partition overlays other partitions.\n", indent,"" );
		
	    if ( tmd_is_marked_not_encrypted(part->tmd) )
		fprintf(f,"%*s  Partition is marked as 'not encrypted'.\n", indent,"" );
	    else
		fprintf(f,"%*s  %s\n", indent,"", wd_print_sig_status(0,0,part,false,true) );

	    u8 * p8 = part->key;
	    fprintf(f,"%*s  Partition key: %02x%02x%02x%02x %02x%02x%02x%02x"
					 " %02x%02x%02x%02x %02x%02x%02x%02x\n",
		indent,"",
		p8[0], p8[1], p8[2], p8[3], p8[4], p8[5], p8[6], p8[7],
		p8[8], p8[9], p8[10], p8[11], p8[12], p8[13], p8[14], p8[15] );

	    if ( !(show_mode & SHOW_F_PRIMARY) )
	    {
		if ( !(show_mode & SHOW_TMD) && part->tmd )
		{
		    const u64 sys_version = ntoh64(part->tmd->sys_version);
		    const u32 hi = sys_version >> 32;
		    const u32 lo = (u32)sys_version;
		    if ( hi == 1 && lo < 0x100 )
			fprintf(f,"%*s  System version: %08x-%08x = IOS 0x%02x = IOS %u\n",
				    indent, "", hi, lo, lo, lo );
		    else
			fprintf(f,"%*s  System version: %08x-%08x\n",
				    indent, "", hi, lo );
		}
	    }

	    fprintf(f,"%*s  boot.bin, ID:    %s\n",
			indent, "", wd_print_id(&part->boot,6,0));
	    fprintf(f,"%*s  boot.bin, title: %.64s\n",
			indent, "", part->boot.dhead.disc_title);
	    fprintf(f,"%*s  Directories:   %7u\n",indent,"",part->fst_dir_count);
	    fprintf(f,"%*s  Files:         %7u\n",indent,"",part->fst_file_count);
	}

	if ( show_mode & SHOW_P_MAP )
	{
	    const u64 off = (u64) part->part_off4 << 2;
	    dump_data(f,indent, off, 0,		    sizeof(*ph),	"TIK:" );
	    dump_data(f,indent, off, ph->tmd_off4,  ph->tmd_size,	"TMD:" );
	    dump_data(f,indent, off, ph->cert_off4, ph->cert_size,	"CERT:" );
	    dump_data(f,indent, off, ph->h3_off4,   WII_H3_SIZE,	"H3:" );
	    dump_data(f,indent, off, ph->data_off4, (u64)ph->data_size4<<2, "Data:" );
	}

	if ( show_mode & SHOW_CERT )
	    Dump_CERT(f,indent+2,wd_load_cert_chain(part,true),false);

	if ( show_mode & SHOW_TICKET )
	{
	    fprintf(f,"%*s  Ticket:\n",indent,"");
	    Dump_TIK_MEM(f,indent+4,&ph->ticket,wd_get_cert_ticket_stat(part,false));
	}

	if ( show_mode & SHOW_TMD && part->tmd )
	{
	    fprintf(f,"%*s  TMD:\n",indent,"");
	    Dump_TMD_MEM(f,indent+4,part->tmd,100,wd_get_cert_tmd_stat(part,false));
	}
      }
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_ISO
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    ShowMode		show_mode,	// what should be printed
    int			dump_level	// dump level: 0..3, ignored if show_mode is set
)
{
    //----- setup

    ASSERT(sf);
    if ( !f || !sf->f.id6[0] )
	return ERR_OK;
    sf->f.read_behind_eof = 2;

    wd_disc_t * disc = OpenDiscSF(sf,true,true);
    if ( show_mode & SHOW_F_PRIMARY )
	disc = sf->disc1;
    if (!disc)
	return ERR_WDISC_NOT_FOUND;

    const u32 used_blocks = wd_count_used_disc_blocks(disc,1,0);
    const u32 used_mib    = ( used_blocks + WII_SECTORS_PER_MIB/2 ) / WII_SECTORS_PER_MIB;
    const bool is_gc	  = disc->disc_type == WD_DT_GAMECUBE;

    FilePattern_t * pat = GetDefaultFilePattern();
    indent = NormalizeIndent(indent) + 2;


    //----- options --show and --long

    if ( show_mode & SHOW__DEFAULT )
    {
	show_mode = SHOW__ALL;
	switch (dump_level)
	{
	    case 0:  
		show_mode = SHOW_INTRO | SHOW_P_TAB;
		break;

	    case 1:
		show_mode &= ~(SHOW_D_MAP|SHOW_USAGE|SHOW_TICKET|SHOW_TMD);
		// fall through

	    case 2:  
		show_mode &= ~SHOW_FILES;
		// fall through

	    case 3:
		show_mode &= ~(SHOW_PATCH|SHOW_RELOCATE);
		break;
	}

	show_mode |= SHOW_F_HEAD;
	if (pat->is_active)
	    show_mode |= SHOW_FILES;
    }

    if ( show_mode & SHOW_F_PRIMARY )
	show_mode &= ~(SHOW_TICKET|SHOW_TMD); // TICKET and TMD data is patched


    //----- print header

    if ( show_mode & SHOW_INTRO )
    {
	char buf[80];
	snprintf(buf,sizeof(buf),"%s%s%s%s",
		wd_get_disc_type_name(disc->disc_type,"?"),
		disc->disc_attrib & WD_DA_GC_MULTIBOOT ? ", MultiBoot" : "",
		disc->disc_attrib & WD_DA_GC_START_PART ? "+" : "",
		disc->disc_attrib & WD_DA_GC_DVD9 ? ", DVD9" : "" );
	dump_header(f,indent-2,sf,&disc->dhead.disc_id,real_path,"Disc type:",buf);

	fprintf(f,"%*sDisc name:         %.64s\n",indent,"",disc->dhead.disc_title);

	ccp title = GetTitle((ccp)&disc->dhead,0);
	if (title)
	    fprintf(f,"%*sDB title:          %s\n",indent,"",title);

	const RegionInfo_t * rinfo = GetRegionInfo(disc->dhead.region_code);
	fprintf(f,"%*sID Region:         %s [%s]\n",indent,"",rinfo->name,rinfo->name4);

	if (is_gc)
	{
	    const enumRegion reg = ntohl(disc->region.region);
	    fprintf(f,"%*sBI2 Region:        %x [%s]\n",
			    indent,"", reg, GetRegionName(reg,"?") );
	}
	else    
	{
	    const enumRegion reg = ntohl(disc->region.region);
	    u8 * p8 = disc->region.region_info;
	    fprintf(f,"%*sRegion setting:    %x [%s] / %02x %02x %02x %02x  %02x %02x %02x %02x\n",
			    indent,"", reg, GetRegionName(reg,"?"),
			    p8[0], p8[1], p8[2], p8[3], p8[4], p8[5], p8[6], p8[7] );
	}

	fprintf(f,"%*sPartitions:       %7u\n",indent,"",disc->n_part);
	fprintf(f,"%*sDirectories:      %7u\n",indent,"",disc->fst_dir_count);
	fprintf(f,"%*sFiles:            %7u\n",indent,"",disc->fst_file_count);
	fprintf(f,"%*sUsed ISO blocks:  %7u * 32 KiB = %u MiB\n",
			indent,"", used_blocks, used_mib );
    }
    else
	fprintf(f,"\n%*sDump of file %s\n",indent-2,"",sf->f.fname);


    //----- partition tables & partitions

    if ( disc->disc_type == WD_DT_GAMECUBE )
	dump_gc_part(f,indent,disc,show_mode);
    else
	dump_wii_part(f,indent,disc,show_mode);


    //----- FST (files)

    if ( show_mode & SHOW_FILES )
    {
	SetupFilePattern(pat);

	fprintf(f,"\n\n%*s%u director%s with %u file%s, disk usage %u MiB:\n\n",
		indent,"",
		disc->fst_dir_count, disc->fst_dir_count==1 ? "y" : "ies",
		disc->fst_file_count, disc->fst_file_count==1 ? "" : "s",
		used_mib );

	wd_print_fst( f, indent+2, disc,
		prefix_mode == WD_IPM_DEFAULT ? WD_IPM_PART_NAME : prefix_mode,
		ConvertShow2PFST(show_mode,SHOW__ALL) | WD_PFST_PART,
		pat->match_all ? 0 : MatchFilePatternFST, pat );
    }


    //----- disc usage

    if ( show_mode & SHOW_USAGE )
    {
	fprintf(f,"\n\n%*sISO Usage Map:\n\n",indent,"");
	wd_filter_usage_table(disc,wdisc_usage_tab,0);
	wd_print_usage_tab(f,indent+2,wdisc_usage_tab,disc->iso_size,false);
    }


    //----- memory map

    if ( show_mode & SHOW_D_MAP )
    {
	MemMap_t mm;
	InitializeMemMap(&mm);
	InsertDiscMemMap(&mm,disc);
	fprintf(f,"\n\n%*sISO Memory Map:\n\n",indent,"");
	PrintMemMap(&mm,f,indent+2);
	ResetMemMap(&mm);
    }


    if ( sf->disc1 && sf->disc1 != sf->disc2 )
    {
	//----- patching tables

	if ( show_mode & SHOW_PATCH )
	{
	    putc('\n',f);
	    wd_print_disc_patch(f,indent,sf->disc1,true,true);
	}


	//----- relocating table

	if ( show_mode & SHOW_RELOCATE )
	{
	    putc('\n',f);
	    wd_print_disc_relocation(f,indent,sf->disc1,true);
	}
    }


    //----- terminate

    if (disc->invalid_part)
    {
	fprintf(f,"\n\nWARNING: Disc contains %u invalid partition%s!\n\n",
		disc->invalid_part, disc->invalid_part == 1 ? "" : "s" );
	return ERR_WPART_INVALID;
    }

    putc('\n',f);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_DOL()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_DOL
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;
    indent = dump_header(f,indent,sf,0,real_path,0,0);

    dol_header_t dol;
    enumError err = ReadSF(sf,0,&dol,sizeof(dol));
    if (!err)
    {
	ntoh_dol_header(&dol,&dol);

	MemMap_t mm1, mm2;
	InitializeMemMap(&mm1);
	InitializeMemMap(&mm2);
	MemMapItem_t * mi = InsertMemMap(&mm1,0,sizeof(dol_header_t));
	StringCopyS(mi->info,sizeof(mi->info),"DOL header");
	mi = InsertMemMap(&mm1,sf->file_size,0);
	StringCopyS(mi->info,sizeof(mi->info),"--- end of file ---");

	int i;
	for ( i = 0; i < DOL_N_SECTIONS; i++ )
	{
	    const u32 size = dol.sect_size[i];
	    if (size)
	    {
		char buf[sizeof(mi->info)];
		if ( i < DOL_N_TEXT_SECTIONS )
		    snprintf(buf,sizeof(buf),"text section #%u",i);
		else
		    snprintf(buf,sizeof(buf),"data section #%u",i-DOL_N_TEXT_SECTIONS);

		const u32 off  = dol.sect_off[i];
		const u32 addr = dol.sect_addr[i];

		mi = InsertMemMap(&mm1,off,size);
		strcpy(mi->info,buf);
		mi = InsertMemMap(&mm2,addr,size);
		strcpy(mi->info,buf);
	    }
	}
		
	fprintf(f,"\n%*sMemory map of DOL file:\n\n",indent,"");
	PrintMemMap(&mm1,f,indent+3);
	ResetMemMap(&mm1);

	mi = InsertMemMap(&mm2,dol.bss_addr,dol.bss_size);
	snprintf(mi->info,sizeof(mi->info),"bss section");
	mi = InsertMemMap(&mm2,dol.entry_addr,0);
	snprintf(mi->info,sizeof(mi->info),"entry point");

	fprintf(f,"\n%*sMemory map of DOL image:\n\n",indent,"");
	mm2.begin = 0xffffffff;
	PrintMemMap(&mm2,f,indent+3);
	ResetMemMap(&mm1);
    }
    putc('\n',f);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		Dump_CERT_BIN() + Dump_CERT() 		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_CERT_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;

    indent = dump_header(f,indent,sf,0,real_path,0,0);

    u8 * buf = malloc(sf->file_size);
    if (!buf)
	OUT_OF_MEMORY;

    enumError err = ReadSF(sf,0,buf,sf->file_size);
    if (!err)
    {
	putc('\n',f);
	Dump_CERT_MEM(f,indent,buf,sf->file_size,true);
    }
    free(buf);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_CERT_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const u8		* cert_data,	// valid pointer to cert data
    size_t		cert_size,	// size of 'cert_data'
    bool		print_ext	// true: print extended version
)
{
    ASSERT(f);
    ASSERT(cert_data);
    indent = NormalizeIndent(indent);

    cert_chain_t cc;
    cert_initialize(&cc);
    cert_append_data(&cc,cert_data,cert_size,false);
    enumError err = Dump_CERT(f,indent,&cc,print_ext);
    cert_reset(&cc);

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_CERT
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const cert_chain_t	* cc,		// valid pinter to cert chain
    bool		print_ext	// true: print extended version
)
{
    DASSERT(f);
    DASSERT(cc);
    indent = NormalizeIndent(indent);

    if (!cc->used)
	fprintf(f,"%*sNo certificates found!%s\n",
		indent, "", print_ext ? "\n" : "" );

    int i;
    for ( i = 0; i < cc->used; i++ )
    {
	Dump_CERT_Item(f,indent,cc->cert+i,i,print_ext,cc);
	if (print_ext)
	    fputc('\n',f);
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

void Dump_CERT_Item
(
    FILE		* f,		// valid output stream
    int			indent,		// normalized indent of output
    const cert_item_t	* item,		// valid item pointer
    int			cert_index,	// >=0: print title with certificate index
    bool		print_ext,	// true: print extended version
    const cert_chain_t	* cc		// not NULL: verify signature
)
{
    if ( cert_index >= 0)
    {
	fprintf(f,"%*sCertificate #%u, size=0x%x=%u\n",
		indent, "", cert_index,
		item->cert_size, item->cert_size );
	indent += 2;
    }

    fprintf(f,"%*sIssuer & Key ID:   %s\n", indent, "", item->name );

    if (item->head)
    {
	const cert_stat_t stat = cc ? cert_check_cert(cc,item,0) : 0;
	dump_sig_type(f,indent,ntohl(item->head->sig_type),stat,false);
    }
    dump_sig_type(f,indent,ntohl(item->data->key_type),0,true);

    if (print_ext)
    {
	const u32 pub_exp = be32(item->data->public_key+item->key_size);
	fprintf(f,"%*sPublic exponent: %11x/hex =%11u\n",indent,"",pub_exp,pub_exp);

	fprintf(f,"%*sPublic key:        ",indent,"");
	dump_hex(f,item->data->public_key,16,0," ...\n");

	if (item->head)
	{
	    fprintf(f,"%*sSignature:         ",indent,"");
	    dump_hex(f,item->head->sig_data,16,0," ...\n");
	}
    }

    if (item->head)
    {
	u8 hash[WII_HASH_SIZE];
	SHA1((u8*)item->data,item->data_size,hash);
	fprintf(f,"%*sRelevant SHA1:     ",indent,"");
	dump_hex(f,hash,20,0,"\n");
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		Dump_TIK_BIN() + Dump_TIK() 		///////////////
///////////////////////////////////////////////////////////////////////////////

static int try_load_cert
(
    cert_chain_t	* cc,		// valid pointer to uninitialized data
    ccp			filename	// filename of source
)
{
    cert_initialize(cc);

    char * pathend = strrchr(filename,'/');
    if (pathend)
    {
	const int len = pathend - filename;
	memcpy(iobuf,filename,len);
	pathend = iobuf + len;
    }
    else
	pathend = iobuf;
    strcpy(pathend,"/cert.bin");
    TRACE("CHECK %s\n",iobuf);
    return cert_append_file(cc,iobuf,false);
}

//-----------------------------------------------------------------------------

enumError Dump_TIK_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;

    const size_t load_size = sf->file_size < sizeof(iobuf) ? sf->file_size : sizeof(iobuf);
    enumError err = ReadSF(sf,0,iobuf,load_size);
    wd_ticket_t * tik = (wd_ticket_t*)iobuf;
    if (!err)
	memcpy(sf->f.id6,tik->title_id+4,4);
    indent = dump_header(f,indent,sf,0,real_path,0,0);

    if (!err)
    {
	if ( sizeof(wd_ticket_t) > load_size )
	    err = ERROR0(ERR_WARNING,"Not a valid TMD file: %s\n",sf->f.fname);
	else
	{
	    putc('\n',f);
	    cert_chain_t cc;

	    if ( sizeof(wd_ticket_t) < load_size )
	    {
		cert_initialize(&cc);
		cert_append_data(&cc,iobuf+sizeof(wd_ticket_t),
			load_size-sizeof(wd_ticket_t),false);
	    }
	    else
		try_load_cert(&cc,sf->f.fname);

	    Dump_TIK_MEM(f,indent,tik,cert_check_ticket(&cc,tik,0));
	    if ( sizeof(wd_ticket_t) < load_size )
	    {
		putc('\n',f);
		Dump_CERT(f,indent,&cc,false);
	    }
	    cert_reset(&cc);
	}
    } 

    putc('\n',f);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_TIK_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_ticket_t	* tik,		// valid pointer to ticket
    cert_stat_t		sig_status	// not NULL: cert status of signature
)
{
    ASSERT(f);
    ASSERT(tik);
    indent = NormalizeIndent(indent);

    fprintf(f,"%*sIssuer:            %s\n", indent,"", tik->issuer );

    dump_sig_type(f,indent,ntohl(tik->sig_type),sig_status,false);
    if ( !tik->sig && !memcmp(tik->sig,tik->sig+1,sizeof(tik->sig)-1) )
	fprintf(f,"\n%*sSignature is cleared (all zero)\n", indent,"" );

    u32 val = ntohs(tik->n_dlc);
    fprintf(f,"%*sN(DLC):          %11x/hex =%11u\n", indent,"", val,val );
    val = tik->common_key_index;
    fprintf(f,"%*sCommon key index:%11u     = '%s'\n",
		indent,"", val, !val ? "standard" : val == 1 ? "korean" : "?" );
    val = ntohl(tik->time_limit);
    fprintf(f,"%*sTime limit:      %11x/hex =%11u [%sabled]\n",
		indent,"", val,val, ntohl(tik->enable_time_limit) ? "en" : "dis" );

    fprintf(f,"%*sTitle key:         ",indent,"");
    dump_hex(f,tik->title_key,sizeof(tik->title_key),0,"\n");

    u8 key[WII_KEY_SIZE];
    wd_decrypt_title_key(tik,key);
    fprintf(f,"%*s Decrypted key:    ",indent,"");
    dump_hex(f,key,sizeof(key),0,"\n");

    fprintf(f,"%*sTicket ID:         ",indent,"");
    dump_hex(f,tik->ticket_id,sizeof(tik->ticket_id),18,"\n");
    fprintf(f,"%*sConsole ID:        ",indent,"");
    dump_hex(f,tik->console_id,sizeof(tik->console_id),18,"\n");
    fprintf(f,"%*sTitle ID:          ",indent,"");
    dump_hex(f,tik->title_id,sizeof(tik->title_id),18,"\n");

    u8 hash[WII_HASH_SIZE];
    SHA1( ((u8*)tik)+WII_TICKET_SIG_OFF, sizeof(*tik)-WII_TICKET_SIG_OFF, hash );
    fprintf(f,"%*sRelevant SHA1:     ",indent,"");
    dump_hex(f,hash,20,0,"\n");

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		Dump_TMD_BIN() + Dump_TMD() 		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_TMD_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;

    const size_t load_size = sf->file_size < sizeof(iobuf) ? sf->file_size : sizeof(iobuf);
    enumError err = ReadSF(sf,0,iobuf,load_size);
    
    wd_tmd_t * tmd = (wd_tmd_t*)iobuf;
    if (!err)
	memcpy(sf->f.id6,tmd->title_id+4,4);
    indent = dump_header(f,indent,sf,0,real_path,0,0);

    if (!err)
    {
	const int n_content = ntohs(tmd->n_content);
	const int tmd_size = sizeof(wd_tmd_t) + n_content * sizeof(wd_tmd_content_t);
	if ( tmd_size > load_size )
	    err = ERROR0(ERR_WARNING,"Not a valid TMD file: %s\n",sf->f.fname);
	else
	{
	    putc('\n',f);
	    cert_chain_t cc;

	    if ( tmd_size < load_size )
	    {
		cert_initialize(&cc);
		cert_append_data(&cc,iobuf+tmd_size,load_size-tmd_size,false);
	    }
	    else
		try_load_cert(&cc,sf->f.fname);

	    Dump_TMD_MEM(f,indent,tmd,n_content,cert_check_tmd(&cc,tmd,0));
	    if ( tmd_size < load_size )
	    {
		putc('\n',f);
		Dump_CERT(f,indent,&cc,false);
	    }
	    cert_reset(&cc);
	}
    }

    putc('\n',f);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_TMD_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_tmd_t	* tmd,		// valid pointer to ticket
    int			n_content,	// number of loaded wd_tmd_content_t elements
    cert_stat_t		sig_status	// not NULL: cert status of signature
)
{
    ASSERT(f);
    ASSERT(tmd);
    ASSERT( n_content >= 0 );
    indent = NormalizeIndent(indent);

    fprintf(f,"%*sIssuer:            %s\n", indent,"", tmd->issuer );

    dump_sig_type(f,indent,ntohl(tmd->sig_type),sig_status,false);
    if ( !tmd->sig && !memcmp(tmd->sig,tmd->sig+1,sizeof(tmd->sig)-1) )
	fprintf(f,"\n%*sSignature is cleared (all zero)\n", indent,"" );

    fprintf(f,"%*sVersion:         %11u\n", indent, "", tmd->version);
    fprintf(f,"%*sCA version:      %11u\n", indent, "", tmd->ca_crl_version);
    fprintf(f,"%*sSigner version:  %11u\n", indent, "", tmd->signer_crl_version);

    u32 high = be32((ccp)&tmd->sys_version);
    u32 low  = be32(((ccp)&tmd->sys_version)+4);
    if ( high == 1 && low < 0x100 )
	fprintf(f,"%*sSystem version:    %08x %08x = IOS 0x%02x = IOS %u\n",
		indent, "", high, low, low, low );
    else
	fprintf(f,"%*sSystem version:    %08x:%08x\n", indent, "", high, low );
    fprintf(f,"%*sTitle ID:          ",indent,"");
    dump_hex(f,tmd->title_id,sizeof(tmd->title_id),18,"\n");

    u32 val = ntohl(tmd->title_type);
    fprintf(f,"%*sTitle type:     %11x/hex =%11u\n", indent,"", val, val );
    val = ntohs(tmd->group_id);
    fprintf(f,"%*sGroup ID:       %11x/hex =%11u\n", indent,"", val, val );
    val = ntohl(tmd->access_rights);
    fprintf(f,"%*sAccess rights:  %11x/hex =%11u\n", indent,"", val, val );
    val = ntohs(tmd->title_version);
    fprintf(f,"%*sTitle version:  %11x/hex =%11u\n", indent,"", val, val );
    val = ntohs(tmd->boot_index);
    fprintf(f,"%*sBoot index:     %11x/hex =%11u\n", indent,"", val, val );

    val = ntohs(tmd->n_content);
    fprintf(f,"%*sN(content):     %11x/hex =%11u\n", indent,"", val, val );
    if ( n_content > val )
	n_content = val;

    int i;
    for ( i = 0; i < n_content; i++ )
    {
	const wd_tmd_content_t * c = tmd->content + i;
	val = ntohl(c->content_id);
	fprintf(f,"%*sContent #%u, ID:   %9x/hex =%11u\n", indent,"", i, val, val );
	val = ntohs(c->index);
	fprintf(f,"%*sContent #%u, index:%9x/hex =%11u\n", indent,"", i, val, val );
	val = ntohs(c->type);
	fprintf(f,"%*sContent #%u, type: %9x/hex =%11u\n", indent,"", i, val, val );
	u64 val64 = ntoh64(c->size);
	fprintf(f,"%*sContent #%u, size:%10llx/hex =%11llu\n", indent,"", i, val64, val64 );
	fprintf(f,"%*sContent #%u, hash:  ",indent,"",i);
	dump_hex(f,c->hash,sizeof(c->hash),0,"\n");
    }

    u8 hash[WII_HASH_SIZE];
    const int tmd_size = sizeof(wd_tmd_t) + n_content * sizeof(wd_tmd_content_t);
    SHA1( ((u8*)tmd)+WII_TMD_SIG_OFF, tmd_size-WII_TMD_SIG_OFF, hash );
    fprintf(f,"%*sRelevant SHA1:     ",indent,"");
    dump_hex(f,hash,20,0,"\n");

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_HEAD_BIN()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_HEAD_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;
    indent = dump_header(f,indent,sf,0,real_path,"File type:","Disc header");

    wd_header_t head;
    enumError err = ReadSF(sf,0,&head,WBFS_INODE_INFO_OFF);
    if (!err)
    {
	WDiscInfo_t wdi;
	InitializeWDiscInfo(&wdi);
	memcpy(&wdi.dhead,&head,sizeof(wdi.dhead));
	CalcWDiscInfo(&wdi,0);

	fprintf(f,"%*sDisc name:         %s\n",indent,"",wdi.dhead.disc_title);
	if (wdi.title)
	    fprintf(f,"%*sDB title:          %s\n",indent,"",wdi.title);

	const RegionInfo_t * rinfo = GetRegionInfo(wdi.id6[3]);
	fprintf(f,"%*sRegion:            %s [%s]\n",indent,"", rinfo->name, rinfo->name4);
	ResetWDiscInfo(&wdi);
    }
    putc('\n',f);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_BOOT_BIN()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_BOOT_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;
    indent = dump_header(f,indent,sf,0,real_path,0,0);

    wd_boot_t wb;
    enumError err = ReadSF(sf,0,&wb,sizeof(wb));
    if (!err)
    {
	WDiscInfo_t wdi;
	InitializeWDiscInfo(&wdi);
	memcpy(&wdi.dhead,&wb.dhead,sizeof(wdi.dhead));
	CalcWDiscInfo(&wdi,0);

	fprintf(f,"%*sDisc name:         %s\n",indent,"",wdi.dhead.disc_title);
	if (wdi.title)
	    fprintf(f,"%*sDB title:          %s\n",indent,"",wdi.title);

	const RegionInfo_t * rinfo = GetRegionInfo(wdi.id6[3]);
	fprintf(f,"%*sRegion:            %s [%s]\n",indent,"",rinfo->name,rinfo->name4);
	ResetWDiscInfo(&wdi);

	ntoh_boot(&wb,&wb);
	u64 val = (u64)wb.dol_off4 << 2;
	fprintf(f,"%*sDOL offset: %12x/hex * 4 =%8llx/hex =%9llu\n",
			indent,"", wb.dol_off4, val, val );
	val = (u64)wb.fst_off4 << 2 ;
	fprintf(f,"%*sFST offset: %12x/hex * 4 =%8llx/hex =%9llu\n",
			indent,"", wb.fst_off4, val, val );
	val = (u64)wb.fst_size4 << 2;
	fprintf(f,"%*sFST size:   %12x/hex * 4 =%8llx/hex =%9llu\n",
			indent,"", wb.fst_size4, val, val  );
    }
    putc('\n',f);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_FST*()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_FST_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    ShowMode		show_mode	// what should be printed
)
{
    ASSERT(sf);
    TRACE("Dump_FST_BIN(%p,%d,%p,,%x)\n", f, indent, sf, show_mode );

    if ( !f || sf->file_size > 20*MiB )
	return ERR_OK;
    indent = NormalizeIndent(indent);

    //----- options --show and --long

    if ( show_mode & SHOW__DEFAULT )
	show_mode = SHOW__ALL | SHOW_F_HEAD;

    if ( show_mode & SHOW_INTRO )
    {
	dump_header(f,indent,sf,0,real_path,0,0);
	indent += 2;
    }

    //----- setup fst

    wd_fst_item_t * ftab_data = malloc(sf->file_size);
    if (!ftab_data)
	OUT_OF_MEMORY;

    enumError err = ReadSF(sf,0,ftab_data,sf->file_size);
    if (!err)
    {
	err = Dump_FST_MEM(f,indent,ftab_data,sf->file_size,sf->f.fname,
				ConvertShow2PFST(show_mode,SHOW__ALL) );
	if (err)
	    ERROR0(err,"fst.bin is invalid: %s\n",sf->f.fname);
    }

    free(ftab_data);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_FST_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_fst_item_t	* ftab_data,	// the FST data
    size_t		ftab_size,	// size of FST data
    ccp			fname,		// NULL or source hint for error message
    wd_pfst_t		pfst		// print fst mode
)
{
    ASSERT(ftab_data);
    TRACE("Dump_FST_MEM(%p,%d,%p,%x,%s,%x)\n",
		f, indent, ftab_data, (u32)ftab_size, fname, pfst );

    indent = NormalizeIndent(indent);
    if (!fname)
	fname = "memory";

    const u32 n_files = ntohl(ftab_data->size);
    if ( (ccp)(ftab_data + n_files) > (ccp)ftab_data + ftab_size )
    {
	if (f)
	    ERROR0(ERR_INVALID_FILE,"fst.bin is invalid: %s\n",fname);
	return ERR_INVALID_FILE;
    }
 
    if (f)
    {
	WiiFst_t fst;
	InitializeFST(&fst);
	CollectFST_BIN(&fst,ftab_data,GetDefaultFilePattern(),false);
	SortFST(&fst,sort_mode,SORT_NAME);
	DumpFilesFST(f,indent,&fst,pfst,0);
	ResetFST(&fst);
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			RenameISOHeader()		///////////////
///////////////////////////////////////////////////////////////////////////////

int RenameISOHeader ( void * data, ccp fname,
	ccp set_id6, ccp set_title, int verbose, int testmode )
{
    ASSERT(data);

    TRACE("RenameISOHeader(%p,,%.6s,%s,v=%d,tm=%d)\n",
		data, set_id6 ? set_id6 : "-",
		set_title ? set_title : "-", verbose, testmode );

    if ( !set_id6 || strlen(set_id6) < 6 )
	set_id6 = 0; // invalid id6

    if ( !set_title || !*set_title )
	set_title = 0; // invalid title

    if ( !set_id6 && !set_title )
	return 0; // nothing to do

    wd_header_t * whead = (wd_header_t*)data;
    char old_id6[7], new_id6[7];
    memset(old_id6,0,sizeof(old_id6));
    StringCopyS(old_id6,sizeof(old_id6),&whead->disc_id);
    memcpy(new_id6,old_id6,sizeof(new_id6));

    if ( testmode || verbose >= 0 )
    {
	if (!fname)
	    fname = "";
	printf(" - %sModify [%s] %s\n",
		testmode ? "WOULD " : "", old_id6, fname );
    }

    if (set_id6)
    {
	memcpy(new_id6,set_id6,6);
	set_id6 = new_id6;
	if ( testmode || verbose >= 0 )
	    printf("   - %sRename ID to `%s'\n", testmode ? "WOULD " : "", set_id6 );
    }

    if (set_title)
    {
	char old_name[0x40];
	StringCopyS(old_name,sizeof(old_name),(ccp)whead->disc_title);

	ccp old_title = GetTitle(old_id6,old_name);
	ccp new_title = GetTitle(new_id6,old_name);

	// and now the destination filename
	SubstString_t subst_tab[] =
	{
	    { 'j', 'J',	0, old_id6 },
	    { 'i', 'I',	0, new_id6 },

	    { 'n', 'N',	0, old_name },

	    { 'p', 'P',	0, old_title },
	    { 't', 'T',	0, new_title },

	    {0,0,0,0}
	};

	char title[PATH_MAX];
	SubstString(title,sizeof(title),subst_tab,set_title,0);
	set_title = title;

	if ( testmode || verbose >= 0 )
	    printf("   - %sSet title to `%s'\n",
			testmode ? "WOULD " : "", set_title );
    }

    return wd_rename(data,set_id6,set_title);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global options			///////////////
///////////////////////////////////////////////////////////////////////////////

bool allow_fst		= false;  // FST diabled by default
bool ignore_setup	= false;  // ignore file 'setup.txt' while composing

wd_select_t part_selector = {0};

u8 wdisc_usage_tab [WII_MAX_SECTORS];
u8 wdisc_usage_tab2[WII_MAX_SECTORS];

///////////////////////////////////////////////////////////////////////////////

struct scan_select_t
{
    wd_select_t		* select;
    ccp			err_text_extend;
    bool		err_printed;
};

//-----------------------------------------------------------------------------

static s64 PartSelectorFunc
(
    void		* param,	// NULL or user defined parameter
    ccp			name,		// normalized name of option
    const CommandTab_t	* cmd_tab,	// valid pointer to command table
    const CommandTab_t	* cmd,		// valid pointer to found command
    char		prefix,		// 0 | '-' | '+' | '='
    s64			result		// current value of result
)
{
    struct scan_select_t * scan_select = param;
    DASSERT(scan_select);
    wd_select_t * select = scan_select->select;
    DASSERT(select);

    if (prefix)
	name++;

    wd_select_mode_t mode = prefix == '-' ? WD_SM_F_DENY : 0;

    //----- scan partitions numbers

    if ( *name >= '0' && *name <= '9' )
    {
	char *end;
	u32 num = strtoul( name, &end, name[1] >= '0' && name[1] <= '9' ? 10 : 0 );
	if ( end > name && !*end )
	{
	    wd_append_select_item(select,WD_SM_ALLOW_PTYPE|mode,0,num);
	    return ERR_OK;
	}
    }


    //----- analyze keywords

    PRINT("name=%s, cmd=%p, prefix=%c\n", name, cmd, prefix );
    if (cmd)
    {
	mode |= cmd->id;
	switch (cmd->id)
	{
	    case WD_SM_ALLOW_PTYPE:
	    case WD_SM_ALLOW_PTAB:
	    case WD_SM_ALLOW_ID:
	    case WD_SM_ALLOW_GC_BOOT:
	    case WD_SM_ALLOW_ALL:
		wd_append_select_item(select,mode,cmd->opt,cmd->opt);
		break;

	    case WD_SM_N_MODE+1:
		select->whole_disc = prefix != '-';
		break;

	    case WD_SM_N_MODE+2:
		select->whole_part = prefix != '-';
		break;
	}

	return ERR_OK;
    }


    //----- analyze ID partition names

    if ( strlen(name) == 4
	&& isalnum((int)name[0])
	&& isalnum((int)name[1])
	&& isalnum((int)name[2])
	&& isalnum((int)name[3]) )
    {
	u32 ptype = toupper((int)name[0]) << 24
		  | toupper((int)name[1]) << 16
		  | toupper((int)name[2]) << 8
		  | toupper((int)name[3]);
	wd_append_select_item(select,WD_SM_ALLOW_PTYPE|mode,0,ptype);
	return ERR_OK;
    }


    //----- analyze #index | #<index | #>index | #index.index

    if ( *name == '#' )
    {
	ccp ptr = name + 1;
	int mode2 = 0;
	switch(*ptr)
	{
	    case '<': ptr++; mode2 = -1; break;
	    case '>': ptr++; mode2 = +1; break;
	    case '=': ptr++; break;
	}

	int add_num = 0;
	if ( mode2 && *ptr == '=' )
	{
	    ptr++;
	    add_num = -mode2;
	}

	char *end;
	u32 num1 = strtoul( ptr, &end, ptr[1] >= '0' && ptr[1] <= '9' ? 10 : 0 );
	if ( end > ptr )
	{
	    if (!*end)
	    {
		if ( !num1 && add_num < 0 )
		    wd_append_select_item(select,WD_SM_ALLOW_ALL|mode,0,num1+add_num);
		else
		{
		    mode |= mode2 < 0
			    ? WD_SM_ALLOW_LT_INDEX
			    :  mode2 > 0
				? WD_SM_ALLOW_GT_INDEX
				: WD_SM_ALLOW_INDEX;
		    wd_append_select_item(select,mode,0,num1+add_num);
		}
		return ERR_OK;
	    }
	    
	    if ( *end == '.' && num1 < WII_MAX_PTAB && !mode2 )
	    {
		ptr = end + 1;
		if ( !*ptr)
		{
		    wd_append_select_item(select,WD_SM_ALLOW_PTAB|mode,num1,0);
		    return ERR_OK;
		}
		else
		{
		    u32 num2 = strtoul( ptr, &end, ptr[1] >= '0' && ptr[1] <= '9' ? 10 : 0 );
		    if ( end > ptr && !*end )
		    {
			wd_append_select_item(select,WD_SM_ALLOW_PTAB_INDEX|mode,num1,num2);
			return ERR_OK;
		    }
		}
	    }
	}
    }


    //----- terminate with error

    scan_select->err_printed = true;
    ERROR0(ERR_SYNTAX,"Illegal partition selector%s: %.20s\n",
		scan_select->err_text_extend, name );
    return ERR_SYNTAX;
}

//-----------------------------------------------------------------------------

enumError ScanPartSelector
(
    wd_select_t * select,	// valid partition selector
    ccp arg,			// argument to scan
    ccp err_text_extend		// error message extention
)
{
    static const CommandTab_t tab[] =
    {
	{ WD_SM_ALLOW_PTYPE,	"DATA",		"D",	WD_PART_DATA },
	{ WD_SM_ALLOW_PTYPE,	"GAME",		"G",	WD_PART_DATA },
	{ WD_SM_ALLOW_PTYPE,	"UPDATE",	"U",	WD_PART_UPDATE },
	{ WD_SM_ALLOW_PTYPE,	"INSTALLER",	"I",	WD_PART_UPDATE },
	{ WD_SM_ALLOW_PTYPE,	"CHANNEL",	"C",	WD_PART_CHANNEL },

	{ WD_SM_ALLOW_PTAB,	"PTAB0",	"T0",	0 },
	{ WD_SM_ALLOW_PTAB,	"PTAB1",	"T1",	1 },
	{ WD_SM_ALLOW_PTAB,	"PTAB2",	"T2",	2 },
	{ WD_SM_ALLOW_PTAB,	"PTAB3",	"T3",	3 },

	{ WD_SM_ALLOW_ID,	"ID",		0,	0 },
	{ WD_SM_ALLOW_GC_BOOT,	"BOOT",		0,	0 },
	{ WD_SM_ALLOW_ALL,	"ALL",		0,	0 },

	{ WD_SM_N_MODE+1,	"1:1",		"RAW",	0 },
	{ WD_SM_N_MODE+2,	"WHOLE",	0,	0 },

	{ 0,0,0,0 }
    };

    DASSERT(select);
    wd_reset_select(select);

    struct scan_select_t scan_select;
    memset(&scan_select,0,sizeof(scan_select));
    scan_select.err_text_extend	= err_text_extend;
    scan_select.select		= select;
    
    const enumError err
	= ScanCommandListFunc(arg,tab,PartSelectorFunc,&scan_select,true);
    if ( err && !scan_select.err_printed )
	ERROR0(err,"Illegal partition selector%s: %.20s\n",
			err_text_extend, arg );
    return err;
}

//-----------------------------------------------------------------------------

int ScanOptPartSelector ( ccp arg )
{
    return ScanPartSelector(&part_selector,optarg," (option --psel)") != ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

u32 ScanPartType ( ccp arg, ccp err_text_extend )
{
    static const CommandTab_t tab[] =
    {
	{ WD_PART_DATA,		"DATA",		"D",		0 },
	{ WD_PART_DATA,		"GAME",		"G",		0 },
	{ WD_PART_UPDATE,	"UPDATE",	"U",		0 },
	{ WD_PART_UPDATE,	"INSTALLER",	"I",		0 },
	{ WD_PART_CHANNEL,	"CHANNEL",	"C",		0 },

	{ 0,0,0,0 }
    };

    const CommandTab_t * cmd = ScanCommand(0,arg,tab);
    if (cmd)
	return cmd->id;

    char * end;
    u32 val = strtoul(arg,&end,10);
    if ( end > arg && !*end )
	return val;

    if ( strlen(arg) == 4
	&& isalnum((int)arg[0])
	&& isalnum((int)arg[1])
	&& isalnum((int)arg[2])
	&& isalnum((int)arg[3]) )
    {
	return be32(arg);
    }

    ERROR0(ERR_SYNTAX,"Illegal partition type%s: %.20s\n",
			err_text_extend, arg );
    return -(u32)1;
}

//-----------------------------------------------------------------------------

enumError ScanPartTabAndType
(
    u32		* res_ptab,		// NULL or result: partition table
    bool	* res_ptab_valid,	// NULL or result: partition table is valid
    u32		* res_ptype,		// NULL or result: partition type
    bool	* res_ptype_valid,	// NULL or result: partition type is valid
    ccp		arg,			// argument to analyze
    ccp		err_text_extend		// text to extent error messages
)
{
    DASSERT(arg);

    u32 ptab = 0;
    ccp sep = strchr(arg,'.');
    bool ptab_valid = sep != 0;
    if (ptab_valid)
    {
	char * end;
	ptab = strtoul(arg,&end,10);
	if ( end != sep || ptab >= WII_MAX_PTAB )
	    return ERROR0(ERR_SYNTAX,
			"Not a valid partition table%s: %.20s\n",
			err_text_extend, arg );
	arg = sep + 1;
	
    }

    u32 ptype = 0;
    bool ptype_valid = !sep || *arg;
    if (ptype_valid)
    {
	ptype = ScanPartType(arg,err_text_extend);
	if ( ptype == -(u32)1 )
	    return ERR_SYNTAX;
    }

    if (res_ptab)
	*res_ptab = ptab;
    if (res_ptab_valid)
	*res_ptab_valid = ptab_valid;
    if (res_ptype)
	*res_ptype = ptype;
    if (res_ptype_valid)
	*res_ptype_valid = ptype_valid;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

wd_ipm_t prefix_mode = WD_IPM_DEFAULT;

//-----------------------------------------------------------------------------

wd_ipm_t ScanPrefixMode ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ WD_IPM_DEFAULT,	"DEFAULT",	"D",		0 },
	{ WD_IPM_AUTO,		"AUTO",		"A",		0 },

	{ WD_IPM_NONE,		"NONE",		"-",		0 },
	{ WD_IPM_POINT,		"POINT",	".",		0 },
	{ WD_IPM_PART_ID,	"ID",		"I",		0 },
	{ WD_IPM_PART_NAME,	"NAME",		"N",		0 },
	{ WD_IPM_PART_INDEX,	"INDEX",	"IDX",		0 },
	{ WD_IPM_COMBI,		"COMBI",	"C",		0 },

	{ 0,0,0,0 }
    };

    const CommandTab_t * cmd = ScanCommand(0,arg,tab);
    if (cmd)
	return cmd->id;

    ERROR0(ERR_SYNTAX,"Illegal prefix mode (option --pmode): '%s'\n",arg);
    return -1;
}

//-----------------------------------------------------------------------------

void SetupSneekMode()
{
    wd_reset_select(&part_selector);
    wd_append_select_item(&part_selector,WD_SM_ALLOW_PTYPE,0,WD_PART_DATA);

    prefix_mode = WD_IPM_NONE;

    FilePattern_t * pat = file_pattern + PAT_DEFAULT;
    pat->rules.used = 0;
    AddFilePattern(":sneek",PAT_DEFAULT);
    SetupFilePattern(pat);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  Iso Mapping			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeIM ( IsoMapping_t * im )
{
    ASSERT(im);
    memset(im,0,sizeof(*im));
}

///////////////////////////////////////////////////////////////////////////////

void ResetIM ( IsoMapping_t * im )
{
    ASSERT(im);

    if (im->field)
    {
	IsoMappingItem_t *imi, *imi_end = im->field  + im->used;
	for ( imi = im->field; imi < imi_end; imi++ )
	    if (imi->data_alloced)
		free(imi->data);
	free(im->field);
    }
    memset(im,0,sizeof(*im));
}

///////////////////////////////////////////////////////////////////////////////

static uint InsertHelperIM ( IsoMapping_t * im, u32 offset, u32 size )
{
    ASSERT(im);

    int beg = 0;
    int end = im->used - 1;
    while ( beg <= end )
    {
	const uint idx = (beg+end)/2;
	IsoMappingItem_t * imi = im->field + idx;
	if ( offset < imi->offset )
	    end = idx - 1 ;
	else if ( offset > imi->offset )
	    beg = idx + 1;
	else if ( size < imi->size )
	    end = idx - 1 ;
	else
	    beg = idx + 1;
    }

    TRACE("InsertHelperIM(%x,%x) %d/%d/%d\n",
		offset, size, beg, im->used, im->size );
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

IsoMappingItem_t * InsertIM
	( IsoMapping_t * im, enumIsoMapType imt, u64 offset, u64 size )
{
    ASSERT(im);

    if ( im->used == im->size )
    {
	im->size += 10;
	im->field = realloc(im->field,im->size*sizeof(*im->field));
	if (!im->field)
	    OUT_OF_MEMORY;
    }

    u32 idx = InsertHelperIM(im,offset,size);

    ASSERT( idx <= im->used );
    IsoMappingItem_t * dest = im->field + idx;
    memmove(dest+1,dest,(im->used-idx)*sizeof(IsoMappingItem_t));
    im->used++;

    memset(dest,0,sizeof(*dest));
    dest->imt	 = imt;
    dest->offset = offset;
    dest->size   = size;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

void PrintIM ( IsoMapping_t * im, FILE * f, int indent )
{
    ASSERT(im);
    if ( !f || !im->used )
	return;
    indent = NormalizeIndent(indent);

    int fw = 80-indent;
    if ( fw < 52 )
	 fw = 52;

    fprintf(f,"\n%*s     offset .. offset end     size  comment\n"
		"%*s%.*s\n",
		indent,"", indent,"", fw, wd_sep_200 );

    u64 prev_end = 0;
    IsoMappingItem_t *imi, *last = im->field + im->used - 1;
    for ( imi = im->field; imi <= last; imi++ )
    {
	const u64 end = imi->offset + imi->size;
	fprintf(f,"%*s%c %9llx .. %9llx %9llx  %s\n", indent,"",
		imi->offset < prev_end ? '!' : ' ',
		imi->offset, end, imi->size, imi->info );
	prev_end = end;
    }
    putc('\n',f);
}

///////////////////////////////////////////////////////////////////////////////

void PrintFstIM
	( WiiFst_t * fst, FILE * f, int indent, bool print_part, ccp title  )
{
    ASSERT(fst);
    if (!f)
	return;
    indent = NormalizeIndent(indent);

    if (title)
	fprintf(f,"\n%*s%s disc:\n",indent,"",title);
    PrintIM(&fst->im,f,indent+2);
    
    if (print_part)
    {
	WiiFstPart_t *part, *part_end = fst->part + fst->part_used;
	for ( part = fst->part; part < part_end; part++ )
	{
	    if (title)
		fprintf(f,"%*s%s %s partition:\n",
			indent, "", title,
			wd_get_part_name(part->part_type,"?"));
	    PrintIM(&part->im,f,indent+2);
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                      Wii FST                    ///////////////
///////////////////////////////////////////////////////////////////////////////

ccp SpecialFilesFST[] =
{
	"/sys/boot.bin",
	"/sys/bi2.bin",
	"/sys/apploader.img",
	"/sys/main.dol",
	"/cert.bin",
	0
};

///////////////////////////////////////////////////////////////////////////////

void InitializeFST ( WiiFst_t * fst )
{
    ASSERT(fst);
    memset(fst,0,sizeof(*fst));
    InitializeIM(&fst->im);
}

///////////////////////////////////////////////////////////////////////////////

void ResetFST ( WiiFst_t * fst )
{
    ASSERT(fst);
    ASSERT( !fst->part_size == !fst->part );
    ASSERT( fst->part_used >= 0 && fst->part_used <= fst->part_size );

    if (fst->part)
    {
	WiiFstPart_t *part, *part_end = fst->part + fst->part_used;
	for ( part = fst->part; part < part_end; part++ )
	    ResetPartFST(part);
	free(fst->part);
    }
    ResetIM(&fst->im);
    free(fst->cache);
    
    wd_close_disc(fst->disc);

    memset(fst,0,sizeof(*fst));
    InitializeIM(&fst->im);
}

///////////////////////////////////////////////////////////////////////////////

void ResetPartFST ( WiiFstPart_t * part )
{
    ASSERT(part);
    ASSERT( !part->file_size == !part->file );
    ASSERT( part->file_used >= 0 && part->file_used <= part->file_size );

    if (part->file)
    {
	WiiFstFile_t *file, *file_end = part->file + part->file_used;
	for ( file = part->file; file < file_end; file++ )
	    FreeString(file->path);
	free(part->file);
    }

    FreeString(part->path);
    ResetStringField(&part->include_list);
    ResetIM(&part->im);
    free(part->ftab);
    free(part->pc);

    memset(part,0,sizeof(*part));
    InitializeIM(&part->im);
}

///////////////////////////////////////////////////////////////////////////////

WiiFstPart_t * AppendPartFST ( WiiFst_t * fst )
{
    ASSERT(fst);
    ASSERT( !fst->part_size == !fst->part );
    ASSERT( fst->part_used >= 0 && fst->part_used <= fst->part_size );

    if ( fst->part_used == fst->part_size )
    {
	fst->part_size += 10;
	fst->part = realloc(fst->part,fst->part_size*sizeof(*fst->part));
	if (!fst->part)
	    OUT_OF_MEMORY;
    }

    WiiFstPart_t * part = fst->part + fst->part_used++;
    memset(part,0,sizeof(*part));
    InitializeStringField(&part->include_list);
    InitializeIM(&part->im);

    return part;
}

///////////////////////////////////////////////////////////////////////////////

WiiFstFile_t * AppendFileFST ( WiiFstPart_t * part )
{
    ASSERT(part);
    ASSERT( !part->file_size == !part->file );
    ASSERT( part->file_used >= 0 && part->file_used <= part->file_size );

    if ( part->file_used == part->file_size )
    {
	part->file_size += part->file_size/4 + 1000;
	part->file = realloc(part->file,part->file_size*sizeof(*part->file));
	TRACE("REALLOC(part->file): %u (size=%zu) -> %p\n",
		part->file_size,part->file_size*sizeof(*part->file),part->file);
	if (!part->file)
	    OUT_OF_MEMORY;
    }

    part->sort_mode = SORT_NONE;

    WiiFstFile_t * file = part->file + part->file_used++;
    memset(file,0,sizeof(*file));
    return file;
}

///////////////////////////////////////////////////////////////////////////////

WiiFstFile_t * FindFileFST ( WiiFstPart_t * part, u32 offset4 )
{
    ASSERT(part);
    ASSERT(part->file_used);

    int beg = 0;
    int end = part->file_used - 1;
    while ( beg <= end )
    {
	const uint idx = (beg+end)/2;
	WiiFstFile_t * file = part->file + idx;
	if ( file->offset4 < offset4 )
	    beg = idx + 1;
	else
	    end = idx - 1 ;
    }

    if ( beg > 0 )
    {
	WiiFstFile_t * file = part->file + beg-1;
	if ( file->offset4 + (file->size+3>>2) > offset4 )
	    beg--;
    }

 #ifdef DEBUG
    {
	TRACE("FindFileFST(%x) %d/%d/%d, searched-off=%x\n",
		    offset4, beg, part->file_used, part->file_size, offset4<<2 );

	u64 off;
	WiiFstFile_t * f;
	
	if ( beg > 0 )
	{
	    f = part->file + beg - 1;
	    off = (u64)f->offset4 << 2;
	    TRACE(" - %5x: %9llx .. %9llx + %8x %s\n",
			beg-1, off, off+f->size, f->size, f->path );
	}
	f = part->file + beg;
	off = (u64)f->offset4 << 2;
	TRACE(" * %5x: %9llx .. %9llx + %8x %s\n",
			beg, off, off+f->size, f->size, f->path );
	if ( beg < part->file_used - 1 )
	{
	    f = part->file + beg + 1;
	    off = (u64)f->offset4 << 2;
	    TRACE(" - %5x: %9llx .. %9llx + %8x %s\n",
			beg+1, off, off+f->size, f->size, f->path );
	}
    }
 #endif

    ASSERT( beg >= 0 && beg < part->file_used );
    return part->file + beg;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			CollectFST()			/////////////// 
///////////////////////////////////////////////////////////////////////////////

typedef struct CollectFST_t
{
	WiiFst_t	* fst;		// valid fst pointer
	wd_disc_t	* disc;		// valid disc pointer
	FilePattern_t	* pat;		// NULL or a valid filter
	bool		ignore_dir;	// true: ignore directories
	bool		store_prefix;	// true: store full path incl. prefix

} CollectFST_t;

//-----------------------------------------------------------------------------

static int CollectFST_helper
(
    struct wd_iterator_t *it	// iterator struct with all infos
)
{
    DASSERT(it);

    CollectFST_t * cf = it->param;
    WiiFst_t * fst = cf->fst;
    ASSERT(fst);
    WiiFstPart_t * part = fst->part_active;

    if ( it->icm >= WD_ICM_DIRECTORY )
    {
	ASSERT(part);
	if (!part)
	    return ERR_OK;

	if ( it->icm == WD_ICM_DIRECTORY )
	{
	    fst->dirs_served++;
	    if (cf->ignore_dir)
		return 0;
	}
	else
	{
	    fst->files_served++;
	    part->total_file_size += it->size;
	}

	if ( cf->pat && !MatchFilePattern(cf->pat,it->fst_name,'/') )
	    return 0;

	noPRINT("%8x >> %8x + %8x  %s\n",it->off4,it->off4>>2,it->size,it->path);

	size_t slen = strlen(it->path);
	if ( fst->max_path_len < slen )
	     fst->max_path_len = slen;
	if ( fst->fst_max_off4 < it->off4 )
	     fst->fst_max_off4 = it->off4;
	if ( fst->fst_max_size < it->size )
	     fst->fst_max_size = it->size;

	WiiFstFile_t * wff = AppendFileFST(part);
	wff->icm	= it->icm;
	wff->offset4	= it->off4;
	wff->size	= it->size;
	wff->path	= strdup( cf->store_prefix ? it->path : it->fst_name );
	wff->data	= (u8*)it->data;
    }
    else switch(it->icm)
    {
	case WD_ICM_OPEN_PART:
	    part			= AppendPartFST(fst);
	    fst->part_active		= part;
	    part->part			= it->part;
	    part->path			= strdup(it->path);
	    part->part_type		= it->part->part_type;
	    part->part_off		= (u64) it->part->part_off4 << 2;

	    memcpy(part->key,it->part->key,sizeof(part->key));
	    wd_aes_set_key(&part->akey,part->key);
	    break;

	case WD_ICM_CLOSE_PART:
	    if (part)
	    {
		fst->total_file_count += part->file_used;
		fst->total_file_size  += part->total_file_size;
		fst->part_active = 0;
		TRACE("PART CLOSED: %u/%u files, %llu/%llu MiB\n",
			part->file_used, fst->total_file_count,
			part->total_file_size/MiB, fst->total_file_size/MiB );
	    }
	    break;

	default:
	    // nothing to do
	    break;
    }

    return 0;
}

//-----------------------------------------------------------------------------

int CollectFST
(
    WiiFst_t		* fst,		// valid fst pointer
    wd_disc_t		* disc,		// valid disc pointer
    FilePattern_t	* pat,		// NULL or a valid filter
    bool		ignore_dir,	// true: ignore directories
    wd_ipm_t		prefix_mode,	// prefix mode
    bool		store_prefix	// true: store full path incl. prefix
)
{
    ASSERT(fst);
    ASSERT(disc);

    // dup and store disc
    wd_dup_disc(disc);
    wd_close_disc(fst->disc);
    fst->disc = disc;

    if (pat)
	SetupFilePattern(pat);

    CollectFST_t cf;
    memset(&cf,0,sizeof(cf));
    cf.fst		= fst;
    cf.disc		= disc;
    cf.pat		= pat && pat->match_all ? 0 : pat;
    cf.ignore_dir	= ignore_dir;
    cf.store_prefix	= store_prefix;
    
    return wd_iterate_files(disc,CollectFST_helper,&cf,prefix_mode);
}

//-----------------------------------------------------------------------------

int CollectFST_BIN
(
    WiiFst_t		* fst,		// valid fst pointer
    const wd_fst_item_t * ftab_data,	// the FST data
    FilePattern_t	* pat,		// NULL or a valid filter
    bool		ignore_dir	// true: ignore directories
)
{
    CollectFST_t cf;
    memset(&cf,0,sizeof(cf));
    cf.fst		= fst;
    cf.pat		= pat && pat->match_all ? 0 : pat;
    cf.ignore_dir	= ignore_dir;

    fst->part_active = AppendPartFST(fst);
    return wd_iterate_fst_files(ftab_data,CollectFST_helper,&cf);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DumpFilesFST
(
    FILE		* f,		// NULL or output file
    int			indent,		// indention of the output
    WiiFst_t		* fst,		// valid FST pointer
    wd_pfst_t		pfst,		// print fst mode
    ccp			prefix		// NULL or path prefix
)
{
    DASSERT(fst);
    PRINT("DumpFilesFST() pfst=%x\n",pfst);
    if ( pfst & WD_PFST_HEADER )
	putchar('\n');

    if (!prefix)
	prefix = "";
    size_t prefix_len = strlen(prefix);

    wd_print_fst_t pf;
    wd_initialize_print_fst(&pf,pfst,f,indent,fst->fst_max_off4,fst->fst_max_size);
    if ( pfst & WD_PFST_HEADER )
	wd_print_fst_header(&pf,fst->max_path_len+prefix_len);
    
    WiiFstPart_t *part, *part_end = fst->part + fst->part_used;
    for ( part = fst->part; part < part_end; part++ )
    {
	WiiFstFile_t * ptr = part->file;
	WiiFstFile_t * end = ptr + part->file_used;

	for ( ; ptr < end; ptr++ )
	    wd_print_fst_item( &pf, part->part,
			ptr->icm, ptr->offset4, ptr->size,
			prefix, ptr->path );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			CreateFST()...			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CreateFST ( WiiFstInfo_t *wfi, ccp dest_path )
{
    TRACE("CreateFST(%p)\n",wfi);
    ASSERT(wfi);

    WiiFst_t * fst = wfi->fst;
    ASSERT(fst);
    ASSERT( !fst->part_size == !fst->part );
    ASSERT( fst->part_used >= 0 && fst->part_used <= fst->part_size );

    wfi->done_count  = 0;
    wfi->total_count = fst->total_file_count;
    wfi->done_size   = 0;
    wfi->total_size  = fst->total_file_size;

    if (wfi->verbose)
    {
	char buf[20];
	wfi->fw_done_count = snprintf(buf,sizeof(buf),"%u",wfi->total_count);

	printf(" - will extract %u file%s of %u partition%s, %llu MiB total\n",
		fst->total_file_count, fst->total_file_count == 1 ? "" : "s",
		fst->part_used, fst->part_used  == 1 ? "" : "s",
		(fst->total_file_size + MiB/2) / MiB );
    }


    //----- iterate partitions

    enumError err = ERR_OK;
    WiiFstPart_t *part_end = fst->part + fst->part_used;
    for ( wfi->part = fst->part; err == ERR_OK && wfi->part < part_end; wfi->part++ )
	err = CreatePartFST(wfi,dest_path);
    wfi->part = 0;

    if ( wfi->sf && ( wfi->sf->show_progress ||wfi->sf->show_summary ) )
	PrintSummarySF(wfi->sf);

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CreatePartFST ( WiiFstInfo_t *wfi, ccp dest_path )
{
    TRACE("CreatePartFST(%p)\n",wfi);
    ASSERT(wfi);

    WiiFstPart_t * part = wfi->part;
    ASSERT(part);


    //----- setup basic dest path

    char path[PATH_MAX];
    char * path_end = path + sizeof(path) - 1;
    char * path_dest = StringCopyE(path,path_end,dest_path);
    if ( path_dest == path )
	path_dest = StringCopyE(path,path_end,"./");
    else if ( path_dest[-1] != '/' )
	*path_dest++ = '/';
    TRACE(" - basepath=%s\n",path);


    //----- setup include list

    StringCat2E(path_dest,path_end,part->path,FST_INCLUDE_FILE);
    ReadStringField(&part->include_list,false,path,true);


    //----- create all files but WD_ICM_COPY

    *path_dest = 0;

    enumError err = ERR_OK;
    WiiFstFile_t *file, *file_end = part->file + part->file_used;
    for ( file = part->file; err == ERR_OK && file < file_end; file++ )
	if ( file->icm != WD_ICM_COPY )
	    err = CreateFileFST(wfi,path,file);


    //----- Update the signatures of the source FST

    if (wfi->fst)
	UpdateSignatureFST(wfi->fst);


    //----- and now write WD_ICM_COPY files
    //      this is needed because changing hash data while processing other files

    for ( file = part->file; err == ERR_OK && file < file_end; file++ )
	if ( file->icm == WD_ICM_COPY )
	    err = CreateFileFST(wfi,path,file);


    //----- write include.list
    
    StringCat2E(path_dest,path_end,part->path,FST_INCLUDE_FILE);
    WriteStringField(&part->include_list,path,true);

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CreateFileFST ( WiiFstInfo_t *wfi, ccp dest_path, WiiFstFile_t * file )
{
    DASSERT(wfi);
    DASSERT(wfi->fst);
    DASSERT(dest_path);
    DASSERT(file);

    WiiFstPart_t * part = wfi->part;
    DASSERT(part);

    wfi->done_count++;
 

    //----- setup path

    char dest[PATH_MAX];
    char * dest_end = dest + sizeof(dest);
    char * dest_part = StringCat2E(dest,dest_end,dest_path,part->path);
    StringCopyE(dest_part,dest_end,file->path);
    TRACE("DEST: %.*s %s\n", (int)(dest_part-dest), dest, dest_part );


    //----- report files with trailing '.'

    char * ptr = dest_part;
    while (*ptr)
    {
	if ( *ptr++ == '/' && *ptr == '.' )
	{
	    while ( *ptr && *ptr != '/' )
		ptr++;
	    char ch = *ptr;
	    *ptr = 0;
	    TRACE("TRAILING POINT: %s\n",dest_part);
	    InsertStringField(&part->include_list,dest_part,false);
	    *ptr = ch;
	}
    }

    //----- directory handling

    if ( file->icm == WD_ICM_DIRECTORY )
    {
	if ( wfi->verbose > 1 )
	    printf(" - %*u/%u create directory %s\n",
			wfi->fw_done_count, wfi->done_count, wfi->total_count,
			dest);
	if (CreatePath(dest))
	    wfi->not_created_count++;
	return ERR_OK;
    }

    if ( file->icm != WD_ICM_FILE && file->icm != WD_ICM_COPY && file->icm != WD_ICM_DATA )
	return 0;


    //----- file handling

    if ( wfi->verbose > 1 )
	printf(" - %*u/%u %s %8u bytes to %s\n",
		wfi->fw_done_count, wfi->done_count, wfi->total_count,
		file->icm == WD_ICM_DATA ? "write  " : "extract", file->size, dest );

    File_t fo;
    InitializeFile(&fo);
    fo.create_directory = true;
    enumError err = CreateFile( &fo, dest, IOM_NO_STREAM, wfi->overwrite );
    if (err)
    {
	ResetFile(&fo,true);
	wfi->not_created_count++;
	return ERR_OK;
    }

    if ( file->size > sizeof(iobuf) )
	PreallocateF(&fo,0,file->size);

 #if 0 && defined(TEST) // test ReadFileFST() [obsolete]
    if ( file->icm == WD_ICM_DATA ) 
	err = WriteF(&fo,file->data,file->size);
    else
    {
	u32 off4 = 0;
	u32 size = file->size;

	while ( size > 0 )
	{
	    const u32 read_size = size < sizeof(iobuf) ? size : sizeof(iobuf);
	    err = ReadFileFST(part,file,off4,iobuf,read_size);
	    if (err)
		break;

	    err = WriteF(&fo,iobuf,read_size);
	    if (err)
		break;

	    size -= read_size;
	    off4 += read_size>>2;

	    //----- progress

	    wfi->done_size += read_size;
	    if ( wfi->sf && wfi->sf->show_progress )
		PrintProgressSF(wfi->done_size,wfi->total_size,wfi->sf);
	}
    }
 #else
    if ( file->icm == WD_ICM_DATA ) 
	err = WriteF(&fo,file->data,file->size);
    else
    {
	u32 size = file->size;
	u32 off4 = file->offset4;

	while ( size > 0 )
	{
	    const u32 read_size = size < sizeof(iobuf) ? size : sizeof(iobuf);
	    err = file->icm == WD_ICM_FILE
			? wd_read_part(part->part,off4,iobuf,read_size,false)
			: wd_read_raw(part->part->disc,off4,iobuf,read_size,0);
	    if (err)
		break;

	    err = WriteF(&fo,iobuf,read_size);
	    if (err)
		break;

	    size -= read_size;
	    off4 += read_size>>2;

	    //----- progress

	    wfi->done_size += read_size;
	    if ( wfi->sf && wfi->sf->show_progress )
		PrintProgressSF(wfi->done_size,wfi->total_size,wfi->sf);
	}
    }
 #endif

    if (wfi->set_time)
    {
	CloseFile(&fo,0);
	SetFileTime(&fo,wfi->set_time);
    }

    ResetFile( &fo, err != ERR_OK );

    return err;    
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadFileFST
(
	WiiFstPart_t *	part,		// valid fst partition pointer
	const WiiFstFile_t * file,	// valid fst file pointer
	u32		off4,		// file offset
	void		* buf,		// destination buffer with at least 'size' bytes
	u32		size		// number of bytes to read
)
{
    DASSERT(part);
    DASSERT(part->part);
    DASSERT(part->part->disc);
    DASSERT(file);
    DASSERT(buf);

    noTRACE("ReadFileFST(off=%x,sz=%x) icm=%x, off=%x, sz=%x\n",
		off4<<2, size, file->icm, file->offset4<<2, file->size );

    char mode = 0;
    switch(file->icm)
    {
	case WD_ICM_FILE:
	    if ( (off4<<2) + size > file->size )
	    {
		mode = 'F';
		break;
	    }
	    return wd_read_part( part->part, file->offset4 + off4, buf, size, false );

	case WD_ICM_COPY:
	    if ( (off4<<2) + size > file->size )
	    {
		mode = 'C';
		break;
	    }
	    return wd_read_raw( part->part->disc, file->offset4 + off4, buf, size, 0 );

	case WD_ICM_DATA:
	    if ( (off4<<2) + size > file->size )
	    {
		mode = 'D';
		break;
	    }
	    memcpy( buf, file->data + (off4<<2), size );
	    return ERR_OK;

	default:
	    if ( off4 || size )
	    {
		mode = '?';
		break;
	    }
	    return ERR_OK;
    }

    return ERROR0(ERR_READ_FAILED,
		"Reading from FST file failed [%c:%llx+%x>%x]: %s\n",
		mode, (u64)off4<<2, size, file->size, file->path );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sort Wii FST			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef int (*compare_func) (const void *, const void *);

//-----------------------------------------------------------------------------

static int sort_fst_by_path ( const void * va, const void * vb )
{
    const WiiFstFile_t * a = (const WiiFstFile_t *)va;
    const WiiFstFile_t * b = (const WiiFstFile_t *)vb;

    const int stat = strcmp(a->path,b->path);
    if (stat)
	return stat;

    if ( a->offset4 != b->offset4 )
	return a->offset4 < b->offset4 ? -1 : 1;

    return a->size < b->size ? -1 : a->size > b->size;
}

//-----------------------------------------------------------------------------

static int sort_fst_by_offset ( const void * va, const void * vb )
{
    const WiiFstFile_t * a = (const WiiFstFile_t *)va;
    const WiiFstFile_t * b = (const WiiFstFile_t *)vb;

    if ( a->icm == WD_ICM_DIRECTORY && b->icm != WD_ICM_DIRECTORY )
	return -1;

    if ( a->icm != WD_ICM_DIRECTORY && b->icm == WD_ICM_DIRECTORY )
	return 1;

    if ( a->icm == WD_ICM_FILE && b->icm != WD_ICM_FILE )
	return 1;

    if ( a->icm != WD_ICM_FILE && b->icm == WD_ICM_FILE )
	return -1;

    if ( a->offset4 != b->offset4 )
	return a->offset4 < b->offset4 ? -1 : 1;

    if ( a->size != b->size )
	return a->size < b->size ? -1 : 1;

    return strcmp(a->path,b->path);
}

//-----------------------------------------------------------------------------

static int sort_fst_by_size ( const void * va, const void * vb )
{
    const WiiFstFile_t * a = (const WiiFstFile_t *)va;
    const WiiFstFile_t * b = (const WiiFstFile_t *)vb;

    if ( a->icm == WD_ICM_DIRECTORY && b->icm != WD_ICM_DIRECTORY )
	return -1;

    if ( a->icm != WD_ICM_DIRECTORY && b->icm == WD_ICM_DIRECTORY )
	return 1;

    if ( a->size != b->size )
	return a->size < b->size ? -1 : 1;

    return sort_fst_by_offset(va,vb);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SortFST ( WiiFst_t * fst, SortMode sort_mode, SortMode default_sort_mode )
{
    if (fst)
    {
	WiiFstPart_t *part, *part_end = fst->part + fst->part_used;
	for ( part = fst->part; part < part_end; part++ )
	    SortPartFST(part,sort_mode,default_sort_mode);
    }
}

///////////////////////////////////////////////////////////////////////////////

void SortPartFST ( WiiFstPart_t * part, SortMode sort_mode, SortMode default_sort_mode )
{
    TRACE("SortPartFST(%p,%d,%d=\n",part,sort_mode,default_sort_mode);
    if (!part)
	return;

    TRACE("SortFST(%p, s=%d,%d) prev=%d\n",
		part, sort_mode, default_sort_mode, part->sort_mode );

    if ( (uint)(sort_mode&SORT__MODE_MASK) >= SORT_DEFAULT )
    {
	sort_mode = (uint)default_sort_mode >= SORT_DEFAULT
			? part->sort_mode
			: default_sort_mode | sort_mode & SORT_REVERSE;
    }

    if ( part->file_used < 2 )
    {
	// zero or one element is sorted!
	part->sort_mode = sort_mode;
	return;
    }

    compare_func func = 0;
    SortMode smode = sort_mode & SORT__MODE_MASK;
    switch(smode)
    {
	case SORT_SIZE:		func = sort_fst_by_size; break;
	case SORT_OFFSET:	func = sort_fst_by_offset; break;

	case SORT_ID:
	case SORT_NAME:
	case SORT_TITLE:
	case SORT_FILE:
	case SORT_REGION:
	case SORT_WBFS:
	case SORT_NPART:
	case SORT_ITIME:
	case SORT_MTIME:
	case SORT_CTIME:
	case SORT_ATIME:
		sort_mode = SORT_NAME;
		func = sort_fst_by_path;
		break;

	default:
		break;
    }

    if ( func && part->sort_mode != sort_mode )
    {
	part->sort_mode = sort_mode;
	qsort(part->file,part->file_used,sizeof(*part->file),func);
	if ( sort_mode & SORT_REVERSE )
	    ReversePartFST(part);
    }
}

///////////////////////////////////////////////////////////////////////////////

void ReversePartFST ( WiiFstPart_t * part )
{
    if ( !part || part->file_used < 2 )
	return;

    ASSERT(part->file);

    WiiFstFile_t *a, *b, temp;
    for ( a = part->file, b = a + part->file_used-1; a < b; a++, b-- )
    {
	memcpy( &temp, a,     sizeof(WiiFstFile_t) );
	memcpy( a,     b,     sizeof(WiiFstFile_t) );
	memcpy( b,     &temp, sizeof(WiiFstFile_t) );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    IsFST()			///////////////
///////////////////////////////////////////////////////////////////////////////

enum
{
	PSUP_P_ID,
	PSUP_P_NAME,
	PSUP_P_OFFSET,

	PSUB__N
};

static SetupDef_t part_setup_def[] =
{
	{ "part-id",		0 },
	{ "part-name",		0 },
	{ "part-offset",	0x10000 },
	{0,0}
};

///////////////////////////////////////////////////////////////////////////////

enumFileType IsFST ( ccp base_path, char * id6_result )
{
    enumFileType ftype = IsFSTPart(base_path,id6_result);
    if ( ftype != FT_ID_DIR )
	return ftype;

    //----- search for a data partition

    return SearchPartitionsFST(base_path,id6_result,0,0,0) & DATA_PART_FOUND
		? FT_ID_FST | FT_A_ISO | FT_A_WII_ISO
		: FT_ID_DIR;
}

///////////////////////////////////////////////////////////////////////////////

int SearchPartitionsFST
(
	ccp base_path,
	char * id6_result,
	char * data_part,
	char * update_part,
	char * channel_part
)
{
    if (id6_result)
	memset(id6_result,0,7);

    char data_buf[PARTITION_NAME_SIZE];
    char update_buf[PARTITION_NAME_SIZE];
    char channel_buf[PARTITION_NAME_SIZE];

    if (!data_part)
	data_part = data_buf;
    if (!update_part)
	update_part = update_buf;
    if (!channel_part)
	channel_part = channel_buf;
    *data_part = *update_part = *channel_part = 0;

    bool data_found = false, update_found = false, channel_found = false;
    char path[PATH_MAX];
    
    DIR * dir = opendir(base_path);
    if (dir)
    {
	while ( !data_found || !update_found || !channel_found )
	{
	    struct dirent * dent = readdir(dir);
	    if (!dent)
		break;

	 #ifdef _DIRENT_HAVE_D_TYPE
	    if ( dent->d_type != DT_DIR )
		continue;
	 #endif

	    if ( !data_found &&
		 (!strcasecmp("data",dent->d_name) || !strcasecmp("p0",dent->d_name)))
	    {
		StringCat3S(path,sizeof(path),base_path,"/",dent->d_name);
		if ( IsFSTPart(path,id6_result) & FT_A_PART_DIR )
		{
		    data_found = true;
		    StringCopyS(data_part,PARTITION_NAME_SIZE,dent->d_name);
		}
	    }

	    if ( !update_found &&
		 (!strcasecmp("update",dent->d_name) || !strcasecmp("p1",dent->d_name)))
	    {
		StringCat3S(path,sizeof(path),base_path,"/",dent->d_name);
		if ( IsFSTPart(path,0) & FT_A_PART_DIR )
		{
		    update_found = true;
		    StringCopyS(update_part,PARTITION_NAME_SIZE,dent->d_name);
		}
	    }

	    if ( !channel_found &&
		 (!strcasecmp("channel",dent->d_name) || !strcasecmp("p2",dent->d_name)))
	    {
		StringCat3S(path,sizeof(path),base_path,"/",dent->d_name);
		if ( IsFSTPart(path,0) & FT_A_PART_DIR )
		{
		    channel_found = true;
		    StringCopyS(channel_part,PARTITION_NAME_SIZE,dent->d_name);
		}
	    }
	}
	closedir(dir);
    }

    return ( data_found    ? DATA_PART_FOUND    : 0 )
	 | ( update_found  ? UPDATE_PART_FOUND  : 0 )
	 | ( channel_found ? CHANNEL_PART_FOUND : 0 );
}

///////////////////////////////////////////////////////////////////////////////

enumFileType IsFSTPart ( ccp base_path, char * id6_result )
{
    TRACE("IsFSTPart(%s,%p)\n",base_path,id6_result);

    if (id6_result)
	memset(id6_result,0,7);

    struct stat st;
    if ( !base_path || stat(base_path,&st) || !S_ISDIR(st.st_mode) )
	return FT_UNKNOWN;

    char path[PATH_MAX];

    //----- check if sys/ and files/ exists

    StringCat2S(path,sizeof(path),base_path,"/sys");
    TRACE(" - test dir  %s\n",path);
    if ( stat(path,&st) || !S_ISDIR(st.st_mode) )
	return FT_ID_DIR;

    StringCat2S(path,sizeof(path),base_path,"/files");
    TRACE(" - test dir  %s\n",path);
    if ( stat(path,&st) || !S_ISDIR(st.st_mode) )
	return FT_ID_DIR;


    //------ boot.bin

    StringCat2S(path,sizeof(path),base_path,"/sys/boot.bin");
    TRACE(" - read file %s\n",path);
    const int fd = open(path,O_RDONLY);
    if ( fd == -1 )
	return FT_ID_DIR;
    
    if ( fstat(fd,&st)
	|| !S_ISREG(st.st_mode)
	|| st.st_size != WII_BOOT_SIZE
	|| id6_result && read(fd,id6_result,6) != 6 )
    {
	close(fd);
	if (id6_result)
	    memset(id6_result,0,7);
	return FT_ID_DIR;
    }
    close(fd);


    //----- patch id
    
    if (id6_result)
    {
	if (!ignore_setup)
	{
	    ScanSetupFile(part_setup_def,base_path,FST_SETUP_FILE,true);
	    ccp part_id = part_setup_def[PSUP_P_ID].param;
	    if (part_id)
		wd_patch_id(id6_result,id6_result,part_id,6);
	}
	PatchId(id6_result,0,6,WD_MODIFY_DISC|WD_MODIFY__AUTO);
    }


    //----- more required files
    
    ccp * fname;
    for ( fname = SpecialFilesFST; *fname; fname++ )
    {
	StringCat2S(path,sizeof(path),base_path,*fname);
	TRACE(" - test file %s\n",path);
	if ( stat(path,&st) || !S_ISREG(st.st_mode) )
	    return FT_ID_DIR;
    }
 
    TRACE(" => FST found, id=%s\n",id6_result?id6_result:"");
    return FT_ID_FST | FT_A_PART_DIR | FT_A_ISO | FT_A_WII_ISO;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  scan partition		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct scan_data_t
{
	WiiFstPart_t * part;	// valid pointer to partition
	char path[PATH_MAX];	// the current path
	char * path_part;	// ptr to 'path': beginning of partition
	char * path_dir;	// ptr to 'path': current directory
	u32  name_pool_size;	// total size of name pool
	u32  depth;		// current depth
	u64  total_size;	// total size of all files

} scan_data_t;

//-----------------------------------------------------------------------------

static u32 scan_part ( scan_data_t * sd )
{
    ASSERT(sd);
    ASSERT(sd->part);

    char * path_end = sd->path + sizeof(sd->path) - 1;
    ASSERT(sd->path_part >= sd->path && sd->path_part < path_end);
    ASSERT(sd->path_dir  >= sd->path && sd->path_dir  < path_end);

    TRACE("scan_part(%.*s|%.*s|%s) depth=%d\n",
		(int)(sd->path_part - sd->path), sd->path,
		(int)(sd->path_dir - sd->path_part), sd->path_part,
		sd->path_dir, sd->depth );

    // save current path_dir
    char * path_dir = sd->path_dir;

    u32 count = 0;
    DIR * dir = opendir(sd->path);
    if (dir)
    {
	for (;;)
	{
	    struct dirent * dent = readdir(dir);
	    if (!dent)
		break;

	    ccp name = dent->d_name;
	    sd->path_dir = StringCopyE(path_dir,path_end,name);
	    if ( *name == '.' )
	    {
		if ( !name[1] || name[1] == '.' && !name[2] )
		    continue;
		if (!FindStringField(&sd->part->include_list,sd->path_part))
		    continue;
	    }

	    struct stat st;
	    if (stat(sd->path,&st))
		continue;

	    count++;
	    WiiFstFile_t * file = AppendFileFST(sd->part);
	    ASSERT(file);

	    if (S_ISDIR(st.st_mode))
	    {
		sd->name_pool_size += strlen(name) + 1;
		file->icm = WD_ICM_DIRECTORY;
		*sd->path_dir++ = '/';
		*sd->path_dir = 0;
		file->path = strdup(sd->path_part);
		noTRACE("DIR:  %s\n",path_dir);
		file->offset4 = sd->depth++;

		// pointer 'file' becomes invalid if realloc() called => store index
		const int idx = file - sd->part->file; 
		const u32 sub_count = scan_part(sd);
		sd->part->file[idx].size = sub_count;
		count += sub_count;
		sd->depth--;
	    }
	    else if (S_ISREG(st.st_mode))
	    {
		sd->name_pool_size += strlen(name) + 1;
		file->path = strdup(sd->path_part);
		noTRACE("FILE: %s\n",path_dir);
		file->icm  = WD_ICM_FILE;
		file->size = st.st_size;
		sd->total_size += ALIGN32(file->size,4);
		CopyFileAttribStat(&sd->part->max_fatt,&st,true);
	    }
	    // else ignore all other files
	}
	closedir(dir);
    }

    // restore path_dir
    sd->path_dir = path_dir;
    return count;
}

//-----------------------------------------------------------------------------
 
u32 ScanPartFST
	( WiiFstPart_t * part, ccp base_path, u32 cur_offset4, wd_boot_t * boot )
{
    ASSERT(part);
    ASSERT(base_path);

    //----- setup

    scan_data_t sd;
    memset(&sd,0,sizeof(sd));
    sd.part = part;
    sd.path_part = StringCat2S(sd.path,sizeof(sd.path),base_path,"/");

    StringCopyE(sd.path_part,sd.path+sizeof(sd.path),FST_INCLUDE_FILE);
    ReadStringField(&part->include_list,false,sd.path,true);

    sd.path_dir = StringCopyE(sd.path_part,sd.path+sizeof(sd.path),"files/");
    WiiFstFile_t * file = AppendFileFST(sd.part);
    ASSERT(file);
    ASSERT( file == part->file );
    file->icm  = WD_ICM_DIRECTORY;
    file->path = strdup(sd.path_part);
    noTRACE("DIR:  %s\n",sd.path_dir);


    //----- scan files

    // pointer 'file' and 'part->file' becomes invalid if realloc() called
    // store the size first and then assign it (cross a sequence point)
    const u32 size = scan_part(&sd);
    ASSERT_MSG( size + 1 == part->file_used,
		"%d+1 != %d [%s]\n", size, part->file_used, part->file->path );
    part->file->size = size;

    // this sort is directory stable (first directory then contents)
    // because each directory path has a terminating '/'.
    // sorting is not neccessary but makes a listing nicer
    SortPartFST(part,SORT_NAME,SORT_NAME);


    //----- generate file table ('fst.bin')

    // alloc buffer
    sd.name_pool_size = ALIGN32(sd.name_pool_size,4);
    part->ftab_size = sizeof(wd_fst_item_t) * part->file_used + sd.name_pool_size;
    part->ftab = calloc(part->ftab_size,1);
    if (!part->ftab)
	OUT_OF_MEMORY;
    wd_fst_item_t * ftab = (wd_fst_item_t*)part->ftab;
    char *namebase = (char*)(ftab + part->file_used);
    char *nameptr = namebase;
    ASSERT( (u8*)namebase + sd.name_pool_size == part->ftab + part->ftab_size );

    if (boot)
    {
	boot->fst_off4  = htonl(cur_offset4);
	boot->fst_size4 = boot->max_fst_size4 = htonl(part->ftab_size>>2);
    }

    IsoMappingItem_t * imi;
    imi = InsertIM(&part->im,IMT_DATA,(u64)cur_offset4<<2,part->ftab_size);
    imi->part = part;
    snprintf(imi->info,sizeof(imi->info),"fst.bin, N(fst)=%u",part->file_used);
    imi->data = part->ftab;
    cur_offset4 += part->ftab_size >> 2;


    //----- fill file table

    // setup first item
    ftab->is_dir = 1;
    ftab->size = htonl(part->file_used);
    ftab++;

    const int MAX_DIR_DEPTH = 50;
    u32 basedir[MAX_DIR_DEPTH+1];
    memset(basedir,0,sizeof(basedir));

    // calculate good start offset -> avoid first sector group
    if ( cur_offset4 < WII_GROUP_DATA_SIZE4 )
	 cur_offset4 = WII_GROUP_DATA_SIZE4;
    else
	cur_offset4 = ALIGN32(cur_offset4,64>>2);
    u32 offset4 = cur_offset4;

    // iterate files and remove directories
    u32 idx = 1, file_count = 0;
    WiiFstFile_t *file_dest = part->file;
    WiiFstFile_t *file_end  = part->file + part->file_used;
    for ( file = part->file+1; file < file_end; file++ )
    {
	ftab->name_off = htonl( nameptr - namebase );
	ccp name = file->path, ptr = name;
	while (*ptr)
	    if ( *ptr++ == '/' && *ptr )
		name = ptr;
	while (*name)
	    *nameptr++ = *name++;

	if ( file->icm == WD_ICM_DIRECTORY )
	{
	    nameptr[-1] = 0; // remove trailing '/'
	    ftab->is_dir = 1;
	    ftab->size = htonl( file->size + idx + 1 );
	    u32 depth = file->offset4;
	    if ( depth < MAX_DIR_DEPTH )
	    {
		ftab->offset4 = htonl(basedir[depth]);
		basedir[depth+1] = idx;
	    }
	    FreeString(file->path);
	}
	else
	{
	    ASSERT(file->icm == WD_ICM_FILE);
	    nameptr++;
	    ftab->size = htonl(file->size);
	    ftab->offset4 = htonl(offset4);
	    file->offset4 = offset4;
	    offset4 += file->size + 3 >> 2;
	    memcpy(file_dest,file,sizeof(*file_dest));
	    file_dest++;
	    file_count++;
	}

	noTRACE("-> %4x: %u %8x %8x %s\n",
		idx, ftab->is_dir, ntohl(ftab->offset4), ntohl(ftab->size),
		namebase + ( ntohl(ftab->name_off) & 0xffffff ));

	idx++;
	ftab++;
	ASSERT( (char*)ftab <= namebase );
	ASSERT( nameptr <= namebase + sd.name_pool_size );
    }
    part->file_used = file_dest - part->file;
    ASSERT( (char*)ftab == namebase );

 #ifdef DEBUG
    {
	FILE * f = fopen("_fst.bin.tmp","wb");
	if (f)
	{
	    fwrite(part->ftab,1,part->ftab_size,f);
	    fclose(f);
	}
    }
 #endif

 #ifdef DEBUG
    if (Dump_FST_MEM(0,0,(wd_fst_item_t*)part->ftab,
				part->ftab_size,"scanned",WD_PFST__ALL))
    {
	Dump_FST_MEM(TRACE_FILE,0,(wd_fst_item_t*)part->ftab,
				part->ftab_size,"scanned",WD_PFST__ALL);
	return ERROR0(ERR_FATAL,"internal fst.bin is invalid -> see trace file\n");
    }
 #endif

    imi = InsertIM( &part->im, IMT_PART_FILES,
			(u64)cur_offset4<<2, (u64)(offset4-cur_offset4)<<2 );
    imi->part = part;
    snprintf(imi->info,sizeof(imi->info),"%u fst files",file_count);
    return offset4;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			generate partition		///////////////
///////////////////////////////////////////////////////////////////////////////

u64 GenPartFST
(
	SuperFile_t	* sf,		// valid file pointer
	WiiFstPart_t	* part,		// valid partition pointer
	ccp		path,		// path of partition directory
	u64		good_off,	// standard partition offset
	u64		min_off		// minimal possible partition offset
)
{
    ASSERT(sf);
    ASSERT(part);
    ASSERT(!part->file_used);


    //----- scan FST_SETUP_FILE

    ccp part_id = 0;
    ccp part_name = 0;
    if (!ignore_setup)
    {
	ScanSetupFile(part_setup_def,path,FST_SETUP_FILE,true);

	part_id   = part_setup_def[PSUP_P_ID].param;
	part_name = part_setup_def[PSUP_P_NAME].param;

	SetupDef_t * sdef = part_setup_def + PSUP_P_OFFSET;
	if (sdef->param)
	    good_off = sdef->value;
	PRINT("SETUP.TXT: id=%s name=%.20s off=%llx\n",part_id,part_name,good_off);
    }


    //----- setup

    if ( good_off < min_off )
	good_off = min_off;
    part->part_off = good_off;

    IsoMappingItem_t * imi;
    char pathbuf[PATH_MAX];

    const u32 good_align =  0x40;
    const u32 max_skip   = 0x100;

    WiiFst_t * fst = sf->fst;
    ASSERT(fst);


    //----- boot.bin + bi2.bin

    ASSERT( WII_BOOT_SIZE == WII_BI2_OFF );
    const u32 boot_size = WII_BOOT_SIZE + WII_BI2_SIZE;
    imi = InsertIM(&part->im,IMT_DATA,0,boot_size);
    imi->part = part;
    imi->data = malloc(boot_size);
    if (!imi->data)
	OUT_OF_MEMORY;
    imi->data_alloced = true;
    u32 cur_offset4 = ALIGN32(boot_size>>2,good_align);

    LoadFile(path, "sys/boot.bin", 0, imi->data,
		WII_BOOT_SIZE, false, &part->max_fatt, true );
    LoadFile(path, "sys/bi2.bin", 0, imi->data+WII_BOOT_SIZE,
		WII_BI2_SIZE, false, &part->max_fatt, true );

    wd_boot_t * boot = imi->data;
    char * title = boot->dhead.disc_title;
    PatchDiscHeader(&boot->dhead,part_id,part_name);

    if ( part->part_type == WD_PART_DATA )
    {
	PatchId(imi->data,0,6,WD_MODIFY_BOOT|WD_MODIFY__AUTO);
	PatchName(title,WD_MODIFY_BOOT|WD_MODIFY__AUTO);
    }
    snprintf(imi->info,sizeof(imi->info),"boot.bin [%.6s] + bi2.bin",(ccp)imi->data);

    if ( !fst->dhead.disc_id && part->part_type == WD_PART_DATA )
    {
	char * dest = (char*)&fst->dhead;
	memset(dest,0,sizeof(fst->dhead));
	memcpy(dest,boot,6);
	memcpy(dest+WII_TITLE_OFF,(ccp)boot+WII_TITLE_OFF,WII_TITLE_SIZE);
	fst->dhead.wii_magic = htonl(WII_MAGIC);
	//HEXDUMP16(3,0,dest,6);
	//HEXDUMP16(3,WII_TITLE_OFF,dest+WII_TITLE_OFF,16);
    }


    //----- disc/header.bin

    if ( part->part_type == WD_PART_DATA )
    {
	LoadFile(path,"disc/header.bin",0,
			&fst->dhead, sizeof(fst->dhead), true,
			&part->max_fatt, true);
	PatchDiscHeader(&fst->dhead,part_id,part_name);
	PatchId(&fst->dhead.disc_id,0,6,WD_MODIFY_DISC|WD_MODIFY__AUTO);
	PatchName(fst->dhead.disc_title,WD_MODIFY_DISC|WD_MODIFY__AUTO);
    }


    //----- disc/region.bin
    
    if ( part->part_type == WD_PART_DATA )
    {
	enumRegion reg = opt_region;
	const RegionInfo_t * rinfo
	    = GetRegionInfo(fst->dhead.region_code);

	if ( reg == REGION__AUTO && rinfo->mandatory )
	    reg = rinfo->reg;
	else if ( reg == REGION__AUTO || reg == REGION__FILE )
	{
	    reg = LoadFile(path,"disc/region.bin",0,
				&fst->region, sizeof(fst->region), true,
				&part->max_fatt, true )
		? REGION__AUTO : REGION__FILE;
	}

	if ( reg != REGION__FILE )
	{
	    memset( &fst->region, 0, sizeof(fst->region) );
	    fst->region.region
		= htonl(  reg == REGION__AUTO ? rinfo->reg : reg );
	}
    }


    //----- apploader.img

    ccp fpath = PathCatPP(pathbuf,sizeof(pathbuf),path,"sys/apploader.img");
    const u32 app_fsize4 = GetFileSize(fpath,0,0,&part->max_fatt,true) + 3 >> 2;
    imi = InsertIM(&part->im,IMT_FILE,WII_APL_OFF,(u64)app_fsize4<<2);
    imi->part = part;
    StringCopyS(imi->info,sizeof(imi->info),"apploader.img");
    imi->data = strdup(fpath);
    imi->data_alloced = true;

    cur_offset4 = ( WII_APL_OFF >> 2 ) + app_fsize4;
    

    //----- main.dol

    // try to use the same offset for main.dol
    u32 good_off4 = ntohl(boot->dol_off4);
    if ( good_off4 >= cur_offset4 && good_off4 <= cur_offset4 + max_skip )
	 cur_offset4 = good_off4;
    boot->dol_off4 = htonl(cur_offset4);
    
    fpath = PathCatPP(pathbuf,sizeof(pathbuf),path,"sys/main.dol");
    const u32 dol_fsize4 = GetFileSize(fpath,0,0,&part->max_fatt,true) + 3 >> 2;
    imi = InsertIM(&part->im,IMT_FILE,(u64)cur_offset4<<2,(u64)dol_fsize4<<2);
    imi->part = part;
    StringCopyS(imi->info,sizeof(imi->info),"main.dol");
    imi->data = strdup(fpath);
    imi->data_alloced = true;

    cur_offset4	+= dol_fsize4;


    //----- fst.bin

    // try to use the same offset for fst
    good_off4 = ntohl(boot->fst_off4);
    if ( good_off4 >= cur_offset4 && good_off4 <= cur_offset4 + max_skip )
	 cur_offset4 = good_off4;

    cur_offset4 = ScanPartFST(part,path,cur_offset4,boot);
    TRACE("dol4=%x,%x  fst4=%x,%x,  cur_offset4=%x\n",
	ntohl(boot->dol_off4), dol_fsize4,
	ntohl(boot->fst_off4), ntohl(boot->fst_size4),
	cur_offset4 );


    //----- get file sizes and setup partition control

    const u32 blocks = ( cur_offset4 - 1 ) / WII_SECTOR_DATA_SIZE4 + 1;

    const s64 ticket_size = GetFileSize(path,"ticket.bin",-1,&part->max_fatt,true);
    bool load_ticket = ticket_size == WII_TICKET_SIZE;
    if ( !load_ticket && ticket_size >= 0 )
	ERROR0(ERR_WARNING,"Content of file 'ticket.bin' ignored!\n");

    const s64 tmd_size = GetFileSize(path,"tmd.bin",-1,&part->max_fatt,true);
    bool load_tmd = tmd_size == WII_TMD_GOOD_SIZE;
    if ( !load_tmd && tmd_size >= 0 )
	ERROR0(ERR_WARNING,"Content of file 'tmd.bin' ignored!\n");

    const s64 cert_size = GetFileSize(path,"cert.bin",0,&part->max_fatt,true);

    wd_part_control_t *pc = malloc(sizeof(wd_part_control_t));
    if (!pc)
	OUT_OF_MEMORY;
    part->pc = pc;
 
    if (clear_part_control(pc, WII_TMD_GOOD_SIZE, cert_size,
					(u64)blocks*WII_SECTOR_SIZE) )
	return ERROR0(ERR_INVALID_FILE,"Content of file 'cert.bin' wrong!\n");


    //----- setup partition control content

    if (load_ticket)
	load_ticket = LoadFile(path,"ticket.bin", 0,
			    &pc->head->ticket, WII_TICKET_SIZE, false,
			    &part->max_fatt, true )
		    == ERR_OK;
    if (!load_ticket)
	ticket_setup(&pc->head->ticket,&fst->dhead.disc_id);
    wd_patch_id(pc->head->ticket.title_id+4,0,part_id,4);

    if (load_tmd)
	load_tmd = LoadFile( path, "tmd.bin", 0,
				pc->tmd, pc->tmd_size, false, &part->max_fatt, true )
		 == ERR_OK;
    if (!load_tmd)
	tmd_setup(pc->tmd,pc->tmd_size,&fst->dhead.disc_id);
    wd_patch_id(pc->tmd->title_id+4,0,part_id,4);

    LoadFile(path,"cert.bin",	0, pc->cert, pc->cert_size,
			false, &part->max_fatt, true );

    if ( part->part_type == WD_PART_DATA )
    {
	PatchId(pc->head->ticket.title_id+4,0,4,WD_MODIFY_TICKET|WD_MODIFY__AUTO);
	PatchId(pc->tmd->title_id+4,0,4,WD_MODIFY_TMD|WD_MODIFY__AUTO);

	if (opt_ios_valid)
	    pc->tmd->sys_version = hton64(opt_ios);
    }

    //----- setup

    InsertMemMap(&sf->modified_list,good_off,sizeof(pc->part_bin));
    
    if ( fst->encoding & ENCODE_CALC_HASH )
    {
	tmd_clear_encryption(pc->tmd,0);

	// calc partition keys
	wd_decrypt_title_key(&pc->head->ticket,part->key);
	wd_aes_set_key(&part->akey,part->key);

	// fill h3 with data to avoid sparse writing
	int groups = (blocks-1)/WII_GROUP_SECTORS+1;
	if ( groups > WII_N_ELEMENTS_H3 )
	     groups = WII_N_ELEMENTS_H3;
	memset(pc->h3,1,groups*WII_HASH_SIZE);
    }
    else
    {
	ASSERT( !( fst->encoding & (ENCODE_ENCRYPT|ENCODE_SIGN) ));
	tmd_clear_encryption(pc->tmd,1);
    }


    //----- insert this partition in mem map of fst

    imi = InsertIM(&fst->im,IMT_DATA,good_off,sizeof(pc->part_bin));
    imi->data = pc->part_bin;
    snprintf(imi->info,sizeof(imi->info),"%s partition, header",
			wd_get_part_name(part->part_type,"?"));

    imi = InsertIM(&fst->im,IMT_PART, good_off+pc->data_off, pc->data_size );
    imi->part = part;
    snprintf(imi->info,sizeof(imi->info),"%s partition, data",
			wd_get_part_name(part->part_type,"?"));


    //----- terminate

    MaxFileAttrib(&sf->f.fatt,&part->max_fatt);
    ResetSetup(part_setup_def);
    return pc->data_size;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   SetupReadFST()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupReadFST ( SuperFile_t * sf )
{
    ASSERT(sf);
    ASSERT(!sf->fst);
    TRACE("SetupReadFST() -> %x %s\n",sf->f.ftype,sf->f.fname);
    noPRINT("SetupReadFST() -> %x %s\n",sf->f.ftype,sf->f.fname);

    SetupIOD(sf,OFT_FST,OFT_FST);
    WiiFst_t * fst = malloc(sizeof(*fst));
    if (!fst)
	OUT_OF_MEMORY;
    sf->fst = fst;
    InitializeFST(fst);


    //----- setup fst->encoding mode

    enumEncoding enc = SetEncoding(encoding,0,0);

    // check ENCODE_F_ENCRYPT
    if ( enc & ENCODE_F_ENCRYPT )
	enc = SetEncoding(enc,0,ENCODE_SIGN|ENCODE_ENCRYPT|ENCODE_CALC_HASH);

    // make some dependency checks
    if ( enc & ENCODE_SIGN )
	enc = SetEncoding(enc,ENCODE_ENCRYPT|ENCODE_CALC_HASH,0);
    if ( enc & ENCODE_M_CRYPT )
	enc = SetEncoding(enc,ENCODE_CALC_HASH,ENCODE_NO_SIGN);
    if ( enc & ENCODE_M_HASH )
	enc = SetEncoding(enc,0,ENCODE_NO_SIGN|ENCODE_DECRYPT);

    // force enrypting and signing if at least one is set
    if ( enc & (ENCODE_SIGN|ENCODE_ENCRYPT) )
	enc = SetEncoding(enc,ENCODE_SIGN|ENCODE_ENCRYPT|ENCODE_CALC_HASH,0);

    fst->encoding = SetEncoding(enc,0,
		enc & ENCODE_F_FAST
			? ENCODE_NO_SIGN|ENCODE_DECRYPT|ENCODE_CLEAR_HASH
			: ENCODE_SIGN|ENCODE_ENCRYPT|ENCODE_CALC_HASH );
    PRINT("ENCODING: %04x -> %04x\n",encoding,fst->encoding);


    //----- setup partitions --> [2do] use part_selector

    u64 min_offset	= WII_PART_OFF;
    u64 update_off	= WII_PART_OFF;
    u64 update_size	= 0;
    u64 data_off	= WII_GOOD_DATA_PART_OFF;
    u64 data_size	= 0;
    u64 channel_off	= WII_GOOD_DATA_PART_OFF;
    u64 channel_size	= 0;
    WiiFstPart_t * data_part = 0;

    if ( sf->f.ftype & FT_A_PART_DIR )
    {
	min_offset = wd_align_part(min_offset,opt_align_part,false);
	data_part = AppendPartFST(fst);
	data_part->part_type = WD_PART_DATA;
	data_size = GenPartFST(sf,data_part,sf->f.fname,data_off,min_offset);
	data_off  = data_part->part_off;
	data_part->path = strdup(sf->f.fname);
	min_offset	= data_off
			+ WII_PARTITION_BIN_SIZE
			+ data_size;
    }
    else
    {
	char data_path[PARTITION_NAME_SIZE];
	char update_path[PARTITION_NAME_SIZE];
	char channel_path[PARTITION_NAME_SIZE];

	int stat = SearchPartitionsFST(sf->f.fname,0,data_path,update_path,channel_path);
	if ( !stat & DATA_PART_FOUND )
	    return ERROR0(ERR_INVALID_FILE,"Not a valid FST: %s\n",sf->f.fname);

	char path_buf[PATH_MAX];
	if ( stat & UPDATE_PART_FOUND )
	{
	    min_offset = wd_align_part(min_offset,opt_align_part,false);
	    ccp path = PathCatPP(path_buf,sizeof(path_buf),sf->f.fname,update_path);
	    WiiFstPart_t * update_part = AppendPartFST(fst);
	    update_part->part_type = WD_PART_UPDATE;
	    update_size = GenPartFST(sf,update_part,path,update_off,min_offset);
	    update_off	= update_part->part_off;
	    min_offset	= update_off
			+ WII_PARTITION_BIN_SIZE
			+ update_size;
	    update_part->path = strdup(path);
	}

	min_offset = wd_align_part(min_offset,opt_align_part,false);
	ccp path = PathCatPP(path_buf,sizeof(path_buf),sf->f.fname,data_path);
	data_part = AppendPartFST(fst);
	data_part->part_type = WD_PART_DATA;
	data_size = GenPartFST(sf,data_part,path,data_off,min_offset);
	data_off  = data_part->part_off;
	min_offset	= data_off
			+ WII_PARTITION_BIN_SIZE
			+ data_size;
	data_part->path = strdup(path);

	if ( stat & CHANNEL_PART_FOUND )
	{
	    min_offset = wd_align_part(min_offset,opt_align_part,false);
	    ccp path = PathCatPP(path_buf,sizeof(path_buf),sf->f.fname,channel_path);
	    WiiFstPart_t * channel_part = AppendPartFST(fst);
	    channel_part->part_type = WD_PART_CHANNEL;
	    channel_size = GenPartFST(sf,channel_part,path,channel_off,min_offset);
	    channel_off  = channel_part->part_off;
	    min_offset	= channel_off
			+ WII_PARTITION_BIN_SIZE
			+ channel_size;
	    channel_part->path = strdup(path);
	}
    }
    ASSERT(data_part);

    TRACE("UPD: %9llx .. %9llx : %9llx\n",update_off, update_off+update_size, update_size);
    TRACE("DAT: %9llx .. %9llx : %9llx\n",data_off, data_off+data_size, data_size);
    TRACE("CHN: %9llx .. %9llx : %9llx\n",channel_off, channel_off+channel_size, channel_size);

    //----- setup partition header

    const u32 n_part	    = 1 + (update_size>0) + (channel_size>0);
    const u64 cnt_tab_size  = WII_MAX_PTAB * sizeof(wd_ptab_info_t);
    const u64 part_tab_size = cnt_tab_size
			    + n_part * sizeof(wd_ptab_entry_t);

    IsoMappingItem_t * imi;
    imi = InsertIM(&fst->im,IMT_DATA,WII_PTAB_REF_OFF,part_tab_size);
    StringCopyS(imi->info,sizeof(imi->info),"partition tables");
    imi->data = calloc(1,part_tab_size);
    if (!imi->data)
	OUT_OF_MEMORY;
    imi->data_alloced = true;

    wd_ptab_info_t * pcount = imi->data;
    pcount->n_part = htonl(n_part);
    pcount->off4   = htonl( WII_PTAB_REF_OFF + cnt_tab_size >> 2 );

    wd_ptab_entry_t * pentry = (void*)((ccp)imi->data+cnt_tab_size);
    if (update_size)
    {
	pentry->off4  = htonl( update_off >> 2 );
	pentry->ptype = htonl( WD_PART_UPDATE );
	pentry++;
    }
    pentry->off4  = htonl( data_off >> 2 );
    pentry->ptype = htonl( WD_PART_DATA );
    if (channel_size)
    {
	pentry++;
	pentry->off4  = htonl( channel_off >> 2 );
	pentry->ptype = htonl( WD_PART_CHANNEL );
    }


    //----- setup work+cache buffer of WII_GROUP_SIZE
    //		== WII_N_ELEMENTS_H1 * WII_N_ELEMENTS_H2 * WII_SECTOR_SIZE

    ASSERT(!fst->cache);
    ASSERT(!fst->cache_part);
    fst->cache = malloc(WII_GROUP_SIZE);
    if (!fst->cache)
	OUT_OF_MEMORY;
    TRACE("CACHE= %x/hex = %u bytes\n",WII_GROUP_SIZE,WII_GROUP_SIZE);


    //----- setup mapping

    imi = InsertIM(&fst->im,IMT_DATA,0,sizeof(fst->dhead));
    imi->data = &fst->dhead;
    snprintf(imi->info,sizeof(imi->info),"disc header [%.6s]",(ccp)imi->data);

    imi = InsertIM(&fst->im,IMT_DATA,WII_REGION_OFF,sizeof(fst->region));
    snprintf(imi->info,sizeof(imi->info),"region settings, region=%u",
		ntohl(fst->region.region));
    imi->data = &fst->region;

    static u8 magic[] = { 0xc3, 0xf8, 0x1a, 0x8e };
    ASSERT( be32(magic) == WII_MAGIC2 );
    ASSERT( sizeof(magic) == WII_MAGIC2_LEN );
    imi = InsertIM(&fst->im,IMT_DATA,WII_MAGIC2_OFF,sizeof(magic));
    StringCopyS(imi->info,sizeof(imi->info),"magic c3-f8-1a-8e");
    imi->data = &magic;


    //----- terminate

 #ifdef DEBUG
    PrintFstIM(fst,TRACE_FILE,0,true,"Memory layout of virtual");
 #endif
    if ( logging > 0 )
	PrintFstIM(fst,stdout,0,logging>1,"Memory layout of virtual");

    const off_t single_size = WII_SECTORS_SINGLE_LAYER * (off_t)WII_SECTOR_SIZE;
    sf->file_size = min_offset > single_size ? min_offset : single_size;

    wd_disc_t * disc = OpenDiscSF(sf,false,true);
    if (!disc)
	return ERROR0(ERR_INTERNAL,"Composing error: %s\n",sf->f.fname);
    sf->f.fatt.size = wd_count_used_disc_blocks(disc,1,0) * (u64)WII_SECTOR_SIZE;

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			UpdateSignatureFST()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError UpdateSignatureFST ( WiiFst_t * fst )
{
    TRACE("UpdateSignatureFST(%p) ENCODING=%d\n",fst, fst ? fst->encoding : 0);
    if ( !fst || fst->encoding & ENCODE_CLEAR_HASH )
	return ERR_OK;

    // invalidate cache
    fst->cache_part = 0;

    // iterate partitions
    WiiFstPart_t *part, *part_end = fst->part + fst->part_used;
    for ( part = fst->part; part < part_end; part++ )
    {
	wd_part_control_t * pc = part->pc;
	TRACE(" - part #%zu: pc=%p\n",part-fst->part,pc);
	if (pc)
	{
	    if (pc->tmd_content)
		SHA1( pc->h3, pc->h3_size, pc->tmd_content->hash );
	    if ( fst->encoding & ENCODE_SIGN )
		part_control_fake_sign(pc,0);

	 #ifdef DEBUG
	    {
		u8 hash[WII_HASH_SIZE];
		SHA1( ((u8*)pc->tmd)+WII_TMD_SIG_OFF, pc->tmd_size-WII_TMD_SIG_OFF, hash );
		TRACE("FAKESIGN: ");
		TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,hash,WII_HASH_SIZE);
	    }
	 #endif
	}
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		      SF/FST read support		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError ReadFST ( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    TRACE("---\n");
    TRACE(TRACE_RDWR_FORMAT, "#FST# ReadFST()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    ASSERT(sf);
    ASSERT(sf->fst);

    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    const IsoMappingItem_t * imi = sf->fst->im.field;
    const IsoMappingItem_t * imi_end = imi + sf->fst->im.used;
    char * dest = buf;
    sf->f.bytes_read += count;

    while ( count > 0 )
    {
	TRACE("imi-idx=%zu off=%llx data=%zx count=%zx\n",
		imi - sf->fst->im.field, (u64)off, dest-(ccp)buf, count );
	while ( imi < imi_end && off >= imi->offset + imi->size )
	    imi++;

	TRACE("imi-idx=%zu imt=%u, off=%llx size=%llx\n",
		imi - sf->fst->im.field, imi->imt, (u64)imi->offset, imi->size );

	if ( imi == imi_end || off + count <= imi->offset )
	{
	    TRACE(">FILL %zx=%zu\n",count,count);
	    memset(dest,0,count);
	    return ERR_OK;
	}

	ASSERT( off < imi->offset + imi->size && off + count > imi->offset );
	if ( off < imi->offset )
	{
	    TRACELINE;
	    size_t fill_count = count;
	    if ( fill_count > imi->offset - off )
		 fill_count = imi->offset - off;
	    TRACE(">FILL %zx=%zu\n",fill_count,fill_count);
	    memset(dest,0,fill_count);

	    off   += fill_count;
	    dest  += fill_count;
	    count -= fill_count;
	}
	ASSERT( off >= imi->offset );
	ASSERT( count );

	TRACELINE;
	const off_t  delta = off - imi->offset;
	const off_t  max_size = imi->size - delta;
	const size_t copy_count = count < max_size ? count : max_size;
	switch(imi->imt)
	{
	    case IMT_ID:
		TRACE(">ID %zx=%zu\n",copy_count,copy_count);
		PatchId(dest,delta,copy_count,WD_MODIFY__ALWAYS);
		break;

	    case IMT_DATA:
		TRACE(">COPY %zx=%zu\n",copy_count,copy_count);
		memcpy(dest,(ccp)imi->data+delta,copy_count);
		break;

	    case IMT_PART:
		ASSERT(imi->part);
		{
		    const enumError err
			= ReadPartFST(sf,imi->part,delta,dest,copy_count);
		    if (err)
			return err;
		}
		break;

	    default:
		TRACELINE;
		return ERROR0(ERR_INTERNAL,0);
	}
	off   += copy_count;
	dest  += copy_count;
	count -= copy_count;
    }

    TRACE("READ done! [%u]\n",__LINE__);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadPartFST ( SuperFile_t * sf, WiiFstPart_t * part,
			off_t off, void * buf, size_t count )
{
    TRACE(TRACE_RDWR_FORMAT, "#PART# ReadPartFST()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    ASSERT(sf);
    ASSERT(sf->fst);
    ASSERT(sf->fst->cache);
    ASSERT(part);

    WiiFst_t * fst = sf->fst;
    char *dest = buf;
    TRACE("CACHE=%p, buf=%p, dest=%p\n",fst->cache,buf,dest);

    u32 group = off/WII_GROUP_SIZE;
    const off_t skip = off - group * (off_t)WII_GROUP_SIZE;
    if (skip)
    {
	TRACE("READ/skip=%llx off=%llx dest=+%zx\n",(u64)skip,(u64)off,dest-(ccp)buf);

	if ( fst->cache_part != part || fst->cache_group != group )
	{
	    const enumError err = ReadPartGroupFST(sf,part,group,fst->cache,1);
	    if (err)
		return err;
	    fst->cache_part  = part;
	    fst->cache_group = group;
	}

	ASSERT( skip < WII_GROUP_SIZE );
	u32 copy_len = WII_GROUP_SIZE - skip;
	if ( copy_len > count )
	     copy_len = count;

	TRACE("COPY/len=%x\n",copy_len);
	memcpy(dest,fst->cache+skip,copy_len);
	TRACELINE;
	dest  += copy_len;
	count -= copy_len;
	group++;
    }

    const u32 n_groups = count / WII_GROUP_SIZE;
    if ( n_groups > 0 )
    {
	TRACE("READ/n_groups=%x off=%llx dest=+%zx\n",n_groups,(u64)off,dest-(ccp)buf);

	// whole groups are not cached because the next read will read next group
	const enumError err = ReadPartGroupFST(sf,part,group,dest,n_groups);
	if (err)
	    return err;

	const size_t read_count = n_groups * (size_t)WII_GROUP_SIZE;
	dest  += read_count;
	count -= read_count;
	group += n_groups;
    }

    if ( count > 0 )
    {
	ASSERT( count < WII_GROUP_SIZE );
	TRACE("READ/count=%zx off=%llx dest=+%zx\n",count,(u64)off,dest-(ccp)buf);

	if ( fst->cache_part != part || fst->cache_group != group )
	{
	    const enumError err = ReadPartGroupFST(sf,part,group,fst->cache,1);
	    if (err)
		return err;
	    fst->cache_part  = part;
	    fst->cache_group = group;
	}
	TRACE("COPY/len=%zx\n",count);
	memcpy(dest,fst->cache,count);
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadPartGroupFST ( SuperFile_t * sf, WiiFstPart_t * part,
				u32 group_no, void * buf, u32 n_groups )
{
    TRACELINE;
    TRACE(TRACE_RDWR_FORMAT, "#PART# read GROUP",
		GetFD(&sf->f), GetFP(&sf->f),
		(u64)group_no, (u64)group_no+n_groups, (size_t)n_groups, "" );

    ASSERT(sf);
    ASSERT(part);
    ASSERT(buf);

    if (!n_groups)
	return ERR_OK;

    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    //----- setup vars

    const u32 delta = n_groups * WII_SECTOR_HASH_SIZE  * WII_GROUP_SECTORS;
    const u32 dsize = n_groups * WII_GROUP_DATA_SIZE;
          u64 off   = group_no * (u64)WII_GROUP_DATA_SIZE;
    const u64 max   = n_groups * (u64)WII_GROUP_DATA_SIZE + off;
    TRACE("delta=%x, dsize=%x, off=%llx..%llx\n",delta,dsize,off,max);

    char * dest = (char*)buf + delta, *src = dest;
    memset(dest,0,dsize);
    TRACE("CACHE=%p, buf=%p, dest=%p\n",sf->fst->cache,buf,dest);
    
    const IsoMappingItem_t * imi = part->im.field;
    const IsoMappingItem_t * imi_end = imi + part->im.used;

    //----- load data

    while ( off < max )
    {
	if (SIGINT_level>1)
	    return ERR_INTERRUPT;

	while ( imi < imi_end && off >= imi->offset + imi->size )
	    imi++;
	TRACE("imi-idx=%zu imt=%u, off=%llx size=%llx\n",
		imi - part->im.field, imi->imt, imi->offset, imi->size );
	TRACE("off=%llx..%llx  dest=%p+%zx\n",off,max,buf,dest-src);
	
	if ( imi == imi_end || max <= imi->offset )
	    break;

	ASSERT( off < imi->offset + imi->size && max > imi->offset );
	if ( off < imi->offset )
	{
	    TRACELINE;
	    size_t skip_count = max-off;
	    if ( skip_count > imi->offset - off )
		 skip_count = imi->offset - off;
	    TRACE("SKIP %zx\n",skip_count);
	    off   += skip_count;
	    dest  += skip_count;
	}
	ASSERT( off >= imi->offset );

	TRACELINE;

	const u32 skip_count = off - imi->offset;
	ASSERT( skip_count <= imi->size );
	u32 max_copy = max - off;
	if ( max_copy > imi->size - skip_count )
	     max_copy = imi->size - skip_count;

	switch(imi->imt)
	{
	    case IMT_ID:
		noTRACE("IMT_ID: %x %x -> %zx (%s)\n",
			skip_count, max_copy, dest-src, imi->info );
		PatchId(dest,skip_count,max_copy,WD_MODIFY__ALWAYS);
		break;

	    case IMT_DATA:
		noTRACE("IMT_DATA: %x %x -> %zx (%s)\n",
			skip_count, max_copy, dest-src, imi->info );
		ASSERT(imi->data);
		memcpy(dest,imi->data+skip_count,max_copy);
		//HEXDUMP16(3,0,dest,max_copy<0x40?max_copy:0x40);
		break;

	    case IMT_FILE:
		noTRACE("IMT_FILE: %x %x -> %zx (%s)\n",
			skip_count, max_copy, dest-src, imi->info );
		ASSERT(imi->data);
		LoadFile(imi->data,0,skip_count,dest,max_copy,false,0,false);
		break;

	    case IMT_PART_FILES:
		noTRACE("IMT_PART_FILES: %x %x -> %zx (%s)\n",
			skip_count, max_copy, dest-src, imi->info );
		if (part->file_used)
		{
		    WiiFstFile_t * file = FindFileFST(part,off>>2);
		    WiiFstFile_t * file_end = part->file + part->file_used;
		    ASSERT(file);
		    u64  loff	= off;	// local copy
		    char *ldest	= dest;	// local copy
		    while ( file < file_end && loff < max )
		    {
			u64 foff = (u64)file->offset4 << 2;
			if ( loff < foff )
			{
			    TRACE("SKIP %llx [%s]\n",foff-loff,file->path);
			    ldest += foff - loff;
			    loff = foff;
			}

			if ( loff < foff + file->size )
			{
			    u32 skip = loff - foff;
			    u32 load_size = file->size - skip;
			    if ( load_size > max-loff )
				 load_size = max-loff;
			    LoadFile(part->path,file->path,skip,ldest,load_size,false,0,false);
			    ldest += load_size;
			    loff  += load_size;
			}
			file++;
		    }
		}
		break;

	    default:
		TRACELINE;
		return ERROR0(ERR_INTERNAL,0);
	}
	//TRACE_HEXDUMP16(3,0,dest,max_copy<0x40?max_copy:0x40);
	off  += max_copy;
	dest += max_copy;
    }
    
    //----- move data into blocks

    dest = buf;
    for ( dest = buf; dest < src; )
    {
	memset(dest,0,WII_SECTOR_HASH_SIZE);
	dest += WII_SECTOR_HASH_SIZE;

	TRACE("GROUP %u+%u (%zx<-%zx)\n",
		group_no, (u32)(dest-(ccp)buf)/WII_SECTOR_SIZE,
		dest-(ccp)buf, src-(ccp)buf );

	memmove(dest,src,WII_SECTOR_DATA_SIZE);
	dest += WII_SECTOR_DATA_SIZE;
	src  += WII_SECTOR_DATA_SIZE;
    };
    ASSERT( dest == src );
    ASSERT( dest == (ccp)buf + n_groups * WII_GROUP_SIZE );


    //----- encrypt groups

    if ( sf->fst->encoding & ENCODE_CALC_HASH )
    {
	for ( dest = buf; dest < src; dest += WII_GROUP_SIZE )
	{
	    EncryptSectorGroup
	    (
		sf->fst->encoding & ENCODE_ENCRYPT ? &part->akey : 0,
		(wd_part_sector_t*)dest,
		WII_GROUP_SECTORS,
		group_no < WII_N_ELEMENTS_H3 ? part->pc->h3 + group_no * WII_HASH_SIZE : 0
	    );
	    group_no++;
	}
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			EncryptSectorGroup()		///////////////
///////////////////////////////////////////////////////////////////////////////

void EncryptSectorGroup
(
    const aes_key_t	* akey,
    wd_part_sector_t	* sect0,
    size_t		n_sectors,
    void		* h3
)
{
    // ASSUME: all header data is cleared (set to 0 in each byte)

    ASSERT(sect0);
    TRACE("EncryptSectorGroup(%p,%p,%zx,h3=%p)\n",akey,sect0,n_sectors,h3);

    if (!n_sectors)
	return;

    if ( n_sectors > WII_N_ELEMENTS_H1 * WII_N_ELEMENTS_H2 )
	 n_sectors = WII_N_ELEMENTS_H1 * WII_N_ELEMENTS_H2;

    wd_part_sector_t * max_sect = sect0 + n_sectors;

    //----- calc SHA-1 for each H0 data for each sector

    int i, j;
    wd_part_sector_t *sect = sect0;
    for ( i = 0; i < n_sectors; i++, sect++ )
	for ( j = 0; j < WII_N_ELEMENTS_H0; j++ )
	    SHA1(sect->data[j],WII_H0_DATA_SIZE,sect->h0[j]);
    
    //----- calc SHA-1 for each H0 hash and copy to others

    for ( sect = sect0; sect < max_sect; )
    {
	wd_part_sector_t * store_sect = sect;
	for ( j = 0; j < WII_N_ELEMENTS_H1 && sect < max_sect; j++, sect++ )
	    SHA1(*sect->h0,sizeof(sect->h0),store_sect->h1[j]);

	wd_part_sector_t * dest_sect;
	for ( dest_sect = store_sect + 1; dest_sect < sect; dest_sect++ )
	    memcpy(dest_sect->h1,store_sect->h1,sizeof(dest_sect->h1));
    }

    //----- calc SHA-1 for each H1 hash and copy to others

    for ( j = 0, sect = sect0; sect < max_sect; j++, sect += WII_N_ELEMENTS_H1 )
	SHA1(*sect->h1,sizeof(sect->h1),sect0->h2[j]);

    for ( sect = sect0+1; sect < max_sect; sect++ )
	memcpy(sect->h2,sect0->h2,sizeof(sect->h2));

    //----- calc SHA-1 of last header and store it in h3

    if (h3)
	SHA1(*sect0->h2,sizeof(sect0->h2),h3);

    //----- encrypt header + data

    if (akey)
	wd_encrypt_sectors(0,akey,sect0,0,sect0,n_sectors);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Verify			///////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef TEST
  #define WATCH_BLOCK 0
#else
  #define WATCH_BLOCK 0
#endif

///////////////////////////////////////////////////////////////////////////////

void InitializeVerify ( Verify_t * ver, SuperFile_t * sf )
{
    TRACE("#VERIFY# InitializeVerify(%p,%p)\n",ver,sf);
    ASSERT(ver);
    memset(ver,0,sizeof(*ver));

    ver->sf		= sf;
    ver->verbose	= verbose;
    ver->max_err_msg	= 10;
}

///////////////////////////////////////////////////////////////////////////////

void ResetVerify ( Verify_t * ver )
{
    TRACE("#VERIFY# ResetVerify(%p)\n",ver);
    ASSERT(ver);
    InitializeVerify(ver,ver->sf);
}

///////////////////////////////////////////////////////////////////////////////

static enumError PrintVerifyMessage ( Verify_t * ver, ccp msg )
{
    DASSERT(ver);
    if ( ver->verbose >= -1 )
    {
	DASSERT(ver->sf);
	DASSERT(ver->part);
	DASSERT(msg);

	wd_part_t * part = ver->part;
	DASSERT(part);

	char pname_buf[20];
	wd_print_part_name(pname_buf,sizeof(pname_buf),part->part_type,WD_PNAME_NAME);

	char count_buf[100];
	if (!ver->disc_index)
	{
	    // no index supported
	    snprintf( count_buf, sizeof(count_buf),".%u%c",
			part->index,
			part->is_encrypted ? ' ' : 'd' );
	}
	else if ( ver->disc_total < 2 )
	{
	    // a single check
	    snprintf( count_buf, sizeof(count_buf),"%u.%u%c",
			ver->disc_index,
			part->index,
			part->is_encrypted ? ' ' : 'd' );
	}
	else
	{
	    // calculate fw
	    const int fw = snprintf(count_buf,sizeof(count_buf),"%u",ver->disc_total);
	    snprintf( count_buf, sizeof(count_buf),"%*u.%u/%u%c",
			fw,
			ver->disc_index,
			part->index,
			ver->disc_total,
			part->is_encrypted ? ' ' : 'd' );
	}

	printf("%*s%-7.7s %s %n%-7s %6.6s %s\n",
		ver->indent, "",
		msg, count_buf, &ver->info_indent,
		pname_buf, (ccp)&ver->sf->disc2->dhead,
		ver->fname ? ver->fname : ver->sf->f.fname );
	fflush(stdout);

	TRACE("%*s%-7.7s %s %n%-7s %6.6s %s\n",
		ver->indent, "",
		msg, count_buf, &ver->info_indent,
		pname_buf, (ccp)&ver->sf->disc2->dhead,
		ver->fname ? ver->fname : ver->sf->f.fname );
    }
    return ERR_DIFFER;
}

///////////////////////////////////////////////////////////////////////////////

static enumError VerifyHash
	( Verify_t * ver, ccp msg, u8 * data, size_t data_len, u8 * ref )
{
    DASSERT(ver);
    DASSERT(ver->part);

 #if WATCH_BLOCK && 0
    {
	const u32 delta_off = (ccp)data - iobuf;
	const u32 delta_blk = delta_off / WII_SECTOR_SIZE;
	const u64 offset    = ver->part->part_offset
			    + ver->part->pc->data_off
			    + ver->group * (u64)WII_GROUP_SIZE
			    + delta_off;
	const u32 block	    = offset / WII_SECTOR_SIZE;

	if ( block == WATCH_BLOCK )
	    printf("WB: %s: delta=%x->%x, cmp(%02x,%02x)\n",
			msg, delta_off, delta_blk, *hash, *ref );
    }
 #endif

    u8 hash[WII_HASH_SIZE];
    SHA1(data,data_len,hash);
    if (!memcmp(hash,ref,WII_HASH_SIZE))
	return ERR_OK;

    PrintVerifyMessage(ver,msg);
    if ( ver->verbose >= -1 && ver->long_count > 0 )
    {
	bool flush = false;

	const u32 delta_off	= (ccp)data - iobuf;
	const u32 delta_blk	= delta_off / WII_SECTOR_SIZE;
	const u64 offset	= ( (u64)ver->part->data_off4 << 2 )
				+ ver->group * (u64)WII_GROUP_SIZE
				+ delta_off;
	const int ref_delta	= ref - data; 

	if ( (ccp)ref >= iobuf && (ccp)ref <= iobuf + sizeof(iobuf) )
	{
	    const u32 block	= offset / WII_SECTOR_SIZE;
	    const u32 block_off	= offset - block * (u64)WII_SECTOR_SIZE;
	    const u32 sub_block	= block_off / WII_SECTOR_HASH_SIZE;

	    printf("%*sgroup=%x.%x, block=%x.%x, data-off=%llx=B+%x, hash-off=%llx=B+%x\n",
			ver->info_indent, "",
			ver->group, delta_blk, block, sub_block,
			offset, block_off, offset + ref_delta, block_off + ref_delta );
	    flush = true;
	}

	if ( ver->long_count > 1 )
	{
	    const size_t dsize = data_len < 16 ?  data_len : 16;
	    printf("%*sDATA",ver->info_indent,"");
	    HexDump(stdout,0,offset,10,dsize,data,dsize);

	    printf("%*sHASH",ver->info_indent,"");
	    HexDump(stdout,0,offset+ref_delta,10,-WII_HASH_SIZE,ref,WII_HASH_SIZE);

	    printf("%*sVERIFY",ver->info_indent,"");
	    HexDump(stdout,0,0,8,-WII_HASH_SIZE,hash,WII_HASH_SIZE);

	    flush = true;
	}

	if (flush)
	    fflush(stdout);
    }
    return ERR_DIFFER;
}

///////////////////////////////////////////////////////////////////////////////

enumError VerifyPartition ( Verify_t * ver )
{
    DASSERT(ver);
    DASSERT(ver->sf);
    DASSERT(ver->part);

    TRACE("#VERIFY# VerifyPartition(%p) sf=%p utab=%p part=%p\n",
		ver, ver->sf, ver->usage_tab, ver->part );
    TRACE(" - psel=%p, v=%d, lc=%d, maxerr=%d\n",
		ver->psel, ver->verbose, ver->long_count, ver->max_err_msg );

 #if WATCH_BLOCK
    printf("WB: WATCH BLOCK %x = %u\n",WATCH_BLOCK,WATCH_BLOCK);
 #endif

    DASSERT( sizeof(iobuf) >= 2*WII_GROUP_SIZE );
    if ( sizeof(iobuf) < WII_GROUP_SIZE )
	return ERROR0(ERR_INTERNAL,0);

    ver->indent = NormalizeIndent(ver->indent);

    if (  ver->verbose > 0
	|| ver->verbose >= 0
	    && ( !ver->part->is_encrypted
		|| ! ( CERT_F_HASH_OK
			& wd_get_cert_ticket_stat(ver->part,true)
			& wd_get_cert_tmd_stat(ver->part,true) )))
    {
	PrintVerifyMessage(ver,">scan");
	printf("%-*s%s\n", ver->info_indent+8, ">info",
		wd_print_sig_status(0,0,ver->part,true,true) );
    }
    else if ( ver->verbose > 1 )
	PrintVerifyMessage(ver,">scan");


    //----- check partition header

    wd_part_t * part = ver->part;
    wd_load_part(part,false,true,false);

    if (!part->is_valid)
    {
	PrintVerifyMessage(ver,"!INVALID");
	return ERR_DIFFER;
    }

    if (tmd_is_marked_not_encrypted(part->tmd))
    {
	PrintVerifyMessage(ver,"!NO-HASH");
	return ERR_DIFFER;
    }


    //----- calculate end of blocks

    u32 block = part->data_off4 / WII_SECTOR_SIZE4;
    u32 block_end = block + part->ph.data_size4 / WII_SECTOR_SIZE4;
    if ( block_end > WII_MAX_SECTORS )
	 block_end = WII_MAX_SECTORS;

    TRACE(" - Partition %u [%s], valid=%d, blocks=%x..%x [%llx..%llx]\n",
		part->part_type,
		wd_get_part_name(part->part_type,"?"),
		part->is_valid,
		block, block_end,
		block * (u64)WII_SECTOR_SIZE, block_end * (u64)WII_SECTOR_SIZE );
    //TRACE_HEXDUMP16(0,block,ver->usage_tab,block_end-block);


    //----- more setup
    
    DASSERT(part->h3);
    const u8  usage_tab_marker = part->usage_id | WD_USAGE_F_CRYPT;
    const int max_differ_count = ver->verbose <= 0
				? 1
				: ver->max_err_msg > 0
					? ver->max_err_msg
					: INT_MAX;
    int differ_count = 0;

    aes_key_t akey;
    wd_aes_set_key(&akey,part->key);

    for ( ver->group = 0; block < block_end; ver->group++, block += WII_GROUP_SECTORS )
    {
	//----- preload data

     #if WATCH_BLOCK
	bool wb_trigger = false;
     #endif

	u32 blk = block;
	int index, found = -1, found_end = -1;
	for ( index = 0; blk < block_end && index < WII_GROUP_SECTORS; blk++, index++ )
	{
	 #if WATCH_BLOCK
	    if ( blk == WATCH_BLOCK )
	    {
		wb_trigger = true;
		printf("WB: ver->usage_tab[%x] = %x, usage_tab_marker=%x\n",
			blk, ver->usage_tab[blk], usage_tab_marker );
	    }
	 #endif
	    if ( ver->usage_tab[blk] == usage_tab_marker )
	    {
		found_end = index+1;
		if ( found < 0 )
		    found = index;
	    }
	}

     #if WATCH_BLOCK
	if (wb_trigger)
	    printf("WB: found=%d..%d\n",found,found_end);
     #endif

	if ( found < 0 )
	{
	    // nothing to do
	    continue;
	}

	const u64 read_off = (block+found) * (u64)WII_SECTOR_SIZE;
	wd_part_sector_t * read_sect = (wd_part_sector_t*)iobuf + found;
	if ( part->is_encrypted )
	    read_sect += WII_GROUP_SECTORS; // inplace decryption not possible
	const enumError err
	    = ReadSF( ver->sf, read_off, read_sect, (found_end-found)*WII_SECTOR_SIZE );
	if (err)
	    return err;

	//----- iterate through preloaded data

	blk = block;
	wd_part_sector_t * sect_h2 = 0;
	int i2; // iterate through H2 elements
	for ( i2 = 0; i2 < WII_N_ELEMENTS_H2 && blk < block_end; i2++ )
	{
	    wd_part_sector_t * sect_h1 = 0;
	    int i1; // iterate through H1 elements
	    for ( i1 = 0; i1 < WII_N_ELEMENTS_H1 && blk < block_end; i1++, blk++ )
	    {
		if (SIGINT_level>1)
		    return ERR_INTERRUPT;

		if ( ver->usage_tab[blk] != usage_tab_marker )
		    continue;

		//----- we have found a used blk -----

		wd_part_sector_t *sect	= (wd_part_sector_t*)iobuf
					+ i2 * WII_N_ELEMENTS_H1 + i1;

		if ( part->is_encrypted )
		    wd_decrypt_sectors(0,&akey,sect+WII_GROUP_SECTORS,sect,0,1);


		//----- check H0 -----

		int i0;
		for ( i0 = 0; i0 < WII_N_ELEMENTS_H0; i0++ )
		{
		    if (VerifyHash(ver,"!H0-ERR",sect->data[i0],WII_H0_DATA_SIZE,sect->h0[i0]))
		    {
			if ( ++differ_count >= max_differ_count )
			    goto abort;
		    }
		}

		//----- check H1 -----

		if (VerifyHash(ver,"!H1-ERR",*sect->h0,sizeof(sect->h0),sect->h1[i1]))
		{
		    if ( ++differ_count >= max_differ_count )
			goto abort;
		    continue;
		}

		//----- check first H1 -----

		if (!sect_h1)
		{
		    // first valid H1 sector
		    sect_h1 = sect;

		    //----- check H1 -----

		    if (VerifyHash(ver,"!H2-ERR",*sect->h1,sizeof(sect->h1),sect->h2[i2]))
		    {
			if ( ++differ_count >= max_differ_count )
			    goto abort;
		    }
		}
		else
		{
		    if (memcmp(sect->h1,sect_h1->h1,sizeof(sect->h1)))
		    {
			PrintVerifyMessage(ver,"!H1-DIFF");
			if ( ++differ_count >= max_differ_count )
			    goto abort;
		    }
		}

		//----- check first H2 -----

		if (!sect_h2)
		{
		    // first valid H2 sector
		    sect_h2 = sect;

		    //----- check H3 -----

		    u8 * h3 = part->h3 + ver->group * WII_HASH_SIZE;
		    if (VerifyHash(ver,"!H3-ERR",*sect->h2,sizeof(sect->h2),h3))
		    {
			if ( ++differ_count >= max_differ_count )
			    goto abort;
		    }
		}
		else
		{
		    if (memcmp(sect->h2,sect_h2->h2,sizeof(sect->h2)))
		    {
			PrintVerifyMessage(ver,"!H2-DIFF");
			if ( ++differ_count >= max_differ_count )
			    goto abort;
		    }
		}
	    }
	}
    }

    //----- check H4 -----

    if ( part->tmd && part->tmd->n_content > 0 )
    {
	u8 hash[WII_HASH_SIZE];
	SHA1( part->h3, WII_H3_SIZE, hash );
	if (memcmp(hash,part->tmd->content[0].hash,WII_HASH_SIZE))
	{
	    PrintVerifyMessage(ver,"!H4-ERR");
	    ++differ_count;
	}
    }

    //----- terminate partition ------

abort:

    if ( ver->verbose >= 0 )
    {
	if (!differ_count)
	    PrintVerifyMessage(ver,"+OK");
	else if ( ver->verbose > 0 && differ_count >= max_differ_count )
	    PrintVerifyMessage(ver,"!ABORT");
    }

    return differ_count ? ERR_DIFFER : ERR_OK;
};

///////////////////////////////////////////////////////////////////////////////

enumError VerifyDisc ( Verify_t * ver )
{
    DASSERT(ver);
    DASSERT(ver->sf);
    TRACE("#VERIFY# VerifyDisc(%p) sf=%p utab=%p part=%p\n",
		ver, ver->sf, ver->usage_tab, ver->part );
    TRACE(" - psel=%p, v=%d, lc=%d, maxerr=%d\n",
		ver->psel, ver->verbose, ver->long_count, ver->max_err_msg );

    //----- setup Verify_t data

    wd_disc_t * disc = OpenDiscSF(ver->sf,true,true);
    if (!disc)
	return ERR_WDISC_NOT_FOUND;

    u8 local_usage_tab[WII_MAX_SECTORS];
    if (!ver->usage_tab)
    {
	TRACE(" - use local_usage_tab\n");
	ver->usage_tab = local_usage_tab;
	wd_filter_usage_table(disc,local_usage_tab,ver->psel);
    }
    
    //----- iterate partitions

    enumError err = ERR_OK;
    int pi, differ_count = 0;

    if ( ver->verbose < 0 )
    {
	// find invalid partitions first for faster results, use disc1
	wd_disc_t * disc1 = ver->sf->disc1;
	for ( pi = 0; pi < disc1->n_part && !SIGINT_level; pi++ )
	{
	    ver->part = wd_get_part_by_index(disc1,pi,0);
	    if ( ver->part->is_enabled && !ver->part->is_valid )
	    {
		PrintVerifyMessage(ver,"!INVALID");
		err = ERR_DIFFER;
		break;
	    }
	}
    }

    if (!err)
    {
	for ( pi = 0; pi < disc->n_part && !SIGINT_level; pi++ )
	{
	    ver->part = wd_get_part_by_index(disc,pi,0);
	    if ( ver->part->is_enabled )
	    {
		err = VerifyPartition(ver);
		if ( err == ERR_DIFFER )
		{
		    differ_count++;
		    if ( ver->verbose < 0 )
			break;
		}
		else if (err)
		    break;
	    }
	}
    }

    //----- terminate

    if ( ver->usage_tab == local_usage_tab )
	ver->usage_tab = 0;

    return err ? err : differ_count ? ERR_DIFFER : ERR_OK;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			     E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

