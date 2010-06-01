
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
///////////////			global options			///////////////
///////////////////////////////////////////////////////////////////////////////

bool hook_enabled	= false; // [2do] for testing only, [obsolete]

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

///////////////////////////////////////////////////////////////////////////////
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
	/*H*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*I*/ { REGION_EUR, 1,  "ITAL", "Italian" },
	/*J*/ { REGION_JAP, 1,  "JAPA", "Japan" },
	/*K*/ { REGION_KOR, 1,  "KORE", "Korea" },
	/*L*/ { REGION_JAP, 1,  "J>PL", "Japan->PAL" },
	/*M*/ { REGION_USA, 1,  "A>PL", "America->PAL" },
	/*N*/ { REGION_JAP, 1,  "J>US", "Japan->NTSC" },
	/*O*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*P*/ { REGION_EUR, 1,  "PAL ", "PAL" },
	/*Q*/ { REGION_KOR, 1,  "KO/J", "Korea (japanese)" },
	/*R*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*S*/ { REGION_EUR, 1,  "SPAN", "Spanish" },
	/*T*/ { REGION_KOR, 1,  "KO/E", "Korea (english)" },
	/*U*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*V*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*W*/ { REGION_EUR,  0, "-?- ", "-?-" },
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

///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumModify ScanModify ( ccp arg )
{
    static const CommandTab_t tab[] =
    {
	{ MODIFY__NONE,		"NONE",		"-",	1 },
	{ MODIFY__ALL,		"ALL",		0,	1 },
	{ MODIFY__AUTO,		"AUTO",		0,	1 },

	{ MODIFY_DISC,		"DISC",		0,	0 },
	{ MODIFY_BOOT,		"BOOT",		0,	0 },
	{ MODIFY_TICKET,	"TICKET",	0,	0 },
	{ MODIFY_TMD,		"TMD",		0,	0 },
	{ MODIFY_WBFS,		"WBFS",		0,	0 },

	{ 0,0,0,0 }
    };

    const int stat = ScanCommandList(arg,tab,0);
    if ( stat >= 0 )
	return stat & MODIFY__ALL ? stat & MODIFY__ALL : stat;

    ERROR0(ERR_SYNTAX,"Illegal modify mode (option --modify): '%s'\n",arg);
    return MODIFY__ERROR;
}

//-----------------------------------------------------------------------------

enumModify opt_modify = MODIFY__AUTO;

