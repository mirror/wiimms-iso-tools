// Modified by Wiimm, 2009-2010
// Copyright 2009 Kwiirk based on negentig.c:
// Copyright 2007,2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "wiidisc.h"
#include "rijndael.h"
#include "crypt.h"

///////////////////////////////////////////////////////////////////////////////

const char * wd_partition_name[]
	= { "DATA", "UPDATE", "CHANNEL", 0 };

//-----------------------------------------------------------------------------

const char * wd_get_partition_name ( u32 ptype, const char * result_if_not_found )
{
    return ptype < sizeof(wd_partition_name)/sizeof(*wd_partition_name)-1
	? wd_partition_name[ptype]
	: result_if_not_found;
}

//-----------------------------------------------------------------------------

char * wd_print_partition_name
	( char * buf, u32 buf_size, u32 ptype, int print_num )
{
    if (print_num)
    {
	if ( ptype < sizeof(wd_partition_name)/sizeof(*wd_partition_name)-1 )
	    snprintf(buf,buf_size,"%7s=%d",wd_partition_name[ptype],ptype);
	else
	    snprintf(buf,buf_size,"%9x",ptype);
    }
    else if ( ptype < sizeof(wd_partition_name)/sizeof(*wd_partition_name)-1 )
	snprintf(buf,buf_size,"%s",wd_partition_name[ptype]);
    else
	snprintf(buf,buf_size,"P%u",ptype);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

// The real common key must be set by the applikation.
// Only the first WII_KEY_SIZE bytes of 'common_key' are used.

static u8 common_key[] = "common key missed";

//-----------------------------------------------------------------------------

const u8 * wd_set_common_key ( const u8 * new_key )
{
    ASSERT( sizeof(common_key) >= WII_KEY_SIZE );
    if (new_key)
    {
	memcpy(common_key,new_key,sizeof(common_key));
	TRACE("new common key:\n");
	TRACE_HEXDUMP16(8,0,common_key,WII_KEY_SIZE);
    }
    return common_key;
}

//-----------------------------------------------------------------------------

const u8 * wd_get_common_key()
{
    ASSERT( sizeof(common_key) >= WII_KEY_SIZE );
    return common_key;
}

///////////////////////////////////////////////////////////////////////////////

void wd_decrypt_title_key ( const wd_ticket_t * tik, u8 * title_key )
{
    u8 iv[WII_KEY_SIZE];

    wbfs_memset( iv, 0, sizeof(iv) );
    wbfs_memcpy( iv, tik->title_id, 8 );
    aes_key_t akey;
    wd_aes_set_key( &akey, common_key );
    wd_aes_decrypt( &akey, iv, &tik->title_key, title_key, WII_KEY_SIZE );

    TRACE("| IV:  "); TRACE_HEXDUMP(0,0,1,WII_KEY_SIZE,iv,WII_KEY_SIZE);
    TRACE("| IN:  "); TRACE_HEXDUMP(0,0,1,WII_KEY_SIZE,&tik->title_key,WII_KEY_SIZE);
    TRACE("| OUT: "); TRACE_HEXDUMP(0,0,1,WII_KEY_SIZE,title_key,WII_KEY_SIZE);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			read functions			///////////////
///////////////////////////////////////////////////////////////////////////////

int wd_read_raw ( wiidisc_t * d, u32 p_offset4, void * p_data, u32 p_len )
{
    DASSERT(d);
    TRACE("#WD# wd_read_raw() %9llx %8x dest=%p\n",(u64)p_offset4<<2,p_len,p_data);

    if (!p_len)
	return 0;

    if (p_data)
    {
	noTRACE("WD-READ: ++   %9llx %5x ++\n",(u64)p_offset4<<2, p_len );

	u8  *data   = p_data;
	u32 len	    = p_len;
	u32 offset4 = p_offset4;

	// force reading only whole HD SECTORS
	char hd_buf[HD_SECTOR_SIZE];

	const int HD_SECTOR_SIZE4 = HD_SECTOR_SIZE >> 2;
	u32 skip4   = offset4 - offset4/HD_SECTOR_SIZE4 * HD_SECTOR_SIZE4;

	if (skip4)
	{
	    const u32 off1 = offset4 - skip4;
	    const int ret = d->read(d->fp,off1,HD_SECTOR_SIZE,hd_buf);
	    if (ret)
		wbfs_fatal("error reading disc (wd_read_raw)");

	    const u32 skip = skip4 << 2;
	    DASSERT( skip < HD_SECTOR_SIZE );
	    u32 copy_len = HD_SECTOR_SIZE - skip;
	    if ( copy_len > len )
		 copy_len = len;

	    noTRACE("WD-READ/PRE:  %9llx %5x -> %5zx %5x, skip=%x,%x\n",
			(u64)off1<<2, HD_SECTOR_SIZE, data-(u8*)p_data, copy_len, skip,skip4 );
	    
	    memcpy(data,hd_buf+skip,copy_len);
	    data    += copy_len;
	    len	    -= copy_len;
	    offset4 += copy_len >> 2;
	}

	const u32 copy_len = len / HD_SECTOR_SIZE * HD_SECTOR_SIZE;
	if ( copy_len > 0 )
	{
	    noTRACE("WD-READ/MAIN: %9llx %5x -> %5zx %5x\n",
			(u64)offset4<<2, copy_len, data-(u8*)p_data, copy_len );
	    const int ret = d->read(d->fp,offset4,copy_len,data);
	    if (ret)
		wbfs_fatal("error reading disc (wd_read_raw)");

	    data    += copy_len;
	    len	    -= copy_len;
	    offset4 += copy_len >> 2;
	}

	if ( len > 0 )
	{
	    ASSERT( len < HD_SECTOR_SIZE );

	    noTRACE("WD-READ/POST: %9llx %5x -> %5zx %5x\n",
			(u64)offset4<<2, HD_SECTOR_SIZE, data-(u8*)p_data, len );
	    const int ret = d->read(d->fp,offset4,HD_SECTOR_SIZE,hd_buf);
	    if (ret)
		wbfs_fatal("error reading disc (wd_read_raw)");

	    memcpy(data,hd_buf,len);
	}
    }

    if ( d->usage_table )
    {
	u32 first_block	= p_offset4 >> WII_SECTOR_SIZE_SHIFT-2;
	u32 end_block	= p_offset4 + ( p_len + WII_SECTOR_SIZE - 1 >> 2 )
			>> WII_SECTOR_SIZE_SHIFT-2;
	if ( end_block > WII_MAX_SECTORS )
	     end_block = WII_MAX_SECTORS;
	noTRACE("mark %x+%x => %x..%x [%x]\n",
		p_offset4, p_len, first_block, end_block, end_block-first_block );

	if ( first_block < end_block )
	    memset( d->usage_table + first_block,
			d->usage_marker, end_block - first_block );
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

#define NO_BLOCK (~(u32)0)

int wd_read_block
	( wiidisc_t *d, const aes_key_t * akey, u32 blockno, u8 * block )
{
    DASSERT(d);
    noTRACE("#WD# #%08x          wd_read_block()\n",blockno);

    if (!d->block_buffer)
    {
	d->block_buffer = wbfs_ioalloc(WII_SECTOR_SIZE);
	d->last_block = NO_BLOCK;
    }

    if ( d->last_block != blockno )
    {
	u8 *buf = blockno ? d->tmp_buffer : d->block_buffer;
	const u32 offset4 =  blockno * ( WII_SECTOR_SIZE >> 2 );
	const int stat = wd_read_raw( d, offset4, buf, WII_SECTOR_SIZE );
	if (stat)
	{
	    d->last_block = NO_BLOCK;
	    return stat;
	}
	d->last_block = blockno;

	if (blockno)
	{
	    if (akey)
	    {
		// decrypt data
		wd_aes_decrypt( akey, buf + WII_SECTOR_IV_OFF,
				buf + WII_SECTOR_DATA_OFF,
				d->block_buffer, WII_SECTOR_DATA_SIZE );
	    }
	    else
		memcpy( d->block_buffer,
			buf+WII_SECTOR_DATA_OFF, WII_SECTOR_DATA_SIZE );
	}
    }

    memcpy( block, d->block_buffer, WII_SECTOR_DATA_SIZE );
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int wd_read ( wiidisc_t *d, const aes_key_t * akey,
		u32 offset4, void * p_destbuf, u32 len, int do_not_read )
{
    DASSERT(d);
    if ( do_not_read && !d->usage_table )
	return 0;

    noTRACE("#WD# %8x %8x wd_read()\n",offset4,len);
    
    u8 * blockbuf = d->tmp_buffer2;
    u8 * destbuf = p_destbuf;
    while (len)
    {
	u32 offset_in_block = offset4 % WII_SECTOR_DATA_SIZE4;
	u32 len_in_block = 0x7c00 - (offset_in_block<<2);
	if (len_in_block > len)
	    len_in_block = len;

	if (do_not_read)
	    d->usage_table[ offset4 / WII_SECTOR_DATA_SIZE4 ]
				= d->usage_marker;
	else
	{
	    const int stat
		= wd_read_block( d, akey,
				 offset4 / WII_SECTOR_DATA_SIZE4, blockbuf );
	    if (stat)
		return stat;
	    wbfs_memcpy( destbuf, blockbuf + (offset_in_block<<2), len_in_block );
	}
	destbuf	+= len_in_block;
	offset4	+= len_in_block >> 2;
	len	-= len_in_block;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int wd_is_block_encrypted
	( wiidisc_t *d, const aes_key_t * akey, u32 block, int unknown_result )
{
    DASSERT(d);
    DASSERT(akey);
    TRACE("wd_is_block_encrypted(%p,%p,block=%x=%u,unknown=%d) off=%llx\n",
		d, akey, block, block, unknown_result, (u64)block * WII_SECTOR_SIZE );

    u8 * rdbuf = d->tmp_buffer;
    DASSERT(rdbuf);
    if (wd_read_raw(d, block*WII_SECTOR_SIZE4,rdbuf,WII_SECTOR_SIZE))
    {
	TRACE(" - read error, return -1\n");
	return -1;
    }
	
    u8 hash[WII_HASH_SIZE];
    SHA1(rdbuf+WII_SECTOR_DATA_OFF,WII_H0_DATA_SIZE,hash);
    TRACE(" - HASH: "); TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,hash,WII_HASH_SIZE);
    TRACE(" - H0:   "); TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,rdbuf,WII_HASH_SIZE);
    if (!memcmp(rdbuf,hash,sizeof(hash)))
    {
	TRACE(" - return 0 [not encrypted]\n"); 
	return 0;
    }

    u8 * dcbuf = d->tmp_buffer2;
    DASSERT(dcbuf);
    u8 iv[WII_KEY_SIZE];
    memset(iv,0,sizeof(iv));
    wd_aes_decrypt (	akey,
			iv,
			rdbuf,
			dcbuf,
			WII_SECTOR_DATA_OFF );
    wd_aes_decrypt (	akey,
			rdbuf + WII_SECTOR_IV_OFF,
			rdbuf + WII_SECTOR_DATA_OFF,
			dcbuf + WII_SECTOR_DATA_OFF,
			WII_SECTOR_DATA_SIZE );

    SHA1(dcbuf+WII_SECTOR_DATA_OFF,WII_H0_DATA_SIZE,hash);
    TRACE(" - HASH: "); TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,hash,WII_HASH_SIZE);
    TRACE(" - H0:   "); TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,dcbuf,WII_HASH_SIZE);
    if (!memcmp(dcbuf,hash,sizeof(hash)))
    {
	TRACE(" - return 1 [encrypted]\n"); 
	return 1;
    }

    TRACE(" - return %d [unknown]\n",unknown_result); 
    return unknown_result;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    read partition functions		///////////////
///////////////////////////////////////////////////////////////////////////////

static void partition_raw_read ( wiidisc_t *d, u32 offset4, void * data, u32 len )
{
    wd_read_raw( d, d->partition_raw_offset4 + offset4, data, len );
}

///////////////////////////////////////////////////////////////////////////////

static void partition_read_block ( wiidisc_t *d, u32 blockno, u8 *block )
{
    wd_read_block( d, d->is_encrypted ? &d->partition_akey : 0,
			d->partition_block + blockno, block );
}

///////////////////////////////////////////////////////////////////////////////

static void partition_read
	( wiidisc_t *d, u32 offset4, void * data, u32 len, int do_not_read )
{
    noTRACE("#WD# %8x %8x partition_read()\n",offset4,len);
    wd_read( d, d->is_encrypted ? &d->partition_akey : 0,
		d->partition_offset4 + offset4, data, len, do_not_read );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 iterator helpers		///////////////
///////////////////////////////////////////////////////////////////////////////

static void mark_usage_table ( wiidisc_t * d, u32 offset4, u32 size )
{
    DASSERT(d->usage_table);

    offset4 += d->partition_offset4;
    const u32 first_block = offset4 / WII_SECTOR_DATA_SIZE4;
    u32 end_block = (( size + WII_SECTOR_DATA_SIZE - 1 >> 2 )
		       + offset4 ) / WII_SECTOR_DATA_SIZE4;
    if ( end_block > WII_MAX_SECTORS )
	 end_block = WII_MAX_SECTORS;

    noTRACE("mark %x+%x => %x..%x [%x]\n",
		offset4, size, first_block, end_block, end_block-first_block );

    if ( first_block < end_block )
	memset( d->usage_table + first_block,
		d->usage_marker, end_block - first_block );
}

///////////////////////////////////////////////////////////////////////////////

static void mark_usage_table_raw ( wiidisc_t * d, u32 offset4, u32 size4 )
{
    DASSERT(d->usage_table);

    offset4 += d->partition_raw_offset4;
    const u32 first_block = offset4 / WII_SECTOR_SIZE4;
    u32 end_block = ( size4 + WII_SECTOR_SIZE4 - 1 + offset4 ) / WII_SECTOR_SIZE4;
    if ( end_block > WII_MAX_SECTORS )
	 end_block = WII_MAX_SECTORS;

    noTRACE("mark %x+%x => %x..%x [%x]\n",
		offset4, size, first_block, end_block, end_block-first_block );

    if ( first_block < end_block )
	memset( d->usage_table + first_block,
		d->usage_marker, end_block - first_block );
}

///////////////////////////////////////////////////////////////////////////////

static u32 append_path ( wiidisc_t *d, const char * name, int append_slash )
{
    const u32 prev_len = d->path_len;
    char *dest = d->path + prev_len;
    char *end  = d->path + sizeof(d->path) - 1;
    ASSERT( dest <= end );

    while ( *name && dest < end )
	*dest++ = *name++;

    if ( append_slash && dest < end )
	*dest++ = '/';

    *dest = 0;
    d->path_len = dest - d->path;
    if ( d->max_path_len < d->path_len )
	 d->max_path_len = d->path_len;

    return prev_len;
}

///////////////////////////////////////////////////////////////////////////////

static void restore_path ( wiidisc_t *d, u32 prev_len )
{
    ASSERT( prev_len >= 0 && prev_len < sizeof(d->path) );
    d->path[prev_len] = 0;
    d->path_len = prev_len;
}

///////////////////////////////////////////////////////////////////////////////

static void iterator_dir ( wiidisc_t *d, const char * name, u32 size )
{
    noTRACE("iterator_dir(%p,%s)\n",d,name);
    ASSERT(d);

    strncpy(d->path,name,sizeof(d->path));
    d->path[sizeof(d->path)-1] = 0;
    d->path_len = strlen(d->path);
    if (d->file_iterator)
	d->file_iterator(d,ICM_DIRECTORY,0,size,0);
}

///////////////////////////////////////////////////////////////////////////////

static void iterator_write
	( wiidisc_t *d, const char * name, u8 * data, u32 size )
{
    noTRACE("iterator_write(%p,%s,%p,%x)\n",d,name,data,size);
    ASSERT(d);

    const u32 prev_len = append_path(d,name,0);
    d->file_iterator(d,ICM_DATA,0,size,data);
    restore_path(d,prev_len);
}

///////////////////////////////////////////////////////////////////////////////

static void iterator_copy
	( wiidisc_t *d, const char * name, u32 offset4, u32 size, const void * data )
{
    noTRACE("iterator_copy(%p,%s,%x,%x)\n",d,name,offset4,size);
    ASSERT(d);

    const u32 prev_len = append_path(d,name,0);
    d->file_iterator(d,ICM_COPY,offset4,size,data);
    restore_path(d,prev_len);
}

///////////////////////////////////////////////////////////////////////////////

static void iterator_extract
	( wiidisc_t *d, const char * name, u32 offset4, u32 size )
{
    noTRACE("iterator_extract(%p,%s,%x,%x)\n",d,name,offset4,size);
    ASSERT(d);

    if (d->usage_table)
	mark_usage_table(d,offset4,size);
    const u32 prev_len = append_path(d,name,0);
    d->file_iterator(d,ICM_FILE,d->partition_offset4+offset4,size,0);
    restore_path(d,prev_len);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 visit disc			///////////////
///////////////////////////////////////////////////////////////////////////////

static void do_fst ( wiidisc_t *d, const wd_fst_item_t * ftab_data )
{
    ASSERT(d);
    ASSERT(ftab_data);

    const u32 n_files = ntohl(ftab_data->size);
    const wd_fst_item_t *ftab_end = ftab_data + n_files;
    const wd_fst_item_t *dir_end = ftab_end, *ftab;

    d->file_count = 0;
    d->dir_count  = 1; // root directory

    if (d->usage_table)
    {
	// mark used blocks
	for ( ftab = ftab_data+1; ftab < ftab_end; ftab++ )
	    if (ftab->is_dir)
		d->dir_count++;
	    else
	    {
		d->file_count++;
		mark_usage_table(d,wbfs_ntohl(ftab->offset4),wbfs_ntohl(ftab->size));
	    }
    }

    if (!d->file_iterator)
	return;


    //----- setup stack

    // reset this values because counting again
    d->file_count = 0;
    d->dir_count  = 1; // root directory

    const int MAX_DEPTH = 25; // maximal supported directory depth
    typedef struct stack_t
    {
	const wd_fst_item_t * dir_end;
	char * path;
    } stack_t;
    stack_t stack_buf[MAX_DEPTH];
    stack_t *stack = stack_buf;
    stack_t *stack_max = stack_buf + MAX_DEPTH;

    //----- setup path
    
    char *path_end = d->path + sizeof(d->path) - MAX_DEPTH;
    char *path = d->path + d->path_len;
    
    //----- main loop

    for ( ftab = ftab_data+1; ftab < ftab_end; ftab++ )
    {
	while ( ftab >= dir_end && stack > stack_buf )
	{
	    // leave a directory
	    stack--;
	    dir_end = stack->dir_end;
	    path = stack->path;
	}

	const char * fname = (char*)ftab_end + (ntohl(ftab->name_off)&0xffffff);
	char * path_dest = path;
	while ( path_dest < path_end && *fname )
	    *path_dest++ = *fname++;
	    
	if (ftab->is_dir)
	{
	    *path_dest++ = '/';
	    *path_dest = 0;
	    d->dir_count++;

	    ASSERT(stack<stack_max);
	    if ( stack < stack_max )
	    {
		stack->dir_end = dir_end;
		stack->path = path;
		stack++;
		dir_end = ftab_data + ntohl(ftab->size);
		path = path_dest;
	    }
	    if (d->file_iterator(d,ICM_DIRECTORY,0,dir_end-ftab-1,0))
		return;
	}
	else
	{
	    *path_dest = 0;
	    d->file_count++;
	    if (d->file_iterator(d,ICM_FILE,
			d->partition_offset4 + wbfs_ntohl(ftab->offset4),
			wbfs_ntohl(ftab->size), 0 )
	    ) return;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

static void do_files ( wiidisc_t * d )
{
    TRACE("do_files()\n");

    ASSERT( sizeof(wd_boot_t) == WII_BOOT_SIZE );
    wd_boot_t * boot = wbfs_ioalloc(sizeof(*boot));
    if (!boot)
	wbfs_fatal("malloc wd_boot_t");
    partition_read(d,0,boot,sizeof(*boot),0);
    ntoh_boot(boot,boot);
    TRACE("fst=%x,%x dol=%x\n",
	boot->fst_off4<<2,boot->fst_size4<<2,boot->dol_off4<<2);

    const u32 fst_size = boot->fst_size4 << 2;
    if (!fst_size)
	return;

    u32 dol_size = 0;
    {
	dol_header_t * dol = (dol_header_t *) d->tmp_buffer;
	partition_read( d, boot->dol_off4, dol, DOL_HEADER_SIZE, 0 );
	ntoh_dol_header(dol,dol);

	// iterate through the 7 code + 11 data segments
	int i;
	for ( i = 0; i < DOL_N_SECTIONS; i++ )
	{
	    const u32 size = dol->sect_off[i] + dol->sect_size[i];
	    if ( dol_size < size )
		 dol_size = size;
	}
	TRACE("DOL-SIZE: %x <= %x\n",dol_size,boot->fst_off4-boot->dol_off4<<2);
	ASSERT( dol_size <= boot->fst_off4-boot->dol_off4<<2 );
    }

    u8 * apl_header		= wbfs_ioalloc(0x20);
    const u32 apl_offset4	= WII_APL_OFF >> 2;
    partition_read( d, apl_offset4, apl_header, 0x20,0);
    const u32 apl_size		= 0x20 + be32(apl_header+0x14) + be32(apl_header+0x18);
    wbfs_iofree(apl_header);

    // fake read dol and partition
    partition_read(d,apl_offset4,0,apl_size,1);
    partition_read(d,boot->dol_off4,0,dol_size,1);

    if (d->file_iterator)
    {
	const u32 boot_offset4	= WII_BOOT_OFF >> 2;
	const u32 bi2_offset4	= WII_BI2_OFF >> 2;

	iterator_dir    (d, "sys/", 5);
	iterator_extract(d, "boot.bin",		boot_offset4,	WII_BOOT_SIZE );
	iterator_extract(d, "bi2.bin",		bi2_offset4,	WII_BI2_SIZE );
	iterator_extract(d, "apploader.img",	apl_offset4,	apl_size );
	iterator_extract(d, "main.dol",		boot->dol_off4,	dol_size );
	iterator_extract(d, "fst.bin",		boot->fst_off4,	fst_size );
    }

    // read and proecess fst
    wd_fst_item_t * fst = wbfs_ioalloc(fst_size);
    if (!fst)
	wbfs_fatal("malloc fst");
    partition_read(d, boot->fst_off4, fst, fst_size, 0 );
    const u32 n_files = ntohl(fst->size);
    TRACE("FST: n_files=%u\n",n_files);
    TRACE_HEXDUMP16(3,0,fst,0x30);

    if (n_files)
    {
	if (d->file_iterator)
	    iterator_dir(d,"files/",n_files-1);
	do_fst(d,fst);
    }

    wbfs_iofree(boot);
    wbfs_iofree(fst);
}

///////////////////////////////////////////////////////////////////////////////

static void do_partition ( wiidisc_t * d )
{
    TRACE("do_partition()\n");

    //----- alloc partition header

    if (!d->ph)
    {
	d->ph = wbfs_ioalloc(sizeof(*d->ph));
	if (!d->ph)
	    wbfs_fatal("malloc wd_part_header_t");
    }

    //----- read partition header

    wd_part_header_t * ph = d->ph;
    partition_raw_read(d,0,ph,sizeof(*ph));
    ntoh_part_header(ph,ph);

    wd_decrypt_title_key( &ph->ticket, d->partition_key );
    wd_aes_set_key(&d->partition_akey,d->partition_key);

    //----- get offset and size values

    d->partition_data_offset4	= ph->data_off4;
    d->partition_data_size4	= ph->data_size4;
    d->partition_block		= d->partition_raw_offset4 +  ph->data_off4 >> 13;
    d->partition_offset4	= d->partition_block * WII_SECTOR_DATA_SIZE4;

    //----- fake load cert + h3

    partition_raw_read( d, ph->cert_off4, 0, ph->cert_size );
    partition_raw_read( d, ph->h3_off4,   0, WII_H3_SIZE );

    //----- load tmd
    
    ASSERT( ph->tmd_size >= sizeof(wd_tmd_t) );
    wbfs_iofree(d->tmd);
    wd_tmd_t * tmd = wbfs_ioalloc(ph->tmd_size);
    if (!tmd)
	wbfs_fatal("malloc wd_tmd_t");
    d->tmd = tmd;
    partition_raw_read( d, ph->tmd_off4,tmd, ph->tmd_size );

    //----- check encryption and trucha signing

    d->tik_is_trucha_signed	= ticket_is_trucha_signed(&ph->ticket,0);
    d->tmd_is_trucha_signed	= tmd_is_trucha_signed(tmd,ph->tmd_size);
    d->is_marked_not_enc	= tmd_is_marked_not_encrypted(tmd);
    d->is_encrypted = !d->is_marked_not_enc
	&& wd_is_block_encrypted(d,&d->partition_akey,d->partition_block,1);
    TRACE("TRUCHA=%d,%d, MARK-NOT-ENC=%d, ENC=%d\n",
	d->tik_is_trucha_signed, d->tmd_is_trucha_signed,
	d->is_marked_not_enc, d->is_encrypted );

    //----- mark whole partition ?

    d->usage_marker = 2 + d->partition_index;
    if ( d->part_sel == WHOLE_DISC && d->usage_table )
	mark_usage_table_raw( d, ph->data_off4, ph->data_size4 );

    //----- open partition ?

    if ( d->open_partition >= 0 )
    {
	d->usage_marker = 1;
	return;
    }

    //----- if file_iterator is set -> call it for each sys file

    if (d->file_iterator)
    {
	char part_name[20];
	wd_print_partition_name(part_name,sizeof(part_name),d->partition_type,0);
  	
	// setup prefix mode
	iterator_prefix_mode_t pmode = d->prefix_mode;
	if ( pmode <= IPM_AUTO )
	     pmode = d->part_sel < 0 ? IPM_PART_NAME : IPM_POINT;

	switch(pmode)
	{
	    case IPM_NONE:
		*d->path_prefix = 0;
		break;

	    case IPM_POINT:
		strcpy(d->path_prefix,"./");
		break;

	    case IPM_PART_IDENT:
		snprintf(d->path_prefix,sizeof(d->path_prefix),"P%u/",d->partition_type);
		break;

	    //case IPM_PART_NAME:
	    default:
		snprintf(d->path_prefix,sizeof(d->path_prefix),"%s/", part_name );
		break;
	}

	//----- signal: new partition

	const u32 pro = d->partition_raw_offset4;
	d->file_iterator(d,ICM_OPEN_PARTITION,pro,0,ph);

	iterator_dir (d, "", 0);
	iterator_copy(d, "ticket.bin",	pro,		   sizeof(ph->ticket),	&ph->ticket );
	iterator_copy(d, "tmd.bin",	pro+ph->tmd_off4,  ph->tmd_size,	tmd );
	iterator_copy(d, "cert.bin",	pro+ph->cert_off4, ph->cert_size,	0 );
	iterator_copy(d, "h3.bin",	pro+ph->h3_off4,   WII_H3_SIZE,		0 );

	iterator_dir (d, "disc/", 0);
	iterator_copy(d, "header.bin",	0,		   sizeof(wd_header_t), 0 );
	iterator_copy(d, "region.bin",	WII_REGION_OFF>>2, sizeof(wd_region_set_t), 0 );
    }

    //----- iterate files

    do_files(d);

    //----- term

    if (d->file_iterator)
	d->file_iterator(d,ICM_CLOSE_PARTITION,0,0,0);
    d->usage_marker = 1;
}

///////////////////////////////////////////////////////////////////////////////

static int test_partition_skip ( u32 partition_type, partition_selector_t part_sel )
{
    switch (part_sel)
    {
	case WHOLE_DISC:		return 0;
	case ALL_PARTITIONS:		return 0;
	case REMOVE_DATA_PARTITION:	return partition_type == DATA_PARTITION_TYPE;
	case REMOVE_UPDATE_PARTITION:	return partition_type == UPDATE_PARTITION_TYPE;
	case REMOVE_CHANNEL_PARTITION:	return partition_type == CHANNEL_PARTITION_TYPE;
	default:			return partition_type != part_sel;
    }
}

///////////////////////////////////////////////////////////////////////////////

static void do_disc ( wiidisc_t * d )
{
    TRACE("do_disc()\n");
    ASSERT(d);

    const int BUFSIZE = HD_SECTOR_SIZE;
    const int MAX_PARTITIONS = BUFSIZE / 2 / sizeof(u32);

    u8 *b = wbfs_ioalloc(BUFSIZE);

    d->partition_index = -1;
    d->usage_marker = 1;
    if ( d->usage_table )
    {
	// these sectors are always marked
	d->usage_table[ 0 ] = 1;
	d->usage_table[ WII_PART_INFO_OFF / WII_SECTOR_SIZE ] = 1;
	d->usage_table[ WII_REGION_OFF    / WII_SECTOR_SIZE ] = 1;
	d->usage_table[ WII_MAGIC2_OFF    / WII_SECTOR_SIZE ] = 1;
    }

    // read disc header and check magic
    wd_read_raw(d,0,b,BUFSIZE);
    memcpy(d->id6,b,6);
    const u32 magic = be32(b+24);
    if ( magic != WII_MAGIC )
    {
	wbfs_error("not a wii disc, magic=%08x",magic);
	return ;
    }

    // read partition table
    wd_read_raw( d, WII_PART_INFO_OFF >> 2, b, BUFSIZE );

    u32 partition_offset[MAX_PARTITIONS];
    u32 partition_type[MAX_PARTITIONS];
    u32 n_partitions = be32(b);
    if ( n_partitions > MAX_PARTITIONS )
	wbfs_fatal("%u partitions found but can only handle %u partitions.\n",
		n_partitions, MAX_PARTITIONS );

    // get partition info
    wd_read_raw( d, be32(b + 4), b, BUFSIZE );
    u32 i;
    for ( i = 0; i < n_partitions; i++ )
    {
	partition_offset[i] = be32( b + 8 * i   );
	partition_type[i]   = be32( b + 8 * i+4 );
    }

    if (d->file_iterator)
	d->file_iterator(d,ICM_OPEN_DISC,0,n_partitions,0);

    // iterate all partitions
    for ( i = 0; i < n_partitions; i++ )
    {
	if ( d->open_partition < 0
		? !test_partition_skip(partition_type[i],d->part_sel)
		: d->part_sel < 0
			? d->open_partition == i
			: partition_type[i] == d->part_sel
	)
	{
	    d->partition_index	    = i;
	    d->partition_type	    = partition_type[i];
	    d->partition_raw_offset4 = partition_offset[i];
	    do_partition(d);
	}
    }

    if (d->file_iterator)
	d->file_iterator(d,ICM_CLOSE_DISC,0,0,0);

    // free memory
    wbfs_iofree(b);
}

///////////////////////////////////////////////////////////////////////////////
// wd_open_disc() : alloc+setup data structure, save parameters

wiidisc_t * wd_open_disc
(
	read_wiidisc_callback_t read,
	void * fp
)
{
    TRACE("wd_open_disc()\n");
    return wd_open_partition(read,fp,-1,ALL_PARTITIONS);
}

///////////////////////////////////////////////////////////////////////////////
// wd_open_disc() : alloc+setup data structure, save parameters

wiidisc_t * wd_open_partition
(
	read_wiidisc_callback_t read,
	void * fp,
	int open_part_index,
	partition_selector_t part_sel
)
{
    TRACE("wd_open_partition(index=%d,sel=%x)\n",open_part_index,part_sel);
    if (validate_file_format_sizes(0))
	wbfs_fatal("check file-format");

    wiidisc_t *d = wbfs_malloc(sizeof(wiidisc_t));
    if (!d)
	OUT_OF_MEMORY;
    wbfs_memset(d,0,sizeof(wiidisc_t));

    d->read = read;
    d->fp = fp;
    d->partition_index = -1;
    d->tmp_buffer  = wbfs_ioalloc(WII_SECTOR_SIZE);
    d->tmp_buffer2 = wbfs_malloc(WII_SECTOR_SIZE);
    if ( !d->tmp_buffer || !d->tmp_buffer2 )
	OUT_OF_MEMORY;

    if ( open_part_index >= 0 )
    {
	d->open_partition = open_part_index;
	d->part_sel = ALL_PARTITIONS;
	do_disc(d);
	if ( d->partition_index != open_part_index )
	{
	    wd_close_disc(d);
	    return 0;
	}
    }
    else if ( part_sel >= 0 )
    {
	// open partition with given id
	d->open_partition = 0; // any value >= 0
	d->part_sel = part_sel;
	do_disc(d);
	if ( d->partition_type != part_sel )
	{
	    wd_close_disc(d);
	    return 0;
	}
    }

    d->open_partition = -1;
    d->part_sel = ALL_PARTITIONS;

    return d;
}

///////////////////////////////////////////////////////////////////////////////
// wd_close_disc() : free data structure

void wd_close_disc ( wiidisc_t * d )
{
    TRACE("wd_close_disc()\n");

    wbfs_iofree(d->ph);
    wbfs_iofree(d->tmd);
    wbfs_iofree(d->block_buffer);
    wbfs_iofree(d->tmp_buffer);
    wbfs_free(d->tmp_buffer2);
    wbfs_free(d);
}

///////////////////////////////////////////////////////////////////////////////

void wd_build_disc_usage
(
	wiidisc_t * d,
	partition_selector_t selector,
	u8 * usage_table,
	u64 iso_size
)
{
    TRACE("wd_build_disc_usage()\n");

    ASSERT(d);
    ASSERT(usage_table);

    wd_iterate_files(d,selector,IPM_AUTO,0,0,usage_table,iso_size);
}

///////////////////////////////////////////////////////////////////////////////

void wd_iterate_files
(
	wiidisc_t * d,
	partition_selector_t selector,
	iterator_prefix_mode_t prefix_mode,
	file_callback_t file_iterator,
	void * param,
	u8 * usage_table,
	u64 iso_size
)
{
    TRACE("wd_iterate_files()\n");

    ASSERT(d);

    // copy parmeters
    d->part_sel		= selector;
    d->prefix_mode	= prefix_mode;
    d->file_iterator	= file_iterator;
    d->user_param	= param;
    d->usage_table	= usage_table;
    d->usage_marker	= 1;

    // preset usage_table if defined
    if (usage_table)
    {
	wbfs_memset( usage_table, 0, WII_MAX_SECTORS );
	if ( selector == WHOLE_DISC )
	{
	    const u32 n_sectors = iso_size
				    ? (u32)( (iso_size-1) / WII_SECTOR_SIZE ) + 1
				    : WII_SECTORS_SINGLE_LAYER;
	    wbfs_memset( usage_table, 1,
			    n_sectors < WII_MAX_SECTORS ? n_sectors : WII_MAX_SECTORS );
	}
    }

    // execute
    do_disc(d);

    // reset parameters
    d->part_sel		= ALL_PARTITIONS;
    d->prefix_mode	= IPM_AUTO;
    d->file_iterator	= 0;
    d->user_param	= 0;
    d->usage_table	= 0;
    d->usage_marker	= 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    etc...			///////////////
///////////////////////////////////////////////////////////////////////////////

void wd_fix_partition_table
	( wiidisc_t * d , partition_selector_t selector, u8 * partition_table )
{ // [codeview]

    TRACE("LIBWBFS: wd_fix_partition_table(%p,sel=%x,%p)\n",
		d, selector, partition_table );

    u8 *b = partition_table;
    u32 partition_offset;
    u32 partition_type;
    u32 n_partitions,i,j;
    u32 *b32;
    if ( selector == ALL_PARTITIONS || selector == WHOLE_DISC )
	return;
    n_partitions = be32(b);
    if (be32(b + 4)-(0x40000>>2) >0x50)
	wbfs_fatal("cannot modify this partition table. Please report the bug.");

    b += (be32(b + 4)-(0x40000>>2))*4;
    j=0;
    for (i = 0; i < n_partitions; i++)
    {
	partition_offset = be32(b + 8 * i);
	partition_type = be32(b + 8 * i+4);
	if (!test_partition_skip(partition_type,selector))
	{
	    b32 = (u32*)(b + 8 * j);
	    b32[0] = wbfs_htonl(partition_offset);
	    b32[1] = wbfs_htonl(partition_type);
	    j++;
	}
    }
    b32 = (u32*)(partition_table);
    *b32 = wbfs_htonl(j);
}

///////////////////////////////////////////////////////////////////////////////
// rename ID and title of a ISO header

int wd_rename
(
	void * p_data,		// pointer to ISO data
	const char * new_id,	// if !NULL: take the first 6 chars as ID
	const char * new_title	// if !NULL: take the first 0x39 chars as title
)
{
    ASSERT(p_data);
    u8 *data = p_data;

    int done = 0;

    if ( new_id && strlen(new_id) >= 6 && memcmp(data,new_id,6) )
    {
	memcpy(data,new_id,6);
	done |= 1;
    }

    if ( new_title && *new_title )
    {
	char buf[WII_TITLE_SIZE];
	memset(buf,0,WII_TITLE_SIZE);
	const size_t slen = strlen(new_title);
	memcpy(buf, new_title, slen < WII_TITLE_SIZE ? slen : WII_TITLE_SIZE-1 );

	data += WII_TITLE_OFF;
	if ( memcmp(data,buf,WII_TITLE_SIZE) )
	{
	    memcpy(data,buf,WII_TITLE_SIZE);
	    done |= 2;
	}
    }

    return done;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			     E N D			///////////////
///////////////////////////////////////////////////////////////////////////////
