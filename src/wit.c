
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
#include <sys/time.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include "debug.h"
#include "version.h"
#include "wiidisc.h"
#include "lib-std.h"
#include "lib-sf.h"
#include "titles.h"
#include "iso-interface.h"
#include "wbfs-interface.h"
#include "match-pattern.h"
#include "crypt.h"

#include "ui-wit.c"

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define TITLE WIT_SHORT ": " WIT_LONG " v" VERSION " r" REVISION \
	" " SYSTEM " - " AUTHOR " - " DATE

int  print_sections	= 0;
int  long_count		= 0;
int  ignore_count	= 0;
int  testmode		= 0;
ccp  opt_dest		= 0;
bool opt_mkdir		= false;
int  opt_limit		= -1;

enumIOMode io_mode	= 0;

//
///////////////////////////////////////////////////////////////////////////////

void help_exit ( bool xmode )
{
    fputs( TITLE "\n", stdout );

    if (xmode)
    {
	int cmd;
	for ( cmd = 0; cmd < CMD__N; cmd++ )
	    PrintHelpCmd(&InfoUI,stdout,0,cmd,0);
    }
    else
	PrintHelpCmd(&InfoUI,stdout,0,0,0);

    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

void version_exit()
{
    if (print_sections)
	fputs("[version]\n",stdout);

    if ( print_sections || long_count )
	printf(	"prog=" WIT_SHORT "\n"
		"name=\"" WIT_LONG "\"\n"
		"version=" VERSION "\n"
		"beta=%d\n"
		"revision=" REVISION  "\n"
		"system=" SYSTEM "\n"
		"author=\"" AUTHOR "\"\n"
		"date=" DATE "\n"
		, BETA_VERSION );
    else
	fputs( TITLE "\n", stdout );
    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

void print_title ( FILE * f )
{
    static bool done = false;
    if (!done)
    {
	done = true;
	if ( verbose >= 1 && f == stdout )
	    fprintf(f,"\n%s\n\n",TITLE);
	else
	    fprintf(f,"*****  %s  *****\n",TITLE);
    }
}

///////////////////////////////////////////////////////////////////////////////

void hint_exit ( enumError stat, ccp command )
{
    if (command)
	fprintf(stderr,
	    "-> Type '%s help %s' for more help.\n\n",
	    progname, command );
    else
	fprintf(stderr,
	    "-> Type '%s -h', '%s help' or '%s help command' for more help.\n\n",
	    progname, progname, progname );
    exit(stat);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     commands                    ///////////////
///////////////////////////////////////////////////////////////////////////////

// common commands of 'wwt' and 'wit'
#define IS_WIT 1
#include "wwt+wit-cmd.c"

///////////////////////////////////////////////////////////////////////////////

enumError cmd_test()
{
 #if 0 || !defined(TEST) // test options

    return cmd_test_options();

 #elif 1

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	u8 buf[10];
	int count;
	ccp res = ScanHexHelper(buf,sizeof(buf),&count,param->arg,99);
	printf("\ncount=%d |%s|\n",count,res);
	HEXDUMP16(0,0,buf,sizeof(buf));
    }
    return ERR_OK;
    
 #elif 1

    {
	ccp msg = "Dieses ist ein langer Satz, "
		  "der noch ein wenig länger ist "
		  "und noch ein wenig länger ist "
		  "und hier endet.\n";
	ERROR1(ERR_WARNING,"%s%s",msg,msg);
	ERROR1(ERR_ERROR,"%s%s",msg,msg);
    }
    return ERR_OK;

 #elif 1

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	char id6[7];
	enumFileType ftype = IsFST(param->arg,id6);
	printf("%05x %-6s %s\n",ftype,id6,param->arg);
    }
    return ERR_OK;
    
 #else

    int i, max = 5;
    for ( i=1; i <= max; i++ )
    {
	fprintf(stderr,"sleep 20 sec (%d/%d)\n",i,max);
	sleep(20);
    }
    return ERR_OK;

 #endif
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_create()
{
    if ( n_param < 1 )
    {
	ERROR0(ERR_SYNTAX,"Missing sub command for CREATE.\n");
	hint_exit(ERR_SYNTAX,"create");
    }

    //----- find sub command

    enum // sub commands
    {
	SC_TICKET,
	SC_TMD,
    };


    static const CommandTab_t tab[] =
    {						    // min + max param
	{ SC_TICKET,	"TICKET",	"TIK",		0 | 0x100 * 2 },
	{ SC_TMD,	"TMD",		0,		0 | 0x100 * 1 },

	{ 0,0,0,0 }
    };

    ParamList_t * param = first_param;
    ccp cmd_name = param->arg;
    int cmd_stat;
    const CommandTab_t * cmd = ScanCommand(&cmd_stat,cmd_name,tab);
    if (!cmd)
    {
	if ( cmd_stat > 0 )
	    ERROR0(ERR_SYNTAX,"Sub command abbreviation is ambiguous: %s\n",cmd_name);
	else
	    ERROR0(ERR_SYNTAX,"Unknown sub command: %s\n",cmd_name);
	hint_exit(ERR_SYNTAX,"create");
    }

    param = param->next;


    //----- find destination file

    if ( !param )
    {
	ERROR0(ERR_SYNTAX,"Missing filename for CREATE.\n");
	hint_exit(ERR_SYNTAX,"create");
    }

    char pbuf1[PATH_MAX], pbuf2[PATH_MAX], namebuf[50];
    ccp path = PathCatPP(pbuf1,sizeof(pbuf1),opt_dest,param->arg);
    if (IsDirectory(path,0))
    {
	ccp src = cmd->name1;
	char * dest = namebuf;
	while (*src)
	    *dest++ = tolower((int)*src++);
	DASSERT( dest < namebuf + sizeof(namebuf) );

	src = ".bin";
	while (*src)
	    *dest++ = tolower((int)*src++);
	DASSERT( dest < namebuf + sizeof(namebuf) );
	*dest = 0;
	
	path = PathCatPP(pbuf2,sizeof(pbuf2),path,namebuf);
    }

    param = param->next;


    //----- check number of params

    const int max_param = ( cmd->opt & 0xff00 ) >> 8;
    const int min_param = cmd->opt & 0xff;
    n_param -= 2;
    if ( n_param < min_param || n_param > max_param )
    {
	ERROR0(ERR_SYNTAX,"Wrong number of arguments.\n");
	hint_exit(ERR_SYNTAX,"create");
    }

    if ( testmode || verbose > 0 )
	printf("Create %s: %s\n",cmd->name1,path);


    //----- execute

    switch(cmd->id)
    {
      case SC_TICKET:
	{
	    wd_ticket_t tik;
	    ticket_setup(&tik,modify_id);

	    if (param)
	    {
		if ( *param->arg && strcmp(param->arg,"-") )
		{
		    const enumError err
			= ScanHex(tik.ticket_id,sizeof(tik.ticket_id),param->arg);
		    if (err)
			return err;
		}
		param = param->next;
	    }

	    if (param)
	    {
		if ( *param->arg && strcmp(param->arg,"-") )
		{
		    u8 key[WII_KEY_SIZE];
		    const enumError err = ScanHex(key,sizeof(key),param->arg);
		    if (err)
			return err;
		    wd_encrypt_title_key(&tik,key);
		}
		param = param->next;
	    }

	    ticket_sign_trucha(&tik,sizeof(tik));
	    if ( verbose > 1 )
		Dump_TIK_MEM(stdout,2,&tik);
	    if (!testmode)
	    {
		const enumError err = SaveFile(path,0,opt_mkdir,
						&tik,sizeof(tik),false);
		if (!err)
		    return err;
	    }
	}
	break;

      case SC_TMD:
	{
	    char tmd_buf[WII_TMD_GOOD_SIZE];
	    wd_tmd_t * tmd = (wd_tmd_t*)tmd_buf;
	    tmd_setup(tmd,sizeof(tmd_buf),modify_id);

	    if (opt_ios_valid)
		tmd->sys_version = hton64(opt_ios);

	    if (param)
	    {
		if ( *param->arg && strcmp(param->arg,"-") )
		{
		    const enumError err
			= ScanHex(tmd->content[0].hash,
				sizeof(tmd->content[0].hash),param->arg);
		    if (err)
			return err;
		}
		param = param->next;
	    }

	    tmd_sign_trucha(tmd,sizeof(tmd_buf));
	    if ( verbose > 1 )
		Dump_TMD_MEM(stdout,2,tmd,1);
	    if (!testmode)
	    {
		const enumError err = SaveFile(path,0,opt_mkdir,
						tmd,sizeof(tmd_buf),false);
		if (!err)
		    return err;
	    }
	}
	break;
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_filelist ( SuperFile_t * sf, Iterator_t * it )
{
    ASSERT(sf);
    ASSERT(it);

    printf("%s\n", it->long_count ? it->real_path : sf->f.fname );
    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError cmd_filelist()
{
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    encoding |= ENCODE_F_FAST; // hint: no encryption needed

    Iterator_t it;
    InitializeIterator(&it);
    it.func		= exec_filelist;
    it.act_non_exist	= ignore_count > 0 ? ACT_IGNORE : ACT_ALLOW;
    it.act_non_iso	= ignore_count > 1 ? ACT_IGNORE : ACT_ALLOW;
    it.act_fst		= allow_fst ? ACT_ALLOW : ACT_IGNORE;
    it.long_count	= long_count;
    const enumError err = SourceIterator(&it,1,true,false);
    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_filetype ( SuperFile_t * sf, Iterator_t * it )
{
    ASSERT(sf);
    ASSERT(it);

    const bool print_header = !(used_options&OB_NO_HEADER);
    ccp ftype = GetNameFT(sf->f.ftype,0);

    if ( it->long_count > 1 )
    {
	if ( print_header && !it->done_count++  )
	    printf("\n"
		"file     disc   size reg split\n"
		"type     ID6     MIB ion   N  %s\n"
		"%s\n",
		it->long_count > 2 ? "real path" : "filename", sep_79 );

	char split[10] = " -";
	if ( sf->f.split_used > 1 )
	    snprintf(split,sizeof(split),"%2d",sf->f.split_used);

	ccp region = "-   ";
	char size[10] = "   -";
	if (sf->f.id6[0])
	{
	    region = GetRegionInfo(sf->f.id6[3])->name4;
	    u32 count = CountUsedIsoBlocksSF(sf,part_selector);
	    if (count)
		snprintf(size,sizeof(size),"%4u",
			(count+WII_SECTORS_PER_MIB/2)/WII_SECTORS_PER_MIB);
	}

	printf("%-8s %-6s %s %s %s  %s\n",
		ftype, sf->f.id6[0] ? sf->f.id6 : "-",
		size, region, split,
		it->long_count > 2 ? it->real_path : sf->f.fname );
    }
    else if (it->long_count)
    {
	if ( print_header && !it->done_count++  )
	    printf("\n"
		"file     disc  split\n"
		"type     ID6     N  filename\n"
		"%s\n", sep_79 );

	char split[10] = " -";
	if ( sf->f.split_used > 1 )
	    snprintf(split,sizeof(split),"%2d",sf->f.split_used);
	printf("%-8s %-6s %s  %s\n",
		ftype, sf->f.id6[0] ? sf->f.id6 : "-", split, sf->f.fname );
    }
    else
    {
	if ( print_header && !it->done_count++  )
	    printf("\n"
		"file\n"
		"type     filename\n"
		"%s\n", sep_79 );

	printf("%-8s %s\n", ftype, sf->f.fname );
    }

    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError cmd_filetype()
{
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    encoding |= ENCODE_F_FAST; // hint: no encryption needed

    Iterator_t it;
    InitializeIterator(&it);
    it.func		= exec_filetype;
    it.act_non_exist	= ignore_count > 0 ? ACT_IGNORE : ACT_ALLOW;
    it.act_non_iso	= ignore_count > 1 ? ACT_IGNORE : ACT_ALLOW;
    it.act_fst		= !allow_fst ? ACT_IGNORE
					 : long_count > 1 ? ACT_EXPAND : ACT_ALLOW;
    it.long_count	= long_count;
    const enumError err = SourceIterator(&it,1,true,false);

    if ( !(used_options&OB_NO_HEADER) && it.done_count )
	putchar('\n');

    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_isosize ( SuperFile_t * sf, Iterator_t * it )
{
    ASSERT(sf);
    ASSERT(it);

    wd_disc_t * disc = 0;
    if (sf->f.id6[0])
    {
	disc = OpenDiscSF(sf,true,true);
	if (disc)
	    wd_filter_usage_table_sel(disc,wdisc_usage_tab,part_selector);
    }

    const bool print_header = !(used_options&OB_NO_HEADER);

    if ( it->long_count > 1 )
    {
	if ( print_header && !it->done_count++  )
	    printf("\n"
		"   ISO  ISO .wbfs 500g\n"
		"blocks  MiB  file WBFS  %s\n"
		"%s\n",
		it->long_count > 2 ? "real path" : "filename", sep_79 );

	char size[30] = "     -    -     -    -";
	if (disc)
	{
	    const u32 count = wd_count_used_blocks(wdisc_usage_tab,1);
	    if (count)
	    {
		// wbfs: size=10g => block size = 2 MiB
		const u32 wfile = 1 + wd_count_used_blocks( wdisc_usage_tab,
						2 * WII_SECTORS_PER_MIB );
		// wbfs: size=500g => block size = 8 MiB
		const u32 w500 =  wd_count_used_blocks( wdisc_usage_tab,
						8 * WII_SECTORS_PER_MIB );

		snprintf(size,sizeof(size),"%6u %4u %5u %4u",
			count, (count+WII_SECTORS_PER_MIB/2)/WII_SECTORS_PER_MIB,
			2*wfile, 8*w500 );
	    }
	}
	printf("%s  %s\n", size, it->long_count > 2 ? it->real_path : sf->f.fname );
    }
    else if ( it->long_count )
    {
	if ( print_header && !it->done_count++  )
	    printf("\n"
		"   ISO  ISO\n"
		"blocks  MiB  filename\n"
		"%s\n", sep_79 );

	char size[20] = "     -    -";
	if (sf->f.id6[0])
	{
	    const u32 count = wd_count_used_blocks(wdisc_usage_tab,1);
	    if (count)
		snprintf(size,sizeof(size),"%6u %4u",
			count, (count+WII_SECTORS_PER_MIB/2)/WII_SECTORS_PER_MIB);
	}
	printf("%s  %s\n", size, sf->f.fname );
    }
    else
    {
	if ( print_header && !it->done_count++  )
	    printf("\n"
		"   ISO\n"
		"blocks  filename\n"
		"%s\n", sep_79 );

	char size[20] = "     -";
	if (sf->f.id6[0])
	{
	    const u32 count = wd_count_used_blocks(wdisc_usage_tab,1);
	    if (count)
		snprintf(size,sizeof(size),"%6u",count);
	}
	printf("%s  %s\n", size, sf->f.fname );
    }

    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError cmd_isosize()
{
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    encoding |= ENCODE_F_FAST; // hint: no encryption needed

    Iterator_t it;
    InitializeIterator(&it);
    it.func		= exec_isosize;
    it.act_non_exist	= ignore_count > 0 ? ACT_IGNORE : ACT_ALLOW;
    it.act_non_iso	= ignore_count > 1 ? ACT_IGNORE : ACT_ALLOW;
    it.act_fst		= allow_fst ? ACT_EXPAND : ACT_IGNORE;
    it.act_wbfs		= ACT_EXPAND;
    it.long_count	= long_count;
    const enumError err = SourceIterator(&it,1,true,false);

    if ( !(used_options&OB_NO_HEADER) && it.done_count )
	putchar('\n');

    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

static void dump_data ( off_t base, u32 off4, off_t size, ccp text )
{
    const u64 off = (off_t)off4 << 2;
    const u64 end = off + size;
    printf("    %-5s %9llx .. %9llx -> %9llx .. %9llx, size:%10llx/hex =%11llu\n",
		text, off, end, (u64)base+off, (u64)base+end, (u64)size, (u64)size );
}

//-----------------------------------------------------------------------------

enumError exec_dump ( SuperFile_t * sf, Iterator_t * it )
{
    TRACE("exec_dump()");
    ASSERT(sf);
    ASSERT(it);
    fflush(0);

    if ( sf->f.ftype & FT_A_ISO )
	return Dump_ISO(stdout,0,sf,it->real_path,opt_show_mode,it->long_count);

    if ( sf->f.ftype & FT_ID_DOL )
	return Dump_DOL(stdout,0,sf,it->real_path);

    if ( sf->f.ftype & FT_ID_FST_BIN )
	return Dump_FST_BIN(stdout,0,sf,it->real_path,opt_show_mode);

    if ( sf->f.ftype & FT_ID_TIK_BIN )
	return Dump_TIK_BIN(stdout,0,sf,it->real_path);

    if ( sf->f.ftype & FT_ID_TMD_BIN )
	return Dump_TMD_BIN(stdout,0,sf,it->real_path);

    if ( sf->f.ftype & FT_ID_HEAD_BIN )
	return Dump_HEAD_BIN(stdout,0,sf,it->real_path);

    if ( sf->f.ftype & FT_ID_BOOT_BIN )
	return Dump_BOOT_BIN(stdout,0,sf,it->real_path);

    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError cmd_dump()
{
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    encoding |= ENCODE_F_FAST; // hint: no encryption needed

    Iterator_t it;
    InitializeIterator(&it);
    it.func		= exec_dump;
    it.act_known	= ACT_ALLOW;
    it.act_wbfs		= ACT_EXPAND;
    it.act_fst		= allow_fst ? ACT_EXPAND : ACT_IGNORE;
    it.long_count	= long_count;

    enumError err = SourceIterator(&it,0,false,true);
    if ( err <= ERR_WARNING )
	err = SourceIteratorCollected(&it,1);
    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_dregion ( SuperFile_t * sf, Iterator_t * it )
{
    TRACE("exec_dump()");
    ASSERT(sf);
    ASSERT(it);
    fflush(0);

    char buf[WII_REGION_SIZE];
    if (!ReadSF(sf,WII_REGION_OFF,buf,sizeof(buf)))
    {
	ASSERT( sizeof(buf) == 32 );
	u32 * p = (u32*)buf;
	printf("%.1s %-6s %08x %08x %08x %08x %08x %08x %08x %08x\n",
		sf->f.id6+3, sf->f.id6,
		ntohl(p[0]), ntohl(p[1]), ntohl(p[2]), ntohl(p[3]),
		ntohl(p[4]), ntohl(p[5]), ntohl(p[6]), ntohl(p[7]) );
    }

    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError cmd_dregion()
{
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    encoding |= ENCODE_F_FAST; // hint: no encryption needed

    printf("R   ID  off=%-8x    +4       +8       +c      +10      +14      +18      +1c\n"
	   "%.80s\n", WII_REGION_OFF, wd_sep_200 );
    Iterator_t it;
    InitializeIterator(&it);
    it.func		= exec_dregion;
    it.act_known	= ACT_ALLOW;
    it.act_wbfs		= ACT_EXPAND;
    it.act_fst		= ACT_IGNORE;

    enumError err = SourceIterator(&it,0,false,true);
    if ( err <= ERR_WARNING )
	err = SourceIteratorCollected(&it,1);
    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_collect ( SuperFile_t * sf, Iterator_t * it )
{
    ASSERT(sf);
    ASSERT(it);
    ASSERT(it->wlist);

    WDiscInfo_t wdi;
    InitializeWDiscInfo(&wdi);
    CalcWDiscInfo(&wdi,sf);

    WDiscList_t * wl = it->wlist;
    WDiscListItem_t * item = AppendWDiscList(wl,&wdi);
    if ( it->real_filename && sf->f.path && *sf->f.path )
    {
	item->fname = sf->f.path;
	sf->f.path  = EmptyString;
    }
    else if ( it->long_count > 2 )
	item->fname = strdup(it->real_path);
    else
    {
	item->fname = sf->f.fname;
	sf->f.fname = EmptyString;
    }
    TRACE("WLIST: %d/%d\n",wl->used,wl->size);

    item->used_blocks = wdi.used_blocks;
    item->size_mib = (sf->f.fatt.size+MiB/2)/MiB;
    wl->total_size_mib += item->size_mib;

    item->ftype = sf->f.ftype;
    item->wbfs_slot = sf->f.ftype & FT_ID_WBFS && sf->wbfs && sf->wbfs->disc
			? sf->wbfs->disc->slot : -1;
    CopyFileAttrib(&item->fatt,&sf->f.fatt);

    ResetWDiscInfo(&wdi);
    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError cmd_id6()
{
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    encoding |= ENCODE_F_FAST; // hint: no encryption needed

    WDiscList_t wlist;
    InitializeWDiscList(&wlist);

    Iterator_t it;
    InitializeIterator(&it);
    it.func		= exec_collect;
    it.act_wbfs		= ACT_EXPAND;
    it.act_fst		= allow_fst ? ACT_EXPAND : ACT_IGNORE;
    it.long_count	= long_count;
    it.wlist		= &wlist;

    enumError err = SourceIterator(&it,0,true,false);
    ResetIterator(&it);
    if ( err > ERR_WARNING )
	return err;

    SortWDiscList(&wlist,sort_mode,SORT_ID, used_options&OB_UNIQUE ? 2 : 0 );

    WDiscListItem_t * ptr = wlist.first_disc;
    WDiscListItem_t * end = ptr + wlist.used;
    for ( ; ptr < end; ptr++ )
	printf("%s\n", ptr->id6 );

    ResetWDiscList(&wlist);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_list ( int long_level )
{
    if ( long_level > 0 )
    {
	RegisterOption(&InfoUI,OPT_LONG,long_level,false);
	long_count += long_level;
    }

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    encoding |= ENCODE_F_FAST; // hint: no encryption needed

    WDiscList_t wlist;
    InitializeWDiscList(&wlist);

    Iterator_t it;
    InitializeIterator(&it);
    it.func		= exec_collect;
    it.act_wbfs		= ACT_EXPAND;
    it.act_fst		= allow_fst ? ACT_EXPAND : ACT_IGNORE;
    it.long_count	= long_count;
    it.real_filename	= print_sections > 0;
    it.wlist		= &wlist;

    enumError err = SourceIterator(&it,1,true,false);
    ResetIterator(&it);
    if ( err > ERR_WARNING )
	return err;

    SortWDiscList(&wlist,sort_mode,SORT_TITLE, used_options&OB_UNIQUE ? 1 : 0 );

    //------------------------------

    if ( print_sections )
    {
	char buf[10];
	snprintf(buf,sizeof(buf),"%u",wlist.used-1);
	const int fw = strlen(buf);

	int i;
	WDiscListItem_t *witem;
	for ( i = 0, witem = wlist.first_disc; i < wlist.used; i++, witem++ )
	{
	    printf("\n[disc-%0*u]\n",fw,i);
	    PrintSectWDiscListItem(stdout,witem,0);
	}
	return ERR_OK;
    }

    //------------------------------

    char footer[200];
    int footer_len = 0;

    WDiscListItem_t *witem, *wend = wlist.first_disc + wlist.used;

    const bool print_header = !(used_options&OB_NO_HEADER);
    const bool line2 = long_count > 2;

    PrintTime_t pt;
    int opt_time = opt_print_time;
    if ( long_count > 1 )
	opt_time = EnablePrintTime(opt_time);
    SetupPrintTime(&pt,opt_time);

    int max_name_wd = 0;
    if (print_header)
    {
	for ( witem = wlist.first_disc; witem < wend; witem++ )
	{
	    const int plen = strlen( witem->title
					? witem->title : witem->name64 );
	    if ( max_name_wd < plen )
		max_name_wd = plen;

	    if ( line2 && witem->fname )
	    {
		const int flen = strlen(witem->fname);
		if ( max_name_wd < flen )
		    max_name_wd = flen;
	    }
	}

	footer_len = snprintf(footer,sizeof(footer),
		"Total: %u discs, %u MiB ~ %u GiB used.",
		wlist.used,
		wlist.total_size_mib, (wlist.total_size_mib+512)/1024 );
    }

    if (long_count)
    {
	if (print_header)
	{
	    int n1, n2;
	    putchar('\n');
	    printf("ID6   %s  MiB Reg.  %n%d discs (%d GiB)%n\n",
		    pt.head, &n1, wlist.used,
		    (wlist.total_size_mib+512)/1024, &n2 );
	    max_name_wd += n1;
	    if ( max_name_wd < n2 )
		max_name_wd = n2;

	    if (line2)
	    {
		fputs(pt.fill,stdout);
		fputs("      n(p) type   path of file\n",stdout);
	    }

	    if ( max_name_wd < footer_len )
		max_name_wd = footer_len;
	    printf("%.*s\n", max_name_wd, wd_sep_200);
	}

	for ( witem = wlist.first_disc; witem < wend; witem++ )
	{
 	    printf("%s%s %4d %s  %s\n",
		    witem->id6, PrintTime(&pt,&witem->fatt),
		    witem->size_mib, GetRegionInfo(witem->id6[3])->name4,
		    witem->title ? witem->title : witem->name64 );
	    if (line2)
		printf("%s%9d %7s %s\n",
		    pt.fill, witem->n_part, GetNameFT(witem->ftype,0),
		    witem->fname ? witem->fname : "" );
	}
    }
    else
    {
	if (print_header)
	{
	    int n1, n2;
	    putchar('\n');
	    printf("ID6    %s %n%d discs (%d GiB)%n\n",
		    pt.head, &n1, wlist.used, (wlist.total_size_mib+512)/1024, &n2 );
	    max_name_wd += n1;
	    if ( max_name_wd < n2 )
		max_name_wd = n2;
	    printf("%.*s\n", max_name_wd, wd_sep_200 );
	}

	for ( witem = wlist.first_disc; witem < wend; witem++ )
	    printf("%s%s  %s\n", witem->id6, PrintTime(&pt,&witem->fatt),
			witem->title ? witem->title : witem->name64 );
    }

    if (print_header)
	printf("%.*s\n%s\n\n", max_name_wd, wd_sep_200, footer );

    ResetWDiscList(&wlist);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_ilist ( SuperFile_t * fi, Iterator_t * it )
{
    ASSERT(fi);
    if ( fi->f.ftype & FT_ID_FST_BIN )
	return Dump_FST_BIN(stdout,0,fi,it->real_path,it->show_mode&~SHOW_INTRO);

    if ( fi->f.ftype & FT__SPC_MASK )
    {
	if ( !(used_options & OB_IGNORE) )
	    PrintErrorFT(&fi->f,FT_A_ISO);
	return ERR_OK;
    }

    if (!fi->f.id6[0])
	return ERR_OK;

    wd_disc_t * disc = OpenDiscSF(fi,true,true);
    if (!disc)
	return ERR_WDISC_NOT_FOUND;
    wd_select(disc,part_selector);

    char prefix[PATH_MAX];
    if ( it->show_mode & SHOW_PATH )
	PathCatPP(prefix,sizeof(prefix),fi->f.fname,"/");
    else
	*prefix = 0;

    WiiFst_t fst;
    InitializeFST(&fst);
    CollectFST(&fst,disc,GetDefaultFilePattern(),false,prefix_mode,true);
    SortFST(&fst,sort_mode,SORT_NAME);
    DumpFilesFST(stdout,0,&fst,ConvertShow2PFST(it->show_mode,0),prefix);
    ResetFST(&fst);

    return disc->invalid_part ? ERR_WDISC_INVALID : ERR_OK;
}

//-----------------------------------------------------------------------------

enumError cmd_ilist ( int long_level )
{
    if ( long_level > 0 )
    {
	RegisterOption(&InfoUI,OPT_LONG,long_level,false);
	long_count += long_level;
    }

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    encoding |= ENCODE_F_FAST; // hint: no encryption needed

    Iterator_t it;
    InitializeIterator(&it);
    //it.act_non_iso	= used_options & OB_IGNORE ? ACT_IGNORE : ACT_WARN;
    it.act_non_iso	= ACT_ALLOW;
    it.act_wbfs		= ACT_EXPAND;
    it.act_fst		= allow_fst ? ACT_EXPAND : ACT_IGNORE;
    it.long_count	= long_count;

    if ( it.show_mode & SHOW__DEFAULT )
    {
	switch (long_count)
	{
	    case 0:  it.show_mode = SHOW_F_HEAD; break;
	    case 1:  it.show_mode = SHOW_F_HEAD | SHOW_SIZE | SHOW_F_DEC; break;
	    case 2:  it.show_mode = SHOW_F_HEAD | SHOW__ALL & ~SHOW_PATH; break;
	    default: it.show_mode = SHOW_F_HEAD | SHOW__ALL; break;
	}
    }
    if ( used_options & OB_NO_HEADER )
	it.show_mode &= ~SHOW_F_HEAD;
    if ( it.show_mode & (SHOW_F_DEC|SHOW_F_HEX) )
	it.show_mode |= SHOW_SIZE;

    enumError err = SourceIterator(&it,0,false,true);
    if ( err <= ERR_WARNING )
    {
	it.func = exec_ilist;
	err = SourceIteratorCollected(&it,1);
    }
    ResetIterator(&it);

    if ( it.show_mode & SHOW_F_HEAD )
	putchar('\n');
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_diff ( SuperFile_t * f1, Iterator_t * it )
{
    if (!f1->f.id6[0])
	return ERR_OK;
    fflush(0);

    SuperFile_t f2;
    InitializeSF(&f2);
    f2.allow_fst = allow_fst;

    enumOFT oft = CalcOFT(output_file_type,opt_dest,f1->f.fname,OFT__DEFAULT);
    ccp oname = oft == OFT_FST
		    ? 0
		    : oft == OFT_WBFS && f1->f.id6[0]
			? f1->f.id6
			: f1->f.outname
			    ? f1->f.outname
			    : f1->f.fname;
    GenImageFileName(&f2.f,opt_dest,oname,oft);
    SubstFileNameSF(&f2,f1,0);

    //f2.f.disable_errors = it->act_non_exist != ACT_WARN;
    enumError err = OpenSF(&f2,0,it->act_non_iso||it->act_wbfs>=ACT_ALLOW,0);
    if (err)
    {
	it->diff_count++;
	return ERR_OK;
    }
    f2.f.disable_errors = false;

    f2.indent		= 5;
    f2.show_progress	= verbose > 1 || progress;
    f2.show_summary	= verbose > 0 || progress;
    f2.show_msec	= verbose > 2;

    const bool raw_mode = part_selector & WD_SEL_WHOLE_DISC || !f1->f.id6[0];
    if (testmode)
    {
	printf( "%s: WOULD DIFF/%s %s:%s : %s:%s\n",
		progname, raw_mode ? "RAW" : "SCRUB",
		oft_name[f1->iod.oft], f1->f.fname,
		oft_name[f2.iod.oft], f2.f.fname );
	ResetSF(&f2,0);
	return ERR_OK;
    }

    if ( verbose > 0 )
    {
	printf( "* DIFF/%s %s:%s -> %s:%s\n",
		raw_mode ? "RAW" : "SCRUB",
		oft_name[f1->iod.oft], f1->f.fname,
		oft_name[f2.iod.oft], f2.f.fname );
    }

    const wd_select_t psel = raw_mode ? WD_SEL_WHOLE_DISC : part_selector;
    FilePattern_t * pat = GetDefaultFilePattern();
    err = SetupFilePattern(pat)
		? DiffFilesSF( f1, &f2, it->long_count, pat, psel, prefix_mode )
		: DiffSF( f1, &f2, it->long_count, psel );

    if ( err == ERR_DIFFER )
    {
	it->diff_count++;
	err = ERR_OK;
	if ( verbose >= 0 )
	    printf( "! ISOs differ: %s:%s : %s:%s\n",
			oft_name[f1->iod.oft], f1->f.fname,
			oft_name[f2.iod.oft], f2.f.fname );
    }
    it->done_count++;

    ResetSF(&f2,0);
    return err;
}

//-----------------------------------------------------------------------------

enumError cmd_diff()
{
    if ( verbose > 0 )
	print_title(stdout);

    if (!opt_dest)
    {
	if (!first_param)
	    return ERROR0(ERR_MISSING_PARAM, "Missing destination parameter\n" );

	ParamList_t * param;
	for ( param = first_param; param->next; param = param->next )
	    ;
	ASSERT(param);
	ASSERT(!param->next);
	opt_dest = param->arg;
	param->arg = 0;
    }

    if ( output_file_type == OFT_UNKNOWN && allow_fst && IsFST(opt_dest,0) )
	output_file_type = OFT_FST;
    if ( output_file_type == OFT_FST && !allow_fst )
	output_file_type = OFT_UNKNOWN;

    if ( prefix_mode <= WD_IPM_AUTO )
	prefix_mode = WD_IPM_PART_NAME;

    FilePattern_t * pat = GetDefaultFilePattern();
    if (SetupFilePattern(pat))
	encoding |= ENCODE_F_FAST; // hint: no encryption needed

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    Iterator_t it;
    InitializeIterator(&it);
    it.act_non_iso	= used_options & OB_IGNORE ? ACT_IGNORE : ACT_WARN;
    it.act_non_exist	= it.act_non_iso;
    it.act_wbfs		= ACT_EXPAND;
    it.act_fst		= allow_fst ? ACT_EXPAND : ACT_IGNORE;
    it.long_count	= long_count;

    if ( testmode > 1 )
    {
	it.func = exec_filetype;
	enumError err = SourceIterator(&it,1,false,false);
	ResetIterator(&it);
	printf("DESTINATION: %s\n",opt_dest);
	return err;
    }

    it.func = exec_diff;
    enumError err = SourceIterator(&it,2,false,false);
    if ( err == ERR_OK && it.diff_count )
	err = ERR_DIFFER;
    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_extract ( SuperFile_t * fi, Iterator_t * it )
{
    ASSERT(fi);

    if (!fi->f.id6[0])
	return ERR_OK;
    fflush(0);

    wd_disc_t * disc = OpenDiscSF(fi,true,true);
    if (!disc)
	return ERR_WDISC_NOT_FOUND;

    char dest_path[PATH_MAX];
    SubstFileNameBuf(dest_path,sizeof(dest_path)-1,fi,0,opt_dest,OFT_UNKNOWN);
    size_t dest_path_len = strlen(dest_path);
    if ( !dest_path_len || dest_path[dest_path_len-1] != '/' )
    {
	dest_path[dest_path_len++] = '/';
	dest_path[dest_path_len] = 0;
    }
   
    if (!it->overwrite)
    {
	struct stat st;
	if (!stat(dest_path,&st))
	{
	    ERROR0(ERR_ALREADY_EXISTS,"Destination already exists: %s",dest_path);
	    return 0;
	}
    }

    if ( testmode || verbose >= 0 )
    {
	printf( "%s: %sEXTRACT %s:%s -> %s\n",
		progname, testmode ? "WOULD " : "",
		oft_name[fi->iod.oft], fi->f.fname, dest_path );
	if (testmode)
	    return ERR_OK;
    }

    fi->indent		= 5;
    fi->show_progress	= verbose > 1 || progress;
    fi->show_summary	= verbose > 0 || progress;
    fi->show_msec	= verbose > 2;

    WiiFst_t fst;
    InitializeFST(&fst);
    CollectFST(&fst,disc,GetDefaultFilePattern(),false,prefix_mode,false);
    SortFST(&fst,sort_mode,SORT_OFFSET);

    WiiFstInfo_t wfi;
    memset(&wfi,0,sizeof(wfi));
    wfi.sf		= fi;
    wfi.fst		= &fst;
    wfi.set_time	= used_options & OB_PRESERVE ? &fi->f.fatt : 0;;
    wfi.overwrite	= it->overwrite;
    wfi.verbose		= long_count > 0 ? long_count : verbose > 0 ? 1 : 0;

    enumError err = CreateFST(&wfi,dest_path);

    if ( !err && wfi.not_created_count )
    {	
	if ( wfi.not_created_count == 1 )
	    err = ERROR0(ERR_CANT_CREATE,
			"1 file or directory not created\n" );
	else
	    err = ERROR0(ERR_CANT_CREATE,
			"%d files and/or directories not created.\n",
			wfi.not_created_count );
    }
    ResetFST(&fst);
    return err;
}

//-----------------------------------------------------------------------------

enumError cmd_extract()
{
    if ( verbose >= 0 )
	print_title(stdout);

    if (!opt_dest)
    {
	if (!first_param)
	    return ERROR0(ERR_MISSING_PARAM, "Missing destination parameter\n" );

	ParamList_t * param;
	for ( param = first_param; param->next; param = param->next )
	    ;
	ASSERT(param);
	ASSERT(!param->next);
	opt_dest = param->arg;
	param->arg = 0;
    }

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    encoding |= ENCODE_F_FAST; // hint: no encryption needed

    Iterator_t it;
    InitializeIterator(&it);
    it.act_non_iso	= used_options & OB_IGNORE ? ACT_IGNORE : ACT_WARN;
    it.act_wbfs		= ACT_EXPAND;
    it.act_fst		= allow_fst ? ACT_EXPAND : ACT_IGNORE;
    it.overwrite	= used_options & OB_OVERWRITE ? 1 : 0;

    enumError err = SourceIterator(&it,0,false,true);
    if ( err <= ERR_WARNING )
    {
	it.func = exec_extract;
	err = SourceIteratorCollected(&it,2);
	if ( err == ERR_OK && it.exists_count )
	    err = ERR_ALREADY_EXISTS;
    }
    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_copy ( SuperFile_t * fi, Iterator_t * it )
{
    if (!fi->f.id6[0])
	return ERR_OK;
    fflush(0);

    const bool scrub_it = it->scrub_it
		&& ( output_file_type == OFT_UNKNOWN || output_file_type == GetOFT(fi) );

    ccp fname = fi->f.path ? fi->f.path : fi->f.fname;
    const enumOFT oft = scrub_it
			? GetOFT(fi)
			: CalcOFT(output_file_type,opt_dest,fname,OFT__DEFAULT);

    SuperFile_t fo;
    InitializeSF(&fo);
    SetupIOD(&fo,oft,oft);

    if (scrub_it)
	fo.f.fname = strdup(fname);
    else
    {
	TRACE("COPY, mkdir=%d\n",opt_mkdir);
	fo.f.create_directory = opt_mkdir;
	ccp oname = oft == OFT_WBFS && fi->f.id6[0]
			? fi->f.id6
			: fi->f.outname
				? fi->f.outname
				: fname;
	GenImageFileName(&fo.f,opt_dest,oname,oft);
	SubstFileNameSF(&fo,fi,0);

	if ( it->update && !StatFile(&fo.f.st,fo.f.fname,-1) )
	    return ERR_OK;
    }

    fo.indent		= 5;
    fo.show_progress	= verbose > 1 || progress;
    fo.show_summary	= verbose > 0 || progress;
    fo.show_msec	= verbose > 2;

    char count_buf[100];
    snprintf(count_buf,sizeof(count_buf), "%u", it->source_list.used );
    snprintf(count_buf,sizeof(count_buf), "%*u/%u",
		(int)strlen(count_buf), it->source_index+1, it->source_list.used );

    const bool raw_mode = part_selector & WD_SEL_WHOLE_DISC || !fi->f.id6[0];
    if (testmode)
    {
	if (scrub_it)
	    printf( "%s: WOULD %s %s %s:%s\n",
		progname, raw_mode ? "COPY " : "SCRUB",
		count_buf, oft_name[oft], fi->f.fname );
	else
	    printf( "%s: WOULD %s %s %s:%s -> %s:%s\n",
			progname, raw_mode ? "COPY " : "SCRUB", count_buf,
			oft_name[fi->iod.oft], fi->f.fname,
			oft_name[oft], fo.f.fname );
	ResetSF(&fo,0);
	return ERR_OK;
    }

    if ( verbose >= 0 )
    {
	if (scrub_it)
	    printf( "* %s %s %s %s %s\n",
		progname, raw_mode ? "COPY " : "SCRUB",
		count_buf, oft_name[oft], fi->f.fname );
	else
	    printf( "* %s %s %s %s:%s -> %s:%s\n",
			progname, raw_mode ? "COPY " : "SCRUB", count_buf,
		    oft_name[fi->iod.oft], fi->f.fname,
		    oft_name[oft], fo.f.fname );
    }

    enumError err = CreateFile( &fo.f, 0, IOM_IS_IMAGE, it->overwrite );
    if ( err == ERR_ALREADY_EXISTS )
    {
	it->exists_count++;
	err = ERR_OK;
	goto abort;
    }
    if (err)
	goto abort;

    if (opt_split)
	SetupSplitFile(&fo.f,oft,opt_split_size);

    err = SetupWriteSF(&fo,oft);
    if (err)
	goto abort;

    err = CopySF( fi, &fo, raw_mode ? WD_SEL_WHOLE_DISC : part_selector );
    if (err)
	goto abort;

    err = RewriteModifiedSF(fi,&fo,0);
    if (err)
	goto abort;
    
    if (it->remove_source)
	RemoveSF(fi);

    return ResetSF( &fo, used_options & OB_PRESERVE ? &fi->f.fatt : 0 );

 abort:
    RemoveSF(&fo);
    return err;
}

//-----------------------------------------------------------------------------

enumError cmd_copy()
{
    if ( used_options & OB_FST )
    {
	enumError cmd_extract();
	return cmd_extract();
    }

    if ( verbose >= 0 )
	print_title(stdout);

    if (!opt_dest)
    {
	if (!first_param)
	    return ERROR0(ERR_MISSING_PARAM, "Missing destination parameter\n" );

	ParamList_t * param;
	for ( param = first_param; param->next; param = param->next )
	    ;
	ASSERT(param);
	ASSERT(!param->next);
	opt_dest = param->arg;
	param->arg = 0;
    }

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    Iterator_t it;
    InitializeIterator(&it);
    it.act_non_iso	= used_options & OB_IGNORE ? ACT_IGNORE : ACT_WARN;
    it.act_wbfs		= ACT_EXPAND;
    it.act_fst		= allow_fst ? ACT_EXPAND : ACT_IGNORE;
    it.overwrite	= used_options & OB_OVERWRITE ? 1 : 0;
    it.update		= used_options & OB_UPDATE ? 1 : 0;
    it.remove_source	= used_options & OB_REMOVE ? 1 : 0;

    if ( testmode > 1 )
    {
	it.func = exec_filetype;
	enumError err = SourceIterator(&it,1,false,false);
	ResetIterator(&it);
	printf("DESTINATION: %s\n",opt_dest);
	return err;
    }

    enumError err = SourceIterator(&it,0,false,true);
    if ( err <= ERR_WARNING )
    {
	it.func = exec_copy;
	err = SourceIteratorCollected(&it,2);
	if ( err == ERR_OK && it.exists_count )
	    err = ERR_ALREADY_EXISTS;
    }
    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_scrub()
{
    if ( verbose >= 0 )
	print_title(stdout);

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    Iterator_t it;
    InitializeIterator(&it);
    it.act_non_iso	= used_options & OB_IGNORE ? ACT_IGNORE : ACT_WARN;
    it.act_wbfs		= it.act_non_iso;
    it.scrub_it		= true;
    it.overwrite	= true;
    it.remove_source	= true;

    if ( testmode > 1 )
    {
	it.func = exec_filetype;
	enumError err = SourceIterator(&it,1,false,false);
	ResetIterator(&it);
	printf("DESTINATION: %s\n",opt_dest);
	return err;
    }

    enumError err = SourceIterator(&it,0,false,true);
    if ( err <= ERR_WARNING )
    {
	it.func = exec_copy;
	err = SourceIteratorCollected(&it,2);
	if ( err == ERR_OK && it.exists_count )
	    err = ERR_ALREADY_EXISTS;
    }
    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_edit ( SuperFile_t * fi, Iterator_t * it )
{
    if (!fi->f.id6[0])
	return ERR_OK;
    fflush(0);

    if (testmode)
    {
	printf( "%s: WOULD EDIT %s:%s\n", progname, oft_name[fi->iod.oft], fi->f.fname );
	return ERR_OK;
    }

    if ( verbose >= 0 )
	printf( "%s: EDIT %s:%s\n", progname, oft_name[fi->iod.oft], fi->f.fname );

    OpenDiscSF(fi,true,true);
    wd_disc_t * disc = fi->disc1;
    
    enumError err = ERR_OK;
    if ( disc && disc->reloc )
    {
	const wd_reloc_t * reloc = disc->reloc;
	int idx;
	for ( idx = 0; idx < WII_MAX_SECTORS && !err; idx++, reloc++ )
	    if ( ( *reloc & (WD_RELOC_F_PATCH|WD_RELOC_F_CLOSE)) == WD_RELOC_F_PATCH )
		err = CopyRawData(fi,fi,idx*WII_SECTOR_SIZE,WII_SECTOR_SIZE);

	reloc = disc->reloc;
	for ( idx = 0; idx < WII_MAX_SECTORS && !err; idx++, reloc++ )
	    if ( *reloc & WD_RELOC_F_CLOSE )
		err = CopyRawData(fi,fi,idx*WII_SECTOR_SIZE,WII_SECTOR_SIZE);
    }

    ResetSF( fi, !err && used_options & OB_PRESERVE ? &fi->f.fatt : 0 );
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError cmd_edit()
{
    if ( verbose >= 0 )
	print_title(stdout);

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    Iterator_t it;
    InitializeIterator(&it);
    it.act_non_iso	= used_options & OB_IGNORE ? ACT_IGNORE : ACT_WARN;
    it.act_wbfs		= it.act_non_iso;

    if ( testmode > 1 )
    {
	it.func = exec_filetype;
	enumError err = SourceIterator(&it,1,false,false);
	ResetIterator(&it);
	printf("DESTINATION: %s\n",opt_dest);
	return err;
    }

    enumError err = SourceIterator(&it,0,false,true);
    if ( err <= ERR_WARNING )
    {
	it.func = exec_edit;
	it.open_modify = !testmode;
	err = SourceIteratorCollected(&it,2);
	if ( err == ERR_OK && it.exists_count )
	    err = ERR_ALREADY_EXISTS;
    }
    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_move ( SuperFile_t * fi, Iterator_t * it )
{
    SuperFile_t fo;
    InitializeSF(&fo);
    SetupIOD(&fo,fi->iod.oft,fi->iod.oft);
    fo.f.create_directory = opt_mkdir;

    ccp oname = fi->iod.oft == OFT_WBFS && fi->f.id6[0]
			? fi->f.id6
			: fi->f.outname
				? fi->f.outname
				: fi->f.fname;
    GenImageFileName(&fo.f,opt_dest,oname,fi->iod.oft);
    SubstFileNameSF(&fo,fi,0);

    if ( strcmp(fi->f.fname,fo.f.fname) )
    {
	if ( !it->overwrite && !stat(fo.f.fname,&fo.f.st) )
	{
	    ERROR0(ERR_CANT_CREATE,"File already exists: %s\n",fo.f.fname);
	}
	else if (!CheckCreated(fo.f.fname,false))
	{
	    if ( testmode || verbose >= 0 )
	    {
		snprintf(iobuf,sizeof(iobuf), "%u", it->source_list.used );
		printf(" - %sMove %*u/%u %s:%s -> %s\n",
		    testmode ? "WOULD " : "",
		    (int)strlen(iobuf), it->source_index+1, it->source_list.used,
		    oft_name[fo.iod.oft], fi->f.fname, fo.f.fname );
	    }

	    if ( !testmode && rename(fi->f.fname,fo.f.fname) )
		return ERROR1( ERR_CANT_CREATE, "Can't create file: %s\n", fo.f.fname );
	}
    }

    ResetSF(&fo,0);
    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError cmd_move()
{
    if ( verbose >= 0 )
	print_title(stdout);

    if (!opt_dest)
    {
	if (!first_param)
	    return ERROR0(ERR_MISSING_PARAM, "Missing destination parameter\n" );

	ParamList_t * param;
	for ( param = first_param; param->next; param = param->next )
	    ;
	ASSERT(param);
	ASSERT(!param->next);
	opt_dest = param->arg;
	param->arg = 0;
    }

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    encoding |= ENCODE_F_FAST; // hint: no encryption needed

    Iterator_t it;
    InitializeIterator(&it);
    it.act_non_iso	= used_options & OB_IGNORE ? ACT_IGNORE : ACT_WARN;
    it.act_wbfs		= ACT_ALLOW;
    it.overwrite	= used_options & OB_OVERWRITE ? 1 : 0;

    if ( testmode > 1 )
    {
	it.func = exec_filetype;
	enumError err = SourceIterator(&it,1,false,false);
	ResetIterator(&it);
	printf("DESTINATION: %s\n",opt_dest);
	return err;
    }

    enumError err = SourceIterator(&it,0,false,true);
    if ( err <= ERR_WARNING )
    {
	it.func = exec_move;
	err = SourceIteratorCollected(&it,2);
    }
    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_rename ( SuperFile_t * fi, Iterator_t * it )
{
    ParamList_t *param, *plus_param = 0;
    for ( param = first_param; param; param = param->next )
    {
	if ( !plus_param && param->selector[0] == '+' )
	    plus_param = param;
	else if ( !strcmp(param->selector,fi->f.id6) )
	    break;
    }

    if (!param)
    {
	if (!plus_param)
	    return ERR_OK;
	param = plus_param;
    }
    param->count++;

    bool change_wbfs	= 0 != ( used_options & OB_WBFS );
    bool change_iso	= 0 != ( used_options & OB_ISO );
    if ( !change_wbfs && !change_iso )
	change_wbfs = change_iso = true;

    if ( fi->iod.oft == OFT_WBFS )
    {
	ASSERT(fi->wbfs);
	return RenameWDisc( fi->wbfs, param->id6, param->arg,
			    change_wbfs, change_iso, verbose, testmode );
    }

    const int bufsize = WII_TITLE_OFF + WII_TITLE_SIZE;
    char buf[bufsize];

    enumError err = ReadSF(fi,0,buf,sizeof(buf));
    if (err)
	return err;

    if (RenameISOHeader( buf, fi->f.fname,
			 param->id6, param->arg, verbose, testmode ))
    {
	err = WriteSF(fi,0,buf,sizeof(buf));
	if (err)
	    return err;
    }
    return err;
}

//-----------------------------------------------------------------------------

enumError cmd_rename ( bool rename_id )
{
    if ( verbose >= 0 )
	print_title(stdout);

    if ( !source_list.used && !recurse_list.used )
	return ERROR0( ERR_MISSING_PARAM, "Missing source files.\n");

    enumError err = CheckParamRename(rename_id,!rename_id,false);
    if (err)
	return err;
    if ( !n_param )
	return ERROR0(ERR_MISSING_PARAM, "Missing renaming parameters.\n" );

    Iterator_t it;
    InitializeIterator(&it);
    it.open_modify	= !testmode;
    it.act_non_iso	= used_options & OB_IGNORE ? ACT_IGNORE : ACT_WARN;
    it.act_wbfs		= ACT_EXPAND;
    it.long_count	= long_count;

    if ( testmode > 1 )
    {
	it.func = exec_filetype;
	enumError err = SourceIterator(&it,1,false,false);
	ResetIterator(&it);
	printf("DESTINATION: %s\n",opt_dest);
	return err;
    }

    it.func = exec_rename;
    err = SourceIterator(&it,0,false,false);
    ResetIterator(&it);
    return err;

    // [2do]: not found
}

//
///////////////////////////////////////////////////////////////////////////////

enumError exec_verify ( SuperFile_t * fi, Iterator_t * it )
{
    ASSERT(fi);
    ASSERT(it);
    if (!fi->f.id6[0])
	return ERR_OK;
    fflush(0);

    it->done_count++;
    if ( testmode || verbose >= 999 )
    {
	printf( "%s: %sVERIFY %s:%s\n",
		progname, testmode ? "WOULD " : "",
		oft_name[fi->iod.oft], fi->f.fname );
	if (testmode)
	    return ERR_OK;
    }

    fi->indent		= 5;
    fi->show_progress	= verbose > 1 || progress;
    fi->show_summary	= verbose > 0 || progress;
    fi->show_msec	= verbose > 2;

    Verify_t ver;
    InitializeVerify(&ver,fi);
    ver.long_count = it->long_count;
    if ( opt_limit >= 0 )
    {
	ver.max_err_msg = opt_limit;
	if (!ver.verbose)
	    ver.verbose = 1;
    }
    if ( it->source_list.used > 1 )
    {
	ver.disc_index = it->source_index+1;
	ver.disc_total = it->source_list.used;
    }

    const enumError err = VerifyDisc(&ver);
    if ( err == ERR_DIFFER )
        it->diff_count++;
    ResetVerify(&ver);
    return err;
}

//-----------------------------------------------------------------------------

enumError cmd_verify()
{
    if ( verbose >= 0 )
	print_title(stdout);

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    Iterator_t it;
    InitializeIterator(&it);
    it.act_non_iso	= used_options & OB_IGNORE ? ACT_IGNORE : ACT_WARN;
    it.act_wbfs		= ACT_EXPAND;
    it.act_fst		= allow_fst ? ACT_EXPAND : ACT_IGNORE;
    it.long_count	= long_count;

    if ( testmode > 1 )
    {
	it.func = exec_filetype;
	enumError err = SourceIterator(&it,1,false,false);
	ResetIterator(&it);
	printf("DESTINATION: %s\n",opt_dest);
	return err;
    }

    enumError err = SourceIterator(&it,0,false,true);
    if ( err <= ERR_WARNING )
    {
	it.func = exec_verify;
	err = SourceIteratorCollected(&it,2);
	if ( err == ERR_OK && it.diff_count )
	    err = ERR_DIFFER;
    }
    ResetIterator(&it);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

typedef struct Mix_t
{
    //--- input data

    ccp		source;		// pointer to source file ( := param->arg )
    u32		ptab;		// destination partition tables
    u32		ptype;		// destination partition type

    //--- secondary data
    
    SuperFile_t	* sf;		// superfile of iso image
    bool	close_sf;	// true: close supefile if done
    wd_disc_t	* disc;		// valid disc pointer
    wd_part_t	* part;		// valid partition pointer

    u32		src_sector;	// index of first source sector
    u32		first_sector;	// index of first destination sector

    //--- Permutation helpers

    u32		a1size;		// size of area 1 (p1off allways = 0)
    u32		a2off;		// offset of area 2
    u32		a2size;		// size of area 2
    
} Mix_t;

//-----------------------------------------------------------------------------

typedef struct MixFree_t
{
	u32 off;		// offset of first free sector
	u32 size;		// number of free sectors

} MixFree_t;


//-----------------------------------------------------------------------------

#define MAX_MIX_PERM 12

typedef struct MixParam_t
{
    Mix_t	* mix;		// valid pointer to table
    int		n_mix;		// number of valid elements of 'mix'

    int		used[MAX_MIX_PERM];
    MixFree_t	field[MAX_MIX_PERM][2*MAX_MIX_PERM+2];

    u32		delta[MAX_MIX_PERM];
    u32		max_end;

} MixParam_t;

//-----------------------------------------------------------------------------

static u32 mark_used_mix
(
    MixFree_t	* base,		// pointer to free field
    u32		used,		// number of used elements in 'base'
    MixFree_t	* ptr,		// current pointer into 'base'
    u32		off,		// offset relative to ptr->off
    u32		size		// size to remove
) 
{
    DASSERT(base);
    DASSERT(used);
    DASSERT(ptr);
    DASSERT(size);
    DASSERT( off + size <= ptr->size );

    if (!off)
    {
	PRINT("\t\t\t\t\t> %x..%x -> %x..%x [%u/%u]\n",
		ptr->off, ptr->off + ptr->size,
		ptr->off + size, ptr->off + ptr->size,
		ptr - base, used );
	ptr->off  += size;
	ptr->size -= size;
	if (!ptr->size)
	{
	    used--;
	    const u32 index = ptr - base;
	    memmove(ptr,ptr+1,(used-index)*sizeof(*ptr));
	}
	return used;
    }

    const u32 end = off + size;
    if ( end == ptr->size )
    {
	PRINT("\t\t\t\t\t> %x..%x -> %x..%x [%u/%u]\n",
		ptr->off, ptr->off + ptr->size,
		ptr->off, ptr->off + ptr->size - size,
		ptr - base, used );
	ptr->size -= size;
	return used;
    }

    // split entry
    const u32 old_off  = ptr->off;
    const u32 old_size = ptr->size;

    const u32 index = ptr - base;
    memmove(ptr+1,ptr,(used-index)*sizeof(*ptr));

    ptr[0].off  = old_off;
    ptr[0].size = off;
    ptr[1].off  = old_off + end;
    ptr[1].size = old_size - end;
    PRINT("\t\t\t\t\t> %x..%x -> %x..%x,%x..%x [%u/%u>%u]\n",
		old_off, old_off + old_size,
		ptr[0].off, ptr[0].off + ptr[0].size,
		ptr[1].off, ptr[1].off + ptr[1].size,
		ptr - base, used, used+1 );
    return used+1;
}

//-----------------------------------------------------------------------------

static void insert_mix ( MixParam_t * p, int pdepth, int mix_index )
{
    DASSERT( p );
    DASSERT( pdepth >= 0 && pdepth < p->n_mix );
    DASSERT( mix_index >= 0 && mix_index < p->n_mix );
    
    MixFree_t * f = p->field[pdepth];
    Mix_t * mix = p->mix + mix_index;

    if (!pdepth)
    {
	// setup free table
	PRINT("\t\t\t\t\t> SETUP\n");
	const u32 start = WII_GOOD_UPDATE_PART_OFF / WII_SECTOR_SIZE;
	p->delta[mix_index] = start;
	if (mix->a2off)
	{
	    p->used[0]	= 2;
	    f[0].off	= start + mix->a1size;
	    f[0].size	= mix->a2off - mix->a1size;
	    f[1].off	= start + mix->a2off + mix->a2size;
	    f[1].size	= ~(u32)0 - f[1].off;
	}
	else
	{
	    p->used[0]	= 1;
	    f[0].off	= start + mix->a1size;
	    f[0].size	= ~(u32)0 - f[0].off;
	}
    }
    else
    {
	u32 used = p->used[pdepth-1];
	memcpy(f,p->field[pdepth-1],sizeof(*f)*used);

	MixFree_t * f1ptr = f;
	MixFree_t * fend = f1ptr + used;
	for ( ; f1ptr < fend; f1ptr++ )
	{
	    if ( f1ptr->size < mix->a1size )
		continue;

	    if (!mix->a2off)
	    {
		p->delta[mix_index] = f1ptr->off;
		used = mark_used_mix(f,used,f1ptr,0,mix->a1size);
		break;
	    }

	    const u32 base  = f1ptr->off;
	    const u32 space = f1ptr->size - mix->a1size;
	    MixFree_t * f2ptr;
	    for ( f2ptr = f1ptr; f2ptr < fend; f2ptr++ )
	    {
		u32 delta = 0;
		u32 off = base + mix->a2off;
		if ( f2ptr->off > off && f2ptr->off <= off + space )
		{
		    delta = f2ptr->off - off;
		    off += delta;
		}

		if ( off >= f2ptr->off && off + mix->a2size <= f2ptr->off + f2ptr->size )
		{
		    p->delta[mix_index] = f1ptr->off + delta;
		    used = mark_used_mix(f,used,f2ptr,off-f2ptr->off,mix->a2size);
		    used = mark_used_mix(f,used,f1ptr,delta,mix->a1size);
		    f1ptr = fend;
		    break;
		}
	    }
	}

	p->used[pdepth] = used;
    }

 #if defined(TEST) && defined(DEBUG)
    {
	int i;
	for ( i = 0; i < p->used[pdepth]; i++ )
	    printf(" %5x .. %8x / %8x\n",
		f[i].off, f[i].off + f[i].size, f[i].size ); 
    }
 #endif
}
 
//-----------------------------------------------------------------------------

static void permutate_mix ( MixParam_t * p )
{
    PRINT("PERMUTATE MIX\n");

    const int n_perm = p->n_mix;
    DASSERT( n_perm <= MAX_MIX_PERM );

    u8 used[MAX_MIX_PERM+1];	// mark source index as used
    memset(used,0,sizeof(used));
    u8 source[MAX_MIX_PERM];	// field with source index into 'mix'
    source[0] = 0;
    int pdepth = 0;
    p->max_end = ~(u32)0;

    u32	delta[MAX_MIX_PERM];
    memset(delta,0,sizeof(delta));

    for(;;)
    {
	u8 src = source[pdepth];
	if ( src > 0 )
	    used[src] = 0;
	src++;
	while ( src <= n_perm && used[src] )
	    src++;
	if ( src > n_perm )
	{
	    if ( --pdepth >= 0 )
		continue;
	    break;
	}

	//PRINT(" src=%d/%d, depth=%d\n",src,n_perm,pdepth);
	DASSERT( src <= n_perm );
	used[src] = 1;
	source[pdepth] = src;
	insert_mix(p,pdepth++,src-1);

	// [2do] : insert part

	if ( pdepth < n_perm )
	{
	    source[pdepth] = 0;
	    continue;
	}

	const u32 used = p->used[pdepth-1];
	const u32 new_end = p->field[pdepth-1][used-1].off;

     #ifdef TEST
	{
	    PRINT("PERM:");
	    int i;
	    for ( i = 0; i < n_perm; i++ )
		PRINT(" %u",source[i]);
	    PRINT(" -> %5x [%5x]%s\n",
		new_end, p->max_end, new_end < p->max_end ? " *" : "" );
	}
     #endif
     
	if ( new_end < p->max_end )
	{
	    p->max_end = new_end;
	    memcpy(delta,p->delta,sizeof(delta));
	}
    }
    
    int i;
    for ( i = 0; i < n_perm; i++ )
	p->mix[i].first_sector = delta[i];
}

//-----------------------------------------------------------------------------

enumError cmd_mix()
{
    opt_hook = -1; // disable hooked discs (no patching/relocation)
 
    if ( verbose >= 0 )
	print_title(stdout);


    //----- check dest option

    if ( !opt_dest || !*opt_dest )
    {
	if (!testmode)
	    return ERROR0(ERR_SEMANTIC,
			"Non empty option --dest/--DEST required.\n");
	opt_dest = "./";
    }


    //----- scan parameters

    int n_mix = 0;
    Mix_t mixtab[WII_MAX_PARTITIONS];
    memset(mixtab,0,sizeof(mixtab));
    TRACE_SIZEOF(Mix_t);
    TRACE_SIZEOF(mixtab);
    TRACE_SIZEOF(MixParam_t);

    const u32 MAX_PART	= OptionUsed[OPT_OVERLAY]
			? MAX_MIX_PERM
			: WII_MAX_PARTITIONS;

    u32 sector = WII_GOOD_UPDATE_PART_OFF / WII_SECTOR_SIZE;
    u32 ptab_count[WII_MAX_PTAB];

    bool src_warn = true;
    ParamList_t * param = first_param;
    while (param)
    {
	//--- scan source file

	ccp srcfile = param->arg;
	if ( src_warn && !strchr(srcfile,'/') && !strchr(srcfile,'.') )
	{
	    src_warn = false;
	    ERROR0(ERR_WARNING,
		"Warning: use at least one '.' or '/' in a filename"
		" to distinguish file names from keywords: %s", srcfile );
	}
	param = param->next;


	//--- scan 'SELECT'

	wd_select_t psel = WD_SEL_PART_DATA|WD_SEL_PART_ACTIVE;
	if ( param && param->next
	     && (  !strcasecmp(param->arg,"select")
		|| !strcasecmp(param->arg,"psel") ))
	{
	    param = param->next;
	    psel = ScanPartSelector(param->arg," ('select')");
	    if ( psel == -(wd_select_t)1 )
		return ERR_SYNTAX;
	    param = param->next;
	}


	//--- scan 'AS'

	u32 ptab = 0, ptype = 0;
	bool ptab_valid = false, ptype_valid = false;
	if ( param && param->next && !strcasecmp(param->arg,"as") )
	{
	    param = param->next;
	    const enumError err
		= ScanPartTabAndType(&ptab,&ptab_valid,&ptype,&ptype_valid,
					param->arg," ('as')");
	    if (err)
		return err;
	    param = param->next;
	}
	PRINT("psel=%llx, as=%u.%u,%d.%x, src=%s\n",
		psel, ptab_valid,ptype_valid,ptab,ptype,srcfile);


	//--- open disc and partitions

	SuperFile_t * sf = AllocSF();
	ASSERT(sf);
	enumError err = OpenSF(sf,srcfile,false,false);
	if (err)
	    return err;

	wd_disc_t * disc = OpenDiscSF(sf,false,true);
	if (!disc)
	    return ERR_WDISC_NOT_FOUND;

	wd_select(disc,psel);
	wd_part_t *part, *end_part = disc->part + disc->n_part;
	Mix_t * mix = 0;
	for ( part = disc->part; part < end_part; part++ )
	{
	    if (!part->is_enabled)
		continue;
	    err = wd_load_part(part,false,false);
	    if (err)
		return err;

	    mix = mixtab + n_mix++;
	    if ( n_mix > MAX_PART )
		return ERROR0(ERR_SEMANTIC,
		    "Maximum supported partition count (%u) exceeded.\n",
		    MAX_PART );

	    mix->source		= srcfile;
	    mix->ptab		= ptab_valid ? ptab : part->ptab_index;
	    mix->ptype		= ptype_valid ? ptype : part->part_type;
	    mix->sf		= sf;
	    mix->disc		= disc;
	    mix->part		= part;
	    mix->first_sector	= sector;
	    mix->src_sector	= part->part_off4 / WII_SECTOR_SIZE4;
	    sector		+= part->end_sector - mix->src_sector;
	    if ( sector & 1 )
		sector++; // align to 64KiB
	    ptab_count[ptab]++;


	    //--- find hole

	    u8 * utab = wd_calc_usage_table(mix->disc);
	    wd_usage_t usage_id = part->usage_id;
	    u32 src_sector = mix->src_sector;

	    while ( src_sector < part->end_sector
		    && ( utab[src_sector] & WD_USAGE__MASK ) == usage_id )
		src_sector++;
	    mix->a1size = mix->a2size = src_sector;
	    u32 max_hole = 0;

	    while ( src_sector < part->end_sector
		    && ( utab[src_sector] & WD_USAGE__MASK ) != usage_id )
		src_sector++;
	    u32 start_sector = src_sector;

	    while ( src_sector < part->end_sector )
	    {
		while ( src_sector < part->end_sector
			&& ( utab[src_sector] & WD_USAGE__MASK ) == usage_id )
		    src_sector++;
		u32 end_sector = src_sector;

		while ( src_sector < part->end_sector
			&& ( utab[src_sector] & WD_USAGE__MASK ) != usage_id )
		    src_sector++;

		if ( end_sector > start_sector )
		{
		    u32 this_hole = start_sector - mix->a2size;
		    if ( this_hole > max_hole )
		    {
			max_hole     = this_hole;
			mix->a1size   = mix->a2size;
			mix->a2off = start_sector;
			start_sector = src_sector;
		    }
		    mix->a2size = end_sector;
		}
	    }

	    mix->a1size -= mix->src_sector;
	    if (mix->a2off)
	    {
		mix->a2size -= mix->a2off;
		mix->a2off  -= mix->src_sector;
	    }
	    else
		mix->a2size = 0;
	}

	if (!mix)
	    return ERROR0(ERR_SEMANTIC,"No partition selected: %s\n",srcfile);
	mix->close_sf = true;
    }

    if (!n_mix)
	return ERROR0(ERR_SEMANTIC,"No source partition selected.\n");

    //----- permutate

    if ( OptionUsed[OPT_OVERLAY] )
    {
	MixParam_t p;
	memset(&p,0,sizeof(p));
	p.mix = mixtab;
	p.n_mix = n_mix;
	permutate_mix(&p);
	sector = p.max_end;
    }


    //----- check max sectors

    if ( sector > WII_MAX_SECTORS )
	return ERROR0(ERR_SEMANTIC,
		"Total size (%u MiB) exceeds maximum size (%u MiB).\n",
		sector / WII_SECTORS_PER_MIB,
		WII_MAX_SECTORS / WII_SECTORS_PER_MIB );


    //----- print log table

    Mix_t *mix, *end_mix = mixtab + n_mix;

    if ( testmode || verbose > 1 )
    {
	int src_fw = 7;
	for ( mix = mixtab; mix < end_mix; mix++ )
	{
	    const int len = strlen(mix->source);
	    if ( src_fw < len )
		 src_fw = len;
	}

	printf("\nMix table (%d partitons, total size=%llu MiB):\n\n"
		"    blocks #1      blocks #2  :      disc offset     : ptab  ptype : source\n"
		"  %.*s\n",
		n_mix, sector* (u64)WII_SECTOR_SIZE / MiB,
		67 + src_fw, wd_sep_200 );

	for ( mix = mixtab; mix < end_mix; mix++ )
	{
	    if (mix->a2off)
	    {
		const u32 end_sector = mix->first_sector + mix->a2off + mix->a2size;
		printf("%7x..%5x + %5x..%5x : %9llx..%9llx : %u %s : %s\n",
		    mix->first_sector,
		    mix->first_sector + mix->a1size,
		    mix->first_sector + mix->a2off,
		    end_sector,
		    mix->first_sector * (u64)WII_SECTOR_SIZE,
		    end_sector * (u64)WII_SECTOR_SIZE,
		    mix->ptab,
		    wd_print_part_name(0,0,mix->ptype,WD_PNAME_COLUMN_9),
		    mix->source );
	    }
	    else
	    {
		const u32 end_sector = mix->first_sector + mix->a1size;
		printf("%7x..%5x                : %9llx..%9llx : %u %s : %s\n",
		    mix->first_sector, end_sector,
		    mix->first_sector * (u64)WII_SECTOR_SIZE,
		    end_sector * (u64)WII_SECTOR_SIZE,
		    mix->ptab,
		    wd_print_part_name(0,0,mix->ptype,WD_PNAME_COLUMN_9),
		    mix->source );
	    }
	}
	putchar('\n');
    }


    //----- setup dhead

    char * dest = iobuf + sprintf(iobuf,"WIT mix of");
    ccp sep = " ";

    for ( mix = mixtab; mix < end_mix; mix++ )
    {
	dest += sprintf(dest,"%s%.6s",sep,&mix->part->boot.dhead.disc_id);
	sep = " + ";
    }
    
    wd_header_t dhead;
    header_setup(&dhead,modify_id,iobuf);


    //----- setup output file
    
    ccp destfile = IsDirectory(opt_dest,true) ? "a.wdf" : "";
    const enumOFT oft = CalcOFT(output_file_type,opt_dest,destfile,OFT__DEFAULT);
    SuperFile_t fo;
    InitializeSF(&fo);
    fo.f.create_directory = opt_mkdir;
    GenImageFileName(&fo.f,opt_dest,destfile,oft);
    SetupIOD(&fo,oft,oft);
    
    if ( testmode || verbose >= 0 )
	printf("\n%sreate [%.6s] %s:%s\n  (%s)\n\n",
		testmode ? "WOULD c" : "C",
		&dhead.disc_id, oft_name[oft],
		fo.f.fname, dhead.disc_title );


    //--- built memory map

    MemMap_t mm;
    InitializeMemMap(&mm);

    for ( mix = mixtab; mix < end_mix; mix++ )
    {
	u64 off  = mix->first_sector * (u64)WII_SECTOR_SIZE;
	u64 size = mix->a1size  * (u64)WII_SECTOR_SIZE;
	MemMapItem_t * item = InsertMemMap(&mm,off,size);
	DASSERT(item);
	item->index = mix - mixtab;
	snprintf(item->info,sizeof(item->info),
		    "partition #%-2u %s", mix - mixtab,
		    wd_print_part_name(0,0,mix->ptype,WD_PNAME_COLUMN_9) );

	if ( mix->a2off )
	{
	    off += mix->a2off * (u64)WII_SECTOR_SIZE;
	    size = mix->a2size  * (u64)WII_SECTOR_SIZE;
	    item = InsertMemMap(&mm,off,size);
	    DASSERT(item);
	    item->index = mix - mixtab | 0x80;
	    snprintf(item->info,sizeof(item->info),
			"partition #%-2u %s+", mix - mixtab,
			wd_print_part_name(0,0,mix->ptype,WD_PNAME_COLUMN_9) );
	}
    }

    if (logging)
    {
	printf("Parition layout of new mixed disc:\n\n");
	PrintMemMap(&mm,stdout,3);
	putchar('\n');
    }

    //----- execute

    enumError err = ERR_OK;

    if (!testmode)
    {
	//--- open file
	
	err = CreateFile( &fo.f, 0, IOM_IS_IMAGE,
				used_options & OB_OVERWRITE ? 1 : 0 );
	if (err)
	    goto abort;

	if (opt_split)
	    SetupSplitFile(&fo.f,oft,opt_split_size);

	err = SetupWriteSF(&fo,oft);
	if (err)
	    goto abort;

	err = SetMinSizeSF(&fo, WII_SECTORS_SINGLE_LAYER * (u64)WII_SECTOR_SIZE );
	if (err)
	    goto abort;


	//--- write disc header
	
	err = WriteSF(&fo,0,&dhead,sizeof(dhead));
	if (err)
	    goto abort;


	//--- write partition tables

	wd_ptab_t ptab;
	memset(&ptab,0,sizeof(ptab));
	{
	    wd_ptab_info_t  * info  = ptab.info;
	    wd_ptab_entry_t * entry = ptab.entry;
	    u32 off4 = (ccp)entry - (ccp)info + WII_PTAB_REF_OFF >> 2;

	    int it;
	    for ( it = 0; it < WII_MAX_PTAB; it++, info++ )
	    {
		int n_part = 0;
		for ( mix = mixtab; mix < end_mix; mix++ )
		{
		    if ( mix->ptab != it )
			continue;

		    n_part++;
		    entry->off4  = htonl(mix->first_sector*WII_SECTOR_SIZE4);
		    entry->ptype = htonl(mix->ptype);
		    entry++;
		}

		if (n_part)
		{
		    info->n_part = htonl(n_part);
		    info->off4   = htonl(off4);
		    off4 += n_part * sizeof(*entry) >> 2;
		}
	    }
	}

	err = WriteSF(&fo,WII_PTAB_REF_OFF,&ptab,sizeof(ptab));
	if (err)
	    goto abort;
	

	//--- write region settings

	wd_region_t reg;
	memset(&reg,0,sizeof(reg));
	reg.region = htonl( opt_region < REGION__AUTO
				? opt_region
				: GetRegionInfo(dhead.region_code)->reg );

	err = WriteSF(&fo,WII_REGION_OFF,&reg,sizeof(reg));
	if (err)
	    goto abort;


	//--- write magic2

	u32 magic2 = htonl(WII_MAGIC2);
	err = WriteSF(&fo,WII_MAGIC2_OFF,&magic2,sizeof(magic2));
	if (err)
	    goto abort;


	//--- copy partitions

	int mi;
	for ( mi = 0; mi < mm.used; mi++ )
	{
	    MemMapItem_t * item = mm.field[mi];
	    DASSERT(item);
	    u32 index = item->index & 0x7f;
	    DASSERT( index < n_mix );
	    mix = mixtab + index;
	    u64 src_off = ( mix->src_sector
				+ (item->index & 0x80 ? mix->a2off : 0 ))
			* (u64)WII_SECTOR_SIZE;
	    if ( verbose > 0 )
		printf(" - copy P.%-2u %9llx -> %9llx, size=%9llx\n",
		    index, src_off, (u64)item->off, (u64)item->size );

	    err = CopyRawData2(mix->sf,src_off,&fo,item->off,item->size);
	    if (err)
		goto abort;
	}
    }

 abort:
    if (err)
	RemoveSF(&fo);

    //--- clean up

    ResetMemMap(&mm);

    for ( mix = mixtab; mix < end_mix; mix++ )
	if (mix->close_sf)
	    FreeSF(mix->sf);

    return ResetSF(&fo,0);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   check options                 ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CheckOptions ( int argc, char ** argv, bool is_env )
{
    TRACE("CheckOptions(%d,%p,%d) optind=%d\n",argc,argv,is_env,optind);

    optind = 0;
    int err = 0;

    for(;;)
    {
      const int opt_stat = getopt_long(argc,argv,OptionShort,OptionLong,0);
      if ( opt_stat == -1 )
	break;

      RegisterOption(&InfoUI,opt_stat,1,is_env);

      switch ((enumGetOpt)opt_stat)
      {
	case GO__ERR:		err++; break;

	case GO_VERSION:	version_exit();
	case GO_HELP:		help_exit(false);
	case GO_XHELP:		help_exit(true);
	case GO_WIDTH:		err += ScanOptWidth(optarg); break;
	case GO_QUIET:		verbose = verbose > -1 ? -1 : verbose - 1; break;
	case GO_VERBOSE:	verbose = verbose <  0 ?  0 : verbose + 1; break;
	case GO_PROGRESS:	progress++; break;
	case GO_LOGGING:	logging++; break;
	case GO_ESC:		err += ScanEscapeChar(optarg) < 0; break;
	case GO_IO:		ScanIOMode(optarg); break;

	case GO_TITLES:		AtFileHelper(optarg,true,AddTitleFile); break;
	case GO_UTF_8:		use_utf8 = true; break;
	case GO_NO_UTF_8:	use_utf8 = false; break;
	case GO_LANG:		lang_info = optarg; break;

	case GO_TEST:		testmode++; break;

	case GO_SOURCE:		AppendStringField(&source_list,optarg,false); break;
	case GO_RECURSE:	AppendStringField(&recurse_list,optarg,false); break;

	case GO_INCLUDE:	AtFileHelper(optarg,0,AddIncludeID); break;
	case GO_INCLUDE_PATH:	AtFileHelper(optarg,0,AddIncludePath); break;
	case GO_EXCLUDE:	AtFileHelper(optarg,0,AddExcludeID); break;
	case GO_EXCLUDE_PATH:	AtFileHelper(optarg,0,AddExcludePath); break;
	case GO_IGNORE:		ignore_count++; break;
	case GO_IGNORE_FST:	allow_fst = false; break;

	case GO_PSEL:		err += ScanOptPartSelector(optarg); break;
	case GO_RAW:		part_selector = WD_SEL_WHOLE_DISC; break;
	case GO_SNEEK:		SetupSneekMode(); break;
	case GO_HOOK:		opt_hook = 1; break;
	case GO_ENC:		err += ScanOptEncoding(optarg); break;
	case GO_REGION:		err += ScanOptRegion(optarg); break;
	case GO_IOS:		err += ScanOptIOS(optarg); break;
	case GO_ID:		err += ScanOptId(optarg); break;
	case GO_NAME:		err += ScanOptName(optarg); break;
	case GO_MODIFY:		err += ScanOptModify(optarg); break;
	case GO_OVERLAY:	break;
	case GO_DEST:		opt_dest = optarg; break;
	case GO_DEST2:		opt_dest = optarg; opt_mkdir = true; break;
	case GO_SPLIT:		opt_split++; break;
	case GO_SPLIT_SIZE:	err += ScanOptSplitSize(optarg); break;
	case GO_TRUNC:		opt_truncate++; break;
	case GO_CHUNK_MODE:	err += ScanChunkMode(optarg); break;
	case GO_CHUNK_SIZE:	err += ScanChunkSize(optarg); break;
	case GO_MAX_CHUNKS:	err += ScanMaxChunks(optarg); break;
	case GO_PRESERVE:	break;
	case GO_UPDATE:		break;
	case GO_OVERWRITE:	break;
	case GO_REMOVE:		break;

	case GO_WDF:		output_file_type = OFT_WDF; break;
	case GO_ISO:		output_file_type = OFT_PLAIN; break;
	case GO_CISO:		output_file_type = OFT_CISO; break;
	case GO_WBFS:		output_file_type = OFT_WBFS; break;
	case GO_FST:		output_file_type = OFT_FST; break;

	case GO_ITIME:	    	SetTimeOpt(PT_USE_ITIME|PT_F_ITIME); break;
	case GO_MTIME:	    	SetTimeOpt(PT_USE_MTIME|PT_F_MTIME); break;
	case GO_CTIME:	    	SetTimeOpt(PT_USE_CTIME|PT_F_CTIME); break;
	case GO_ATIME:	    	SetTimeOpt(PT_USE_ATIME|PT_F_ATIME); break;

	case GO_LONG:		long_count++; break;
	case GO_UNIQUE:	    	break;
	case GO_NO_HEADER:	break;
	case GO_SECTIONS:	print_sections++; break;
	case GO_SHOW:		err += ScanOptShow(optarg); break;
	case GO_SORT:		err += ScanOptSort(optarg); break;

	case GO_RDEPTH:
	    if (ScanSizeOptU32(&opt_recurse_depth,optarg,1,0,
			    "rdepth",0,MAX_RECURSE_DEPTH,0,0,true))
		err++;
	    break;

	case GO_PMODE:
	    {
		const int new_pmode = ScanPrefixMode(optarg);
		if ( new_pmode == -1 )
		    err++;
		else
		    prefix_mode = new_pmode;
	    }
	    break;

	case GO_FILES:
	    if (AtFileHelper(optarg,PAT_OPT_FILES,AddFilePattern))
		err++;
	break;

	case GO_TIME:
	    if ( ScanAndSetPrintTimeMode(optarg) == PT__ERROR )
		err++;
	    break;

	case GO_LIMIT:
	    {
		u32 limit;
		if (ScanSizeOptU32(&limit,optarg,1,0,"limit",0,INT_MAX,0,0,true))
		    err++;
		else
		    opt_limit = limit;
	    }
	    break;

	// no default case defined
	//	=> compiler checks the existence of all enum values

      }
    }
    if ( used_options & OB_NO_HEADER )
	opt_show_mode &= ~SHOW_F_HEAD;

 #ifdef DEBUG
    DumpUsedOptions(&InfoUI,TRACE_FILE,11);
 #endif

    if ( verbose > 3 && !is_env )
    {
	print_title(stdout);
	printf("PROGRAM_NAME   = %s\n",progname);
	if (lang_info)
	    printf("LANG_INFO      = %s\n",lang_info);
	ccp * sp;
	for ( sp = search_path; *sp; sp++ )
	    printf("SEARCH_PATH[%td] = %s\n",sp-search_path,*sp);
	printf("\n");
    }

    return !err ? ERR_OK : max_error ? max_error : ERR_SYNTAX;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   check command                 ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CheckCommand ( int argc, char ** argv )
{
    TRACE("CheckCommand(%d,) optind=%d\n",argc,optind);

    if ( optind >= argc )
    {
	ERROR0(ERR_SYNTAX,"Missing command.\n");
	hint_exit(ERR_SYNTAX,0);
    }

    int cmd_stat;
    const CommandTab_t * cmd_ct = ScanCommand(&cmd_stat,argv[optind],CommandTab);
    if (!cmd_ct)
    {
	if ( cmd_stat > 0 )
	    ERROR0(ERR_SYNTAX,"Command abbreviation is ambiguous: %s\n",argv[optind]);
	else
	    ERROR0(ERR_SYNTAX,"Unknown command: %s\n",argv[optind]);
	hint_exit(ERR_SYNTAX,0);
    }

    TRACE("COMMAND FOUND: #%lld = %s\n",(u64)cmd_ct->id,cmd_ct->name1);

    enumError err = VerifySpecificOptions(&InfoUI,cmd_ct);
    if (err)
	hint_exit(err,cmd_ct->name1);

    argc -= optind+1;
    argv += optind+1;

    while ( argc-- > 0 )
	AtFileHelper(*argv++,false,AddParam);

    switch ((enumCommands)cmd_ct->id)
    {
	case CMD_VERSION:	version_exit();
	case CMD_HELP:		PrintHelp(&InfoUI,stdout,0,0); break;
	case CMD_TEST:		err = cmd_test(); break;
	case CMD_ERROR:		err = cmd_error(); break;
	case CMD_EXCLUDE:	err = cmd_exclude(); break;
	case CMD_TITLES:	err = cmd_titles(); break;
	case CMD_CREATE:	err = cmd_create(); break;

	case CMD_FILELIST:	err = cmd_filelist(); break;
	case CMD_FILETYPE:	err = cmd_filetype(); break;
	case CMD_ISOSIZE:	err = cmd_isosize(); break;

	case CMD_DUMP:		err = cmd_dump(); break;
	case CMD_DREGION:	err = cmd_dregion(); break;
	case CMD_ID6:		err = cmd_id6(); break;
	case CMD_LIST:		err = cmd_list(0); break;
	case CMD_LIST_L:	err = cmd_list(1); break;
	case CMD_LIST_LL:	err = cmd_list(2); break;
	case CMD_LIST_LLL:	err = cmd_list(3); break;

	case CMD_ILIST:		err = cmd_ilist(0); break;
	case CMD_ILIST_L:	err = cmd_ilist(1); break;
	case CMD_ILIST_LL:	err = cmd_ilist(2); break;

	case CMD_DIFF:		err = cmd_diff(); break;
	case CMD_EXTRACT:	err = cmd_extract(); break;
	case CMD_COPY:		err = cmd_copy(); break;
	case CMD_SCRUB:		err = cmd_scrub(); break;
	case CMD_EDIT:		err = cmd_edit(); break;
	case CMD_MOVE:		err = cmd_move(); break;
	case CMD_RENAME:	err = cmd_rename(true); break;
	case CMD_SETTITLE:	err = cmd_rename(false); break;

	case CMD_VERIFY:	err = cmd_verify(); break;
	case CMD_MIX:		err = cmd_mix(); break;

	// no default case defined
	//	=> compiler checks the existence of all enum values

	case CMD__NONE:
	case CMD__N:
	    help_exit(false);
    }

    return PrintErrorStat(err,cmd_ct->name1);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                       main()                    ///////////////
///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    SetupLib(argc,argv,WIT_SHORT,PROG_WIT);
    allow_fst = true;

    ASSERT( OPT__N_SPECIFIC <= 64 );

    InitializeStringField(&source_list);
    InitializeStringField(&recurse_list);

    //----- process arguments

    if ( argc < 2 )
    {
	printf("\n%s\nVisit %s%s for more info.\n\n",TITLE,URI_HOME,WIT_SHORT);
	hint_exit(ERR_OK,0);
    }

    enumError err = CheckEnvOptions("WIT_OPT",CheckOptions);
    if (err)
	hint_exit(err,0);

    err = CheckOptions(argc,argv,false);
    if (err)
	hint_exit(err,0);

    SetupFilePattern(file_pattern+PAT_OPT_FILES);
    err = CheckCommand(argc,argv);

    if (SIGINT_level)
	err = ERROR0(ERR_INTERRUPT,"Program interrupted by user.");
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////

