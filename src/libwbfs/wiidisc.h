
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

#ifndef WIIDISC_H
#define WIIDISC_H

#include "file-formats.h"
#include "rijndael.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    consts			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_select_idx_t // modern partition selector, index values
{
    WD_PART_DATA	=  0,	// DATA partition
    WD_PART_UPDATE	=  1,	// UPDATE partition
    WD_PART_CHANNEL	=  2,	// CHANNEL partition

    WD_SELI_PART_MAX	= 50,	// maximal 'wd_select_t' supported partition
    WD_SELI_PART_ID,		// special ID partitons selected
    WD_SELI_PART_ACTIVE,	// partition type selection is active

    WD_SELI_PTAB_0,		// partition table 0 selected
    WD_SELI_PTAB_1,		// partition table 0 selected
    WD_SELI_PTAB_2,		// partition table 0 selected
    WD_SELI_PTAB_3,		// partition table 0 selected
    WD_SELI_PTAB_ACTIVE,	// partition tables selection is active

    WD_SELI_WHOLE_PART,		// don't scrub partition data
    WD_SELI_WHOLE_DISC,		// copy whole disc, includes WD_SELI_WHOLE_PART
    
    WD_SELI__N			// number of values

} wd_select_idx_t;

///////////////////////////////////////////////////////////////////////////////

typedef enum wd_select_t // modern partition selector, bit values
{
    WD_SEL_PART_DATA		= 1ull << WD_PART_DATA,
    WD_SEL_PART_UPDATE		= 1ull << WD_PART_UPDATE,
    WD_SEL_PART_CHANNEL		= 1ull << WD_PART_CHANNEL,
    WD_SEL_PART_MAX		= 1ull << WD_SELI_PART_MAX,
    WD_SEL_PART_ID		= 1ull << WD_SELI_PART_ID,
    WD_SEL_PART_ACTIVE		= 1ull << WD_SELI_PART_ACTIVE,

    WD_SEL_PTAB_0		= 1ull << WD_SELI_PTAB_0,
    WD_SEL_PTAB_1		= 1ull << WD_SELI_PTAB_1,
    WD_SEL_PTAB_2		= 1ull << WD_SELI_PTAB_2,
    WD_SEL_PTAB_3		= 1ull << WD_SELI_PTAB_3,
    WD_SEL_PTAB_ACTIVE		= 1ull << WD_SELI_PTAB_ACTIVE,

    WD_SEL_WHOLE_PART		= 1ull << WD_SELI_WHOLE_PART,
    WD_SEL_WHOLE_DISC		= 1ull << WD_SELI_WHOLE_DISC,

    WD_SEL_PART__MASK		= WD_SEL_PART_ACTIVE - 1,

    WD_SEL_PTAB__MASK		= WD_SEL_PTAB_0
				| WD_SEL_PTAB_1
				| WD_SEL_PTAB_2
				| WD_SEL_PTAB_3,

    WD_SEL__MASK		= ( 1ull << WD_SELI__N ) - 1,

} wd_select_t;

///////////////////////////////////////////////////////////////////////////////

typedef enum wd_icm_t // iterator call mode
{
    WD_ICM_OPEN_PART,		// called for each openening of a partition
    WD_ICM_CLOSE_PART,		// called if partitions closed

    WD_ICM_DIRECTORY,		// item is a directory; this is the first non hint message

    WD_ICM_FILE,		// item is a partition file -> extract with key
    WD_ICM_COPY,		// item is a disc area -> disc raw copy
    WD_ICM_DATA,		// item contains pure data [not used yet]

} wd_icm_t;

///////////////////////////////////////////////////////////////////////////////

typedef enum wd_ipm_t // iterator prefix mode
{
    WD_IPM_DEFAULT,		// not defined -> like AUTO in wiidisc
    WD_IPM_AUTO,		// for single partitions: WD_IPM_PART_NAME, else: WD_IPM_POINT

    WD_IPM_NONE,		// no prefix: ""
    WD_IPM_POINT,		// prefix with "./"
    WD_IPM_PART_INDEX,		// prefix with 'P' and partition id: "P%u/"
    WD_IPM_PART_NAME,		// prefix with partition name or "P<id>": "NAME/"

} wd_ipm_t;

