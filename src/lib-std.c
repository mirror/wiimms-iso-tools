
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
#define _XOPEN_SOURCE 1

#include <sys/time.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#if defined(__CYGWIN__)
  #include <cygwin/fs.h>
  #include <io.h>
  //#include <locale.h>
#elif defined(__APPLE__)
  #include <sys/disk.h>
#elif defined(__linux__)
  #include <linux/fs.h>
#endif

#include "version.h"
#include "lib-std.h"
#include "lib-sf.h"
#include "lib-bzip2.h"
#include "lib-lzma.h"
#include "wbfs-interface.h"
#include "crypt.h"
#include "titles.h"
#include "ui.h"
#include "dclib-utf8.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Setup			///////////////
///////////////////////////////////////////////////////////////////////////////

enumProgID	prog_id			= PROG_UNKNOWN;
u32		revision_id		= SYSTEMID + REVISION_NUM;
ccp		progname		= "?";
ccp		search_path[5]		= {0};
ccp		lang_info		= 0;
volatile int	SIGINT_level		= 0;
volatile int	verbose			= 0;
volatile int	logging			= 0;
int		progress		= 0;
SortMode	sort_mode		= SORT_DEFAULT;
ShowMode	opt_show_mode		= SHOW__DEFAULT;
wd_size_mode_t	opt_unit		= WD_SIZE_DEFAULT;
RepairMode	repair_mode		= REPAIR_NONE;
char		escape_char		= '%';
bool		use_utf8		= true;
enumOFT		output_file_type	= OFT_UNKNOWN;
int		opt_truncate		= 0;
int		opt_split		= 0;
u64		opt_split_size		= 0;
ccp		opt_clone		= 0;
int		testmode		= 0;
ccp		opt_dest		= 0;
bool		opt_mkdir		= false;
int		opt_limit		= -1;
int		print_sections		= 0;
int		long_count		= 0;
int		ignore_count		= 0;
u32		job_limit		= ~(u32)0;
enumIOMode	io_mode			= 0;
bool		opt_no_expand		= false;
u32		opt_recurse_depth	= DEF_RECURSE_DEPTH;
PreallocMode	prealloc_mode		= PREALLOC_DEFAULT;

StringField_t	source_list;
StringField_t	recurse_list;
StringField_t	created_files;
char		iobuf [0x400000];		// global io buffer
const char	zerobuf[0x40000]	= {0};	// global zero buffer

// 'tempbuf' is only for short usage
//	==> don't call other functions while using tempbuf
u8		* tempbuf		= 0;	// global temp buffer -> AllocTempBuffer()
size_t		tempbuf_size		= 0;	// size of 'tempbuf'

const char sep_79[80] =		//  79 * '-' + NULL
	"----------------------------------------"
	"---------------------------------------";

///////////////////////////////////////////////////////////////////////////////

static void sig_handler ( int signum )
{
    static const char usr_msg[] =
	"\n"
	"****************************************************************\n"
	"***  SIGNAL USR%d CATCHED: VERBOSE LEVEL %s TO %-4d    ***\n"
	"***  THE EFFECT IS DELAYED UNTIL BEGINNING OF THE NEXT JOB.  ***\n"
	"****************************************************************\n";

    fflush(stdout);
    switch(signum)
    {
      case SIGINT:
      case SIGTERM:
	SIGINT_level++;
	TRACE("#SIGNAL# INT/TERM, level = %d\n",SIGINT_level);

	switch(SIGINT_level)
	{
	  case 1:
	    fprintf(stderr,
		"\n"
		"****************************************************************\n"
		"***  PROGRAM INTERRUPTED BY USER (LEVEL=1).                  ***\n"
		"***  PROGRAM WILL TERMINATE AFTER CURRENT JOB HAS FINISHED.  ***\n"
		"****************************************************************\n" );
	    break;

	  case 2:
	    fprintf(stderr,
		"\n"
		"*********************************************************\n"
		"***  PROGRAM INTERRUPTED BY USER (LEVEL=2).           ***\n"
		"***  PROGRAM WILL TRY TO TERMINATE NOW WITH CLEANUP.  ***\n"
		"*********************************************************\n" );
	    break;

	  default:
	    fprintf(stderr,
		"\n"
		"*************************************************************\n"
		"***  PROGRAM INTERRUPTED BY USER (LEVEL>=3).              ***\n"
		"***  PROGRAM WILL TERMINATE IMMEDIATELY WITHOUT CLEANUP.  ***\n"
		"*************************************************************\n" );
	    fflush(stderr);
	    exit(ERR_INTERRUPT);
	  }
	  break;

      case SIGUSR1:
	if ( verbose >= -1 )
	    verbose--;
	TRACE("#SIGNAL# USR1: verbose = %d\n",verbose);
	fprintf(stderr,usr_msg,1,"DECREASED",verbose);
	break;

      case SIGUSR2:
	if ( verbose < 4 )
	    verbose++;
	TRACE("#SIGNAL# USR2: verbose = %d\n",verbose);
	fprintf(stderr,usr_msg,2,"INCREASED",verbose);
	break;

      default:
	TRACE("#SIGNAL# %d\n",signum);
    }
    fflush(stderr);
}

///////////////////////////////////////////////////////////////////////////////

void SetupLib ( int argc, char ** argv, ccp p_progname, enumProgID prid )
{
 #ifdef DEBUG
    if (!TRACE_FILE)
    {
	char fname[100];
	snprintf(fname,sizeof(fname),"_trace-%s.tmp",p_progname);
	TRACE_FILE = fopen(fname,"w");
	if (!TRACE_FILE)
	    fprintf(stderr,"open TRACE_FILE failed: %s\n",fname);
    }
 #endif

 #ifdef __BYTE_ORDER
    TRACE("__BYTE_ORDER=%d\n",__BYTE_ORDER);
 #endif
 #ifdef LITTLE_ENDIAN
    TRACE("LITTLE_ENDIAN=%d\n",LITTLE_ENDIAN);
 #endif
 #ifdef BIG_ENDIAN
    TRACE("BIG_ENDIAN=%d\n",BIG_ENDIAN);
 #endif

    // numeric types

    TRACE("-\n");
    TRACE_SIZEOF(bool);
    TRACE_SIZEOF(short);
    TRACE_SIZEOF(int);
    TRACE_SIZEOF(long);
    TRACE_SIZEOF(long long);
    TRACE_SIZEOF(size_t);
    TRACE_SIZEOF(off_t);

    TRACE_SIZEOF(u8);
    TRACE_SIZEOF(u16);
    TRACE_SIZEOF(u32);
    TRACE_SIZEOF(u64);
    TRACE_SIZEOF(s8);
    TRACE_SIZEOF(s16);
    TRACE_SIZEOF(s32);
    TRACE_SIZEOF(s64);

    // base types A-Z

    TRACE("-\n");
    TRACE_SIZEOF(AWData_t);
    TRACE_SIZEOF(AWRecord_t);
    TRACE_SIZEOF(CISO_Head_t);
    TRACE_SIZEOF(CISO_Info_t);
    TRACE_SIZEOF(CheckDisc_t);
    TRACE_SIZEOF(CheckWBFS_t);
    TRACE_SIZEOF(CommandTab_t);
    TRACE_SIZEOF(DataArea_t);
    TRACE_SIZEOF(DataList_t);
    TRACE_SIZEOF(CommandTab_t);
    TRACE_SIZEOF(FileAttrib_t);
    TRACE_SIZEOF(FileCache_t);
    TRACE_SIZEOF(FilePattern_t);
    TRACE_SIZEOF(File_t);
    TRACE_SIZEOF(ID_DB_t);
    TRACE_SIZEOF(ID_t);
    TRACE_SIZEOF(InfoCommand_t);
    TRACE_SIZEOF(InfoOption_t);
    TRACE_SIZEOF(InfoUI_t);
    TRACE_SIZEOF(IOData_t);
    TRACE_SIZEOF(IsoMappingItem_t);
    TRACE_SIZEOF(IsoMapping_t);
    TRACE_SIZEOF(Iterator_t);
    TRACE_SIZEOF(MemMapItem_t);
    TRACE_SIZEOF(MemMap_t);
    TRACE_SIZEOF(ParamList_t);
    TRACE_SIZEOF(PartitionInfo_t);
    TRACE_SIZEOF(PrintTime_t);
    TRACE_SIZEOF(RegionInfo_t);
    TRACE_SIZEOF(StringField_t);
    TRACE_SIZEOF(StringItem_t);
    TRACE_SIZEOF(StringList_t);
    TRACE_SIZEOF(SubstString_t);
    TRACE_SIZEOF(SuperFile_t);
    TRACE_SIZEOF(TDBfind_t);
    TRACE_SIZEOF(Verify_t);
    TRACE_SIZEOF(WBFS_t);
    TRACE_SIZEOF(WDF_Chunk_t);
    TRACE_SIZEOF(WDF_Head_t);
    TRACE_SIZEOF(WDiscInfo_t);
    TRACE_SIZEOF(WDiscListItem_t);
    TRACE_SIZEOF(WDiscList_t);
    TRACE_SIZEOF(WiiFstFile_t);
    TRACE_SIZEOF(WiiFstInfo_t);
    TRACE_SIZEOF(WiiFstPart_t);
    TRACE_SIZEOF(WiiFst_t);

    // base types a-z

    TRACE("-\n");
    TRACE_SIZEOF(aes_key_t);

    TRACE_SIZEOF(cert_chain_t);
    TRACE_SIZEOF(cert_data_t);
    TRACE_SIZEOF(cert_head_t);
    TRACE_SIZEOF(cert_item_t);
    TRACE_SIZEOF(cert_stat_flags_t);
    TRACE_SIZEOF(cert_stat_t);

    TRACE_SIZEOF(dcUnicodeTripel);
    TRACE_SIZEOF(dol_header_t);
    TRACE_SIZEOF(id6_t);
    TRACE_SIZEOF(sha1_hash);

    TRACE_SIZEOF(wbfs_disc_info_t);
    TRACE_SIZEOF(wbfs_disc_t);
    TRACE_SIZEOF(wbfs_head_t);
    TRACE_SIZEOF(wbfs_inode_info_t);
    TRACE_SIZEOF(wbfs_param_t);
    TRACE_SIZEOF(wbfs_t);

    TRACE_SIZEOF(wd_boot_t);
    TRACE_SIZEOF(wd_compression_t);
    TRACE_SIZEOF(wd_disc_t);
    TRACE_SIZEOF(wd_file_list_t);
    TRACE_SIZEOF(wd_file_t);
    TRACE_SIZEOF(wd_fst_item_t);
    TRACE_SIZEOF(wd_header_128_t);
    TRACE_SIZEOF(wd_header_t);
    TRACE_SIZEOF(wd_icm_t);
    TRACE_SIZEOF(wd_ipm_t);
    TRACE_SIZEOF(wd_iterator_t);
    TRACE_SIZEOF(wd_memmap_item_t);
    TRACE_SIZEOF(wd_memmap_t);
    TRACE_SIZEOF(wd_modify_t);
    TRACE_SIZEOF(wd_part_control_t);
    TRACE_SIZEOF(wd_part_header_t);
    TRACE_SIZEOF(wd_part_sector_t);
    TRACE_SIZEOF(wd_part_t);
    TRACE_SIZEOF(wd_part_type_t);
    TRACE_SIZEOF(wd_patch_mode_t);
    TRACE_SIZEOF(wd_pfst_t);
    TRACE_SIZEOF(wd_pname_mode_t);
    TRACE_SIZEOF(wd_print_fst_t);
    TRACE_SIZEOF(wd_ptab_info_t);
    TRACE_SIZEOF(wd_ptab_entry_t);
    TRACE_SIZEOF(wd_ptab_t);
    TRACE_SIZEOF(wd_region_t);
    TRACE_SIZEOF(wd_reloc_t);
    TRACE_SIZEOF(wd_select_item_t);
    TRACE_SIZEOF(wd_select_mode_t);
    TRACE_SIZEOF(wd_select_t);
    TRACE_SIZEOF(wd_ticket_t);
    TRACE_SIZEOF(wd_tmd_content_t);
    TRACE_SIZEOF(wd_tmd_t);
    TRACE_SIZEOF(wd_usage_t);

    TRACE_SIZEOF(wia_controller_t);
    TRACE_SIZEOF(wia_segment_t);
    TRACE_SIZEOF(wia_disc_t);
    TRACE_SIZEOF(wia_exception_t);
    TRACE_SIZEOF(wia_except_list_t);
    TRACE_SIZEOF(wia_file_head_t);
    TRACE_SIZEOF(wia_group_t);
    TRACE_SIZEOF(wia_part_data_t);
    TRACE_SIZEOF(wia_part_t);
    TRACE_SIZEOF(wia_raw_data_t);

    // assertions

    TRACE("-\n");
    ASSERT( 1 == sizeof(u8)  );
    ASSERT( 2 == sizeof(u16) );
    ASSERT( 4 == sizeof(u32) );
    ASSERT( 8 == sizeof(u64) );
    ASSERT( 1 == sizeof(s8)  );
    ASSERT( 2 == sizeof(s16) );
    ASSERT( 4 == sizeof(s32) );
    ASSERT( 8 == sizeof(s64) );

    ASSERT( Count1Bits32(sizeof(WDF_Hole_t)) == 1 );
    ASSERT( sizeof(CISO_Head_t) == CISO_HEAD_SIZE );

    ASSERT(  79 == strlen(sep_79) );
    ASSERT( 200 == strlen(wd_sep_200) );

    validate_file_format_sizes(1);

    //----- setup textmode for cygwin stdout+stderr

    #if defined(__CYGWIN__)
	setmode(fileno(stdout),O_TEXT);
	setmode(fileno(stderr),O_TEXT);
	//setlocale(LC_ALL,"en_US.utf-8");
    #endif


    //----- setup prog id

    prog_id = prid;

    #ifdef WIIMM_TRUNK
	revision_id = (prid << 20) + SYSTEMID + REVISION_NUM + REVID_WIIMM_TRUNK;
    #elif defined(WIIMM)
	revision_id = (prid << 20) + SYSTEMID + REVISION_NUM + REVID_WIIMM;
    #else
	revision_id = (prid << 20) + SYSTEMID + REVISION_NUM + REVID_UNKNOWN;
    #endif


    //----- setup progname

    if ( argc > 0 && *argv && **argv )
	p_progname = *argv;
    progname = strrchr(p_progname,'/');
    progname = progname ? progname+1 : p_progname;
    argv[0] = (char*)progname;

    TRACE("##PROG## REV-ID=%08x, PROG-ID=%d, PROG-NAME=%s\n",
		revision_id, prog_id, progname );


    //----- setup signals

    static const int sigtab[] = { SIGTERM, SIGINT, SIGUSR1, SIGUSR2, -1 };
    int i;
    for ( i = 0; sigtab[i] >= 0; i++ )
    {
	struct sigaction sa;
	memset(&sa,0,sizeof(sa));
	sa.sa_handler = &sig_handler;
	sigaction(sigtab[i],&sa,0);
    }


    //----- setup search_path

    ccp *sp = search_path, *sp2;
    ASSERT( sizeof(search_path)/sizeof(*search_path) > 4 );

    // determine program path
    char proc_path[30];
    snprintf(proc_path,sizeof(proc_path),"/proc/%u/exe",getpid());
    TRACE("PROC-PATH: %s\n",proc_path);

    static char share[] = "/share/wit/";
    static char local_share[] = "/usr/local/share/wit/";

    char path[PATH_MAX];
    if (readlink(proc_path,path,sizeof(path)))
    {
	// program path found!
	TRACE("PROG-PATH: %s\n",path);

	char * file_ptr = strrchr(path,'/');
	if ( file_ptr )
	{
	    // seems to be a real path -> terminate string behind '/'
	    *++file_ptr = 0;
	    *sp = strdup(path);
	    if (!*sp)
		OUT_OF_MEMORY;
	    TRACE("SEARCH_PATH[%zd] = %s\n",sp-search_path,*sp);
	    sp++;

	    if ( file_ptr-5 >= path && !memcmp(file_ptr-4,"/bin/",5) )
	    {
		StringCopyS(file_ptr-5,sizeof(path),share);
		*sp = strdup(path);
		if (!*sp)
		    OUT_OF_MEMORY;
		TRACE("SEARCH_PATH[%zd] = %s\n",sp-search_path,*sp);
		sp++;
	    }
	}
    }

    // insert 'local_share' if not already done

    for ( sp2 = search_path; sp2 < sp && strcmp(*sp2,local_share); sp2++ )
	;
    if ( sp2 == sp )
    {
	*sp = strdup(local_share);
	if (!*sp)
	    OUT_OF_MEMORY;
	TRACE("SEARCH_PATH[%zd] = %s\n",sp-search_path,*sp);
	sp++;
    }

    // insert CWD if not already done
    getcwd(path,sizeof(path)-1);
    strcat(path,"/");
    for ( sp2 = search_path; sp2 < sp && strcmp(*sp2,path); sp2++ )
	;
    if ( sp2 == sp )
    {
	*sp = strdup("./");
	if (!*sp)
	    OUT_OF_MEMORY;
	TRACE("SEARCH_PATH[%zd] = %s\n",sp-search_path,*sp);
	sp++;
    }

    *sp = 0;
    ASSERT( sp - search_path < sizeof(search_path)/sizeof(*search_path) );


    //----- setup language info

    char * wit_lang = getenv("WIT_LANG");
    if ( wit_lang && *wit_lang )
    {
	lang_info = strdup(wit_lang);
	TRACE("LANG_INFO = %s [WIT_LANG]\n",lang_info);
    }
    else
    {
	char * lc_ctype = getenv("LC_CTYPE");
	if (lc_ctype)
	{
	    char * lc_ctype_end = lc_ctype;
	    while ( *lc_ctype_end >= 'a' && *lc_ctype_end <= 'z' )
		lc_ctype_end++;
	    const int len = lc_ctype_end - lc_ctype;
	    if ( len > 0 )
	    {
		char * temp = malloc(len+1);
		if (!temp)
		    OUT_OF_MEMORY;
		memcpy(temp,lc_ctype,len);
		temp[len] = 0;
		lang_info = temp;
		TRACE("LANG_INFO = %s\n",lang_info);
	    }
	}
    }


    //----- setup common key
    
    {
	static u8 key_base[] = "Wiimms WBFS Tool";
	static u8 key_mask[WD_CKEY__N][WII_KEY_SIZE] =
	{
	    {
		0x25, 0x00, 0x94, 0x12,  0x2a, 0x3e, 0x4e, 0x1f,
		0x03, 0x2d, 0xc5, 0xc9,  0x30, 0x2a, 0x58, 0xab
	    },
	    {
		0xad, 0x5c, 0x95, 0x84,  0x80, 0xda, 0x93, 0xd5,
		0x58, 0x06, 0xfe, 0x77,  0xf9, 0xe7, 0x69, 0x22
	    },
	};

	u8 h1[WII_HASH_SIZE], h2[WII_HASH_SIZE], key[WII_KEY_SIZE];

	SHA1(key_base,WII_KEY_SIZE,h1);
	int ci;
	for ( ci = 0; ci < WD_CKEY__N; ci++ )
	{
	    SHA1(wd_get_common_key(ci),WII_KEY_SIZE,h2);

	    int i;
	    for ( i = 0; i < WII_KEY_SIZE; i++ )
		key[i] = key_mask[ci][i] ^ h1[i] ^ h2[i];

	    wd_set_common_key(ci,key);
	}
    }

    //----- setup data structures

    InitializeAllFilePattern();
    wd_initialize_select(&part_selector);


    //----- verify oft_info

 #if defined(TEST) || defined(DEBUG)
    {
	ASSERT( OFT__N + 1 == sizeof(oft_info)/sizeof(*oft_info) );
	enumOFT oft;
	for ( oft = 0; oft <= OFT__N; oft++ )
	{
	    ASSERT( oft_info[oft].oft == oft );
	}
    }
 #endif
}

