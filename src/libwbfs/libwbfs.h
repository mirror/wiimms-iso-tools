
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

#ifndef LIBWBFS_H
#define LIBWBFS_H

#include "file-formats.h"
#include "wiidisc.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

///////////////////////////////////////////////////////////////////////////////

#define WBFS_MAGIC ( 'W'<<24 | 'B'<<16 | 'F'<<8 | 'S' )
#define WBFS_VERSION 1
#define WBFS_NO_BLOCK (~(u32)0)

#define WBFS_MAX_SECT		    0x10000

///////////////////////////////////////////////////////////////////////////////

//  WBFS first wbfs_sector structure:
//
//  -----------
// | wbfs_head |  (hd_sec_sz)
//  -----------
// |	       |
// | disc_info |
// |	       |
//  -----------
// |	       |
// | disc_info |
// |	       |
//  -----------
// |	       |
// | ...       |
// |	       |
//  -----------
// |	       |
// | disc_info |
// |	       |
//  -----------
// |	       |
// |freeblk_tbl|
// |	       |
//  -----------
//

//-----------------------------------------------------------------------------

// callback definition. Return 1 on fatal error
// (callback is supposed to make retries until no hopes..)

typedef int (*rw_sector_callback_t)(void*fp,u32 lba,u32 count,void*iobuf);
typedef void (*progress_callback_t) (u64 done, u64 total, void * callback_data );

//-----------------------------------------------------------------------------

typedef struct wbfs_t
{
    wbfs_head_t	* head;

    /* hdsectors, the size of the sector provided by the hosting hard drive */
    u32		hd_sec_sz;
    u8		hd_sec_sz_s;		// the power of two of the last number
    u32		n_hd_sec;		// the number of hd sector in the wbfs partition

    /* standard wii sector (0x8000 bytes) */
    u32		wii_sec_sz;		// always WII_SECTOR_SIZE
    u8		wii_sec_sz_s;	// always 15
    u32		n_wii_sec;
    u32		n_wii_sec_per_disc;	// always WII_MAX_SECTORS

    /* The size of a wbfs sector */
    u32		wbfs_sec_sz;
    u32		wbfs_sec_sz_s;
    u16		n_wbfs_sec;		// this must fit in 16 bit!
    u16		n_wbfs_sec_per_disc;	// size of the lookup table

    u32		part_lba;		// the lba of the wbfs header

    /* virtual methods to read write the partition */
    rw_sector_callback_t read_hdsector;
    rw_sector_callback_t write_hdsector;
    void	* callback_data;

    u16		max_disc;		// maximal number of possible discs
    u32		freeblks_lba;		// the hd sector of the free blocks table
    u32		freeblks_lba_count;	// number of hd sector used by free blocks table
    u32		freeblks_size4;		// size in u32 of free blocks table
    u32		freeblks_mask;		// mask for last used u32 of freeblks
    u32		* freeblks;		// if not NULL: copy of free blocks table

    u16		disc_info_sz;

    u8		* tmp_buffer;		// pre-allocated buffer for unaligned read
    u8		(*id_list)[7];		// list with all disc ids

    bool	is_dirty;		// if >0: call wbfs_sync() on close
    u32		n_disc_open;		// number of open discs

} wbfs_t;

//-----------------------------------------------------------------------------

typedef struct wbfs_disc_t
{
    wbfs_t	* p;
    wbfs_disc_info_t * header;		// pointer to wii header
    int		slot;			// disc slot, range= 0 .. wbfs_t::max_disc-1
    bool	is_used;		// disc is marked as 'used'?
    bool	is_valid;		// disc has valid id and magic
    bool	is_deleted;		// disc has valid id and deleted_magic
    bool	is_iinfo_valid;		// disc has a valid wbfs_inode_info_t
    bool	is_creating;		// disc is in creation process
    bool	is_dirty;		// if >0: call wbfs_sync_disc_header() on close

} wbfs_disc_t;

//-----------------------------------------------------------------------------

be64_t	wbfs_setup_inode_info
	( wbfs_t * p, wbfs_inode_info_t * ii, bool is_valid, int is_changed );
int	wbfs_is_inode_info_valid( wbfs_t * p, wbfs_inode_info_t * ii );

//-----------------------------------------------------------------------------

