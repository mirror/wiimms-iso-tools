
/***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project.                                 *
 *   Visit http://wit.wiimm.de/ for project details and sources.           *
 *                                                                         *
 *   Copyright (c) 2009 Kwiirk                                             *
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

#include "libwbfs.h"
#include <errno.h>

///////////////////////////////////////////////////////////////////////////////

#ifndef WIN32
 #define likely(x)	__builtin_expect(!!(x), 1)
 #define unlikely(x)	__builtin_expect(!!(x), 0)
#else
 #define likely(x)	(x)
 #define unlikely(x)	(x)
#endif

///////////////////////////////////////////////////////////////////////////////

#define WBFS_ERROR(x) do {wbfs_error(x);goto error;}while(0)
#define ALIGN_LBA(x) (((x)+p->hd_sec_sz-1)&(~(p->hd_sec_sz-1)))

///////////////////////////////////////////////////////////////////////////////
// debugging macros

#ifndef TRACE
 #define TRACE(...)
#endif

#ifndef ASSERT
 #define ASSERT(cond)
#endif

///////////////////////////////////////////////////////////////////////////////

char wbfs_slot_mode_info[WBFS_SLOT__MASK+1] = ".#!!" "-?!!" "-?!!" "-?!!";

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

be64_t wbfs_setup_inode_info
	( wbfs_t * p, wbfs_inode_info_t * ii, bool is_valid, int is_changed )
{
    ASSERT(p);
    ASSERT(p->head);
    ASSERT(ii);
    ASSERT( sizeof(wbfs_inode_info_t) == WBFS_INODE_INFO_SIZE );

    memcpy(ii,p->head,WBFS_INODE_INFO_HEAD_SIZE);
    ii->info_version = htonl(WBFS_INODE_INFO_VERSION);

    be64_t now = hton64(wbfs_time());

    if (is_changed)
	ii->ctime = ii->atime = now; 

    if (is_valid)
    {
	if ( !ntoh64(ii->itime) )
	    ii->itime = now;
	if ( !ntoh64(ii->mtime) )
	    ii->mtime = now;
	if ( !ntoh64(ii->ctime) )
	    ii->ctime = now;
	if ( !ntoh64(ii->atime) )
	    ii->atime = now;
	ii->dtime = 0;
    }
    else if ( !ntoh64(ii->dtime) )
	ii->dtime = now;

    return now;
}

///////////////////////////////////////////////////////////////////////////////

int wbfs_is_inode_info_valid ( wbfs_t * p, wbfs_inode_info_t * ii )
{
    ASSERT(p);
    ASSERT(p->head);
    ASSERT(ii);
    ASSERT( sizeof(wbfs_inode_info_t) == WBFS_INODE_INFO_SIZE );

    // if valid   -> return WBFS_INODE_INFO_VERSION
    // if invalid -> return 0

    const u32 version = ntohl(ii->info_version);
    return !memcmp(ii,p->head,WBFS_INODE_INFO_CMP_SIZE)
	&& version > 0
	&& version <= WBFS_INODE_INFO_VERSION
		? version : 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef WIT // not used in WiT

static int force_mode=0;

void wbfs_set_force_mode(int force)
{
    force_mode = force;
}

#endif // !WIT 
///////////////////////////////////////////////////////////////////////////////

static u8 size_to_shift(u32 size)
{
    u8 ret = 0;
    while (size)
    {
	ret++;
	size>>=1;
    }
    return ret-1;
}

///////////////////////////////////////////////////////////////////////////////
#ifndef WIT // not used in WiT

#define read_le32_unaligned(x) ((x)[0]|((x)[1]<<8)|((x)[2]<<16)|((x)[3]<<24))

void wbfs_sync(wbfs_t*p);

wbfs_t * wbfs_open_hd
(
	rw_sector_callback_t read_hdsector,
	rw_sector_callback_t write_hdsector,
	void *callback_data,
	int hd_sector_size,
	int num_hd_sector __attribute((unused)),
	int reset
)
{ // [codeview]

    int i
#ifdef UNUSED_STUFF
    = num_hd_sector
#endif
      , ret;
    u8 part_table[16*4];
    u8 *ptr, *tmp_buffer = wbfs_ioalloc(hd_sector_size);
    if (!tmp_buffer)
	OUT_OF_MEMORY;

    ret = read_hdsector(callback_data,0,1,tmp_buffer);
    if (ret)
	return 0;

    //find wbfs partition
    wbfs_memcpy(part_table,tmp_buffer+0x1be,16*4);
    ptr = part_table;
    for (i=0; i<4; i++,ptr+=16)
    {
	u32 part_lba = read_le32_unaligned(ptr+0x8);
	wbfs_head_t *head = (wbfs_head_t *)tmp_buffer;
#ifdef UNUSED_STUFF
	ret =
#endif
	    read_hdsector(callback_data,part_lba,1,tmp_buffer);
	// verify there is the magic.
	if (head->magic == wbfs_htonl(WBFS_MAGIC))
	{
	    wbfs_t *p = wbfs_open_partition(	read_hdsector,
					     write_hdsector,
					     callback_data,
					     hd_sector_size,
					     0,
					     part_lba,reset
					   );
	    return p;
	}
    }
    if (reset)// make a empty hd partition..
    {
    }
    return 0;
}

#endif // !WIT 
//
///////////////////////////////////////////////////////////////////////////////
#ifndef WIT // not used in WiT

wbfs_t * wbfs_open_partition
		( rw_sector_callback_t read_hdsector,
		  rw_sector_callback_t write_hdsector,
		  void *callback_data,
		  int hd_sector_size,
		  int num_hd_sector,
		  u32 part_lba,
		  int reset )
{
    wbfs_param_t par;
    memset(&par,0,sizeof(par));

    par.read_hdsector	= read_hdsector;
    par.write_hdsector	= write_hdsector;
    par.callback_data	= callback_data;
    par.hd_sector_size	= hd_sector_size;
    par.num_hd_sector	= num_hd_sector;
    par.part_lba	= part_lba;
    par.reset		= reset > 0;
    par.clear_inodes	= reset > 1;
    par.setup_iinfo	= reset > 2;
    par.force_mode	= force_mode > 0;   // global parameter

    return wbfs_open_partition_param(&par);
}

#endif // !WIT 
///////////////////////////////////////////////////////////////////////////////

wbfs_t * wbfs_open_partition_param ( wbfs_param_t * par )
{
    TRACE("LIBWBFS: +wbfs_open_partition_param()\n");
    ASSERT(par);

    //----- alloc mem

    int hd_sector_size	= par->hd_sector_size ? par->hd_sector_size : 512;
    wbfs_head_t *head	= wbfs_ioalloc(hd_sector_size);
    wbfs_t	*p	= wbfs_malloc(sizeof(wbfs_t));
    if ( !p || !head )
	OUT_OF_MEMORY;

    memset(p,0,sizeof(*p));

    //----- store head and parameters

    p->head		= head;
    p->part_lba		= par->part_lba;
    p->read_hdsector	= par->read_hdsector;
    p->write_hdsector	= par->write_hdsector;
    p->callback_data	= par->callback_data;

    //----- init/load partition header

    if ( par->reset > 0 )
    {
	// if reset: make some calculations and store the result in the header

	wbfs_memset(head,0,hd_sector_size);
	head->magic		= wbfs_htonl(WBFS_MAGIC);
	head->wbfs_version	= WBFS_VERSION;
	head->hd_sec_sz_s	= size_to_shift(hd_sector_size);
	hd_sector_size		= 1 << head->hd_sec_sz_s;
	head->n_hd_sec		= wbfs_htonl(par->num_hd_sector);

	if ( par->old_wii_sector_calc > 0 )
	{
	    //*** this is the old calculation with rounding error
	    p->n_wii_sec = ( par->num_hd_sector / WII_SECTOR_SIZE) * hd_sector_size;
	}
	else
	{
	    //*** but *here* we can/should use the correct one
	    p->n_wii_sec = par->num_hd_sector / ( WII_SECTOR_SIZE / hd_sector_size );
	}

	if ( par->wbfs_sector_size > 0 )
	    head->wbfs_sec_sz_s = size_to_shift(par->wbfs_sector_size);
	else
	    head->wbfs_sec_sz_s
		= wbfs_calc_size_shift( head->hd_sec_sz_s,
					par->num_hd_sector,
					par->old_wii_sector_calc );
    }
    else
    {
	// no reset: just load header

	p->read_hdsector(p->callback_data,p->part_lba,1,head);
    }

    //----- validation tests

    if ( head->magic != wbfs_htonl(WBFS_MAGIC) )
	WBFS_ERROR("bad magic");

    if ( par->force_mode <= 0 )
    {
	if ( hd_sector_size && head->hd_sec_sz_s != size_to_shift(hd_sector_size) )
	    WBFS_ERROR("hd sector size doesn't match");

	if ( par->num_hd_sector && head->n_hd_sec != wbfs_htonl(par->num_hd_sector) )
	    WBFS_ERROR("hd num sector doesn't match");
    }


    //----- setup wbfs geometry

    wbfs_calc_geometry(p,
		wbfs_ntohl(head->n_hd_sec),
		1 << head->hd_sec_sz_s,
		1 << head->wbfs_sec_sz_s );


    //----- load/setup free block table

    noTRACE("FB: size4=%x=%u, mask=%x=%u\n",
		p->freeblks_size4, p->freeblks_size4,
		p->freeblks_mask, p->freeblks_mask );

#if !NEW_WBFS_INTERFACE
    if ( par->reset <= 0 )
    {
	// table will alloc and read only if needed
	p->freeblks = 0;
    }
    else
    {
	const size_t fb_memsize = p->freeblks_lba_count * p->hd_sec_sz;

	// init with all free blocks
	p->freeblks = wbfs_ioalloc(fb_memsize);

	// fill complete array with zeros
	wbfs_memset(p->freeblks,0,fb_memsize);

	// fill used area with 0xff
	wbfs_memset(p->freeblks,0xff,p->freeblks_size4*4);

	// fix the last entry
	p->freeblks[p->freeblks_size4-1] = wbfs_htonl(p->freeblks_mask);
    }
#endif

    //----- etc

    p->tmp_buffer = wbfs_ioalloc(p->hd_sec_sz);
    if (!p->tmp_buffer)
	OUT_OF_MEMORY;

    if ( par->reset > 0 )
    {
	if ( par->setup_iinfo > 0 )
	{
	    // build an empty inode with a valid inode_info and write it to all slots.

	    const int disc_info_sz_lba =  p->disc_info_sz >> p->hd_sec_sz_s;
	    wbfs_disc_info_t * info = wbfs_ioalloc(p->disc_info_sz);
	    if (!info)
		OUT_OF_MEMORY;
	    memset(info,0,p->disc_info_sz);
	    wbfs_inode_info_t * iinfo = wbfs_get_inode_info(p,info,0);
	    wbfs_setup_inode_info(p,&par->iinfo,0,0);
	    memcpy(iinfo,&par->iinfo,sizeof(*iinfo));

	    int slot;
	    for ( slot = 0; slot < p->max_disc; slot++ )
	    {
		const int lba = p->part_lba + 1 + slot * disc_info_sz_lba;
		p->write_hdsector(p->callback_data,lba,disc_info_sz_lba,info);
	    }

	    wbfs_iofree(info);
	}
	else if ( par->clear_inodes > 0 )
	{
	    // clear all disc header, but we are nice to sparse files
	    //  -> read the data first and only if not zeroed we write back

	    const int disc_info_sz_lba =  p->disc_info_sz >> p->hd_sec_sz_s;
	    wbfs_disc_info_t * info = wbfs_ioalloc(p->disc_info_sz);
	    if (!info)
		OUT_OF_MEMORY;

	    int slot;
	    for ( slot = 0; slot < p->max_disc; slot++ )
	    {
		const int lba = p->part_lba + 1 + slot * disc_info_sz_lba;
		p->read_hdsector(p->callback_data,lba,disc_info_sz_lba,info);

		const u32 * ptr = (u32*)info;
		const u32 * end = ptr + p->disc_info_sz/sizeof(*ptr);
		while ( ptr < end )
		{
		    if (*ptr++)
		    {
			memset(info,0,p->disc_info_sz);
			p->write_hdsector(p->callback_data,lba,disc_info_sz_lba,info);
			break;
		    }
		    //TRACE("%p %p %p\n",info,ptr,end);
		}
	    }

	    wbfs_iofree(info);
	}
    }


#if NEW_WBFS_INTERFACE

    //---- check free blocks table and find invalid discs

    PRINT("NEW WBFS INTERFACE: +++ check free blocks table and find invalid discs\n");
    TRACE("NEW WBFS INTERFACE: +++ check free blocks table and find invalid discs\n");

    if (wbfs_calc_used_blocks(p,true,false))
	goto error;

 #ifdef DEBUG
    TRACE("NEW WBFS INTERFACE: --- free blocks table ready\n");
    PRINT("NEW WBFS INTERFACE: --- free blocks table ready\n");
    wbfs_print_block_usage(stdout,3,p,false);
 #endif

#endif // NEW_WBFS_INTERFACE


    //----- all done, terminate

    if ( par->reset > 0 )
    {
#if NEW_WBFS_INTERFACE
	p->is_dirty = p->used_block_dirty = true;
#else
	p->is_dirty = true;
#endif    
	wbfs_sync(p);
    }

    return p;


    //----- error handling

error:
    p->is_dirty = false;
    wbfs_close(p);
    wbfs_iofree(head);
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
// public calculation helpers

int wbfs_calc_size_shift
	( u32 hd_sec_sz_s, u32 num_hd_sector, int old_wii_sector_calc )
{
    const u32 hd_sector_size = 1 << hd_sec_sz_s;

    const u32 n_wii_sec =  old_wii_sector_calc > 0
		//*** this is the old calculation with rounding error
		? ( num_hd_sector / WII_SECTOR_SIZE) * hd_sector_size
		//*** but *here* we can/should use the correct one
		: num_hd_sector / ( WII_SECTOR_SIZE / hd_sector_size );

    // ensure that wbfs_sec_sz is big enough to address every blocks using 16 bits
    // choose minimum wblk_sz that fits this partition size
    // the max value chooses the maximal supported partition size
    //   - start_value < 6 ==> n_wbfs_sec_per_disc becomes to large

    DASSERT( ( 1 << WII_SECTOR_SIZE_SHIFT + WBFS_MIN_SECTOR_SHIFT ) == WBFS_MIN_SECTOR_SIZE );
    DASSERT( ( 1 << WII_SECTOR_SIZE_SHIFT + WBFS_MAX_SECTOR_SHIFT ) == WBFS_MAX_SECTOR_SIZE );

    int shift_count;
    for ( shift_count = WBFS_MIN_SECTOR_SHIFT;
	  shift_count <= WBFS_MAX_SECTOR_SHIFT;
	  shift_count++
	)
    {
	// ensure that wbfs_sec_sz is big enough to address every blocks using 16 bits
	if ( n_wii_sec < (u32)WBFS_MAX_SECT << shift_count )
	    break;
    }

    return shift_count + WII_SECTOR_SIZE_SHIFT;
}

///////////////////////////////////////////////////////////////////////////////

void wbfs_calc_geometry
(
	wbfs_t * p,		// pointer to wbfs_t, p->head must be NULL or valid
	u32 n_hd_sec,		// total number of hd_sec in partition
	u32 hd_sec_sz,		// size of a hd/partition sector
	u32 wbfs_sec_sz		// size of a wbfs sector
)
{
    ASSERT(p);

    //----- parameter calculations & corrections

    const u32 hd_sec_sz_s	= size_to_shift(hd_sec_sz);
    hd_sec_sz			= 1 << hd_sec_sz_s;

    const u32 wbfs_sec_sz_s	= size_to_shift(wbfs_sec_sz);
    wbfs_sec_sz			= 1 << wbfs_sec_sz_s;

    //----- setup header if not NULL

    if (p->head)
    {
	memset(p->head,0,sizeof(p->head));
	p->head->magic		= wbfs_htonl(WBFS_MAGIC);
	p->head->n_hd_sec	= wbfs_htonl(n_hd_sec);
	p->head->hd_sec_sz_s	= hd_sec_sz_s;
	p->head->wbfs_sec_sz_s	= wbfs_sec_sz_s;
	p->head->wbfs_version	= WBFS_VERSION;
    }

    //----- setup some wii constants

    p->wii_sec_sz		= WII_SECTOR_SIZE;
    p->wii_sec_sz_s		= WII_SECTOR_SIZE_SHIFT;
    p->n_wii_sec_per_disc	= WII_MAX_SECTORS;

    //----- transfer parameters

    p->hd_sec_sz_s		= hd_sec_sz_s;
    p->hd_sec_sz		= hd_sec_sz;
    p->n_hd_sec			= n_hd_sec;

    p->wbfs_sec_sz_s		= wbfs_sec_sz_s;
    p->wbfs_sec_sz		= wbfs_sec_sz;

    //----- base calculations

    // ************************************************************************
    // ***  This calculation has a rounding bug. But we must leave it here  ***
    // ***  because 'n_wbfs_sec' & 'freeblks_lba' are based in this value.  ***
    // ************************************************************************

    p->n_wii_sec	= ( p->n_hd_sec / p->wii_sec_sz ) * p->hd_sec_sz;
    p->n_wbfs_sec	= p->n_wii_sec >> ( p->wbfs_sec_sz_s - p->wii_sec_sz_s );
    p->n_wbfs_sec_per_disc
			= p->n_wii_sec_per_disc >> ( p->wbfs_sec_sz_s - p->wii_sec_sz_s );
    p->disc_info_sz	= ALIGN_LBA( sizeof(wbfs_disc_info_t) + p->n_wbfs_sec_per_disc*2 );

    //----- 'free blocks table' calculations

    const u32 fb_memsize= ALIGN_LBA(p->n_wbfs_sec/8);	// memory size of 'freeblks'
    p->freeblks_lba	= ( p->wbfs_sec_sz - p->n_wbfs_sec / 8 ) >> p->hd_sec_sz_s;
    p->freeblks_lba_count
			= fb_memsize >> p->hd_sec_sz_s;

    //----- here we correct the previous wrong calulations

    p->n_wii_sec	= p->n_hd_sec / ( p->wii_sec_sz / p->hd_sec_sz );
    const u32 n_wbfs_sec= p->n_wii_sec >> ( p->wbfs_sec_sz_s - p->wii_sec_sz_s );
    p->n_wbfs_sec	= n_wbfs_sec < WBFS_MAX_SECT ? n_wbfs_sec : WBFS_MAX_SECT;

    //----- calculate and fix the needed space for free blocks table

    p->freeblks_size4	= ( p->n_wbfs_sec-1 + 31 ) / 32;
    if ( p->freeblks_size4 > fb_memsize/4 )
    {
	// not enough memory to store complete free blocks table :(
	p->freeblks_size4 = fb_memsize/4;

	// fix the number of WBFS sectors too
	const u32 max_sec = p->freeblks_size4 * 32 + 1; // remember: 1 based
	if ( p->n_wbfs_sec > max_sec )
	     p->n_wbfs_sec = max_sec;
    }
    p->freeblks_mask	= ( 1ull << ( (p->n_wbfs_sec-1) & 31 )) - 1;
    if (!p->freeblks_mask)
	p->freeblks_mask = ~(u32)0;

    //----- calculate max_disc

    p->max_disc = ( p->freeblks_lba - 1 ) / ( p->disc_info_sz >> p->hd_sec_sz_s );
    if ( p->max_disc > p->hd_sec_sz - sizeof(wbfs_head_t) )
	 p->max_disc = p->hd_sec_sz - sizeof(wbfs_head_t);
}

//
///////////////////////////////////////////////////////////////////////////////

void wbfs_sync ( wbfs_t * p )
{
    // writes wbfs header and free blocks to hardisk

    TRACE("LIBWBFS: +wbfs_sync(%p) wr_hds=%p, head=%p, freeblks=%p\n",
		p, p->write_hdsector, p->head, p->freeblks );

    if (p->write_hdsector)
    {
	p->write_hdsector ( p->callback_data, p->part_lba, 1, p->head );

 #if NEW_WBFS_INTERFACE

	if ( p->used_block && p->used_block_dirty )
	{
	    wbfs_load_freeblocks(p);
	    if (p->freeblks)
		p->write_hdsector( p->callback_data,
				   p->part_lba + p->freeblks_lba,
				   p->freeblks_lba_count,
				   p->freeblks );
	    p->used_block_dirty = false;
	}
	
 #else
	if (p->freeblks)
	    p->write_hdsector ( p->callback_data,
				p->part_lba + p->freeblks_lba,
				p->freeblks_lba_count,
				p->freeblks );
 #endif

	p->is_dirty = false;
    }
}

///////////////////////////////////////////////////////////////////////////////

void wbfs_close ( wbfs_t * p )
{
    TRACE("LIBWBFS: +wbfs_close(%p) disc_open=%d, head=%p, freeblks=%p\n",
		p, p->n_disc_open, p->head, p->freeblks );

    if (p->n_disc_open)
	WBFS_ERROR("trying to close wbfs while discs still open");

    if (p->is_dirty)
	wbfs_sync(p);

    wbfs_free_freeblocks(p);
 #if NEW_WBFS_INTERFACE
    wbfs_iofree(p->block0);
    wbfs_free(p->used_block);
 #endif
    wbfs_free(p->id_list);
    wbfs_iofree(p->head);
    wbfs_iofree(p->tmp_buffer);
    wbfs_free(p);

 error:
    return;
}

///////////////////////////////////////////////////////////////////////////////

wbfs_disc_t * wbfs_open_disc_by_id6 ( wbfs_t* p, u8 * discid )
{
    ASSERT(p);
    ASSERT(discid);

    const int slot = wbfs_find_slot(p,discid);
    return slot < 0 ? 0 : wbfs_open_disc_by_slot(p,slot,0);
}

///////////////////////////////////////////////////////////////////////////////

static wbfs_disc_t * wbfs_open_disc_by_info
	( wbfs_t * p, u32 slot, wbfs_disc_info_t * info, int force_open )
{
    ASSERT(p);
    ASSERT( slot >= 0 );
    ASSERT(info);

    wbfs_disc_t * d = wbfs_malloc(sizeof(*d));
    if (!d)
	OUT_OF_MEMORY;
    d->p = p;
    d->slot = slot;
    d->header = info;

    d->is_used = p->head->disc_table[slot] != WBFS_SLOT_FREE;
    if ( *d->header->dhead )
    {
	d->disc_type = get_header_disc_type((wd_header_t*)d->header->dhead,&d->disc_attrib);
	const u32 wii_magic = be32(d->header->dhead+WII_MAGIC_OFF);
	const u32 gc_magic  = be32(d->header->dhead+GC_MAGIC_OFF);
	noTRACE("MAGIC: %08x %08x => %d %d\n",
		wii_magic, gc_magic, wii_magic == WII_MAGIC, gc_magic == GC_MAGIC ); 
	d->is_valid   = wii_magic == WII_MAGIC || gc_magic == GC_MAGIC;
	d->is_deleted = wii_magic == WII_MAGIC_DELETED;
    }
    else
	d->is_valid = d->is_deleted = false;

    if ( !force_open && !d->is_valid )
    {
	p->head->disc_table[slot] = WBFS_SLOT_FREE;
	wbfs_free(info);
	wbfs_free(d);
	return 0;
    }

    wbfs_get_disc_inode_info(d,1);
    p->n_disc_open++;
    return d;
}

///////////////////////////////////////////////////////////////////////////////

wbfs_disc_t * wbfs_open_disc_by_slot ( wbfs_t * p, u32 slot, int force_open )
{
    ASSERT(p);
    TRACE("LIBWBFS: wbfs_open_disc_by_slot(%p,slot=%u,force=%d) max=%u, disctab=%u\n",
		p, slot, p->max_disc, p->head->disc_table[slot], force_open );

    if ( slot >= p->max_disc || !p->head->disc_table[slot] && !force_open )
	return 0;

    wbfs_disc_info_t * info = wbfs_ioalloc(p->disc_info_sz);
    if (!info)
	OUT_OF_MEMORY;

    const u32 disc_info_sz_lba = p->disc_info_sz >> p->hd_sec_sz_s;
    if ( p->read_hdsector (
			p->callback_data,
			p->part_lba + 1 + slot * disc_info_sz_lba,
			disc_info_sz_lba,
			info ) )
    {
	wbfs_free(info);
	return 0;
    }

    return wbfs_open_disc_by_info(p,slot,info,force_open);
}

///////////////////////////////////////////////////////////////////////////////

wbfs_disc_t * wbfs_create_disc
(
    wbfs_t	* p,		// valid WBFS descriptor
    const void	* disc_header,	// NULL or disc header to copy
    const void	* disc_id	// NULL or ID6: check non existence
				// disc_id overwrites the id of disc_header
)
{
    ASSERT(p);

    if ( !disc_id && disc_header )
	disc_id = disc_header;

    if (disc_id)
    {
	int slot = wbfs_find_slot(p,disc_id);
	if ( slot >= 0 )
	{
	    wbfs_error("Disc with id '%s' already exists.",
		wd_print_id(disc_id,6,0));
	    return 0;
	}
    }

    //----- find a free slot

    int slot;
    for ( slot = 0; slot < p->max_disc; slot++ )
	if (!p->head->disc_table[slot])
	    break;

    if ( slot == p->max_disc )
    {
	wbfs_error("Limit of %u discs alreday reached.",p->max_disc);
	return 0;
    }


    //----- open slot

    p->head->disc_table[slot] = WBFS_SLOT_VALID;
    p->is_dirty = true;

    wbfs_disc_info_t * info = wbfs_ioalloc(p->disc_info_sz);
    if (!info)
	OUT_OF_MEMORY;
    memset(info,0,p->disc_info_sz);
    wd_header_t * dhead = (wd_header_t*)info->dhead;
    dhead->wii_magic = htonl(WII_MAGIC);

    if (disc_header)
	memcpy(info->dhead,disc_header,sizeof(info->dhead));

    if (disc_id)
	wd_patch_id(info->dhead,info->dhead,disc_id,6);

    wbfs_disc_t * disc = wbfs_open_disc_by_info(p,slot,info,true);
    if (disc)
	disc->is_creating = disc->is_dirty = true;
    return disc;
}

///////////////////////////////////////////////////////////////////////////////

int wbfs_sync_disc_header ( wbfs_disc_t * d )
{
    DASSERT(d);
    if ( !d || !d->p || !d->header )
	 return 1;

    d->is_dirty = false;
    wbfs_t * p = d->p;
    const u32 disc_info_sz_lba = p->disc_info_sz >> p->hd_sec_sz_s;
//HEXDUMP16(0,0x100,d->header->wlba_table,16);
    return p->write_hdsector (
			p->callback_data,
			p->part_lba + 1 + d->slot * disc_info_sz_lba,
			disc_info_sz_lba,
			d->header );
}

///////////////////////////////////////////////////////////////////////////////

void wbfs_close_disc ( wbfs_disc_t * d )
{
    ASSERT( d );
    ASSERT( d->header );
    ASSERT( d->p );
    ASSERT( d->p->n_disc_open > 0 );

    if (d->is_dirty)
	wbfs_sync_disc_header(d);

    d->p->n_disc_open--;
    wbfs_iofree(d->header);
    wbfs_free(d);
}

///////////////////////////////////////////////////////////////////////////////

wbfs_inode_info_t * wbfs_get_inode_info
	( wbfs_t *p, wbfs_disc_info_t * info, int clear_mode )
{
    ASSERT(p);
    ASSERT(info);
    wbfs_inode_info_t * iinfo 
	= (wbfs_inode_info_t*)(info->dhead + WBFS_INODE_INFO_OFF);

    if ( clear_mode>1 || clear_mode && !wbfs_is_inode_info_valid(p,iinfo) )
	memset(iinfo,0,sizeof(wbfs_inode_info_t));
    return iinfo;
}

///////////////////////////////////////////////////////////////////////////////

wbfs_inode_info_t * wbfs_get_disc_inode_info ( wbfs_disc_t * d, int clear_mode )
{
    ASSERT(d);
    wbfs_inode_info_t * iinfo =  wbfs_get_inode_info(d->p,d->header,clear_mode);
    d->is_iinfo_valid = wbfs_is_inode_info_valid(d->p,iinfo) != 0;
    return iinfo;
}

///////////////////////////////////////////////////////////////////////////////
// rename a disc

int wbfs_rename_disc
(
    wbfs_disc_t		* d,		// pointer to an open disc
    const char		* new_id,	// if !NULL: take the first 6 chars as ID
    const char		* new_title,	// if !NULL: take the first 0x39 chars as title
    int			chg_wbfs_head,	// if !0: change ID/title of WBFS header
    int			chg_iso_head	// if !0: change ID/title of ISO header
)
{
    ASSERT(d);
    ASSERT(d->p);
    ASSERT(d->header);

    wbfs_t * p = d->p;

    wbfs_inode_info_t * iinfo = wbfs_get_disc_inode_info(d,1);
    const be64_t now = wbfs_setup_inode_info(p,iinfo,d->is_valid,0);

    int do_sync = 0;
    if ( chg_wbfs_head
	&& wd_rename(d->header->dhead,new_id,new_title) )
    {
	iinfo->ctime = iinfo->atime = now;
	do_sync++;
    }

    if ( chg_iso_head )
    {
	u16 wlba = ntohs(d->header->wlba_table[0]);
	if (wlba)
	{
	    u8 * tmpbuf = p->tmp_buffer;
	    ASSERT(tmpbuf);
	    const u32 lba = wlba << ( p->wbfs_sec_sz_s - p->hd_sec_sz_s );
	    int err = p->read_hdsector( p->callback_data, lba, 1, tmpbuf );
	    if (err)
		return err;
	    if (wd_rename(tmpbuf,new_id,new_title))
	    {
		iinfo->mtime = iinfo->ctime = iinfo->atime = now;
		err = p->write_hdsector( p->callback_data, lba, 1, tmpbuf );
		if (err)
		    return err;
		do_sync++;
	    }
	}
    }

    if (do_sync)
    {
	TRACE("wbfs_rename_disc() now=%llu i=%llu m=%llu c=%llu a=%llu d=%llu\n",
		ntoh64(now),
		ntoh64(iinfo->itime),
		ntoh64(iinfo->mtime),
		ntoh64(iinfo->ctime),
		ntoh64(iinfo->atime),
		ntoh64(iinfo->dtime));

	const int err = wbfs_sync_disc_header(d);
	if (err)
	    return err;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int wbfs_touch_disc
(
    wbfs_disc_t		* d,		// pointer to an open disc
    u64			itime,		// if != 0: new itime
    u64			mtime,		// if != 0: new mtime
    u64			ctime,		// if != 0: new ctime
    u64			atime		// if != 0: new atime
)
{
    ASSERT(d);
    ASSERT(d->p);
    ASSERT(d->header);

    TRACE("wbfs_touch_disc(%p,%llu,%llu,%llu,%llu)\n",d,itime,mtime,ctime,atime);

    wbfs_inode_info_t * iinfo = wbfs_get_disc_inode_info(d,1);
    wbfs_setup_inode_info(d->p,iinfo,d->is_valid,0);

    if (itime)
	iinfo->itime = hton64(itime);
    if (mtime)
	iinfo->mtime = hton64(mtime);
    if (ctime)
	iinfo->ctime = hton64(ctime);
    if (atime)
	iinfo->atime = hton64(atime);

    return wbfs_sync_disc_header(d);
}

///////////////////////////////////////////////////////////////////////////////

void wbfs_print_block_usage
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    const wbfs_t	* p,		// valid WBFS descriptor
    bool		print_all	// false: ignore const lines
)
{
    DASSERT(p);
 #if NEW_WBFS_INTERFACE
    DASSERT(p->used_block);
    wbfs_print_usage_tab( f, indent, p->used_block, p->n_wbfs_sec,
				p->wbfs_sec_sz, print_all );
 #endif
}

///////////////////////////////////////////////////////////////////////////////

const char wbfs_usage_name_tab[256] =
{
	".#23456789ABCDEFGHIJKLMNOPQRSTUV"
	"WXYZabcdefghijklmnopqrstuvwxyz++"
	"++++++++++++++++++++++++++++++++"
	"+++++++++++++++++++++++++++++++*"

	"0123456789ABCDEFGHIJKLMNOPQRSTUV"
	"WXYZabcdefghijklmnopqrstuvwxyz++"
	"++++++++++++++++++++++++++++++++"
	"+++++++++++++++++++++++++++++++$"
};

//-----------------------------------------------------------------------------

void wbfs_print_usage_tab
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    const u8		* used_block,	// valid pointer to usage table
    u32			block_used_sz,	// size of 'used_block'
    u32			sector_size,	// wbfs sector size
    bool		print_all	// false: ignore const lines
)
{
    wd_print_byte_tab( f, indent, used_block, block_used_sz, block_used_sz,
			sector_size, wbfs_usage_name_tab, print_all );
}

///////////////////////////////////////////////////////////////////////////////

// offset is pointing 32bit words to address the whole dvd, although len is in bytes

int wbfs_disc_read ( wbfs_disc_t *d, u32 offset, u8 *data, u32 len )
{ // [codeview]

    wbfs_t *p = d->p;
    u16 wlba = offset>>(p->wbfs_sec_sz_s-2);
    u32 iwlba_shift = p->wbfs_sec_sz_s - p->hd_sec_sz_s;
    u32 lba_mask = (p->wbfs_sec_sz-1)>>(p->hd_sec_sz_s);
    u32 lba = (offset>>(p->hd_sec_sz_s-2))&lba_mask;
    u32 off = offset&((p->hd_sec_sz>>2)-1);
    u16 iwlba = wbfs_ntohs(d->header->wlba_table[wlba]);
    u32 len_copied;
    int err = 0;
    u8  *ptr = data;
    if (unlikely(iwlba==0))
	return 1;
    if (unlikely(off))
    {
	off *= 4;
	err = p->read_hdsector(p->callback_data,
			       p->part_lba + (iwlba<<iwlba_shift) + lba, 1, p->tmp_buffer);
	if (err)
	    return err;
	len_copied = p->hd_sec_sz - off;
	if (likely(len < len_copied))
	    len_copied = len;
	wbfs_memcpy(ptr, p->tmp_buffer + off, len_copied);
	len -= len_copied;
	ptr += len_copied;
	lba++;
	if (unlikely(lba>lba_mask && len))
	{
	    lba = 0;
	    iwlba = wbfs_ntohs(d->header->wlba_table[++wlba]);
	    if (unlikely(iwlba==0))
		return 1;
	}
    }
    while (likely(len>=p->hd_sec_sz))
    {
	u32 nlb = len>>(p->hd_sec_sz_s);

	if (unlikely(lba + nlb > p->wbfs_sec_sz)) // dont cross wbfs sectors..
	    nlb = p->wbfs_sec_sz-lba;
	err = p->read_hdsector(p->callback_data,
			       p->part_lba + (iwlba<<iwlba_shift) + lba, nlb, ptr);
	if (err)
	    return err;
	len -= nlb<<p->hd_sec_sz_s;
	ptr += nlb<<p->hd_sec_sz_s;
	lba += nlb;
	if (unlikely(lba>lba_mask && len))
	{
	    lba = 0;
	    iwlba =wbfs_ntohs(d->header->wlba_table[++wlba]);
	    if (unlikely(iwlba==0))
		return 1;
	}
    }
    if (unlikely(len))
    {
	err = p->read_hdsector(p->callback_data,
			       p->part_lba + (iwlba<<iwlba_shift) + lba, 1, p->tmp_buffer);
	if (err)
	    return err;
	wbfs_memcpy(ptr, p->tmp_buffer, len);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

u32 wbfs_count_discs ( wbfs_t * p )
{
    u32 i,count=0;
    for ( i = 0; i < p->max_disc; i++ )
	if (p->head->disc_table[i])
	    count++;
    return count;
}

///////////////////////////////////////////////////////////////////////////////

static u32 wbfs_sector_used ( wbfs_t * p, wbfs_disc_info_t * di )
{
    u32 tot_blk = 0, j;
    for ( j = 0; j < p->n_wbfs_sec_per_disc; j++ )
	if (wbfs_ntohs(di->wlba_table[j]))
	    tot_blk++;
    return tot_blk;
}

///////////////////////////////////////////////////////////////////////////////

enumError wbfs_get_disc_info
(
    wbfs_t		* p,		// valid wbfs descriptor
    u32			index,		// disc index: 0 .. num_dics-1
    u8			* header,	// header to store data
    int			header_size,	// size of 'header'
    u32			* slot_found,	// not NULL: store slot of found disc
    wd_disc_type_t	* disc_type,	// not NULL: store disc type
    wd_disc_attrib_t	* disc_attrib,	// not NULL: store disc attrib
    u32			* size4		// not NULL: store 'size>>2' of found disc
)
{
    u32 slot, count = 0;
    for( slot = 0; slot < p->max_disc; slot++ )
	if (p->head->disc_table[slot])
	    if ( count++ == index )
	    {
		if (slot_found)
		    *slot_found = slot;
		return wbfs_get_disc_info_by_slot(p,slot,header,header_size,
						disc_type,disc_attrib,size4);
	    }
    return ERR_WDISC_NOT_FOUND;
}

///////////////////////////////////////////////////////////////////////////////

enumError wbfs_get_disc_info_by_slot
(
    wbfs_t		* p,		// valid wbfs descriptor
    u32			slot,		// disc index: 0 .. num_dics-1
    u8			* header,	// not NULL: header to store data
    int			header_size,	// size of 'header'
    wd_disc_type_t	* disc_type,	// not NULL: store disc type
    wd_disc_attrib_t	* disc_attrib,	// not NULL: store disc attrib
    u32			* size4		// not NULL: store 'size>>2' of found disc
)
{
    ASSERT(p);
    if ( slot >= p->max_disc || !p->head->disc_table[slot] )
	return ERR_WDISC_NOT_FOUND;

    const u32 disc_info_sz_lba = p->disc_info_sz >> p->hd_sec_sz_s;
    p->read_hdsector(	p->callback_data,
			p->part_lba + 1 + slot * disc_info_sz_lba,
			1,
			p->tmp_buffer );


    const u32 wii_magic = be32(p->tmp_buffer+WII_MAGIC_OFF);
    const u32 gc_magic  = be32(p->tmp_buffer+GC_MAGIC_OFF);
    TRACE("MAGIC: %08x %08x => %d %d\n",
		wii_magic, gc_magic, wii_magic == WII_MAGIC, gc_magic == GC_MAGIC ); 

    if ( wii_magic != WII_MAGIC && gc_magic != GC_MAGIC )
    {
	p->head->disc_table[slot] = WBFS_SLOT_FREE;
	return ERR_WARNING;
    }

    if (header)
    {
	if (header_size > (int)p->hd_sec_sz)
	    header_size = p->hd_sec_sz;
	memcpy( header, p->tmp_buffer, header_size );
    }

    if ( disc_type || disc_attrib )
    {
	const wd_disc_type_t dt
	    = get_header_disc_type((wd_header_t*)p->tmp_buffer,disc_attrib);
	if (disc_type)
	    *disc_type = dt;
    }

    if (size4)
    {
	u32 sec_used;
	u8 * header = wbfs_ioalloc(p->disc_info_sz);

	p->read_hdsector (  p->callback_data,
			    p->part_lba + 1 + slot * disc_info_sz_lba,
			    disc_info_sz_lba,
			    header );

	sec_used = wbfs_sector_used(p,(wbfs_disc_info_t *)header);
	wbfs_iofree(header);
	*size4 = sec_used << (p->wbfs_sec_sz_s-2);
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

id6_t * wbfs_load_id_list ( wbfs_t * p, int force_reload )
{
    ASSERT(p);
    ASSERT(p->head);
    ASSERT(p->tmp_buffer);
    if ( p->id_list && !force_reload )
	return p->id_list;

    const int id_item_size = sizeof(*p->id_list);
    const int id_list_size = (p->max_disc+1) * id_item_size;
    const u32 disc_info_sz_lba = p->disc_info_sz >> p->hd_sec_sz_s;

    TRACE("LIBWBFS: +wbfs_load_id_list(%p,%d) id_list_size=%u*%u=%d\n",
		p, force_reload, p->max_disc, id_item_size, id_list_size );

    if (!p->id_list)
    {
	TRACE("MALLOC id_list = %d * (%d+1) = %d\n",
		id_item_size, p->max_disc, id_list_size );
	p->id_list = wbfs_malloc(id_list_size);
	if (!p->id_list)
	    OUT_OF_MEMORY;
    }
    memset(p->id_list,0,id_list_size);

    u32 slot;
    for ( slot = 0; slot < p->max_disc; slot++ )
	if (p->head->disc_table[slot])
	{
	    p->read_hdsector(p->callback_data,
			     p->part_lba + 1 + slot * disc_info_sz_lba,
			     1, p->tmp_buffer );
	    TRACE(" - slot #%03u: %.6s\n",slot,p->tmp_buffer);
	    memcpy(p->id_list[slot],p->tmp_buffer,id_item_size);
	}

    return p->id_list;
}

//
///////////////////////////////////////////////////////////////////////////////

int wbfs_find_slot ( wbfs_t * p, const u8 * disc_id )
{
    ASSERT(p);
    TRACE("LIBWBFS: +wbfs_find_slot(%p,%.6s)\n",p,disc_id);

    wbfs_load_id_list(p,0);
    ASSERT(p->id_list);

    const int id_item_size = sizeof(*p->id_list);

    u32 slot;
    for ( slot = 0; slot < p->max_disc; slot++ )
	if ( p->head->disc_table[slot]
		&& !memcmp(p->id_list[slot],disc_id,id_item_size) )
    {
	TRACE("LIBWBFS: -wbfs_find_slot() slot=%u\n",slot);
	return slot;
    }

    TRACE("LIBWBFS: -wbfs_find_slot() ID_NOT_FOUND\n");
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

u32 * wbfs_free_freeblocks ( wbfs_t * p )
{
    DASSERT(p);

 #if NEW_WBFS_INTERFACE
    if (p->block0)
    {
	if ( p->freeblks && ( (u8*)p->freeblks < p->block0
			   || (u8*)p->freeblks > p->block0 + p->wbfs_sec_sz ))
	{
	    wbfs_iofree(p->freeblks);
	}
	p->freeblks = (u32*)( p->block0
				+ ( p->part_lba + p->freeblks_lba ) * p->hd_sec_sz );
	return p->freeblks;
    }
 #endif // NEW_WBFS_INTERFACE

    if (p->freeblks)
    {
	wbfs_iofree(p->freeblks);
	p->freeblks = 0;
    }
    
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

u32 * wbfs_load_freeblocks ( wbfs_t * p )
{
    DASSERT(p);

 #if NEW_WBFS_INTERFACE

    if (p->block0)
	wbfs_free_freeblocks(p); // setup 'freeblks' pointer from block0

    bool dirty = p->used_block_dirty;
    const size_t fb_memsize = p->freeblks_lba_count * p->hd_sec_sz;
    if ( !p->freeblks && fb_memsize )
    {
	p->freeblks = wbfs_ioalloc(fb_memsize);
	dirty = true;
    }

    if ( p->freeblks && dirty )
    {
	PRINT("WBFS: CALC FBT\n");

	// fill complete array with zeros == mark all blocks as used
	wbfs_memset(p->freeblks,0,fb_memsize);

	u32 idx = 0, mask = 0;
	u8 *ptr, *end = p->used_block + p->n_wbfs_sec;
	for ( ptr = p->used_block + 1; ptr < end; ptr++ )
	    if (!*ptr)
	    {
		const u32 bl = ptr - p->used_block - 1;
		const u32 new_idx = bl / 32;
		if ( idx != new_idx )
		{
		    if ( idx >= p->freeblks_size4 )
			break; // end of free blocks table reached
		    p->freeblks[idx] = htonl(mask);
		    idx = new_idx;
		    mask = 0;
		}
		mask |= (u32)1 << ( bl & 31 );
	    }
	if ( idx < p->freeblks_size4 )
	    p->freeblks[idx] = htonl(mask);
    }

    return p->freeblks;

 #else // !NEW_WBFS_INTERFACE

    if ( !p->freeblks && p->freeblks_lba_count )
    {
	p->freeblks = wbfs_ioalloc( p->freeblks_lba_count * p->hd_sec_sz );
	if (!p->freeblks)
	    OUT_OF_MEMORY;

	p->read_hdsector( p->callback_data,
			  p->part_lba + p->freeblks_lba,
			  p->freeblks_lba_count,
			  p->freeblks );

	// fix the last entry
	p->freeblks[p->freeblks_size4-1] &= wbfs_htonl(p->freeblks_mask);
    }

    return p->freeblks;

 #endif
}

///////////////////////////////////////////////////////////////////////////////
#if NEW_WBFS_INTERFACE

int wbfs_calc_used_blocks
(
    wbfs_t	* p,		// valid WBFS descriptor
    bool	force_reload,	// true: definitely reload block #0
    bool	store_block0	// true: don't free block0
)
{
    DASSERT(p);
    
    //------ setup

    u8 * block0;
    if (p->block0)
    {
	block0 = p->block0;
	store_block0 = true;
    }
    else
    {
	block0 = wbfs_ioalloc(p->wbfs_sec_sz);
	force_reload = true;
    }

    const size_t used_size = p->n_wbfs_sec + 32;
    if (!p->used_block)
	p->used_block = malloc(used_size);
    u8 * used = p->used_block;

    const size_t id_list_size = (p->max_disc+1) * sizeof(*p->id_list);
    if (!p->id_list)
	p->id_list = wbfs_malloc(id_list_size);

    if ( !block0 || !used || !p->id_list )
	OUT_OF_MEMORY;

    memset(used,0,used_size);
    memset(p->id_list,0,id_list_size);

    p->new_slot_err = p->all_slot_err = 0;


    //----- read block #0

    if (force_reload)
    {
	PRINT("READ BLOCK0\n");
	int read_stat = p->read_hdsector( p->callback_data,
					p->part_lba,
					p->wbfs_sec_sz / p->hd_sec_sz,
					block0 );
	if (read_stat)
	{
	    if (!store_block0)
		wbfs_iofree(block0);
	    return read_stat;
	}
    }


    //----- scan free blocks table and mark free blocks with 0x80

    u32 * fbt0 = (u32*)( block0 + ( p->part_lba + p->freeblks_lba ) * p->hd_sec_sz );
    u32 * fbt = fbt0;
    PRINT("block0=%p..%p, fbt=%p [%zx]\n",
		block0,block0+p->wbfs_sec_sz,fbt,(ccp)fbt-(ccp)block0);
    u8 * dest = used + 1;
    u32 i;
    for ( i = 0; i < p->freeblks_size4; i++ )
    {
	DASSERT( dest - used <= used_size - 32 );
	u32 v = wbfs_ntohl(*fbt++);
	noPRINT("  %05x,%08x -> %04zx\n",i,v,dest-used);
	if ( v == 0 )
	{
	    // 32 used blocks
	    dest += 32;
	}
	else if ( v == ~(u32)0 )
	{
	    // 32 free blocks
	    memset(dest,0x80,32);
	    dest += 32;
	}
	else
	{
	    u32 j;
	    for ( j = 0; j < 32; j++, v >>= 1 )
		*dest++ = (v&1) ? 0x80 : 0x00;
	}
    }


    //----- scan discs, pass 1/2

    const bool valid_slot_info = p->head->wbfs_version > 1;
    p->head->wbfs_version = WBFS_VERSION;

    int slot;
    wbfs_disc_info_t * info = (wbfs_disc_info_t*)( block0 + p->hd_sec_sz );

    for ( slot = 0; slot < p->max_disc; slot++ )
    {
	DASSERT( (u8*)info + p->disc_info_sz < block0 + p->wbfs_sec_sz );
	u8 slot_info = p->head->disc_table[slot]; // [dt]
	if (slot_info)
	{
	    if (valid_slot_info)
		p->all_slot_err |= slot_info & ~WBFS_SLOT_VALID;
	    else
		slot_info = WBFS_SLOT_VALID;

	    memcpy(p->id_list[slot],info,6);
	    noPRINT("NEW WBFS INTERFACE: check slot %u: stat=%x, off=%zx, id=%s\n",
			slot, slot_info, (u8*)info - block0,
			wd_print_id(info,6,0) );

	    u16 * wlba_tab = info->wlba_table;
//HEXDUMP16(0,0x100,wlba_tab,16);
	    int bl, bl_count = 0;
	    for ( bl = 0; bl < p->n_wbfs_sec_per_disc; bl++ )
	    {
		const u32 wlba = wbfs_ntohs(wlba_tab[bl]);
		if ( wlba >= p->n_wbfs_sec )
		{
		    PRINT("!!! NEW WBFS INTERFACE: invalid slot %u.%u\n",slot,bl);
		    slot_info = slot_info & ~WBFS_SLOT_VALID | WBFS_SLOT_INVALID;
		    p->new_slot_err |= WBFS_SLOT_INVALID;
		}
		else if ( wlba > 0 )
		{
		    bl_count++;
		    
		    if ( used[wlba] & 0x80 )
		    {
			PRINT("!!! NEW WBFS INTERFACE: slot %u.%x used free block #%x [%02x]\n",
				slot, bl, wlba,  used[wlba] );
			slot_info |= WBFS_SLOT_F_FREED;
			p->new_slot_err |= WBFS_SLOT_F_FREED;
		    }

		    if ( ( used[wlba] & 0x7f ) < 0x7f )
			used[wlba]++;
		}
	    }

	    if (!bl_count)
	    {
		PRINT("!!! NEW WBFS INTERFACE: disc @ slot %u does'n have any block\n",slot);
		slot_info |= WBFS_SLOT_INVALID;
		p->new_slot_err |= WBFS_SLOT_INVALID;
	    }

	    p->head->disc_table[slot] = slot_info;
	}
	info = (wbfs_disc_info_t*)( (u8*)info + p->disc_info_sz );
    }


    //----- normalize 'used'

    fbt = fbt0;
    u32 *fbt_end = fbt + p->freeblks_size4;
    memset(fbt,0,p->freeblks_size4*4);
    u32 v = 0;
    int count = 32;
    bool pass2_needed = false;

    for ( i = 1; i < p->n_wbfs_sec; i++ )
    {
	const u8 ucnt = used[i] &= 0x7f;
	if (!ucnt)
	    v |= 1 << (i-1&31);
	else if ( ucnt > 1 )
	    pass2_needed = true;

	if ( !--count && fbt < fbt_end )
	{
	    noPRINT("FBT[%04x] = %08x\n",4*(int)(fbt-fbt0),v);
	    *fbt++ = wbfs_htonl(v);
	    v = 0;
	    count = 32;
	}
    }

    if ( fbt < fbt_end )
    {
	noPRINT("FBT[%04x] = %08x [END]\n",4*(int)(fbt-fbt0),v);
	*fbt = wbfs_htonl(v);
    }
	
    used[0] = 0xff;
    //HEXDUMP16(0,0,fbt0,p->freeblks_size4*4);


    //----- scan discs, pass 2/2

    if ( pass2_needed )
    {
	info = (wbfs_disc_info_t*)( block0 + p->hd_sec_sz );
	for ( slot = 0; slot < p->max_disc; slot++ )
	{
	    DASSERT( (u8*)info + p->disc_info_sz < block0 + p->wbfs_sec_sz );
	    u8 slot_info = p->head->disc_table[slot];
	    if (slot_info)
	    {
		u16 * wlba_tab = info->wlba_table;
		int bl;
		for ( bl = 0; bl < p->n_wbfs_sec_per_disc; bl++ )
		{
		    const u32 wlba = wbfs_ntohs(wlba_tab[bl]);
		    if ( wlba > 0 && wlba < p->n_wbfs_sec && used[wlba] > 1 )
		    {
			PRINT("!!! NEW WBFS INTERFACE: slot %u.%x shares block #%x [%02x]\n",
				slot, bl, wlba, used[wlba] );
			slot_info |= WBFS_SLOT_F_SHARED;
			p->new_slot_err |= WBFS_SLOT_F_SHARED;
		    }
		}
	    }
	    info = (wbfs_disc_info_t*)( (u8*)info + p->disc_info_sz );
	}
    }


    //----- terminate

    
    if (store_block0)
    {
	p->block0 = block0;
	wbfs_load_freeblocks(p);
    }
    else
	wbfs_iofree(block0);

    p->all_slot_err |= p->new_slot_err;
    PRINT("SLOT-STAT: %02x/%02x\n",p->new_slot_err,p->all_slot_err);
    return 0;    
}

#endif // NEW_WBFS_INTERFACE
///////////////////////////////////////////////////////////////////////////////
#if NEW_WBFS_INTERFACE

u32 wbfs_find_free_blocks
(
    // returns index of first free block or WBFS_NO_BLOCK if not enough blocks free

    wbfs_t	* p,		// valid WBFS descriptor
    u32		n_needed	// number of needed blocks
)
{
    DASSERT(p);
    DASSERT(p->used_block);
    DASSERT(n_needed);

    u8 *p1, *p2, *end = p->used_block + p->n_wbfs_sec;
    for ( p1 = p->used_block + 1; p1 < end && *p1; p1++ )
	;

    int count = n_needed;
    for ( p2 = p1; p2 < end; p2++ )
	if ( !*p2 && --count <= 0 )
	    break;

    if ( count > 0 )
	return WBFS_NO_BLOCK;

    PRINT("found: %5zd..%5zd [%5zd]\n",p1-p->used_block,p2-p->used_block,p2-p1);
    u8 *found = p1;
    u32 range = p2 - p1;

    while ( range >= n_needed )
    {
	for ( p1++; p1 < end && *p1; p1++ )
	    ;
	for ( p2++; p2 < end && *p2; p2++ )
	    ;
	if ( p2 >= end )
	break;

	if ( p2 - p1 < range )
	{
	    PRINT("found: %5zd..%5zd [%5zd]\n",p1-p->used_block,p2-p->used_block,p2-p1);
	    found = p1;
	    range = p2 - p1;
	}
    }

    return found - p->used_block;
}

#endif // NEW_WBFS_INTERFACE
///////////////////////////////////////////////////////////////////////////////

u32 wbfs_get_free_block_count ( wbfs_t * p )
{
 #if NEW_WBFS_INTERFACE

    DASSERT(p);
    DASSERT(p->used_block);

    u32 count = 0;
    u8 *ptr, *end = p->used_block + p->n_wbfs_sec;
    for ( ptr = p->used_block + 1; ptr < end; ptr++ )
	if (!*ptr)
	    count++;
    return count;

 #else

    u32 i, j, count = 0;

    wbfs_load_freeblocks(p);

    for ( i = 0; i < p->freeblks_size4; i++ )
    {
	u32 v = wbfs_ntohl(p->freeblks[i]);
	if (v == ~(u32)0)
	    count+=32;
	else if (v!=0)
	    for (j=0; j<32; j++)
		if (v & (1<<j))
		    count++;
    }
    return count;

 #endif
}

///////////////////////////////////////////////////////////////////////////////
// [2do] [obsolete] : replace it by direct call to wd_is_block_used()

static bool old_is_block_used
(
    const u8		* used,		// valid pointer to usage table
    u32			i,		// index of block
    u32			wblk_sz		// if >1: number of sectors per block
)
{
    u32 k;
    i *= wblk_sz;
    for ( k = 0; k < wblk_sz; k++ )
	if ( i + k < WII_MAX_SECTORS && used[i+k] )
	    return 1;
    return 0;
}

static bool is_block_used
(
    const u8		* usage_table,	// valid pointer to usage table
    u32			block_index,	// index of block
    u32			block_size	// if >1: number of sectors per block
)
{
    const bool r1 = old_is_block_used(usage_table,block_index,block_size);
    const bool r2 = wd_is_block_used(usage_table,block_index,block_size);
    ASSERT(r1==r2);
    return r2;
}

///////////////////////////////////////////////////////////////////////////////

u32 wbfs_alloc_block
(
    wbfs_t		* p,		// valid WBFS descriptor
    u32			start_block	// >0: start search at this block
)
{
 #if NEW_WBFS_INTERFACE

    DASSERT(p);
    DASSERT(p->used_block);

    if ( start_block < 1 || start_block >= p->n_wbfs_sec )
	 start_block = 1;

    u32 bl = start_block;
    do
    {
	if (!p->used_block[bl])
	{
	    p->used_block[bl] = 1;
	    p->used_block_dirty = p->is_dirty = true;
	    noPRINT("wbfs_alloc_block(%p,%u) -> %d\n",p,start_block,bl);
	    return bl;
	}
	
	if ( ++bl >= p->n_wbfs_sec )
	    bl = 1;

    } while ( bl != start_block );

    return WBFS_NO_BLOCK;
    
 #else

    u32 i;
    for ( i = 0; i < p->freeblks_size4; i++ )
    {
	u32 v = wbfs_ntohl(p->freeblks[i]);
	if ( v != 0 )
	{
	    u32 j;
	    for ( j = 0; j < 32; j++ )
		if ( v & 1<<j )
		{
		    p->is_dirty = true;
		    p->freeblks[i] = wbfs_htonl( v & ~(1<<j) );
		    noPRINT("wbfs_alloc_block(%p,%u) -> %d\n",p,start_block,i*32 + j + 1);
		    return i*32 + j + 1;
		}
	}
    }
    return WBFS_NO_BLOCK;

 #endif
}

///////////////////////////////////////////////////////////////////////////////

static u32 find_last_used_block ( wbfs_t * p )
{
 #if NEW_WBFS_INTERFACE

    DASSERT(p);
    DASSERT(p->used_block);
    ASSERT( p->used_block[0] == 0xff );

    u8 * ptr = p->used_block + p->n_wbfs_sec - 1;
    while (!*ptr)
	ptr--;
    return ptr - p->used_block;

 #else

    int i;
    for ( i = p->freeblks_size4 - 1; i >= 0; i-- )
    {
	u32 v = wbfs_ntohl(p->freeblks[i]);
	if ( i == p->freeblks_size4 - 1 )
	    v |= ~p->freeblks_mask;

	if ( v != ~(u32)0 )
	{
	    int j;
	    for ( j = 31; j >= 0; j-- )
		if ( !(v & 1<<j) )
		    return i*32 + j + 1;
	}
    }
    return 0;

 #endif
}

///////////////////////////////////////////////////////////////////////////////

void wbfs_free_block ( wbfs_t *p, u32 bl )
{
 #if NEW_WBFS_INTERFACE

    DASSERT(p);
    DASSERT(p->used_block);

    if ( bl > 0 && bl < p->n_wbfs_sec && p->used_block[bl] )
    {
	p->used_block[bl] = 0;
	p->used_block_dirty = p->is_dirty = true;
    }

 #else

    noTRACE("wbfs_free_block(%u)\n",bl);
    if ( bl > 0 && bl < p->n_wbfs_sec )
    {
	const u32 i = (bl-1) / 32;
	const u32 j = (bl-1) & 31;
	const u32 v = wbfs_ntohl(p->freeblks[i]);
	const u32 w = v | 1 << j;
	if ( w != v )
	{
	    p->freeblks[i] = wbfs_htonl(w);
	    p->is_dirty = true;
	    noTRACE(" i=%u j=%u v=%x -> %x\n",i,j,v,p->freeblks[i]);
	}
    }

 #endif
}

///////////////////////////////////////////////////////////////////////////////

void wbfs_use_block ( wbfs_t *p, u32 bl )
{
 #if NEW_WBFS_INTERFACE

    DASSERT(p);
    DASSERT(p->used_block);

    if ( bl > 0 && bl < p->n_wbfs_sec && !p->used_block[bl] )
    {
	p->used_block[bl] = 1;
	p->used_block_dirty = p->is_dirty = true;
    }

 #else

    if ( bl > 0 && bl < p->n_wbfs_sec )
    {
	const u32 i = (bl-1) / 32;
	const u32 j = (bl-1) & 31;
	const u32 v = wbfs_ntohl(p->freeblks[i]);
	p->freeblks[i] = wbfs_htonl( v & ~(1<<j) );
    }

 #endif
}

//
///////////////////////////////////////////////////////////////////////////////

u32 wbfs_add_disc
(
    wbfs_t			*p,
    wd_read_func_t		read_src_wii_disc,
    void			*callback_data,
    progress_callback_t		spinner,
    const wd_select_t		* psel,
    int				copy_1_1
)
{
    wbfs_param_t par;
    memset(&par,0,sizeof(par));

    par.read_src_wii_disc	= read_src_wii_disc;
    par.callback_data		= callback_data;
    par.spinner			= spinner;
    par.psel			= psel;
    
    wd_select_t select_whole;
    if (copy_1_1)
    {
	wd_initialize_select(&select_whole);
	select_whole.whole_disc = true;
	par.psel = &select_whole;
    }

    const u32 stat = wbfs_add_disc_param(p,&par);
    if (par.open_disc)
	wbfs_close_disc(par.open_disc);
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

u32 wbfs_add_disc_param ( wbfs_t *p, wbfs_param_t * par )
{
    ASSERT(p);
    ASSERT(par);

    par->slot = -1; // no slot assigned
    par->open_disc = 0;

    int i, slot;
    u32 wii_sec_per_wbfs_sect = 1 << (p->wbfs_sec_sz_s-p->wii_sec_sz_s);
    wbfs_disc_info_t *info = 0;
    u8* copy_buffer = 0;
    int disc_info_sz_lba;

    u8 * used = wbfs_malloc(p->n_wii_sec_per_disc);
    if (!used)
	OUT_OF_MEMORY;


    //----- open source disc

    wd_disc_t * disc = wd_dup_disc(par->wd_disc);
    if (!disc)
    {
	disc = wd_open_disc(	par->read_src_wii_disc,
				par->callback_data,
				par->iso_size,
				0,
				0 );
	if (!disc)
	    WBFS_ERROR("unable to open wii disc");
    }

    wd_filter_usage_table(disc,used,par->psel);
    const bool copy_1_1 = disc->whole_disc;


    //----- count total number of blocks to write

    u32 current_block = 0;
    u32 total_blocks  = 0;

    for ( i = 0; i < p->n_wbfs_sec_per_disc; i++ )
	if ( copy_1_1 || is_block_used(used, i, wii_sec_per_wbfs_sect) )
	    total_blocks++;

    const u32 free_blocks = wbfs_get_free_block_count(p);
    if ( total_blocks > free_blocks )
    {
	wbfs_error("New discs needs %u wbfs blocks (%s)"
		" but only %u blocks are available.",
		total_blocks,
		wd_print_size(0,0,p->wbfs_sec_sz,false,WD_SIZE_AUTO),
		free_blocks );
	goto error;
    }

    if (par->spinner)
	par->spinner(0,total_blocks,par->callback_data);



 // [codeview]

    for ( i = 0; i < p->max_disc; i++) // find a free slot.
	if (p->head->disc_table[i] == WBFS_SLOT_FREE)
	    break;

    if (i == p->max_disc)
	WBFS_ERROR("no space left on device (table full)");

    p->head->disc_table[i] = WBFS_SLOT_VALID;
    slot = i;

#if !NEW_WBFS_INTERFACE
    wbfs_load_freeblocks(p);
#endif

    // build disc info
    info = wbfs_ioalloc(p->disc_info_sz);
    if (!info)
	OUT_OF_MEMORY;
    memset(info,0,p->disc_info_sz);
    // [2do] use wd_read_and_patch()
    par->read_src_wii_disc(par->callback_data, 0, 0x100, info->dhead);

    copy_buffer = wbfs_ioalloc(p->wbfs_sec_sz);
    if (!copy_buffer)
	WBFS_ERROR("alloc memory");

 #ifndef WIT // WIT does it in an other way (patching while reading)
    const u32 ptab_off   = wd_get_ptab_sector(disc) * WII_SECTOR_SIZE;
    const int ptab_index = ptab_off >> p->wbfs_sec_sz_s;
 #endif

    u32 bl = 0;
#if NEW_WBFS_INTERFACE
    if ( p->balloc_mode == WBFS_BA_AVOID_FRAG
	|| p->balloc_mode == WBFS_BA_AUTO
		&& p->hd_sec_sz * (u64)p->n_hd_sec >= 20*(u64)GiB )
    {
	bl = wbfs_find_free_blocks(p,total_blocks);
	PRINT("WBFS: AVOID FRAG, first block=%u\n",bl);
    }
#endif

    for ( i = 0; i < p->n_wbfs_sec_per_disc; i++ )
    {
	info->wlba_table[i] = wbfs_htons(0);
	if (copy_1_1 || is_block_used(used, i, wii_sec_per_wbfs_sect))
	{
	    bl = wbfs_alloc_block(p,bl);
	    if ( bl == WBFS_NO_BLOCK )
	    {
		// free disc slot
		p->head->disc_table[slot] = WBFS_SLOT_FREE;

		// free already allocated blocks
		int j;
		for ( j = 0; j < i; j++ )
		{
		    bl = wbfs_ntohs(info->wlba_table[j]);
		    if (bl)
			wbfs_free_block(p,bl);
		}
		wbfs_sync(p);
		WBFS_ERROR("No space left on device (WBFS runs full)");
	    }
	    info->wlba_table[i] = wbfs_htons(bl);

	    u8 * dest = copy_buffer;
	    const u32 wiimax = (i+1) * wii_sec_per_wbfs_sect;
	    u32 subsec = 0;
	    while ( subsec < wii_sec_per_wbfs_sect )
	    {
		const u32 wiisec = i * wii_sec_per_wbfs_sect + subsec;
		if ( wiisec < WII_MAX_SECTORS && used[wiisec] )
		{
		    u32 wiiend = wiisec+1;
		    while ( wiiend < wiimax && used[wiiend] )
			wiiend++;
		    const u32 size = ( wiiend - wiisec ) * p->wii_sec_sz;
		    // [2do] use wd_read_and_patch()
		    if (par->read_src_wii_disc(par->callback_data,
				wiisec * (p->wii_sec_sz>>2), size, dest ))
			WBFS_ERROR("error reading disc");

		    dest += size;
		    subsec += wiiend - wiisec;
		}
		else
		{
		    TRACE("LIBWBFS: FILL sec %u>%u -> %p\n",subsec,wiisec,dest);
		    memset(dest,0,p->wii_sec_sz);
		    dest += p->wii_sec_sz;
		    subsec++;
		}
	    }

 #ifndef WIT // WIT does it in an other way
	    // fix the partition table.
	    if ( i == ptab_index )
		wd_patch_ptab(	disc,
				copy_buffer + ptab_off - i * p->wbfs_sec_sz,
				false );
 #endif

	    p->write_hdsector(	p->callback_data,
				p->part_lba + bl * (p->wbfs_sec_sz / p->hd_sec_sz),
				p->wbfs_sec_sz / p->hd_sec_sz,
				copy_buffer );

	    if (par->spinner)
		par->spinner(++current_block,total_blocks,par->callback_data);

 	}
   }

    // inode info
    par->iinfo.itime = 0ull;
    wbfs_setup_inode_info(p,&par->iinfo,1,1);
    memcpy( info->dhead + WBFS_INODE_INFO_OFF,
	    &par->iinfo,
	    sizeof(par->iinfo) );

    // write disc info
    disc_info_sz_lba = p->disc_info_sz >> p->hd_sec_sz_s;
    p->write_hdsector(	p->callback_data,
			p->part_lba + 1 + slot * disc_info_sz_lba,
			disc_info_sz_lba,
			info );
    if (p->id_list)
	memcpy(p->id_list[slot],info,sizeof(*p->id_list));
    wbfs_sync(p);

    par->slot = slot;
    par->open_disc = wbfs_open_disc_by_info(p,slot,info,0);
    info = 0;

error:
    wd_close_disc(disc);
    if (used)
	wbfs_free(used);
    if (info)
	wbfs_iofree(info);
    if (copy_buffer)
	wbfs_iofree(copy_buffer);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

u32 wbfs_add_phantom ( wbfs_t *p, const char * phantom_id, u32 wii_sectors )
{
    ASSERT(p);
    TRACE("LIBWBFS: +wbfs_add_phantom(%p,%s,%u)\n",
	    p, phantom_id, wii_sectors );

    if ( !phantom_id || !*phantom_id || !wii_sectors )
	return 1;

    if ( wii_sectors > WII_MAX_SECTORS )
	wii_sectors = WII_MAX_SECTORS;

    const u32 wii_sec_per_wbfs_sect = 1 << (p->wbfs_sec_sz_s-p->wii_sec_sz_s);
    wbfs_disc_info_t * info = 0;

    // find a free slot.
    int slot;
    for ( slot = 0; slot < p->max_disc; slot++ )
	if (!p->head->disc_table[slot])
	    break;

    u32 err = 0;
    if ( slot == p->max_disc )
    {
	err++;
	WBFS_ERROR("no space left on device (table full)");
    }

    p->head->disc_table[slot] = WBFS_SLOT_VALID;
#if !NEW_WBFS_INTERFACE
    wbfs_load_freeblocks(p);
#endif

    // build disc info
    info = wbfs_ioalloc(p->disc_info_sz);
    if (!info)
	OUT_OF_MEMORY;
    memset(info,0,p->disc_info_sz);
    memcpy(info->dhead,phantom_id,6);
    snprintf( (char*)info->dhead + WII_TITLE_OFF,
	WII_TITLE_SIZE, "Phantom %.6s @ slot %u -> not a real disc, for tests only!",
		phantom_id, slot );

    const u32 max_wbfs_sect = (wii_sectors-1) / wii_sec_per_wbfs_sect + 1;
    TRACE(" - add %u wbfs sectors to slot %u\n",max_wbfs_sect,slot);

    u32 bl = 0;
    int i;
    for ( i = 0; i < max_wbfs_sect; i++)
    {
	bl = wbfs_alloc_block(p,bl);
	if ( bl == WBFS_NO_BLOCK )
	{
	    if (!i)
		p->head->disc_table[slot] = WBFS_SLOT_FREE;
	    err++;
	    break; // use smaller phantom
	}

	if ( i == 0 )
	    p->write_hdsector(	p->callback_data,
				p->part_lba + bl * (p->wbfs_sec_sz / p->hd_sec_sz),
				1,
				info );

	info->wlba_table[i] = wbfs_htons(bl);
    }

    // setup inode info
    wbfs_inode_info_t * iinfo = wbfs_get_inode_info(p,info,2);
    wbfs_setup_inode_info(p,iinfo,1,1);

    // write disc info
    *(u32*)(info->dhead+24) = wbfs_ntohl(0x5D1C9EA3);
    u32 disc_info_sz_lba = p->disc_info_sz >> p->hd_sec_sz_s;
    p->write_hdsector(	p->callback_data,
			p->part_lba + 1 + slot * disc_info_sz_lba,
			disc_info_sz_lba,
			info );

    if (p->id_list)
	memcpy(p->id_list[slot],info,sizeof(*p->id_list));
    wbfs_sync(p);

error:
    if (info)
	wbfs_iofree(info);
    TRACE("LIBWBFS: -wbfs_add_phantom() return %u\n",err);
    return err;
}

///////////////////////////////////////////////////////////////////////////////
// old 'rename title' interface

u32 wbfs_ren_disc ( wbfs_t * p, u8 * discid, u8 * newname )
{
    wbfs_disc_t *d = wbfs_open_disc_by_id6(p, discid);
    if (!d)
	return 1;

    // use the new implementation
    int err = wbfs_rename_disc(d,0,(char*)newname,1,0);
    wbfs_close_disc(d);
    return err;
}

///////////////////////////////////////////////////////////////////////////////
// old 'rename id' interface

u32 wbfs_nid_disc ( wbfs_t * p, u8 * discid, u8 * newid )
{
    wbfs_disc_t *d = wbfs_open_disc_by_id6(p, discid);
    if (!d)
	return 1;

    // use the new implementation
    int err = wbfs_rename_disc(d,(char*)newid,0,1,0);
    wbfs_close_disc(d);
    return err;
}

///////////////////////////////////////////////////////////////////////////////
#ifndef WIT // not used in WiT

u32 wbfs_estimate_disc
(
    wbfs_t		*p,
    wd_read_func_t	read_src_wii_disc,
    void		*callback_data,
    const wd_select_t	* psel
)
{ // [codeview]
    u8 *b;
    int i;
    u32 tot;
    u32 wii_sec_per_wbfs_sect = 1 << (p->wbfs_sec_sz_s-p->wii_sec_sz_s);
    u8 *used = 0;
    wbfs_disc_info_t *info = 0;
    wd_disc_t * disc = 0;

    tot = 0;

    used = wbfs_malloc(p->n_wii_sec_per_disc);
    if (!used)
	WBFS_ERROR("unable to alloc memory");

    disc = wd_open_disc(read_src_wii_disc,callback_data,0,0,0);
    if (!disc)
	WBFS_ERROR("unable to open wii disc");
    wd_filter_usage_table_sel(disc,used,psel);

    info = wbfs_ioalloc(p->disc_info_sz);
    b = (u8 *)info;
    read_src_wii_disc(callback_data, 0, 0x100, info->dhead);

    for (i = 0; i < p->n_wbfs_sec_per_disc; i++)
	if ((used, i, wii_sec_per_wbfs_sect))
	    tot++;

error:
    wd_close_disc(disc);

    if (used)
	wbfs_free(used);

    if (info)
	wbfs_iofree(info);

    return tot * ((p->wbfs_sec_sz / p->hd_sec_sz) * 512);
}

#endif // !WIT
///////////////////////////////////////////////////////////////////////////////

u32 wbfs_rm_disc ( wbfs_t * p, u8 * discid, int free_slot_only )
{
    TRACE("LIBWBFS: +wbfs_rm_disc(%p,%.6s,%d)\n",p,discid,free_slot_only);
    ASSERT(p);
    ASSERT(discid);
    ASSERT(p->head);

    wbfs_disc_t *d = wbfs_open_disc_by_id6(p,discid);
    if (!d)
	return 1;
    int slot = d->slot;

    TRACE("LIBWBFS: disc_table[slot=%d]=%x\n", slot, p->head->disc_table[slot] );

    if (!free_slot_only)
    {
#if !NEW_WBFS_INTERFACE
	wbfs_load_freeblocks(p);
#endif
	int i;
	for ( i=0; i< p->n_wbfs_sec_per_disc; i++)
	{
	    u16 iwlba = wbfs_ntohs(d->header->wlba_table[i]);
	    //TRACE("FREE %u\n",iwlba);
	    if (iwlba)
		wbfs_free_block(p,iwlba);
	}

	wbfs_inode_info_t * iinfo = wbfs_get_disc_inode_info(d,1);
	ASSERT(iinfo);
	wbfs_setup_inode_info(p,iinfo,0,1);
 #ifdef TEST
	*(u32*)(d->header->dhead+WII_MAGIC_OFF) = wbfs_htonl(WII_MAGIC_DELETED);
 #endif
	const u32 disc_info_sz_lba = p->disc_info_sz >> p->hd_sec_sz_s;
	p->write_hdsector(
			p->callback_data,
			p->part_lba + 1 + slot * disc_info_sz_lba,
			disc_info_sz_lba,
			d->header );
    }
    wbfs_close_disc(d);

    p->head->disc_table[slot] = WBFS_SLOT_FREE;
    if (p->id_list)
	memset(p->id_list[slot],0,sizeof(*p->id_list));
    wbfs_sync(p);

    TRACE("LIBWBFS: -wbfs_rm_disc() return=0\n");
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

u32 wbfs_trim ( wbfs_t * p ) // trim the file-system to its minimum size
{
 #if NEW_WBFS_INTERFACE

    DASSERT(p);
    DASSERT(p->used_block);
    DASSERT( p->wbfs_sec_sz_s >= p->hd_sec_sz_s );

    const u32 max_block = find_last_used_block(p) + 1;
    wbfs_calc_geometry( p, max_block << p->wbfs_sec_sz_s - p->hd_sec_sz_s,
			p->hd_sec_sz, p->wbfs_sec_sz );
    p->used_block_dirty = p->is_dirty = true;
    wbfs_sync(p);

 #else    

    u32 max_block = find_last_used_block(p) + 1;
    p->n_hd_sec = max_block << p->wbfs_sec_sz_s - p->hd_sec_sz_s;
    p->head->n_hd_sec = wbfs_htonl(p->n_hd_sec);

    TRACE("max_block=%u, n_hd_sec=%u\n",max_block,p->n_hd_sec);

    // mark all blocks 'used'
    wbfs_load_freeblocks(p);
    memset(p->freeblks,0,p->freeblks_size4*4);
    wbfs_sync(p);

 #endif

    // os layer will truncate the file.
    TRACE("LIBWBFS: -wbfs_trim() return=%u\n",max_block);
    return max_block;
}

///////////////////////////////////////////////////////////////////////////////
// data extraction

u32 wbfs_extract_disc
	( wbfs_disc_t*d, rw_sector_callback_t write_dst_wii_sector,
	  void *callback_data,progress_callback_t spinner)
{
    // [codeview]

    wbfs_t *p = d->p;
    u8* copy_buffer = 0;
    int tot = 0, cur = 0;
    int i;
    int filling_info = 0;

    int src_wbs_nlb=p->wbfs_sec_sz/p->hd_sec_sz;
    int dst_wbs_nlb=p->wbfs_sec_sz/p->wii_sec_sz;

    copy_buffer = wbfs_ioalloc(p->wbfs_sec_sz);

    if (!copy_buffer)
	WBFS_ERROR("alloc memory");

    if (spinner)
    {
	// count total number to write for spinner
	for (i = 0; i < p->n_wbfs_sec_per_disc; i++)
	{
	    if (wbfs_ntohs(d->header->wlba_table[i]))
		tot++;
	}
	spinner(0,tot,callback_data);
    }

    for (i = 0; i < p->n_wbfs_sec_per_disc; i++)
    {
	u32 iwlba = wbfs_ntohs(d->header->wlba_table[i]);
	if (iwlba)
	{
	    cur++;
	    if (spinner)
		spinner(cur,tot,callback_data);

	    if (p->read_hdsector(p->callback_data, p->part_lba + iwlba*src_wbs_nlb, src_wbs_nlb, copy_buffer))
		WBFS_ERROR("reading disc");
	    if (write_dst_wii_sector(callback_data, i*dst_wbs_nlb, dst_wbs_nlb, copy_buffer))
		WBFS_ERROR("writing disc");
	}
	else
	{
	    switch (filling_info)
	    {
		case 0:
		    if (cur == tot)
			filling_info = 1;
		    break;

		case 1:
		    //fprintf(stderr, "Filling empty space in extracted image. Please wait...\n");
		    filling_info = 2;
		    break;

		case 2:
		default:
		    break;
	    }
	}
    }
    wbfs_iofree(copy_buffer);

    wbfs_inode_info_t * iinfo = wbfs_get_disc_inode_info(d,1);
    iinfo->atime = wbfs_setup_inode_info(p,iinfo,1,0);
    return 0;

error:
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

// u32 wbfs_extract_file ( wbfs_disc_t * d, char *path );