///////////////////////////////////////////////////////////////////////////////

typedef enum wd_pfst_t // print-fst mode
{
    WD_PFST_HEADER	= 0x01,  // print table header
    WD_PFST_PART	= 0x02,  // print partition intro
    WD_PFST_OFFSET	= 0x04,  // print offset
    WD_PFST_SIZE_HEX	= 0x08,  // print size in hex
    WD_PFST_SIZE_DEC	= 0x10,  // print size in dec

    WD_PFST__ALL	= 0x1f

} wd_pfst_t;

///////////////////////////////////////////////////////////////////////////////

typedef enum wd_usage_t // usage table values
{
    WD_USAGE_UNUSED,		// block is not used, always = 0
    WD_USAGE_DISC,		// block is used for disc managment
    WD_USAGE_PART_0,		// index for first partition
    
    WD_USAGE__MASK	= 0x7f,	// mask for codes above
    WD_USAGE_F_CRYPT	= 0x80,	// flag: encrypted data block

} wd_usage_t;

///////////////////////////////////////////////////////////////////////////////

typedef enum wd_pname_mode_t // partition name modes
{
    WD_PNAME_NAME_NUM_9,	// NAME+NUM if possible, ID or NUM else, fw=9
    WD_PNAME_COLUMN_9,		// NAME+NUM|NUM|ID, fw=9
    WD_PNAME_NUM,		// ID or NUM
    WD_PNAME_NAME,		// NAME if possible, ID or NUM else
    WD_PNAME_P_NAME,		// NAME if possible, 'P-'ID or 'P'NUM else
    WD_PNAME_NUM_INFO,		// NUM + '[INFO]'

    WD_PNAME__N			// numper of supported formats

} wd_pname_mode_t;

///////////////////////////////////////////////////////////////////////////////

typedef enum wd_patch_mode_t // patching modes
{
    WD_PAT_DISC_DATA		// raw disc data

} wd_patch_mode_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			callback functions		///////////////
///////////////////////////////////////////////////////////////////////////////

struct wd_disc_t;
struct wd_part_t;
struct wd_iterator_t;

//-----------------------------------------------------------------------------
// Callback read function. Returns 0 on success and any other value on failutre.

typedef int (*wd_read_func_t)
(
	void		* read_data,	// user defined data
	u32		offset4,	// offset/4 to read
	u32		count,		// num of bytes to read
	void		* iobuf		// buffer, alloced with wbfs_ioalloc
);

//-----------------------------------------------------------------------------
// callback definition for memory dump with wd_dump_mem()

typedef void (*wd_mem_func_t)
(
	void		* param,	// user defined parameter
	u64		offset,		// offset of object
	u64		size,		// size of object
	ccp		info		// info about object
);

//-----------------------------------------------------------------------------
// callback definition for file iteration. if return != 0 => abort

