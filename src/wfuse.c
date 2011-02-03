

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
#define FUSE_USE_VERSION  25

#include <fuse.h>
#include <pthread.h>
#include <errno.h>

#include "debug.h"
#include "version.h"
#include "lib-std.h"
#include "lib-sf.h"
#include "titles.h"
#include "iso-interface.h"
#include "wbfs-interface.h"

#include "ui-wfuse.c"

//
///////////////////////////////////////////////////////////////////////////////

#define TITLE WFUSE_SHORT ": " WFUSE_LONG " v" VERSION " r" REVISION \
	" " SYSTEM " - " AUTHOR " - " DATE

///////////////////////////////////////////////////////////////////////////////

typedef enum enumOpenMode
{
	OMODE_ISO,
	OMODE_WBFS_ISO,
	OMODE_WBFS

} enumMode;

enumMode open_mode = OMODE_ISO;

bool is_iso      = false;
bool is_wbfs_iso = false;
bool is_wbfs     = false;
int  wbfs_slot	 = -1;

///////////////////////////////////////////////////////////////////////////////

typedef struct fuse_file_info fuse_file_info;

static char * source_file = 0;
static char * mount_point = 0;

static SuperFile_t main_sf;
static WBFS_t wbfs;

static struct stat stat_dir;
static struct stat stat_file;
static struct stat stat_link;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			built in help			///////////////
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

static enumError CheckOptions ( int argc, char ** argv )
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
///////////////			    mutex			///////////////
///////////////////////////////////////////////////////////////////////////////

static void lock_mutex()
{
    int err = pthread_mutex_lock(&mutex);
    if (err)
	TRACE("MUTEX LOCK ERR %u\n",err);
	
}

//-----------------------------------------------------------------------------

