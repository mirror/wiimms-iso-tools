
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

#include "patch.h"
#include "lib-std.h"
#include "lib-sf.h"
#include "wbfs-interface.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--enc				///////////////
///////////////////////////////////////////////////////////////////////////////

int opt_hook = 0; // <0: disabled, =0: auto, >0: enabled

///////////////////////////////////////////////////////////////////////////////

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

enumEncoding encoding = ENCODE_DEFAULT;

int ScanOptEncoding ( ccp arg )
{
    const int new_encoding = ScanEncoding(arg);
    if ( new_encoding == -1 )
	return 1;
    encoding = new_encoding;
    return 0;
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
///////////////			--region			///////////////
///////////////////////////////////////////////////////////////////////////////

enumRegion ScanRegion ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ REGION_JAP,		"JAPAN",	"JAP",		0 },
	{ REGION_USA,		"USA",		0,		0 },
	{ REGION_EUR,		"EUROPE",	"EUR",		0 },
	{ REGION_KOR,		"KOREA",	"KOR",		0 },

	{ REGION__AUTO,		"AUTO",		0,		0 },
	{ REGION__FILE,		"FILE",		0,		0 },

	{ 0,0,0,0 }
    };

    const int stat = ScanCommandListMask(arg,tab);
    if ( stat >= 0 )
	return stat;

    // try if arg is a number
    char * end;
    ulong num = strtoul(arg,&end,10);
    if ( end != arg && !*end )
	return num;

    ERROR0(ERR_SYNTAX,"Illegal region mode (option --region): '%s'\n",arg);
    return REGION__ERROR;
}

//-----------------------------------------------------------------------------

enumRegion opt_region = REGION__AUTO;

int ScanOptRegion ( ccp arg )
{
    const int new_region = ScanRegion(arg);
    if ( new_region == REGION__ERROR )
	return 1;
    opt_region = new_region;
    return 0;
}

//-----------------------------------------------------------------------------

ccp GetRegionName ( enumRegion region, ccp unkown_value )
{
    static ccp tab[] =
    {
	"Japan",
	"USA",
	"Europe",
	"Korea"
    };

    return (unsigned)region < sizeof(tab)/sizeof(*tab)
		? tab[region]
		: unkown_value;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static const RegionInfo_t RegionTable[] =
{
	// -> http://www.wiibrew.org/wiki/Title_Database#Region_Codes

	/*A*/ { REGION_EUR,  0, "ALL ", "All" },
	/*B*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*C*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*D*/ { REGION_EUR, 1,  "GERM", "German" },
	/*E*/ { REGION_USA, 1,  "NTSC", "NTSC" },
	/*F*/ { REGION_EUR, 1,  "FREN", "French" },
	/*G*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*H*/ { REGION_EUR,  0, "NL  ", "Netherlands" },	// ??
	/*I*/ { REGION_EUR, 1,  "ITAL", "Italian" },
	/*J*/ { REGION_JAP, 1,  "JAPA", "Japan" },
	/*K*/ { REGION_KOR, 1,  "KORE", "Korea" },
	/*L*/ { REGION_JAP, 1,  "J>PL", "Japan->PAL" },
	/*M*/ { REGION_USA, 1,  "A>PL", "America->PAL" },
	/*N*/ { REGION_JAP, 1,  "J>US", "Japan->NTSC" },
	/*O*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*P*/ { REGION_EUR, 1,  "PAL ", "PAL" },
	/*Q*/ { REGION_KOR, 1,  "KO/J", "Korea (japanese)" },
	/*R*/ { REGION_EUR,  0, "RUS ", "Russia" },		// ??
	/*S*/ { REGION_EUR, 1,  "SPAN", "Spanish" },
	/*T*/ { REGION_KOR, 1,  "KO/E", "Korea (english)" },
	/*U*/ { REGION_EUR,  0, "AUS ", "Australia" },		// ??
	/*V*/ { REGION_EUR,  0, "SCAN", "Scandinavian" },	// ??
	/*W*/ { REGION_EUR,  0, "CHIN", "China" },		// ??
	/*X*/ { REGION_EUR, 1,  "RF  ", "Region free" },
	/*Y*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*Z*/ { REGION_EUR,  0, "-?- ", "-?-" },

	/*?*/ { REGION_EUR,  0, "-?- ", "-?-" } // illegal region_code
};

//-----------------------------------------------------------------------------

const RegionInfo_t * GetRegionInfo ( char region_code )
{
    region_code = toupper((int)region_code);
    if ( region_code < 'A' || region_code > 'Z' )
	region_code = 'Z' + 1;
    return RegionTable + (region_code-'A');
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--common-key			///////////////
///////////////////////////////////////////////////////////////////////////////

wd_ckey_index_t ScanCommonKey ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ WD_CKEY_STANDARD,	"STANDARD",	0,		0 },
	{ WD_CKEY_KOREA,	"KOREAN",	0,		0 },
	{ WD_CKEY__N,		"AUTO",		0,		0 },

	{ 0,0,0,0 }
    };

    const int stat = ScanCommandListMask(arg,tab);
    if ( stat >= 0 )
	return stat;

    // try if arg is a number
    char * end;
    ulong num = strtoul(arg,&end,10);
    if ( end != arg && !*end && num < WD_CKEY__N )
	return num;

    ERROR0(ERR_SYNTAX,"Illegal common key index (option --common-key): '%s'\n",arg);
    return -1;
}

