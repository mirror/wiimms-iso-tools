
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

#include "wiidisc.h"
#include <ctype.h>

#ifdef __CYGWIN__
    #define ISALNUM(a) isalnum((int)(a))
#else
    #define ISALNUM isalnum
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    common key			///////////////
///////////////////////////////////////////////////////////////////////////////

// The real common key must be set by the application.
// Only the first WII_KEY_SIZE bytes of 'common_key' are used.

static u8 common_key[] = "common key missed";

///////////////////////////////////////////////////////////////////////////////

const u8 * wd_get_common_key()
{
    ASSERT( sizeof(common_key) >= WII_KEY_SIZE );
    return common_key;
}

///////////////////////////////////////////////////////////////////////////////

const u8 * wd_set_common_key
(
	const u8 * new_key		// the new key
)
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

///////////////////////////////////////////////////////////////////////////////

void wd_decrypt_title_key
(
	const wd_ticket_t * tik,	// pointer to ticket
	u8 * title_key			// the result
)
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
///////////////			names, ids and titles		///////////////
///////////////////////////////////////////////////////////////////////////////

const char * wd_part_name[]
	= { "DATA", "UPDATE", "CHANNEL", 0 };

///////////////////////////////////////////////////////////////////////////////

const char * wd_get_part_name
(
	u32	ptype,			// partition type
	ccp	result_if_not_found	// result if no name found
)
{
    return ptype < sizeof(wd_part_name)/sizeof(*wd_part_name)-1
	? wd_part_name[ptype]
	: result_if_not_found;
}

///////////////////////////////////////////////////////////////////////////////

