
#define _GNU_SOURCE 1

#include "debug.h"
#include "wiidisc.h"
#include "lib-std.h"
#include "lib-sf.h"
#include "iso-interface.h"

#include "ui-wit.h"

//-----------------------------------------------------------------------------

void print_title ( FILE * f );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct Mix_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct Mix_t
{
    //--- input data

    ccp		source;		// pointer to source file ( := param->arg )
    u32		ptab;		// destination partition tables
    u32		ptype;		// destination partition type

    //--- secondary data
    
    SuperFile_t	* sf;		// superfile of iso image
    bool	have_pat;	// true if partitons have ignore pattern
    bool	free_data;	// true: free 'sf'
    wd_disc_t	* disc;		// valid disc pointer
    wd_part_t	* part;		// valid partition pointer

    u32		src_sector;	// index of first source sector
    u32		src_end_sector;	// index of last source sector + 1
    u32		dest_sector;	// index of first destination sector
    u32		n_blocks;	// number of continuous sector blocks

    //--- Permutation helpers

    u32		a1size;		// size of area 1 (p1off allways = 0)
    u32		a2off;		// offset of area 2
    u32		a2size;		// size of area 2

    //--- sector mix data

    u32		* used_free;	// 2*n_blocks+1 used and free blocks counts
    
} Mix_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct MixFree_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct MixFree_t
{
	u32 off;		// offset of first free sector
	u32 size;		// number of free sectors

} MixFree_t;


//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct MixParam_t		///////////////
///////////////////////////////////////////////////////////////////////////////

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