//-----------------------------------------------------------------------------

enumRegion opt_common_key = WD_CKEY__N;

int ScanOptCommonKey ( ccp arg )
{
    const wd_ckey_index_t new_common_key = ScanCommonKey(arg);
    if ( new_common_key == -1 )
	return 1;
    opt_common_key = new_common_key;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--ios				///////////////
///////////////////////////////////////////////////////////////////////////////

u64 opt_ios = 0;
bool opt_ios_valid = false;

//-----------------------------------------------------------------------------

bool ScanSysVersion ( u64 * ios, ccp arg )
{
    u32 stat, lo, hi = 1;
    
    arg = ScanNumU32(arg,&stat,&lo,0,~(u32)0);
    if (!stat)
	return false;

    if ( *arg == ':' || *arg == '-' )
    {
	arg++;
	hi = lo;
	arg = ScanNumU32(arg,&stat,&lo,0,~(u32)0);
	if (!stat)
	    return false;
    }

    if (ios)
	*ios = (u64)hi << 32 | lo;

    return !*arg;
}

//-----------------------------------------------------------------------------

int ScanOptIOS ( ccp arg )
{
    opt_ios = 0;
    opt_ios_valid = false;

    if ( !arg || !*arg )
	return 0;

    opt_ios_valid = ScanSysVersion(&opt_ios,arg);
    if (opt_ios_valid)
	return 0;

    ERROR0(ERR_SYNTAX,"Illegal system version (option --ios): %s\n",arg);
    return 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--modify			///////////////
///////////////////////////////////////////////////////////////////////////////

wd_modify_t ScanModify ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ WD_MODIFY__NONE,	"NONE",		"-",	WD_MODIFY__ALL },
	{ WD_MODIFY__ALL,	"ALL",		0,	WD_MODIFY__ALL },
	{ WD_MODIFY__AUTO,	"AUTO",		0,	WD_MODIFY__ALL },

	{ WD_MODIFY_DISC,	"DISC",		0,	0 },
	{ WD_MODIFY_BOOT,	"BOOT",		0,	0 },
	{ WD_MODIFY_TICKET,	"TICKET",	0,	0 },
	{ WD_MODIFY_TMD,	"TMD",		0,	0 },
	{ WD_MODIFY_WBFS,	"WBFS",		0,	0 },

	{ 0,0,0,0 }
    };

    const int stat = ScanCommandList(arg,tab,0,true,0,0);
    if ( stat >= 0 )
	return ( stat & WD_MODIFY__ALL ? stat & WD_MODIFY__ALL : stat ) | WD_MODIFY__ALWAYS;

    ERROR0(ERR_SYNTAX,"Illegal modify mode (option --modify): '%s'\n",arg);
    return -1;
}

