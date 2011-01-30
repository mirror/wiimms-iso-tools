
/***************************************************************************
 *                    __            __ _ ___________                       *
 *                    \ \          / /| |____   ____|                      *
 *                     \ \        / / | |    | |                           *
 *                      \ \  /\  / /  | |    | |                           *
 *                       \ \/  \/ /   | |    | |                           *
 *                        \  /\  /    | |    | |                           *
 *                         \/  \/     |_|    |_|                           *
 *                                                                         *
 *                           Wiimms ISO Tools                              *
 *                         http://wit.wiimm.de/                            *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project.                                 *
 *   Visit http://wit.wiimm.de/ for project details and sources.           *
 *                                                                         *
 *   Copyright (c) 2009-2011 by Dirk Clemens <wiimm@wiimm.de>              *
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
#include <sys/ioctl.h>

#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#if defined(__CYGWIN__)
  #include <cygwin/fs.h>
  #include <io.h>
#elif defined(__APPLE__)
  #include <sys/disk.h>
#elif defined(__linux__)
  #include <linux/fs.h>
#endif

#include "debug.h"
#include "version.h"
#include "types.h"
#include "lib-sf.h"
#include "lib-std.h"
#include "match-pattern.h"
#include "crypt.h"
#include "lib-lzma.h"
#include "titles.h"
#include "iso-interface.h"

#define CMD1_FW 10

#ifdef HAVE_WORK_DIR
  #include "wtest+.c"
#endif

///////////////////////////////////////////////////////////////////////////////

#define NAME "wtest"
#undef TITLE
#define TITLE NAME " v" VERSION " r" REVISION " " SYSTEM " - " AUTHOR " - " DATE

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError WriteBlock ( SuperFile_t * sf, char ch, off_t off, u32 count )
{
    ASSERT(sf);
    if ( count > sizeof(iobuf) )
	count = sizeof(iobuf);

    memset(iobuf,ch,count);
    return WriteWDF(sf,off,iobuf,count);
}

///////////////////////////////////////////////////////////////////////////////

static enumError gen_wdf ( ccp fname )
{
    SuperFile_t sf;
    InitializeSF(&sf);

    enumError stat = CreateFile(&sf.f,fname,IOM_IS_IMAGE,1);
    if (stat)
	return stat;

    stat = SetupWriteWDF(&sf);
    if (stat)
	goto abort;

    WriteBlock(&sf,'a',0x40,0x30);
    WriteBlock(&sf,'b',0x08,0x04);
    WriteBlock(&sf,'c',0x60,0x20);
    WriteBlock(&sf,'d',0x80,0x10);
    WriteBlock(&sf,'d',0x30,0x20);

    return CloseSF(&sf,0);

 abort:
    CloseSF(&sf,0);
    unlink(fname);
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_zero_wdf()			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError test_zero_wdf()
{
    static char fname1[] = "compare.wdf.tmp";
    static char fname2[] = "zero.wdf.tmp";

    gen_wdf(fname1);
    gen_wdf(fname2);

    SuperFile_t sf;
    InitializeSF(&sf);
    enumError err = OpenSF(&sf,fname2,true,true);
    if (err)
	goto abort;

    WriteZeroSF(&sf,0x38,0x40);
    WriteZeroSF(&sf,0x0a,0x28);
    TRACE("########## BEGIN ##########\n");
    WriteZeroSF(&sf,0x8c,0x80);
    TRACE("########## END ##########\n");

    return CloseSF(&sf,0);

 abort:
    CloseSF(&sf,0);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_string_field()		///////////////
///////////////////////////////////////////////////////////////////////////////

static void test_string_field()
{
    StringField_t sf;
    InitializeStringField(&sf);

    InsertStringField(&sf,"b",false);
    InsertStringField(&sf,"a",false);
    InsertStringField(&sf,"c",false);
    InsertStringField(&sf,"b",false);

    int i;
    for ( i = 0; i < sf.used; i++ )
	printf("%4d.: |%s|\n",i,sf.field[i]);

    ResetStringField(&sf);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_create_file()		///////////////
///////////////////////////////////////////////////////////////////////////////

static void test_create_file()
{
    File_t f;
    InitializeFile(&f);

    CreateFile(&f,"pool/hallo.tmp",IOM_NO_STREAM,1);
    SetupSplitFile(&f,OFT_PLAIN,0x80);
    WriteAtF(&f,0x150,"Hallo\n",6);
    printf("*** created -> press ENTER: "); fflush(stdout); getchar();

    CloseFile(&f,1);
    printf("*** closed -> press ENTER: "); fflush(stdout); getchar();
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_create_sparse_file()	///////////////
///////////////////////////////////////////////////////////////////////////////

static void test_create_sparse_file()
{
    File_t f;
    InitializeFile(&f);
    CreateFile(&f,"/cygdrive/d/sparse.tmp",IOM_NO_STREAM,1);
    WriteAtF(&f,0x10000000,"Hallo\n",6);
    CloseFile(&f,0);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_splitted_file()		///////////////
///////////////////////////////////////////////////////////////////////////////

static void test_splitted_file()
{
    File_t of;
    InitializeFile(&of);

    GenDestFileName(&of,"pool/","split-file",".xxx",false);
    CreateFile( &of, 0, IOM_NO_STREAM,true);

    printf("*** created -> press ENTER: "); fflush(stdout); getchar();

    SetupSplitFile(&of,OFT_PLAIN,0x80);

    static char abc[] = "abcdefghijklmnopqrstuvwxyz\n";
    static char ABC[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n";

    int i;
    for ( i = 0; i < 10; i++ )
	WriteF(&of,abc,strlen(abc));

    printf("*** written -> press ENTER: "); fflush(stdout); getchar();

    TRACELINE;
    CloseFile(&of,0);

    printf("*** closed -> press ENTER: "); fflush(stdout); getchar();

    File_t f;
    InitializeFile(&f);
    OpenFileModify(&f,"pool/split-file.xxx",IOM_NO_STREAM);
    SetupSplitFile(&f,OFT_PLAIN,0x100);

    SeekF(&f,0xc0);

    for ( i = 0; i < 20; i++ )
	WriteF(&f,ABC,strlen(ABC));

    char buf[200];
    ReadAtF(&f,0,buf,sizeof(buf));
    printf("%.*s|\n",(int)sizeof(buf),buf);

    CloseFile(&f,false);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_copy_to_wbfs()		///////////////
///////////////////////////////////////////////////////////////////////////////

#include "wbfs-interface.h"

enumError CreateWBFSFile
(
    SuperFile_t * sf,		// valid pointer
    ccp		fname,
    int		overwrite,
    const void	* disc_header,	// NULL or disc header to copy
    const void	* disc_id	// NULL or ID6: check non existence
				// disc_id overwrites the id of disc_header
)
{
    DASSERT(sf);
    enumError err = CreateSF(sf,fname,OFT_WBFS,IOM_IS_WBFS_PART,overwrite);
    if ( !err && sf->wbfs )
    {
	CloseWDisc(sf->wbfs);
	wbfs_disc_t * disc = wbfs_create_disc(sf->wbfs->wbfs,disc_header,disc_id);
	if (disc)
	    sf->wbfs->disc = disc;
	else
	    err = ERR_CANT_CREATE;
    }

    return err;
}

///////////////////////////////////////////////////////////////////////////////

int test_copy_to_wbfs ( int argc, char ** argv )
{
    int i;
    for ( i = 1; i < argc; i++ )
    {
	SuperFile_t fi;
	InitializeSF(&fi);
	if (!OpenSF(&fi,argv[i],false,false))
	{
	    wd_disc_t * disc = OpenDiscSF(&fi,false,true);
	    if (disc)
	    {
		SuperFile_t fo;
		InitializeSF(&fo);
		enumError err = CreateWBFSFile(&fo,"pool/a.wbfs",true,&disc->dhead,0);
		if (!err)
		    err = CopySF(&fi,&fo);
		ResetSF(&fo,0);
	    }
	}
	ResetSF(&fi,0);
    }
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_print_size()		///////////////
///////////////////////////////////////////////////////////////////////////////

int test_print_size ( int argc, char ** argv )
{
 #if 0
    u64 i;
    for ( i = 1; i; i <<= 10 )
    {
	printf("%21llu |%s|%s|\n", i,
		wd_print_size_1024(0,0,i,true),
		wd_print_size_1024(0,0,i,false) );
    }

    u64 prev = 0;
    for ( i = 1; i > prev; i *= 10 )
    {
	prev = i;
	printf("%21llu |%s|%s|\n", i,
		wd_print_size_1024(0,0,i,true),
		wd_print_size_1024(0,0,i,false) );
    }

    i = ~(u64)0;
    printf("%21llu |%s|%s|\n", i,
		wd_print_size_1024(0,0,i,true),
		wd_print_size_1024(0,0,i,false) );
 #else
 
    u64 size = 1000000000;
    wd_size_mode_t mode;
    for ( mode = 0; mode < WD_SIZE_N_MODES; mode++ )
	printf("|%s|\t|%s|\t|%s|\t|%s|\t|%s|\t|%s|\n",
		wd_get_size_unit(mode,"?"),
		wd_print_size(0,0,size,true,mode|WD_SIZE_F_NO_UNIT),
		wd_print_size(0,0,size,true,mode|WD_SIZE_F_1000),
		wd_print_size(0,0,size,true,mode),
		wd_print_size(0,0,size,false,mode|WD_SIZE_F_1000),
		wd_print_size(0,0,size,false,mode) );

 #endif
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_wbfs_free_blocks()		///////////////
///////////////////////////////////////////////////////////////////////////////

static int test_wbfs_free_blocks ( int argc, char ** argv )
{
    int i;
    for ( i = 1; i < argc; i++ )
    {
	WBFS_t w;
	InitializeWBFS(&w);
	if (!OpenWBFS(&w,argv[i],true,0))
	{
	    printf("\n*** %s ***\n",argv[i]);
	 #if NEW_WBFS_INTERFACE
	    int i;
	    for ( i = 1; ; i *= 2 )
	    {
		u32 bl = wbfs_find_free_blocks(w.wbfs,i);
		printf("blocks: %d +%d\n",bl,i);
		if ( bl == WBFS_NO_BLOCK )
		    break;
	    }
	 #endif
	}
	ResetWBFS(&w);
    }
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_open_wdisk()		///////////////
///////////////////////////////////////////////////////////////////////////////

static void dump_wbfs ( WBFS_t * w, enumError err, ccp title )
{
    DASSERT(w);
    DASSERT(title);

    printf("\n----- %s [%s] -----\n",title,GetErrorName(err));
    printf("%p: sf=%p wbfs=%p disc=%p slot=%d\n",
		w, w->sf, w->wbfs, w->disc, w->disc_slot );

    SuperFile_t * sf = w->sf;
    if (sf)
    {
	printf("sf: id=%s, oft=%u=%s, wbfs=%p, fname=%s\n",
		sf->f.id6,
		sf->iod.oft, oft_info[sf->iod.oft].name,
		sf->wbfs, sf->f.fname );

	char buf[16];
	memset(buf,0,sizeof(buf));
	ReadSF(sf,0,buf,sizeof(buf));
	HEXDUMP16(0,0,buf,sizeof(buf));
    }
}

//-------------------------------------------------------------------------------

static int test_open_wdisk ( int argc, char ** argv )
{
    int i;
    for ( i = 1; i < argc; i++ )
    {
	WBFS_t wbfs;
	InitializeWBFS(&wbfs);
	enumError err = OpenWBFS(&wbfs,argv[i],true,0);
	dump_wbfs(&wbfs,err,"Open WBFS");

	err = OpenWDiscSlot(&wbfs,0,false);
	dump_wbfs(&wbfs,err,"Open Disc");

	err = OpenWDiscSF(&wbfs);
	dump_wbfs(&wbfs,err,"Open Disc");

	err = CloseWDisc(&wbfs);
	dump_wbfs(&wbfs,err,"Close Disc");
	
	err = ResetWBFS(&wbfs);
	dump_wbfs(&wbfs,err,"Close WBFS");
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test()				///////////////
///////////////////////////////////////////////////////////////////////////////

int test ( int argc, char ** argv )
{
    //test_zero_wdf();
    //test_string_field();

    //test_create_file();
    //test_create_sparse_file();
    //test_splitted_file();
    //test_copy_to_wbfs(argc,argv);
    //test_print_size(argc,argv);
    //test_wbfs_free_blocks(argc,argv);
    test_open_wdisk(argc,argv);
   
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_filename()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void test_filename ( int argc, char ** argv )
{
    int i;
    char buf[PATH_MAX];

    for ( i = 1; i < argc; i++ )
    {
	NormalizeFileName(buf,buf+sizeof(buf),argv[i],true);
	printf("%s -> %s\n",argv[i],buf);
   }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_match_pattern()		///////////////
///////////////////////////////////////////////////////////////////////////////

static void test_match_pattern ( int argc, char ** argv )
{
    int i, j, i_end = argc, j_start = 1;
    for ( i = 1; i < argc; i++ )
	if (!strcmp(argv[i],"-"))
	{
	    i_end   = i;
	    j_start = i + 1;
	    break;
	}

    for ( i = 1; i < i_end; i++ )
	for ( j = j_start; j < argc; j++ )
	    printf("%u |%s|%s|\n",MatchPattern(argv[i],argv[j],'/'),argv[i],argv[j]);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_open_disc()		///////////////
///////////////////////////////////////////////////////////////////////////////

static void test_open_disc ( int argc, char ** argv )
{
    putchar('\n');
    int i, dump_level = 0;
    bool print_mm = false, print_ptab = false;
    wd_pfst_t pfst = 0;

    for ( i = 1; i < argc; i++ )
    {
	if (!strcmp(argv[i],"-l") )	dump_level++;
	if (!strcmp(argv[i],"-ll"))	dump_level += 2;
	if (!strcmp(argv[i],"-lll"))	dump_level += 3;
	if (!strcmp(argv[i],"-m"))	print_mm = true;
	if (!strcmp(argv[i],"-p"))	print_ptab = true;
	if (!strcmp(argv[i],"-f"))	pfst |= WD_PFST__ALL;
	if (!strcmp(argv[i],"-o"))	pfst |= WD_PFST_OFFSET|WD_PFST_HEADER;
	if (!strcmp(argv[i],"-h"))	pfst |= WD_PFST_SIZE_HEX|WD_PFST_HEADER;
	if (!strcmp(argv[i],"-d"))	pfst |= WD_PFST_SIZE_DEC|WD_PFST_HEADER;
	if (!strcmp(argv[i],"-L"))	logging++;

	if ( *argv[i] == '-' )
	    continue;

	SuperFile_t sf;
	InitializeSF(&sf);
	if (!OpenSF(&sf,argv[i],false,false))
	{
	    printf("*** %s\n",sf.f.fname);
	    enumError err;
	    wd_disc_t * disc = wd_open_disc(WrapperReadDirectSF,&sf,sf.file_size,sf.f.fname,&err);
	    if (disc)
	    {
		putchar('\n');
		wd_print_disc(stdout,3,disc,dump_level);

		if (print_mm)
		{
		    MemMap_t mm;
		    InitializeMemMap(&mm);
		    InsertDiscMemMap(&mm,disc);
		    printf("\nMemory map:\n\n");
		    PrintMemMap(&mm,stdout,3);
		    ResetMemMap(&mm);
		}

		if (print_ptab)
		{
		    printf("\nPartition tables:\n\n");
		    char buf[WII_MAX_PTAB_SIZE];
		    ReadSF(&sf,WII_PTAB_REF_OFF,buf,WII_MAX_PTAB_SIZE);
		    u64 * ptr = (u64*)(buf + WII_MAX_PTAB_SIZE) - 1;
		    while ( ptr > (u64*)buf && !*ptr )
			ptr--;
		    ptr++;
		    HexDump16(stdout,3,WII_PTAB_REF_OFF,buf,(ccp)ptr-buf);

		    putchar('\n');
		    wd_patch_ptab(disc,buf,true);
		    ptr = (u64*)(buf + WII_MAX_PTAB_SIZE) - 1;
		    while ( ptr > (u64*)buf && !*ptr )
			ptr--;
		    ptr++;
		    HexDump16(stdout,3,WII_PTAB_REF_OFF,buf,(ccp)ptr-buf);
		}

		if (pfst)
		{
		    printf("\nFile list:\n\n");
		    wd_print_fst(stdout,3,disc,WD_IPM_AUTO,pfst,0,0);
		}

		wd_close_disc(disc);
		putchar('\n');
	    }
	    else
		printf("\t==> FAILED, err=%x=%d!\n\n",err,err);
	    CloseSF(&sf,0);
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_hexdump()			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError test_hexdump_sf ( SuperFile_t *sf, u64 begin, u64 end )
{
    DASSERT(sf);
    printf("  Dump %llx .. %llx\n",begin,end);
    while ( begin < end )
    {
	const u32 read_count = sizeof(iobuf) < end-begin ? sizeof(iobuf) : end-begin;
	const enumError err = ReadSF(sf,begin,iobuf,read_count);
	if (err)
	    return err;
	ccp ptr = iobuf;
	ccp end_ptr = iobuf + read_count;
	while ( ptr < end_ptr )
	{
	    const u32 count = 16 < end_ptr - ptr ? 16 : end_ptr - ptr;
	    if (memcmp(ptr,zerobuf,count))
		HexDump(stdout,3,begin,9,16,ptr,count);
	    ptr += count;
	    begin += count;
	}
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static void test_hexdump ( int argc, char ** argv )
{
    putchar('\n');
    int i;
    bool dump_part = false;

    for ( i = 1; i < argc; i++ )
    {
	if (!strcmp(argv[i],"-p"))	dump_part = true;
	if (!strcmp(argv[i],"-L"))	logging++;
	if ( *argv[i] == '-' )
	    continue;

	SuperFile_t sf;
	InitializeSF(&sf);
	if (!OpenSF(&sf,argv[i],false,false))
	{
	    printf("*** %s\n",sf.f.fname);
	    test_hexdump_sf(&sf,0x00000,0x50000);
	    if (dump_part)
	    {
		test_hexdump_sf(&sf,0x0050000,0x00502c0);
		test_hexdump_sf(&sf,0xf800000,0xf8002c0);
	    }
	    CloseSF(&sf,0);
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_sha1()			///////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_OPENSSL

#undef SHA1
#include <openssl/sha.h>

///////////////////////////////////////////////////////////////////////////////

void test_sha1()
{
    const int N = 50000;
    const int M = 50000;

    int wit_failed = 0;
    u8 h1[100], h2[100], source[1000-21];
    printf("\n*** test SHA1 ***\n\n");

    int i;

    u32 t1 = GetTimerMSec();
    for ( i=0; i<M; i++ )
	SHA1(source,sizeof(source),h1);
    t1 = GetTimerMSec() - t1;

    u32 t2 = GetTimerMSec();
    for ( i=0; i<M; i++ )
	WIT_SHA1(source,sizeof(source),h2);
    t2 = GetTimerMSec() - t2;

    printf("WIT_SHA1: %8u msec / %u = %6llu nsec\n",t2,M,(u64)t2*1000000/M);
    printf("SHA1:     %8u msec / %u = %6llu nsec\n",t1,M,(u64)t1*1000000/M);

    for ( i = 0; i < N; i++ )
    {
	RandomFill(h1,sizeof(h1));
	memcpy(h2,h1,sizeof(h2));
	RandomFill(source,sizeof(source));

	SHA1(source,sizeof(source),h1);
	WIT_SHA1(source,sizeof(source),h2);

	if (memcmp(h2,h1,sizeof(h2)))
	    wit_failed++;
    }
    printf("WWT failed:%7u/%u\n\n",wit_failed,N);

    HexDump(stdout,0,0,0,24,h2,24);
    HexDump(stdout,0,0,0,24,h1,24);
}

#endif // HAVE_OPENSSL
//
///////////////////////////////////////////////////////////////////////////////
///////////////			 disc info			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_part_info_t
{
    //----- base infos

    int			index;		// zero based index within wd_disc_t
    int			ptab_index;	// zero based index of owning partition table
    int			ptab_part_index;// zero based index within owning partition table
    u32			part_type;	// partition type
#if 0
    char		ticket_id4[5];	// NULL or ID4 of ticket
    char		tmd_id4[5];	// NULL or ID4 of tmd
    char		boot_id6[7];	// NULL or ID6 of boot.bin
    char		boot_title[WII_TITLE_SIZE+1];
					// NULL or title of boot.bin

    //----- partition status

    bool		is_valid;	// true if this partition is valid
    bool		is_encrypted;	// true if this partition is encrypted
    bool		is_overlay;	// true if this partition overlays other partitions
    bool		is_gc;		// true for GC partition => no crypt, no hash

    //----- size statistics

    u32			used_sectors;	// number of used (non scrubbed) sectors
    u32			total_sectors;	// total number of sectors
#endif

} wd_part_info_t;

///////////////////////////////////////////////////////////////////////////////

typedef struct wd_disc_info_t
{
    //----- base infos

    char		id6[7];		// NULL or ID6 of disc header
    char		title[WII_TITLE_SIZE+1];
					// NULL or title of disc header

#if 0
    wd_disc_type_t	disc_type;	// disc type
    wd_disc_attrib_t	disc_attrib;	// disc attrib


    //----- size statstics

    u32			used_sectors;	// number of used (non scrubbed) sectors
    u32			total_sectors;	// total number of sectors
					//	== index of last used sector + 1


    //----- members for external usage

    ccp			source;
    ccp			filename;
    int			wbfs_slot;
    u64			source_size;
#endif

    //----- partition info

    u32			n_part;		// total number of disc partitions
    u32			used_part;	// number of used elements in 'part'
    u32			alloced_part;	// number of alloced elements in 'part'
    wd_part_info_t	part[0];	// info about partitions

} wd_disc_info_t;

///////////////////////////////////////////////////////////////////////////////

wd_part_info_t * wd_get_part_info
(
    wd_part_info_t	* pinfo,	// store info here.
					// if NULL: allocate mem -> call free()
    wd_part_t		* part		// valid partition pointer
)
{
    DASSERT(part);

    //----- prepare data structure

    if (!pinfo)
    {
	pinfo = malloc(sizeof(*pinfo));
	if (!pinfo)
	    OUT_OF_MEMORY;
    }
    memset(pinfo,0,sizeof(*pinfo));


    //-----  fill partiton data

    // [2do]

    return pinfo;
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_part_info_section
(
    FILE		* f,		// valid output file
    wd_part_info_t	* pinfo,	// calid partition info
    int			disc_idx,	// >=0: print index in section header
    int			part_idx	// >=0: print index in section header
)
{
    DASSERT(f);
    DASSERT(pinfo);

    if ( disc_idx >= 0 )
    {
	if ( part_idx >= 0 )
	    printf("[disc-%u:partition-%u]\n", disc_idx, part_idx );
	else
	    printf("[disc-%u:partition]\n", disc_idx );
    }
    else
    {
	if ( part_idx >= 0 )
	    printf("[partition-%u]\n", part_idx );
	else
	    printf("[partition]\n" );
    }


    printf(
	"part-index=%u\n"
	"ptab-index=%u\n"
	"ptab-part-index=%u\n"
	"part-type=%x\n"
	"\n"
	,pinfo->index
	,pinfo->ptab_index
	,pinfo->ptab_part_index
	,pinfo->part_type
	);
}

///////////////////////////////////////////////////////////////////////////////

wd_disc_info_t * wd_get_disc_info
(
    wd_disc_info_t	* dinfo,	// store info here.
					// if NULL: allocate mem -> call free()
    wd_disc_t		* disc,		// valid disc pointer
    int			pmode		// partition mode:
					//   0: no partition info
					//   1: p-info for main partition only
					//   2: p-info for all partitions
)
{
    DASSERT(disc);

    //----- prepare data structure

    int alloced_part, used_part = pmode < 1 ? 0 : pmode < 2 ? 1 : disc->n_part;
    
    if (dinfo)
    {
	alloced_part = dinfo->alloced_part;
	if ( used_part > alloced_part )
	     used_part = alloced_part;
    }
    else
    {
	alloced_part = used_part;
	dinfo = malloc( sizeof(*dinfo) + alloced_part * sizeof(wd_part_info_t) );
	if (!dinfo)
	    OUT_OF_MEMORY;
    }
    memset(dinfo,0,sizeof(*dinfo));
    dinfo->alloced_part = alloced_part;


    //----- fill disc data

    // [2do]


    //-----  fill partiton data

    if ( used_part == 1 && disc->main_part )
    {
	int idx = dinfo->used_part++;
	wd_get_part_info( dinfo->part + idx, disc->main_part );
    }
    else if ( used_part > 0 )
    {
	int idx;
	for ( idx = 0; idx < used_part; idx++ )
	    wd_get_part_info( dinfo->part + idx, disc->part + idx );
	dinfo->used_part = idx;
    }

    return dinfo;
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_disc_info_section
(
    FILE		* f,		// valid output file
    wd_disc_info_t	* dinfo,	// calid partition info
    int			disc_idx,	// >=0: print index in section header
    bool		pmode		// true: print aprtitions sectiosn too
)
{
    DASSERT(f);
    DASSERT(dinfo);

    if ( disc_idx >= 0 )
	printf("[disc-%u]\n", disc_idx );
    else
	printf("[disc]\n" );

    printf(
	"disc-id=%s\n"
	"disc-title=%s\n"
	"db-title=%s\n"
	"\n"
	,wd_print_id(dinfo->id6,6,0)
	,dinfo->title
	,GetTitle(dinfo->id6,"")
	);

    if (pmode)
    {
	int idx;
	for ( idx = 0; idx < dinfo->used_part; idx++ )
	    wd_print_part_info_section(f,dinfo->part+idx,disc_idx,idx);
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			develop()			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError develop ( int argc, char ** argv )
{
    PRINT(" %4zu == sizeof(wd_part_info_t)\n", sizeof(wd_part_info_t) );
    PRINT(" %4zu == sizeof(wd_disc_info_t)\n", sizeof(wd_disc_info_t) );

    SuperFile_t sf;
    InitializeSF(&sf);
    int i;
    for ( i = 1; i < argc; i++ )
    {
	ResetSF(&sf,0);
	if (OpenSF(&sf,argv[i],false,false))
	    continue;

	printf("*** %s\n",sf.f.fname);
	wd_disc_t * disc = OpenDiscSF(&sf,false,true);
	if (!disc)
	{
	    CloseSF(&sf,0);
	    continue;
	}

	wd_disc_info_t * dinfo = wd_get_disc_info(0,disc,2);
	wd_print_disc_info_section(stdout,dinfo,i,true);
	free(dinfo);
	
	CloseSF(&sf,0);
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			command definitions		///////////////
///////////////////////////////////////////////////////////////////////////////

enum
{
    CMD_TEST,			// test();

    CMD_FILENAME,		// test_filename(argc,argv);
    CMD_MATCH_PATTERN,		// test_match_pattern(argc,argv);
    CMD_OPEN_DISC,		// test_open_disc(argc,argv);
    CMD_HEXDUMP,		// test_hexdump(argc,argv);

    CMD_SHA1,			// test_sha1();
    CMD_WIIMM,			// test_wiimm(argc,argv);

    CMD_DEVELOP,		// develop(argc,argv);
    CMD_HELP,			// help_exit();

    CMD__N
};

///////////////////////////////////////////////////////////////////////////////

static const CommandTab_t CommandTab[] =
{
	{ CMD_TEST,		"TEST",		"T",		0 },

	{ CMD_FILENAME,		"FILENAME",	"FN",		0 },
	{ CMD_MATCH_PATTERN,	"MATCH",	0,		0 },
	{ CMD_OPEN_DISC,	"OPENDISC",	"ODISC",	0 },
	{ CMD_HEXDUMP,		"HEXDUMP",	0,		0 },

 #ifdef HAVE_OPENSSL
	{ CMD_SHA1,		"SHA1",		0,		0 },
 #endif
 #ifdef HAVE_WORK_DIR
	{ CMD_WIIMM,		"WIIMM",	"W",		0 },
 #endif

	{ CMD_DEVELOP,		"DEVELOP",	"D",		0 },
	{ CMD_HELP,		"HELP",		"?",		0 },

	{ CMD__N,0,0,0 }
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			help_exit()			///////////////
///////////////////////////////////////////////////////////////////////////////

void help_exit()
{
    printf("\nCommands:\n\n");
    const CommandTab_t * cmd;
    for ( cmd = CommandTab; cmd->name1; cmd++ )
	if (cmd->name2)
	    printf("  %-*s | %s\n",CMD1_FW,cmd->name1,cmd->name2);
	else
	    printf("  %s\n",cmd->name1);
    putchar('\n');
 #ifdef HAVE_WORK_DIR
    wiimm_help_exit(false);
 #endif
    exit(ERR_SYNTAX);
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			main()				///////////////
///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    printf("*****  %s  *****\n",TITLE);
    SetupLib(argc,argv,NAME,PROG_UNKNOWN);

    printf("term width = %d\n",GetTermWidth(80,0));

 #if defined(TEST) && defined(DEBUG)
    if (0)
    {
	id6_t * id6 = (id6_t*)iobuf;
	PRINT("sizeof(id6_t)=%zd, %p,%p,%p -> %zu,%zu,%zu\n",
		sizeof(id6_t),
		id6, id6+1, id6+2,
		(ccp)id6-iobuf, (ccp)(id6+1)-iobuf, (ccp)(id6+2)-iobuf );
    }
 #endif

    if ( argc < 2 )
	help_exit();

    int cmd_stat;
    const CommandTab_t * cmd_ct = ScanCommand(&cmd_stat,argv[1],CommandTab);
    if (!cmd_ct)
    {
	if ( cmd_stat > 0 )
	    ERROR0(ERR_SYNTAX,"Command abbreviation is ambiguous: %s\n\n",argv[1]);
	else
	    ERROR0(ERR_SYNTAX,"Unknown command: %s\n\n",argv[1]);
	help_exit();
    }

    argv[1] = argv[0];
    argv++;
    argc--;

    switch(cmd_ct->id)
    {
	case CMD_TEST:			return test(argc,argv); break;

	case CMD_FILENAME:		test_filename(argc,argv); break;
	case CMD_MATCH_PATTERN:		test_match_pattern(argc,argv); break;
	case CMD_OPEN_DISC:		test_open_disc(argc,argv); break;
	case CMD_HEXDUMP:		test_hexdump(argc,argv); break;

 #ifdef HAVE_OPENSSL
	case CMD_SHA1:			test_sha1(); break;
 #endif
 #ifdef HAVE_WORK_DIR
	case CMD_WIIMM:			test_wiimm(argc,argv); break;
 #endif

	case CMD_DEVELOP:		develop(argc,argv); break;

	//case CMD_HELP:
	default:
	    help_exit();
    }

    CloseAll();

    if (SIGINT_level)
	ERROR0(ERR_INTERRUPT,"Program interrupted by user.");

    return max_error;
}

///////////////////////////////////////////////////////////////////////////////