typedef struct wbfs_param_t // function parameters
{
  //----- parameters for wbfs_open_partition_param()

	// call back data
	rw_sector_callback_t	read_hdsector;		// read sector function
	rw_sector_callback_t	write_hdsector;		// write sector function

	// needed parameters
	u32			hd_sector_size;		// HD sector size (0=>512)
	u32			part_lba;		// partition LBA delta
	int			old_wii_sector_calc;	// >0 => use old & buggy calculation
	int			force_mode;		// >0 => no plausibility tests
	int			reset;			// >0 => format disc

	// only used if formatting disc (if reset>0)
	u32			num_hd_sector;		// num of HD sectors
	int			clear_inodes;		// >0 => clear inodes
	int			setup_iinfo;		// >0 => clear inodes & use iinfo
	int			wbfs_sector_size;	// >0 => force wbfs_sec_sz_s


  //----- parameters for wbfs_open_partition_param()
  
	// call back data
	wd_read_func_t		read_src_wii_disc;	// read wit sector [obsolete?]
	progress_callback_t	spinner;		// progress callback

  //----- parameters for wbfs_add_disc_param()

	u64			iso_size;		// size of iso image in bytes
	wd_disc_t		*wd_disc;		// NULL or the source disc
	const wd_select_t	* psel;			// partition selector

  //----- multi use parameters

	// call back data
	void			*callback_data;		// used defined data

	// inode info
	wbfs_inode_info_t	iinfo;			// additional infos

  //----- infos (output of wbfs framework)

	int			slot;			// >=0: slot of last added disc
	wbfs_disc_t		*open_disc;		// NULL or open disc

} wbfs_param_t;

//-----------------------------------------------------------------------------

/*! @brief open a MSDOS partitionned harddrive. This tries to find a wbfs partition into the harddrive
   @param read_hdsector,write_hdsector: accessors to a harddrive
   @hd_sector_size: size of the hd sector. Can be set to zero if the partition in already initialized
   @num_hd_sector:  number of sectors in this disc. Can be set to zero if the partition in already initialized
   @reset: not implemented, This will format the whole harddrive with one wbfs partition that fits the whole disk.
   calls wbfs_error() to have textual meaning of errors
   @return NULL in case of error
*/

#ifndef WIT // not used in WiT

 wbfs_t * wbfs_open_hd(rw_sector_callback_t read_hdsector,
		      rw_sector_callback_t write_hdsector,
		      void *callback_data,
		      int hd_sector_size, int num_hd_sector, int reset);

#endif

//-----------------------------------------------------------------------------

/*! @brief open a wbfs partition
   @param read_hdsector,write_hdsector: accessors to the partition
   @hd_sector_size: size of the hd sector. Can be set to zero if the partition in already initialized
   @num_hd_sector:  number of sectors in this partition. Can be set to zero if the partition in already initialized
   @partition_lba:  The partitio offset if you provided accessors to the whole disc.
   @reset: initialize the partition with an empty wbfs.
   calls wbfs_error() to have textual meaning of errors
   @return NULL in case of error
*/
wbfs_t*wbfs_open_partition(rw_sector_callback_t read_hdsector,
			   rw_sector_callback_t write_hdsector,
			   void *callback_data,
			   int hd_sector_size, int num_hd_sector, u32 partition_lba, int reset);

wbfs_t * wbfs_open_partition_param ( wbfs_param_t * par );

int wbfs_calc_size_shift
	( u32 hd_sec_sz_s, u32 num_hd_sector, int old_wii_sector_calc );

void wbfs_calc_geometry
(
	wbfs_t * p,		// pointer to wbfs_t, p->head must be NULL or valid
	u32 n_hd_sec,		// total number of hd_sec in partition
	u32 hd_sec_sz,		// size of a hd/partition sector
	u32 wbfs_sec_sz		// size of a wbfs sector
);

//-----------------------------------------------------------------------------

void wbfs_close ( wbfs_t * p );
void wbfs_sync  ( wbfs_t * p );

wbfs_disc_t * wbfs_open_disc_by_id6  ( wbfs_t * p, u8 * id6 );
wbfs_disc_t * wbfs_open_disc_by_slot ( wbfs_t * p, u32 slot, int force_open );

wbfs_disc_t * wbfs_create_disc
(
    wbfs_t	* p,		// valid WBFS descriptor
    const void	* disc_header,	// NULL or disc header to copy
    const void	* disc_id	// NULL or ID6: check non existence
				// disc_id overwrites the id of disc_header
);

int wbfs_sync_disc_header ( wbfs_disc_t * d );
void wbfs_close_disc ( wbfs_disc_t * d );

wbfs_inode_info_t * wbfs_get_inode_info ( wbfs_t *p, wbfs_disc_info_t *info, int clear_mode );
wbfs_inode_info_t * wbfs_get_disc_inode_info ( wbfs_disc_t * d, int clear_mode );
	// clear_mode == 0 : don't clear
	// clear_mode == 1 : clear if invalid
	// clear_mode == 2 : clear always

// rename a disc
int wbfs_rename_disc
(
	wbfs_disc_t * d,	// pointer to an open disc
	const char * new_id,	// if !NULL: take the first 6 chars as ID
	const char * new_title,	// if !NULL: take the first 0x39 chars as title
	int change_wbfs_head,	// if !0: change ID/title of WBFS header
	int change_iso_head	// if !0: change ID/title of ISO header
);