//-----------------------------------------------------------------------------

wd_modify_t opt_modify = WD_MODIFY__AUTO | WD_MODIFY__ALWAYS;

int ScanOptModify ( ccp arg )
{
    const int new_modify = ScanModify(arg);
    if ( new_modify == -1 )
	return 1;
    ASSERT( new_modify & WD_MODIFY__ALWAYS );
    opt_modify = new_modify;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--id & --name			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp modify_id		= 0;
ccp modify_name		= 0;

static char modify_id_buf[7];
static char modify_name_buf[WII_TITLE_SIZE];

//-----------------------------------------------------------------------------

int ScanOptId ( ccp arg )
{
    if ( !arg || !*arg )
    {
	modify_id = 0;
	return 0;
    }

    const size_t max_len = sizeof(modify_id_buf) - 1;
    size_t len = strlen(arg);
    if ( len > max_len )
    {
	ERROR0(ERR_SYNTAX,"option --id: id is %d characters to long: %s\n",
		len - max_len, arg);
	return 1;
    }

    memset(modify_id_buf,0,sizeof(modify_id_buf));
    char *dest = modify_id_buf;
    while ( *arg )
	if ( *arg == '.' || *arg == '_' )
	    *dest++ = *arg++;
	else if ( isalnum((int)*arg) )
	    *dest++ = toupper((int)*arg++);
	else
	{
	    ERROR0(ERR_SYNTAX,"option --id: illaegal character at index #%u: %s\n",
		    dest - modify_id_buf, arg);
	    return 1;
	}
    modify_id = modify_id_buf;

    return 0;
}

//-----------------------------------------------------------------------------

int ScanOptName ( ccp arg )
{
    if ( !arg || !*arg )
    {
	modify_name = 0;
	return 0;
    }

    const size_t max_len = sizeof(modify_name_buf) - 1;
    size_t len = strlen(arg);
    if ( len > max_len )
    {
	ERROR0(ERR_WARNING,"option --name: name is %d characters to long:\n!\t-> %s\n",
		len - max_len, arg);
	len = max_len;
    }
	
    ASSERT( len < sizeof(modify_name_buf) );
    memset(modify_name_buf,0,sizeof(modify_name_buf));
    memcpy(modify_name_buf,arg,len);
    modify_name = modify_name_buf;
    
    return 0;
}

//-----------------------------------------------------------------------------

bool PatchId ( void * id, int skip, int maxlen, wd_modify_t condition )
{
    ASSERT(id);
    if ( !modify_id || maxlen < 1 || !(opt_modify & condition) )
	return false;

    TRACE("PATCH ID: %.*s -> %.*s\n",maxlen,(ccp)id,maxlen,modify_id);

    ccp src = modify_id;
    while ( skip-- > 0 && *src )
	src++;

    char *dest;
    for ( dest = id; *src && maxlen > 0; src++, dest++, maxlen-- )
    {
	if ( *src != '.' )
	    *dest = *src;
    }
    return true;
}

//-----------------------------------------------------------------------------

bool CopyPatchedDiscId ( void * dest, const void * src )
{
    memcpy(dest,src,6);
    ((char*)dest)[6] = 0;
    return opt_hook >= 0 && PatchId(dest,0,6,WD_MODIFY_DISC|WD_MODIFY__AUTO);
}

//-----------------------------------------------------------------------------

bool PatchName ( void * name, wd_modify_t condition )
{
    ASSERT(name);
    if ( !modify_name || !(opt_modify & condition) )
	return false;

    ASSERT( strlen(modify_name) < WII_TITLE_SIZE );
    memcpy(name,modify_name,WII_TITLE_SIZE);
    return true;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--trim				///////////////
///////////////////////////////////////////////////////////////////////////////

enumTrim opt_trim = TRIM_DEFAULT;

//-----------------------------------------------------------------------------

enumTrim ScanTrim
(
    ccp arg,			// argument to scan
    ccp err_text_extend		// error message extention
)
{
    static const CommandTab_t tab[] =
    {
	{ TRIM_DEFAULT,		"DEFAULT",	0,	TRIM_M_ALL },

	{ TRIM_NONE,		"NONE",		"-",	TRIM_ALL },
	{ TRIM_ALL,		"ALL",		0,	TRIM_ALL },
	{ TRIM_FAST,		"FAST",		0,	TRIM_ALL },

	{ TRIM_DISC,		"DISC",		"D",	TRIM_DEFAULT },
	{ TRIM_PART,		"PARTITION",	"P",	TRIM_DEFAULT },
	{ TRIM_FST,		"FILESYSTEM",	"F",	TRIM_DEFAULT },

	{ 0,			"BEGIN",	0,	TRIM_DEFAULT | TRIM_F_END },
	{ TRIM_F_END,		"END",		0,	TRIM_DEFAULT | TRIM_F_END },

	{ 0,0,0,0 }
    };

    const int stat = ScanCommandList(arg,tab,0,true,0,0);
    if ( stat >= 0 )
	return stat;

    ERROR0(ERR_SYNTAX,"Illegal trim mode%s: '%s'\n",err_text_extend,arg);
    return -1;
}

//-----------------------------------------------------------------------------

int ScanOptTrim
(
    ccp arg			// argument to scan
)
{
    const int new_trim = ScanTrim(arg," (option --trim)");
    if ( new_trim == -1 )
	return 1;
    opt_trim = new_trim;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--align & --align-part		///////////////
///////////////////////////////////////////////////////////////////////////////

u32 opt_align1		= 0;
u32 opt_align2		= 0;
u32 opt_align3		= 0;
u32 opt_align_part	= 0;

//-----------------------------------------------------------------------------

int ScanOptAlign ( ccp p_arg )
{
    opt_align1 = opt_align2 = opt_align3 = 0;

    static u32 * align_tab[] = { &opt_align1, &opt_align2, &opt_align3, 0 };
    u32 ** align_ptr = align_tab;

    ccp arg = p_arg, prev_arg = 0;
    char * dest_end = iobuf + sizeof(iobuf) - 1;

    while ( arg && *arg && *align_ptr )
    {
	ccp save_arg = arg;
	char * dest = iobuf;
	while ( dest < dest_end && *arg && *arg != ',' )
	    *dest++ = *arg++;
	*dest = 0;
	if ( dest > iobuf && ScanSizeOptU32(
				*align_ptr,	// u32 * num
				iobuf,		// ccp source
				1,		// default_factor1
				0,		// int force_base
				"align",	// ccp opt_name
				4,		// u64 min
				WII_SECTOR_SIZE,// u64 max
				0,		// u32 multiple
				1,		// u32 pow2
				true		// bool print_err
				))
	    return 1;

	if ( prev_arg && align_ptr[-1][0] >= align_ptr[0][0] )
	{
	    ERROR0(ERR_SEMANTIC,
			"Option --align: Ascending order expected: %.*s\n",
			arg - prev_arg, prev_arg );
	    return 1;
	}

	align_ptr++;
	if ( !*align_ptr || !*arg )
	    break;
	arg++;
	prev_arg = save_arg;
    }

    if (*arg)
    {
	ERROR0(ERR_SEMANTIC,
		"Option --align: End of parameter expected: %.20s\n",arg);
	return 1;
    }
	
    return 0;
}

//-----------------------------------------------------------------------------

int ScanOptAlignPart ( ccp arg )
{
    return ScanSizeOptU32(
		&opt_align_part,	// u32 * num
		arg,			// ccp source
		1,			// default_factor1
		0,			// int force_base
		"align-part",		// ccp opt_name
		WII_SECTOR_SIZE,	// u64 min
		WII_GROUP_SIZE,		// u64 max
		0,			// u32 multiple
		1,			// u32 pow2
		true			// bool print_err
		) != ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--disc-size			///////////////
///////////////////////////////////////////////////////////////////////////////

u64 opt_disc_size = 0;

//-----------------------------------------------------------------------------

int ScanOptDiscSize ( ccp arg )
{
    return ScanSizeOptU64(&opt_disc_size,arg,GiB,0,"disc-size",
		0, WII_SECTOR_SIZE * (u64)WII_MAX_SECTORS,
		WII_SECTOR_SIZE, 0, true ) != ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    --add-files & --rm-files		///////////////
///////////////////////////////////////////////////////////////////////////////

StringField_t add_file;
StringField_t repl_file;

//-----------------------------------------------------------------------------

int ScanOptFile ( ccp arg, bool add )
{
    if ( arg && *arg )
    {
	StringField_t * sf = add ? &add_file : &repl_file;
	InsertStringField(sf,arg,false);
    }
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		      RewriteModifiedSF()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError RewriteModifiedSF ( SuperFile_t * fi, SuperFile_t * fo, WBFS_t * wbfs )
{
    ASSERT(fi);
    ASSERT(fi->f.is_reading);
    if (wbfs)
	fo = wbfs->sf;
    ASSERT(fo);
    ASSERT(fo->f.is_writing);
    TRACE("+++ RewriteModifiedSF(%p,%p,%p), oft=%d,%d\n",fi,fo,wbfs,fi->iod.oft,fo->iod.oft);

    wd_disc_t * disc = fi->disc1;
    if ( !fi->modified_list.used && ( !disc || !disc->reloc ))
    {
	TRACE("--- RewriteModifiedSF() ERR_OK: nothing to do\n");
	return ERR_OK;
    }

    UpdateSignatureFST(fi->fst); // NULL allowed

    if ( logging > 2 )
    {
	printf("\nRewrite:\n\n");
	PrintMemMap(&fi->modified_list,stdout,3);
	putchar('\n');
    }

 #ifdef DEBUG
    fprintf(TRACE_FILE,"Rewrite:\n");
    PrintMemMap(&fi->modified_list,TRACE_FILE,3);
 #endif

    IOData_t iod;
    memcpy(&iod,&fo->iod,sizeof(iod));
    WBFS_t * saved_wbfs = fo->wbfs;
    bool close_disc = false;

    if (wbfs)
    {
	TRACE(" - WBFS stat: w=%p, disc=#%d,%p, oft=%d\n",
		wbfs, wbfs->disc_slot, wbfs->disc, fo->iod.oft );
	if (!wbfs->disc)
	{
	    OpenWDiscSlot(wbfs,wbfs->disc_slot,0);
	    if (!wbfs->disc)
	    {
		TRACE("--- RewriteModifiedSF() ERR_CANT_OPEN: wbfs disc\n");
		return ERR_CANT_OPEN;
	    }
	    close_disc = true;
	}
	SetupIOD(fo,OFT_WBFS,OFT_WBFS);
	fo->wbfs = wbfs;
    }

    int idx;
    enumError err = ERR_OK;
    for ( idx = 0; idx < fi->modified_list.used && !err; idx++ )
    {
	const MemMapItem_t * mmi = fi->modified_list.field[idx];
	err = CopyRawData(fi,fo,mmi->off,mmi->size);
    }

    if ( disc && disc->reloc )
    {
	const wd_reloc_t * reloc = disc->reloc;
	for ( idx = 0; idx < WII_MAX_SECTORS && !err; idx++, reloc++ )
	    if ( *reloc & WD_RELOC_F_LAST )
		err = CopyRawData(fi,fo,idx*WII_SECTOR_SIZE,WII_SECTOR_SIZE);
    }

    if (close_disc)
    {
	CloseWDisc(wbfs);
	wbfs->disc = 0;
    }

    memcpy(&fo->iod,&iod,sizeof(fo->iod));
    fo->wbfs = saved_wbfs;
    TRACE("--- RewriteModifiedSF() err=%u: END\n");
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			     E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