int ScanOptModify ( ccp arg )
{
    const int new_modify = ScanModify(arg);
    if ( new_modify == MODIFY__ERROR )
	return 1;
    opt_modify = new_modify;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
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

bool PatchId ( void * id, int maxlen, enumModify condition )
{
    ASSERT(id);
    if ( !modify_id || maxlen < 1 || !(opt_modify & condition) )
	return false;

    TRACE("PATCH ID: %.*s -> %.*s\n",maxlen,(ccp)id,maxlen,modify_id);

    ccp src;
    char *dest;
    for ( src = modify_id, dest = id;
	  *src && maxlen > 0;
	  src++, dest++, maxlen-- )
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
    return hook_enabled && PatchId(dest,6,MODIFY_DISC|MODIFY__AUTO);
}

//-----------------------------------------------------------------------------

bool PatchName ( void * name, enumModify condition )
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
///////////////			   PatchMap_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializePatch ( Patch_t * pat )
{
    ASSERT(pat);
    memset(pat,0,sizeof(*pat));
}

///////////////////////////////////////////////////////////////////////////////

void ResetPatch ( Patch_t * pat )
{
    ASSERT(pat);

    if (pat->map)
    {
	ASSERT(pat->map_size);

	int imap;
	for ( imap = 0; imap < pat->map_used; imap++ )
	{
	    PatchMap_t * map = pat->map + imap;
	    if (map->item)
	    {
		ASSERT(map->item_size);

		int iitem;
		for ( iitem = 0; iitem < map->item_used; iitem++ )
		{
		    PatchItem_t * item = map->item + iitem;
		    if ( item->pmode == PATCHMD_DATA_ALLOCED )
			free(item->data);
		}
		free(map->item);
	    }
	}
	free(pat->map);
    }
    
    InitializePatch(pat);
}

///////////////////////////////////////////////////////////////////////////////

void PrintPatch ( Patch_t * pat, FILE * f, int indent, int verbose )
{   
    if ( !pat || !pat->map_used || !f || verbose < 0 )
	return;

    indent = NormalizeIndent(indent);

    // [2do] header + alignment

    ASSERT(pat->map);
    int imap;
    for ( imap = 0; imap < pat->map_used; imap++ )
    {
	PatchMap_t * map = pat->map + imap;
	printf("%11llx ..%11llx :%8x : %s\n",
		map->offset, map->offset + map->size, map->size,
		map->part ? "Partition" : "Disc" );

	ASSERT(map->item);
	int iitem;
	for ( iitem = 0; iitem < map->item_used; iitem++ )
	{
	    PatchItem_t * item = map->item + iitem;
	    printf("%25x :%8x ; %s\n",
		item->offset, item->size, item->comment );
	}
    }
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
    TRACE("RewriteModifiedSF(%p,%p,%p), oft=%d,%d\n",fi,fo,wbfs,fi->iod.oft,fo->iod.oft);

    if (!fi->modified_list.used)
	return ERR_OK;

    UpdateSignatureFST(fi->fst); // NULL allowed

    if ( logging > 2 )
    {
	printf("\nRewrite:\n\n");
	PrintMemMap(&fi->modified_list,stdout,3);
	putchar('\n');
    }

 #ifdef DEBUG
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
		return ERR_CANT_OPEN;
	    close_disc = true;
	}
	SetupIOD(fo,OFT_WBFS,OFT_WBFS);
	fo->wbfs = wbfs;
    }

    int idx;
    enumError err = ERR_OK;
    for ( idx = 0; idx < fi->modified_list.used; idx++ )
    {
	const MemMapItem_t * mmi = fi->modified_list.field[idx];
	err = CopyRawData(fi,fo,mmi->off,mmi->size);
	if (err)
	    break;
    }

    if (close_disc)
    {
	CloseWDisc(wbfs);
	wbfs->disc = 0;
    }

    memcpy(&fo->iod,&iod,sizeof(fo->iod));
    fo->wbfs = saved_wbfs;
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 ISO Modifier			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError ISOModifier
	( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    ASSERT(sf);
    ASSERT(sf->std_read_func);

    TRACE(TRACE_RDWR_FORMAT, "#IM# ISOModifier()",
	GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    return sf->std_read_func(sf,off,buf,count);
}

///////////////////////////////////////////////////////////////////////////////

enumError SetupISOModifier ( SuperFile_t * sf )
{
 #ifndef TEST // [2do]
    hook_enabled = false;
 #endif
 
    ASSERT(sf);
    if ( !hook_enabled || !sf->f.id6 )
	return ERR_OK; // only supported for Wii ISO images

    PatchItem_t * item;

    //----- encryption / decryption

    if ( ( encoding & ENCODE_M_CRYPT ) != 0 )
    {
	// ??? [2do]
    }


    //----- region settings

    if ( opt_region != REGION__AUTO )
    {
	// ??? [2do]
    }


    //----- system version

    if ( opt_ios_valid )
    {
	// ??? [2do]
    }


    //----- system version

    if ( opt_ios_valid )
    {
	// ??? [2do]
    }


    //----- patch ID

    if ( modify_id )
    {
	if ( opt_modify & (MODIFY_DISC|MODIFY__AUTO) )
	{
	    item = InsertDiscDataPM(sf,sf->f.id6,0,6,false);
	    //ASSERT(item);
	}
	// ??? [2do]
    }


    //----- patch NAME

    if ( modify_name )
    {
	// ??? [2do]
    }


    //----- terminate

    if (sf->patch)
    {
	sf->iod.read_func = ISOModifier;
     #ifdef DEBUG
	PrintPatch(sf->patch,TRACE_FILE,3,1);
     #endif
	if (logging)
	    PrintPatch(sf->patch,stdout,3,logging-1);
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static PatchItem_t * InsertDiscPM ( Patch_t * pat, u64 offset, u32 size )
{
    // ??? [2do]
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

PatchItem_t * InsertDiscDataPM
(
	struct SuperFile_t * sf,
	void * data,
	u64 offset,
	u32 size,
	bool alloced
)
{
    ASSERT(sf);
    if (!sf->patch)
    {
	sf->patch = malloc(sizeof(*sf->patch));
	if (!sf->patch)
	    OUT_OF_MEMORY;
	InitializePatch(sf->patch);
    }

    // ??? [2do]
#if 0
    u64 doffset = 
    PatchItem_t * item = InsertDiscPM(pat,offset/HD_SECTOR_SIZE);
    Patch_t * pat = sf->patch;
#endif
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			     E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