int wbfs_touch_disc
(
	wbfs_disc_t * d,	// pointer to an open disc
	u64 itime,		// if != 0: new itime
	u64 mtime,		// if != 0: new mtime
	u64 ctime,		// if != 0: new ctime
	u64 atime		// if != 0: new atime
);

//-----------------------------------------------------------------------------

/*! @brief accessor to the wii disc
  @param d: a pointer to already open disc
  @param offset: an offset inside the disc, *points 32bit words*, allowing to access 16GB data
  @param len: The length of the data to fetch, in *bytes*
 */
// offset is pointing 32bit words to address the whole dvd, although len is in bytes
int wbfs_disc_read(wbfs_disc_t*d,u32 offset, u8 *data, u32 len);

/*! @return the number of discs inside the paritition */
u32 wbfs_count_discs(wbfs_t*p);

u32 wbfs_alloc_block ( wbfs_t * p );

enumError wbfs_get_disc_info
		( wbfs_t*p, u32 idx,  u8 *header, int header_size, u32 *size );
enumError wbfs_get_disc_info_by_slot
		( wbfs_t*p, u32 slot, u8 *header, int header_size, u32 *size );

/*! get the number of unuseds block of the partition.
  to be multiplied by p->wbfs_sec_sz (use 64bit multiplication) to have the number in bytes
*/
u32 wbfs_count_unusedblocks ( wbfs_t * p );

/******************* write access  ******************/

void wbfs_load_id_list	( wbfs_t * p, int force_reload );
int  wbfs_find_slot	( wbfs_t * p, const u8 * disc_id );

void wbfs_load_freeblocks ( wbfs_t * p );
void wbfs_free_block	  ( wbfs_t * p, u32 bl );
void wbfs_use_block	  ( wbfs_t * p, u32 bl );

/*! add a wii dvd inside the partition
  @param read_src_wii_disc: a callback to access the wii dvd. offsets are in 32bit, len in bytes!
  @callback_data: private data passed to the callback
  @spinner: a pointer to a function that is regulary called to update a progress bar.
  @sel: selects which partitions to copy.
  @copy_1_1: makes a 1:1 copy, whenever a game would not use the wii disc format, and some data is hidden outside the filesystem.
 */
u32 wbfs_add_disc
(
    wbfs_t		* p,
    wd_read_func_t	read_src_wii_disc,
    void		* callback_data,
    progress_callback_t	spinner,
    const wd_select_t	* psel,
    int			copy_1_1
);

u32 wbfs_add_disc_param ( wbfs_t * p, wbfs_param_t * par );

u32 wbfs_add_phantom ( wbfs_t * p, const char * phantom_id, u32 wii_sector_count );

u32 wbfs_estimate_disc
(
    wbfs_t		* p,
    wd_read_func_t	read_src_wii_disc,
    void		* callback_data,
    const wd_select_t	* psel
);

// remove a disc from partition
u32 wbfs_rm_disc ( wbfs_t * p, u8 * discid, int free_slot_only );

// rename a wiidvd inside a partition
u32 wbfs_ren_disc(wbfs_t*p, u8* discid, u8* newname);

// edit a wiidvd diskid
u32 wbfs_nid_disc(wbfs_t*p, u8* discid, u8* newid);

/*! trim the file-system to its minimum size
  This allows to use wbfs as a wiidisc container
 */
u32 wbfs_trim(wbfs_t*p);

/*! extract a disc from the wbfs, unused sectors are just untouched, allowing descent filesystem to only really usefull space to store the disc.
Even if the filesize is 4.7GB, the disc usage will be less.
 */
u32 wbfs_extract_disc(wbfs_disc_t*d, rw_sector_callback_t write_dst_wii_sector,void *callback_data,progress_callback_t spinner);

/*! extract a file from the wii disc filesystem.
  E.G. Allows to extract the opening.bnr to install a game as a system menu channel
 */
u32 wbfs_extract_file(wbfs_disc_t*d, char *path);

// remove some sanity checks
void wbfs_set_force_mode(int force);


/* OS specific functions provided by libwbfs_<os>.c */

wbfs_t *wbfs_try_open(char *disk, char *partition, int reset);
wbfs_t *wbfs_try_open_partition(char *fn, int reset);

void *wbfs_open_file_for_read(char*filename);
void *wbfs_open_file_for_write(char*filename);
int wbfs_read_file(void*handle, int len, void *buf);
void wbfs_close_file(void *handle);
void wbfs_file_reserve_space(void*handle,long long size);
void wbfs_file_truncate(void *handle,long long size);
int wbfs_read_wii_file(void *_handle, u32 _offset, u32 count, void *buf);
int wbfs_write_wii_sector_file(void *_handle, u32 lba, u32 count, void *buf);

///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // LIBWBFS_H