///////////////////////////////////////////////////////////////////////////////

void CloseAll()
{
    CloseWBFSCache();
}

///////////////////////////////////////////////////////////////////////////////

enumError CheckEnvOptions ( ccp varname, check_opt_func func )
{
    TRACE("CheckEnvOptions(%s,%p)\n",varname,func);

    ccp env = getenv(varname);
    if ( !env || !*env )
	return ERR_OK;

    TRACE("env[%s] = %s\n",varname,env);

    const int envlen = strlen(env);
    char * buf = malloc(envlen+1);
    if (!buf)
	OUT_OF_MEMORY;
    char * dest = buf;

    int argc = 1; // argv[0] = progname
    ccp src = env;
    while (*src)
    {
	while ( *src > 0 && *src <= ' ' ) // skip blanks & control
	    src++;

	if (!*src)
	    break;

	argc++;
	while ( *(u8*)src > ' ' )
	    *dest++ = *src++;
	*dest++ = 0;
	ASSERT( dest <= buf+envlen+1 );
    }
    TRACE("argc = %d\n",argc);

    char ** argv = malloc((argc+1)*sizeof(*argv));
    if (!argv)
	OUT_OF_MEMORY;
    argv[0] = (char*)progname;
    argv[argc] = 0;
    dest = buf;
    int i;
    for ( i = 1; i < argc; i++ )
    {
	TRACE("argv[%d] = %s\n",i,dest);
	argv[i] = dest;
	while (*dest)
	    dest++;
	dest++;
	ASSERT( dest <= buf+envlen+1 );
    }

    enumError stat = func(argc,argv,true);
    if (stat)
	fprintf(stderr,
	    "Errors above while scanning the environment variable '%s'\n",varname);

    // don't free() because is's possible that there are pointers to arguments
    //free(argv);
    //free(buf);

    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			error messages			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetErrorName ( int stat )
{
    switch(stat)
    {
	case ERR_OK:			return "OK";
	case ERR_DIFFER:		return "FILES DIFFER";
	case ERR_NOTHING_TO_DO:		return "NOTHING TO DO";
	case ERR_NO_SOURCE_FOUND:	return "NO SOURCE FOUND";
	case ERR_JOB_IGNORED:		return "JOB IGNORED";
	case ERR_WARNING:		return "WARNING";

	case ERR_INVALID_FILE:		return "INVALID FILE";

	case ERR_NO_WDF:		return "NO WDF";
	case ERR_WDF_VERSION:		return "WDF VERSION NOT SUPPORTED";
	case ERR_WDF_SPLIT:		return "SPLITTED WDF NOT SUPPORTED";
	case ERR_WDF_INVALID:		return "INVALID WDF";

	case ERR_NO_CISO:		return "NO WDF";
	case ERR_CISO_INVALID:		return "INVALID WDF";

	case ERR_WPART_INVALID:		return "INVALID WII PARTITION";
	case ERR_WDISC_INVALID:		return "INVALID WII DISC";
	case ERR_WDISC_NOT_FOUND:	return "WII DISC NOT FOUND";

	case ERR_NO_WBFS_FOUND:		return "NO WBFS FOUND";
	case ERR_TO_MUCH_WBFS_FOUND:	return "TO MUCH WBFS FOUND";
	case ERR_WBFS_INVALID:		return "INVALID WBFS";

	case ERR_NO_WIA:		return "NO WIA FOUND";
	case ERR_WIA_INVALID:		return "INVALID WIA";
	case ERR_BZIP2:			return "BZIP2 ERROR";
	case ERR_LZMA:			return "LZMA ERROR";

	case ERR_ALREADY_EXISTS:	return "FILE ALREADY EXISTS";
	case ERR_CANT_OPEN:		return "CAN'T OPEN FILE";
	case ERR_CANT_CREATE:		return "CAN'T CREATE FILE";
	case ERR_CANT_CREATE_DIR:	return "CAN'T CREATE DIRECTORY";
	case ERR_WRONG_FILE_TYPE:	return "WRONG FILE TYPE";
	case ERR_READ_FAILED:		return "READ FILE FAILED";
	case ERR_REMOVE_FAILED:		return "REMOVE FILE FAILED";
	case ERR_WRITE_FAILED:		return "WRITE FILE FAILED";

	case ERR_WBFS:			return "WBFS ERROR";

	case ERR_MISSING_PARAM:		return "MISSING PARAMETERS";
	case ERR_SEMANTIC:		return "SEMANTIC ERROR";
	case ERR_SYNTAX:		return "SYNTAX ERROR";

	case ERR_INTERRUPT:		return "INTERRUPT";

	case ERR_ERROR:			return "ERROR";

	case ERR_NOT_IMPLEMENTED:	return "NOT IMPLEMENTED YET";
	case ERR_INTERNAL:		return "INTERNAL ERROR";
	case ERR_OUT_OF_MEMORY:		return "OUT OF MEMORY";
	case ERR_FATAL:			return "FATAL ERROR";
    }
    return "?";
}

///////////////////////////////////////////////////////////////////////////////

ccp GetErrorText ( int stat )
{
    switch(stat)
    {
	case ERR_OK:			return "Ok";
	case ERR_DIFFER:		return "Files differ";
	case ERR_NOTHING_TO_DO:		return "Nothing to do";
	case ERR_NO_SOURCE_FOUND:	return "No source file found";
	case ERR_JOB_IGNORED:		return "Job ignored";
	case ERR_WARNING:		return "Unspecific warning";

	case ERR_INVALID_FILE:		return "File has invalid content";

	case ERR_NO_WDF:		return "File is not a WDF";
	case ERR_WDF_VERSION:		return "WDF version not supported";
	case ERR_WDF_SPLIT:		return "Splitted WDF not supported";
	case ERR_WDF_INVALID:		return "File is an invalid WDF";

	case ERR_NO_CISO:		return "File is not a CISO";
	case ERR_CISO_INVALID:		return "File is an invalid CISO";

	case ERR_WPART_INVALID:		return "Invalid Wii partition";
	case ERR_WDISC_INVALID:		return "Invalid Wii disc";
	case ERR_WDISC_NOT_FOUND:	return "Wii disc not found";

	case ERR_NO_WBFS_FOUND:		return "No WBFS found";
	case ERR_TO_MUCH_WBFS_FOUND:	return "To much WBFS found";
	case ERR_WBFS_INVALID:		return "Invalid WBFS";

	case ERR_NO_WIA:		return "File is not a WIA";
	case ERR_WIA_INVALID:		return "File is an invalid WIA";
	case ERR_BZIP2:			return "A bzip2 error occurred";
	case ERR_LZMA:			return "A lzma error occurred";

	case ERR_ALREADY_EXISTS:	return "File already exists";
	case ERR_CANT_OPEN:		return "Can't open file";
	case ERR_CANT_CREATE:		return "Can't create file";
	case ERR_CANT_CREATE_DIR:	return "Can't create directory";
	case ERR_WRONG_FILE_TYPE:	return "Wrong type of file";
	case ERR_READ_FAILED:		return "Reading from file failed";
	case ERR_REMOVE_FAILED:		return "Removing a file failed";
	case ERR_WRITE_FAILED:		return "Writing to file failed";

	case ERR_WBFS:			return "A WBFS error occurred";

	case ERR_MISSING_PARAM:		return "Missing ate least one parameter";
	case ERR_SEMANTIC:		return "Semantic error";
	case ERR_SYNTAX:		return "Syntax error";

	case ERR_INTERRUPT:		return "Program interrupted by user";

	case ERR_ERROR:			return "Unspecific error";

	case ERR_NOT_IMPLEMENTED:	return "Not implemented yet";
	case ERR_INTERNAL:		return "Internal error";
	case ERR_OUT_OF_MEMORY:		return "Allocation of dynamic memory failed";
	case ERR_FATAL:			return "Unspecific fatal error";
    }
    return "?";
}

///////////////////////////////////////////////////////////////////////////////

enumError last_error = ERR_OK;
enumError max_error  = ERR_OK;
u32 error_count = 0;

///////////////////////////////////////////////////////////////////////////////

int PrintError ( ccp func, ccp file, uint line,
		int syserr, enumError err_code, ccp format, ... )
{
    fflush(stdout);
    char msg[1000];
    const int plen = strlen(progname)+2;

    if (format)
    {
	va_list arg;
	va_start(arg,format);
	vsnprintf(msg,sizeof(msg),format,arg);
	msg[sizeof(msg)-2] = 0;
	va_end(arg);

	const int mlen = strlen(msg);
	if ( mlen > 0 && msg[mlen-1] != '\n' )
	{
	    msg[mlen]   = '\n';
	    msg[mlen+1] = 0;
	}
    }
    else
	StringCat2S(msg,sizeof(msg),GetErrorText(err_code),"\n");

    ccp prefix = err_code == ERR_OK ? "" : err_code <= ERR_WARNING ? "! " : "!! ";
    const int fw = GetTermWidth(80,40) - 1;

 #ifdef DEBUG
    TRACE("%s%s #%d [%s] in %s() @ %s#%d\n",
		prefix, err_code <= ERR_WARNING ? "WARNING" : "ERROR",
		err_code, GetErrorName(err_code), func, file, line );
    TRACE("%s%*s%s",prefix,plen,"",msg);
    if (syserr)
	TRACE("!! ERRNO=%d: %s\n",syserr,strerror(syserr));
    fflush(TRACE_FILE);
 #endif

 #if defined(EXTENDED_ERRORS)
    if ( err_code > ERR_WARNING )
 #else
    if ( err_code >= ERR_NOT_IMPLEMENTED )
 #endif
    {
	if ( err_code > ERR_WARNING )
	    fprintf(stderr,"%s%s: ERROR #%d [%s] in %s() @ %s#%d\n",
		prefix, progname, err_code, GetErrorName(err_code), func, file, line );
	else
	    fprintf(stderr,"%s%s: WARNING in %s() @ %s#%d\n",
		prefix, progname, func, file, line );

     #if defined(EXTENDED_ERRORS) && EXTENDED_ERRORS > 1
	fprintf(stderr,"%s -> %s/%s?annotate=%d#l%d\n",
		prefix, URI_VIEWVC, file, REVISION_NEXT, line );
     #endif

	fputs(prefix,stderr);
	PutLines(stderr,plen,fw,0,prefix,msg);
    }
    else
    {
	fprintf(stderr,"%s%s:",prefix,progname);
	PutLines(stderr,plen,fw,strlen(progname)+1,prefix,msg);
    }

    if (syserr)
    {
	fprintf(stderr,"%s%*s-> ",prefix,plen,"");
	snprintf(msg,sizeof(msg),"%s [%d]",strerror(syserr),syserr);
	PutLines(stderr,plen+3,fw,plen+3,prefix,msg);
    }
    fflush(stderr);

    if ( err_code > ERR_OK )
	error_count++;

    last_error = err_code;
    if ( max_error < err_code )
	max_error = err_code;

    if ( err_code > ERR_NOT_IMPLEMENTED )
	exit(err_code);

    return err_code;
}

///////////////////////////////////////////////////////////////////////////////

void HexDump16 ( FILE * f, int indent, u64 addr,
		 const void * data, size_t count )
{
    HexDump(f,indent,addr,4,16,data,count);
}

//-----------------------------------------------------------------------------

void HexDump ( FILE * f, int indent, u64 addr, int addr_fw, int row_len,
		const void * p_data, size_t count )
{
    if ( !f || !p_data || !count )
	return;

    const int MAX_LEN = 256;
    char buf[MAX_LEN+1];
    
    const u8 * data = p_data;

    indent = NormalizeIndent(indent);
    addr_fw = NormalizeIndent(addr_fw);

    const bool show_ascii = row_len >= 0;
    if ( row_len < 0 )
	row_len = -row_len;
    else if ( row_len < 1 )
	row_len = 16;
    else if ( row_len > MAX_LEN )
	row_len = MAX_LEN;

    const int fw = snprintf(buf,sizeof(buf),"%llx",addr+count-1);
    if ( addr_fw < fw )
	 addr_fw = fw;

    while ( count > 0 )
    {
	fprintf(f,"%*s%*llx:", indent,"", addr_fw, addr );
	addr += row_len;
	char * dest = buf;

	int i;
	for ( i = 0; i < row_len; i++ )
	{
	    u8 ch = *data++;
	    if ( count > 0 )
	    {
		count--;
		fprintf(f,"%s%02x ", i&3 ? "" : " ", ch );
		*dest++ = ch < ' ' || ch >= 0x7f ? '.' : ch;
	    }
	    else
		fprintf(f,"%s   ", i&3 ? "" : " " );
	}
	*dest = 0;
	if (show_ascii)
	    fprintf(f,":%s:\n",buf);
	else
	    fputc('\n',f);
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			terminal cap			///////////////
///////////////////////////////////////////////////////////////////////////////

u32 opt_width = 0;

///////////////////////////////////////////////////////////////////////////////

int ScanOptWidth ( ccp source )
{
    return ERR_OK != ScanSizeOptU32(
			&opt_width,		// u32 * num
			source,			// ccp source
			1,			// default_factor1
			0,			// int force_base
			"width",		// ccp opt_name
			40,			// u64 min
			10000,			// u64 max
			1,			// u32 multiple
			0,			// u32 pow2
			true			// bool print_err
			);
}

///////////////////////////////////////////////////////////////////////////////

int GetTermWidth ( int default_value, int min_value )
{
    int term_width = opt_width > 0
			? opt_width
			: GetTermWidthFD(STDOUT_FILENO,-1,min_value);
    if ( term_width <= 0 )
	term_width = GetTermWidthFD(STDERR_FILENO,-1,min_value);

    return term_width > 0 ? term_width : default_value;
}

///////////////////////////////////////////////////////////////////////////////

int GetTermWidthFD ( int fd, int default_value, int min_value )
{
    TRACE("GetTermWidthFD(%d,%d)\n",fd,default_value);

 #ifdef TIOCGSIZE
    TRACE(" - have TIOCGSIZE\n");
 #endif

 #ifdef TIOCGWINSZ
    TRACE(" - have TIOCGWINSZ\n");
 #endif

    if (isatty(fd))
    {
     #ifdef TIOCGSIZE
	{
	    struct ttysize ts;
	    if ( !ioctl(fd,TIOCGSIZE,&ts))
	    {
		TRACE(" - TIOCGSIZE = %d*%d\n",ts.ts_cols,ts.ts_lines);
		if ( ts.ts_cols > 0 )
		    return ts.ts_cols > min_value ? ts.ts_cols : min_value;
	    }
	}
     #endif

     #ifdef TIOCGWINSZ
	{
	    struct winsize ws;
	    if ( !ioctl(fd,TIOCGWINSZ,&ws))
	    {
		TRACE(" - TIOCGWINSZ = %d*%d\n",ws.ws_col,ws.ws_row);
		if ( ws.ws_col > 0 )
		    return ws.ws_col > min_value ? ws.ws_col : min_value;
	    }
	}
     #endif
    }

    return default_value ? default_value : opt_width;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    timer			///////////////
///////////////////////////////////////////////////////////////////////////////

u32 GetTimerMSec()
{
    struct timeval tval;
    gettimeofday(&tval,NULL);

    static time_t timebase = 0;
    if (!timebase)
	timebase = tval.tv_sec;

    return ( tval.tv_sec - timebase ) * 1000 + tval.tv_usec/1000;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintMSec ( char * buf, int bufsize, u32 msec, bool PrintMSec )
{
    if (PrintMSec)
	snprintf(buf,bufsize,"%02d:%02d:%02d.%03d",
	    msec/3600000, msec/60000%60, msec/1000%60, msec%1000 );
    else
	snprintf(buf,bufsize,"%02d:%02d:%02d",
	    msec/3600000, msec/60000%60, msec/1000%60 );
    ccp ptr = buf;
    int colon_counter = 0;
    while ( *ptr == '0' || *ptr == ':' && !colon_counter++ )
	ptr++;
    return *ptr == ':' ? ptr-1 : ptr;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			string functions		///////////////
///////////////////////////////////////////////////////////////////////////////

ccp PathCatPP ( char * buf, size_t bufsize, ccp path1, ccp path2 )
{
    // concatenate path + path

    if ( !path1 || !*path1 )
	return path2 ? path2 : "";

    if ( !path2 || !*path2 )
	return path1;

    char * ptr = StringCopyS(buf,bufsize-1,path1);
    DASSERT( ptr > buf );
    if ( ptr[-1] != '/' )
	*ptr++ = '/';
    while ( *path2 == '/' )
	path2++;
    StringCopyE(ptr,buf+bufsize,path2);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp PathCatPPE ( char * buf, size_t bufsize, ccp path1, ccp path2, ccp ext )
{
    // concatenate path + path + extension

    char * ptr = path1 ? StringCopyS(buf,bufsize-1,path1) : buf;
    if ( ptr > buf && ptr[-1] != '/' )
	*ptr++ = '/';

    if (path2)
    {
	while ( *path2 == '/' )
	    path2++;
	ptr = StringCopyE(ptr,buf+bufsize,path2);
    }

    if (ext)
	StringCopyE(ptr,buf+bufsize,ext);

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

int NormalizeIndent ( int indent )
{
    return indent < 0 ? 0 : indent < 50 ? indent : 50;
}

///////////////////////////////////////////////////////////////////////////////

int CheckIDHelper // helper for all other id functions
(
	const void	* id,		// valid pointer to test ID
	int		max_len,	// max length of ID, a NULL terminates ID too
	bool		allow_any_len,	// if false, only length 4 and 6 are allowed
	bool		ignore_case,	// lower case letters are allowed
	bool		allow_point	// the wildcard '.' is allowed
)
{
    ASSERT(id);
    ccp ptr = id;
    ccp end = ptr + max_len;
    while ( ptr != end && ( *ptr >= 'A' && *ptr <= 'Z'
			|| ignore_case && *ptr >= 'a' && *ptr <= 'z'
			|| *ptr >= '0' && *ptr <= '9'
			|| allow_point && *ptr == '.'
			|| *ptr == '_' ))
	ptr++;

    const int len = ptr - (ccp)id;
    return allow_any_len || len == 4 || len == 6 ? len : 0;
}

//-----------------------------------------------------------------------------

int CheckID // check up to 7 chars for ID4|ID6
(
	const void	* id,		// valid pointer to test ID
	bool		ignore_case,	// lower case letters are allowed
	bool		allow_point	// the wildcard '.' is allowed
)
{
    // check up to 7 chars
    return CheckIDHelper(id,7,false,ignore_case,allow_point);
}

//-----------------------------------------------------------------------------

bool CheckID4 // check exact 4 chars
(
	const void	* id,		// valid pointer to test ID
	bool		ignore_case,	// lower case letters are allowed
	bool		allow_point	// the wildcard '.' is allowed
)
{
    // check exact 4 chars
    return CheckIDHelper(id,4,false,ignore_case,allow_point) == 4;
}

//-----------------------------------------------------------------------------

bool CheckID6 // check exact 6 chars
(
	const void	* id,		// valid pointer to test ID
	bool		ignore_case,	// lower case letters are allowed
	bool		allow_point	// the wildcard '.' is allowed
)
{
    // check exact 6 chars
    return CheckIDHelper(id,6,false,ignore_case,allow_point) == 6;
}

//-----------------------------------------------------------------------------

int CountIDChars // count number of valid ID chars, max = 1000
(
	const void	* id,		// valid pointer to test ID
	bool		ignore_case,	// lower case letters are allowed
	bool		allow_point	// the wildcard '.' is allowed
)
{
    // count number of valid ID chars
    return CheckIDHelper(id,1000,true,ignore_case,allow_point);
}

//-----------------------------------------------------------------------------

char * ScanID ( char * destbuf7, int * destlen, ccp source )
{
    ASSERT(destbuf7);

    memset(destbuf7,0,7);
    ccp id_start = 0;
    if (source)
    {
	ccp src = source;

	// skip CTRL and SPACES
	while ( *src > 0 && *src <= ' ' )
	    src++;

	if ( ( *src == '*' || *src == '+' ) && ( !src[1] || src[1] == '=' ) )
	{
	    if (destlen)
		*destlen = 1;
	    return src[1] == '=' && src[2] ? (char*)src+2 : 0;
	}

	// scan first word
	const int id_len = CheckID(src,false,false);

	if ( id_len == 4 )
	{
	    TRACE("4 chars found:%.6s\n",id_start);
	    id_start = src;
	    src += id_len;

	    // skip CTRL and SPACES
	    while ( *src > 0 && *src <= ' ' )
		src++;

	    if (!*src)
	    {
		memcpy(destbuf7,id_start,4);
		if (destlen)
		    *destlen = 4;
		return 0;
	    }
	    id_start = 0;
	}
	else if ( id_len == 6 )
	{
	    // we have found an ID6 canidat
	    TRACE("6 chars found:%.6s\n",id_start);
	    id_start = src;
	    src += id_len;

	    // skip CTRL and SPACES
	    while ( *src > 0 && *src <= ' ' )
		src++;

	    if ( !*src || *src == '=' )
	    {
		if ( *src == '=' )
		    src++;

		// pure 'ID6' or 'ID6 = name found
		memcpy(destbuf7,id_start,6);
		if (destlen)
		    *destlen = 6;
		return *src ? (char*)src : 0;
	    }
	}

	// scan for latest '...[ID6]...'
	while (*src)
	{
	    while ( *src && *src != '[' ) // ]
		src++;

	    if ( *src == '[' && src[7] == ']' && CheckID(++src,false,false) == 6 )
	    {
		id_start = src;
		src += 8;
	    }
	    if (*src)
		src++;
	}
    }
    if (id_start)
	memcpy(destbuf7,id_start,6);
    if (destlen)
	*destlen = id_start ? 6 : 0;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    time printing & scanning		///////////////
///////////////////////////////////////////////////////////////////////////////

int opt_print_time  = PT__DEFAULT;
int opt_time_select = PT__DEFAULT & PT__USE_MASK;

///////////////////////////////////////////////////////////////////////////////

int ScanPrintTimeMode ( ccp arg, int prev_mode )
{
    #undef E
    #undef EM
    #undef IT
    #undef MT
    #undef CT
    #undef AT

    #define E PT_ENABLED
    #define EM PT__ENABLED_MASK
    #define IT PT_USE_ITIME|PT_F_ITIME
    #define MT PT_USE_MTIME|PT_F_MTIME
    #define CT PT_USE_CTIME|PT_F_CTIME
    #define AT PT_USE_ATIME|PT_F_ATIME

    static const CommandTab_t tab[] =
    {
	{ 0,			"RESET",	"-",	PT__MASK },

	{ PT_DISABLED,		"OFF",		0,	EM },
	{ PT_ENABLED,		"ON",		0,	EM },

	{ E|PT_SINGLE,		"SINGLE",	"1",	EM|PT__MULTI_MASK|PT__F_MASK },
	{ E|PT_MULTI,		"MULTI",	"+",	EM|PT__MULTI_MASK },

	{ E|PT_MULTI,		"NONE",		"0",	EM|PT__MULTI_MASK|PT__F_MASK },
	{ E|PT_MULTI|PT__F_MASK,"ALL",		"*",	EM|PT__MULTI_MASK|PT__F_MASK },

	{ E|IT,			"I",		0,	EM|PT__USE_MASK },
	{ E|MT,			"M",		0,	EM|PT__USE_MASK },
	{ E|CT,			"C",		0,	EM|PT__USE_MASK },
	{ E|AT,			"A",		0,	EM|PT__USE_MASK },

	{ E|PT_PRINT_DATE,	"DATE",		0,	EM|PT__PRINT_MASK },
	{ E|PT_PRINT_TIME,	"TIME",		"MIN",	EM|PT__PRINT_MASK },
	{ E|PT_PRINT_SEC,	"SEC",		0,	EM|PT__PRINT_MASK },

	{ E|IT|PT_PRINT_DATE,	"IDATE",	0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|MT|PT_PRINT_DATE,	"MDATE",	0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|CT|PT_PRINT_DATE,	"CDATE",	0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|AT|PT_PRINT_DATE,	"ADATE",	0,	EM|PT__USE_MASK|PT__PRINT_MASK },

	{ E|IT|PT_PRINT_TIME,	"ITIME",	"IMIN",	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|MT|PT_PRINT_TIME,	"MTIME",	"MMIN",	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|CT|PT_PRINT_TIME,	"CTIME",	"CMIN",	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|AT|PT_PRINT_TIME,	"ATIME",	"AMIN",	EM|PT__USE_MASK|PT__PRINT_MASK },

	{ E|IT|PT_PRINT_SEC,	"ISEC",		0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|MT|PT_PRINT_SEC,	"MSEC",		0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|CT|PT_PRINT_SEC,	"CSEC",		0,	EM|PT__USE_MASK|PT__PRINT_MASK },
	{ E|AT|PT_PRINT_SEC,	"ASEC",		0,	EM|PT__USE_MASK|PT__PRINT_MASK },

	{ 0,0,0,0 }
    };

    #undef E
    #undef EM
    #undef IT
    #undef MT
    #undef CT
    #undef AT

    const int stat = ScanCommandListMask(arg,tab);
    if ( stat >= 0 )
	return SetPrintTimeMode(prev_mode,stat);

    ERROR0(ERR_SYNTAX,"Illegal time mode (option --time): '%s'\n",arg);
    return PT__ERROR;
}

///////////////////////////////////////////////////////////////////////////////

int ScanAndSetPrintTimeMode ( ccp argv )
{
    const int stat = ScanPrintTimeMode(argv,opt_print_time);
    if ( stat >= 0 )
	opt_print_time  = stat;
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int SetPrintTimeMode ( int prev_mode, int new_mode )
{
    TRACE("SetPrintTimeMode(%x,%x)\n",prev_mode,new_mode);
    if ( new_mode & PT__USE_MASK )
	prev_mode = prev_mode & ~PT__USE_MASK | new_mode & PT__USE_MASK;

    prev_mode |= new_mode & PT__F_MASK;

    if ( new_mode & PT__MULTI_MASK )
	prev_mode = prev_mode & ~PT__MULTI_MASK | new_mode & PT__MULTI_MASK;

    if ( new_mode & PT__PRINT_MASK )
	prev_mode = prev_mode & ~PT__PRINT_MASK | new_mode & PT__PRINT_MASK;

    if ( new_mode & PT__ENABLED_MASK )
	prev_mode = prev_mode & ~PT__ENABLED_MASK | new_mode & PT__ENABLED_MASK;

    TRACE(" -> %x\n",prev_mode);
    return prev_mode;
}

///////////////////////////////////////////////////////////////////////////////

int EnablePrintTime ( int opt_time )
{
    return SetPrintTimeMode(PT__DEFAULT|PT_PRINT_DATE,opt_time|PT_ENABLED);
}

///////////////////////////////////////////////////////////////////////////////

void SetTimeOpt ( int opt_time )
{
    opt_print_time = SetPrintTimeMode( opt_print_time, opt_time|PT_ENABLED );
}

///////////////////////////////////////////////////////////////////////////////

void SetupPrintTime ( PrintTime_t * pt, int opt_time )
{
    TRACE("SetupPrintTime(%p,%x)\n",pt,opt_time);
    ASSERT(pt);
    memset(pt,0,sizeof(*pt));

    if ( opt_time & PT_SINGLE )
    {
	opt_time &= ~PT__F_MASK;
	switch( opt_time & PT__USE_MASK )
	{
	    case PT_USE_ITIME:	opt_time |= PT_F_ITIME; break;
	    case PT_USE_CTIME:	opt_time |= PT_F_CTIME; break;
	    case PT_USE_ATIME:	opt_time |= PT_F_ATIME; break;
	    default:		opt_time |= PT_F_MTIME; break;
	}
    }
    else if ( !(opt_time&PT__F_MASK) )
	opt_time |= PT_F_MTIME;

    ccp head_format;
    switch ( opt_time & (PT__ENABLED_MASK|PT__PRINT_MASK) )
    {
	case PT_ENABLED|PT_PRINT_SEC:
	    pt->format	= " %Y-%m-%d %H:%M:%S";
	    pt->undef	= " ---------- --:--:--";
	    head_format	= "   #-date    #-time ";
	    break;

	case PT_ENABLED|PT_PRINT_TIME:
	    pt->format	= " %Y-%m-%d %H:%M";
	    pt->undef	= " ---------- --:--";
	    head_format	= "   #-date  #-time";
	    break;

	case PT_ENABLED|PT_PRINT_DATE:
	    pt->format	= " %Y-%m-%d";
	    pt->undef	= " ----------";
	    head_format	= "   #-date  ";
	    break;

	default:
	    pt->format	= "";
	    pt->undef	= "";
	    head_format	= "";
	    opt_time &= ~PT__F_MASK;
	    break;
    }

    pt->mode = opt_time & PT__MASK;
    pt->wd1  = strlen(head_format);

    ASSERT(   pt->wd1 < PT_BUF_SIZE );
    ASSERT( 4*pt->wd1 < sizeof(pt->head) );
    ASSERT( 4*pt->wd1 < sizeof(pt->fill) );
    ASSERT( 4*pt->wd1 < sizeof(pt->tbuf) );

    pt->nelem = 0;
    char *head = pt->head, *fill = pt->fill;
    ccp mptr = "imca";
    while (*mptr)
    {
	char ch = *mptr++;
	if ( opt_time & PT_F_ITIME )
	{
	    ccp src;
	    for ( src = head_format; *src; src++ )
	    {
		*head++ = *src == '#' ? ch : *src;
		*fill++ = ' ';
	    }
	    pt->nelem++;
	}
	opt_time >>= 1;
    }
    *head = 0;
    *fill = 0;
    pt->wd  = pt->nelem * pt->wd1;

    TRACE(" -> head:   |%s|\n",pt->head);
    TRACE(" -> fill:   |%s|\n",pt->fill);
    TRACE(" -> format: |%s|\n",pt->format);
    TRACE(" -> undef:  |%s|\n",pt->undef);
}

///////////////////////////////////////////////////////////////////////////////

char * PrintTime ( PrintTime_t * pt, const FileAttrib_t * fa )
{
    ASSERT(pt);
    ASSERT(fa);

    if (!pt->wd)
	*pt->tbuf = 0;
    else
    {
	const time_t * timbuf[] = { &fa->itime, &fa->mtime, &fa->ctime, &fa->atime, 0 };
	const time_t ** timptr = timbuf;

	char *dest = pt->tbuf, *end = dest + sizeof(pt->tbuf);
	int mode;
	for ( mode = pt->mode; *timptr; timptr++, mode >>= 1 )
	    if ( mode & PT_F_ITIME )
	    {
		const time_t thetime = **timptr;
		if (!thetime)
		    dest = StringCopyE(dest,end,pt->undef);
		else
		{
		    struct tm * tm = localtime(&thetime);
		    dest += strftime(dest,end-dest,pt->format,tm);
		}
	    }
    }
    return pt->tbuf;
}

///////////////////////////////////////////////////////////////////////////////

time_t SelectTime ( const FileAttrib_t * fa, int opt_time )
{
    ASSERT(fa);
    switch ( opt_time & PT__USE_MASK )
    {
	case PT_USE_ITIME: return fa->itime;
	case PT_USE_CTIME: return fa->ctime;
	case PT_USE_ATIME: return fa->atime;
	default:	   return fa->mtime;
    }
}

///////////////////////////////////////////////////////////////////////////////

SortMode SelectSortMode ( int opt_time )
{
    switch ( opt_time & PT__USE_MASK )
    {
	case PT_USE_ITIME: return SORT_ITIME;
	case PT_USE_CTIME: return SORT_CTIME;
	case PT_USE_ATIME: return SORT_ATIME;
	default:	   return SORT_MTIME;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

time_t ScanTime ( ccp arg )
{
    static ccp tab[] =
    {
	"%Y-%m-%d %H:%M:%S",
	"%Y-%m-%d %H:%M",
	"%Y-%m-%d %H%M%S",
	"%Y-%m-%d %H%M",
	"%Y-%m-%d %H",
	"%Y-%m-%d",
	"%Y%m%d %H%M%S",
	"%Y%m%d %H%M",
	"%Y%m%d %H",
	"%Y%m%d",
	"%s",
	0
    };

    ccp * tptr;
    for ( tptr = tab; *tptr; tptr++ )
    {
	struct tm tm;
	memset(&tm,0,sizeof(tm));
	tm.tm_mon = 1;
	tm.tm_mday = 1;
	ccp res = strptime(arg,*tptr,&tm);
	if (res)
	{
	    while (isblank((int)*res))
		res++;
	    if (!*res)
	    {
		time_t tim = mktime(&tm);
		if ( tim != (time_t)-1 )
		    return tim;
	    }
	}
    }

    ERROR0(ERR_SYNTAX,"Illegal time format: %s",arg);
    return (time_t)-1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    scan size			///////////////
///////////////////////////////////////////////////////////////////////////////

u64 ScanSizeFactor ( char ch_factor, int force_base )
{
    if ( force_base == 1000 )
    {
	switch (ch_factor)
	{
	    case 'b': case 'c': return     1;
	    case 'k': case 'K': return KB_SI;
	    case 'm': case 'M': return MB_SI;
	    case 'g': case 'G': return GB_SI;
	    case 't': case 'T': return TB_SI;
	    case 'p': case 'P': return PB_SI;
	    case 'e': case 'E': return EB_SI;

	    case 'u': case 'U': return GC_DISC_SIZE;
	    case 'w': case 'W': return WII_SECTORS_SINGLE_LAYER *(u64)WII_SECTOR_SIZE;
	}
    }
    else if ( force_base == 1024 )
    {
	switch (ch_factor)
	{
	    case 'b': case 'c': return   1;
	    case 'k': case 'K': return KiB;
	    case 'm': case 'M': return MiB;
	    case 'g': case 'G': return GiB;
	    case 't': case 'T': return TiB;
	    case 'p': case 'P': return PiB;
	    case 'e': case 'E': return EiB;

	    case 'u': case 'U': return GC_DISC_SIZE;
	    case 'w': case 'W': return WII_SECTORS_SINGLE_LAYER *(u64)WII_SECTOR_SIZE;
	}
    }
    else
    {
	switch (ch_factor)
	{
	    case 'b':
	    case 'c': return 1;

	    case 'k': return KB_SI;
	    case 'm': return MB_SI;
	    case 'g': return GB_SI;
	    case 't': return TB_SI;
	    case 'p': return PB_SI;
	    case 'e': return EB_SI;

	    case 'K': return KiB;
	    case 'M': return MiB;
	    case 'G': return GiB;
	    case 'T': return TiB;
	    case 'P': return PiB;
	    case 'E': return EiB;

	    case 'u':
	    case 'U': return GC_DISC_SIZE;

	    case 'w':
	    case 'W': return WII_SECTORS_SINGLE_LAYER *(u64)WII_SECTOR_SIZE;
	}
    }
    return 0;
}

//-----------------------------------------------------------------------------

char * ScanSizeTerm ( double * num, ccp source, u64 default_factor, int force_base )
{
    ASSERT(source);

    char * end;
    double d = strtod(source,&end);
    if ( end > source )
    {
	// something was read
	u64 factor = ScanSizeFactor(*end,force_base);
	if (factor)
	    end++;
	else
	    factor = default_factor;

	if (factor)
	    d *= factor;
	else
	    end = (char*)source;
    }

    if (num)
	*num = d;

    return end;
}

//-----------------------------------------------------------------------------

char * ScanSize ( double * num, ccp source,
		  u64 default_factor1, u64 default_factor2, int force_base )
{
    DASSERT(source);
    TRACE("ScanSize(df=%llx,%llx, base=%u)\n",
			default_factor1, default_factor2, force_base );

    double sum = 0.0;
    bool add = true;
    char * end = 0;
    for (;;)
    {
	double term;
	end = ScanSizeTerm(&term,source,default_factor1,force_base);
	if ( end == source )
	    break;
	if (add)
	    sum += term;
	else
	    sum -= term;

	while ( *end > 0 && *end <= ' ' )
	    end++;

	if ( *end == '+' )
	    add = true;
	else if ( *end == '-' )
	    add = false;
	else
	    break;

	source = end+1;
	while ( *source > 0 && *source <= ' ' )
	    source++;

	if ( !*source && default_factor2 )
	{
	    if (add)
		sum += default_factor2;
	    else
		sum -= default_factor2;
	    end = (char*)source;
	    break;
	}

	default_factor1 = default_factor2;
    }

    if (num)
	*num = sum;

    return end;
}

//-----------------------------------------------------------------------------

char * ScanSizeU32 ( u32 * num, ccp source,
		     u64 default_factor1, u64 default_factor2, int force_base )
{
    double d;
    char * end = ScanSize(&d,source,default_factor1,default_factor2,force_base);
    //d = ceil(d+0.5);
    if ( d < 0 || d > ~(u32)0 )
	end = (char*)source;
    else if (num)
	*num = (u32)d;

    return end;
}

//-----------------------------------------------------------------------------

char * ScanSizeU64 ( u64 * num, ccp source,
		     u64 default_factor1, u64 default_factor2, int force_base )
{
    double d;
    char * end = ScanSize(&d,source,default_factor1,default_factor2,force_base);
    //d = ceil(d+0.5);
    if ( d < 0 || d > ~(u64)0 )
	end = (char*)source;
    else if (num)
	*num = (u64)d;

    return end;
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanSizeOpt
	( double * num, ccp source,
	  u64 default_factor1, u64 default_factor2, int force_base,
	  ccp opt_name, u64 min, u64 max, bool print_err )
{
    double d;
    char * end = ScanSize(&d,source,default_factor1,default_factor2,force_base);

 #ifdef DEBUG
    {
	u64 size = d;
	TRACE("--%s %8.6g ~ %llu ~ %llu GiB ~ %llu GB\n",
		opt_name, d, size, (size+GiB/2)/GiB, (size+500000000)/1000000000 );
    }
 #endif

    enumError err = ERR_OK;

    if ( source == end || *end )
    {
	err = ERR_SYNTAX;
	if (print_err)
	    ERROR0(ERR_SYNTAX,
			"Illegal number for option --%s: %s\n",
			opt_name, source );
    }
    else if ( min > 0 && d < min )
    {
	err = ERR_SYNTAX;
	if (print_err)
	    ERROR0(ERR_SEMANTIC,
			"--%s to small (must not <%llu): %s\n",
			opt_name, min, source );
    }
    else if ( max > 0 && d > max )
    {
	err = ERR_SYNTAX;
	if (print_err)
	    ERROR0(ERR_SEMANTIC,
			"--%s to large (must not >%llu): %s\n",
			opt_name, max, source );
    }

    if ( num && !err )
	*num = d;
    return err;
}

//-----------------------------------------------------------------------------

enumError ScanSizeOptU64
	( u64 * num, ccp source, u64 default_factor1, int force_base,
	  ccp opt_name, u64 min, u64 max, u32 multiple, u32 pow2, bool print_err )
{
    if (!max)
	max = ~(u64)0;

    if ( pow2 && !force_base )
    {
	// try base 1024 first without error messages
	u64 val;
	if (!ScanSizeOptU64( &val, source, default_factor1, 1024,
				opt_name, min,max, multiple, pow2, false ))
	{
	    if (num)
		*num = val;
	    return ERR_OK;
	}
    }

    double d = 0.0;
    enumError err = ScanSizeOpt(&d,source,default_factor1,
				multiple ? multiple : 1,
				force_base,opt_name,min,max,print_err);

    u64 val;
    if ( d < 0.0 )
    {
	val = 0;
	err = ERR_SEMANTIC;
	if (print_err)
	    ERROR0(ERR_SEMANTIC, "--%s: negative values not allowed: %s\n",
			opt_name, source );
    }
    else
	val = d;

    if ( err == ERR_OK && pow2 > 0 )
    {
	int shift_count = 0;
	u64 shift_val = val;
	if (val)
	{
	    while (!(shift_val&1))
	    {
		shift_count++;
		shift_val >>= 1;
	    }
	}

	if ( shift_val != 1 || shift_count/pow2*pow2 != shift_count )
	{
	    err = ERR_SEMANTIC;
	    if (print_err)
		ERROR0(ERR_SYNTAX,
			"--%s: value must be a power of %d but not %llu\n",
			opt_name, 1<<pow2, val );
	}
    }

    if ( err == ERR_OK && multiple > 1 )
    {
	u64 xval = val / multiple * multiple;
	if ( xval != val )
	{
	    if ( min > 0 && xval < min )
		xval += multiple;

	    if (print_err)
		ERROR0(ERR_WARNING,
			"--%s: value must be a multiple of %u -> use %llu instead of %llu.\n",
			opt_name, multiple, xval, val );
	    val = xval;
	}
    }

    if ( num && !err )
	*num = val;
    return err;
}

//-----------------------------------------------------------------------------

enumError ScanSizeOptU32
	( u32 * num, ccp source, u64 default_factor1, int force_base,
	  ccp opt_name, u64 min, u64 max, u32 multiple, u32 pow2, bool print_err )
{
    if ( !max || max > ~(u32)0 )
	max = ~(u32)0;

    u64 val;
    enumError err = ScanSizeOptU64( &val, source, default_factor1, force_base,
				opt_name, min, max, multiple, pow2, print_err );

    if ( num && !err )
	*num = (u32)val;
    return err;
}

///////////////////////////////////////////////////////////////////////////////

int ScanOptSplitSize ( ccp source )
{
    opt_split++;
    return ERR_OK != ScanSizeOptU64(
			&opt_split_size,	// u64 * num
			source,			// ccp source
			GiB,			// default_factor1
			0,			// int force_base
			"split-size",		// ccp opt_name
			MIN_SPLIT_SIZE,		// u64 min
			0,			// u64 max
			DEF_SPLIT_FACTOR,	// u32 multiple
			0,			// u32 pow2
			true			// bool print_err
			);
}

///////////////////////////////////////////////////////////////////////////////

int ScanOptRDepth ( ccp source )
{
    return ERR_OK != ScanSizeOptU32(
			&opt_recurse_depth,	// u32 * num
			source,			// ccp source
			1,			// default_factor1
			0,			// int force_base
			"rdepth",		// ccp opt_name
			0,			// u64 min
			MAX_RECURSE_DEPTH,	// u64 max
			0,			// u32 multiple
			0,			// u32 pow2
			true			// bool print_err
			);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  scan num/range		///////////////
///////////////////////////////////////////////////////////////////////////////

char * ScanNumU32 ( ccp arg, u32 * p_stat, u32 * p_num, u32 min, u32 max )
{
    ASSERT(arg);
    ASSERT(p_num);
    TRACE("ScanNumU32(%s)\n",arg);

    while ( *arg > 0 && *arg <= ' ' )
	arg++;

    char * end;
    u32 num = strtoul(arg,&end, arg[1] >= '0' && arg[1] <= '9' ? 10 : 0 );
    u32 stat = end > arg;
    if (stat)
    {
	if ( num < min )
	    num = min;
	else if ( num > max )
	    num = max;

	while ( *end > 0 && *end <= ' ' )
	    end++;
    }
    else
	num = 0;

    if (p_stat)
	*p_stat = stat;
    *p_num = num;

    TRACE("END ScanNumU32() stat=%u, n=%u ->%s\n",stat,num,arg);
    return end;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanRangeU32 ( ccp arg, u32 * p_stat, u32 * p_n1, u32 * p_n2, u32 min, u32 max )
{
    ASSERT(arg);
    ASSERT(p_n1);
    ASSERT(p_n2);
    TRACE("ScanRangeU32(%s)\n",arg);

    int stat = 0;
    u32 n1 = ~(u32)0, n2 = 0;

    while ( *arg > 0 && *arg <= ' ' )
	arg++;

    if ( *arg == '-' )
	n1 = min;
    else
    {
	char * end;
	u32 num = strtoul(arg,&end,0);
	if ( arg == end )
	    goto abort;

	stat = 1;
	arg = end;
	n1 = num;

	while ( *arg > 0 && *arg <= ' ' )
	    arg++;
    }

    if ( *arg != '-' )
    {
	stat = 1;
	n2 = n1;
	goto abort;
    }
    arg++;

    while ( *arg > 0 && *arg <= ' ' )
	arg++;

    char * end;
    n2 = strtoul(arg,&end,0);
    if ( end == arg )
	n2 = max;
    stat = 2;
    arg = end;

 abort:

    if ( stat > 0 )
    {
	if ( n1 < min )
	    n1 = min;
	if ( n2 > max )
	    n2 = max;
    }

    if ( !stat || n1 > n2 )
    {
	stat = 0;
	n1 = ~(u32)0;
	n2 = 0;
    }

    if (p_stat)
	*p_stat = stat;
    *p_n1 = n1;
    *p_n2 = n2;

    while ( *arg > 0 && *arg <= ' ' )
	arg++;

    TRACE("END ScanRangeU32() stat=%u, n=%u..%u ->%s\n",stat,n1,n2,arg);
    return (char*)arg;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    ScanHex()			///////////////
///////////////////////////////////////////////////////////////////////////////

// 0..15 : hex digit
//    50 : spaces and cotnrol charactzers
//    51 : allowed separators
//    99 : invalid char

static u8 hextab[256] =
{
   99,50,50,50, 50,50,50,50, 50,50,50,50, 50,50,50,50, // 0x00 .. 0x0f
   50,50,50,50, 50,50,50,50, 50,50,50,50, 50,50,50,50, // 0x10 .. 0x1f

   50,99,99,99, 99,99,99,99, 99,99,99,99, 51,51,51,99, //  !"# $%&' ()*+ ,-./
    0, 1, 2, 3,  4, 5, 6, 7,  8, 9,51,99, 99,99,99,99, // 0123 4567 89:; <=>?
   99,10,11,12, 13,14,15,99, 99,99,99,99, 99,99,99,99, // @ABC DEFG HIJK LMNO
   99,99,99,99, 99,99,99,99, 99,99,99,99, 99,99,99,99, // PQRS TUVW XYZ[ \]^_
   99,10,11,12, 13,14,15,99, 99,99,99,99, 99,99,99,99, // `abc defg hijk lmno
   99,99,99,99, 99,99,99,99, 99,99,99,99, 99,99,99,99, // PQRS TUVW xyz {|}~

   99,99,99,99, 99,99,99,99, 99,99,99,99, 99,99,99,99, // 0x80 .. 0x8f
   99,99,99,99, 99,99,99,99, 99,99,99,99, 99,99,99,99, // 0x90 .. 0x9f
   99,99,99,99, 99,99,99,99, 99,99,99,99, 99,99,99,99, // 0xa0 .. 0xaf
   99,99,99,99, 99,99,99,99, 99,99,99,99, 99,99,99,99, // 0xb0 .. 0xbf
   99,99,99,99, 99,99,99,99, 99,99,99,99, 99,99,99,99, // 0xc0 .. 0xcf
   99,99,99,99, 99,99,99,99, 99,99,99,99, 99,99,99,99, // 0xd0 .. 0xdf
   99,99,99,99, 99,99,99,99, 99,99,99,99, 99,99,99,99, // 0xe0 .. 0xef
   99,99,99,99, 99,99,99,99, 99,99,99,99, 99,99,99,99, // 0xf0 .. 0xff
};

///////////////////////////////////////////////////////////////////////////////

char * ScanHexHelper
(
    void	* buf,		// valid pointer to result buf
    int		buf_size,	// number of byte to read
    int		* bytes_read,	// NULL or result: number of read bytes
    ccp		arg,		// source string
    int		err_level	// error level (print message):
				//	 = 0: don't print errors
				//	>= 1: print syntax errors
				//	>= 2: msg if bytes_read<buf_size
				//	>= 3: msg if arg contains more characters
)
{
    DASSERT( buf );
    DASSERT( buf_size > 0 );
    DASSERT( arg );
    
    u8 * dest = buf;
    memset(dest,0,buf_size);

    const u8 * src = (const u8*)arg;
    int read_count = 0;
    while ( hextab[*src] < 99 && read_count < buf_size )
    {
	while ( hextab[*src] >= 50 && hextab[*src] <= 99 )
	    src++;

	if ( *src == '0' && ( src[1] == 'x' || src[1] == 'X' ))
	    src += 2;

	const u8 * end = src;
	while ( hextab[*end] < 16 )
	    end++;

	if ( (end-src) & 1 )
	{
	    *dest++ = hextab[*src++];
	    read_count++;
	}
	
	while ( src < end && read_count < buf_size )
	{
	    *dest    = hextab[*src++] << 4;
	    *dest++ |= hextab[*src++];
	    read_count++;
	}
    }
    
    // [2do] What 2do?? Is this a forgotten marker?

    while ( hextab[*src] == 50 )
	src++;

    if ( err_level > 0 )
    {
	if ( read_count < buf_size && *src )
	    ERROR0(ERR_SYNTAX,"Illegal character for hex input: %.20s\n",src);
	else if ( err_level > 1 && read_count < buf_size )
	    ERROR0(ERR_SYNTAX,"Missing %d hex characters: %.20s\n",
			buf_size - read_count, arg);
	else if ( err_level > 2 && *src )
	    ERROR0(ERR_SYNTAX,"Unexpected characters: %.20s\n",src);
    }

    if (bytes_read)
	*bytes_read = read_count;
    return (char*)src;
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanHex
(
    void	* buf,		// valid pointer to result buf
    int		buf_size,	// number of byte to read
    ccp		arg		// source string
)
{
    int count;
    arg = ScanHexHelper(buf,buf_size,&count,arg,99);

    return count == buf_size && !*arg ? ERR_OK : ERR_SYNTAX;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    scan compression option		///////////////
///////////////////////////////////////////////////////////////////////////////

wd_compression_t opt_compr_method = WD_COMPR__DEFAULT;
int opt_compr_level		  = 0;	// 0=default, 1..9=valid
u32 opt_compr_chunk_size	  = 0;	// 0=default

///////////////////////////////////////////////////////////////////////////////

static wd_compression_t ScanCompression_helper
(
    ccp			arg,		// argument to scan
    bool		silent,		// don't print error message
    int			* level,	// not NULL: appendix '.digit' allowed
					// The level will be stored in '*level'
    u32			* chunk_size	// not NULL: appendix '@size' allowed
					// The size will be stored in '*chunk_size'
)
{
    enum
    {
	COMPR_MEM = WD_COMPR__N+1
    };

    static const CommandTab_t tab[] =
    {
	{ WD_COMPR_NONE,	"NONE",		0,	0 },
	{ WD_COMPR_PURGE,	"PURGE",	"WDF",	0 },
	{ WD_COMPR_BZIP2,	"BZIP2",	"BZ2",	0 },
	{ WD_COMPR_LZMA,	"LZMA",		"LZ",	0 },
	{ WD_COMPR_LZMA2,	"LZMA2",	"LZ2",	0 },

	{ WD_COMPR__DEFAULT,	"DEFAULT",	"D",	0 },
	{ WD_COMPR__FAST,	"FAST",		"F",	0x300 + 10 },
	{ WD_COMPR__GOOD,	"GOOD",		"G",	0 },
	{ WD_COMPR__BEST,	"BEST",		"B",	0x900 + 50 },

	{ COMPR_MEM,		"MEM",		"M",	0 },

	{ WD_COMPR_NONE,	"0",		0,	0 },
	{ WD_COMPR_LZMA,	"1",		0,	0x100 +  5 },
	{ WD_COMPR_LZMA,	"2",		0,	0x200 + 10 },
	{ WD_COMPR_LZMA,	"3",		0,	0x300 + 30 },
	{ WD_COMPR_LZMA,	"4",		0,	0x400 + 30 },
	{ WD_COMPR_LZMA,	"5",		0,	0x400 + 50 },
	{ WD_COMPR_LZMA,	"6",		0,	0x500 + 20 },
	{ WD_COMPR_LZMA,	"7",		0,	0x500 + 50 },
	{ WD_COMPR_LZMA,	"8",		0,	0x600 + 50 },
	{ WD_COMPR_LZMA,	"9",		0,	0x900 + 50 },

	{ 0,0,0,0 }
    };

    char argbuf[100];
    ccp scan_arg = arg ? arg : "";
    while ( *scan_arg > 0 && *scan_arg <= ' ' )
	scan_arg++;

    if ( level || chunk_size )
    {
	if (level)
	    *level = -1; // -1 = mark as 'not set'
	if (chunk_size)
	    *chunk_size = 0;

	const int len = strlen(scan_arg);
	if ( len < sizeof(argbuf) )
	{
	    strcpy(argbuf,scan_arg);
	    scan_arg = argbuf;

	    if (chunk_size)
	    {
		char * found = strchr(argbuf,'@');
		if (found)
		{
		    *found++ = 0;
		    char * end;
		    unsigned num = strtoul(found,&end,10);
		    if (*end)
		    {
			if (!silent)
			    ERROR0(ERR_SYNTAX,
				"Unsigned number expected: --compression @%s\n",found);
			return -1;
		    }
		    *chunk_size = ( num ? num : WIA_DEF_CHUNK_FACTOR ) * WII_GROUP_SIZE;
		}
	    }
	    
	    if (level)
	    {
		char * found = strchr(argbuf,'.');
		if ( found && found[1] >= '0' && found[1] <= '9' && !found[2] )
		{
		    *level = found[1] - '0';
		    *found = 0;
		}
	    }

	    if (!*argbuf)
		return WD_COMPR__DEFAULT;
	}
    }

    const CommandTab_t * cmd = ScanCommand(0,scan_arg,tab);
    if (cmd)
    {
	wd_compression_t compr = cmd->id;
	u32 opt = cmd->opt;

	if ( compr == (wd_compression_t)COMPR_MEM )
	{
	    compr = WD_COMPR__DEFAULT;
	    u32 memlimit = GetMemLimit();
	    if (memlimit)
	    {
		typedef struct tripel
		{
		    wd_compression_t	compr;
		    int			level;
		    u32			csize;
		} tripel;

		static const tripel tab[] =
		{
		    { WD_COMPR_LZMA,	7, 50 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	7, 40 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	6, 50 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	6, 40 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	6, 30 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	5, 50 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	5, 40 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	5, 30 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	5, 20 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	4, 50 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	4, 40 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	4, 30 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	4, 20 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	4, 10 * WIA_BASE_CHUNK_SIZE },

		  #ifndef NO_BZIP2
		    { WD_COMPR_BZIP2,	9, 50 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_BZIP2,	8, 40 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_BZIP2,	7, 30 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_BZIP2,	6, 25 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_BZIP2,	5, 20 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_BZIP2,	4, 15 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_BZIP2,	3, 10 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_BZIP2,	2,  5 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_BZIP2,	1,  1 * WIA_BASE_CHUNK_SIZE },
		  #endif

		    { WD_COMPR_LZMA,	4,  5 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	3,  5 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	2,  5 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	1,  5 * WIA_BASE_CHUNK_SIZE },
		    { WD_COMPR_LZMA,	1,  1 * WIA_BASE_CHUNK_SIZE },

		    { 0,0,0 } // end marker
		};

		const tripel * p;
		for ( p = tab; p->level; p++ )
		{
		    if ( CalcMemoryUsageWIA(p->compr,p->level,p->csize,true) <= memlimit )
		    {
			compr = p->compr;
			opt   = p->level << 8 | p->csize / WIA_BASE_CHUNK_SIZE;
			break;
		    }
		}
	    }
	}
	
	if (opt)
	{
	    if ( level && *level < 1 )
		*level = opt >> 8;
	    if ( chunk_size && *chunk_size <= 1 )
		*chunk_size = ( opt & 0xff ) * WIA_BASE_CHUNK_SIZE;
	}

	return compr;
    }

#if 0 // [obsolete]
    char * end;
    u32 val = strtoul(scan_arg,&end,10);
 #ifdef TEST
    if ( end > scan_arg && !*end )
	return val;
 #else
    if ( end > scan_arg && !*end && val < WD_COMPR__N )
	return val;
 #endif
#endif

    if (!silent)
	ERROR0(ERR_SYNTAX,"Illegal compression method: '%s'\n",scan_arg);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

wd_compression_t ScanCompression
(
    ccp			arg,		// argument to scan
    bool		silent,		// don't print error message
    int			* level,	// not NULL: appendix '.digit' allowed
					// The level will be stored in '*level'
    u32			* chunk_size	// not NULL: appendix '@size' allowed
					// The size will be stored in '*chunk_size'
)
{
    wd_compression_t compr = ScanCompression_helper(arg,silent,level,chunk_size);
    if (level)
    {
	if ( *level < 0 )
	    *level = 0;
	else switch(compr)
	{
	    case WD_COMPR__N:
	    case WD_COMPR_NONE:
	    case WD_COMPR_PURGE:
		*level = 0;
		break;

	    case WD_COMPR_BZIP2:
	     #ifdef NO_BZIP2
		*level = 0;
	     #else
		*level = CalcCompressionLevelBZIP2(*level);
	     #endif
		break;
	    
	    case WD_COMPR_LZMA:
	    case WD_COMPR_LZMA2:
		*level = CalcCompressionLevelLZMA(*level);
		break;
	}
    }

    return compr;
}

///////////////////////////////////////////////////////////////////////////////

int ScanOptCompression
(
    bool		set_oft_wia,	// true: output_file_type := OFT_WIA
    ccp			arg		// argument to scan
)
{
    if (set_oft_wia)
	output_file_type = OFT_WIA;
    
    if (arg)
    {
	int new_level = 0;
	u32 new_chunk_size;
	const int new_compr = ScanCompression(arg,false,&new_level,&new_chunk_size);
	if ( new_compr == -1 )
	    return 1;
	opt_compr_method	 = new_compr;
	opt_compr_level	 = new_level;
	opt_compr_chunk_size = new_chunk_size;
    }
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan mem option			///////////////
///////////////////////////////////////////////////////////////////////////////

u64 opt_mem = 0;

///////////////////////////////////////////////////////////////////////////////

int ScanOptMem
(
    ccp			arg,		// argument to scan
    bool		print_err	// true: print error messages
)
{
    u64 num;
    enumError err = ScanSizeOptU64
			( &num,		// u64 * num,
			  arg,		// ccp source,
			  MiB,		// u64 default_factor1,
			  0,		// int force_base,
			  "mem",	// ccp opt_name,
			  0,		// u64 min,
			  0,		// u64 max,
			  0,		// u32 multiple,
			  0,		// u32 pow2,
			  print_err	// bool print_err
			);

    if (err)
	return 1;

    opt_mem = num;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

u64 GetMemLimit()
{
    static bool done = false;
    if ( !done && !opt_mem )
    {
	done = true;

	char * env = getenv("WIT_MEM");
	if ( env && *env )
	    ScanOptMem(env,false);

	if (!opt_mem)
	{
	    FILE * f = fopen("/proc/meminfo","r");
	    if (f)
	    {
		TRACE("SCAN /proc/meminfo\n");
		char buf[500];
		while (fgets(buf,sizeof(buf),f))
		{
		    if (memcmp(buf,"MemTotal:",9))
			continue;

		    char * ptr;
		    s64 num = strtoul(buf+9,&ptr,10);
		    if (num)
		    {
			while ( *ptr && *ptr <= ' ' )
			    ptr++;
			switch (*ptr)
			{
			    case 'k': case 'K': num *= KiB; break;
			    case 'm': case 'M': num *= MiB; break;
			    case 'g': case 'G': num *= GiB; break;
			}
			num -= 50 * MiB + num/5;
			opt_mem = num < 1 ? 1 : num;
		    }
		    break;
		}
		fclose(f);
	    }
	}
    }

    return opt_mem;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 CommandTab_t			///////////////
///////////////////////////////////////////////////////////////////////////////

const CommandTab_t * ScanCommand
(
    int			* res_abbrev,	// NULL or pointer to result 'abbrev_count'
    ccp			arg,		// argument to scan
    const CommandTab_t	* cmd_tab	// valid pointer to command table
)
{
    ASSERT(arg);
    char cmd_buf[COMMAND_NAME_MAX];

    char *dest = cmd_buf;
    char *end  = cmd_buf + sizeof(cmd_buf) - 1;
    while ( *arg && dest < end )
	*dest++ = toupper((int)*arg++);
    *dest = 0;
    const int cmd_len = dest - cmd_buf;

    int abbrev_count = 0;
    const CommandTab_t *ct, *cmd_ct = 0, *abbrev_ct = 0;
    for ( ct = cmd_tab; ct->name1; ct++ )
    {
	if ( !strcmp(ct->name1,cmd_buf) || ct->name2 && !strcmp(ct->name2,cmd_buf) )
	{
	    cmd_ct = ct;
	    break;
	}
	if ( !memcmp(ct->name1,cmd_buf,cmd_len)
		|| ct->name2 && !memcmp(ct->name2,cmd_buf,cmd_len) )
	{
	    if ( !abbrev_ct || abbrev_ct->id != ct->id || abbrev_ct->opt != ct->opt )
	    {
		abbrev_ct = ct;
		abbrev_count++;
	    }
	}
    }

    if (cmd_ct)
	abbrev_count = 0;
    else if ( abbrev_count == 1 )
	cmd_ct = abbrev_ct;
    else if (!abbrev_count)
	abbrev_count = -1;

    if (res_abbrev)
	*res_abbrev = abbrev_count;

    return cmd_ct;
}

///////////////////////////////////////////////////////////////////////////////

s64 ScanCommandList
(
    ccp			arg,		// argument to scan
    const CommandTab_t	* cmd_tab,	// valid pointer to command table
    CommandCallbackFunc	func,		// NULL or calculation function
    bool		allow_prefix,	// allow '-' | '+' | '=' as prefix
    u32			max_number,	// allow numbers < 'max_number' (0=disabled)
    s64			result		// start value for result
)
{
    ASSERT(arg);

    char cmd_buf[COMMAND_NAME_MAX];
    char *end  = cmd_buf + sizeof(cmd_buf) - 1;

    for (;;)
    {
	while ( *arg > 0 && *arg <= ' ' || *arg == ',' )
	    arg++;

	if (!*arg)
	    return result;

	char *dest = cmd_buf;
	while ( *arg > ' ' && *arg != ',' &&
		( *arg != '+' || dest == cmd_buf ) && dest < end )
	    *dest++ = *arg++;
	*dest = 0;
	char prefix = 0;
	int abbrev_count;
	const CommandTab_t * cptr = ScanCommand(&abbrev_count,cmd_buf,cmd_tab);
	if ( !cptr && allow_prefix && cmd_buf[1]
	    && ( *cmd_buf == '+' || *cmd_buf == '-' || *cmd_buf == '/' || *cmd_buf == '=' ))
	{
	    prefix = *cmd_buf == '/' ? '-' : *cmd_buf;
	    cptr = ScanCommand(&abbrev_count,cmd_buf+1,cmd_tab);
	}

	CommandTab_t ct_num = { 0, cmd_buf, 0, 0 };
	if ( max_number && abbrev_count )
	{
	    char * start = cmd_buf + (prefix!=0);
	    ulong num = strtol(start,&dest,10);
	    if ( num < max_number && dest > start && !*dest )
	    {
		ct_num.id = num;
		cptr = &ct_num;
	    }
	}
	
	if ( !cptr || cptr->opt && prefix && prefix != '+' )
	    return -1;


	if (func)
	{
	    result = func(0,cmd_buf,cmd_tab,cptr,prefix,result);
	    if ( result == -(s64)1 )
		return result;
	}
	else
	{
	    switch (prefix)
	    {
		case 0:
		case '+': result  =  result & ~cptr->opt | cptr->id; break;
		case '-': result &= ~cptr->id; break;
		case '=': result  =  cptr->id; break;
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanCommandListFunc
(
    ccp			arg,		// argument to scan
    const CommandTab_t	* cmd_tab,	// valid pointer to command table
    CommandCallbackFunc	func,		// calculation function
    void		* param,	// used define parameter for 'func'
    bool		allow_prefix	// allow '-' | '+' | '=' as prefix
)
{
    ASSERT(arg);
    ASSERT(func);

    char cmd_buf[COMMAND_NAME_MAX];
    char *end  = cmd_buf + sizeof(cmd_buf) - 1;

    for (;;)
    {
	while ( *arg > 0 && *arg <= ' ' || *arg == ',' )
	    arg++;

	if (!*arg)
	    return ERR_OK;

	char *dest = cmd_buf;
	while ( *arg > ' ' && *arg != ',' &&
		( *arg != '+' || dest == cmd_buf ) && dest < end )
	    *dest++ = *arg++;
	*dest = 0;
	char prefix = 0;
	int abbrev_count;
	const CommandTab_t * cptr = ScanCommand(&abbrev_count,cmd_buf,cmd_tab);
	if ( !cptr && allow_prefix && cmd_buf[1]
	    && ( *cmd_buf == '+' || *cmd_buf == '-' || *cmd_buf == '/' || *cmd_buf == '=' ))
	{
	    prefix = *cmd_buf == '/' ? '-' : *cmd_buf;
	    cptr = ScanCommand(&abbrev_count,cmd_buf+1,cmd_tab);
	}

	const enumError err = func(param,cmd_buf,cmd_tab,cptr,prefix,0);
	if (err)
	    return err;
    }
}

///////////////////////////////////////////////////////////////////////////////

static s64 ScanCommandListMaskHelper
(
    void		* param,	// NULL or user defined parameter
    ccp			name,		// normalized name of option
    const CommandTab_t	* cmd_tab,	// valid pointer to command table
    const CommandTab_t	* cmd,		// valid pointer to found command
    char		prefix,		// 0 | '-' | '+' | '='
    s64			result		// current value of result
)
{
    return cmd->opt
		? result & ~cmd->opt | cmd->id
		: cmd->id;
}

//-----------------------------------------------------------------------------

s64 ScanCommandListMask
(
    ccp			arg,		// argument to scan
    const CommandTab_t	* cmd_tab	// valid pointer to command table
)
{
    return ScanCommandList(arg,cmd_tab,ScanCommandListMaskHelper,false,0,0);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    sort mode			///////////////
///////////////////////////////////////////////////////////////////////////////

SortMode ScanSortMode ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ SORT_NONE,	"NONE",		"-",		SORT__MASK },

	{ SORT_ID,	"ID",		0,		SORT__MODE_MASK },
	{ SORT_NAME,	"NAME",		"N",		SORT__MODE_MASK },
	{ SORT_TITLE,	"TITLE",	"T",		SORT__MODE_MASK },
	{ SORT_FILE,	"FILE",		"F",		SORT__MODE_MASK },
	{ SORT_SIZE,	"SIZE",		"SZ",		SORT__MODE_MASK },
	{ SORT_OFFSET,	"OFFSET",	"OF",		SORT__MODE_MASK },
	{ SORT_REGION,	"REGION",	"R",		SORT__MODE_MASK },
	{ SORT_WBFS,	"WBFS",		0,		SORT__MODE_MASK },
	{ SORT_NPART,	"NPART",	0,		SORT__MODE_MASK },

	{ SORT_ITIME,	"ITIME",	"IT",		SORT__MODE_MASK },
	{ SORT_MTIME,	"MTIME",	"MT",		SORT__MODE_MASK },
	{ SORT_CTIME,	"CTIME",	"CT",		SORT__MODE_MASK },
	{ SORT_ATIME,	"ATIME",	"AT",		SORT__MODE_MASK },
	{ SORT_TIME,	"TIME",		"TI",		SORT__MODE_MASK },
	{ SORT_TIME,	"DATE",		"D",		SORT__MODE_MASK },

	{ SORT_DEFAULT,	"DEFAULT",	0,		SORT__MODE_MASK },

	{ 0,		"ASCENDING",	0,		SORT_REVERSE },
	{ SORT_REVERSE,	"DESCENDING",	"REVERSE",	SORT_REVERSE },

	{ 0,0,0,0 }
    };

    const int stat = ScanCommandListMask(arg,tab);
    if ( stat >= 0 )
	return stat;

    ERROR0(ERR_SYNTAX,"Illegal sort mode (option --sort): '%s'\n",arg);
    return SORT__ERROR;
}

//-----------------------------------------------------------------------------

int ScanOptSort ( ccp arg )
{
    const SortMode new_mode = ScanSortMode(arg);
    if ( new_mode == SORT__ERROR )
	return 1;

    TRACE("SORT-MODE set: %d -> %d\n",sort_mode,new_mode);
    sort_mode = new_mode;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			options show + unit		///////////////
///////////////////////////////////////////////////////////////////////////////

ShowMode ScanShowMode ( ccp arg )
{
    enum
    {
	DEC_ALL = SHOW_F_DEC1 | SHOW_F_DEC,
	HEX_ALL = SHOW_F_HEX1 | SHOW_F_HEX,
    };

    static const CommandTab_t tab[] =
    {
	{ SHOW__NONE,		"NONE",		"-",		SHOW__ALL },
	{ SHOW__ALL,		"ALL",		0,		0 },

	{ SHOW_INTRO,		"INTRO",	0,		0 },
	{ SHOW_D_ID,		"D-ID",		"DID",		0 },
	{ SHOW_P_ID,		"P-IDS",	"PIDS",		0 },
	{ SHOW_P_TAB,		"P-TAB",	"PTAB",		0 },
	{ SHOW_P_INFO,		"P-INFO",	"PINFO",	0 },
	{ SHOW_P_MAP,		"P-MAP",	"PMAP",		0 },
	{ SHOW_D_MAP,		"D-MAP",	"DMAP",		0 },
	{ SHOW_CERT,		"CERTIFICATES",	0,		0 },
	{ SHOW_TICKET,		"TICKET",	"TIK",		0 },
	{ SHOW_TMD,		"TMD",		0,		0 },
	{ SHOW_USAGE,		"USAGE",	0,		0 },
	{ SHOW_FILES,		"FILES",	0,		0 },
	{ SHOW_PATCH,		"PATCH",	0,		0 },
	{ SHOW_RELOCATE,	"RELOCATE",	0,		0 },
	{ SHOW_PATH,		"PATH",		0,		0 },

	{ SHOW_OFFSET,		"OFFSET",	0,		0 },
	{ SHOW_SIZE,		"SIZE",		0,		0 },

	{ SHOW__ID,		"IDS",		0,		0 },
	{ SHOW__PART,		"PART",		0,		0 },
	{ SHOW__MAP,		"MAP",		0,		0 },

	{ DEC_ALL,		"DEC",		0,		SHOW_F_HEX1 },
	{ 0,			"-DEC",		0,		SHOW_F_DEC },
	{ DEC_ALL,		"=DEC",		0,		HEX_ALL },
	{ HEX_ALL,		"HEX",		0,		SHOW_F_DEC1 },
	{ 0,			"-HEX",		0,		SHOW_F_HEX },
	{ HEX_ALL,		"=HEX",		0,		DEC_ALL },

	{ SHOW_F_HEAD,		"HEADER",	0,		0 },
	{ SHOW_F_PRIMARY,	"PRIMARY",	"1",		0 },

	{ 0,0,0,0 }
    };

    int stat = ScanCommandList(arg,tab,0,true,0,SHOW_F_HEAD);
    if ( stat != -1 )
	return stat;

    ERROR0(ERR_SYNTAX,"Illegal show mode (option --show): '%s'\n",arg);
    return SHOW__ERROR;
}

//-----------------------------------------------------------------------------

int ScanOptShow ( ccp arg )
{
    const ShowMode new_mode = ScanShowMode(arg);
    if ( new_mode == SHOW__ERROR )
	return 1;

    TRACE("SHOW-MODE set: %d -> %d\n",opt_show_mode,new_mode);
    opt_show_mode = new_mode;
    return 0;
}

//-----------------------------------------------------------------------------

int ConvertShow2PFST
(
	ShowMode show_mode,	// show mode
	ShowMode def_mode	// default mode
)
{
    TRACE("ConvertShow2PFST(%x,%x)\n",show_mode,def_mode);
    const ShowMode MASK_OFF_SIZE = SHOW_OFFSET | SHOW_SIZE;
    if ( !(show_mode & MASK_OFF_SIZE) )
	show_mode |= def_mode & MASK_OFF_SIZE;

    const ShowMode MASK_DEC_HEX = SHOW_F_DEC | SHOW_F_HEX;
    if ( !(show_mode & MASK_DEC_HEX) )
	show_mode |= def_mode & MASK_DEC_HEX;

    noTRACE(" --> val=%x, off/size=%x, dec/hex=%x\n",
	show_mode, show_mode & MASK_OFF_SIZE, show_mode & MASK_DEC_HEX );

    wd_pfst_t pfst = 0;
    if ( show_mode & SHOW_F_HEAD )
	pfst |= WD_PFST_HEADER;
    if ( show_mode & SHOW_OFFSET )
	pfst |= WD_PFST_OFFSET;
    if ( show_mode & SHOW_SIZE )
    {
	switch ( show_mode & MASK_DEC_HEX )
	{
	    case SHOW_F_DEC:
		pfst |= WD_PFST_SIZE_DEC;
		break;

	    case SHOW_F_HEX:
		pfst |= WD_PFST_SIZE_HEX;
		break;

	    default:
		pfst |= WD_PFST_SIZE_DEC|WD_PFST_SIZE_HEX; break;
	}
    }

    TRACE(" --> PFST=%x\n",pfst);
    return pfst;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

wd_size_mode_t ScanUnit ( ccp arg )
{
    enum
    {
	F_1000 = WD_SIZE_F_1000,
	F_1024 = WD_SIZE_F_1024,
    };

    static const CommandTab_t tab[] =
    {
	{ WD_SIZE_DEFAULT,	"DEFAULT",0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },

	{ WD_SIZE_F_1000,	"1000",	"10",	WD_SIZE_M_BASE },
	{ WD_SIZE_F_1024,	"1024",	"2",	WD_SIZE_M_BASE },

	{ WD_SIZE_AUTO,		"AUTO",	0,	WD_SIZE_M_MODE },
	{ WD_SIZE_BYTES,	"BYTES",0,	WD_SIZE_M_MODE },
	{ WD_SIZE_K,		"K",	0,	WD_SIZE_M_MODE },
	{ WD_SIZE_M,		"M",	0,	WD_SIZE_M_MODE },
	{ WD_SIZE_G,		"G",	0,	WD_SIZE_M_MODE },
	{ WD_SIZE_T,		"T",	0,	WD_SIZE_M_MODE },
	{ WD_SIZE_P,		"P",	0,	WD_SIZE_M_MODE },
	{ WD_SIZE_E,		"E",	0,	WD_SIZE_M_MODE },

	{ WD_SIZE_K | F_1000,	"KB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },
	{ WD_SIZE_M | F_1000,	"MB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },
	{ WD_SIZE_G | F_1000,	"GB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },
	{ WD_SIZE_T | F_1000,	"TB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },
	{ WD_SIZE_P | F_1000,	"PB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },
	{ WD_SIZE_E | F_1000,	"EB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },

	{ WD_SIZE_K | F_1024,	"KIB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },
	{ WD_SIZE_M | F_1024,	"MIB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },
	{ WD_SIZE_G | F_1024,	"GIB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },
	{ WD_SIZE_T | F_1024,	"TIB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },
	{ WD_SIZE_P | F_1024,	"PIB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },
	{ WD_SIZE_E | F_1024,	"EIB",	0,	WD_SIZE_M_MODE|WD_SIZE_M_BASE },

	{ WD_SIZE_HD_SECT,	"HDSS",	"HSS",	WD_SIZE_M_MODE },
	{ WD_SIZE_WD_SECT,	"WDSS",	"WSS",	WD_SIZE_M_MODE },
	{ WD_SIZE_GC,		"GAMECUBE","GC",WD_SIZE_M_MODE },
	{ WD_SIZE_WII,		"WII",	0,	WD_SIZE_M_MODE },
	
	{ 0,0,0,0 }
    };

    int stat = ScanCommandList(arg,tab,0,true,0,0);
    if ( stat != -1 )
	return stat;

    ERROR0(ERR_SYNTAX,"Illegal unit mode (option --unit): '%s'\n",arg);
    return -1;
}

//-----------------------------------------------------------------------------

int ScanOptUnit ( ccp arg )
{
    const wd_size_mode_t new_mode = ScanUnit(arg);
    if ( new_mode == -1 )
	return 1;

    TRACE("OPT --unit set: %x -> %x\n",opt_unit,new_mode);
    opt_unit = new_mode;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   repair mode			///////////////
///////////////////////////////////////////////////////////////////////////////

RepairMode ScanRepairMode ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ REPAIR_NONE,		"NONE",		"-",	REPAIR_ALL },

	{ REPAIR_FBT,		"FBT",		"F",	0 },
	{ REPAIR_INODES,	"INODES",	"I",	0 },
	{ REPAIR_DEFAULT,	"STANDARD",	"STD",	0 },

	{ REPAIR_RM_INVALID,	"RM-INVALID",	"RI",	0 },
	{ REPAIR_RM_OVERLAP,	"RM-OVERLAP",	"RO",	0 },
	{ REPAIR_RM_FREE,	"RM-FREE",	"RF",	0 },
	{ REPAIR_RM_EMPTY,	"RM-EMPTY",	"RE",	0 },
	{ REPAIR_RM_ALL,	"RM-ALL",	"RA",	0 },

	{ REPAIR_ALL,		"ALL",		"*",	0 },
	{ 0,0,0,0 }
    };

    int stat = ScanCommandList(arg,tab,0,true,0,0);
    if ( stat != -1 )
	return stat;

    ERROR0(ERR_SYNTAX,"Illegal repair mode (option --repair): '%s'\n",arg);
    return REPAIR__ERROR;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    string lists & fields		///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeStringField ( StringField_t * sf )
{
    ASSERT(sf);
    memset(sf,0,sizeof(*sf));
}

///////////////////////////////////////////////////////////////////////////////

void ResetStringField ( StringField_t * sf )
{
    ASSERT(sf);
    if ( sf && sf->used > 0 )
    {
	ASSERT(sf->field);
	ccp *ptr = sf->field, *end;
	for ( end = ptr + sf->used; ptr < end; ptr++ )
	    free((char*)*ptr);
	free(sf->field);
    }
    InitializeStringField(sf);
}

///////////////////////////////////////////////////////////////////////////////

ccp FindStringField ( StringField_t * sf, ccp key )
{
    bool found;
    int idx = FindStringFieldHelper(sf,&found,key);
    return found ? sf->field[idx] : 0;
}

///////////////////////////////////////////////////////////////////////////////

static ccp * InsertStringFieldHelper ( StringField_t * sf, int idx )
{
    DASSERT(sf);
    DASSERT( sf->used <= sf->size );
    if ( sf->used == sf->size )
    {
	sf->size += 0x100;
	sf->field = realloc(sf->field,sf->size*sizeof(ccp));
	if (!sf->field)
	    OUT_OF_MEMORY;
    }
    DASSERT( idx <= sf->used );
    ccp * dest = sf->field + idx;
    memmove(dest+1,dest,(sf->used-idx)*sizeof(ccp));
    sf->used++;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

bool InsertStringField ( StringField_t * sf, ccp key, bool move_key )
{
    if (!key)
	return 0;

    bool found;
    int idx = FindStringFieldHelper(sf,&found,key);
    if (found)
    {
	if (move_key)
	    free((char*)key);
    }
    else
    {
	ccp * dest = InsertStringFieldHelper(sf,idx);
	*dest = move_key ? key : strdup(key);
    }

    return !found;
}

///////////////////////////////////////////////////////////////////////////////

StringItem_t * InsertStringID6 ( StringField_t * sf, void * id6, char flag, ccp arg )
{
    if (!id6)
	return 0;

    bool found;
    int idx = FindStringFieldHelper(sf,&found,id6);
    ccp * dest;
    if (found)
    {
	DASSERT( idx < sf->used );
	dest = sf->field + idx;
	free((char*)*dest);
    }
    else
	dest = InsertStringFieldHelper(sf,idx);

    const int arg_len   = arg ? strlen(arg) : 0;
    const int item_size = sizeof(StringItem_t) + arg_len + 1;
    StringItem_t * item = malloc(item_size);
    if (!item)
	OUT_OF_MEMORY;

    *dest = (ccp)item;
    memset(item,0,item_size);
    strncpy(item->id6,id6,6);
    item->flag = flag;
    if (arg)
	memcpy(item->arg,arg,arg_len);
    return item;
}

///////////////////////////////////////////////////////////////////////////////

bool RemoveStringField ( StringField_t * sf, ccp key )
{
    bool found;
    uint idx = FindStringFieldHelper(sf,&found,key);
    if (found)
    {
	sf->used--;
	ASSERT( idx <= sf->used );
	ccp * dest = sf->field + idx;
	free((char*)dest);
	memmove(dest,dest+1,(sf->used-idx)*sizeof(ccp));
    }
    return found;
}

///////////////////////////////////////////////////////////////////////////////

void AppendStringField ( StringField_t * sf, ccp key, bool move_key )
{
    if (key)
    {
	ASSERT( sf->used <= sf->size );
	if ( sf->used == sf->size )
	{
	    sf->size += 0x100;
	    sf->field = realloc(sf->field,sf->size*sizeof(ccp));
	    if (!sf->field)
		OUT_OF_MEMORY;
	}
	TRACE("AppendStringField(%s,%d) %d/%d\n",key,move_key,sf->used,sf->size);
	ccp * dest = sf->field + sf->used++;
	*dest = move_key ? key : strdup(key);
    }
}

///////////////////////////////////////////////////////////////////////////////

uint FindStringFieldHelper ( StringField_t * sf, bool * p_found, ccp key )
{
    ASSERT(sf);

    int beg = 0;
    if ( sf && key )
    {
	int end = sf->used - 1;
	while ( beg <= end )
	{
	    uint idx = (beg+end)/2;
	    int stat = strcmp(key,sf->field[idx]);
	    if ( stat < 0 )
		end = idx - 1 ;
	    else if ( stat > 0 )
		beg = idx + 1;
	    else
	    {
		TRACE("FindStringFieldHelper(%s) FOUND=%d/%d/%d\n",
			key, idx, sf->used, sf->size );
		if (p_found)
		    *p_found = true;
		return idx;
	    }
	}
    }

    TRACE("FindStringFieldHelper(%s) failed=%d/%d/%d\n",
		key, beg, sf->used, sf->size );

    if (p_found)
	*p_found = false;
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadStringField
	( StringField_t * sf, bool keep_order, ccp filename, bool silent )
{
    ASSERT(sf);
    ASSERT(filename);
    ASSERT(*filename);

    TRACE("ReadStringField(%p,%d,%s,%d)\n",sf,keep_order,filename,silent);

    FILE * f = fopen(filename,"rb");
    if (!f)
    {
	if (!silent)
	    ERROR1(ERR_CANT_OPEN,"Can't open file: %s\n",filename);
	return ERR_CANT_OPEN;
    }

    while (fgets(iobuf,sizeof(iobuf)-1,f))
    {
	char * ptr = iobuf;
	while (*ptr)
	    ptr++;
	if ( ptr > iobuf && ptr[-1] == '\n' )
	{
	    ptr--;
	    if ( ptr > iobuf && ptr[-1] == '\r' )
		ptr--;
	}
	
	if ( ptr > iobuf )
	{
	    *ptr++ = 0;
	    const size_t len = ptr-iobuf;
	    ptr = malloc(len);
	    memcpy(ptr,iobuf,len);
	    if (keep_order)
		AppendStringField(sf,ptr,true);
	    else
		InsertStringField(sf,ptr,true);
	}
    }

    fclose(f);
    return ERR_OK;
}
	

///////////////////////////////////////////////////////////////////////////////

enumError WriteStringField
	( StringField_t * sf, ccp filename, bool rm_if_empty )
{
    ASSERT(sf);
    ASSERT(filename);
    ASSERT(*filename);

    TRACE("WriteStringField(%p,%s,%d)\n",sf,filename,rm_if_empty);

    if ( !sf->used && rm_if_empty )
    {
	unlink(filename);
	return ERR_OK;
    }
    
    FILE * f = fopen(filename,"wb");
    if (!f)
	return ERROR1(ERR_CANT_CREATE,"Can't create file: %s\n",filename);

    ccp *ptr = sf->field, *end;
    for ( end = ptr + sf->used; ptr < end; ptr++ )
	fprintf(f,"%s\n",*ptr);
    fclose(f);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  string lists			///////////////
///////////////////////////////////////////////////////////////////////////////

int AtFileHelper
(
    ccp arg,
    int mode,
    int mode_expand,
    int (*func) ( ccp arg, int mode )
)
{
    if ( !arg || !*arg || !func )
	return 0;

    TRACE("AtFileHelper(%s,%x,%x)\n",arg,mode,mode_expand);
    if ( *arg != '@' )
	return func(arg,mode);

    FILE * f;
    char buf[PATH_MAX];
    const bool use_stdin = arg[1] == '-' && !arg[2];

    if (use_stdin)
	f = stdin;
    else
    {
     #ifdef __CYGWIN__
	char buf[PATH_MAX];
	NormalizeFilenameCygwin(buf,sizeof(buf),arg+1);
	f = fopen(buf,"r");
     #else
	f = fopen(arg+1,"r");
     #endif
	if (!f)
	    return func(arg,mode);
    }

    ASSERT(f);

    u32 max_stat = 0;
    while (fgets(buf,sizeof(buf)-1,f))
    {
	char * ptr = buf;
	while (*ptr)
	    ptr++;
	if ( ptr > buf && ptr[-1] == '\n' )
	    ptr--;
	if ( ptr > buf && ptr[-1] == '\r' )
	    ptr--;
	*ptr = 0;
	const u32 stat = func(buf,mode_expand);
	if ( max_stat < stat )
	     max_stat = stat;
    }
    fclose(f);
    return max_stat;
}

///////////////////////////////////////////////////////////////////////////////

uint n_param = 0, id6_param_found = 0;
ParamList_t * first_param = 0;
ParamList_t ** append_param = &first_param;

///////////////////////////////////////////////////////////////////////////////

static ParamList_t* GetPoolParam()
{
    static ParamList_t * pool = 0;
    static int n_pool = 0;

    if (!n_pool)
    {
	const int alloc_count = 100;
	pool = (ParamList_t*) calloc(alloc_count,sizeof(ParamList_t));
	if (!pool)
	    OUT_OF_MEMORY;
	n_pool = alloc_count;
    }

    n_pool--;
    return pool++;
}

///////////////////////////////////////////////////////////////////////////////

ParamList_t * AppendParam ( ccp arg, int is_temp )
{
    if ( !arg || !*arg )
	return 0;

    TRACE("ARG#%02d: %s\n",n_param,arg);

    ParamList_t * param = GetPoolParam();
    if (is_temp)
    {
	param->arg = strdup(arg);
	if (!param->arg)
	    OUT_OF_MEMORY;
    }
    else
	param->arg = (char*)arg;

    while (*append_param)
	append_param = &(*append_param)->next;

    noTRACE("INS: A=%p->%p P=%p &N=%p->%p\n",
	    append_param, *append_param,
	    param, &param->next, param->next );
    *append_param = param;
    append_param = &param->next;
    noTRACE("  => A=%p->%p\n", append_param, *append_param );
    n_param++;

    return param;
}

///////////////////////////////////////////////////////////////////////////////

int AddParam ( ccp arg, int is_temp )
{
    return AppendParam(arg,is_temp) ? 0 : 1;
}

///////////////////////////////////////////////////////////////////////////////

void AtExpandParam ( ParamList_t ** p_param )
{
    if ( !p_param || !*p_param )
	return;

    ParamList_t * param = *p_param;
    if ( param->is_expanded || !param->arg || *param->arg != '@' )
	return;

    FILE * f;
    char buf[PATH_MAX];
    const bool use_stdin = param->arg[1] == '-' && !param->arg[2];
    if (use_stdin)
	f = stdin;
    else
    {
     #ifdef __CYGWIN__
	NormalizeFilenameCygwin(buf,sizeof(buf),param->arg+1);
	f = fopen(buf,"r");
     #else
	f = fopen(param->arg+1,"r");
     #endif
	if (!f)
	    return;
    }

    ASSERT(f);

    u32 count = 0;
    while (fgets(buf,sizeof(buf)-1,f))
    {
	char * ptr = buf;
	while (*ptr)
	    ptr++;
	if ( ptr > buf && ptr[-1] == '\n' )
	    ptr--;
	if ( ptr > buf && ptr[-1] == '\r' )
	    ptr--;
	*ptr = 0;

	if (count++)
	{
	    // insert a new item
	    ParamList_t * new_param = GetPoolParam();
	    new_param->next = param->next;
	    param->next = new_param;
	    param = new_param;
	    n_param++;
	}
	param->arg = strdup(buf);
	param->is_expanded = true;
    }
    fclose(f);

    if (!count)
    {
	*p_param = param->next;
	n_param--;
    }

    append_param = &first_param;
}

///////////////////////////////////////////////////////////////////////////////

void AtExpandAllParam ( ParamList_t ** p_param )
{
    if (p_param)
	for ( ; *p_param; p_param = &(*p_param)->next )
	    AtExpandParam(p_param);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		     string substitutions		///////////////
///////////////////////////////////////////////////////////////////////////////

char * SubstString
	( char * buf, size_t bufsize, SubstString_t * tab, ccp source, int * count )
{
    ASSERT(buf);
    ASSERT(bufsize > 1);
    ASSERT(tab);
    TRACE("SubstString(%s)\n",source);

    char tempbuf[PATH_MAX];
    int conv_count = 0;

    char *dest = buf;
    char *end = buf + bufsize + 1;
    if (source)
	while ( dest < end && *source )
	{
	    if ( *source != escape_char || *++source == escape_char )
	    {
		*dest++ = *source++;
		continue;
	    }
	    ccp start = source - 1;

	    u32 p1, p2, stat;
	    source = ScanRangeU32(source,&stat,&p1,&p2,0,~(u32)0);
	    if ( stat == 1 )
		p1 = 0;
	    else if ( stat < 1 )
	    {
		p1 = 0;
		p2 = ~(u32)0;
	    }

	    char ch = *source++;
	    int convert = 0;
	    if ( ch == 'u' || ch == 'U' )
	    {
		convert++;
		ch = *source++;
	    }
	    else if ( ch == 'l' || ch == 'L' )
	    {
		convert--;
		ch = *source++;
	    }
	    if (!ch)
		break;

	    size_t count = source - start;

	    SubstString_t * ptr;
	    for ( ptr = tab; ptr->c1; ptr++ )
		if ( ch == ptr->c1 || ch == ptr->c2 )
		{
		    if (ptr->str)
		    {
			const size_t slen = strlen(ptr->str);
			if ( p1 > slen )
			    p1 = slen;
			if ( p2 > slen )
			    p2 = slen;
			count = p2 - p1;
			start = ptr->str+p1;
			conv_count++;
		    }
		    else
			count = 0;
		    break;
		}

	    if (!ptr->c1) // invalid conversion
		convert = 0;

	    if ( count > sizeof(tempbuf)-1 )
		 count = sizeof(tempbuf)-1;
	    TRACE("COPY '%.*s' conv=%d\n",(int)count,start,convert);
	    if ( convert > 0 )
	    {
		char * tp = tempbuf;
		while ( count-- > 0 )
		    *tp++ = toupper((int)*start++);
		*tp = 0;
	    }
	    else if ( convert < 0 )
	    {
		char * tp = tempbuf;
		while ( count-- > 0 )
		    *tp++ = tolower((int)*start++); // cygwin needs the '(int)'
		*tp = 0;
	    }
	    else
	    {
		memcpy(tempbuf,start,count);
		tempbuf[count] = 0;
	    }
	    dest = NormalizeFileName(dest,end,tempbuf,ptr->allow_slash);
	}

    if (count)
	*count = conv_count;
    *dest = 0;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

int ScanEscapeChar ( ccp arg )
{
    if ( arg && strlen(arg) > 1 )
    {
	ERROR0(ERR_SYNTAX,"Illegal character (option --esc): '%s'\n",arg);
	return -1;
    }

    escape_char = arg ? *arg : 0;
    return (unsigned char)escape_char;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Memory Maps			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeMemMap ( MemMap_t * mm )
{
    ASSERT(mm);
    memset(mm,0,sizeof(*mm));
}

///////////////////////////////////////////////////////////////////////////////

void ResetMemMap ( MemMap_t * mm )
{
    ASSERT(mm);

    uint i;
    if (mm->field)
    {
	for ( i = 0; i < mm->used; i++ )
	    free(mm->field[i]);
	free(mm->field);
    }
    memset(mm,0,sizeof(*mm));
}

///////////////////////////////////////////////////////////////////////////////

MemMapItem_t * FindMemMap ( MemMap_t * mm, off_t off, off_t size )
{
    ASSERT(mm);

    off_t off_end = off + size;
    
    int beg = 0;
    int end = mm->used - 1;
    while ( beg <= end )
    {
	uint idx = (beg+end)/2;
	MemMapItem_t * mi = mm->field[idx];
	if ( off_end <= mi->off )
	    end = idx - 1 ;
	else if ( off >= mi->off + mi->size )
	    beg = idx + 1;
	else
	    return mi;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

uint InsertMemMapIndex
(
    // returns the index of the new item

    MemMap_t		* mm,		// mem map pointer
    off_t		off,		// offset of area
    off_t		size		// size of area
)
{
    DASSERT(mm);
    uint idx = FindMemMapHelper(mm,off,size);

    ASSERT( mm->used <= mm->size );
    if ( mm->used == mm->size )
    {
	mm->size += 64;
	mm->field = realloc(mm->field,mm->size*sizeof(MemMapItem_t*));
	if (!mm->field)
	    OUT_OF_MEMORY;
    }

    DASSERT( idx <= mm->used );
    MemMapItem_t ** dest = mm->field + idx;
    memmove(dest+1,dest,(mm->used-idx)*sizeof(MemMapItem_t*));
    mm->used++;

    MemMapItem_t * mi = malloc(sizeof(MemMapItem_t));
    if (!mi)
	OUT_OF_MEMORY;
    mi->off  = off;
    mi->size = size;
    mi->overlap = 0;
    *dest = mi;
    return idx;
}

///////////////////////////////////////////////////////////////////////////////

MemMapItem_t * InsertMemMap
(
    // returns a pointer to a new item (never NULL)

    MemMap_t		* mm,		// mem map pointer
    off_t		off,		// offset of area
    off_t		size		// size of area
)
{
    const uint idx = InsertMemMapIndex(mm,off,size);
    // a C sequence piunt is importand here
    return mm->field[idx];
}

///////////////////////////////////////////////////////////////////////////////

static bool TieMemMap 
(
    // returns true if element are tied togehther

    MemMap_t		* mm,		// mem map pointer
    uint		idx,		// tie element 'idx' and 'idx+1'
    bool		force		// always tie and not only if overlapped
)
{
    DASSERT(mm);
    DASSERT( idx+1 < mm->used );

    MemMapItem_t * i1 = mm->field[idx];
    MemMapItem_t * i2 = mm->field[idx+1];
    if ( force || i1->off + i1->size >= i2->off )
    {
	const off_t new_size = i2->off + i2->size - i1->off;
	if ( i1->size < new_size )
	     i1->size = new_size;
	free(i2);
	idx++;
	mm->used--;
	memmove( mm->field + idx,
		 mm->field + idx + 1,
		 ( mm->used - idx ) * sizeof(MemMapItem_t*) );

	return true;
    }
    
    return false;
}

///////////////////////////////////////////////////////////////////////////////

MemMapItem_t * InsertMemMapTie
(
    // returns a pointer to a new or existing item (never NULL)

    MemMap_t		* mm,		// mem map pointer
    off_t		off,		// offset of area
    off_t		size		// size of area
)
{
    uint idx = InsertMemMapIndex(mm,off,size);

    if ( idx > 0 && TieMemMap(mm,idx-1,false) )
	idx--;

    while ( idx + 1 < mm->used && TieMemMap(mm,idx,false) )
	;

    return mm->field[idx];
}

///////////////////////////////////////////////////////////////////////////////

void InsertMemMapWrapper
(
	void		* param,	// user defined parameter
	u64		offset,		// offset of object
	u64		size,		// size of object
	ccp		info		// info about object
)
{
    noTRACE("InsertMemMapWrapper(%p,%llx,%llx,%s)\n",param,offset,size,info);
    ASSERT(param);
    MemMapItem_t * mi = InsertMemMap(param,offset,size);
    StringCopyS(mi->info,sizeof(mi->info),info);
}

///////////////////////////////////////////////////////////////////////////////

void InsertDiscMemMap
(
	MemMap_t	* mm,		// valid memore map pointer
	wd_disc_t	* disc		// valid disc pointer
)
{
    noTRACE("InsertDiscMemMap(%p,%p)\n",mm,disc);
    DASSERT(mm);
    DASSERT(disc);
    wd_print_mem(disc,InsertMemMapWrapper,mm);
}

///////////////////////////////////////////////////////////////////////////////

uint FindMemMapHelper ( MemMap_t * mm, off_t off, off_t size )
{
    ASSERT(mm);

    int beg = 0;
    int end = mm->used - 1;
    while ( beg <= end )
    {
	uint idx = (beg+end)/2;
	MemMapItem_t * mi = mm->field[idx];
	if ( off < mi->off )
	    end = idx - 1 ;
	else if ( off > mi->off )
	    beg = idx + 1;
	else if ( size < mi->size )
	    end = idx - 1 ;
	else if ( size > mi->size )
	    beg = idx + 1;
	else
	{
	    TRACE("FindMemMapHelper(%llx,%llx) FOUND=%d/%d/%d\n",
		    (u64)off, (u64)size, idx, mm->used, mm->size );
	    return idx;
	}
    }

    TRACE("FindStringFieldHelper(%llx,%llx) failed=%d/%d/%d\n",
		(u64)off, (u64)size, beg, mm->used, mm->size );
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

uint CalCoverlapMemMap ( MemMap_t * mm )
{
    ASSERT(mm);

    uint i, count = 0;
    MemMapItem_t * prev = 0;
    for ( i = 0; i < mm->used; i++ )
    {
	MemMapItem_t * ptr = mm->field[i];
	ptr->overlap = 0;
	if ( prev && ptr->off < prev->off + prev->size )
	{
	    ptr ->overlap |= 1;
	    prev->overlap |= 2;
	    count++;
	}
	prev = ptr;
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

void PrintMemMap ( MemMap_t * mm, FILE * f, int indent )
{
    ASSERT(mm);
    if ( !f || !mm->used )
	return;

    CalCoverlapMemMap(mm);
    indent = NormalizeIndent(indent);

    static char ovl[][3] = { "  ", "!.", ".!", "!!" };

    int i, max_ilen = 4;
    for ( i = 0; i < mm->used; i++ )
    {
	MemMapItem_t * ptr = mm->field[i];
	ptr->info[sizeof(ptr->info)-1] = 0;
	const int ilen = strlen(ptr->info);
	if ( max_ilen < ilen )
	    max_ilen = ilen;
    }

    fprintf(f,"%*s      unused :  off(beg) ..  off(end) :      size : info\n%*s%.*s\n",
	    indent, "", indent, "", max_ilen+52, wd_sep_200 );

    off_t max_end = mm->begin;
    for ( i = 0; i < mm->used; i++ )
    {
	MemMapItem_t * ptr = mm->field[i];
	if ( !i && max_end > ptr->off )
	    max_end = ptr->off;
	const off_t end = ptr->off + ptr->size;
	if ( ptr->off > max_end )
	    fprintf(f,"%*s%s%10llx :%10llx ..%10llx :%10llx : %s\n",
		indent, "", ovl[ptr->overlap&3], (u64)( ptr->off - max_end ),
		(u64)ptr->off, (u64)end, (u64)ptr->size, ptr->info );
	else
	    fprintf(f,"%*s%s           :%10llx ..%10llx :%10llx : %s\n",
		indent, "", ovl[ptr->overlap&3],
		(u64)ptr->off, (u64)end, (u64)ptr->size, ptr->info );
	if ( max_end < end )
	    max_end = end;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			setup files			///////////////
///////////////////////////////////////////////////////////////////////////////

size_t ResetSetup
(
	SetupDef_t * list	// object list terminated with an element 'name=NULL'
)
{
    DASSERT(list);
    size_t count;
    for ( count = 0; list->name; list++, count++ )
    {
	FreeString(list->param);
	list->param = 0;
	list->value = 0;
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanSetupFile
(
	SetupDef_t * list,	// object list terminated with an element 'name=NULL'
	ccp path1,		// filename of text file, part 1
	ccp path2,		// filename of text file, part 2
	bool silent		// true: suppress error message if file not found
)
{
    DASSERT(list);
    DASSERT(path1||path2);

    ResetSetup(list);

    char pathbuf[PATH_MAX];
    ccp path = PathCatPP(pathbuf,sizeof(pathbuf),path1,path2);
    TRACE("ScanSetupFile(%s,%d)\n",path,silent);

    FILE * f = fopen(path,"rb");
    if (!f)
    {
	if (!silent)
	    ERROR1(ERR_CANT_OPEN,"Can't open file: %s\n",path);
	return ERR_CANT_OPEN;
    }
 
    while (fgets(iobuf,sizeof(iobuf)-1,f))
    {
	//----- skip spaces

	char * ptr = iobuf;
	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr++;

	if ( *ptr == '!' || *ptr == '#' )
	    continue;

	//----- find end of name

	char * name = ptr;
	while ( isalnum((int)*ptr) || *ptr == '-' )
	    ptr++;
	if (!*ptr)
	    continue;

	*ptr++ = 0;

	//----- skip spaces and check for '='

	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr++;

	if ( *ptr != '=' )
	    continue;

	//----- check if name is a known parameter

	SetupDef_t * item;
	for ( item = list; item->name; item++ )
	    if (!strcmp(item->name,name))
		break;
	if (!item->name)
	    continue;

	//----- trim parameter

	ptr++; // skip '='
	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr++;

	char * param = ptr;

	while (*ptr)
	    ptr++;

	ptr--;
	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr--;
	ptr[1] = 0;

	item->param = strdup(param);
	if (item->factor)
	{
	    ScanSizeU64(&item->value,param,1,1,0);
	    if ( item->factor > 1 )
		item->value = item->value / item->factor * item->factor;
	}
    }
    
    fclose(f);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			data area & list		///////////////
///////////////////////////////////////////////////////////////////////////////

void SetupDataList
(
    DataList_t		* dl,		// Object for setup
    const DataArea_t	* da		// Source list,
					//  terminated with an element where addr==NULL
					// The content of this area must not changed
					//  while accessing the data list
)
{
    DASSERT(dl);
    memset(dl,0,sizeof(*dl));
    dl->area = da;
}

///////////////////////////////////////////////////////////////////////////////

size_t ReadDataList // returns number of writen bytes
(
    DataList_t		* dl,		// NULL or pointer to data list
    void		* buf,		// destination buffer
    size_t		size		// size of destination buffer
)
{
    u8 * dest = buf;
    size_t written = 0;
    if ( dl && dl->area )
    {
	while ( size > 0 )
	{
	    if (!dl->current.size)
	    {
		noPRINT("NEXT AREA: %p, %p, %zu\n",
			dl->area, dl->area->data, dl->area->size );
		if (!dl->area->data)
		    break;
		memcpy(&dl->current,dl->area++,sizeof(dl->current));
	    }

	    const size_t copy_size = size < dl->current.size ? size : dl->current.size;
	    noPRINT("COPY AREA: %p <- %p, size = %zu=%zx\n",
			dest,dl->current.data,copy_size,copy_size);
	    memcpy(dest,dl->current.data,copy_size);
	    written		+= copy_size;
	    dest		+= copy_size;
	    dl->current.data	+= copy_size;
	    dl->current.size	-= copy_size;
	    size		-= copy_size;
	}
    }
    return written;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			random mumbers			///////////////
///////////////////////////////////////////////////////////////////////////////
// thanx to Donald Knuth

const u32 RANDOM32_C_ADD = 2 * 197731421; // 2 Primzahlen
const u32 RANDOM32_COUNT_BASE = 4294967; // Primzahl == ~UINT_MAX32/1000;

static int random32_a_index = -1;	// Index in die a-Tabelle
static u32 random32_count = 1;		// Abwaerts-Zähler bis zum Wechsel von a,c
static u32 random32_a,
	   random32_c,
	   random32_X;			// Die letzten Werte

static u32 random32_a_tab[] =		// Init-Tabelle
{
    0xbb40e62d, 0x3dc8f2f5, 0xdc024635, 0x7a5b6c8d,
    0x583feb95, 0x91e06dbd, 0xa7ec03f5, 0
};

//-----------------------------------------------------------------------------

u32 Random32 ( u32 max )
{
    if (!--random32_count)
    {
	// Neue Berechnung von random32_a und random32_c faellig

	if ( random32_a_index < 0 )
	{
	    // allererste Initialisierung auf Zeitbasis
	    Seed32Time();
	}
	else
	{
	    random32_c += RANDOM32_C_ADD;
	    random32_a = random32_a_tab[++random32_a_index];
	    if (!random32_a)
	    {
		random32_a_index = 0;
		random32_a = random32_a_tab[0];
	    }

	    random32_count = RANDOM32_COUNT_BASE;
	}
    }

    // Jetzt erfolgt die eigentliche Berechnung

    random32_X = random32_a * random32_X + random32_c;

    if (!max)
	return random32_X;

    return ( (u64)max * random32_X ) >> 32;
}

//-----------------------------------------------------------------------------

u64 Seed32Time ()
{
    struct timeval tval;
    gettimeofday(&tval,NULL);
    const u64 random_time_bits = (u64) tval.tv_usec << 16 ^ tval.tv_sec;
    return Seed32( ( random_time_bits ^ getpid() ) * 197731421u );
}

//-----------------------------------------------------------------------------

u64 Seed32 ( u64 base )
{
    uint a_tab_len = 0;
    while (random32_a_tab[a_tab_len])
	a_tab_len++;
    const u32 base32 = base / a_tab_len;

    random32_a_index	= base % a_tab_len;
    random32_a		= random32_a_tab[random32_a_index];
    random32_c		= ( base32 & 15 ) * RANDOM32_C_ADD + 1;
    random32_X		= base32 ^ ( base >> 32 );
    random32_count	= RANDOM32_COUNT_BASE;

    return base;
}

//-----------------------------------------------------------------------------

void RandomFill ( void * buf, size_t size )
{
    size_t xsize = size / sizeof(u32);
    if (xsize)
    {
	size -= xsize * sizeof(u32);
	u32 * ptr = buf;
	while ( xsize-- > 0 )
	    *ptr++ = Random32(0);
	buf = ptr;
    }

    u8 * ptr = buf;
    while ( size-- > 0 )
	*ptr++ = Random32(0);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    bit handling		///////////////
///////////////////////////////////////////////////////////////////////////////

const uchar TableBitCount[0x100] =
{
	0,1,1,2, 1,2,2,3, 1,2,2,3, 2,3,3,4,
	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,

	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,

	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,

	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,
	4,5,5,6, 5,6,6,7, 5,6,6,7, 6,7,7,8
};

///////////////////////////////////////////////////////////////////////////////

uint Count1Bits ( const void * data, size_t len )
{
    uint count = 0;
    const uchar * d = data;
    while ( len-- > 0 )
	count += TableBitCount[*d++];
    return count;
}

///////////////////////////////////////////////////////////////////////////////

uint Count1Bits8 ( u8 data )
{
    return TableBitCount[data];
}

///////////////////////////////////////////////////////////////////////////////

uint Count1Bits16 ( u16 data )
{
    const u8 * d = (u8*)&data;
    return TableBitCount[d[0]]
	 + TableBitCount[d[1]];
}

///////////////////////////////////////////////////////////////////////////////

uint Count1Bits32 ( u32 data )
{
    const u8 * d = (u8*)&data;
    return TableBitCount[d[0]]
	 + TableBitCount[d[1]]
	 + TableBitCount[d[2]]
	 + TableBitCount[d[3]];
}

///////////////////////////////////////////////////////////////////////////////

uint Count1Bits64 ( u64 data )
{
    const u8 * d = (u8*)&data;
    return TableBitCount[d[0]]
	 + TableBitCount[d[1]]
	 + TableBitCount[d[2]]
	 + TableBitCount[d[3]]
	 + TableBitCount[d[4]]
	 + TableBitCount[d[5]]
	 + TableBitCount[d[6]]
	 + TableBitCount[d[7]];
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    etc				///////////////
///////////////////////////////////////////////////////////////////////////////

size_t AllocTempBuffer ( size_t needed_size )
{
    // 'tempbuf' is only for short usage
    //     ==> don't call other functions while using tempbuf

    // align to 4K
    needed_size = needed_size + 0xfff & ~(size_t)0xfff;

    if ( tempbuf_size < needed_size )
    {
	noPRINT("$$$ ALLOC TEMPBUF, SIZE: %zx > %zx (%s -> %s)\n",
		tempbuf_size, needed_size,
		wd_print_size_1024(0,0,tempbuf_size,false),
		wd_print_size_1024(0,0,needed_size,false) );
	tempbuf_size = needed_size;
	free(tempbuf);
	tempbuf = malloc(needed_size);
	if (!tempbuf)
	    OUT_OF_MEMORY;
    }
    return tempbuf_size;
}

///////////////////////////////////////////////////////////////////////////////

int ScanPreallocMode ( ccp arg )
{
 #ifdef NO_PREALLOC
    static char errmsg[] = "Preallocation not supported and option --prealloc is ignored!\n";
 #endif

    if ( !arg || !*arg )
    {
     #ifdef NO_PREALLOC
	ERROR0(ERR_WARNING,errmsg);
     #else
	prealloc_mode = PREALLOC_OPT_DEFAULT;
     #endif
	return 0;
    }

    static const CommandTab_t tab[] =
    {
	{ PREALLOC_OFF,		"OFF",		"0",	0 },
	{ PREALLOC_SMART,	"SMART",	"1",	0 },
	{ PREALLOC_ALL,		"ALL",		"2",	0 },

	{ 0,0,0,0 }
    };

    const CommandTab_t * cmd = ScanCommand(0,arg,tab);
    if (cmd)
    {
     #ifdef NO_PREALLOC
	if ( cmd->id != PREALLOC_OFF )
	    ERROR0(ERR_WARNING,errmsg);
     #else
	prealloc_mode = cmd->id;
     #endif
	return 0;
    }

    ERROR0(ERR_SYNTAX,"Illegal preallocation mode (option --prealloc): '%s'\n",arg);
    return 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    END				///////////////
///////////////////////////////////////////////////////////////////////////////

