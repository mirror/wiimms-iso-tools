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
#include <iso-interface.h>

#include "debug.h"
#include "version.h"
#include "types.h"
#include "lib-sf.h"
#include "lib-std.h"
#include "match-pattern.h"
#include "crypt.h"


#ifdef HAVE_WORK_DIR
  #include "wtest+.c"
#endif

///////////////////////////////////////////////////////////////////////////////

#define NAME "wtest"
#undef TITLE
#define TITLE NAME " v" VERSION " r" REVISION " " SYSTEM " - " AUTHOR " - " DATE

//
///////////////////////////////////////////////////////////////////////////////

static enumError WriteBlock ( SuperFile_t * sf, char ch, off_t off, u32 count )
{
    ASSERT(sf);
    if ( count > sizeof(iobuf) )
	count = sizeof(iobuf);

    memset(iobuf,ch,count);
    return WriteWDF(sf,off,iobuf,count);
}

//
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
///////////////////////////////////////////////////////////////////////////////

static void read_behind_file ( ccp fname )
{
    File_t f;
    InitializeFile(&f);

    OpenFile(&f,fname,IOM_IS_IMAGE);
    ReadAtF(&f,0,iobuf,sizeof(iobuf));
    ReadAtF(&f,0x10000,iobuf,sizeof(iobuf));
    ClearFile(&f,false);

    OpenFile(&f,fname,IOM_IS_IMAGE);
    f.read_behind_eof = 1;
    ReadAtF(&f,0,iobuf,sizeof(iobuf));
    ReadAtF(&f,0x10000,iobuf,sizeof(iobuf));
    ClearFile(&f,false);
}

///////////////////////////////////////////////////////////////////////////////

static void read_behind_wdf ( ccp fname )
{
    SuperFile_t sf;
    InitializeSF(&sf);

    OpenFile(&sf.f,fname,IOM_IS_IMAGE);
    SetupReadWDF(&sf);
    ReadWDF(&sf,0,iobuf,sizeof(iobuf));
    ReadWDF(&sf,0x10000,iobuf,sizeof(iobuf));
    CloseSF(&sf,0);

    OpenFile(&sf.f,fname,IOM_IS_IMAGE);
    SetupReadWDF(&sf);
    sf.f.read_behind_eof = 1;
    ReadWDF(&sf,0,iobuf,sizeof(iobuf));
    ReadWDF(&sf,0x10000,iobuf,sizeof(iobuf));
    CloseSF(&sf,0);
}

//
///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////