//
///////////////////////////////////////////////////////////////////////////////
///////////////			verify_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError verify_mix ( Mix_t * mixtab, int n_mix, bool dump )
{
    ASSERT(mixtab);
    
    u8 utab[WII_MAX_SECTORS];
    memset(utab,WD_USAGE_UNUSED,sizeof(utab));
    utab[ 0 ]					= WD_USAGE_DISC;
    utab[ WII_PTAB_REF_OFF / WII_SECTOR_SIZE ]	= WD_USAGE_DISC;
    utab[ WII_REGION_OFF   / WII_SECTOR_SIZE ]	= WD_USAGE_DISC;
    utab[ WII_MAGIC2_OFF   / WII_SECTOR_SIZE ]	= WD_USAGE_DISC;

    u32 error_count = 0;
    u8 dest_id = WD_USAGE_PART_0;
    Mix_t *mix, *endmix = mixtab + n_mix;
    for ( mix = mixtab; mix < endmix; mix++, dest_id++ )
    {
	wd_part_t * part = mix->part;
	DASSERT(part);
	const u8 src_id = part->usage_id;

	int max_sectors = WII_MAX_SECTORS - ( mix->src_sector > mix->dest_sector
					    ? mix->src_sector : mix->dest_sector );
	u8 *dest = utab + mix->dest_sector, *enddest = dest + max_sectors;
	const u8 *src = mix->disc->usage_table + mix->src_sector;
	for ( ; dest < enddest; dest++, src++ )
	    if ( ( *src & WD_USAGE__MASK ) == src_id )
	    {
		if (*dest)
		    error_count++;
		*dest = dest_id | *src & WD_USAGE_F_CRYPT;
	    }
    }

    if (dump)
    {
	printf("Usage table:\n\n");
	wd_dump_usage_tab(stdout,2,utab,false);
	putchar('\n');
    }

    if (error_count)
	return ERROR0(ERR_INTERNAL,"Verification of overlay failed!\n");
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			mark_used_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

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
    noTRACE("MARK=%x+%x, ptr=%x+%x\n",off,size,ptr->off,ptr->size);
    ASSERT_MSG( off + size <= ptr->size,
		"off+size=%x+%x=%x <= ptr->size=%x\n",
		off, size, off+size, ptr->size );

    if (!off)
    {
	noTRACE("\t\t\t\t\t> %x..%x -> %x..%x [%u/%u]\n",
		ptr->off, ptr->off + ptr->size,
		ptr->off + size, ptr->off + ptr->size,
		(int)(ptr-base), used );
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
	noTRACE("\t\t\t\t\t> %x..%x -> %x..%x [%u/%u]\n",
		ptr->off, ptr->off + ptr->size,
		ptr->off, ptr->off + ptr->size - size,
		(int)(ptr-base), used );
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
    noTRACE("\t\t\t\t\t> %x..%x -> %x..%x,%x..%x [%u/%u>%u]\n",
		old_off, old_off + old_size,
		ptr[0].off, ptr[0].off + ptr[0].size,
		ptr[1].off, ptr[1].off + ptr[1].size,
		(int)(ptr-base), used, used+1 );
    return used+1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			insert_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void insert_mix ( MixParam_t * p, int pdepth, int mix_index )
{
    DASSERT( p );
    DASSERT( pdepth >= 0 && pdepth < p->n_mix );
    DASSERT( mix_index >= 0 && mix_index < p->n_mix );
    
    MixFree_t * f = p->field[pdepth];
    Mix_t * mix = p->mix + mix_index;

    noTRACE("MIX#%u, a1=0+%x, a2=%x+%x\n",mix_index,mix->a1size,mix->a2off,mix->a2size);
    if (!pdepth)
    {
	// setup free table
	noTRACE("\t\t\t\t\t> SETUP\n");
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

 #if defined(TEST) && defined(DEBUG) && 0
    {
	int i;
	for ( i = 0; i < p->used[pdepth]; i++ )
	    printf(" %5x .. %8x / %8x\n",
		f[i].off, f[i].off + f[i].size, f[i].size ); 
    }
 #endif
}
 
//
///////////////////////////////////////////////////////////////////////////////
///////////////			permutate_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

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

	noTRACE(" src=%d/%d, depth=%d\n",src,n_perm,pdepth);
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

     #if defined(TEST) && 0
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
	p->mix[i].dest_sector = delta[i];
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			setup_sector_mix()		///////////////
///////////////////////////////////////////////////////////////////////////////

static void setup_sector_mix ( Mix_t * mix )
{
    DASSERT(mix);
    DASSERT(mix->disc);
    DASSERT(mix->part);
    DASSERT(!mix->used_free);

    const int uf_count = 2 * mix->n_blocks + 1;
    u32 * uf = calloc(uf_count,sizeof(u32));
    if (!uf)
	OUT_OF_MEMORY;
    mix->used_free = uf;

    wd_usage_t usage_id = mix->part->usage_id;
    u8 * utab = wd_calc_usage_table(mix->disc);
    u32 src_sector = mix->src_sector;
    u32 last_value = src_sector;

    while ( src_sector < mix->src_end_sector )
    {
	while ( src_sector < mix->src_end_sector
		&& ( utab[src_sector] & WD_USAGE__MASK ) == usage_id )
	    src_sector++;
	*uf++ = src_sector - last_value;
	last_value = src_sector;

	while ( src_sector < mix->src_end_sector
		&& ( utab[src_sector] & WD_USAGE__MASK ) != usage_id )
	    src_sector++;
	*uf++ = src_sector - last_value;
	last_value = src_sector;

	noTRACE("SETUP: %5x %5x\n",uf[-2],uf[-1]);
	ASSERT( uf < mix->used_free + uf_count );
    }
    
    ASSERT( uf+1 == mix->used_free + uf_count );
    uf[-1] = ~(u32)0 >> 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sector_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void sector_mix ( MixParam_t * p, Mix_t * m1, Mix_t * m2 )
{
    DASSERT(p);
    DASSERT(m1);
    DASSERT(m2);
    DASSERT(m1->used_free);
    DASSERT(m2->used_free);

    const u32 n_sect2 = m2->src_end_sector - m2->src_sector;
    const u32 max_delta = p->max_end - n_sect2 + 1;

    u32 delta = *m1->used_free;

    for(;;)
    {
     restart:
	PRINT("TRY DELTA %x/%x\n",delta,max_delta);

	if ( delta >= max_delta )
	    return; // abort

	u32 * uf1 = m1->used_free;
	u32 * uf2 = m2->used_free;
	u32 hole_size = delta;

	for(;;)
	{
	    //---- uf1

	    for(;;)
	    {
		noTRACE("\tuf1a=%8x,%8x, hole_size=%5x\n",uf1[0],uf1[1],hole_size);
		const u32 used_val = *uf1++;
		if (!used_val)
		    goto ok;
		if ( hole_size < used_val )
		{
		    delta += used_val - hole_size;
		    goto restart;
		}
		hole_size -= used_val;

		noTRACE("\tuf1b=%8x,%8x, hole_size=%5x\n",uf1[0],uf1[1],hole_size);
		const u32 free_val = *uf1++;
		DASSERT(free_val);
		if ( hole_size <= free_val )
		{
		    hole_size = free_val - hole_size;
		    break;
		}
		hole_size -= free_val;
	    }

	    //---- uf2

	    for(;;)
	    {
		noTRACE("\tuf2a=%8x,%8x, hole_size=%5x\n",uf2[0],uf2[1],hole_size);
		const u32 used_val = *uf2++;
		if (!used_val)
		    goto ok;
		if ( hole_size < used_val )
		{
		    BINGO;
		    delta += used_val - hole_size;
		    goto restart;
		}
		hole_size -= used_val;

		noTRACE("\tuf2b=%8x,%8x, hole_size=%5x\n",uf2[0],uf2[1],hole_size);
		const u32 free_val = *uf2++;
		DASSERT(free_val);
		if ( hole_size <= free_val )
		{
		    hole_size = free_val - hole_size;
		    break;
		}
		hole_size -= free_val;
	    }
	}

	break;
    }
    
 ok:;
    const u32 max_end = delta + n_sect2;
    PRINT(">>> FOUND: delta=%x, max_end=%x %c %x\n",
		delta, max_end,
		max_end < p->max_end ? '<' : max_end > p->max_end ? '>' : '=',
		p->max_end);

    if ( max_end < p->max_end )
    {
	const u32 start = WII_GOOD_UPDATE_PART_OFF / WII_SECTOR_SIZE;
	m1->dest_sector = start;
	m2->dest_sector = start + delta;
	p->max_end = max_end;
     #ifdef TEST
	verify_mix(p->mix,p->n_mix,false);
     #endif
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sector_mix_2()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void sector_mix_2 ( MixParam_t * p )
{
    DASSERT(p);
    DASSERT( p->n_mix == 2 );
    
    setup_sector_mix(p->mix);
    setup_sector_mix(p->mix+1);

    PRINT("SECTOR MIX 0,1\n");
    sector_mix(p,p->mix,p->mix+1);

    PRINT("SECTOR MIX 1,0\n");
    sector_mix(p,p->mix+1,p->mix);
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			cmd_mix()			///////////////
///////////////////////////////////////////////////////////////////////////////

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

	AtExpandParam(&param);
	ccp srcfile = param->arg;
	if ( src_warn && !strchr(srcfile,'/') && !strchr(srcfile,'.') )
	{
	    src_warn = false;
	    ERROR0(ERR_WARNING,
		"Warning: use at least one '.' or '/' in a filename"
		" to distinguish file names from keywords: %s", srcfile );
	}
	param = param->next;


	//--- scan qualifiers
	
	bool scan_qualifier = true;
	wd_select_t psel;
	wd_initialize_select(&psel);
	wd_append_select_item(&psel,WD_SM_DENY_PTAB,1,0);
	wd_append_select_item(&psel,WD_SM_DENY_PTAB,2,0);
	wd_append_select_item(&psel,WD_SM_DENY_PTAB,3,0);
	wd_append_select_item(&psel,WD_SM_ALLOW_PTYPE,0,WD_PART_DATA);

	u32 ptab = 0, ptype = 0;
	bool ptab_valid = false, ptype_valid = false;
	bool pattern_active = false;
	ResetFilePattern( file_pattern + PAT_PARAM );

	while ( scan_qualifier )
	{
	    scan_qualifier = false;
	    AtExpandParam(&param);

	    //--- scan 'SELECT'

	    if ( param && param->next
		 && (  !strcasecmp(param->arg,"select")
		    || !strcasecmp(param->arg,"psel") ))
	    {
		param = param->next;
		AtExpandParam(&param);
		const enumError err = ScanPartSelector(&psel,param->arg," ('select')");
		if (err)
		    return err;
		param = param->next;
		scan_qualifier = true;
	    }


	    //--- scan 'AS'

	    if ( param && param->next && !strcasecmp(param->arg,"as") )
	    {
		param = param->next;
		AtExpandParam(&param);
		const enumError err
		    = ScanPartTabAndType(&ptab,&ptab_valid,&ptype,&ptype_valid,
					    param->arg," ('as')");
		if (err)
		    return err;
		param = param->next;
		scan_qualifier = true;
	    }


	    //--- scan 'IGNORE'

	    if ( param && param->next && !strcasecmp(param->arg,"ignore") )
	    {
		param = param->next;
		if (AtFileHelper(param->arg,PAT_PARAM,PAT_PARAM,AddFilePattern))
		    return ERR_SYNTAX;
		param = param->next;
		pattern_active = true;
		scan_qualifier = true;
	    }
	}


	//--- analyze pattern

	FilePattern_t * pat = 0;
	if (pattern_active)
	{
	    pat = file_pattern + PAT_PARAM;
	    SetupFilePattern(pat);
	    DefineNegatePattern(pat,true);
	    if (!pat->rules.used)
		pat = 0;
	}


	//--- open disc and partitions

	SuperFile_t * sf = AllocSF();
	ASSERT(sf);
	enumError err = OpenSF(sf,srcfile,false,false);
	if (err)
	    return err;

	wd_disc_t * disc = OpenDiscSF(sf,false,true);
	if (!disc)
	    return ERR_WDISC_NOT_FOUND;

	//wd_print_select(stdout,3,&psel);
	wd_select(disc,&psel);
	wd_reset_select(&psel);

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
	    mix->have_pat	= pat != 0;
	    mix->disc		= disc;
	    mix->part		= part;
	    mix->dest_sector	= sector;
	    mix->src_sector	= part->part_off4 / WII_SECTOR_SIZE4;
	    ptab_count[ptab]++;


	    //--- find largest hole

	    if (pat)
		wd_select_part_files(part,IsFileSelected,pat);
	    wd_usage_t usage_id = part->usage_id;
	    u8 * utab = wd_calc_usage_table(mix->disc);

	    u32 src_sector = mix->src_sector;
	    u32 max_hole_off = 0, max_hole_size = 0;

	    for(;;)
	    {
		mix->n_blocks++;
		while ( src_sector < part->end_sector
			&& ( utab[src_sector] & WD_USAGE__MASK ) == usage_id )
		    src_sector++;
		mix->src_end_sector = src_sector;

		while ( src_sector < part->end_sector
			&& ( utab[src_sector] & WD_USAGE__MASK ) != usage_id )
		    src_sector++;

		if ( src_sector == part->end_sector )
		    break;

		const u32 hole_size = src_sector - mix->src_end_sector;
		if ( hole_size > max_hole_size )
		{
		    max_hole_size = hole_size;
		    max_hole_off  = mix->src_end_sector;
		}
	    }

	    if (max_hole_size)
	    {
		mix->a1size = max_hole_off - mix->src_sector;
		mix->a2off  = mix->a1size + max_hole_size;
		mix->a2size = mix->src_end_sector - (max_hole_off+max_hole_size);
	    }
	    else
	    {
		mix->a1size = mix->src_end_sector - mix->src_sector;
		mix->a2off  = 0;
		mix->a2size = 0;
	    }

	    sector += mix->src_end_sector - mix->src_sector;
	    if ( sector & 1 )
		sector++; // align to 64KiB

	    PRINT("N-BLOCKS=%d, A1=0+%x, A2=%x+%x\n",
			mix->n_blocks, mix->a1size, mix->a2off, mix->a2size );
	}

	if (!mix)
	    return ERROR0(ERR_SEMANTIC,"No partition selected: %s\n",srcfile);
	mix->free_data = true;
    }

    if (!n_mix)
	return ERROR0(ERR_SEMANTIC,"No source partition selected.\n");

    PRINT("*** CHAIN => END SECTOR = %x\n",sector);

    //----- permutate

    enum { MD_STD, MD_PERM, MD_SMIX } mode = MD_STD;

    if ( n_mix > 1 && OptionUsed[OPT_OVERLAY] )
    {
	MixParam_t p;
	memset(&p,0,sizeof(p));
	p.mix = mixtab;
	p.n_mix = n_mix;
	permutate_mix(&p);
	if ( sector > p.max_end )
	{
	    mode = MD_PERM;
	    PRINT("*** PERMUTATE => END SECTOR = %x < %x\n",p.max_end,sector);
	    sector = p.max_end;
	}

	if ( n_mix == 2 )
	{   
	    sector_mix_2(&p);
	    if ( sector > p.max_end )
	    {
		mode = MD_SMIX;
		PRINT("*** SECTOR MIX 2 -> END SECTOR = %x < %x\n",p.max_end,sector);
		sector = p.max_end;
	    }
	}
    }


    //----- check max sectors

    if ( sector > WII_MAX_SECTORS )
	return ERROR0(ERR_SEMANTIC,
		"Total size (%u MiB) exceeds maximum size (%u MiB).\n",
		sector / WII_SECTORS_PER_MIB,
		WII_MAX_SECTORS / WII_SECTORS_PER_MIB );


    //----- print log table

    enumError err = verify_mix( mixtab, n_mix, testmode > 1 || verbose > 2 );

    Mix_t *mix, *end_mix = mixtab + n_mix;

    if ( testmode || verbose > 1 )
    {
	int src_fw = 13;
	for ( mix = mixtab; mix < end_mix; mix++ )
	{
	    const int len = strlen(mix->source);
	    if ( src_fw < len )
		 src_fw = len;
	}

	printf("\nMix table (%d partitons, total size=%llu MiB):\n\n",
		    n_mix, sector* (u64)WII_SECTOR_SIZE / MiB );

	if ( mode < MD_SMIX )
	{
	    printf("    blocks #1      blocks #2  :      disc offset     "
		    ": ptab  ptype : ignore + source\n"
		    "  %.*s\n",
		    69 + src_fw, wd_sep_200 );

	    for ( mix = mixtab; mix < end_mix; mix++ )
	    {
		if (mix->a2off)
		{
		    const u32 end_sector = mix->dest_sector + mix->a2off + mix->a2size;
		    printf("%7x..%5x + %5x..%5x : %9llx..%9llx : %u %s : %c %s\n",
			mix->dest_sector,
			mix->dest_sector + mix->a1size,
			mix->dest_sector + mix->a2off,
			end_sector,
			mix->dest_sector * (u64)WII_SECTOR_SIZE,
			end_sector * (u64)WII_SECTOR_SIZE,
			mix->ptab,
			wd_print_part_name(0,0,mix->ptype,WD_PNAME_COLUMN_9),
			mix->have_pat ? '+' : '-', mix->source );
		}
		else
		{
		    const u32 end_sector = mix->dest_sector + mix->a1size;
		    printf("%7x..%5x                : %9llx..%9llx : %u %s : %c %s\n",
			mix->dest_sector, end_sector,
			mix->dest_sector * (u64)WII_SECTOR_SIZE,
			end_sector * (u64)WII_SECTOR_SIZE,
			mix->ptab,
			wd_print_part_name(0,0,mix->ptype,WD_PNAME_COLUMN_9),
			mix->have_pat ? '+' : '-', mix->source );
		}
	    }
	}
	else
	{
	    printf("  n(blks) :      disc offset     : ptab  ptype : ignore + source\n"
		    "  %.*s\n",
		    49 + src_fw, wd_sep_200 );

	    for ( mix = mixtab; mix < end_mix; mix++ )
	    {
		const u32 end_sector = mix->dest_sector + mix->a2off + mix->a2size;
		printf("%9u : %9llx..%9llx : %u %s : %c %s\n",
		    mix->n_blocks,
		    mix->dest_sector * (u64)WII_SECTOR_SIZE,
		    end_sector * (u64)WII_SECTOR_SIZE,
		    mix->ptab,
		    wd_print_part_name(0,0,mix->ptype,WD_PNAME_COLUMN_9),
		    mix->have_pat ? '+' : '-', mix->source );
	    }
	}
	putchar('\n');
    }

    if (err) // verify status
	return err;

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

    if ( oft == OFT_WBFS )
	return ERROR0(ERR_CANT_CREATE,
		"Output to WBFS files not supported yet.");

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
	u64 off  = mix->dest_sector * (u64)WII_SECTOR_SIZE;
	u64 size = mix->a1size  * (u64)WII_SECTOR_SIZE;
	MemMapItem_t * item = InsertMemMap(&mm,off,size);
	DASSERT(item);
	item->index = mix - mixtab;
	snprintf(item->info,sizeof(item->info),
		    "partition #%-2u %s", (int)(mix-mixtab),
		    wd_print_part_name(0,0,mix->ptype,WD_PNAME_COLUMN_9) );

	if ( mix->a2off )
	{
	    off += mix->a2off * (u64)WII_SECTOR_SIZE;
	    size = mix->a2size  * (u64)WII_SECTOR_SIZE;
	    item = InsertMemMap(&mm,off,size);
	    DASSERT(item);
	    item->index = mix - mixtab | 0x80;
	    snprintf(item->info,sizeof(item->info),
			"partition #%-2u %s+", (int)(mix-mixtab),
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
		    entry->off4  = htonl(mix->dest_sector*WII_SECTOR_SIZE4);
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
    {
	free(mix->used_free);
	if (mix->free_data)
	    FreeSF(mix->sf);
    }

    return ResetSF(&fo,0);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

