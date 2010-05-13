
#define _GNU_SOURCE 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "debug.h"
#include "iso-interface.h"
#include "wbfs-interface.h"
#include "titles.h"
#include "match-pattern.h"
#include "dirent.h"
#include "crypt.h"

#define ALIGN32(d,a) ((d)+((a)-1)&~(u32)((a)-1))

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 dump files			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct dump_info_t
{
	FILE * f;
	int indent;

} dump_info_t;

//-----------------------------------------------------------------------------

static int dump_file ( wiidisc_t * d, iterator_call_mode_t icm,
			u32 offset4, u32 size, const void * data )
{
 #ifdef DEBUG
    if (icm == ICM_OPEN_PARTITION)
    {
	const wd_part_header_t * ph = data;
	ASSERT(ph);
	TRACE("ICM_OPEN_PARTITION: off=%llx, data=%llx+%llx\n",
			(u64)offset4<<2,
			(u64)(offset4+ph->data_off4)<<2,
			(u64)ph->data_size4<<2 );
    }
 #endif

    if ( icm >= ICM_DIRECTORY
	&& MatchFilePattern(file_pattern+PAT_OPT_FILES,d->path) )
    {
	dump_info_t * di = (dump_info_t*)d->user_param;
	ASSERT(di);
	ASSERT(di->f);

	if ( icm == ICM_DIRECTORY )
	{
	    char sizebuf[20] = "-";
	    if (size)
		snprintf(sizebuf,sizeof(sizebuf),"N=%u",size);
	    fprintf(di->f,"%*s          -%9s  %s%s\n",
			di->indent,"", sizebuf, d->path_prefix, d->path );
	}
	else if ( icm == ICM_FILE )
	{
	    const u64 poff = (u64)(offset4-d->partition_offset4) << 2;
	    fprintf(di->f,"%*s%11llx %8x  %s%s\n",
			di->indent,"", poff, size, d->path_prefix, d->path );
	}
	else if ( icm == ICM_COPY )
	{
	    fprintf(di->f,"%*s%11llx#%8x  %s%s\n",
			di->indent,"", (u64)offset4<<2, size, d->path_prefix, d->path );
	}
	else if ( icm == ICM_DATA )
	{
	    fprintf(di->f,"%*s          - %8x  %s%s\n",
			di->indent,"", size, d->path_prefix, d->path );
	}
    }
    return 0;
}

//-----------------------------------------------------------------------------

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