static void unlock_mutex()
{
    int err = pthread_mutex_unlock(&mutex);
    if (err)
	TRACE("MUTEX UNLOCK ERR %u\n",err);
	
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct DiscFile_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct DiscFile_t
{
    bool		used;		// false: entry not used
    uint		lock_count;	// number of locks
    time_t		timestamp;	// timestamp of last usage

    int			slot;		// -1 or slot index of WBFS
    WBFS_t		* wbfs;		// NULL or wbfs
    SuperFile_t 	* sf;		// NULL or file pointer
    wd_disc_t		* disc;		// NULL or disc pointer
    volatile WiiFst_t	* fst;		// NULL or collected files

    struct stat		stat_dir;	// template for directories
    struct stat		stat_file;	// template for regular files
    struct stat		stat_link;	// template for soft links

} DiscFile_t;

#define MAX_DISC_FILES		100
#define GOOD_DISC_FILES		  5
#define DISC_FILES_TIMEOUT1	 15
#define DISC_FILES_TIMEOUT2	 60

DiscFile_t dfile[MAX_DISC_FILES];
int n_dfile = 0;

///////////////////////////////////////////////////////////////////////////////

static DiscFile_t * get_disc_file ( int slot )
{
    DiscFile_t * df;
    DiscFile_t * end_dfile = dfile + MAX_DISC_FILES;
    
    lock_mutex();
    
    //----- try to find 'slot'

    for ( df = dfile; df < end_dfile; df++ )
	if ( df->used && df->slot == slot )
	{
	    df->lock_count++;
	    TRACE(">>D<< ALLOC DISC #%zu, slot=%d, lock=%d, n=%d/%d\n",
		df-dfile, df->slot, df->lock_count, n_dfile, MAX_DISC_FILES );
	    unlock_mutex();
	    return df;
	}
    
    //----- not found: find first free slot && close timeouts

    time_t ref_time = time(0)
		- ( n_dfile < GOOD_DISC_FILES ? DISC_FILES_TIMEOUT1 : DISC_FILES_TIMEOUT2 );

    DiscFile_t * found_df = 0;
    for ( df = dfile; df < end_dfile; df++ )
    {
	if ( df->used
	    && !df->lock_count
	    && ( n_dfile == MAX_DISC_FILES || df->timestamp < ref_time ) )
	{
	    TRACE(">>D<< CLOSE DISC #%zu, slot=%d, lock=%d, n=%d/%d\n",
		df-dfile, df->slot, df->lock_count, n_dfile, MAX_DISC_FILES );
	    if (df->fst)
	    {
		WiiFst_t * fst = (WiiFst_t*)df->fst; // avoid volatile warnings
	        df->fst = 0;
	        ResetFST(fst);
	        free(fst);
	    }
	    ResetWBFS(df->wbfs);
	    free(df->wbfs);
	    memset(df,0,sizeof(df));
	    n_dfile--;
	}

	if ( !found_df && !df->used )
	    found_df = df;
    }

    //----- open new file

    if (found_df)
    {
	memset(found_df,0,sizeof(found_df));
	WBFS_t * wbfs = malloc(sizeof(*wbfs));
	InitializeWBFS(wbfs);
	enumError err = OpenWBFS(wbfs,source_file,true,0);
	wbfs->cache_candidate = false;
	if (!err)
	{
	    err = OpenWDiscSlot(wbfs,slot,false);
	    if (!err)
		err = OpenWDiscSF(wbfs);
	}

	wd_disc_t * disc = err ? 0 : OpenDiscSF(wbfs->sf,true,true);
	TRACE(">>D<< disc=%p\n",disc);
	if (!disc)
	{
	    ResetWBFS(wbfs);
	    free(wbfs);
	    found_df = 0;
	}
	else
	{
	    wd_calc_disc_status(disc,true);

	    found_df->used		= true;
	    found_df->lock_count	= 1;
	    found_df->slot		= slot;
	    found_df->wbfs		= wbfs;
	    found_df->sf		= wbfs->sf;
	    found_df->disc		= disc;

	    memcpy(&found_df->stat_dir, &stat_dir, sizeof(found_df->stat_dir ));
	    memcpy(&found_df->stat_file,&stat_file,sizeof(found_df->stat_file));
	    memcpy(&found_df->stat_link,&stat_link,sizeof(found_df->stat_link));

	    wbfs_disc_t * d = wbfs->disc;
	    if (d)
	    {
		wbfs_inode_info_t * iinfo = wbfs_get_disc_inode_info(d,0);
		if (wbfs_is_inode_info_valid(wbfs->wbfs,iinfo))
		{
		    time_t tim = ntoh64(iinfo->atime);
		    if (tim)
			found_df->stat_dir .st_atime =
			found_df->stat_file.st_atime =
			found_df->stat_link.st_atime = tim;

		    tim = ntoh64(iinfo->mtime);
		    if (tim)
			found_df->stat_dir .st_mtime =
			found_df->stat_file.st_mtime =
			found_df->stat_link.st_mtime = tim;

		    tim = ntoh64(iinfo->ctime);
		    if (tim)
			found_df->stat_dir .st_ctime =
			found_df->stat_file.st_ctime =
			found_df->stat_link.st_ctime = tim;
		}
	    }
	    
	    n_dfile++;

	    TRACE(">>D<< OPEN DISC #%zu, slot=%d, lock=%d, n=%d/%d\n",
		found_df-dfile, found_df->slot, found_df->lock_count,
		n_dfile, MAX_DISC_FILES );
	}
    }

    unlock_mutex();

    return found_df;
}

///////////////////////////////////////////////////////////////////////////////

static void free_disc_file ( DiscFile_t * df )
{
    if (df)
    {
	DASSERT(df->used);
	DASSERT(df->lock_count);
	TRACE(">>D<< FREE DISC #%zu, slot=%d, lock=%d, n=%d/%d\n",
		df-dfile, df->slot, df->lock_count, n_dfile, MAX_DISC_FILES );

	lock_mutex();
	df->lock_count--;
	df->timestamp = time(0);
	unlock_mutex();
    }
}

///////////////////////////////////////////////////////////////////////////////

static WiiFst_t * get_fst ( DiscFile_t * df )
{
    DASSERT(df);
    WiiFst_t * fst = (WiiFst_t*)df->fst; // cast to avaoid volatile warnings
    if (!fst)
    {
	lock_mutex();
	fst = (WiiFst_t*)df->fst;
	if (!fst)
	{
	    fst = malloc(sizeof(*fst));
	    if (!fst)
		OUT_OF_MEMORY;
	    InitializeFST(fst);
	    CollectFST(fst,df->disc,0,false,WD_IPM_SLASH,true);
	    SortFST(fst,SORT_NAME,SORT_NAME);
	    df->fst = fst;
	}
	unlock_mutex();
    }
    return fst;
}

///////////////////////////////////////////////////////////////////////////////

static int get_fst_pointer // return relevant subpath length; -1 on error
(
    WiiFstPart_t	** found_part,	// not NULL: store found partition
    WiiFstFile_t	** found_file,	// not NULL: store first founds item
    WiiFstFile_t	** list_end,	// not NULL: store end of list
    DiscFile_t		* df,		// valid disc file
    wd_part_t		* part,		// NULL or relevant partition
    ccp			subpath		// NULL or subpath
)
{
    DASSERT(df);
    TRACE("get_fst_pointer(...,%s)\n",subpath);

    if (found_part)
	*found_part = 0;
    if (found_file)
	*found_file = 0;
    if (list_end)
	*list_end = 0;
    if (!part)
	return -1;
    if (!subpath)
	subpath = "";

    WiiFst_t * fst = get_fst(df);
    if (fst)
    {
	WiiFstPart_t *fst_part, *part_end = fst->part + fst->part_used;
	for ( fst_part = fst->part; fst_part < part_end; fst_part++ )
	    if ( part == fst_part->part )
	    {
		noTRACE("PART-FOUND:\n");
		// [2do] binary search

		int slen = strlen(subpath);
		WiiFstFile_t * ptr = fst_part->file;
		WiiFstFile_t * end = ptr + fst_part->file_used;
		for ( ; ptr < end; ptr++ )
		{
		    noTRACE("CHECK: %s\n",ptr->path);
		    if (!memcmp(subpath,ptr->path,slen))
		    {
			const char ch = ptr->path[slen];
			noTRACE("FOUND[%02x]: %s\n",ch,ptr->path);
			if ( !ch || ch == '/' )
			{
			    if (found_part)
				*found_part = fst_part;
			    if (found_file)
				*found_file = ptr;
			    if (list_end)
				*list_end = end;
			    return slen;
			}
		    }
		}
		break;
	    }
    }
    return -1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			info helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

#define INFO_SIZE 500

///////////////////////////////////////////////////////////////////////////////

static size_t print_root_info
(
    char		* pbuf,
    size_t		bufsize
)
{
    DASSERT(pbuf);

    return snprintf( pbuf, bufsize,
		"is-iso=%u\n"
		"is-wbfs=%u\n"
		"file-type=%s\n"
		"file-size=%llu\n"
		"split-count=%u\n"
		,is_iso
		,is_wbfs
		,oft_info[main_sf.iod.oft].name
		,(u64)main_sf.f.st.st_size
		,main_sf.f.split_used > 1 ? main_sf.f.split_used : 0
		);
}

///////////////////////////////////////////////////////////////////////////////

static size_t print_wbfs_info
(
    char		* pbuf,
    size_t		bufsize
)
{
    DASSERT(pbuf);
    char * dest = pbuf;

    wbfs_t * w = wbfs.wbfs;
    if (w)    
    {
	dest += snprintf( pbuf, bufsize,
		"hd-sector-size=%u\n"
		"n-hd-sectors=%u\n"

		"wii-sector-size=%u\n"
		"wii-sectors=%u\n"
		"wii-sectors-per-disc=%u\n"

		"wbfs-sector-size=%u\n"
		"wbfs-sectors=%u\n"
		"wbfs-sectors-per-disc=%u\n"

		"part-lba=%u\n"

		"used-discs=%u\n"
		"free-discs=%u\n"
		"total-discs=%u\n"

		"used-size-mib=%u\n"
		"free-size-mib=%u\n"
		"total-size-mib=%u\n"

		,w->hd_sec_sz
		,w->n_hd_sec

		,w->wii_sec_sz
		,w->n_wii_sec
		,w->n_wii_sec_per_disc

		,w->wbfs_sec_sz
		,w->n_wbfs_sec
		,w->n_wbfs_sec_per_disc

		,w->part_lba

		,wbfs.used_discs
		,wbfs.free_discs
		,wbfs.total_discs

		,wbfs.used_mib
		,wbfs.free_mib
		,wbfs.total_mib
		);
    }

    *dest = 0;
    return dest - pbuf;
}

///////////////////////////////////////////////////////////////////////////////

static size_t print_iso_info
(
    DiscFile_t		* df,
    char		* pbuf,
    size_t		bufsize
)
{
    DASSERT(df);
    DASSERT(pbuf);
    char * dest = pbuf;
    //char * end = pbuf + bufsize - 1;
    
    wd_disc_t * disc = df->disc;
    if (disc)
    {
	char id[7];
	wd_print_id(&disc->dhead.disc_id,6,id);
	dest += snprintf( pbuf, bufsize,
		"disc-id=%s\n"
		"disc-title=%.64s\n"
		"db-title=%s\n"
		"n-part=%u\n"
		,id
		,(ccp)disc->dhead.disc_title
		,GetTitle(id,"")
		,disc->n_part
		);
    }

    *dest = 0;
    return dest - pbuf;
}

///////////////////////////////////////////////////////////////////////////////

static ccp get_sign_info ( cert_stat_t stat )
{
    if ( stat & CERT_F_HASH_FAILED )	return "not signed";
    if ( stat & CERT_F_HASH_FAKED )	return "fake signed";
    if ( stat & CERT_F_HASH_OK )	return "well signed";
    return "";
}

///////////////////////////////////////////////////////////////////////////////

static size_t print_part_info
(
    wd_part_t		* part,
    char		* pbuf,
    size_t		bufsize
)
{
    DASSERT(part);
    DASSERT(pbuf);

    return snprintf( pbuf, bufsize,
		"index=%u\n"
		"ptab=%u\n"
		"ptab-index=%u\n"
		"type=%x\n"
		"size=%llu\n"

		"is-overlay=%u\n"
		"is-gc=%u\n"
		"is-encrypted=%u\n"
		"is-decrypted=%u\n"
		"is-skeleton=%u\n"
		"is-scrubbed=%u\n"
		"ticket-signed=%s\n"
		"tmd-signed=%s\n"

		"start-sector=%u\n"
		"end-sector=%u\n"

		"fst-entries=%u\n"
		"virtual-dirs=%u\n"
		"virtual-files=%u\n"

		,part->index
		,part->ptab_index
		,part->ptab_part_index
		,part->part_type
		,part->part_size

		,part->is_overlay
		,part->is_gc
		,0 != ( part->sector_stat & WD_SS_ENCRYPTED )
		,0 != ( part->sector_stat & WD_SS_DECRYPTED )
		,0 != ( part->sector_stat & WD_SS_F_SKELETON )
		,0 != ( part->sector_stat & WD_SS_F_SCRUBBED )

		,get_sign_info(part->cert_tik_stat)
		,get_sign_info(part->cert_tmd_stat)

		,part->data_sector
		,part->end_sector

		,part->fst_n
		,part->fst_dir_count
		,part->fst_file_count
		);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			analyze_path()			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumAnaPath
{
    AP_UNKNOWN,

    AP_ROOT,
    AP_ROOT_INFO,

    AP_ISO,
    AP_ISO_RAW,
    AP_ISO_INFO,
    AP_ISO_PART,

    AP_ISO_PART_DATA,
    AP_ISO_PART_UPDATE,
    AP_ISO_PART_CHANNEL,
    AP_ISO_PART_MAIN,

    AP_WBFS,
    AP_WBFS_SLOT,
    AP_WBFS_ID,
    AP_WBFS_INFO,

} enumAnaPath;

//-----------------------------------------------------------------------------

typedef struct AnaPath_t
{
    enumAnaPath	ap;
    int		len;
    ccp		path;

} AnaPath_t;

//-----------------------------------------------------------------------------

#define DEF_AP(a,s) { a, sizeof(s)-1, s }

static AnaPath_t ana_path_tab_root[] =
{
    DEF_AP( AP_ISO,		"/iso" ),
    DEF_AP( AP_WBFS_SLOT,	"/wbfs/slot" ),
    DEF_AP( AP_WBFS_ID,		"/wbfs/id" ),
    DEF_AP( AP_WBFS_INFO,	"/wbfs/info.txt" ),
    DEF_AP( AP_WBFS,		"/wbfs" ),
    DEF_AP( AP_ROOT_INFO,	"/info.txt" ),
    {0,0,0}
};

static AnaPath_t ana_path_tab_iso[] =
{
    DEF_AP( AP_ISO_RAW,		"/raw.iso" ),
    DEF_AP( AP_ISO_INFO,	"/info.txt" ),
    DEF_AP( AP_ISO_PART_DATA,	"/part/data" ),
    DEF_AP( AP_ISO_PART_UPDATE,	"/part/update" ),
    DEF_AP( AP_ISO_PART_CHANNEL,"/part/channel" ),
    DEF_AP( AP_ISO_PART_MAIN,	"/part/main" ),
    DEF_AP( AP_ISO_PART,	"/part" ),
    {0,0,0}
};

//-----------------------------------------------------------------------------

static ccp analyze_path ( enumAnaPath * stat, ccp path, AnaPath_t * tab )
{
    TRACE("analyze_path(%p,%s,%p)\n",stat,path,tab);
    DASSERT(tab);
    if (!path)
	path = "";
    const int plen = strlen(path);

    for ( ; tab->path; tab++ )
    {
	if ( ( plen == tab->len || plen > tab->len && path[tab->len] == '/' )
	    && !memcmp(path,tab->path,tab->len) )
	{
	    TRACE(" - found: id=%u, len=%u, sub=%s\n",tab->ap,tab->len,path+tab->len);
	    if (stat)
		*stat = tab->ap;
	    return path + tab->len;
	}
    }

    if ( !*path || !strcmp(path,"/") )
    {
	TRACE(" - root\n");
	if (stat)
	    *stat = AP_ROOT;
	if (*path)
	    path++;
    }
    else if (stat)
	*stat = AP_UNKNOWN;

    return path;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			analyze_part()			///////////////
///////////////////////////////////////////////////////////////////////////////

static wd_part_t * analyze_part
(
    ccp		* subpath,	// result
    ccp		path,		// source path
    wd_disc_t	* disc		// disc
)
{
    TRACE("analyze_part(%s)\n",path);
    DASSERT(path);

    wd_part_t * found_part = 0;

    if ( disc
	 && path[0] == '/'
	 && path[1] >= '0' && path[1] < '0' + WII_MAX_PTAB
	 && path[2] == '.' )
    {
	//TRACELINE;
	char * end;
	const uint p_idx = strtoul(path+3,&end,10);
	if ( end > path+3 && ( !*end || *end == '/' ))
	{
	    const uint t_idx = path[1] - '0';
	    TRACE(" - check %u.%u\n",t_idx,p_idx);
	    int ip;
	    for ( ip = 0; ip < disc->n_part; ip++ )
	    {
		wd_part_t * part = disc->part + ip;
		TRACE(" - cmp %u.%u\n",part->ptab_index,part->ptab_part_index);
		if ( t_idx == part->ptab_index && p_idx == part->ptab_part_index )
		{
		    TRACE(" - found %u.%u\n",t_idx,p_idx);
		    path = end;
		    found_part = part;
		    break;
		}
	    }
	}
    }

    if (subpath)
	*subpath = path;
    return found_part;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			analyze_slot()			///////////////
///////////////////////////////////////////////////////////////////////////////

static int analyze_slot
(
    ccp		* subpath,	// result
    ccp		path		// source path
)
{
    TRACE("analyze_slot(%s)\n",path);
    DASSERT(path);

    int found_slot = -1;
    wbfs_t * w = wbfs.wbfs;
    if ( path[0] == '/' && w )
    {
	char * end;
	const uint slot = strtoul(path+1,&end,10);
	if ( end > path+1
	    && ( !*end || *end == '/' )
	    && slot < w->max_disc
	    &&  w->head->disc_table[slot] )
	{
	    found_slot = slot;
	    path = end;
	}
    }

    if (subpath)
	*subpath = path;
    return found_slot;
}

///////////////////////////////////////////////////////////////////////////////

static int analyze_wbfs_id
(
    ccp		path		// source path
)
{
    TRACE("analyze_wbfs_id(%s)\n",path);
    DASSERT(path);

    wbfs_t * w = wbfs.wbfs;
    if ( path[0] == '/' && strlen(path) == 7 && w )
    {
	id6_t * id_list = wbfs_load_id_list(w,false);
	if (id_list)
	{
	    int slot;
	    for ( slot = 0; slot < w->max_disc; slot++ )
		if ( w->head->disc_table[slot] && !memcmp(path+1,id_list[slot],6) )
		    return slot;
	}
    }

    return -1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			wfuse_getattr()			///////////////
///////////////////////////////////////////////////////////////////////////////

static u64 get_iso_size
(
    DiscFile_t		* df
)
{
    DASSERT(df);

    u64 fsize = df->sf->file_size;
    if (!fsize)
	fsize = (u64)WII_SECTOR_SIZE
		* ( df->disc->usage_max > WII_SECTORS_SINGLE_LAYER
		  ? df->disc->usage_max : WII_SECTORS_SINGLE_LAYER );
    return fsize;
}

///////////////////////////////////////////////////////////////////////////////

static int wfuse_getattr_iso
(
    const char		* path,
    struct stat		* st,
    DiscFile_t		* df
)
{
    TRACE("##### wfuse_getattr_iso(%s)\n",path);
    DASSERT(path);
    DASSERT(st);
    DASSERT(df);
    DASSERT(df->sf);

    char pbuf[INFO_SIZE];

    enumAnaPath ap;
    ccp subpath = analyze_path(&ap,path,ana_path_tab_iso);

    switch(ap)
    {
	case AP_ROOT:
	    memcpy(st,&df->stat_dir,sizeof(*st));
	    st->st_nlink = 3;
	    return 0;

	case AP_ISO_RAW:
	    if (!*subpath)
	    {
		memcpy(st,&df->stat_file,sizeof(*st));
		st->st_size = get_iso_size(df);
		return 0;
	    }
	    break;

	case AP_ISO_INFO:
	    if (!*subpath)
	    {
		memcpy(st,&df->stat_file,sizeof(*st));
		st->st_size = print_iso_info(df,pbuf,sizeof(pbuf));
		return 0;
	    }
	    break;

	case AP_ISO_PART:
	    if (!*subpath)
	    {
		memcpy(st,&df->stat_dir,sizeof(*st));
		if (df->disc)
		    st->st_nlink += df->disc->n_part;
		return 0;
	    }

	    wd_part_t * part = analyze_part(&subpath,subpath,df->disc);
	    if (part)
	    {
		if (!strcmp(subpath,"/info.txt"))
		{
		    memcpy(st,&df->stat_file,sizeof(*st));
		    st->st_size = print_part_info(part,pbuf,sizeof(pbuf));
		    return 0;
		}
		
		WiiFstFile_t *file, *end;
		const uint slen = get_fst_pointer(0,&file,&end,df,part,subpath);
		if (file)
		{
		    if ( file->icm == WD_ICM_DIRECTORY )
		    {
			memcpy(st,&df->stat_dir,sizeof(*st));
			for ( file++; file < end && !memcmp(subpath,file->path,slen); file++ )
			{
			    if ( file->icm == WD_ICM_DIRECTORY )
			    {
				ccp path = file->path+slen;
				if ( *path == '/' )
				{
				    path++;
				    if ( *path && !strchr(path,'/') )
					st->st_nlink++;
				}
			    }
			}
		    }
		    else
		    {
			memcpy(st,&df->stat_file,sizeof(*st));
			st->st_size = file->size;
		    }
		    return 0;
		}
	    }
	    break;

	case AP_ISO_PART_DATA:
	case AP_ISO_PART_UPDATE:
	case AP_ISO_PART_CHANNEL:
	case AP_ISO_PART_MAIN:
	    if (!*subpath)
	    {
		memcpy(st,&df->stat_link,sizeof(*st));
		return 0;
	    }
	    break;

	default:
	    break;
    }

    memset(st,0,sizeof(*st));
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

static int wfuse_getattr
(
    const char		* path,
    struct stat		* st
)
{
    TRACE("##### wfuse_getattr(%s)\n",path);
    DASSERT(path);
    DASSERT(st);

    enumAnaPath ap;
    ccp subpath = analyze_path(&ap,path,ana_path_tab_root);

    char pbuf[INFO_SIZE];

    switch(ap)
    {
	case AP_ROOT:
	    memcpy(st,&stat_dir,sizeof(*st));
	    st->st_nlink = is_wbfs_iso ? 4 : 3;
	    return 0;
		
	case AP_ROOT_INFO:
	    if (!*subpath)
	    {
		memcpy(st,&stat_file,sizeof(*st));
		st->st_size = print_root_info(pbuf,sizeof(pbuf));
		return 0;
	    }
	    break;
		
	case AP_ISO:
	    noTRACE(" -> AP_ISO, sub=%s\n",subpath);
	    if (!*subpath) // -> /iso
	    {
		switch(open_mode)
		{
		    case OMODE_ISO:
			memcpy(st,&stat_dir,sizeof(*st));
			st->st_nlink = 3;
			return 0;

		    case OMODE_WBFS_ISO:
			memcpy(st,&stat_link,sizeof(*st));
			return 0;

		    case OMODE_WBFS:
			memset(st,0,sizeof(*st));
			return 1;
		}
	    }
	    return wfuse_getattr_iso(subpath,st,dfile);

	case AP_WBFS:
	    if ( is_wbfs && !*subpath )
	    {
		memcpy(st,&stat_dir,sizeof(*st));
		st->st_nlink = 4;
		return 0;
	    }
	    break;

	case AP_WBFS_SLOT:
	    if (!is_wbfs)
		break;
	    TRACE("subpath = |%s|\n",subpath);
	    if (!*subpath)
	    {
		memcpy(st,&stat_dir,sizeof(*st));
		st->st_nlink = 2 + wbfs.used_discs;
		return 0;
	    }

	    const int slot = analyze_slot(&subpath,subpath);
	    if ( slot >= 0 )
	    {
		if (!*subpath)
		{
		    memcpy(st,&stat_dir,sizeof(*st));
		    st->st_nlink = 3;
		    return 0;
		}

		DiscFile_t * df = get_disc_file(slot);
		if (df)
		{
		    const int stat = wfuse_getattr_iso(subpath,st,df);
		    free_disc_file(df);
		    return stat;
		}
	    }
	    break;
	    
	case AP_WBFS_ID:
	    if (!is_wbfs)
		break;
	    if (!*subpath)
	    {
		memcpy(st,&stat_dir,sizeof(*st));
		st->st_nlink = 2 + wbfs.used_discs;
		return 0;
	    }
	    else
	    {
		int slot = analyze_wbfs_id(subpath);
		if ( slot >= 0 )
		{
		    memcpy(st,&stat_link,sizeof(*st));
		    return 0;
		}
	    }
	    break;

	case AP_WBFS_INFO:
	    if (!*subpath)
	    {
		memcpy(st,&stat_file,sizeof(*st));
		st->st_size = print_wbfs_info(pbuf,sizeof(pbuf));
		return 0;
	    }
	    break;

	default:
	    break;
    }

    memset(st,0,sizeof(*st));
    return 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			wfuse_read()			///////////////
///////////////////////////////////////////////////////////////////////////////

static int copy_helper
(
    char		* buf,		// read buffer
    size_t		size,		// size of buf and size to read
    off_t		offset,		// read offset
    const void		* src,		// source buffer
    size_t		src_len		// length of 'copy_buf'
) 
{
    DASSERT(buf);
    DASSERT(src);

    if ( offset >= src_len )
	return 0;

    src_len -= offset;
    if ( src_len > size )
	 src_len = size;
    memcpy(buf,(ccp)src+offset,src_len);
    return src_len;
}

///////////////////////////////////////////////////////////////////////////////

static int wfuse_read_iso
(
    const char		* path,		// path to the file
    char		* buf,		// read buffer
    size_t		size,		// size of buf and size to read
    off_t		offset,		// read offset
    fuse_file_info	* info,		// fuse info

    DiscFile_t		* df,		// related disc file
    char		* pbuf,		// temporary print buffer
    size_t		pbuf_size	// size of 'pbuf'
) 
{
    TRACE("##### wfuse_read_iso(%s,%llu+%zu)\n",path,(u64)offset,size);

    enumAnaPath ap;
    ccp subpath = analyze_path(&ap,path,ana_path_tab_iso);
    wd_part_t * part;

    switch(ap)
    {
	case AP_ISO_INFO:
	    if (!*subpath)
		return copy_helper(buf,size,offset,pbuf,
					print_iso_info(df,pbuf,pbuf_size));
	    break;

	case AP_ISO_RAW:
	    if (!*subpath)
	    {
		const u64 fsize = get_iso_size(df);
		if ( offset < fsize )
		{
		    if ( size > fsize - offset )
			 size = fsize - offset;
		    return ReadSF(df->sf,offset,buf,size) ? -EIO : size;
		}
	    }
	    return 0;

	case AP_ISO_PART:
	    part = analyze_part(&subpath,subpath,df->disc);
	    if ( part && *subpath )
	    {
		if (!strcmp(subpath,"/info.txt"))
		    return copy_helper(buf,size,offset,pbuf,
					print_part_info(part,pbuf,pbuf_size));

		WiiFstPart_t *fst_part;
		WiiFstFile_t *file;
		get_fst_pointer(&fst_part,&file,0,df,part,subpath);
		if ( file && file->icm != WD_ICM_DIRECTORY )
		{
		    if ( offset < file->size )
		    {
			if ( size > file->size - offset )
			     size = file->size - offset;
			lock_mutex();
			int stat = ReadFileFST(fst_part,file,offset,buf,size) ? -EIO : size;
			unlock_mutex();
			return stat;
		    }
		    return 0;
		}
	    }
	    break;

	default:
	    break;
    }

    return -ENOENT;
}

///////////////////////////////////////////////////////////////////////////////

static int wfuse_read
(
    const char		* path,		// path to the file
    char		* buf,		// read buffer
    size_t		size,		// size of buf and size to read
    off_t		offset,		// read offset
    fuse_file_info	* info		// fuse info
) 
{
    TRACE("##### wfuse_read(%s,%llu+%zu)\n",path,(u64)offset,size);

    enumAnaPath ap;
    ccp subpath = analyze_path(&ap,path,ana_path_tab_root);

    char pbuf[INFO_SIZE];

    switch(ap)
    {
	case AP_ROOT_INFO:
	    if (!*subpath)
		return copy_helper(buf,size,offset,pbuf,
					print_root_info(pbuf,sizeof(pbuf)));
	    break;

	case AP_ISO:
	    return wfuse_read_iso( subpath, buf, size, offset, info,
					dfile, pbuf,sizeof(pbuf) );

	case AP_WBFS_SLOT:
	    if ( is_wbfs && *subpath )
	    {
		const int slot = analyze_slot(&subpath,subpath);
		if ( slot >= 0 )
		{
		    DiscFile_t * df = get_disc_file(slot);
		    if (df)
		    {
			const int stat
			    = wfuse_read_iso( subpath, buf, size, offset, info,
					df, pbuf,sizeof(pbuf) );
			free_disc_file(df);
			return stat;
		    }
		}
	    }
	    break;

	case AP_WBFS_INFO:
	    if (!*subpath)
		return copy_helper(buf,size,offset,pbuf,
					print_wbfs_info(pbuf,sizeof(pbuf)));
	    break;

	default:
	    break;
    }

    return -ENOENT;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			wfuse_readlink()		///////////////
///////////////////////////////////////////////////////////////////////////////

static int wfuse_readlink_iso
(
    const char		* path,
    char		* fuse_buf,
    size_t		bufsize,
    DiscFile_t		* df
)
{
    TRACE("##### wfuse_readlink(%s)\n",path);
    DASSERT(fuse_buf);
    DASSERT(df);

    enumAnaPath ap;
    ccp subpath = analyze_path(&ap,path,ana_path_tab_iso);

    switch(ap)
    {
	case AP_ROOT:
	    if ( !*subpath && is_wbfs_iso )
	    {
		snprintf(fuse_buf,bufsize,"wbfs/slot/%u/",wbfs_slot);
		return 0;
	    }
	    break;
	    
	case AP_ISO_PART_DATA:
	    if (!*subpath)
	    {
		wd_disc_t * disc = df->disc;
		if ( disc && disc->data_part )
		    snprintf(fuse_buf,bufsize,"%u.%u/",
			disc->data_part->ptab_index,
			disc->data_part->ptab_part_index);
		return 0;
	    }
	    break;

	case AP_ISO_PART_UPDATE:
	    if (!*subpath)
	    {
		wd_disc_t * disc = df->disc;
		if ( disc && disc->update_part )
		    snprintf(fuse_buf,bufsize,"%u.%u/",
			disc->update_part->ptab_index,
			disc->update_part->ptab_part_index);
		return 0;
	    }
	    break;

	case AP_ISO_PART_CHANNEL:
	    if (!*subpath)
	    {
		wd_disc_t * disc = df->disc;
		if ( disc && disc->channel_part )
		    snprintf(fuse_buf,bufsize,"%u.%u/",
			disc->channel_part->ptab_index,
			disc->channel_part->ptab_part_index);
		return 0;
	    }
	    break;

	case AP_ISO_PART_MAIN:
	    if (!*subpath)
	    {
		wd_disc_t * disc = df->disc;
		if ( disc && disc->main_part )
		    snprintf(fuse_buf,bufsize,"%u.%u/",
			disc->main_part->ptab_index,
			disc->main_part->ptab_part_index);
		return 0;
	    }
	    break;
	    
	default:
	    break;
    }
    *fuse_buf = 0;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

static int wfuse_readlink
(
    const char		* path,
    char		* fuse_buf,
    size_t		bufsize
)
{
    TRACE("##### wfuse_readlink(%s)\n",path);
    DASSERT(fuse_buf);

    enumAnaPath ap;
    ccp subpath = analyze_path(&ap,path,ana_path_tab_root);

    switch(ap)
    {
	case AP_ISO:
	    return wfuse_readlink_iso(subpath,fuse_buf,bufsize,dfile);
	    
	case AP_WBFS_ID:
	    if ( is_wbfs && *subpath )
	    {
		int slot = analyze_wbfs_id(subpath);
		if ( slot >= 0 )
		    snprintf(fuse_buf,bufsize,"../slot/%u/",slot);
		return 0;
	    }
	    break;

	case AP_WBFS_SLOT:
	    if (!is_wbfs)
		break;
	    if (*subpath)
	    {
		const int slot = analyze_slot(&subpath,subpath);
		if ( slot >= 0 )
		{
		    DiscFile_t * df = get_disc_file(slot);
		    if (df)
		    {
			const int stat
			    = wfuse_readlink_iso(subpath,fuse_buf,bufsize,df);
			free_disc_file(df);
			return stat;
		    }
		}
	    }
	    break;

	default:
	    break;
    }
    *fuse_buf = 0;
    return 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			wfuse_readdir()			///////////////
///////////////////////////////////////////////////////////////////////////////

static int wfuse_readdir_iso
(
    const char		* path,
    void		* fuse_buf,
    fuse_fill_dir_t	filler,
    off_t		offset,
    fuse_file_info	* info,
    DiscFile_t		* df
)
{
    TRACE("##### wfuse_readdir_iso(%s,df=%zu)\n",path,df-dfile);
    DASSERT(filler);
    DASSERT(df);
    DASSERT(df->sf);

    enumAnaPath ap;
    ccp subpath = analyze_path(&ap,path,ana_path_tab_iso);

    char pbuf[100];

    switch(ap)
    {
	case AP_ROOT:
	    if (!*subpath)
	    {
		filler(fuse_buf,".",0,0);
		filler(fuse_buf,"..",0,0);
		filler(fuse_buf,"part",0,0);
		filler(fuse_buf,"raw.iso",0,0);
		filler(fuse_buf,"info.txt",0,0);
	    }
	    break;

	case AP_ISO_PART:
	    filler(fuse_buf,".",0,0);
	    filler(fuse_buf,"..",0,0);

	    if (!*subpath)
	    {
		wd_disc_t * disc = df->disc;
		if (disc)
		{
		    int ip;
		    for ( ip = 0; ip < disc->n_part; ip++ )
		    {
			wd_part_t * part = disc->part + ip;
			snprintf( pbuf, sizeof(pbuf),"%u.%u",
				part->ptab_index, part->ptab_part_index );
			filler(fuse_buf,pbuf,0,0);
		    }
		    if (disc->data_part)
			filler(fuse_buf,"data",0,0);
		    if (disc->update_part)
			filler(fuse_buf,"update",0,0);
		    if (disc->channel_part)
			filler(fuse_buf,"channel",0,0);
		    if (disc->main_part)
			filler(fuse_buf,"main",0,0);
		}
		return 0;
	    }
	    
	    wd_part_t * part = analyze_part(&subpath,subpath,df->disc);
	    if (part)
	    {
		if (!*subpath)
		    filler(fuse_buf,"info.txt",0,0);

		WiiFstFile_t *ptr, *end;
		int slen = get_fst_pointer(0,&ptr,&end,df,part,subpath);
		if ( slen >= 0 )
		{
		    for ( ; ptr < end && !memcmp(subpath,ptr->path,slen); ptr++ )
		    {
			ccp path = ptr->path+slen;
			noTRACE("XCHECK: %s\n",path);
			if ( *path == '/' )
			{
			    path++;
			    TRACE("PATH: %s\n",path);
			    if ( *path && !strchr(path,'/') )
				filler(fuse_buf,path,0,0);
			}
		    }
		}
	    }
	    break;

	default:
	    break;

    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static int wfuse_readdir
(
    const char		* path,
    void		* fuse_buf,
    fuse_fill_dir_t	filler,
    off_t		offset,
    fuse_file_info	* info
)
{
    TRACE("##### wfuse_readdir(%s)\n",path);

    DASSERT(filler);
    enumAnaPath ap;
    ccp subpath = analyze_path(&ap,path,ana_path_tab_root);

    char pbuf[20];

    switch(ap)
    {
	case AP_ROOT:
	    if (!*subpath)
	    {
		filler(fuse_buf,".",0,0);
		filler(fuse_buf,"..",0,0);
		if (is_iso)
		    filler(fuse_buf,"iso",0,0);
		if (is_wbfs)
		    filler(fuse_buf,"wbfs",0,0);
		filler(fuse_buf,"info.txt",0,0);
	    }
	    break;
		
	case AP_ISO:
	    return wfuse_readdir_iso(subpath,fuse_buf,filler,offset,info,dfile);

	case AP_WBFS:
	    if ( is_wbfs && !*subpath )
	    {
		filler(fuse_buf,".",0,0);
		filler(fuse_buf,"..",0,0);
		filler(fuse_buf,"slot",0,0);
		filler(fuse_buf,"id",0,0);
		filler(fuse_buf,"info.txt",0,0);
	    }
	    break;

	case AP_WBFS_SLOT:
	    if (!is_wbfs)
		break;

	    if (!*subpath)
	    {
		filler(fuse_buf,".",0,0);
		filler(fuse_buf,"..",0,0);
		wbfs_t * w = wbfs.wbfs;
		if (w)
		{
		    int slot;
		    for ( slot = 0; slot < w->max_disc; slot++ )
			if (w->head->disc_table[slot])
			{
			    snprintf(pbuf,sizeof(pbuf),"%u",slot);
			    filler(fuse_buf,pbuf,0,0);
			}
		}
	    }
	    else
	    {
		const int slot = analyze_slot(&subpath,subpath);
		if ( slot >= 0 )
		{
		    DiscFile_t * df = get_disc_file(slot);
		    if (df)
		    {
			const int stat
			    = wfuse_readdir_iso(subpath,fuse_buf,filler,offset,info,df);
			free_disc_file(df);
			return stat;
		    }
		}
	    }
	    break;
    
	case AP_WBFS_ID:
	    if ( is_wbfs && !*subpath )
	    {
		filler(fuse_buf,".",0,0);
		filler(fuse_buf,"..",0,0);
		wbfs_t * w = wbfs.wbfs;
		if (w)
		{
		    id6_t * id_list = wbfs_load_id_list(w,false);
		    int slot;
		    for ( slot = 0; slot < w->max_disc; slot++ )
			if (w->head->disc_table[slot])
			    filler(fuse_buf,id_list[slot],0,0);
		}
	    }
	    break;
    
	default:
	    break;
    }
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    main()			///////////////
///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    //----- setup

    SetupLib(argc,argv,WFUSE_SHORT,PROG_WFUSE);
    InitializeSF(&main_sf);
    InitializeWBFS(&wbfs);
    memset(&dfile,0,sizeof(dfile));
    GetTitle("ABC",0); // force loading title DB


    //----- process arguments

    if ( argc < 2 )
    {
	printf("\n%s\nVisit %s%s for more info.\n\n",TITLE,URI_HOME,WFUSE_SHORT);
	hint_exit(ERR_OK);
    }

    printf( TITLE "\n" );
    if ( CheckOptions(argc,argv) || optind + 2 != argc )
	hint_exit(ERR_SYNTAX);

    source_file = argv[optind];
    mount_point = argv[optind+1];

    enumError err = OpenSF(&main_sf,source_file,true,false);
    if (err)
	return err;


    //----- setup stat templates

    memcpy(&stat_dir, &main_sf.f.st,sizeof(stat_dir));
    memcpy(&stat_file,&main_sf.f.st,sizeof(stat_file));
    memcpy(&stat_link,&main_sf.f.st,sizeof(stat_link));

    const mode_t mode	= main_sf.f.st.st_mode & 0444;
    stat_dir .st_mode	= S_IFDIR | mode | mode >> 2;
    stat_file.st_mode	= S_IFREG | mode;
    stat_link.st_mode	= S_IFLNK | mode | mode >> 2;

    stat_dir .st_nlink	= 2;
    stat_file.st_nlink	= 1;
    stat_link.st_nlink	= 1;

    stat_dir .st_size	= 0;
    stat_file.st_size	= 0;
    stat_link.st_size	= 0;

    stat_dir .st_blocks	= 0;
    stat_file.st_blocks	= 0;
    stat_link.st_blocks	= 0;


    //----- analyze file type

    enumFileType ftype = AnalyzeFT(&main_sf.f);
    if ( ftype & FT_ID_WBFS )
    {
	PRINT("-> WBFS\n");
	is_wbfs = true;
	open_mode = OMODE_WBFS;
	err = SetupWBFS(&wbfs,&main_sf,true,0,0);
	if (err)
	    return err;
	wbfs.cache_candidate = false;
	CalcWBFSUsage(&wbfs);

	if ( wbfs.used_discs == 1 && wbfs.wbfs )
	{
	    wbfs_t * w = wbfs.wbfs;
	    wbfs_load_id_list(w,false);
	    int slot;
	    for ( slot = 0; slot < w->max_disc; slot++ )
		if (w->head->disc_table[slot])
		{
		    wbfs_slot = slot;
		    is_iso = is_wbfs_iso = true;
		    open_mode = OMODE_WBFS_ISO;
		    break;
		}
	}
    }
    else if ( ftype & FT_A_ISO )
    {
	PRINT("-> ISO\n");
	is_iso = true;
	open_mode = OMODE_ISO;

	wd_disc_t * disc = OpenDiscSF(&main_sf,true,true);
	if (!disc)
	    return ERR_INVALID_FILE;

	wd_calc_disc_status(disc,true);

	// setup the only disc file!
	dfile->used		= true;
	dfile->lock_count	= 1;
	dfile->slot		= -1;
	dfile->sf		= &main_sf;
	dfile->disc		= disc;

	memcpy(&dfile->stat_dir, &stat_dir, sizeof(dfile->stat_dir ));
	memcpy(&dfile->stat_file,&stat_file,sizeof(dfile->stat_file));
	memcpy(&dfile->stat_link,&stat_link,sizeof(dfile->stat_link));

	n_dfile			= 1;
    }
    else
    {
	return ERROR0(ERR_INVALID_FILE,
			"File type '%s' not supported: %s\n",
			GetNameFT(ftype,0), source_file );
    }

    printf("mount %s:%s -> %s\n",
		oft_info[main_sf.iod.oft].name, source_file, mount_point );
    source_file = realpath(source_file,0);


    //----- start fuse

    char * av[10];
    int ac = 0;
    av[ac++] = argv[0];
    av[ac++] = mount_point;
    av[ac] = 0;
    ASSERT( ac < sizeof(av)/sizeof(*av) );
    
    static struct fuse_operations wfuse_oper =
    {
	.getattr    = wfuse_getattr,
	.read	    = wfuse_read,
	.readlink   = wfuse_readlink,
	.readdir    = wfuse_readdir,
    };

    return fuse_main(ac,av,&wfuse_oper);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////