char * wd_print_part_name
(
	char		* buf,		// result buffer
	size_t		buf_size,	// size of 'buf'
	u32		ptype,		// partition type
	wd_pname_mode_t	mode		// print mode
)
{
    ccp id4 = (ccp)&ptype;
    const bool is_id = ISALNUM(id4[0]) && ISALNUM(id4[1])
		    && ISALNUM(id4[2]) && ISALNUM(id4[3]);
    const ccp name = ptype < sizeof(wd_part_name)/sizeof(*wd_part_name)-1
		   ?  wd_part_name[ptype] : 0;

    switch (mode)
    {
      case WD_PNAME_NAME_NUM_9:
	if (name)
	    snprintf(buf,buf_size,"%7s=%x",name,ptype);
	else if (is_id)
	{
	    ptype = wbfs_htonl(ptype); // we need big endian here
	    snprintf(buf,buf_size,"   \"%.4s\"",id4);
	}
	else
	    snprintf(buf,buf_size,"%9x",ptype);
	break;

      case WD_PNAME_COLUMN_9:
	if (name)
	    snprintf(buf,buf_size,"%7s %x",name,ptype);
	else if (is_id)
	{
	    ptype = wbfs_htonl(ptype); // we need big endian here
	    snprintf(buf,buf_size,"   \"%.4s\"",id4);
	}
	else
	    snprintf(buf,buf_size,"%9x",ptype);
	break;

      case WD_PNAME_NAME:
	if (name)
	{
	    snprintf(buf,buf_size,"%s",name);
	    break;
	}
	// fall through

       case WD_PNAME_NUM:
	if (is_id)
	{
	    ptype = wbfs_htonl(ptype); // we need big endian here
	    snprintf(buf,buf_size,"%.4s",id4);
	}
	else
	    snprintf(buf,buf_size,"%x",ptype);
	break;

      case WD_PNAME_P_NAME:
	if (name)
	    snprintf(buf,buf_size,"%s",name);
	else if (is_id)
	{
	    ptype = wbfs_htonl(ptype); // we need big endian here
	    snprintf(buf,buf_size,"P-%.4s",id4);
	}
	else
	    snprintf(buf,buf_size,"P%x",ptype);
	break;

     //case WD_PNAME_NUM_INFO:
      default:
	if (name)
	    snprintf(buf,buf_size,"%x [%s]",ptype,name);
	else if (is_id)
	{
	    ptype = wbfs_htonl(ptype); // we need big endian here
	    snprintf(buf,buf_size,"%x [\"%.4s\"]",ptype,id4);
	}
	else
	    snprintf(buf,buf_size,"%x [?]",ptype);
	break;
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////
// returns a pointer to a printable ID, teminated with 0

char * wd_print_id
(
	const void	* id,		// ID to convert in printable format
	size_t		id_len,		// len of 'id'
	void		* buf		// Pointer to buffer, size >= id_len + 1
					// If NULL, a local circulary static buffer is used
)
{
    enum
    {
	SBUF_COUNT = 5,
	SBUF_SIZE  = 10
    };

    static int  sbuf_index = 0;
    static char sbuf[SBUF_COUNT][SBUF_SIZE+1];
    
    if (!buf)
    {
	// use static buffer
	buf = sbuf[sbuf_index];
	sbuf_index = ( sbuf_index + 1 ) % SBUF_COUNT;
	if ( id_len > SBUF_SIZE )
	     id_len = SBUF_SIZE;
    }

    ccp src = id;
    char * dest = buf;
    while ( id_len-- > 0 )
    {
	const char ch = *src++;
	*dest++ = ch >= ' ' && ch < 0x7f ? ch : '.';
    }
    *dest = 0;

    return buf;
}

///////////////////////////////////////////////////////////////////////////////
// rename ID and title of a ISO header

int wd_rename
(
	void		* p_data,	// pointer to ISO data
	ccp		new_id,		// if !NULL: take the first 6 chars as ID
	ccp		new_title	// if !NULL: take the first 0x39 chars as title
)
{
    DASSERT(p_data);

    int done = 0;

    if ( new_id )
    {
	ccp end_id;
	char * dest = p_data;
	for ( end_id = new_id + 6; new_id < end_id && *new_id; new_id++, dest++ )
	    if ( *new_id != '.' && *new_id != *dest )
	    {
		*dest = *new_id;
		done |= 1;
	    }
    }

    if ( new_title && *new_title )
    {
	char buf[WII_TITLE_SIZE];
	memset(buf,0,WII_TITLE_SIZE);
	const size_t slen = strlen(new_title);
	memcpy(buf, new_title, slen < WII_TITLE_SIZE ? slen : WII_TITLE_SIZE-1 );

	u8 *dest = p_data;
	dest += WII_TITLE_OFF;
	if ( memcmp(dest,buf,WII_TITLE_SIZE) )
	{
	    memcpy(dest,buf,WII_TITLE_SIZE);
	    done |= 2;
	}
    }

    return done;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			read functions			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError wd_read_raw
(
	wd_disc_t	* disc,		// valid disc pointer
	u32		disc_offset4,	// disc offset/4
	void		* dest_buf,	// destination buffer
	u32		read_size,	// number of bytes to read 
	wd_usage_t	usage_id	// not 0: mark usage usage_tab with this value
)
{
    DASSERT(disc);
    DASSERT(disc->read_func);
    TRACE("#WD# wd_read_raw() %9llx %8x dest=%p\n",
			(u64)disc_offset4<<2, read_size, dest_buf );

    if (!read_size)
	return ERR_OK;

    if (dest_buf)
    {
	noTRACE("WD-READ: ++   %9llx %5x ++\n",(u64)disc_offset4<<2, read_size );

	u8  *dest   = dest_buf;
	u32 len	    = read_size;
	u32 offset4 = disc_offset4;

	// force reading only whole HD SECTORS
	char hd_buf[HD_SECTOR_SIZE];

	const int HD_SECTOR_SIZE4 = HD_SECTOR_SIZE >> 2;
	u32 skip4   = offset4 - offset4/HD_SECTOR_SIZE4 * HD_SECTOR_SIZE4;

	if (skip4)
	{
	    const u32 off1 = offset4 - skip4;
	    const int ret = disc->read_func(disc->read_data,off1,HD_SECTOR_SIZE,hd_buf);
	    if (ret)
	    {
		wbfs_error("error reading disc (wd_read_raw)");
		return ERR_READ_FAILED;
	    }

	    const u32 skip = skip4 << 2;
	    DASSERT( skip < HD_SECTOR_SIZE );
	    u32 copy_len = HD_SECTOR_SIZE - skip;
	    if ( copy_len > len )
		 copy_len = len;

	    noTRACE("WD-READ/PRE:  %9llx %5x -> %5zx %5x, skip=%x,%x\n",
			(u64)off1<<2, HD_SECTOR_SIZE, dest-(u8*)dest_buf, copy_len, skip,skip4 );
	    
	    memcpy(dest,hd_buf+skip,copy_len);
	    dest    += copy_len;
	    len	    -= copy_len;
	    offset4 += copy_len >> 2;
	}

	const u32 copy_len = len / HD_SECTOR_SIZE * HD_SECTOR_SIZE;
	if ( copy_len > 0 )
	{
	    noTRACE("WD-READ/MAIN: %9llx %5x -> %5zx %5x\n",
			(u64)offset4<<2, copy_len, dest-(u8*)dest_buf, copy_len );
	    const int ret = disc->read_func(disc->read_data,offset4,copy_len,dest);
	    if (ret)
	    {
		wbfs_error("error reading disc (wd_read_raw)");
		return ERR_READ_FAILED;
	    }

	    dest    += copy_len;
	    len	    -= copy_len;
	    offset4 += copy_len >> 2;
	}

	if ( len > 0 )
	{
	    ASSERT( len < HD_SECTOR_SIZE );

	    noTRACE("WD-READ/POST: %9llx %5x -> %5zx %5x\n",
			(u64)offset4<<2, HD_SECTOR_SIZE, dest-(u8*)dest_buf, len );
	    const int ret = disc->read_func(disc->read_data,offset4,HD_SECTOR_SIZE,hd_buf);
	    if (ret)
	    {
		wbfs_error("error reading disc (wd_read_raw)");
		return ERR_READ_FAILED;
	    }

	    memcpy(dest,hd_buf,len);
	}
    }

    if ( usage_id )
    {
	u32 first_block	= disc_offset4 >> WII_SECTOR_SIZE_SHIFT-2;
	u32 end_block	= disc_offset4 + ( read_size + WII_SECTOR_SIZE - 1 >> 2 )
			>> WII_SECTOR_SIZE_SHIFT-2;
	if ( end_block > WII_MAX_SECTORS )
	     end_block = WII_MAX_SECTORS;
	noTRACE("mark %x+%x => %x..%x [%x]\n",
		disc_offset4, read_size, first_block, end_block, end_block-first_block );

	if ( first_block < end_block )
	{
	    memset( disc->usage_table + first_block,
			usage_id, end_block - first_block );
	    if ( disc->usage_max < end_block )
		 disc->usage_max = end_block;
	}
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_read_part_raw
(
	wd_part_t	* part,		// valid pointer to a disc partition
	u32		offset4,	// offset/4 to partition start
	void		* dest_buf,	// destination buffer
	u32		read_size,	// number of bytes to read 
	bool		mark_block	// true: mark block in 'usage_table'
)
{
    DASSERT(part);
    return wd_read_raw( part->disc,
			part->part_off4 + offset4,
			dest_buf,
			read_size,
			mark_block ? part->usage_id : 0 );
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_read_part_block
(
	wd_part_t	* part,		// valid pointer to a disc partition
	u32		block_num,	// block number of partition
	u8		* block,	// destination buf
	bool		mark_block	// true: mark block in 'usage_table'
)
{
    TRACE("#WD# #%08x          wd_read_part_block()\n",block_num);
    DASSERT(part);
    DASSERT(part->disc);

    wd_disc_t * disc = part->disc;

    enumError err = ERR_OK;
    if ( disc->block_part != part || disc->block_num != block_num )
    {
	
	 err = wd_read_raw(	part->disc,
				part->data_off4 + block_num * WII_SECTOR_SIZE4,
				disc->temp_buf,
				WII_SECTOR_SIZE,
				part->usage_id | WD_USAGE_F_CRYPT );

	if (err)
	{
	    memset(disc->block_buf,0,sizeof(disc->block_buf));
	    disc->block_part = 0;
	}
	else
	{
	    disc->block_part = part;
	    disc->block_num  = block_num;
	    
	    if (part->is_encrypted)
	    {
		if ( disc->akey_part != part )
		{
		    disc->akey_part = part;
		    wd_aes_set_key(&disc->akey,part->key);
		}

		// decrypt data
		wd_aes_decrypt(	&disc->akey,
				disc->temp_buf + WII_SECTOR_IV_OFF,
				disc->temp_buf + WII_SECTOR_DATA_OFF,
				disc->block_buf,
				WII_SECTOR_DATA_SIZE );
	    }
	    else
		memcpy(	disc->block_buf,
			disc->temp_buf + WII_SECTOR_DATA_OFF,
			WII_SECTOR_DATA_SIZE );
	}
    }
    
    memcpy( block, disc->block_buf, WII_SECTOR_DATA_SIZE );
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_read_part
(
	wd_part_t	* part,		// valid pointer to a disc partition
	u32		data_offset4,	// partition data offset/4
	void		* dest_buf,	// estination buffer
	u32		read_size,	// number of bytes to read 
	bool		mark_block	// true: mark block in 'usage_table'
)
{
    TRACE("#WD# %8x %8x wd_read_part()\n",data_offset4,read_size);
    DASSERT(part);
    DASSERT(part->disc);
    DASSERT(dest_buf);

    wd_disc_t * disc = part->disc;

    u8 * temp = disc->temp_buf;
    u8 * dest = dest_buf;

    enumError err = ERR_OK;
    while ( read_size && err == ERR_OK )
    {
	u32 offset4_in_block = data_offset4 % WII_SECTOR_DATA_SIZE4;
	u32 len_in_block = WII_SECTOR_DATA_SIZE - ( offset4_in_block << 2 );
	if ( len_in_block > read_size )
	     len_in_block = read_size;

	err = wd_read_part_block( part,
				  data_offset4 / WII_SECTOR_DATA_SIZE4,
				  temp,
				  mark_block );
	memcpy( dest, temp + (offset4_in_block<<2), len_in_block );

	dest		+= len_in_block;
	data_offset4	+= len_in_block >> 2;
	read_size	-= len_in_block;
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////

void wd_mark_part
(
	wd_part_t	* part,		// valid pointer to a disc partition
	u32		data_offset4,	// partition data offset/4
	u32		data_size	// number of bytes to mark
)
{
    DASSERT(part);
    DASSERT(part->disc);

    const u32 block_delta = part->data_off4 /  WII_SECTOR_SIZE4;
    const u32 first_block = data_offset4 / WII_SECTOR_DATA_SIZE4 + block_delta;
    u32 end_block = (( data_size + WII_SECTOR_DATA_SIZE - 1 >> 2 )
		       + data_offset4 ) / WII_SECTOR_DATA_SIZE4
		       + block_delta;
    if ( end_block > WII_MAX_SECTORS )
	 end_block = WII_MAX_SECTORS;

    noTRACE("mark %x+%x => %x..%x [%x]\n",
		data_offset4, data_size, first_block, end_block, end_block-first_block );

    if ( first_block < end_block )
    {
	wd_disc_t * disc = part->disc;
	memset( disc->usage_table + first_block,
		part->usage_id | WD_USAGE_F_CRYPT,
		end_block - first_block );
	if ( disc->usage_max < end_block )
	     disc->usage_max = end_block;
    }
}

///////////////////////////////////////////////////////////////////////////////

int wd_is_block_encrypted
(
	wd_part_t	* part,		// valid pointer to a disc partition
	u32		block_num,	// block number of partition
	int		unknown_result	// result if status is unknown
)
{
    TRACE("wd_is_block_encrypted(%p,%u,%d)\n", part, block_num, unknown_result );
    DASSERT(part);
    DASSERT(part->disc);

    wd_disc_t * disc = part->disc;

    u8 * rdbuf = disc->temp_buf;
    enumError err = wd_read_raw(	part->disc,
					part->data_off4 + block_num * WII_SECTOR_SIZE4,
					rdbuf,
					WII_SECTOR_SIZE,
					0 );
    if (err)
    {
	TRACE(" - read error, return -1\n");
	return -1;
    }
	
    u8 hash[WII_HASH_SIZE];
    SHA1( rdbuf + WII_SECTOR_DATA_OFF, WII_H0_DATA_SIZE, hash );
    TRACE(" - HASH: "); TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,hash,WII_HASH_SIZE);
    TRACE(" - H0:   "); TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,rdbuf,WII_HASH_SIZE);
    if (!memcmp(rdbuf,hash,sizeof(hash)))
    {
	TRACE(" - return 0 [not encrypted]\n"); 
	return 0;
    }

    if ( disc->akey_part != part )
    {
	disc->akey_part = part;
	wd_aes_set_key(&disc->akey,part->key);
    }

    u8 dcbuf[WII_SECTOR_SIZE];
    u8 iv[WII_KEY_SIZE];
    memset(iv,0,sizeof(iv));
    wd_aes_decrypt (	&disc->akey,
			iv,
			rdbuf,
			dcbuf,
			WII_SECTOR_DATA_OFF );
    wd_aes_decrypt (	&disc->akey,
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
///////////////			open and close			///////////////
///////////////////////////////////////////////////////////////////////////////

wd_disc_t * wd_open_disc
(
	wd_read_func_t	read_func,	// read function, always valid
	void		* read_data,	// data pointer for read function
	u64		file_size,	// size of file, unknown if 0
	enumError	* error_code	// store error code if not NULL
)
{
    DASSERT(read_func);
    TRACE("wd_open_disc(%p,%p,%llx=%llu,%p)\n",
		read_func, read_data, file_size, file_size, error_code );

    //----- setup

    wd_disc_t * disc = wbfs_malloc(sizeof(*disc));
    if (!disc)
	OUT_OF_MEMORY;
    memset(disc,0,sizeof(*disc));

    disc->read_func = read_func;
    disc->read_data = read_data;
    disc->file_size = file_size;
    disc->open_count = 1;
    

    //----- read disc header

    enumError err = wd_read_raw(disc,0,&disc->dhead,sizeof(disc->dhead),WD_USAGE_DISC);
    if ( !err && wbfs_ntohl(disc->dhead.magic) != WII_MAGIC )
	err = ERR_WDISC_NOT_FOUND;
    memcpy(disc->id6,&disc->dhead,6);


    //----- read partition data

    if (!err)
	err = wd_read_raw( disc, WII_PTAB_REF_OFF>>2,
				&disc->ptab_info, sizeof(disc->ptab_info), WD_USAGE_DISC );
    
    if (!err)
    {
	int n_part = 0, ipt;
	for ( ipt = 0; ipt < WII_MAX_PTAB; ipt++ )
	    n_part += wbfs_ntohl(disc->ptab_info[ipt].n_part);
	if ( n_part > WII_MAX_PARTITIONS )
	     n_part = WII_MAX_PARTITIONS;
	disc->n_part = n_part;

	disc->ptab_entry = wbfs_calloc(n_part,sizeof(*disc->ptab_entry));
	disc->part = wbfs_calloc(n_part,sizeof(*disc->part));
	if ( !disc->ptab_entry || !disc->part )
	    OUT_OF_MEMORY;

	disc->n_ptab = 0;
	for ( n_part = ipt = 0; ipt < WII_MAX_PTAB && !err; ipt++ )
	{
	    wd_ptab_info_t * pt = disc->ptab_info + ipt;
	    int n = wbfs_ntohl(pt->n_part);
	    if ( n > disc->n_part - n_part ) // limit n
		 n = disc->n_part - n_part;

	    if (n)
	    {
		disc->n_ptab++;
		wd_ptab_entry_t * pe = disc->ptab_entry + n_part;
		err = wd_read_raw( disc, wbfs_ntohl(pt->off4),
					pe, n*sizeof(*pe), WD_USAGE_DISC );
		if (!err)
		{
		    int pi;
		    for ( pi = 0; pi < n; pi++, pe++ )
		    {
			wd_part_t * part	= disc->part + n_part + pi;
			part->index		= n_part + pi;
			part->usage_id		= part->index + WD_USAGE_PART_0;
			part->ptab_index	= ipt;
			part->ptab_part_index	= pi;
			part->part_type		= wbfs_ntohl(pe->ptype);
			part->part_off4		= wbfs_ntohl(pe->off4);
			part->is_enabled	= true;
			part->disc		= disc;
		    }
		}
		n_part += n;
	    }
	}
	DASSERT( err || n_part == disc->n_part );
    }


    //----- read region settings and magic2

    if (!err)
	err = wd_read_raw( disc, WII_REGION_OFF>>2, &disc->region,
				sizeof(disc->region), WD_USAGE_DISC );

    if (!err)
	err = wd_read_raw( disc, WII_MAGIC2_OFF>>2, &disc->magic2,
				sizeof(disc->magic2), WD_USAGE_DISC );

    //----- setup patching
    
    if (!err)
    {
	wd_patch_item_t * item
	    = wd_insert_patch(	&disc->patch,
				WD_PAT_DISC_DATA,
				WII_PTAB_REF_OFF,
				WII_MAX_PTAB_SIZE );
	DASSERT(item);
	snprintf(item->info,sizeof(item->info),"partiton tables");
	item->data = &disc->ptab;
	wd_patch_ptab(disc,item->data,true);
    }


    //----- terminate

 #ifdef DEBUG
    wd_dump_disc(TRACE_FILE,10,disc,0);
 #endif

    if (err)
    {
	wd_close_disc(disc);
	disc = 0;
    }

    if (error_code)
	*error_code = err;
    return disc;
}

///////////////////////////////////////////////////////////////////////////////

wd_disc_t * wd_dup_disc
(
	wd_disc_t * disc		// NULL or a valid disc pointer
)
{
    TRACE("wd_dup_disc(%p) open_count=%d\n", disc, disc ? disc->open_count : -1 );
    if (disc)
	disc->open_count++;
    return disc;
}

///////////////////////////////////////////////////////////////////////////////

void wd_close_disc
(
	wd_disc_t * disc		// NULL or valid disc pointer
)
{
    TRACE("wd_close_disc(%p) open_count=%d\n", disc, disc ? disc->open_count : -1 );

    if ( disc && --disc->open_count <= 0 )
    {
	wd_reset_patch(&disc->patch);
	if (disc->part)
	{
	    int pi;
	    wd_part_t * part = disc->part;
	    for ( pi = 0; pi < disc->n_part; pi++, part++ )
	    {
		wd_reset_patch(&part->patch);
		wbfs_free(part->tmd);
		wbfs_free(part->cert);
		wbfs_free(part->h3);
		wbfs_free(part->fst);
	    } 
	    wbfs_free(disc->part);
	} 
	wbfs_free(disc->ptab_entry);
	wbfs_free(disc);
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			get partition data		///////////////
///////////////////////////////////////////////////////////////////////////////

wd_part_t * wd_get_part_by_index
(
	wd_disc_t	*disc,		// valid disc pointer
	int		index,		// zero based partition index within wd_disc_t
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
)
{
    TRACE("wd_get_part_by_index(%p,%d,%d)\n",disc,index,load_data);
    ASSERT(disc);

    if ( index < 0 || index >= disc->n_part )
	return 0;

    wd_part_t * part = disc->part + index;
    part->disc = disc;

    if ( load_data > 0 )
	wd_load_part( part, load_data > 1, load_data > 2 );
    
    return part;
}

///////////////////////////////////////////////////////////////////////////////

wd_part_t * wd_get_part_by_usage
(
	wd_disc_t	* disc,		// valid disc pointer
	u8		usage_id,	// value of 'usage_table'
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
)
{
    TRACE("wd_get_part_by_usage(%p,%02x,%d)\n",disc,usage_id,load_data);
    const int idx = (int)( usage_id & WD_USAGE__MASK ) - WD_USAGE_PART_0;
    return wd_get_part_by_index(disc,idx,load_data);
}

///////////////////////////////////////////////////////////////////////////////

wd_part_t * wd_get_part_by_ptab
(
	wd_disc_t	*disc,		// valid disc pointer
	int		ptab_index,	// zero based index of partition table
	int		ptab_part_index,// zero based index within owning partition table
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
)
{
    TRACE("wd_get_part_by_ptab(%p,%d,%d,%d)\n",
		disc, ptab_index, ptab_part_index, load_data );
    ASSERT(disc);

    wd_part_t * part = disc->part;
    int i;
    for ( i = 0; i < disc->n_part; i++, part++ )
	if ( part->ptab_index == ptab_index
	  && part->ptab_part_index == ptab_part_index )
	{
	    return wd_get_part_by_index(disc,i,load_data);
	}
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

wd_part_t * wd_get_part_by_type
(
	wd_disc_t	*disc,		// valid disc pointer
	u32		part_type,	// find first partition with this type
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
)
{
    TRACE("wd_get_part_by_ptab(%p,%x,%d)\n",disc,part_type,load_data);
    ASSERT(disc);

    wd_part_t * part = disc->part;
    int i;
    for ( i = 0; i < disc->n_part; i++, part++ )
	if ( part->part_type == part_type )
	    return wd_get_part_by_index(disc,i,load_data);
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			load partition data		///////////////
///////////////////////////////////////////////////////////////////////////////

const int SYS_DIR_COUNT  = 3;
const int SYS_FILE_COUNT = 11;

enumError wd_load_part
(
	wd_part_t	*part,		// valid disc partition pointer
	bool		load_cert,	// true: load cert data too
	bool		load_h3		// true: load h3 data too
)
{
    TRACE("wd_load_part(%p,%d,%d)\n",part,load_cert,load_h3);

    ASSERT(part);
    wd_disc_t * disc = part->disc;
    ASSERT(disc);

    const wd_part_header_t * ph = &part->ph;

    const bool load_now = !part->is_loaded;
    if (load_now)
    {
	TRACE(" - load now\n");

	part->is_loaded = true;
	part->is_valid  = false;

	wbfs_iofree(part->tmd);  part->tmd  = 0;
	wbfs_iofree(part->cert); part->cert = 0;
	wbfs_iofree(part->h3);   part->h3   = 0;
	wbfs_iofree(part->fst);  part->fst  = 0;


	//----- load partition header

	enumError err = wd_read_part_raw(part,0,&part->ph,sizeof(part->ph),true);
	if (err)
	    return ERR_READ_FAILED;

	ntoh_part_header(&part->ph,&part->ph);
	wd_decrypt_title_key(&ph->ticket,part->key);

	part->data_off4 = part->part_off4 + ph->data_off4;
	part->part_size = (u64)( part->data_off4 + ph->data_size4 - part->part_off4 ) << 2;

	TRACE("part=%llx,%llx, tmd=%llx,%x, cert=%llx,%x, h3=%llx, data=%llx,%llx\n",
		(u64)part->part_off4	<< 2,	part->part_size,
		(u64)ph->tmd_off4	<< 2,	ph->tmd_size,
		(u64)ph->cert_off4	<< 2,	ph->cert_size,
		(u64)ph->h3_off4	<< 2,
		(u64)ph->data_off4	<< 2,	(u64)ph->data_size4 << 2 );
		

	//----- file size tests

	if ( disc->file_size )
	{
	    u64 off = (u64)( part->part_off4 + ph->data_off4 + ph->data_size4 ) << 2;
	    if ( off > disc->file_size )
		return ERR_READ_FAILED;
	}


	//----- load tmd

	if ( ph->tmd_size < sizeof(wd_tmd_t) )
	    return ERR_READ_FAILED;
	wd_tmd_t * tmd = wbfs_ioalloc(ph->tmd_size);
	if (!tmd)
	    OUT_OF_MEMORY;
	part->tmd = tmd;
	err = wd_read_part_raw( part, ph->tmd_off4, tmd, ph->tmd_size, true );
	if (err)
	    return ERR_READ_FAILED;


	//----- check encryption

	part->is_encrypted = !tmd_is_marked_not_encrypted(tmd)
			   && wd_is_block_encrypted(part,0,1);


	//----- load boot.bin 
	
	wd_boot_t * boot = &part->boot;
	err = wd_read_part(part,WII_BOOT_OFF,boot,sizeof(*boot),true);
	if (err)
	    return ERR_READ_FAILED;
	ntoh_boot(boot,boot);


	//----- calculate size of main.dol

	{
	    dol_header_t * dol = (dol_header_t*) disc->temp_buf;
	    
	    err = wd_read_part(part,boot->dol_off4,dol,DOL_HEADER_SIZE,true);
	    if (err)
		return ERR_READ_FAILED;
	    ntoh_dol_header(dol,dol);

	    // iterate through all segments
	    int i;
	    u32 dol_size = 0;
	    for ( i = 0; i < DOL_N_SECTIONS; i++ )
	    {
		const u32 size = dol->sect_off[i] + dol->sect_size[i];
		if ( dol_size < size )
		     dol_size = size;
	    }
	    TRACE("DOL-SIZE: %x <= %x\n",dol_size,boot->fst_off4-boot->dol_off4<<2);
	    ASSERT( dol_size <= boot->fst_off4-boot->dol_off4<<2 );
	    part->dol_size = dol_size;
	    
	    wd_mark_part(part,boot->dol_off4,dol_size);
	}


	//----- calculate size of apploader.img

	{
	    u8 * apl_header = (u8*) disc->temp_buf;
	    err = wd_read_part(part,WII_APL_OFF>>2,apl_header,0x20,false);
	    if (err)
		return ERR_READ_FAILED;
	    part->apl_size = 0x20 + be32(apl_header+0x14) + be32(apl_header+0x18);

	    wd_mark_part(part,WII_APL_OFF>>2,part->apl_size);
	}


	//----- load and iterate fst

	u32 fst_n		= 0;
	u32 fst_max_off4	= 0;
	u32 fst_max_size	= 0;
	u32 fst_dir_count	= SYS_DIR_COUNT;
	u32 fst_file_count	= SYS_FILE_COUNT;

	const u32 fst_size = boot->fst_size4 << 2;
	if (fst_size)
	{
	    wd_fst_item_t * fst = malloc(fst_size);
	    if (!fst)
		OUT_OF_MEMORY;
	    part->fst = fst;
	    err = wd_read_part(part,boot->fst_off4,fst,fst_size,true);
	    if (err)
		return ERR_READ_FAILED;

	    // mark used blocks

	    fst_dir_count++; // directory './files/'
	    fst_n = ntohl(fst->size);
	    const wd_fst_item_t *fst_end = fst + fst_n;

	    for ( fst++; fst < fst_end; fst++ )
		if (fst->is_dir)
		    fst_dir_count++;
		else
		{
		    fst_file_count++;
		    const u32 off4 = wbfs_ntohl(fst->offset4);
		    if ( fst_max_off4 < off4 )
			 fst_max_off4 = off4;
		    const u32 size = wbfs_ntohl(fst->size);
		    if ( fst_max_size < size )
			 fst_max_size = size;
		    wd_mark_part(part,off4,size);
		}
	}

	part->fst_n		= fst_n;
	part->fst_max_off4	= fst_max_off4;
	part->fst_max_size	= fst_max_size;
	part->fst_dir_count	= fst_dir_count;
	part->fst_file_count	= fst_file_count;

	//----- all done => mark as valid

	part->is_valid = true;
	wd_calc_fst_statistics(disc,false);
    }


    if (part->is_valid)
    {
	if ( ph->cert_off4 )
	{
	    if ( !part->cert && load_cert )
	    {
		part->cert = malloc(ph->cert_size);
		if (!part->cert)
		    OUT_OF_MEMORY;
		wd_read_part_raw( part, ph->cert_off4, part->cert,
					ph->cert_size, true );
	    }
	    else if (load_now)
		wd_read_part_raw( part, ph->cert_off4, 0, ph->cert_size, true );
	}

	if ( ph->h3_off4 )
	{
	    if ( !part->h3 && load_h3 )
	    {
		part->h3 = malloc(WII_H3_SIZE);
		if (!part->h3)
		    OUT_OF_MEMORY;
		wd_read_part_raw( part, ph->h3_off4, part->h3, WII_H3_SIZE, true );
	    }
	    else if (load_now)
		wd_read_part_raw( part, ph->h3_off4, 0, WII_H3_SIZE, true );
	}
    }

    return part->is_valid ? ERR_WARNING : ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_load_all_part
(
	wd_disc_t	* disc,		// valid disc pointer
	bool		load_cert,	// true: load cert data too
	bool		load_h3		// true: load h3 data too
)
{
    ASSERT(disc);

    enumError max_err = ERR_OK;
    int pi;
    for ( pi = 0; pi < disc->n_part; pi++ )
    {
	DASSERT(disc->part);
	const enumError err = wd_load_part(disc->part+pi,load_cert,load_h3);
	if ( max_err < err )
	     max_err = err;
    }

    return max_err;
}

///////////////////////////////////////////////////////////////////////////////

void wd_calc_fst_statistics
(
	wd_disc_t	* disc,		// valid disc pointer
	bool		sum_all		// false: summarize only enabled partitions
)
{
    DASSERT(disc);

    u32 n		= 0;
    u32 max_off4	= 0;
    u32 max_size	= 0;
    u32 dir_count	= 0;
    u32 file_count	= 0;

    wd_part_t *part, *part_end = disc->part + disc->n_part;
    for ( part = disc->part; part < part_end; part++ )
    {
	if ( sum_all || part->is_valid && part->is_enabled )
	{
	    n		+= part->fst_n;
	    dir_count	+= part->fst_dir_count;
	    file_count	+= part->fst_file_count;

	    if ( max_off4 < part->fst_max_off4 )
		 max_off4 = part->fst_max_off4;
	    if ( max_size < part->fst_max_size )
		 max_size = part->fst_max_size;
	}
    }

    disc->fst_n		 = n;
    disc->fst_max_off4	 = max_off4;
    disc->fst_max_size	 = max_size;
    disc->fst_dir_count	 = dir_count;
    disc->fst_file_count = file_count;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			select partitions		///////////////
///////////////////////////////////////////////////////////////////////////////

bool wd_is_part_selected
(
	wd_select_t	select,		// partition selector bit field
	u32		part_type,	// partiton type
	u32		ptab_index	// index of partiton table
)
{
    if ( select & WD_SEL_WHOLE_DISC )
	return true;

    if ( select & WD_SEL_PART_ACTIVE )
    {
	ccp d = (ccp)&part_type;
	if ( ISALNUM(d[0]) && ISALNUM(d[1]) && ISALNUM(d[2]) && ISALNUM(d[3]) )
	{
	    if ( !(select & WD_SEL_PART_ID) )
		return false;
	}
	else if ( part_type > WD_SELI_PART_MAX || !( 1ull << part_type & select ) )
	    return false;
    }

    if ( select & WD_SEL_PTAB_ACTIVE )
    {
	if ( ptab_index >= WII_MAX_PTAB || !( WD_SEL_PTAB_0 << ptab_index & select ) )
	    return false;
    }
    
    // WD_SELI_WHOLE_PART does not matter

    return true;
}

///////////////////////////////////////////////////////////////////////////////

int wd_select
(
	wd_disc_t	* disc,		// valid disc pointer
	wd_select_t	select		// partition selector bit field
)
{
    ASSERT(disc);

    int ip, count = 0;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = disc->part + ip;
	wd_load_part(part,false,false);
	part->is_enabled = part->is_valid
			&& wd_is_part_selected(select,part->part_type,part->ptab_index);
	count += part->is_enabled;
    }
    wd_calc_fst_statistics(disc,false);
    disc->patch_ptab_recommended = count != disc->n_part
				&& !(select & WD_SEL_WHOLE_DISC);
    return count;
}

///////////////////////////////////////////////////////////////////////////////

wd_select_t wd_set_select
(
	wd_select_t	select,		// current selector value
	wd_select_t	set_mask	// bits to set
)
{
    select |= set_mask & (WD_SELI_WHOLE_PART|WD_SELI_WHOLE_DISC);

    wd_select_t temp = set_mask & WD_SEL_PART__MASK;
    if (temp)
	select |= temp | WD_SEL_PART_ACTIVE;

    temp = set_mask & WD_SEL_PTAB__MASK;
    if (temp)
	select |= temp | WD_SEL_PTAB_ACTIVE;

    return select & WD_SEL__MASK;
}

///////////////////////////////////////////////////////////////////////////////

wd_select_t wd_clear_select
(
	wd_select_t	select,		// current selector value
	wd_select_t	clear_mask	// bits to clear
)
{
    select &= ~( clear_mask & (WD_SELI_WHOLE_PART|WD_SELI_WHOLE_DISC) );

    wd_select_t temp = clear_mask & WD_SEL_PART__MASK;
    if (temp)
    {
	if ( !(select & WD_SEL_PART_ACTIVE) )
	    select |= WD_SEL_PART__MASK | WD_SEL_PART_ACTIVE;
	select &= ~temp;
    }

    temp = clear_mask & WD_SEL_PTAB__MASK;
    if (temp)
    {
	if ( !(select & WD_SEL_PTAB_ACTIVE) )
	    select |= WD_SEL_PTAB__MASK | WD_SEL_PTAB_ACTIVE;
	select &= ~temp;
    }

    return select & WD_SEL__MASK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool wd_patch_ptab
(
	wd_disc_t	* disc,		// valid disc pointer
	void		* data,		// pointer to data area, WII_MAX_PTAB_SIZE
	bool		force_patch	// false: patch only if needed
)
{
    DASSERT(disc);
    DASSERT(data);

    wd_load_all_part(disc,false,false);
    
    if (!force_patch)
    {
	// test if any partitons is invalid or disabled
	int ip;
	for ( ip = 0; ip < disc->n_part; ip++ )
	{
	    wd_part_t * part = disc->part + ip;
	    if ( !part->is_valid || !part->is_enabled )
	    {
		force_patch = true;
		break;
	    }
	}
	if (!force_patch)
	    return false;
    }
 
    memset(data,0,WII_MAX_PTAB_SIZE);
    wd_ptab_t * ptab = data;
    wd_ptab_info_t  * info  = ptab->info;
    wd_ptab_entry_t * entry = ptab->entry;
    u32 off4 = (ccp)entry - (ccp)info + WII_PTAB_REF_OFF >> 2;

    int it;
    for ( it = 0; it < WII_MAX_PTAB; it++, info++ )
    {
	int n_part = 0, ip;
	for ( ip = 0; ip < disc->n_part; ip++ )
	{
	    wd_part_t * part = disc->part + ip;
	    if ( !part->is_valid || !part->is_enabled || part->ptab_index != it )
		continue;

	    n_part++;
	    entry->off4  = wbfs_htonl(part->part_off4);
	    entry->ptype = wbfs_htonl(part->part_type);
	    entry++;
	}

	if (n_part)
	{
	    info->n_part = wbfs_htonl(n_part);
	    info->off4   = wbfs_htonl(off4);
	    off4 += n_part * sizeof(*entry) >> 2;
	}
    }

    return true;   
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 usage table			///////////////
///////////////////////////////////////////////////////////////////////////////

const char wd_usage_name_tab[256] =
{
	".*ABCDEFGHIJKLMNOPQRSTUVWXYZABCD"
	"EFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJ"
	"KLMNOPQRSTUVWXYZABCDEFGHIJKLMNOP"
	"QRSTUVWXYZABCDEFGHIJKLMNOPQRSTUV"

	"?!abcdefghijklmnopqrstuvwxyzabcd"
	"efghijklmnopqrstuvwxyzabcdefghij"
	"klmnopqrstuvwxyzabcdefghijklmnop"
	"qrstuvwxyzabcdefghijklmnopqrstuv"
};

const u8 wd_usage_class_tab[256] =
{
	0,1,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,
	2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,
	2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,
	2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,

	4,5,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,
	3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,
	3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,
	3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3
};

///////////////////////////////////////////////////////////////////////////////

u8 * wd_calc_usage_table
(
	wd_disc_t	* disc		// valid disc partition pointer
)
{
    DASSERT(disc);
    wd_load_all_part(disc,false,false);
    return disc->usage_table;
}

///////////////////////////////////////////////////////////////////////////////

u8 * wd_filter_usage_table
(
	wd_disc_t	* disc,		// valid disc pointer
	u8		* usage_table,	// NULL or result. If NULL -> malloc()
	bool		whole_part,	// true: mark complete partitions
	bool		whole_disc	// true: mark all sectors
)
{
    wd_calc_usage_table(disc);

    if (!usage_table)
    {
	usage_table = malloc(WII_MAX_SECTORS);
	if (!usage_table)
	    OUT_OF_MEMORY;
    }
    DASSERT(usage_table);

    memcpy(usage_table,disc->usage_table,WII_MAX_SECTORS);

    u8 transform[0x100];
    memset( transform, whole_disc ? WD_USAGE_DISC : WD_USAGE_UNUSED, sizeof(transform) );
    transform[WD_USAGE_DISC] = WD_USAGE_DISC;

    disc->patch_ptab_recommended = false;

    int ip;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = disc->part + ip;

	if ( part->is_valid && part->is_enabled )
	{
	    u8 val = part->usage_id & WD_USAGE__MASK;
	    transform[val] = val;
	    val |= WD_USAGE_F_CRYPT;
	    transform[val] = val;

	    if ( whole_part )
	    {
		const u32 first_block = part->data_off4 /  WII_SECTOR_SIZE4;
		u32 end_block = ( part->ph.data_size4 + WII_SECTOR_SIZE4 - 1 )
			      / WII_SECTOR_SIZE4 + first_block;
		if ( end_block > WII_MAX_SECTORS )
		     end_block = WII_MAX_SECTORS;

		noTRACE("mark %llx+%llx => %x..%x [%x]\n",
			    (u64)part->data_off4<<2,
			    (u64)part->ph.data_size4,
			    first_block, end_block, end_block-first_block );

		if ( first_block < end_block )
		    memset( usage_table + first_block,
			    part->usage_id | WD_USAGE_F_CRYPT,
			    end_block - first_block );
	    }
	}
	else
	    disc->patch_ptab_recommended = !whole_disc;
    }

    u32 n_sect = disc->file_size
		? ( disc->file_size + WII_SECTOR_SIZE - 1 ) / WII_SECTOR_SIZE
		: WII_MAX_SECTORS;
    u8 *ptr, *end = usage_table + ( n_sect < WII_MAX_SECTORS ? n_sect : WII_MAX_SECTORS );
    for ( ptr = usage_table; ptr < end; ptr++ )
	*ptr = transform[*ptr];

    return usage_table;
}

///////////////////////////////////////////////////////////////////////////////

u8 * wd_filter_usage_table_sel
(
	wd_disc_t	* disc,		// valid disc pointer
	u8		* usage_table,	// NULL or result. If NULL -> malloc()
	wd_select_t	select		// partition selector bit field
)
{
    wd_select(disc,select);

    const bool whole_disc = ( select & WD_SEL_WHOLE_DISC ) != 0;
    const bool whole_part = whole_disc || ( select & WD_SEL_WHOLE_PART );
    return wd_filter_usage_table( disc, usage_table, whole_part, whole_disc );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u32 wd_count_used_disc_blocks
(
	wd_disc_t	* disc,		// valid pointer to a disc partition
	u32		block_size	// if >1: count every 'block_size'
					//        continuous blocks as one block
)
{
    wd_calc_usage_table(disc);
    return wd_count_used_blocks(disc->usage_table,block_size);
}

///////////////////////////////////////////////////////////////////////////////

u32 wd_count_used_disc_blocks_sel
(
	wd_disc_t	* disc,		// valid pointer to a disc partition
	u32		block_size,	// if >1: count every 'block_size'
					//        continuous blocks as one block
	wd_select_t	select		// partition selector bit field
)
{
    u8 usage_tab[WII_MAX_SECTORS];
    wd_filter_usage_table_sel(disc,usage_tab,select);
    return wd_count_used_blocks(usage_tab,block_size);
}

///////////////////////////////////////////////////////////////////////////////

u32 wd_count_used_blocks
(
	const u8 *	usage_table,	// valid pointer to usage table
	u32		block_size	// if >1: count every 'block_size'
					//        continuous blocks as one block
)
{
    DASSERT(usage_table);

    const u8 * end_tab = usage_table + WII_MAX_SECTORS;
    u32 count = 0;
    
    if ( block_size > 1 )
    {
	if ( block_size < WII_MAX_SECTORS )
	{
	    //----- count all but last block

	    end_tab -= block_size;
	    for ( ; usage_table < end_tab; usage_table += block_size )
	    {
		int i;
		for ( i = 0; i < block_size; i++ )
		    if (usage_table[i])
		    {
			count++;
			break;
		    }
	    }

	    end_tab += block_size;
	}

	//----- count last blocks

	while ( usage_table < end_tab )
	    if ( *usage_table++ )
	    {
		count++;
		break;
	    }
    }
    else
    {
	// optimization for single block count

	while ( usage_table < end_tab )
	    if ( *usage_table++ )
		count++;
    }
	
    return count;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  file iteration		///////////////
///////////////////////////////////////////////////////////////////////////////

int wd_iterate_files
(
	wd_disc_t	* disc,		// valid pointer to a disc partition
	wd_file_func_t	func,		// call back function
	void		* param,	// user defined parameter
	wd_ipm_t	prefix_mode	// prefix mode
)
{
    ASSERT(disc);
    ASSERT(func);

    if ( !disc->part || !disc->n_part )
	return 0;

    wd_load_all_part(disc,false,false);
    wd_part_t *part, *part_end = disc->part + disc->n_part;

    wd_iterator_t it;
    memset(&it,0,sizeof(it));
    it.param = param;
    it.disc  = disc;


    //----- prefix mode

    if ( prefix_mode <= WD_IPM_AUTO )
    {
	int count = 0;
	for ( part = disc->part; part < part_end; part++ )
	    if ( part->is_valid && part->is_enabled )
	    {
		if ( part->part_type == WD_PART_DATA )
		    count++;
		else
		    count += 2;
	    }
	prefix_mode = count == 1 ? WD_IPM_POINT : WD_IPM_PART_NAME;
    }
    it.prefix_mode = prefix_mode;


    //----- iterate partitions

    int stat = 0;
    for ( part = disc->part; part < part_end && !stat; part++ )
    {
	if ( !part->is_valid || !part->is_enabled )
	    continue;

	it.part = part;


	switch(prefix_mode)
	{
	    case WD_IPM_NONE:
		break;

	    case WD_IPM_POINT:
		strcpy(it.prefix,"./");
		break;

	    case WD_IPM_PART_INDEX:
		snprintf(it.prefix,sizeof(it.prefix),"P%x/", part->part_type );
		break;

	    //case WD_IPM_PART_NAME:
	    default:
		wd_print_part_name(it.prefix,sizeof(it.prefix)-1,part->part_type,WD_PNAME_P_NAME);
		strcat(it.prefix,"/");
		break;
	}

	it.prefix_len = strlen(it.prefix);
	strcpy(it.path,it.prefix);
	it.fst_name = it.path + it.prefix_len;

	it.icm	= WD_ICM_OPEN_PART;
	const u32 off4 = it.off4 = part->part_off4;
	stat = func(&it);
	if (stat)
	    break;


	//----- './' files

	const wd_part_header_t * ph = &part->ph;

	it.icm  = WD_ICM_DIRECTORY;
	it.off4 = 0;
	it.size = SYS_DIR_COUNT + SYS_FILE_COUNT + part->fst_n - 1;
	it.data = 0;
	stat = func(&it);
	if (stat)
	    break;

	it.icm  = WD_ICM_DATA;
	it.off4 = off4;
	it.size = sizeof(part->ph.ticket);
	it.data = &part->ph.ticket;
	strcpy(it.fst_name,"ticket.bin");
	stat = func(&it);
	if (stat)
	    break;

	DASSERT( it.icm == WD_ICM_DATA );
	it.off4 = off4 + ph->tmd_off4;
	it.size = ph->tmd_size;
	it.data = part->tmd;
	strcpy(it.fst_name,"tmd.bin");
	stat = func(&it);
	if (stat)
	    break;

	it.icm  = WD_ICM_COPY;
	it.off4 = off4 + ph->cert_off4;
	it.size = ph->cert_size;
	it.data = 0;
	strcpy(it.fst_name,"cert.bin");
	stat = func(&it);
	if (stat)
	    break;

	DASSERT( it.icm == WD_ICM_COPY );
	it.off4 = off4 + ph->h3_off4;
	it.size = WII_H3_SIZE;
	DASSERT ( !it.data );
	strcpy(it.fst_name,"h3.bin");
	stat = func(&it);
	if (stat)
	    break;
	

	//----- 'disc/' files

	it.icm  = WD_ICM_DIRECTORY;
	it.off4 = 0;
	it.size = 2;
	it.data = 0;
	strcpy(it.fst_name,"disc/");
	stat = func(&it);
	if (stat)
	    break;

	it.icm  = WD_ICM_DATA;
	it.off4 = 0;
	it.size = sizeof(disc->dhead);
	it.data = &disc->dhead;
	strcpy(it.fst_name,"disc/header.bin");
	stat = func(&it);
	if (stat)
	    break;

	DASSERT( it.icm == WD_ICM_DATA );
	it.off4 = WII_REGION_OFF >> 2;
	it.size = sizeof(disc->region);
	it.data = &disc->region;
	strcpy(it.fst_name,"disc/region.bin");
	stat = func(&it);
	if (stat)
	    break;

	//----- 'sys/' files

	it.icm  = WD_ICM_DIRECTORY;
	it.off4 = 0;
	it.size = 5;
	it.data = 0;
	strcpy(it.fst_name,"sys/");
	stat = func(&it);
	if (stat)
	    break;

	it.icm  = WD_ICM_FILE;
	it.off4 = WII_BOOT_OFF >> 2;
	it.size = WII_BOOT_SIZE;
	DASSERT(!it.data);
	strcpy(it.fst_name,"sys/boot.bin");
	stat = func(&it);
	if (stat)
	    break;

	DASSERT( it.icm == WD_ICM_FILE );
	it.off4 = WII_BI2_OFF >> 2;
	it.size = WII_BI2_SIZE;
	DASSERT(!it.data);
	strcpy(it.fst_name,"sys/bi2.bin");
	stat = func(&it);
	if (stat)
	    break;

	DASSERT( it.icm == WD_ICM_FILE );
	it.off4 = WII_APL_OFF >> 2;
	it.size = part->apl_size;
	DASSERT(!it.data);
	strcpy(it.fst_name,"sys/apploader.img");
	stat = func(&it);
	if (stat)
	    break;

	DASSERT( it.icm == WD_ICM_FILE );
	it.off4 = part->boot.dol_off4;
	it.size = part->dol_size;
	DASSERT(!it.data);
	strcpy(it.fst_name,"sys/main.dol");
	stat = func(&it);
	if (stat)
	    break;

	DASSERT( it.icm == WD_ICM_FILE );
	it.off4 = part->boot.fst_off4;
	it.size = part->boot.fst_size4 << 2;
	DASSERT(!it.data);
	strcpy(it.fst_name,"sys/fst.bin");
	stat = func(&it);
	if (stat)
	    break;


	//----- FST files

	const wd_fst_item_t *fst = part->fst;

	if (fst)
	{
	    //----- setup stack

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

	    char *path_end = it.path + sizeof(it.path) - MAX_DEPTH;
	    it.icm  = WD_ICM_DIRECTORY;
	    it.off4 = 0;
	    it.size = part->fst_n-1;
	    DASSERT(!it.data);
	    strcpy(it.fst_name,"files/");
	    stat = func(&it);
	    if (stat)
		break;

	
	    //----- main loop

	    const wd_fst_item_t *fst_end = fst + part->fst_n;
	    const wd_fst_item_t *dir_end = fst_end;
	    char * path_ptr = it.fst_name + 6;

	    for ( fst++; fst < fst_end && !stat; fst++ )
	    {
		while ( fst >= dir_end && stack > stack_buf )
		{
		    // leave a directory
		    stack--;
		    dir_end = stack->dir_end;
		    path_ptr = stack->path;
		}

		const char * fname = (char*)fst_end
				   + (wbfs_ntohl(fst->name_off)&0xffffff);
		char * path_dest = path_ptr;
		while ( path_dest < path_end && *fname )
		    *path_dest++ = *fname++;

		if (fst->is_dir)
		{
		    *path_dest++ = '/';
		    *path_dest = 0;

		    ASSERT(stack<stack_max);
		    if ( stack < stack_max )
		    {
			stack->dir_end = dir_end;
			stack->path = path_ptr;
			stack++;
			dir_end = part->fst + ntohl(fst->size);
			path_ptr = path_dest;
		    }

		    it.icm  = WD_ICM_DIRECTORY;
		    it.off4 = 0;
		    it.size = dir_end-fst-1;
		    stat = func(&it);
		}
		else
		{
		    *path_dest = 0;
		    it.icm  = WD_ICM_FILE;
		    it.off4 = wbfs_ntohl(fst->offset4);
		    it.size = wbfs_ntohl(fst->size);
		    stat = func(&it);
		}
	    }
	}
	if (stat)
	    break;


	//----- WD_ICM_CLOSE_PART

	*it.fst_name = 0;
	it.icm  = WD_ICM_CLOSE_PART;
	it.off4 = off4;
	it.size = 0;
	it.data = 0;
	stat = func(&it);
    }

    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    print files			///////////////
///////////////////////////////////////////////////////////////////////////////

const char wd_sep_200[201] = // 200 * '-' + NULL
	"--------------------------------------------------"
	"--------------------------------------------------"
	"--------------------------------------------------"
	"--------------------------------------------------";

///////////////////////////////////////////////////////////////////////////////

void wd_initialize_print_fst
(
	wd_print_fst_t	* pf,		// valid pointer
	wd_pfst_t	mode,		// mode for setup
	FILE		* f,		// NULL or output file
	int		indent,		// indention of the output
	u32		max_off4,	// NULL or maximal offset4 value of all files
	u32		max_size	// NULL or maximal size value of all files
)
{
    DASSERT(pf);
    memset(pf,0,sizeof(*pf));

    pf->f		= f ? f : stdout;
    pf->indent		= wd_normalize_indent(indent);
    pf->mode		= mode;

    char buf[50];

    if ( mode & WD_PFST_OFFSET )
	pf->fw_offset = max_off4 > 0
			? snprintf(buf,sizeof(buf),"%llx",(u64)max_off4<<2)
			: 9;

    if ( mode & WD_PFST_SIZE_HEX )
	pf->fw_size_hex = max_size > 0
			? snprintf(buf,sizeof(buf),"%x",max_size)
			: 7;

    if ( mode & WD_PFST_SIZE_DEC )
	pf->fw_size_dec = max_size > 0
			? snprintf(buf,sizeof(buf),"%u",max_size)
			: 8;
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_fst_header
(
	wd_print_fst_t	* pf,		// valid pointer
	int		max_name_len	// max name len, needed for separator line
)
{
    ASSERT(pf);
    ASSERT(pf->f);

    pf->indent = wd_normalize_indent(pf->indent);
    if ( max_name_len < 11 )
	max_name_len = 11;

    //----- first header line

    fprintf(pf->f,"%*s",pf->indent,"");

    if ( pf->fw_offset )
    {
	if ( pf->fw_offset < 6 )
	     pf->fw_offset = 6;
	fprintf(pf->f,"%*s  ",pf->fw_offset,"offset");
	max_name_len += 2 + pf->fw_offset;
    }

    if ( pf->fw_size_hex )
    {
	if ( pf->fw_size_hex < 4 )
	     pf->fw_size_hex = 4;
	fprintf(pf->f,"%*s ",pf->fw_size_hex,"size");
	max_name_len += 1 + pf->fw_size_hex;
    }

    if ( pf->fw_size_dec )
    {
	if ( pf->fw_size_dec < 4 )
	     pf->fw_size_dec = 4;
	fprintf(pf->f,"%*s ",pf->fw_size_dec,"size");
	max_name_len += 1 + pf->fw_size_dec;
    }

    //----- second header line

    fprintf(pf->f,"\n%*s",pf->indent,"");

    if ( pf->fw_offset )
	fprintf(pf->f,"%*s  ",pf->fw_offset,"hex");

    if ( pf->fw_size_hex )
	fprintf(pf->f,"%*s ",pf->fw_size_hex,"hex");

    if ( pf->fw_size_dec )
	fprintf(pf->f,"%*s ",pf->fw_size_dec,"dec");

    ccp sep = "";
    if ( pf->fw_offset || pf->fw_size_hex || pf->fw_size_dec )
    {
	sep = " ";
	max_name_len++;
    }

    fprintf(pf->f,
	    "%spath + file\n"
	    "%*s%.*s\n",
	    sep, pf->indent, "", max_name_len, wd_sep_200 );
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_fst_item
(
	wd_print_fst_t	* pf,		// valid pointer
	wd_part_t	* part,		// valid pointer to a disc partition
	wd_icm_t	icm,		// iterator call mode
	u32		offset4,	// offset/4 to read
	u32		size,		// size of object
	ccp		fname1,		// NULL or file name, part 1
	ccp		fname2		// NULL or file name, part 2
)
{
    DASSERT(pf);
    DASSERT(pf->f);
    DASSERT(part);

    char buf[200];
    ccp sep = pf->fw_offset || pf->fw_size_hex || pf->fw_size_dec ? " " : "";

    switch (icm)
    {
	case WD_ICM_OPEN_PART:
	    if ( pf->mode & WD_PFST_PART )
	    {
		int indent = pf->indent;
		if (pf->fw_offset)
		    indent += 2 + pf->fw_offset;
		if (pf->fw_size_hex)
		    indent += 1 + pf->fw_size_hex;
		if (pf->fw_size_dec)
		    indent += 1 + pf->fw_size_dec;
		fprintf(pf->f,"%*s%s** Partition #%u.%u, %s **\n",
		    indent, "", sep,
		    part->ptab_index, part->ptab_part_index,
		    wd_print_part_name(buf,sizeof(buf),part->part_type,WD_PNAME_NUM_INFO));
	    }
	    break;

	case WD_ICM_DIRECTORY:
	    fprintf(pf->f,"%*s",pf->indent,"");
	    if (pf->fw_offset)
		fprintf(pf->f,"%*s  ",pf->fw_offset,"-");
	    if (pf->fw_size_dec)
	    {
		if (pf->fw_size_hex)
		    fprintf(pf->f,"%*s ",pf->fw_size_hex,"-");
		fprintf(pf->f,"N=%-*u",pf->fw_size_dec-1,size);
	    }
	    else if (pf->fw_size_hex)
		fprintf(pf->f,"N=%-*u",pf->fw_size_hex-1,size);
	    fprintf(pf->f,"%s%s%s\n", sep, fname1 ? fname1 : "", fname2 ? fname2 : "" );
	    break;

	case WD_ICM_FILE:
	case WD_ICM_COPY:
	case WD_ICM_DATA:
	    fprintf(pf->f,"%*s",pf->indent,"");
	    if (pf->fw_offset)
		fprintf(pf->f,"%*llx%c ",
			pf->fw_offset, (u64)offset4<<2, icm == WD_ICM_FILE ? '+' : ' ' );
	    if (pf->fw_size_hex)
		fprintf(pf->f,"%*x ", pf->fw_size_hex, size);
	    if (pf->fw_size_dec)
		fprintf(pf->f,"%*u ", pf->fw_size_dec, size);
	    fprintf(pf->f,"%s%s%s\n", sep, fname1 ? fname1 : "", fname2 ? fname2 : "" );
	    break;

	case WD_ICM_CLOSE_PART:
	    break;
    }
}

///////////////////////////////////////////////////////////////////////////////

static int wd_print_fst_item_wrapper
(
	struct wd_iterator_t *it	// iterator struct with all infos
)
{
    DASSERT(it);
    wd_print_fst_t *d = it->param;
    DASSERT(d);
    if ( !d->filter_func || !d->filter_func(it) )
	wd_print_fst_item( d, it->part, it->icm, it->off4, it->size, it->path, 0 );
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_fst
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	wd_disc_t	* disc,		// valid pointer to a disc partition
	wd_ipm_t	prefix_mode,	// prefix mode
	wd_pfst_t	pfst_mode,	// print mode
	wd_file_func_t	filter_func,	// NULL or filter function
	void		* filter_param	// user defined parameter
)
{
    ASSERT(f);
    indent = wd_normalize_indent(indent);

    //----- setup pf and calc fw

    if ( pfst_mode & (WD_PFST_OFFSET|WD_PFST_SIZE_HEX|WD_PFST_SIZE_DEC) )
    {
	wd_load_all_part(disc,false,false);
	wd_calc_fst_statistics(disc,false);
    }

    wd_print_fst_t pf;
    wd_initialize_print_fst(&pf,pfst_mode,f,indent,disc->fst_max_off4,disc->fst_max_size);
    pf.filter_func	= filter_func;
    pf.filter_param	= filter_param;

    //----- print out

    if ( pfst_mode & WD_PFST_HEADER )
	wd_print_fst_header(&pf,50);

    wd_iterate_files(disc,wd_print_fst_item_wrapper,&pf,prefix_mode);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    patching			///////////////
///////////////////////////////////////////////////////////////////////////////

void wd_reset_patch 
(
	wd_patch_t  * patch		// NULL or patching data
)
{
    if (patch)
    {
	if (patch->item)
	{
	    int idx;
	    for ( idx = 0; idx < patch->used; idx++ )
	    {
		wd_patch_item_t * item = patch->item + idx;
		if (item->data_alloced)
		    free(item->data);
	    }
	    free(patch->item);
	}
	memset(patch,0,sizeof(*patch));
    }
}

///////////////////////////////////////////////////////////////////////////////

static u32 wd_insert_patch_helper
(
	wd_patch_t	* patch,	// NULL or patching data
	u64		offset,		// offset of object
	u64		size,		// size of object
	bool		* result_found	// true, if item found
)
{
    DASSERT(patch);
    DASSERT(result_found);

    int beg = 0;
    int end = patch->used - 1;
    while ( beg <= end )
    {
	const u32 idx = (beg+end)/2;
	wd_patch_item_t * item = patch->item + idx;
	if ( offset < item->offset )
	    end = idx - 1 ;
	else if ( offset > item->offset )
	    beg = idx + 1;
	else if ( size < item->size )
	    end = idx - 1 ;
	else if ( size > item->size ) 
	    beg = idx + 1;
	else
	{
	    *result_found = true;
	    return idx;
	}
    }

    *result_found = false;
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

wd_patch_item_t * wd_insert_patch
(
	wd_patch_t	* patch,	// NULL or patching data
	wd_patch_mode_t	mode,		// patch mode
	u64		offset,		// offset of object
	u64		size		// size of object
)
{
    DASSERT(patch);

    if ( patch->used == patch->size )
    {
	patch->size += 10;
	patch->item = realloc(patch->item,patch->size*sizeof(*patch->item));
	if (!patch->item)
	    OUT_OF_MEMORY;
    }

    bool found;
    u32 idx = wd_insert_patch_helper(patch,offset,size,&found);

    DASSERT( idx <= patch->used );
    wd_patch_item_t * item = patch->item + idx;
    if (!found)
    {
	memmove(item+1,item,(patch->used-idx)*sizeof(*item));
	patch->used++;
    }
    else if (item->data_alloced)
	free(item->data);

    memset(item,0,sizeof(*item));
    item->mode	 = mode;
    item->offset = offset;
    item->size   = size;
    return item;
}

///////////////////////////////////////////////////////////////////////////////

void wd_dump_patch
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	wd_patch_t	* patch		// NULL or patching data
)
{
    ASSERT(patch);
    if ( !f || !patch->used )
	return;

    indent = wd_normalize_indent(indent);
    int fw = 80 - indent;
    if ( fw < 52 )
	 fw = 52;

    fprintf(f,"\n%*s     offset .. offset end     size  comment\n"
		"%*s%.*s\n",
		indent,"", indent,"", fw, wd_sep_200 );

    u64 prev_end = 0;
    wd_patch_item_t *item, *last = patch->item + patch->used - 1;
    for ( item = patch->item; item <= last; item++ )
    {
	const u64 end = item->offset + item->size;
	fprintf(f,"%*s%c %9llx .. %9llx %9llx  %s\n", indent,"",
		item->offset < prev_end ? '!' : ' ',
		item->offset, end, item->size, item->info );
	prev_end = end;
    }
    putc('\n',f);
}

///////////////////////////////////////////////////////////////////////////////

void wd_dump_disc_patch
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	wd_disc_t	* disc,		// valid pointer to a disc partition
	bool		print_title,	// true: print table titles
	bool		print_part	// true: print partitions too
)
{
    ASSERT(disc);
    if (!f)
	return;
    indent = wd_normalize_indent(indent);

    if (disc->patch.used)
    {
	if (print_title)
	    fprintf(f,"\n%*sPatching table for the disc:\n",indent,"");
	wd_dump_patch(f,indent+2,&disc->patch);
    }

    if (print_part)
    {
	wd_part_t *part, *part_end = disc->part + disc->n_part;
	for ( part = disc->part; part < part_end; part++ )
	{
	    if (part->patch.used)
	    {
		if (print_title)
		{
		    char buf[50];
		    fprintf(f,"%*sPatching table for partition %s:\n",
			indent, "",
			wd_print_part_name(buf,sizeof(buf),part->part_type,WD_PNAME_NUM_INFO));
		}
		wd_dump_patch(f,indent+2,&part->patch);
	    }
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			dump data structure		///////////////
///////////////////////////////////////////////////////////////////////////////

int wd_normalize_indent
(
	int		indent		// base vlaue to normalize
)
{
    return indent < 0 ? 0 : indent < 50 ? indent : 50;
}

///////////////////////////////////////////////////////////////////////////////

void wd_dump_disc
(
	FILE		*f,		// valid output file
	int		indent,		// indention of the output
	wd_disc_t	*disc,		// valid disc pointer
	int		dump_level	// dump level
					//	>0: print extended part info 
					//	>1: print usage table
)
{
    TRACE("wd_dump_disc()\n");

    ASSERT(f);
    ASSERT(disc);

    if ( indent < 0 )
	indent = 0;
    else if ( indent > 50 )
	indent = 50;

    fprintf(f,"%*sRead func+data:     %p %p\n", indent,"",
		disc->read_func, disc->read_data );

    fprintf(f,"%*sFile size:          %llx/hex = %llu\n", indent,"",
		disc->file_size, disc->file_size );

    const u8 *m1 = (u8*)&disc->dhead.magic;
    const u8 *m2 = (u8*)&disc->magic2;
    fprintf(f,"%*sMagics:             %02x-%02x-%02x-%02x %02x-%02x-%02x-%02x\n",
		indent,"",
		m1[0], m1[1], m1[2], m1[3], 
		m2[0], m2[1], m2[2], m2[3] );

    fprintf(f,"%*sID and title:       %.6s, %.64s\n", indent,"",
		&disc->dhead.disc_id,  disc->dhead.game_title );

    u8 * p8 = disc->region.region_info;
    fprintf(f,"%*sRegion setting:     %d / %02x %02x %02x %02x  %02x %02x %02x %02x\n",
		indent,"", wbfs_ntohl(disc->region.region),
		p8[0], p8[1], p8[2], p8[3], p8[4], p8[5], p8[6], p8[7] );

    int ipt;
    for ( ipt = 0; ipt < WII_MAX_PTAB; ipt++ )
    {
	wd_ptab_info_t * pt = disc->ptab_info + ipt;
	const int n = wbfs_ntohl(pt->n_part);
	if (n)
	    fprintf(f,"%*sPartition table #%u: off=%llx, N(part)=%u\n", indent,"",
		ipt, (u64)wbfs_ntohl(pt->off4)<<2, n );
    }

    const bool load_part = dump_level > 0;
    int ip;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = wd_get_part_by_index(disc,ip,load_part);

	if (load_part)
	    putchar('\n');

	char pname[30];
	fprintf(f,"%*sPartition #%-2u [%u.%02u], "
		"loaded=%d, valid=%d, enabled=%d, enc=%d, type=%s\n",
		indent,"",
		part->index, part->ptab_index , part->ptab_part_index,
		part->is_loaded, part->is_valid, part->is_enabled,
		part->is_encrypted,
		wd_print_part_name(pname,sizeof(pname),
					part->part_type, WD_PNAME_NUM_INFO ));

	if ( load_part && part->is_valid )
	{
	    u64 off = part->part_off4 << 2;
	    fprintf(f,"%*s  TICKET:      %9llx .. %9llx, %9zx\n", indent,"",
		off, off + sizeof(part->ph), sizeof(part->ph) );

	    if ( part->ph.tmd_off4 )
	    {
		off = ( part->part_off4 + part->ph.tmd_off4 ) << 2;
		fprintf(f,"%*s  TMD:         %9llx .. %9llx, %9x\n", indent,"",
		    off, off + part->ph.tmd_size, part->ph.tmd_size );
	    }

	    if ( part->ph.cert_off4 )
	    {
		off = ( part->part_off4 + part->ph.cert_off4 ) << 2;
		fprintf(f,"%*s  CERT:        %9llx .. %9llx, %9x,  loaded=%d\n", indent,"",
		    off, off + part->ph.cert_size, part->ph.cert_size,
		    part->cert != 0 );
	    }

	    if ( part->ph.h3_off4 )
	    {
		off = ( part->part_off4 + part->ph.h3_off4 ) << 2;
		fprintf(f,"%*s  H3:          %9llx .. %9llx, %9x,  loaded=%d\n", indent,"",
		    off, off + WII_H3_SIZE, WII_H3_SIZE,
		    part->h3 != 0 );
	    }

	    if ( part->ph.data_off4 )
	    {
		off = ( part->part_off4 + part->ph.data_off4 ) << 2;
		u64 data_size = part->ph.data_size4 << 2;
		fprintf(f,"%*s  DATA:        %9llx .. %9llx, %9llx\n", indent,"",
		    off, off + data_size, data_size );
	    }
	}
    }

 #if 0 // defined(DEBUG) && defined(TEST)
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = wd_get_part_by_index(disc,ip,load_part);

	fprintf(f,"%*s  ID: |",indent,"");
	int pmode;
	for ( pmode = 0; pmode < WD_PNAME__N; pmode++ )
	{
	    char pname[30];
	    wd_print_part_name(pname,sizeof(pname),part->part_type,pmode);
	    fprintf(f,"%s|",pname);
	}
	fputc('\n',f);
    }
 #endif

    if ( dump_level > 1 )
    {
	fprintf(f,"\n%*sUsage map:\n",indent,"");
	wd_dump_disc_usage_tab(f,indent+2,disc,false);
    }
}

///////////////////////////////////////////////////////////////////////////////

void wd_dump_disc_usage_tab
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	wd_disc_t	* disc,		// valid disc pointer
	bool		print_all	// false: suppress repeated lines
)
{
    wd_dump_usage_tab( f, indent, wd_calc_usage_table(disc), print_all );
}

///////////////////////////////////////////////////////////////////////////////

void wd_dump_usage_tab
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	const u8	* usage_tab,	// valid pointer, size = WII_MAX_SECTORS
	bool		print_all	// false: ignore const lines
)
{
    ASSERT(f);
    ASSERT(usage_tab);

    if ( indent < 0 )
	indent = 0;
    else if ( indent > 50 )
	indent = 50;

    indent += 9; // add address field width

    const u8 * ptr = usage_tab;
    const u8 * tab_end = ptr + WII_MAX_SECTORS - 1;
    while ( !*tab_end && tab_end > ptr )
	tab_end--;
    tab_end++;

    enum { line_count = 64 };
    char buf[2*line_count];
    char skip_buf[2*line_count];

    int skip_count = 0;
    u64 skip_addr = 0;

    while ( ptr < tab_end )
    {
	const u64 addr = (u64)( ptr - usage_tab ) * WII_SECTOR_SIZE;
	
	char * dest = buf;
	const u8 * line_end = ptr + line_count;
	if ( line_end > tab_end )
	    line_end = tab_end;

	int pos = 0, last_count = 0;
	const u8 cmp_val = *ptr;
	while ( ptr < line_end )
	{
	    if ( !( pos++ & 15 ) )
		*dest++ = ' ';
	    last_count += *ptr == cmp_val;
	    *dest++ = wd_usage_name_tab[*ptr++];
	}
	*dest = 0;
	DASSERT( dest < buf + sizeof(buf) );
	if ( last_count < line_count )
	{
	    if (skip_count)
	    {
		if ( skip_count == 1 )
		    fprintf(f,"%*llx:%s\n",indent,skip_addr,skip_buf);
		else
		    fprintf(f,"%*llx:%s *%5u\n",indent,skip_addr,skip_buf,skip_count);
		skip_count = 0;
	    }
	    fprintf(f,"%*llx:%s\n",indent,addr,buf);
	}
	else if (!skip_count++)
	{
	    memcpy(skip_buf,buf,sizeof(skip_buf));
	    skip_addr = addr;
	}
    }

    if ( skip_count == 1 )
	fprintf(f,"%*llx:%s\n",indent,skip_addr,skip_buf);
    else if (skip_count)
	fprintf(f,"%*llx:%s *%5u\n",indent,skip_addr,skip_buf,skip_count);
}

///////////////////////////////////////////////////////////////////////////////

void wd_dump_mem
(
    wd_disc_t		* disc,		// valid disc pointer
    wd_mem_func_t	func,		// valid function pointer
    void		* param		// user defined parameter
)
{
    ASSERT(disc);
    DASSERT(func);
    TRACE("wd_dump_mem(%p,%p,%p)\n",disc,func,param);

    char msg[80];

    //----- header

    const u8 *m = (u8*)&disc->dhead.magic;
    snprintf( msg, sizeof(msg),
		"Header, magic=%02x-%02x-%02x-%02x, id=%s",
		m[0], m[1], m[2], m[3], wd_print_id(&disc->dhead.disc_id,6,0) );
    func(param,0,sizeof(disc->dhead),msg);


    //----- partition tables

    func(param,WII_PTAB_REF_OFF,sizeof(disc->ptab_info),
		"Partition address table");

    int ip;
    for ( ip = 0; ip < WII_MAX_PTAB; ip++ )
    {
	const u32 np = wbfs_ntohl(disc->ptab_info[ip].n_part);
	if (np)
	{
	    const u64 off = (u64)wbfs_ntohl(disc->ptab_info[ip].off4) << 2;
	    snprintf( msg, sizeof(msg),
			"Partition table #%u with %u partition%s",
			ip, np, np == 1 ? "" : "s" );
	    func(param,off,np*sizeof(wd_ptab_entry_t),msg);
	}
    }
	

    //----- region settings and magic2

    snprintf( msg, sizeof(msg),
		"Region settings, region=%x",
		wbfs_ntohl(disc->region.region) );
    func(param,WII_REGION_OFF,sizeof(disc->region),msg);

    m = (u8*)&disc->magic2;
    snprintf( msg, sizeof(msg),
		"Magic2: %02x-%02x-%02x-%02x",
		m[0], m[1], m[2], m[3] );
    func(param,WII_MAGIC2_OFF,sizeof(disc->magic2),msg);


    //----- iterate partitions

    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = wd_get_part_by_index(disc,ip,1);

	char pname_buf[20];
	char * pname = wd_print_part_name(pname_buf,sizeof(pname_buf),
						part->part_type, WD_PNAME_NAME );
	char * dest = msg + snprintf( msg, sizeof(msg),
					"P.%u.%u %s: ",
					part->ptab_index, part->ptab_part_index,
					pname );
	size_t msgsize = msg + sizeof(msg) - dest;

	if (!part->is_valid)
	{
	    snprintf( dest, msgsize, "** INVALID **" );
	    func(param,(u64)part->part_off4<<2,0,msg);
	}
	else if (part->is_enabled)
	{
	    const wd_part_header_t *ph = &part->ph;

	    //--- header + ticket

	    snprintf( dest, msgsize, "ticket, id=%s",
			wd_print_id(ph->ticket.title_id+4,4,0) );
	    func(param,(u64)part->part_off4<<2,sizeof(*ph),msg);

	    //--- tmd

	    if (ph->tmd_off4)
	    {
		const u64 sys_version = wbfs_ntoh64(part->tmd->sys_version);
		const u32 hi = sys_version >> 32;
		const u32 lo = (u32)sys_version;
		if ( hi == 1 && lo < 0x100 )
		    snprintf( dest, msgsize, "tmd, ios=%u, id=%s",
				lo, wd_print_id(part->tmd->title_id+4,4,0) );
		else
		    snprintf( dest, msgsize, "tmd, sys=%08x-%08x, id=%s",
				hi, lo, wd_print_id(part->tmd->title_id+4,4,0) );
		func(param,(u64)(part->part_off4+ph->tmd_off4)<<2,ph->tmd_size,msg);
	    }

	    //--- cert

	    if (ph->cert_off4)
	    {
		snprintf( dest, msgsize, "cert" );
		func(param,(u64)(part->part_off4+ph->cert_off4)<<2,ph->cert_size,msg);
	    }
	    
	    //--- h3

	    if (ph->h3_off4)
	    {
		snprintf( dest, msgsize, "h3" );
		func(param,(u64)(part->part_off4+ph->h3_off4)<<2,WII_H3_SIZE,msg);
	    }

	    //--- data

	    if (ph->data_off4)
	    {
		u64 off = (u64)part->data_off4<<2;
		u64 end = off + ((u64)ph->data_size4<<2);
		const u32 fst_sect = part->fst_n
			? off/WII_SECTOR_SIZE + part->boot.fst_off4/WII_SECTOR_DATA_SIZE4
			: 0;
		while ( off < end )
		{
		    u32 sect = off / WII_SECTOR_SIZE;
		    u32 start = sect;
		    while ( sect < WII_MAX_SECTORS && !disc->usage_table[sect] )
			sect++;
		    if ( start < sect )
		    {
			u64 off2 = sect * (u64)WII_SECTOR_SIZE;
			if ( off2 > end )
			    off2 = end;
			if ( off < off2 )
			{
			    //snprintf( dest, msgsize, "unused" );
			    //func(param,off,off2-off,msg);
			    off = off2;
			}
			start = sect;
		    }

		    while ( sect < WII_MAX_SECTORS && disc->usage_table[sect] )
			sect++;
		    if ( start < sect )
		    {
			u64 off2 = sect * (u64)WII_SECTOR_SIZE;
			if ( off2 > end )
			    off2 = end;
			if ( off < off2 )
			{
			    if ( fst_sect >= start && fst_sect < sect )
				snprintf( dest, msgsize, "data+fst, N(fst)=%u", part->fst_n );
			    else
				snprintf( dest, msgsize, "data+fst" );
			    func(param,off,off2-off,msg);
			    off = off2;
			}
		    }

		    if ( sect >= WII_MAX_SECTORS )
			break;
		}
	    }
	}
    }

    //----- end of wii disc

    if (disc->file_size)
	func(param,disc->file_size,0,
		"-- End of file/disc --");
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			     E N D			///////////////
///////////////////////////////////////////////////////////////////////////////