static int dump_header
(
	FILE * f,		// output stream
	int indent,		// indent
	SuperFile_t * sf,	// file to dump
	ccp real_path		// NULL or pointer to real path
)
{
    ASSERT(f);
    ASSERT(sf);
    indent = NormalizeIndent(indent);

    fprintf(f,"\n%*sDump of file %s\n\n",indent,"",sf->f.fname);
    indent += 2;
    if ( real_path && *real_path && strcmp(sf->f.fname,real_path) )
	fprintf(f,"%*sReal path:       %s\n",indent,"",real_path);
    if (*sf->f.id6)
	fprintf(f,"%*sID & type:       %s, %s\n",
		indent,"", sf->f.id6, GetNameFT(sf->f.ftype,0) );
    else
	fprintf(f,"%*stype:            %s\n",
		indent,"", GetNameFT(sf->f.ftype,0) );

    const u64 file_size = sf->file_size;
    if (sf->wc)
    {
	fprintf(f,"%*sVirtual size: %12llx/hex =%11llu =%5llu MiB\n",
		indent,"", file_size, file_size, (file_size+MiB/2)/MiB );

	const u64 size = sf->f.st.st_size;
	fprintf(f,"%*sWDF file size:%12llx/hex =%11llu =%5llu MiB, %lld%%\n",
		indent,"", size, size, (size+MiB/2)/MiB, size * 100 / file_size );
    }
    else
	fprintf(f,"%*sFile size:    %12llx/hex =%11llu =%5llu MiB\n",
		indent,"", file_size, file_size, (file_size+MiB/2)/MiB );

    return indent;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_ISO()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_ISO
(
	FILE * f,		// output stream
	int indent,		// indent
	SuperFile_t * sf,	// file to dump
	ccp real_path,		// NULL or pointer to real path
	int dump_level		// dump level: 0..2
)
{
    char buf1[100];

    ASSERT(sf);
    if ( !f || !sf->f.id6[0] )
	return ERR_OK;
    sf->f.read_behind_eof = 2;

    WDiscInfo_t wdi;
    InitializeWDiscInfo(&wdi);

    MemMap_t mm;
    MemMapItem_t * mi;
    InitializeMemMap(&mm);

    wiidisc_t * disc = 0;
    u32 used_blocks = 0, used_mib = 0;

    indent = dump_header(f,indent,sf,real_path);
    enumError err = ReadSF(sf,0,&wdi.dhead,sizeof(wdi.dhead));
    if (err)
	goto abort;
    CalcWDiscInfo(&wdi,0);

    err = LoadPartitionInfo(sf, &wdi, dump_level>1 ? &mm : 0 );
    if (err)
	goto dump_mm;

    fprintf(f,"%*sDisc name:       %s\n",indent,"",wdi.dhead.game_title);
    if (wdi.title)
	fprintf(f,"%*sDB title:        %s\n",indent,"",wdi.title);
    fprintf(f,"%*sRegion:          %s [%s]\n",indent,"",wdi.region,wdi.region4);
    u8 * p8 = wdi.regionset.region_info;
    fprintf(f,"%*sRegion setting:  %d / %02x %02x %02x %02x  %02x %02x %02x %02x\n",
		indent,"", wdi.regionset.region,
		p8[0], p8[1], p8[2], p8[3], p8[4], p8[5], p8[6], p8[7] );

    //--------------------------------------------------

    disc = wd_open_disc(WrapperReadSF,sf);
    if (disc)
    {
	wd_build_disc_usage(disc,ALL_PARTITIONS,wdisc_usage_tab,sf->file_size);
	fprintf(f,"%*sDiretories:     %7u\n",indent,"",disc->dir_count);
	fprintf(f,"%*sFiles:          %7u\n",indent,"",disc->file_count);
	used_blocks = CountUsedBlocks(wdisc_usage_tab,1);
	used_mib    = ( used_blocks + WII_SECTORS_PER_MIB/2 ) / WII_SECTORS_PER_MIB;
	fprintf(f,"%*sUsed ISO blocks:%7u => %u MiB\n",
		indent,"", used_blocks, used_mib );
    }

    //--------------------------------------------------

    u32 nt = wdi.n_ptab;
    u32 np = wdi.n_part;

    fprintf(f,"\n%*s%d partition table%s with %d partition%s:\n\n"
	    "%*s   tab.idx   n(part)       offset(part.tab) .. end(p.tab)\n"
	    "%*s  --------------------------------------------------------\n",
		indent,"", nt, nt == 1 ? "" : "s",
		np, np == 1 ? "" : "s",
		indent,"", indent,"" );

    int i;
    wd_part_count_t *pc = wdi.pcount;
    for ( i = 0; i < WII_MAX_PART_INFO; i++, pc++ )
	if (pc->n_part)
	{
	    const u64 off = (off_t)pc->off4<<2;
	    fprintf(f,"%*s%7d %8d %11x*4 = %10llx .. %10llx\n",
		indent,"", i, pc->n_part, pc->off4, off,
		off + pc->n_part * sizeof(wd_part_table_entry_t) );
	}

    //--------------------------------------------------

    fprintf(f,"\n%*s%d partition%s:\n\n"
	"%*s   index      type      offset .. end offset   size/hex =   size/dec =  MiB\n"
	"%*s  --------------------------------------------------------------------------\n",
	indent,"", np, np == 1 ? "" : "s", indent,"", indent,"" );

    WDPartInfo_t *pi;
    for ( i = 0, pi = wdi.pinfo; i < np; i++, pi++ )
    {
	ccp part_name = wd_get_partition_name(pi->ptype,0);
	if (part_name)
	    snprintf(buf1,sizeof(buf1),"%7s %d",part_name,pi->ptype);
	else
	{
	    char id4[5];
	    id4[0] = pi->ptype >> 24;
	    id4[1] = pi->ptype >> 16;
	    id4[2] = pi->ptype >>  8;
	    id4[3] = pi->ptype;
	    if ( CheckID(id4) == 4 )
		snprintf(buf1,sizeof(buf1),"   \"%.4s\"",id4);
	    else
		snprintf(buf1,sizeof(buf1),"%9x",pi->ptype);
	}

	if ( pi->size )
	{
	    const u64 size = pi->size;
	    fprintf(f,"%*s%5d.%-2d %s %11llx ..%11llx %10llx =%11llu =%5llu\n",
		indent,"", pi->ptable, pi->index, buf1, (u64)pi->off,
		(u64)pi->off + size, size, size, (size+MiB/2)/MiB );
	}
	else
	    fprintf(f,"%*s%5d.%-2d %s %11llx         ** INVALID PARTITION **\n",
		indent,"", pi->ptable, pi->index, buf1, (u64)pi->off );
    }

    //--------------------------------------------------

    if (dump_level)
      for ( i = 0, pi = wdi.pinfo; i < np; i++, pi++ )
	if ( pi->size )
	{
	    fprintf(f,"\n%*sPartition table #%d, partition #%d, type %x [%s]:\n",
		    indent,"", pi->ptable, pi->index, pi->ptype,
		    wd_get_partition_name(pi->ptype,"?") );

	    if ( pi->is_marked_not_enc > 0 )
		fprintf(f,"%*s  Partition is marked as 'not encrypted'.\n", indent,"" );
	    else if ( pi->is_encrypted >= 0 || pi->is_trucha_signed >= 0 )
	    {
		fprintf(f,"%*s  Partition is", indent,"" );
		ccp bind = "";
		if ( pi->is_encrypted >= 0 )
		{
		    fprintf(f,"%s encrypted", pi->is_encrypted ? "" : " not" );
		    bind = " and";
		}
		    
		if ( pi->is_trucha_signed >= 0 )
		    fprintf(f,"%s%s trucha signed.\n",
				bind, pi->is_trucha_signed ? "" : " not" );
		else
		    fprintf(f,".\n");
	    }

	    p8 = pi->part_key;
	    fprintf(f,"%*s  Partition key: %02x%02x%02x%02x %02x%02x%02x%02x"
				     " %02x%02x%02x%02x %02x%02x%02x%02x\n",
		indent,"",
		p8[0], p8[1], p8[2], p8[3], p8[4], p8[5], p8[6], p8[7],
		p8[8], p8[9], p8[10], p8[11], p8[12], p8[13], p8[14], p8[15] );

	    dump_data(f,indent, pi->off, pi->ph.tmd_off4,  pi->ph.tmd_size,	"TMD:" );
	    dump_data(f,indent, pi->off, pi->ph.cert_off4, pi->ph.cert_size,	"Cert:" );
	    dump_data(f,indent, pi->off, pi->ph.h3_off4,   WII_H3_SIZE,		"H3:" );
	    dump_data(f,indent, pi->off, pi->ph.data_off4, (off_t)pi->ph.data_size4<<2, "Data:" );
	}

    //--------------------------------------------------

    FilePattern_t * pat = GetDefaultFilePattern();
    if ( SetupFilePattern(pat) && disc )
    {
	ccp filter_info = "";
	if ( partition_selector != ALL_PARTITIONS
	    && partition_selector != WHOLE_DISC )
	{
	    const u32 dir_count  = disc->dir_count;
	    const u32 file_count = disc->file_count;
	    wd_build_disc_usage(disc,partition_selector,wdisc_usage_tab,sf->file_size);
	    if ( dir_count != disc->dir_count || file_count != disc->file_count )
		filter_info = pat->match_all
				? " (partitions filtered)"
				: " (partitions & files filtered)";
	}

	if ( !pat->match_all && !*filter_info )
	    filter_info = " (files filtered)";

	fprintf(f,"\n%*s%u director%s with %u file%s, disk usage = %u MiB%s:\n\n"
		"%*s   p-offset     size  name\n"
		"%*s  %.*s\n",
		indent,"", disc->dir_count, disc->dir_count==1 ? "y" : "ies",
		disc->file_count, disc->file_count==1 ? "" : "s",
		used_mib, filter_info,
		indent,"", indent,"", disc->max_path_len + 44, sep_200 );

	iterator_prefix_mode_t pmode = prefix_mode;
	if ( pmode == IPM_DEFAULT )
	    pmode = IPM_PART_NAME;

	dump_info_t di;
	di.f = f;
	di.indent = indent;
	wd_iterate_files(disc,partition_selector,pmode,dump_file,&di,0,0);
    }

    //--------------------------------------------------

 dump_mm:

    if ( dump_level > 1 )
    {
	mi = InsertMemMap(&mm,0,0x100);
	StringCopyS(mi->info,sizeof(mi->info),"Disc header");

	fprintf(f,"\n\n%*sISO Memory Map:\n\n",indent,"");
	PrintMemMap(&mm,f,indent+3);
    }

 abort:

    putc('\n',f);
    if (disc)
	wd_close_disc(disc);
    ResetWDiscInfo(&wdi);
    ResetMemMap(&mm);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_DOL()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_DOL
(
	FILE * f,		// output stream
	int indent,		// indent
	SuperFile_t * sf,	// file to dump
	ccp real_path,		// NULL or pointer to real path
	int dump_level		// dump level: 0..2
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;
    indent = dump_header(f,indent,sf,real_path);

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
///////////////			 Dump_BOOT_BIN()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_BOOT_BIN
(
	FILE * f,		// output stream
	int indent,		// indent
	SuperFile_t * sf,	// file to dump
	ccp real_path,		// NULL or pointer to real path
	int dump_level		// dump level: 0..2
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;
    indent = dump_header(f,indent,sf,real_path);

    wd_boot_t wb;
    enumError err = ReadSF(sf,0,&wb,sizeof(wb));
    if (!err)
    {
	WDiscInfo_t wdi;
	InitializeWDiscInfo(&wdi);
	memcpy(&wdi.dhead,&wb.dhead,sizeof(wdi.dhead));
	CalcWDiscInfo(&wdi,0);

	fprintf(f,"%*sDisc name:       %s\n",indent,"",wdi.dhead.game_title);
	if (wdi.title)
	    fprintf(f,"%*sDB title:        %s\n",indent,"",wdi.title);
	fprintf(f,"%*sRegion:          %s [%s]\n",indent,"",wdi.region,wdi.region4);
	u8 * p8 = wdi.regionset.region_info;
	fprintf(f,"%*sRegion setting:  %d / %02x %02x %02x %02x  %02x %02x %02x %02x\n\n",
		    indent,"", wdi.regionset.region,
		    p8[0], p8[1], p8[2], p8[3], p8[4], p8[5], p8[6], p8[7] );
	ResetWDiscInfo(&wdi);

	ntoh_boot(&wb,&wb);
	u64 val = (u64)wb.dol_off4 << 2;
	fprintf(f,"%*sDOL offset: %10x/hex * 4 =%8llx/hex =%9llu\n",
			indent,"", wb.dol_off4, val, val );
	val = (u64)wb.fst_off4 << 2 ;
	fprintf(f,"%*sFST offset: %10x/hex * 4 =%8llx/hex =%9llu\n",
			indent,"", wb.fst_off4, val, val );
	val = (u64)wb.fst_size4 << 2;
	fprintf(f,"%*sFST size:   %10x/hex * 4 =%8llx/hex =%9llu\n",
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
	FILE * f,		// output stream
	int indent,		// indent
	SuperFile_t * sf,	// file to dump
	ccp real_path,		// NULL or pointer to real path
	int dump_level		// dump level: 0..2
)
{
    ASSERT(sf);
    if ( !f || sf->file_size > 10*MiB )
	return ERR_OK;
    indent = dump_header(f,indent,sf,real_path);

    //----- setup fst

    wd_fst_item_t * ftab_data = malloc(sf->file_size);
    if (!ftab_data)
	OUT_OF_MEMORY;

    enumError err = ReadSF(sf,0,ftab_data,sf->file_size);
    if (!err)
    {
	err = Dump_FST(f,indent,ftab_data,sf->file_size,sf->f.fname);
	if (err)
	    ERROR0(err,"fst.bin is invalid: %s\n",sf->f.fname);
    }

    free(ftab_data);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_FST
(
	FILE * f,				// output stream or NULL if silent
	int indent,				// indent
	const wd_fst_item_t * ftab_data,	// the FST data
	size_t ftab_size,			// size of FST data
	ccp fname				// filename or hint
)
{
    ASSERT(ftab_data);
    TRACE("Dump_FST(%p,%d,%p,%x,%s)\n",f,indent,ftab_data,(u32)ftab_size,fname);

    indent = NormalizeIndent(indent);
    if (!fname)
	fname = "memory";

    //----- setup fst

    const u32 n_files = ntohl(ftab_data->size);
    const wd_fst_item_t * ftab_end = ftab_data + n_files;
    const wd_fst_item_t * dir_end = ftab_end;

    ccp ftab_data_end = (ccp)ftab_data + ftab_size;
    if ( ftab_data_end < (ccp)ftab_end )
    {
	if (f)
	    ERROR0(ERR_INVALID_FILE,"fst.bin is invalid: %s\n",fname);
	return ERR_INVALID_FILE;
    }
    const u32 max_name_delta = ftab_data_end - (ccp)ftab_end;
    
    //----- setup path
    
    const int MAX_DEPTH = 25; // maximal supported directory depth

    char path_buf[1000];
    char *path_end = path_buf + sizeof(path_buf) - MAX_DEPTH;
    char *path = path_buf;


    //----- setup stack

    typedef struct stack_t
    {
	const wd_fst_item_t * ftab;
	const wd_fst_item_t * dir_end;
	char * path;
    } stack_t;
    stack_t stack_buf[MAX_DEPTH];
    stack_t *stack = stack_buf;
    stack_t *stack_max = stack_buf + MAX_DEPTH;

    stack->ftab	   = ftab_data;
    stack->dir_end = ftab_end;
    stack->path	   = path;
    stack++;
    
    //----- print header

    if (f)
    {
	int fw = 80-indent;
	if ( fw < 45 )
	     fw = 45;

	fprintf(f,"\n%*s       is offset/4     size  file_path\n"
		"%*sindex dir base dir  end dir  directory_path/\n"
		"%*s%.*s\n",
		indent,"", indent,"", indent,"", fw, sep_200 );
    }

    //----- main loop

    int err_count = 0;

    const wd_fst_item_t *ftab;
    for ( ftab = ftab_data+1; ftab < ftab_end; ftab++ )
    {
	while ( ftab >= dir_end && stack > stack_buf )
	{
	    stack--;
	    dir_end = stack->dir_end;
	    path = stack->path;
	}

	ccp name_ptr;
	u32 name_delta = ntohl(ftab->name_off) & 0xffffff;
	if ( name_delta < max_name_delta )
	    name_ptr = (ccp)ftab_end + name_delta;
	else
	{
	    err_count++;
	    if (f)
	    {
		*path = 0;
		fprintf(f,"%*s!! WARNING: name index out of range (%x, max=%x): %s/?\n",
			indent, "", name_delta, max_name_delta, path_buf );
	    }
	    name_ptr = "?";
	}
	char * path_dest = StringCopyE(path,path_end,name_ptr);

	if (ftab->is_dir)
	{
	    *path_dest++ = '/';
	    *path_dest = 0;

	    ASSERT(stack<stack_max);
	    if ( stack < stack_max )
	    {
		stack->ftab    = ftab;
		stack->dir_end = dir_end;
		stack->path    = path;
		stack++;
		path = path_dest;
		
		const wd_fst_item_t * new_dir_end  = ftab_data + ntohl(ftab->size);
		if ( new_dir_end > dir_end )
		{
		    err_count++;
		    if (f)
			fprintf(f,"%*s!! WARNING: end of directory behind end of parent directory.\n"
				  "%*s!!          -> %s\n",
				  indent, "", indent, "", path_buf );
		}
		dir_end = new_dir_end;

		// check back tracking
		u32 idx = (u32)ntohl(ftab->offset4);
		if ( stack >= stack_buf+2 && ftab_data + idx != stack[-2].ftab )
		{
		    err_count++;
		    if (f)
			fprintf(f,"%*s!! WARNING: wrong back trace index (%x but not %x): %s\n",
				indent, "", idx, (u32)(stack[-2].ftab - ftab_data), path_buf );
		}
	    }
	    else
	    {
		err_count++;
		if (f)
		    fprintf(f,"%*s!! WARNING: directory level to deep (>%u): %s\n",
				indent, "", MAX_DEPTH, path_buf );
	    }
	}
	if (f)
	    fprintf(f,"%*s%5x. %2x %8x %8x  %s\n",
		indent,"", (int)(ftab-ftab_data), ftab->is_dir,
		(u32)ntohl(ftab->offset4), (u32)ntohl(ftab->size), path_buf );
    }

    return err_count ? ERR_INVALID_FILE : ERR_OK;
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
    StringCopyS(old_id6,sizeof(old_id6),(ccp)&whead->wii_disc_id);
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
	StringCopyS(old_name,sizeof(old_name),(ccp)whead->game_title);

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

partition_selector_t partition_selector = ALL_PARTITIONS;

u8 wdisc_usage_tab [WII_MAX_SECTORS];
u8 wdisc_usage_tab2[WII_MAX_SECTORS];

//-----------------------------------------------------------------------------

partition_selector_t ScanPartitionSelector ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ DATA_PARTITION_TYPE,		"DATA",		"D",		0 },
	 { DATA_PARTITION_TYPE,		"GAME",		"G",		0 },
	{ UPDATE_PARTITION_TYPE,	"UPDATE",	"U",		0 },
	{ CHANNEL_PARTITION_TYPE,	"CHANNEL",	"C",		0 },

	{ WHOLE_DISC,			"WHOLE",	"RAW",		0 },
	{ WHOLE_DISC,			"1:1",		0,		0 },
	{ ALL_PARTITIONS,		"ALL",		"*",		0 },

	{ REMOVE_DATA_PARTITION,	"NO-DATA",	"NODATA",	0 },
	 { REMOVE_DATA_PARTITION,	"NO-GAME",	"NOGAME",	0 },
	{ REMOVE_UPDATE_PARTITION,	"NO-UPDATE",	"NOUPDATE",	0 },
	{ REMOVE_CHANNEL_PARTITION,	"NO-CHANNEL",	"NOCHANNEL",	0 },

	{ 0,0,0,0 }
    };

    const CommandTab_t * cmd = ScanCommand(0,arg,tab);
    if (cmd)
	return cmd->id;

    // try if arg is a number
    char * end;
    ulong num = strtoul(arg,&end,10);
    if ( end != arg && !*end )
	return num;

    ERROR0(ERR_SYNTAX,"Illegal partition selector (option --psel): '%s'\n",arg);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

iterator_prefix_mode_t prefix_mode = IPM_DEFAULT;

//-----------------------------------------------------------------------------

iterator_prefix_mode_t ScanPrefixMode ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ IPM_DEFAULT,		"DEFAULT",	"D",		0 },
	{ IPM_AUTO,		"AUTO",		"A",		0 },

	{ IPM_NONE,		"NONE",		"-",		0 },
	{ IPM_POINT,		"POINT",	".",		0 },
	{ IPM_PART_IDENT,	"IDENT",	"I",		0 },
	{ IPM_PART_NAME,	"NAME",		"N",		0 },

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
    partition_selector = DATA_PARTITION_TYPE;
    prefix_mode = IPM_NONE;

    FilePattern_t * pat = file_pattern + PAT_DEFAULT;
    pat->rules.used = 0;
    AddFilePattern("=sneek",PAT_DEFAULT);
    SetupFilePattern(pat);
}

///////////////////////////////////////////////////////////////////////////////

enumEncoding encoding = ENCODE_DEFAULT;

//-----------------------------------------------------------------------------

enumEncoding ScanEncoding ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ 0,			"AUTO",		0,		ENCODE_MASK },

	{ ENCODE_CLEAR_HASH,	"NO-HASH",	"NOHASH",	ENCODE_CALC_HASH },
	{ ENCODE_CALC_HASH,	"HASHONLY",	0,		ENCODE_CLEAR_HASH },

	{ ENCODE_DECRYPT,	"DECRYPT",	0,		ENCODE_ENCRYPT },
	{ ENCODE_ENCRYPT,	"ENCRYPT",	0,		ENCODE_DECRYPT },

	{ ENCODE_NO_SIGN,	"NO-SIGN",	"NOSIGN",	ENCODE_SIGN },
	{ ENCODE_SIGN,		"SIGN",		"TRUCHA",	ENCODE_NO_SIGN },

	{ 0,0,0,0 }
    };

    const int stat = ScanCommandListMask(arg,tab);
    if ( stat >= 0 )
	return stat & ENCODE_MASK;

    ERROR0(ERR_SYNTAX,"Illegal encoding mode (option --enc): '%s'\n",arg);
    return -1;
}