static void test_splitted_file()
{
    File_t of;
    InitializeFile(&of);

    GenDestFileName(&of,"pool/","split-file",".xxx",0);
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
	    //if ( i != j )
		printf("%u |%s|%s|\n",MatchPattern(argv[i],argv[j]),argv[i],argv[j]);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void open_partition ( SuperFile_t * sf, int index, partition_selector_t sel )
{
    ASSERT(sf);
    TRACELINE;
    wiidisc_t * disc = wd_open_partition(WrapperReadSF,sf,index,sel);
    if (disc)
    {
	TRACELINE;
	printf("%2d. %2x -> %2d. %2x  %s\n",
		index, sel, disc->partition_index, disc->partition_type, sf->f.fname );
	wd_close_disc(disc);
    }
    else
	printf("%2d. %2x ->  -   -  %s\n",
		index, sel, sf->f.fname );
    TRACELINE;
}

///////////////////////////////////////////////////////////////////////////////

static void test_open_partition ( int argc, char ** argv )
{
    int i;
    for ( i = 1; i < argc; i++ )
    {
	SuperFile_t sf;
	InitializeSF(&sf);
	if (!OpenSF(&sf,argv[i],false,false))
	{
	    open_partition(&sf, 0,DATA_PARTITION_TYPE);
	    open_partition(&sf, 1,DATA_PARTITION_TYPE);
	    open_partition(&sf, 2,DATA_PARTITION_TYPE);
	    open_partition(&sf,-1,DATA_PARTITION_TYPE);
	    open_partition(&sf,-1,UPDATE_PARTITION_TYPE);
	    open_partition(&sf,-1,CHANNEL_PARTITION_TYPE);
	    CloseSF(&sf,0);
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
// test_encryption()

static size_t DiffHexDump ( ccp title, const void * p_m1, const void * p_m2,
			off_t off, size_t size, int max_lines )
{
    const u8 * m1 = p_m1;
    const u8 * m2 = p_m2;
    const u8 * m1_end = m1 + size;
    ASSERT(m1);
    ASSERT(m2);

    if (!memcmp(p_m1,p_m2,size))
	return size;

    const size_t COLS = 16;

    size_t eq = size;
    int err_count = 0;

    for(;;)
    {
	while ( m1 < m1_end && *m1++ == *m2++ )
	    ;
	if ( m1 == m1_end )
	    break;

	m1--;
	m2--;

	off_t addr = off + m1 - (u8*)p_m1;
	if (!err_count)
	{
	    if ( title && *title )
		printf("\nDiff failed: %s\n",title);
	    eq = addr;
	}

	size_t max = m1_end - m1;
	if ( max > COLS )
	    max = COLS;
	HexDump(stdout,0,addr,10,COLS,m1,max);
	HexDump(stdout,0,addr,10,COLS,m2,max);

	if ( ++err_count == max_lines )
	    break;

	m1 += COLS;
	m2 += COLS;
    }
    
    return eq;    
}

///////////////////////////////////////////////////////////////////////////////

static int test_encryption_sf ( SuperFile_t * sf, wiidisc_t * disc )
{
    ASSERT(sf);
    ASSERT(disc);
    printf("\n***** TEST ENCRYPTION: %s [%d,%llx+%llx+%llx] *****\n\n",
		sf->f.fname,
		disc->partition_type,
		(u64)disc->partition_raw_offset4<<2,
		(u64)disc->partition_data_offset4<<2,
		(u64)disc->partition_data_size4<<2  );

    aes_key_t akey;
    wd_aes_set_key(&akey,disc->partition_key);

    const size_t grp_sectors	= WII_N_ELEMENTS_H1 * WII_N_ELEMENTS_H2;
    const size_t grp_size	= grp_sectors * sizeof(wd_part_sector_t);
    wd_part_control_t * pc	= malloc(sizeof(wd_part_control_t));
    wd_part_control_t * pc_load	= malloc(sizeof(wd_part_control_t));
    wd_part_sector_t *data	= malloc(grp_size);
    wd_part_sector_t *data_load	= malloc(grp_size);
    if ( !pc || !pc_load || !data || !data_load )
	OUT_OF_MEMORY;

    //---------------

    ASSERT( (u64)disc->partition_data_offset4<<2 == sizeof(pc->part_bin) ); 
    off_t off = (u64)disc->partition_raw_offset4<<2;
    ReadSF(sf,off,pc_load,sizeof(pc->part_bin));
    memcpy(pc,pc_load,sizeof(pc->part_bin));
    //printf("%8x %8x\n",*(u32*)(pc_load->part_bin+0x2bc),*(u32*)(pc->part_bin+0x2bc));

    setup_part_control(pc_load);
    DiffHexDump("pc #1:",pc_load,pc,0,sizeof(pc->part_bin),5);
    ASSERT(!memcmp(pc,pc_load,sizeof(pc->part_bin)));

    setup_part_control(pc);
    DiffHexDump("pc #2:",pc_load,pc,0,sizeof(pc->part_bin),5);
    ASSERT(!memcmp(pc,pc_load,sizeof(pc->part_bin)));

    u8 hash[WII_HASH_SIZE];
    SHA1( ((u8*)pc->tmd)+WII_TMD_SIG_OFF, pc->tmd_size-WII_TMD_SIG_OFF, hash );
    printf("HASH: ");
    HexDump(stdout,0,0,1,WII_HASH_SIZE,hash,WII_HASH_SIZE);
    
    //---------------

    off += pc_load->data_off;
    off_t max_off = off + (u64)disc->partition_data_size4<<2;

    int h3_index = 0;
    wd_build_disc_usage(disc,disc->partition_type,wdisc_usage_tab,sf->file_size);
    
    for ( ; off < max_off; off += grp_size, h3_index++ )
    {
	int sect;

	bool skip = false;
	u32 idx = off/WII_SECTOR_SIZE;
	for ( sect = 0; sect < grp_sectors; sect++ )
	    if (!wdisc_usage_tab[idx+sect])
	    {
		skip = true;
		break;
		
	    }

	if (skip)
	    continue;

	printf("%s: %9llx .. %9llx [%d]\n",
			skip ? "SKIP" : "TEST", (u64)off, (u64)off+grp_size, h3_index );

	// setup data

	ReadSF(sf,off,data_load,grp_size);
	for ( sect = 0; sect < grp_sectors; sect++ )
	{
	    wd_part_sector_t *d1 = data_load + sect, *d2 = data + sect;
	    memset(d2,0,WII_SECTOR_DATA_OFF);
	    wd_aes_decrypt( &akey, (ccp)d1+WII_SECTOR_IV_OFF,
				d1->data, d2->data, WII_SECTOR_DATA_SIZE );
	}

	// calc hash + encrypt

	EncryptSectorGroup( &akey, data, (max_off-off)/WII_SECTOR_SIZE,
			h3_index < WII_N_ELEMENTS_H3 ? pc->h3 + h3_index * WII_HASH_SIZE : 0 );

	// compare

	if ( DiffHexDump("data: load <-> calc",
			data_load,data,off,grp_size,5) != grp_size )
	    return 1;

	//break; // for fast tests only
    }
   
    //---------------

    printf("TEST H3:\n");
    if ( DiffHexDump("h3: load <-> calc",
			pc_load->h3, pc->h3, 0, pc->h3_size, 5 ) != pc->h3_size )
	return 1;

    //---------------

    printf("TEST TMD:\n");
    SHA1( pc->h3, pc->h3_size, pc->tmd->content[0].hash );
    if ( DiffHexDump("tmd: load <-> calc",
			pc_load->tmd, pc->tmd, 0, pc->tmd_size, 5 ) != pc->tmd_size )
	return 1;

    //---------------

    printf("TEST HEAD:\n");
    if ( DiffHexDump("head: load <-> calc",
			pc_load, pc, 0, sizeof(pc->part_bin), 5 ) != sizeof(pc->part_bin) )
	return 1;

    //---------------

    free(pc);
    free(pc_load);
    free(data);
    free(data_load);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static int test_encryption ( int argc, char ** argv )
{
    int i, stat = 0;
    
    for ( i = 1; i < argc && !stat; i++ )
    {
	SuperFile_t sf;
	InitializeSF(&sf);
	if (!OpenSF(&sf,argv[i],false,false))
	{
	    wiidisc_t * disc = wd_open_partition(WrapperReadSF,&sf,-1,DATA_PARTITION_TYPE);
	    if (disc)
	    {
		stat = test_encryption_sf(&sf,disc);
		wd_close_disc(disc);
	    }
	    CloseSF(&sf,0);
	}
    }
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void show_disc_usage_map ( int argc, char ** argv )
{
    partition_selector_t sel = ALL_PARTITIONS;

    int i;
    for ( i = 1; i < argc; i++ )
    {
	if ( !strcmp(argv[i],"-w"))
	{
	    sel = WHOLE_DISC;
	    continue;
	}

	SuperFile_t sf;
	InitializeSF(&sf);
	if (!OpenSF(&sf,argv[i],false,false))
	{
	    wiidisc_t * disc = wd_open_disc(WrapperReadSF,&sf);
	    if (disc)
	    {
		printf("\n*** %s ***\n",argv[i]);
		wd_build_disc_usage(disc,sel,wdisc_usage_tab,sf.file_size);
		wd_close_disc(disc);

		u8 * ptr = wdisc_usage_tab;
		u8 * tab_end = ptr + sizeof(wdisc_usage_tab);
		int skip_count = 0;
		while ( ptr < tab_end )
		{   
		    char * dest = iobuf + sprintf(iobuf,"%9llx:",
				(u64)(ptr-wdisc_usage_tab) * WII_SECTOR_SIZE );
		    u8 * line_end = ptr + 64;
		    if ( line_end > tab_end )
			line_end = tab_end;

		    int count = 0, pos = 0;
		    while ( ptr < line_end )
		    {
			if ( !( pos++ & 15 ) )
			    *dest++ = ' ';
			const u8 ch = *ptr++;
			if (!ch)
			    *dest++ = '.';
			else
			{
			    count++;
			    *dest++ = ch <= 10 ? '0' + ch : '?';
			}
		    }
		    if (count)
		    {
			if (skip_count)
			{
			    skip_count = 0;
			    printf("      ...\n");
			}
			*dest = 0;
			printf("%s\n",iobuf);
		    }
		    else
			skip_count++;
		}
	    }
	    CloseSF(&sf,0);
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void show_disc_usage_table ( int argc, char ** argv )
{
    partition_selector_t sel = ALL_PARTITIONS;

    int i;
    for ( i = 1; i < argc; i++ )
    {
	if ( !strcmp(argv[i],"-w"))
	{
	    sel = WHOLE_DISC;
	    continue;
	}

	SuperFile_t sf;
	InitializeSF(&sf);
	if (!OpenSF(&sf,argv[i],false,false))
	{
	    wiidisc_t * disc = wd_open_disc(WrapperReadSF,&sf);
	    if (disc)
	    {
		printf("\n*** %s ***\n",argv[i]);
		wd_build_disc_usage(disc,sel,wdisc_usage_tab,sf.file_size);
		wd_close_disc(disc);

		u8 * ptr = wdisc_usage_tab;
		u8 * tab_end = ptr + sizeof(wdisc_usage_tab);
		while ( ptr < tab_end )
		{
		    const u32 b1 = ptr - wdisc_usage_tab;

		    u8 active = *ptr;
		    while ( ptr < tab_end && *ptr == active )
			ptr++;

		    if (active)
		    {
			const u32 b2 = ptr - wdisc_usage_tab;
			const u64 o2 = b2 * (u64)WII_SECTOR_SIZE;
			const u64 o1 = b1 * (u64)WII_SECTOR_SIZE;

			printf(" %2x : %5x .. %5x, %5x => %9llx .. %9llx, %9llx\n",
				active, b1, b2, b2-b1, o1, o2, o2-o1 );
		    }
		    else
			printf("  -\n");
		}
	    }
	    CloseSF(&sf,0);
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
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
    u8 h1[100], h2[100], source[1000-21];;

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
///////////////////////////////////////////////////////////////////////////////

enum
{
    CMD_HELP,			// help_exit();
    CMD_TEST,			// test();

    CMD_MATCH_PATTERN,		// test_match_pattern(argc,argv);
    CMD_OPEN_PARTITION,		// test_open_partition(argc,argv);
    CMD_ENCRYPTION,		// test_encryption(argc,argv);
    CMD_USAGE_MAP,		// show_disc_usage_map(argc,argv);
    CMD_USAGE_TABLE,		// show_disc_usage_table(argc,argv);

    CMD_SHA1,			// test_sha1();
    CMD_WIIMM,			// test_wiimm(argc,argv);

    CMD__N
};


static const CommandTab_t CommandTab[] =
{
	{ CMD_HELP,		"HELP",		"?",		0 },
	{ CMD_TEST,		"TEST",		"T",		0 },

	{ CMD_MATCH_PATTERN,	"MATCH",	0,		0 },
	{ CMD_OPEN_PARTITION,	"OPENPART",	"OPART",	0 },
	{ CMD_ENCRYPTION,	"ENCRYPTION",	0,		0 },
	{ CMD_USAGE_MAP,	"USAGEMAP",	"UMAP",		0 },
	{ CMD_USAGE_TABLE,	"USAGETABLE",	"UTAB",		0 },

 #ifdef HAVE_OPENSSL
	{ CMD_SHA1,		"SHA1",		0,		0 },
 #endif
 #ifdef HAVE_WORK_DIR
	{ CMD_WIIMM,		"WIIMM",	"W",		0 },
 #endif

	{ CMD__N,0,0,0 }
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void help_exit()
{
    printf("\nCommands:\n\n");
    const CommandTab_t * cmd;
    for ( cmd = CommandTab; cmd->name1; cmd++ )
	if (cmd->name2)
	    printf("  %-10s | %s\n",cmd->name1,cmd->name2);
	else
	    printf("  %s\n",cmd->name1);
    putchar('\n');
    exit(ERR_SYNTAX);
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int test ( int argc, char ** argv )
{
 #if 0
    unlink("_temp.wdf");
    gen_wdf("_temp.wdf");
    read_behind_file("_temp.wdf");
    read_behind_wdf("_temp.wdf");
 #endif

    //test_string_field();

    //test_create_file();
    //test_create_sparse_file();
    //test_splitted_file();

    return test_zero_wdf();

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    printf("*****  %s  *****\n",TITLE);
    SetupLib(argc,argv,NAME,PROG_UNKNOWN);

    printf("term width = %d\n",GetTermWidth(80,0));

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

    switch(cmd_ct->id)
    {
	case CMD_HELP:			help_exit(); break;
	case CMD_TEST:			return test(argc,argv); break;

	case CMD_MATCH_PATTERN:		test_match_pattern(argc,argv); break;
	case CMD_OPEN_PARTITION:	test_open_partition(argc,argv); break;
	case CMD_ENCRYPTION:		test_encryption(argc,argv); break;
	case CMD_USAGE_MAP:		show_disc_usage_map(argc,argv); break;
	case CMD_USAGE_TABLE:		show_disc_usage_table(argc,argv); break;

 #ifdef HAVE_OPENSSL
	case CMD_SHA1:			test_sha1(); break;
 #endif
 #ifdef HAVE_WORK_DIR
	case CMD_WIIMM:			test_wiimm(argc,argv); break;
 #endif

	default:
	    help_exit();
    }

    if (SIGINT_level)
	ERROR0(ERR_INTERRUPT,"Program interrupted by user.");

    return max_error;
}

///////////////////////////////////////////////////////////////////////////////