typedef int (*wd_file_func_t)
(
	struct wd_iterator_t *it	// iterator struct with all infos
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_patch_item_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_patch_item_t // patching data item
{
    wd_patch_mode_t	mode;		// patching mode
    u64			offset;		// offset
    u64			size;		// size
    struct wd_part_t	* part;		// NULL or relevant partition
    void		* data;		// NULL or pointer to data/filename
    bool		data_alloced;	// true if data must be freed.
    char		info[30];	// comment for dumps

} wd_patch_item_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_patch_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_patch_t // patching data structure
{
    wd_patch_item_t	* item;		// pointer to first item
    u32			used;		// number of used items
    u32			size;		// number of allocated items

} wd_patch_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_part_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_part_t
{
    //----- base infos

    int			index;		// zero based index within wd_disc_t
    wd_usage_t		usage_id;	// marker for usage_table
    int			ptab_index;	// zero based index of owning partition table
    int			ptab_part_index;// zero based index within owning partition table
    u32			part_type;	// partition type

    u32			part_off4;	// offset/4 of partition relative to disc start
    u64			part_size;	// total size of partition

    struct wd_disc_t	* disc;		// pointer to disc
    wd_patch_t		patch;		// patching data

    //----- status

    bool		is_loaded;	// true if this partition info was loaded
    bool		is_valid;	// true if this partition is valid
    bool		is_enabled;	// true if this partition is enabled
    bool		is_encrypted;	// true if this partition is encrypted


    //----- partition data, only valid if 'is_valid' is true
    
    wd_part_header_t	ph;		// partition header (incl. ticket), host endian
    wd_tmd_t		* tmd;		// NULL or pointer to tmd, size = ph.tmd_size
    u8			* cert;		// NULL or pointer to cert, size = ph.cert_size
    u8			* h3;		// NULL or pointer to h3, size = WII_H3_SIZE

    u8			key[WII_KEY_SIZE];
					// partition key, needed to build aes key
    u32			data_off4;	// offset/4 of partition data relative to disc start

    wd_boot_t		boot;		// copy of boot.bin, host endian
    u32			dol_size;	// size of main.dol
    u32			apl_size;	// size of apploader.img

    wd_fst_item_t	* fst;		// pointer to fst data
    u32			fst_n;		// number or elements in fst
    u32			fst_max_off4;	// informative: maximal offset4 value of all files
    u32			fst_max_size;	// informative: maximal size value of all files
    u32			fst_dir_count;	// informative: number or directories in fst
    u32			fst_file_count;	// informative: number or real files in fst

} wd_part_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_disc_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_disc_t
{
    //----- open parameters

    wd_read_func_t	read_func;	// read function, always valid
    void		* read_data;	// data pointer for read function
    u64			file_size;	// size of file, 0=unknown
    int			open_count;	// open counter

    //----- errror support

    char		error_term[200]; // termination string for error messages

    //----- raw data

    char		id6[7];		// id6, copy of 'dhead', 0 term 
    wd_header_t		dhead;		// copy of disc header
    wd_region_t		region;		// copy of disc region settings
    wd_ptab_info_t	ptab_info[WII_MAX_PTAB];
					// copy of partion table info
    wd_ptab_entry_t	*ptab_entry;	// collected copy of all partion entries
    u32			magic2;		// second magic @ WII_MAGIC2_OFF

    //----- patching data

    wd_patch_t		patch;		// patching data
    wd_ptab_t		ptab;		// partition tables

    //----- partitions

    int			n_ptab;		// number of valid partition tables
    int			n_part;		// number of partitions
    wd_part_t		* part;		// partition data
    u32			invalid_part;	// number of invalid partitions
    u32			fst_n;		// informative: number or elements in fst
    u32			fst_max_off4;	// informative: maximal offset4 value of all files
    u32			fst_max_size;	// informative: maximal size value of all files
    u32			fst_dir_count;	// informative: number or directories in fst
    u32			fst_file_count;	// informative: number or real files in fst
    bool		patch_ptab_recommended;
					// informative: patch ptab is recommended

    //----- usage table

    u8			usage_table[WII_MAX_SECTORS];
					// usage table of disc
    int			usage_max;	// ( max used index of 'usage_table' ) + 1

    //----- block cache

    u8			block_buf[WII_SECTOR_DATA_SIZE];
					// cache for partition blocks
    u32			block_num;	// block number of last loaded 'block_buf'
    wd_part_t		* block_part;	// partition of last loaded 'block_buf'

    //----- akey cache

    aes_key_t		akey;		// aes key for 'akey_part'
    wd_part_t		* akey_part;	// partition of 'akey'

    //----- temp buffer

    u8			temp_buf[WII_SECTOR_SIZE];
					// temp buffer for reading operations

} wd_disc_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_iterator_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_iterator_t
{
    //----- global parameters

    void		* param;	// user defined parameter
    wd_disc_t		* disc;		// valid disc pointer
    wd_part_t		* part;		// valid disc partition pointer

    //----- file specific parameters
    
    wd_icm_t		icm;		// iterator call mode
    u32			off4;		// offset/4 to read
    u32			size;		// size of object
    const void		* data;		// NULL or pointer to data

    //----- filename

    wd_ipm_t		prefix_mode;	// prefix mode
    char		prefix[20];	// prefix of path
    int			prefix_len;	// := strlen(prefix)
    char		path[2000];	// full path including prefix
    char		*fst_name;	// pointer := path + prefix_len

} wd_iterator_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_print_fst_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_print_fst_t
{
	//----- base data

	FILE		* f;		// valid output file
	int		indent;		// indention of the output
	wd_pfst_t	mode;		// print mode

	//----- field widthes

	int		fw_offset;	// field width or 0 if hidden
	int		fw_size_dec;	// field width or 0 if hidden
	int		fw_size_hex;	// field width or 0 if hidden

	//----- filter function, used by wd_print_fst_item_wrapper()

	wd_file_func_t	filter_func;	// NULL or filter function
	void		* filter_param;	// user defined parameter

} wd_print_fst_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  interface			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError wd_print_error
(
    ccp		func,		// calling function, use macro __FUNCTION__
    ccp		file,		// source file, use macro __FILE__
    uint	line,		// line number of source file, use macro __LINE__
    enumError	err,		// error code
    ccp		format,		// NULL or format string for fprintf() function.
    ...				// parameters for 'format'
);

///////////////////////////////////////////////////////////////////////////////
// common key

const u8 * wd_get_common_key();

//-----------------------------------------------------------------------------

const u8 * wd_set_common_key
(
	const u8 * new_key		// the new key
);

//-----------------------------------------------------------------------------

void wd_decrypt_title_key
(
	const wd_ticket_t * tik,	// pointer to ticket
	u8 * title_key			// the result
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// names, ids and titles

extern const char * wd_part_name[];

//-----------------------------------------------------------------------------

const char * wd_get_part_name
(
	u32	ptype,			// partition type
	ccp	result_if_not_found	// result if no name found
);

//-----------------------------------------------------------------------------

char * wd_print_part_name
(
	char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
	size_t		buf_size,	// size of 'buf', ignored if buf==NULL
	u32		ptype,		// partition type
	wd_pname_mode_t	mode		// print mode
);

//-----------------------------------------------------------------------------
// returns a pointer to a printable ID, teminated with 0
	
char * wd_print_id
(
	const void	* id,		// ID to convert in printable format
	size_t		id_len,		// len of 'id'
	void		* buf		// Pointer to buffer, size >= id_len + 1
					// If NULL, a local circulary static buffer is used
);

//-----------------------------------------------------------------------------
// rename ID and title of a ISO header

int wd_rename
(
	void		* data,		// pointer to ISO data
	ccp		new_id,		// if !NULL: take the first 6 chars as ID
	ccp		new_title	// if !NULL: take the first 0x39 chars as title
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// read functions

enumError wd_read_raw
(
	wd_disc_t	* disc,		// valid disc pointer
	u32		disc_offset4,	// disc offset/4
	void		* dest_buf,	// destination buffer
	u32		read_size,	// number of bytes to read
	wd_usage_t	usage_id	// not 0: mark usage usage_tab with this value
);

//-----------------------------------------------------------------------------

enumError wd_read_part_raw
(
	wd_part_t	* part,		// valid pointer to a disc partition
	u32		offset4,	// offset/4 to partition start
	void		* dest_buf,	// destination buffer
	u32		read_size,	// number of bytes to read 
	bool		mark_block	// true: mark block in 'usage_table'
);

//-----------------------------------------------------------------------------

enumError wd_read_part_block
(
	wd_part_t	* part,		// valid pointer to a disc partition
	u32		block_num,	// block number of partition
	u8		* block,	// destination buf
	bool		mark_block	// true: mark block in 'usage_table'
);

//-----------------------------------------------------------------------------

enumError wd_read_part
(
	wd_part_t	* part,		// valid pointer to a disc partition
	u32		data_offset4,	// partition data offset/4
	void		* dest_buf,	// destination buffer
	u32		read_size,	// number of bytes to read 
	bool		mark_block	// true: mark block in 'usage_table'
);

//-----------------------------------------------------------------------------

void wd_mark_part
(
	wd_part_t	* part,		// valid pointer to a disc partition
	u32		data_offset4,	// partition data offset/4
	u32		data_size	// number of bytes to mark
);

//-----------------------------------------------------------------------------

int wd_is_block_encrypted
(
	wd_part_t	* part,		// valid pointer to a disc partition
	u32		block_num,	// block number of partition
	int		unknown_result	// result if status is unknown
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// open and close

wd_disc_t * wd_open_disc
(
	wd_read_func_t	read_func,	// read function, always valid
	void		* read_data,	// data pointer for read function
	u64		file_size,	// size of file, unknown if 0
	ccp		file_name,	// used for error messages if not NULL
	enumError	* error_code	// store error code if not NULL
);

//-----------------------------------------------------------------------------

wd_disc_t * wd_dup_disc
(
	wd_disc_t * disc		// NULL or a valid disc pointer
);

//-----------------------------------------------------------------------------

void wd_close_disc
(
	wd_disc_t * disc		// NULL or a valid disc pointer
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// get partition

wd_part_t * wd_get_part_by_index
(
	wd_disc_t	* disc,		// valid disc pointer
	int		index,		// zero based partition index within wd_disc_t
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
);

//-----------------------------------------------------------------------------

wd_part_t * wd_get_part_by_usage
(
	wd_disc_t	* disc,		// valid disc pointer
	u8		usage_id,	// value of 'usage_table'
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
);

//-----------------------------------------------------------------------------

wd_part_t * wd_get_part_by_ptab
(
	wd_disc_t	* disc,		// valid disc pointer
	int		ptab_index,	// zero based index of partition table
	int		ptab_part_index,// zero based index within owning partition table
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
);

//-----------------------------------------------------------------------------

wd_part_t * wd_get_part_by_type
(
	wd_disc_t	* disc,		// valid disc pointer
	u32		part_type,	// find first partition with this type
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// load partition data

enumError wd_load_part
(
	wd_part_t	* part,		// valid disc partition pointer
	bool		load_cert,	// true: load cert data too
	bool		load_h3		// true: load h3 data too
);

//-----------------------------------------------------------------------------

enumError wd_load_all_part
(
	wd_disc_t	* disc,		// valid disc pointer
	bool		load_cert,	// true: load cert data too
	bool		load_h3		// true: load h3 data too
);

//-----------------------------------------------------------------------------

enumError wd_calc_fst_statistics
(
	wd_disc_t	* disc,		// valid disc pointer
	bool		sum_all		// false: summarize only enabled partitions
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// select partitions

bool wd_is_part_selected
(
	wd_select_t	select,		// partition selector bit field
	u32		part_type,	// partiton type
	u32		ptab_index	// index of partiton table
);

//-----------------------------------------------------------------------------

int wd_select
(
	wd_disc_t	* disc,		// valid disc pointer
	wd_select_t	select		// partition selector bit field
);

//-----------------------------------------------------------------------------

wd_select_t wd_set_select
(
	wd_select_t	select,		// current selector value
	wd_select_t	set_mask	// bits to set
);

//-----------------------------------------------------------------------------

wd_select_t wd_clear_select
(
	wd_select_t	select,		// current selector value
	wd_select_t	clear_mask	// bits to clear
);

//-----------------------------------------------------------------------------

bool wd_patch_ptab
(
	wd_disc_t	* disc,		// valid disc pointer
	void		* data,		// pointer to data area, WII_MAX_PTAB_SIZE
	bool		force_patch	// false: patch only if needed
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// usage table

extern const char wd_usage_name_tab[256];
extern const u8   wd_usage_class_tab[256];

//-----------------------------------------------------------------------------

u8 * wd_calc_usage_table
(
	wd_disc_t	* disc		// valid disc partition pointer
);

//-----------------------------------------------------------------------------

u8 * wd_filter_usage_table
(
	wd_disc_t	* disc,		// valid disc pointer
	u8		* usage_table,	// NULL or result. If NULL -> malloc()
	bool		whole_part,	// true: mark complete partitions
	bool		whole_disc	// true: mark all sectors, incl. whole_part
);

//-----------------------------------------------------------------------------

u8 * wd_filter_usage_table_sel
(
	wd_disc_t	* disc,		// valid disc pointer
	u8		* usage_table,	// NULL or result. If NULL -> malloc()
	wd_select_t	select		// partition selector bit field
);

//-----------------------------------------------------------------------------

u32 wd_count_used_disc_blocks
(
	wd_disc_t	* disc,		// valid pointer to a disc partition
	u32		block_size	// if >1: count every 'block_size'
					//        continuous blocks as one block
);

//-----------------------------------------------------------------------------

u32 wd_count_used_disc_blocks_sel
(
	wd_disc_t	* disc,		// valid pointer to a disc partition
	u32		block_size,	// if >1: count every 'block_size'
					//        continuous blocks as one block
	wd_select_t	select		// partition selector bit field
);

//-----------------------------------------------------------------------------

u32 wd_count_used_blocks
(
	const u8 *	usage_table,	// valid pointer to usage table
	u32		block_size	// if >1: count every 'block_size'
					//        continuous blocks as one block
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// file iteration

int wd_iterate_files
(
    wd_disc_t		* disc,		// valid pointer to a disc partition
    wd_file_func_t	func,		// call back function
    void		* param,	// user defined parameter
    wd_ipm_t		prefix_mode	// prefix mode
);

//-----------------------------------------------------------------------------

int wd_iterate_fst_files
(
    const wd_fst_item_t	*fst_base,	// NULL or pointer to FST data
    wd_file_func_t	func,		// call back function
    void		* param		// user defined parameter
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// print files

extern const char wd_sep_200[201]; // 200 * '-' + NULL

//-----------------------------------------------------------------------------

void wd_initialize_print_fst
(
	wd_print_fst_t	* pf,		// valid pointer
	wd_pfst_t	mode,		// mode for setup
	FILE		* f,		// NULL or output file
	int		indent,		// indention of the output
	u32		max_off4,	// NULL or maximal offset4 value of all files
	u32		max_size	// NULL or maximal size value of all files
);

//-----------------------------------------------------------------------------

void wd_print_fst_header
(
	wd_print_fst_t	* pf,		// valid pointer
	int		max_name_len	// max name len, needed for separator line
);

//-----------------------------------------------------------------------------

void wd_print_fst_item
(
	wd_print_fst_t	* pf,		// valid pointer
	wd_part_t	* part,		// valid pointer to a disc partition
	wd_icm_t	icm,		// iterator call mode
	u32		offset4,	// offset/4 to read
	u32		size,		// size of object
	ccp		fname1,		// NULL or file name, part 1
	ccp		fname2		// NULL or file name, part 2
);

//-----------------------------------------------------------------------------

void wd_print_fst
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	wd_disc_t	* disc,		// valid pointer to a disc partition
	wd_ipm_t	prefix_mode,	// prefix mode
	wd_pfst_t	pfst_mode,	// print mode
	wd_file_func_t	filter_func,	// NULL or filter function
	void		* filter_param	// user defined parameter
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// patching

void wd_reset_patch 
(
	wd_patch_t	* patch		// NULL or patching data
);

//-----------------------------------------------------------------------------

wd_patch_item_t * wd_insert_patch
(
	wd_patch_t	* patch,	// NULL or patching data
	wd_patch_mode_t	mode,		// patch mode
	u64		offset,		// offset of object
	u64		size		// size of object
);


//-----------------------------------------------------------------------------

void wd_dump_patch
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	wd_patch_t	* patch		// NULL or patching data
);

//-----------------------------------------------------------------------------

void wd_dump_disc_patch
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	wd_disc_t	* disc,		// valid pointer to a disc partition
	bool		print_title,	// true: print table titles
	bool		print_part	// true: print partitions too
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// dump data structure

int wd_normalize_indent
(
	int		indent		// base vlaue to normalize
);

//-----------------------------------------------------------------------------

void wd_dump_disc
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	wd_disc_t	* disc,		// valid disc pointer
	int		dump_level	// dump level
					//	>0: print extended part info 
					//	>1: print usage table
);

//-----------------------------------------------------------------------------

void wd_dump_disc_usage_tab
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	wd_disc_t	* disc,		// valid disc pointer
	bool		print_all	// false: ignore const lines
);

//-----------------------------------------------------------------------------

void wd_dump_usage_tab
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	const u8	* usage_tab,	// valid pointer, size = WII_MAX_SECTORS
	bool		print_all	// false: ignore const lines
);

//-----------------------------------------------------------------------------

void wd_dump_mem
(
    wd_disc_t		* disc,		// valid disc pointer
    wd_mem_func_t	func,		// valid function pointer
    void		* param		// user defined parameter
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIIDISC_H