//-----------------------------------------------------------------------------

static const enumEncoding encoding_m_tab[]
	= { ENCODE_M_HASH, ENCODE_M_CRYPT, ENCODE_M_SIGN, 0 };

enumEncoding SetEncoding
	( enumEncoding val, enumEncoding set_mask, enumEncoding default_mask )
{
    TRACE("SetEncoding(%04x,%04x,%04x)\n",val,set_mask,default_mask);

    const enumEncoding * tab;
    for ( tab = encoding_m_tab; *tab; tab++ )
    {
	// set values
	if ( set_mask & *tab )
	    val = val & ~*tab | set_mask & *tab;

	// if more than 1 bit set: clear it
	if ( Count1Bits32( val & *tab ) > 1 )
	    val &= ~*tab;

	// if no bits are set: use default
	if ( !(val & *tab) )
	    val |= default_mask & *tab;
    }

    TRACE(" -> %04x\n", val & ENCODE_MASK );
    return val & ENCODE_MASK;
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
	uint idx = (beg+end)/2;
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

void DumpIM ( IsoMapping_t * im, FILE * f, int indent )
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
		indent,"", indent,"", fw, sep_200 );

    u64 prev_end = 0;
    IsoMappingItem_t *imi, *last = im->field + im->used - 1;
    for ( imi = im->field; imi <= last; imi++ )
    {
	ccp title;
	switch(imi->imt)
	{
	    case IMT_DATA:
		title = "DATA";
		break;

	    case IMT_FILE:
		title = "FILE";
		break;

	    case IMT_PART_FILES:
		title = "PART FILES...";
		break;

	    case IMT_PART:
		title = "PART";
		break;

	    default:
		ASSERT(0);
		title = "--UNKNOWN--";
	}

	const u64 end = imi->offset + imi->size;
	fprintf(f,"%*s%c %9llx .. %9llx %9llx  %s %s\n", indent,"",
		imi->offset < prev_end ? '!' : ' ',
		imi->offset, end, imi->size, title,
		imi->comment ? imi->comment : "" );
	prev_end = end;
    }
    putc('\n',f);
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
	"/ticket.bin",
	"/tmd.bin",
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int CollectFST ( wiidisc_t * disc, iterator_call_mode_t icm,
			u32 offset4, u32 size, const void * data )
{
    ASSERT(disc);
    IsoFileIterator_t * ifi = disc->user_param;
    ASSERT(ifi);
    WiiFst_t * fst = ifi->fst;
    ASSERT(fst);
    WiiFstPart_t * part = fst->part_active;

 #if defined(DEBUG) || defined(TEST)
    if ( icm == ICM_FILE && disc->usage_table )
    {
	// validate usage_table
	u64 off = (u64)offset4<<2;
	u32 block1 = off/WII_SECTOR_DATA_SIZE;
	u32 block2 = (off+size+WII_SECTOR_DATA_SIZE-1)/WII_SECTOR_DATA_SIZE;
	ASSERT( block2 <= WII_MAX_SECTORS );
	if ( block2 > WII_MAX_SECTORS )
	     block2 = WII_MAX_SECTORS;
	for ( ; block1 < block2; block1++ )
	    ASSERT( disc->usage_table[block1] == disc->usage_marker );
    }
 #endif

    if ( icm >= ICM_DIRECTORY )
    {
	ASSERT(part);
	if (!part)
	    return ERR_OK;

	if ( icm == ICM_DIRECTORY )
	{
	    fst->dirs_served++;
	    if (fst->ignore_dir)
		return 0;
	}
	else
	{
	    fst->files_served++;
	    part->total_file_size += size;
	}

	if ( ifi->pat && !MatchFilePattern(ifi->pat,disc->path) )
	    return 0;

	if ( fst->max_path_len < disc->path_len )
	     fst->max_path_len = disc->path_len;

	WiiFstFile_t * wff = AppendFileFST(part);
	wff->icm	= icm;
	wff->offset4	= offset4;
	wff->size	= size;
	wff->path	= strdup(disc->path);

	if ( icm == ICM_DATA )
	{
	    wff->data = malloc(size);
	    if (!wff->data)
		OUT_OF_MEMORY;
	    memcpy(wff->data,data,size);
	}
    }
    else switch(icm)
    {
	case ICM_OPEN_PARTITION:
	 #ifdef DEBUG
	    {
		const wd_part_header_t * ph = data;
		ASSERT(ph);
		TRACE("ICM_OPEN_PARTITION: off=%llx, data=%llx+%llx\n",
				(u64)offset4<<2,
				(u64)(offset4+ph->data_off4)<<2,
				(u64)ph->data_size4<<2 );
	    }
	 #endif
	    part			= AppendPartFST(fst);
	    fst->part_active		= part;
	    part->part_offset		= (u64)offset4 << 2;
	    part->part_index		= disc->partition_index;
	    part->part_type		= disc->partition_type;
	    part->is_marked_not_enc	= disc->is_marked_not_enc;
	    part->is_encrypted		= disc->is_encrypted;
	    part->is_trucha_signed	= disc->is_trucha_signed;
	    part->path			= strdup(disc->path_prefix);
	    memcpy(part->part_key,disc->partition_key,sizeof(part->part_key));
	    memcpy(&part->part_akey,&disc->partition_akey,sizeof(part->part_akey));
	    break;

	case ICM_CLOSE_PARTITION:
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


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int CollectPartitions ( wiidisc_t * disc, iterator_call_mode_t icm,
			u32 offset4, u32 size, const void * data )
{
    noTRACE("CollectPartitions(icm=%d)\n",icm);

    ASSERT(disc);
    IsoFileIterator_t * ifi = disc->user_param;
    ASSERT(ifi);
    WiiFst_t * fst = ifi->fst;
    ASSERT(fst);
    WiiFstPart_t * part = fst->part_active;

    switch(icm)
    {
	case ICM_OPEN_PARTITION:
	 #ifdef DEBUG
	    {
		const wd_part_header_t * ph = data;
		ASSERT(ph);
		TRACE("ICM_OPEN_PARTITION: off=%llx, data=%llx+%llx\n",
				(u64)offset4<<2,
				(u64)(offset4+ph->data_off4)<<2,
				(u64)ph->data_size4<<2 );
	    }
	 #endif
	    part			= AppendPartFST(fst);
	    fst->part_active		= part;
	    part->part_offset		= (u64)offset4 << 2;
	    part->part_index		= disc->partition_index;
	    part->part_type		= disc->partition_type;
	    part->is_marked_not_enc	= disc->is_marked_not_enc;
	    part->is_encrypted		= disc->is_encrypted;
	    part->is_trucha_signed	= disc->is_trucha_signed;
	    part->path			= strdup(disc->path_prefix);
	    memcpy(part->part_key,disc->partition_key,sizeof(part->part_key));
	    memcpy(&part->part_akey,&disc->partition_akey,sizeof(part->part_akey));

	    part->pc = malloc(sizeof(wd_part_control_t));
	    if (!part->pc)
		OUT_OF_MEMORY;
	    wd_read_raw( disc, offset4, part->pc->part_bin, sizeof(part->pc->part_bin) );
	    setup_part_control(part->pc);
	    break;

	case ICM_CLOSE_PARTITION:
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

 #if defined(DEBUG) || defined(DEBUG_ASSERT) || defined(TEST)

    if ( disc->usage_table )
    {
	if ( icm == ICM_FILE )
	{
	    // validate usage_table
	    u64 off = (u64)offset4<<2;
	    u32 block1 = off/WII_SECTOR_DATA_SIZE;
	    u32 block2 = (off+size+WII_SECTOR_DATA_SIZE-1)/WII_SECTOR_DATA_SIZE;
	    ASSERT( block2 <= WII_MAX_SECTORS );
	    if ( block2 > WII_MAX_SECTORS )
		 block2 = WII_MAX_SECTORS;
	    for ( ; block1 < block2; block1++ )
		ASSERT_MSG( disc->usage_table[block1] == disc->usage_marker,
			"INVALID USAGE TABLE (INTERNAL ERROR) => PLEASE REPORT\n"
			"data=%x+%x, block=%x..%x, off=%llx, marker is %x [%x] %x but not %x\n",
			offset4,
			size,
			block1,
			block2,
			block1 * (u64)WII_SECTOR_SIZE,
			disc->usage_table[block1-1],
			disc->usage_table[block1],
			disc->usage_table[block1+1],
			disc->usage_marker );
	}
	return 0;
    }
 #endif

    // don't iterate files
    return 1;
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

    wfi->done_count =  0;
    wfi->total_count = fst->total_file_count;

    if (wfi->verbose)
    {
	char buf[20];
	wfi->fw_done_count = snprintf(buf,sizeof(buf),"%u",wfi->total_count);

	printf(" - will extract %u file%s of %u partition%s, %llu MiB total\n",
		fst->total_file_count, fst->total_file_count == 1 ? "" : "s",
		fst->part_used, fst->part_used  == 1 ? "" : "s",
		(fst->total_file_size + MiB/2) / MiB );
    }

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


    //----- create all files but ICM_COPY

    *path_dest = 0;

    enumError err = ERR_OK;
    WiiFstFile_t *file, *file_end = part->file + part->file_used;
    for ( file = part->file; err == ERR_OK && file < file_end; file++ )
	if ( file->icm != ICM_COPY )
	    err = CreateFileFST(wfi,path,file);

    //----- Update the signatures of the source FST

    if (wfi->fst)
	UpdateSignatureFST(wfi->fst);


    //----- and now write ICM_COPY files
    //      this is needed because changing hash data while processing other files

    for ( file = part->file; err == ERR_OK && file < file_end; file++ )
	if ( file->icm == ICM_COPY )
	    err = CreateFileFST(wfi,path,file);


    //----- write include.list
    
    StringCat2E(path_dest,path_end,part->path,FST_INCLUDE_FILE);
    WriteStringField(&part->include_list,path,true);

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CreateFileFST ( WiiFstInfo_t *wfi, ccp dest_path, WiiFstFile_t * file )
{
    ASSERT(wfi);
    ASSERT(dest_path);
    ASSERT(file);

    wiidisc_t * disc = wfi->disc;
    ASSERT(disc);

    WiiFstPart_t * part = wfi->part;
    ASSERT(part);

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

    if ( file->icm == ICM_DIRECTORY )
    {
	if ( wfi->verbose > 1 )
	    printf(" - %*u/%u create directory %s\n",
			wfi->fw_done_count, wfi->done_count, wfi->total_count,
			dest);
	return CreatePath(dest);
    }

    if ( file->icm != ICM_FILE && file->icm != ICM_COPY && file->icm != ICM_DATA )
	return 0;


    //----- file handling

    if ( wfi->verbose > 1 )
	printf(" - %*u/%u %s %8u bytes to %s\n",
		wfi->fw_done_count, wfi->done_count, wfi->total_count,
		file->icm == ICM_DATA ? "write  " : "extract", file->size, dest );

    File_t fo;
    InitializeFile(&fo);
    fo.create_directory = true;
    enumError err = CreateFile( &fo, dest, IOM_NO_STREAM, wfi->overwrite );
    if (err)
    {
	ResetFile(&fo,true);
	return err;
    }
    
    if ( file->icm == ICM_FILE )
    {
	u32 size = file->size;
	u32 off4 = file->offset4;

	while ( size > 0 )
	{
	    const u32 read_size = size < sizeof(iobuf) ? size : sizeof(iobuf);
	    if ( wd_read( disc, part->is_encrypted ? &part->part_akey : 0,
				off4, (u8*)iobuf, read_size, 0) )
	    {
		err = ERR_READ_FAILED;
		break;
	    }

	    err = WriteF(&fo,iobuf,read_size);
	    if (err)
		break;

	    size -= read_size;
	    off4 += read_size>>2;
	}
    }
    else if ( file->icm == ICM_COPY )
    {
	u32 size = file->size;
	u32 off4 = file->offset4;

	while ( size > 0 )
	{
	    const u32 read_size = size < sizeof(iobuf) ? size : sizeof(iobuf);
	    if ( wd_read_raw( disc, off4, (u8*)iobuf, read_size ) )
	    {
		err = ERR_READ_FAILED;
		break;
	    }

	    err = WriteF(&fo,iobuf,read_size);
	    if (err)
		break;

	    size -= read_size;
	    off4 += read_size>>2;
	}
    }
    else
    {
	ASSERT( file->icm == ICM_DATA );
	err = WriteF(&fo,file->data,file->size);
    }

    if (wfi->set_time)
    {
	CloseFile(&fo,0);
	SetFileTime(&fo,wfi->set_time);
    }

    ResetFile( &fo, err != ERR_OK );

    //----- progress

    if ( wfi->sf && wfi->sf->show_progress )
	PrintProgressSF(wfi->done_count,wfi->total_count,wfi->sf);

    return err;    
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

static int sort_fst_by_size ( const void * va, const void * vb )
{
    const WiiFstFile_t * a = (const WiiFstFile_t *)va;
    const WiiFstFile_t * b = (const WiiFstFile_t *)vb;

    if ( a->size != b->size )
	return a->size < b->size ? -1 : 1;

    if ( a->offset4 != b->offset4 )
	return a->offset4 < b->offset4 ? -1 : 1;

    return strcmp(a->path,b->path);
}

//-----------------------------------------------------------------------------

static int sort_fst_by_offset ( const void * va, const void * vb )
{
    const WiiFstFile_t * a = (const WiiFstFile_t *)va;
    const WiiFstFile_t * b = (const WiiFstFile_t *)vb;

    if ( a->offset4 != b->offset4 )
	return a->offset4 < b->offset4 ? -1 : 1;

    if ( a->size != b->size )
	return a->size < b->size ? -1 : 1;

    return strcmp(a->path,b->path);
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

enumFileType IsFST ( ccp base_path, char * id6_result )
{
    enumFileType ftype = IsFSTPart(base_path,id6_result);
    if ( ftype != FT_ID_DIR )
	return ftype;

    //----- search for a data partition

    return SearchPartitionsFST(base_path,id6_result,0,0,0) & DATA_PART_FOUND
		? FT_ID_FST|FT_A_ISO
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

    char id6_buf[7];
    if (!id6_result)
	id6_result = id6_buf;
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
	|| read(fd,id6_result,6) != 6 )
    {
	close(fd);
	memset(id6_result,0,7);
	return FT_ID_DIR;
    }
    close(fd);

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
    return FT_ID_FST|FT_A_PART_DIR|FT_A_ISO;
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
		file->icm = ICM_DIRECTORY;
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
		file->icm  = ICM_FILE;
		file->size = st.st_size;
		sd->total_size += ALIGN32(file->size,4);
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
    file->icm  = ICM_DIRECTORY;
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
	boot->fst_size4 = htonl(part->ftab_size>>2);
    }

    IsoMappingItem_t * imi;
    imi = InsertIM(&part->im,IMT_DATA,(u64)cur_offset4<<2,part->ftab_size);
    imi->part = part;
    imi->comment = "fst.bin";
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
    u32 idx = 1;
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

	if ( file->icm == ICM_DIRECTORY )
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
	    ASSERT(file->icm == ICM_FILE);
	    nameptr++;
	    ftab->size = htonl(file->size);
	    ftab->offset4 = htonl(offset4);
	    file->offset4 = offset4;
	    offset4 += file->size + 3 >> 2;
	    memcpy(file_dest,file,sizeof(*file_dest));
	    file_dest++;
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
    if (Dump_FST(0,0,(wd_fst_item_t*)part->ftab,part->ftab_size,"scanned"))
    {
	Dump_FST(TRACE_FILE,0,(wd_fst_item_t*)part->ftab,part->ftab_size,"scanned");
	return ERROR0(ERR_FATAL,"internal fst.bin is invalid -> see trace file\n");
    }
 #endif

    imi = InsertIM( &part->im, IMT_PART_FILES,
			(u64)cur_offset4<<2, (u64)(offset4-cur_offset4)<<2 );
    imi->part = part;
    return offset4;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			generate partition		///////////////
///////////////////////////////////////////////////////////////////////////////

u64 GenPartFST ( SuperFile_t * sf, WiiFstPart_t * part, ccp path, u64 base_off )
{
    ASSERT(sf);
    ASSERT(sf->fst);
    ASSERT(part);
    ASSERT(!part->file_used);

    IsoMappingItem_t * imi;
    char pathbuf[PATH_MAX];

    const u32 good_align =  0x40;
    const u32 max_skip   = 0x100;


    //----- boot.bin + bi2.bin

    ASSERT( WII_BOOT_SIZE == WII_BI2_OFFSET );
    const u32 boot_size = WII_BOOT_SIZE + WII_BI2_SIZE;
    imi = InsertIM(&part->im,IMT_DATA,0,boot_size);
    imi->part = part;
    imi->comment = "boot.bin + bi2.bin";
    imi->data = malloc(boot_size);
    if (!imi->data)
	OUT_OF_MEMORY;
    imi->data_alloced = true;
    u32 cur_offset4 = ALIGN32(boot_size>>2,good_align);

    LoadFile(path,"sys/boot.bin",0,imi->data,WII_BOOT_SIZE,false);
    LoadFile(path,"sys/bi2.bin",0,imi->data+WII_BOOT_SIZE,WII_BI2_SIZE,false);

    wd_boot_t * boot = imi->data;
    if (!sf->fst->disc_header.wii_disc_id)
	memcpy(&sf->fst->disc_header,boot,sizeof(sf->fst->disc_header));


    //----- disc/header.bin + disc/region.bin

    LoadFile(path,"disc/header.bin",0,
			&sf->fst->disc_header,sizeof(sf->fst->disc_header),true);
    LoadFile(path,"disc/region.bin",0,
			&sf->fst->region_set,sizeof(sf->fst->region_set),true);


    //----- apploader.img

    ccp fpath = PathCat2S(pathbuf,sizeof(pathbuf),path,"sys/apploader.img");
    const u32 app_fsize4 = GetFileSize(fpath,0) + 3 >> 2;
    imi = InsertIM(&part->im,IMT_FILE,WII_APL_OFFSET,(u64)app_fsize4<<2);
    imi->part = part;
    imi->comment = "apploader.img";
    imi->data = strdup(fpath);
    imi->data_alloced = true;

    cur_offset4 = ( WII_APL_OFFSET >> 2 ) + app_fsize4;
    

    //----- main.dol

    // try to use the same offset for main.dol
    u32 good_off4 = ntohl(boot->dol_off4);
    if ( good_off4 >= cur_offset4 && good_off4 <= cur_offset4 + max_skip )
	 cur_offset4 = good_off4;
    boot->dol_off4 = htonl(cur_offset4);
    
    fpath = PathCat2S(pathbuf,sizeof(pathbuf),path,"sys/main.dol");
    const u32 dol_fsize4 = GetFileSize(fpath,0) + 3 >> 2;
    imi = InsertIM(&part->im,IMT_FILE,(u64)cur_offset4<<2,(u64)dol_fsize4<<2);
    imi->part = part;
    imi->comment = "main.dol";
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

    const u32 ticket_size = GetFileSize(path,"ticket.bin");
    if ( ticket_size < WII_TICKET_SIZE )
	return ERROR0(ERR_INVALID_FILE,"Content of file 'ticket.bin' wrong!\n");

    wd_part_control_t *pc = malloc(sizeof(wd_part_control_t));
    if (!pc)
	OUT_OF_MEMORY;
    part->pc = pc;
 
    const u32 tmd_size    = GetFileSize(path,"tmd.bin");
    const u32 cert_size   = GetFileSize(path,"cert.bin");
    if (clear_part_control(pc,tmd_size,cert_size,(u64)blocks*WII_SECTOR_SIZE))
	return ERROR0(ERR_INVALID_FILE,"Content of file 'tmd.bin' or 'cert.bin' wrong!\n");


    //----- setup partition control content

    // load files
    LoadFile(path,"ticket.bin",	0, &pc->head->ticket, sizeof(pc->head->ticket), false );
    LoadFile(path,"tmd.bin",	0, pc->tmd, pc->tmd_size, false );
    LoadFile(path,"cert.bin",	0, pc->cert, pc->cert_size, false );

    // setup
    InsertMemMap(&sf->modified_list,base_off,sizeof(pc->part_bin));
    
    if ( sf->fst->encoding & ENCODE_CALC_HASH )
    {
	tmd_clear_encryption(pc->tmd,0);

	// calc partition keys
	wd_decrypt_title_key(&pc->head->ticket,part->part_key);
	wd_aes_set_key(&part->part_akey,part->part_key);

	// fill h3 with data to avoid sparse writing
	int groups = (blocks-1)/WII_GROUP_SECTORS+1;
	if ( groups > WII_N_ELEMENTS_H3 )
	     groups = WII_N_ELEMENTS_H3;
	memset(pc->h3,1,groups*WII_HASH_SIZE);
    }
    else
    {
	ASSERT( !( sf->fst->encoding & (ENCODE_ENCRYPT|ENCODE_SIGN) ));
	tmd_clear_encryption(pc->tmd,1);
    }


    //----- insert this partition in mem map of fst

    imi = InsertIM(&sf->fst->im,IMT_DATA,base_off,sizeof(pc->part_bin));
    imi->comment = "partition header";
    imi->data = pc->part_bin;

    imi = InsertIM(&sf->fst->im,IMT_PART, base_off+pc->data_off, pc->data_size );
    imi->part = part;
    imi->comment = "partition data";


    //----- terminate

    if ( logging > 1 )
    {
	printf("Memory layout of %s partition:\n",
		wd_get_partition_name(part->part_type,"?"));
	DumpIM(&part->im,stdout,3);
    }
 #ifdef DEBUG
    DumpIM(&part->im,TRACE_FILE,3);
 #endif

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

    sf->oft = OFT_FST;
    WiiFst_t * fst = malloc(sizeof(*fst));
    if (!fst)
	OUT_OF_MEMORY;
    sf->fst = fst;
    InitializeFST(fst);

    //----- setup fst->encoding mode


    enumEncoding enc = SetEncoding(encoding,0,0);

    // make some dependency checks
    if ( enc & ENCODE_SIGN )
	enc = SetEncoding(enc,ENCODE_ENCRYPT|ENCODE_CALC_HASH,0);
    if ( enc & ENCODE_M_CRYPT )
	enc = SetEncoding(enc,ENCODE_CALC_HASH,ENCODE_NO_SIGN);
    if ( enc & ENCODE_M_HASH )
	enc = SetEncoding(enc,0,ENCODE_NO_SIGN|ENCODE_DECRYPT);

    fst->encoding = SetEncoding(enc,0,
		enc & ENCODE_F_FAST
			? ENCODE_NO_SIGN|ENCODE_DECRYPT|ENCODE_CLEAR_HASH
			: ENCODE_SIGN|ENCODE_ENCRYPT|ENCODE_CALC_HASH );
    TRACE("ENCODING: %04x -> %04x\n",encoding,fst->encoding);


    //----- setup mapping

    IsoMappingItem_t * imi;
    imi = InsertIM(&fst->im,IMT_DATA,0,sizeof(fst->disc_header));
    imi->comment = "disc header";
    imi->data = &fst->disc_header;

    imi = InsertIM(&fst->im,IMT_DATA,WII_REGION_OFF,sizeof(fst->region_set));
    imi->comment = "region settings";
    imi->data = &fst->region_set;


    //----- setup partitions
    
    u64 update_off	= WII_GOOD_UPDATE_PART_OFF;
    u64 update_size	= 0;
    u64 data_off	= WII_GOOD_DATA_PART_OFF;
    u64 data_size	= 0;
    u64 channel_off	= WII_GOOD_DATA_PART_OFF;
    u64 channel_size	= 0;
    WiiFstPart_t * data_part = 0;

    if ( sf->f.ftype & FT_A_PART_DIR )
    {
	data_part = AppendPartFST(fst);
	data_part->part_type = DATA_PARTITION_TYPE;
	data_size = GenPartFST(sf,data_part,sf->f.fname,data_off);
	data_part->path = strdup(sf->f.fname);
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
	    ccp path = PathCat2S(path_buf,sizeof(path_buf),sf->f.fname,update_path);
	    WiiFstPart_t * update_part = AppendPartFST(fst);
	    update_part->part_type = UPDATE_PARTITION_TYPE;
	    update_size = GenPartFST(sf,update_part,path,update_off);
	    const u32 update_end = update_off + WII_PARTITION_BIN_SIZE + update_size;
	    if ( data_off < update_end )
		 data_off = update_end;
	    update_part->path = strdup(path);
	}

	ccp path = PathCat2S(path_buf,sizeof(path_buf),sf->f.fname,data_path);
	data_part = AppendPartFST(fst);
	data_part->part_type = DATA_PARTITION_TYPE;
	data_size = GenPartFST(sf,data_part,path,data_off);
	data_part->path = strdup(path);

	if ( stat & CHANNEL_PART_FOUND )
	{
	    const u32 data_end = data_off + WII_PARTITION_BIN_SIZE + data_size;
	    if ( channel_off < data_end )
		 channel_off = data_end;

	    ccp path = PathCat2S(path_buf,sizeof(path_buf),sf->f.fname,channel_path);
	    WiiFstPart_t * channel_part = AppendPartFST(fst);
	    channel_part->part_type = CHANNEL_PARTITION_TYPE;
	    channel_size = GenPartFST(sf,channel_part,path,channel_off);
	    channel_part->path = strdup(path);
	}
    }
    ASSERT(data_part);


    //----- setup partition header

    const u32 n_part	    = 1 + (update_size>0) + (channel_size>0);
    const u64 cnt_tab_size  = WII_MAX_PART_INFO * sizeof(wd_part_count_t);
    const u64 part_tab_size = cnt_tab_size
			    + n_part * sizeof(wd_part_table_entry_t);
    imi = InsertIM(&fst->im,IMT_DATA,WII_PART_INFO_OFF,part_tab_size);
    imi->comment = "partition tables";
    imi->data = calloc(1,part_tab_size);
    if (!imi->data)
	OUT_OF_MEMORY;
    imi->data_alloced = true;

    wd_part_count_t * pcount = imi->data;
    pcount->n_part = htonl(n_part);
    pcount->off4   = htonl( WII_PART_INFO_OFF + cnt_tab_size >> 2 );

    wd_part_table_entry_t * pentry = (void*)((ccp)imi->data+cnt_tab_size);
    if (update_size)
    {
	pentry->off4  = htonl( update_off >> 2 );
	pentry->ptype = htonl( UPDATE_PARTITION_TYPE );
	pentry++;
    }
    pentry->off4  = htonl( data_off >> 2 );
    pentry->ptype = htonl( DATA_PARTITION_TYPE );
    if (channel_size)
    {
	pentry++;
	pentry->off4  = htonl( channel_off >> 2 );
	pentry->ptype = htonl( CHANNEL_PARTITION_TYPE );
    }


    //----- setup work+cache buffer of WII_GROUP_SIZE
    //		== WII_N_ELEMENTS_H1 * WII_N_ELEMENTS_H2 * WII_SECTOR_SIZE

    ASSERT(!fst->cache);
    ASSERT(!fst->cache_part);
    fst->cache = malloc(WII_GROUP_SIZE);
    if (!fst->cache)
	OUT_OF_MEMORY;
    TRACE("CACHE= %x/hex = %u bytes\n",WII_GROUP_SIZE,WII_GROUP_SIZE);


    //----- terminate

    if ( logging > 0 )
    {
	printf("Memory layout of virtual disc:\n");
	DumpIM(&fst->im,stdout,3);
    }
 #ifdef DEBUG
    DumpIM(&fst->im,TRACE_FILE,3);
 #endif

    const off_t single_size = WII_SECTORS_SINGLE_LAYER * (off_t)WII_SECTOR_SIZE;
    sf->file_size = data_off + data_size;
    if ( sf->file_size < single_size )
	sf->file_size = single_size;

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
		part_control_sign_trucha(pc,0);

	 #ifdef DEBUG
	    {
		u8 hash[WII_HASH_SIZE];
		SHA1( ((u8*)pc->tmd)+WII_TMD_SIG_OFF, pc->tmd_size-WII_TMD_SIG_OFF, hash );
		TRACE("TRUCHA: ");
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
	    TRACE("READ done! [%u]\n",__LINE__);
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
	switch(imi->imt)
	{
	    case IMT_DATA:
	    case IMT_PART:
		{
		    const off_t  delta = off - imi->offset;
		    const off_t  max_size = imi->size - delta;
		    const size_t copy_count = count < max_size ? count : max_size;
		    if ( imi->imt == IMT_DATA )
		    {
			TRACE(">COPY %zx=%zu\n",copy_count,copy_count);
			memcpy(dest,(ccp)imi->data+delta,copy_count);
		    }
		    else
		    {
			ASSERT(imi->part);
			const enumError err
			    = ReadPartFST(sf,imi->part,delta,dest,copy_count);
			if (err)
			    return err;
		    }
		    off   += copy_count;
		    dest  += copy_count;
		    count -= copy_count;
		}
		break;

	    default:
		TRACELINE;
		return ERROR0(ERR_INTERNAL,0);
	}
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

    const u32 delta = n_groups * WII_SECTOR_DATA_OFF  * WII_GROUP_SECTORS;
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
	    case IMT_DATA:
		noTRACE("IMT_DATA: %x %x -> %zx (%s)\n",
			skip_count, max_copy, dest-src, imi->comment );
		ASSERT(imi->data);
		memcpy(dest,imi->data+skip_count,max_copy);
		//HEXDUMP16(3,0,dest,max_copy<0x40?max_copy:0x40);
		break;

	    case IMT_FILE:
		noTRACE("IMT_FILE: %x %x -> %zx (%s)\n",
			skip_count, max_copy, dest-src, imi->comment );
		ASSERT(imi->data);
		LoadFile(imi->data,0,skip_count,dest,max_copy,false);
		break;

	    case IMT_PART_FILES:
		noTRACE("IMT_PART_FILES: %x %x -> %zx (%s)\n",
			skip_count, max_copy, dest-src, imi->comment );
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
			    LoadFile(part->path,file->path,skip,ldest,load_size,false);
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
	memset(dest,0,WII_SECTOR_DATA_OFF);
	dest += WII_SECTOR_DATA_OFF;

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
		sf->fst->encoding & ENCODE_ENCRYPT ? &part->part_akey : 0,
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
	EncryptSectors(akey,sect0,sect0,n_sectors);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			En/DecryptSectors()		///////////////
///////////////////////////////////////////////////////////////////////////////

static const u8 iv0[WII_KEY_SIZE] = {0}; // always NULL

///////////////////////////////////////////////////////////////////////////////

void EncryptSectors
(
    const aes_key_t	* akey,
    const void		* sect_src,
    void		* sect_dest,
    size_t		n_sectors
)
{
    ASSERT(akey);
    const u8 * src = sect_src;
    u8 * dest = sect_dest;

    while ( n_sectors-- > 0 )
    {
	// encrypt header
	wd_aes_encrypt(	akey,
			iv0,
			src,
			dest,
			WII_SECTOR_DATA_OFF );

	// encrypt data
	wd_aes_encrypt(	akey,
			dest + WII_SECTOR_IV_OFF,
			src  + WII_SECTOR_DATA_OFF,
			dest + WII_SECTOR_DATA_OFF,
			WII_SECTOR_DATA_SIZE );

	src  += WII_SECTOR_SIZE;
	dest += WII_SECTOR_SIZE;
    }
}

///////////////////////////////////////////////////////////////////////////////

void DecryptSectors
(
    const aes_key_t	* akey,
    const void		* sect_src,
    void		* sect_dest,
    size_t		n_sectors
)
{
    ASSERT(akey);
    const u8 * src = sect_src;
    u8 * dest = sect_dest;
    u8 tempbuf[WII_SECTOR_SIZE];

    while ( n_sectors-- > 0 )
    {
	const u8 * src1 = src;
	if ( src == dest )
	{
	    // inplace decryption not possible => use tempbuf
	    memcpy(tempbuf,src,WII_SECTOR_SIZE);
	    src1 = tempbuf;
	}

	// decrypt data
	wd_aes_decrypt(	akey,
			src1 + WII_SECTOR_IV_OFF,
			src1 + WII_SECTOR_DATA_OFF,
			dest + WII_SECTOR_DATA_OFF,
			WII_SECTOR_DATA_SIZE );

	// decrypt header
	wd_aes_decrypt(	akey,
			iv0,
			src1,
			dest,
			WII_SECTOR_DATA_OFF);

	src  += WII_SECTOR_SIZE;
	dest += WII_SECTOR_SIZE;
    }
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
    ver->selector	= partition_selector;
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
	DASSERT(ver->disc);
	DASSERT(ver->part);
	DASSERT(msg);

	WiiFstPart_t * part = ver->part;

	char pname_buf[20];
	wd_print_partition_name(pname_buf,sizeof(pname_buf),part->part_type,0);

	char count_buf[100];
	if (!ver->disc_index)
	{
	    // no index supported
	    snprintf( count_buf, sizeof(count_buf),".%u%c",
			part->part_index,
			part->is_encrypted ? ' ' : 'd' );
	}
	else if ( ver->disc_total < 2 )
	{
	    // a single check
	    snprintf( count_buf, sizeof(count_buf),"%u.%u%c",
			ver->disc_index,
			part->part_index,
			part->is_encrypted ? ' ' : 'd' );
	}
	else
	{
	    // calculate fw
	    const int fw = snprintf(count_buf,sizeof(count_buf),"%u",ver->disc_total);
	    snprintf( count_buf, sizeof(count_buf),"%*u.%u/%u%c",
			fw,
			ver->disc_index,
			part->part_index,
			ver->disc_total,
			part->is_encrypted ? ' ' : 'd' );
	}

	printf("%*s%-7s %s %n%-7s %6.6s %s\n",
		ver->indent, "",
		msg, count_buf, &ver->info_indent,
		pname_buf, ver->disc->id6,
		ver->fname ? ver->fname : ver->sf->f.fname );
	fflush(stdout);
    }
    return ERR_DIFFER;
}

///////////////////////////////////////////////////////////////////////////////

static enumError VerifyHash
	( Verify_t * ver, ccp msg, u8 * data, size_t data_len, u8 * ref )
{
    DASSERT(ver);
    DASSERT(ver->part);
    DASSERT(ver->part->pc);

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
	const u64 offset	= ver->part->part_offset
				+ ver->part->pc->data_off
				+ ver->group * (u64)WII_GROUP_SIZE
				+ delta_off;
	const int ref_delta	= ref - data; 

	if ( (ccp)ref >= iobuf && (ccp)ref <= iobuf + sizeof(iobuf) )
	{
	    const u32 block	= offset / WII_SECTOR_SIZE;
	    const u32 block_off	= offset - block * (u64)WII_SECTOR_SIZE;
	    const u32 sub_block	= block_off / WII_SECTOR_DATA_OFF;

	    printf("%*sgroup=%x.%x, block=%x.%x, data-off=%llx=B+%x, hash-off=%llx=B+%x\n",
			ver->info_indent, "",
			ver->group, delta_blk, block, sub_block,
			offset, block_off, offset + ref_delta, block_off + ref_delta );
	    flush = true;
	}

	if ( ver->long_count > 1 )
	{
	    printf("%*sDATA",ver->info_indent,"");
	    HexDump(stdout,0,offset,10,WII_HASH_SIZE,hash,WII_HASH_SIZE);

	    printf("%*sHASH",ver->info_indent,"");
	    HexDump(stdout,0,offset+ref_delta,10,WII_HASH_SIZE,ref,WII_HASH_SIZE);

	    printf("%*sVERIFY",ver->info_indent,"");
	    HexDump(stdout,0,0,8,WII_HASH_SIZE,hash,WII_HASH_SIZE);

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
    DASSERT(ver->disc);
    DASSERT(ver->part);

    TRACE("#VERIFY# VerifyPartition(%p) sf=%p disc=%p utab=%p fst=%p part=%p\n",
		ver, ver->sf, ver->disc, ver->usage_tab, ver->fst, ver->part );
    TRACE(" - psel=%x, v=%d, lc=%d, maxerr=%d\n",
		ver->verbose, ver->verbose, ver->long_count, ver->max_err_msg );

 #if WATCH_BLOCK
    printf("WB: WATCH BLOCK %x = %u\n",WATCH_BLOCK,WATCH_BLOCK);
 #endif

    DASSERT( sizeof(iobuf) >= 2*WII_GROUP_SIZE );
    if ( sizeof(iobuf) < WII_GROUP_SIZE )
	return ERROR0(ERR_INTERNAL,0);

    ver->indent = NormalizeIndent(ver->indent);
    if ( ver->verbose > 1 )
	PrintVerifyMessage(ver,">scan");


    //----- check partition header

    WiiFstPart_t * part = ver->part;

    if (part->is_marked_not_enc)
    {
	PrintVerifyMessage(ver,"!NO-HASH");
	return ERR_DIFFER;
    }

    const wd_part_control_t * pc = part->pc;
    if ( !pc || !pc->is_valid )
    {
	PrintVerifyMessage(ver,"!INVALID");
	return ERR_DIFFER;
    }


    //----- calculate end of blocks

    u32 block = ( part->part_offset + pc->data_off ) / WII_SECTOR_SIZE;
    u32 block_end = block + pc->data_size / WII_SECTOR_SIZE;
    if ( block_end > WII_MAX_SECTORS )
	 block_end = WII_MAX_SECTORS;

    TRACE(" - Partition %u [%s], valid=%d, blocks=%x..%x\n",
		part->part_type,
		wd_get_partition_name(part->part_type,"?"),
		pc->is_valid,
		block, block_end );
    TRACE_HEXDUMP16(0,block,ver->usage_tab,block_end-block);

    const int usage_tab_marker = 2 + part->part_index;
    const int max_differ_count = ver->verbose <= 0
				? 1
				: ver->max_err_msg > 0
					? ver->max_err_msg
					: INT_MAX;
    int differ_count = 0;

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
		    DecryptSectors(&part->part_akey,sect+WII_GROUP_SECTORS,sect,1);


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

		    u8 * h3 = pc->h3 + ver->group * WII_HASH_SIZE;
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

    if (pc->tmd_content)
    {
	u8 hash[WII_HASH_SIZE];
	SHA1( pc->h3, pc->h3_size, hash );
	if (memcmp(hash,pc->tmd_content->hash,WII_HASH_SIZE))
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
    TRACE("#VERIFY# VerifyDisc(%p) sf=%p disc=%p utab=%p fst=%p part=%p\n",
		ver, ver->sf, ver->disc, ver->usage_tab, ver->fst, ver->part );
    TRACE(" - psel=%x, v=%d, lc=%d, maxerr=%d\n",
		ver->verbose, ver->verbose, ver->long_count, ver->max_err_msg );

    //----- setup Verify_t data

    wiidisc_t * disc = 0;
    if (!ver->disc)
    {
	TRACE(" - open disc\n");
	disc = wd_open_disc(WrapperReadSF,ver->sf);
	if (!disc)
	    return ERROR0(ERR_CANT_OPEN,"Can't open dics: %s\n",ver->sf->f.fname);
	ver->disc = disc;
    }

    u8 local_usage_tab[WII_MAX_SECTORS];
    if (!ver->usage_tab)
    {
	TRACE(" - use local_usage_tab\n");
	ver->usage_tab = local_usage_tab;
    }
    
    WiiFst_t fst;
    if (!ver->fst)
    {
	TRACE(" - use fst\n");
	InitializeFST(&fst);

	IsoFileIterator_t ifi;
	memset(&ifi,0,sizeof(ifi));
	ifi.sf  = ver->sf;
	ifi.fst = &fst;

	wd_iterate_files
	(
		ver->disc,
		ver->selector,
		IPM_DEFAULT,
		CollectPartitions,
		&ifi,
		ver->usage_tab,
		ver->sf->file_size
	);
	
	ver->fst = &fst;
    }
    else if ( ver->usage_tab == local_usage_tab )
    {
	TRACE(" - fill usage table\n");
	wd_build_disc_usage
	(
		ver->disc,
		ver->selector,
		ver->usage_tab,
		ver->sf->file_size
	);
    }


    //----- iterate partitions

    enumError err = ERR_OK;
    int pi, differ_count = 0;
    for ( pi = 0; pi < ver->fst->part_used && !SIGINT_level; pi++ )
    {
	ver->part = ver->fst->part + pi;
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


    //----- reset Verify_t data

    if ( ver->fst == &fst )
    {
	ver->fst = 0;
	ResetFST(&fst);
    }

    if ( ver->usage_tab == local_usage_tab )
	ver->usage_tab = 0;

    if (disc)
    {
	ver->disc = 0;
	wd_close_disc(disc);
    }

    return err ? err : differ_count ? ERR_DIFFER : ERR_OK;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			     E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

