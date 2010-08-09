
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

#ifndef WIT_ISO_INTERFACE_H
#define WIT_ISO_INTERFACE_H 1

#include "types.h"
#include "lib-sf.h"
#include "patch.h"
#include "match-pattern.h"
#include "libwbfs/libwbfs.h"
#include "libwbfs/rijndael.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  wii iso discs                  ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct WDiscInfo_t
{
	wd_header_t	dhead;
	u32		magic2;

	uint		disc_index;
	char		id6[7];
	u64		size;
	u64		iso_size;
	u32		used_blocks;
	ccp		title;		// pointer to title DB
	u32		n_part;		// number of partitions

} WDiscInfo_t;

//-----------------------------------------------------------------------------

typedef struct WDiscListItem_t
{
	u32  size_mib;		// size of the source in MiB
	u32  used_blocks;	// number of used ISO blocks
	char id6[7];		// ID6
	char name64[65];	// disc name from header
	ccp  title;		// ptr into title DB (not alloced)
	u16  part_index;	// WBFS partition index
	u16  n_part;		// number of partitions
	s16  wbfs_slot;		// slot number
	enumFileType ftype;	// the type of the file
	ccp  fname;		// filename, alloced
	FileAttrib_t fatt;	// file attributes: size, itime, mtime, ctime, atime

} WDiscListItem_t;

//-----------------------------------------------------------------------------

typedef struct WDiscList_t
{
	WDiscListItem_t * first_disc;
	u32 used;
	u32 size;
	u32 total_size_mib;
	SortMode sort_mode;

} WDiscList_t;


//
///////////////////////////////////////////////////////////////////////////////
///////////////			 dump files			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_ISO
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    ShowMode		show_mode,	// what should be printed
    int			dump_level	// dump level: 0..3, ignored if show_mode is set
);

enumError Dump_DOL
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
);

enumError Dump_TIK_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
);

enumError Dump_TIK_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_ticket_t	* tik		// valid pointer to ticket
);

enumError Dump_TMD_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
);

enumError Dump_TMD_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_tmd_t	* tmd,		// valid pointer to ticket
    int n_content			// number of loaded wd_tmd_content_t elementzs
);

enumError Dump_HEAD_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
);

enumError Dump_BOOT_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
);

enumError Dump_FST_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    ShowMode		show_mode	// what should be printed
);

enumError Dump_FST_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_fst_item_t	* ftab_data,	// the FST data
    size_t		ftab_size,	// size of FST data
    ccp			fname,		// NULL or source hint for error message
    wd_pfst_t		pfst		// print fst mode
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 source iterator                 ///////////////
///////////////////////////////////////////////////////////////////////////////

struct Iterator_t;
typedef enumError (*IteratorFunc) ( SuperFile_t * sf, struct Iterator_t * it );

//-----------------------------------------------------------------------------

typedef enum enumAction
{
	ACT_WARN,	// ignore with message
	ACT_IGNORE,	// ignore without message
	ACT_ALLOW,	// allow
	ACT_EXPAND,	// allow and expand (wbfs+sft only)

} enumAction;

//-----------------------------------------------------------------------------

typedef struct Iterator_t
{
	int		depth;		// current directory depth
	int		max_depth;	// maximal directory depth
	IteratorFunc	func;		// call back function
	ccp		real_path;	// pointer to real_path;

	// options

	bool		open_modify;	// open in modify mode
	enumAction	act_non_exist;	// action for non existing files
	enumAction	act_non_iso;	// action for non iso files
	enumAction	act_known;	// action for non iso files but well known files
	enumAction	act_wbfs;	// action for wbfs files with n(disc) != 1
	enumAction	act_fst;	// action for fst
	enumAction	act_open;	// action for open output files

	// source file list

	StringField_t	source_list;	// collect first than run
	int		source_index;	// informative: index of current file

	// statistics

	u32		num_of_files;	// number of found files
	u32		num_of_dirs;	// number of found directories
	u32		num_empty_dirs;	// number of empty base directories

	// user defined parameters, ignores by SourceIterator()

	ShowMode	show_mode;	// active show mode, initialized by opt_show_mode
	bool		scrub_it;	// SCRUB instead of COPY
	bool		update;		// update option set
	bool		newer;		// newer option set
	bool		overwrite;	// overwrite option set
	bool		remove_source;	// remove option set
	int		real_filename;	// set real filename without any selector
	int		long_count;	// long counter for output
	uint		done_count;	// done counter
	uint		diff_count;	// diff counter
	uint		exists_count;	// 'file alread exists' counter
	WDiscList_t	* wlist;	// pointer to WDiscList_t to collect data
	struct WBFS_t	* wbfs;		// open WBFS
	dev_t		open_dev;	// dev_t of open output file
	ino_t		open_ino;	// ino_t of open output file

} Iterator_t;

//-----------------------------------------------------------------------------

void InitializeIterator ( Iterator_t * it );
void ResetIterator ( Iterator_t * it );

enumError SourceIterator
(
	Iterator_t * it,
	int warning_mode,
	bool current_dir_is_default,
	bool collect_fnames
);

enumError SourceIteratorCollected ( Iterator_t * it, int warning_mode );

enumError SourceIteratorWarning ( Iterator_t * it, enumError max_err, bool silent );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global options			///////////////
///////////////////////////////////////////////////////////////////////////////

extern bool allow_fst;

extern wd_select_t part_selector;

extern u8 wdisc_usage_tab [WII_MAX_SECTORS];
extern u8 wdisc_usage_tab2[WII_MAX_SECTORS];

enumError ScanPartSelector
(
    wd_select_t * select,	// valid partiton selector
    ccp arg,			// argument to scan
    ccp err_text_extend		// error message extention
);

int ScanOptPartSelector ( ccp arg );
u32 ScanPartType ( ccp arg, ccp err_text_extend );

enumError ScanPartTabAndType
(
    u32		* res_ptab,		// NULL or result: partition table
    bool	* res_ptab_valid,	// NULL or result: partition table is valid
    u32		* res_ptype,		// NULL or result: partition type
    bool	* res_ptype_valid,	// NULL or result: partition type is valid
    ccp		arg,			// argument to analyze
    ccp		err_text_extend		// text to extent error messages
);

//-----------------------------------------------------------------------------

extern wd_ipm_t prefix_mode;
wd_ipm_t ScanPrefixMode ( ccp arg );
void SetupSneekMode();

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  Iso Mapping			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumIsoMapType
{
	IMT_ID,			// copy ID with respect to '.' as 'unchanged'
	IMT_DATA,		// raw data
	IMT_FILE,		// data := filename
	IMT_PART_FILES,		// files of partition
	IMT_PART,		// a partition
	IMT__N

} enumIsoMapType;

//-----------------------------------------------------------------------------

struct WiiFstPart_t;

typedef struct IsoMappingItem_t
{
	enumIsoMapType	imt;		// map type
	u64		offset;		// offset
	u64		size;		// size
	struct WiiFstPart_t *part;	// NULL or relevant partition
	void		*data;		// NULL or pointer to data
	bool		data_alloced;	// true if data must be freed.
	char		info[30];	// comment for dumps

} IsoMappingItem_t;

//-----------------------------------------------------------------------------

typedef struct IsoMapping_t
{
	IsoMappingItem_t *field;	// NULL or pointer to first item
	u32 used;			// used items
	u32 size;			// alloced items

} IsoMapping_t;

//-----------------------------------------------------------------------------

void InitializeIM ( IsoMapping_t * im );
void ResetIM ( IsoMapping_t * im );
IsoMappingItem_t * InsertIM
	( IsoMapping_t * im, enumIsoMapType imt, u64 offset, u64 size );

void PrintIM ( IsoMapping_t * im, FILE * f, int indent );

//
///////////////////////////////////////////////////////////////////////////////
///////////////                      Wii FST                    ///////////////
///////////////////////////////////////////////////////////////////////////////

#define FST_INCLUDE_FILE "include.fst"

enum // some const
{
	PARTITION_NAME_SIZE	= 20,

	DATA_PART_FOUND		= 1,
	UPDATE_PART_FOUND	= 2,
	CHANNEL_PART_FOUND	= 4,
};

extern ccp SpecialFilesFST[]; // NULL terminated list

//-----------------------------------------------------------------------------

typedef struct WiiFstFile_t
{
	u16		icm;			// wd_icm_t
	u32		offset4;		// offset in 4*bytes steps
	u32		size;			// size of file
	ccp		path;			// alloced path name
	u8		* data;			// raw data

} WiiFstFile_t;

//-----------------------------------------------------------------------------

typedef struct WiiFstPart_t
{
	//----- wd interface

	wd_part_t	* part;			// NULL or partition pointer

	//----- partition info

	u32		part_type;		// partition type
	u64		part_off;		// offset of partition relative to disc start
	u8		key[WII_KEY_SIZE];	// partition key
	aes_key_t	akey;			// partition aes key
	ccp		path;			// prefix path to partition
	wd_part_control_t * pc;			// ticket + cert + tmd + h3;

	//----- files

	WiiFstFile_t	* file;			// alloced list of files
	u32		file_used;		// number of used elements in 'file'
	u32		file_size;		// number of allocated elements in 'file'
	SortMode	sort_mode;		// current sort mode
	StringField_t	include_list;		// list of files with trailing '.'
	u64		total_file_size;	// total size of all files

	//----- generator data

	u8		*ftab;			// file table (fst.bin)
	u32		ftab_size;		// size of file table
	IsoMapping_t	im;			// iso mapping

	//----- status
	
	int done;				// set if operation was done

} WiiFstPart_t;

//-----------------------------------------------------------------------------

typedef struct WiiFst_t
{
	//----- wd interface

	wd_disc_t	* disc;			// NULL or disc pointer

	//----- partitions

	WiiFstPart_t	* part;			// partition infos
	u32		part_used;		// number of used elements in 'part'
	u32		part_size;		// number of allocated elements in 'part'
	WiiFstPart_t	* part_active;		// active partition

	// statistics

	u32		total_file_count;	// number of all files of all partition
	u64		total_file_size;	// total size of all files of all partition

	//----- options

	bool		ignore_dir;		// if true: don't collect dir names

	//----- file statistics
	
	u32		files_served;		// total number of files served
	u32		dirs_served;		// total number of files served
	u32		max_path_len;		// max path len of file[].path
	u32		fst_max_off4;		// maximal offset4 value of all files
	u32		fst_max_size;		// maximal size value of all files

	//----- generator data

	IsoMapping_t	im;			// iso mapping
	wd_header_t	dhead;			// disc header
	wd_region_t	region;			// region settings

	u8		*cache;			// cache with WII_GROUP_SECTORS elements
	WiiFstPart_t	*cache_part;		// partition of valid cache data
	u32		cache_group;		// partition group of cache data

	enumEncoding	encoding;		// the encoding mode

} WiiFst_t;

//-----------------------------------------------------------------------------

typedef struct WiiFstInfo_t
{
	SuperFile_t	* sf;			// NULL or pointer to input file
	WiiFst_t	* fst;			// NULL or pointer to file system
	WiiFstPart_t	* part;			// NULL or pointer to partion

	u32		total_count;		// total files to proceed
	u32		done_count;		// preceeded files
	u32		fw_done_count;		// field width of 'done_count'
	u64		done_file_size;		// helper variable for progress statistics
	u32		not_created_count;	// number of not created files+dirs

	FileAttrib_t	* set_time;		// NULL or set time attrib
	bool		overwrite;		// allow ovwerwriting
	int		verbose;		// the verbosity level

} WiiFstInfo_t;

//-----------------------------------------------------------------------------

void InitializeFST ( WiiFst_t * fst );
void ResetFST ( WiiFst_t * fst );
void ResetPartFST ( WiiFstPart_t * part );

WiiFstPart_t * AppendPartFST ( WiiFst_t * fst );
WiiFstFile_t * AppendFileFST ( WiiFstPart_t * part );
WiiFstFile_t * FindFileFST ( WiiFstPart_t * part, u32 offset4 );

int CollectFST
(
    WiiFst_t		* fst,		// valid fst pointer
    wd_disc_t		* disc,		// valid disc pointer
    FilePattern_t	* pat,		// NULL or a valid filter
    bool		ignore_dir,	// true: ignore directories
    wd_ipm_t		prefix_mode,	// prefix mode
    bool		store_prefix	// true: store full path incl. prefix
);

int CollectFST_BIN
(
    WiiFst_t		* fst,		// valid fst pointer
    const wd_fst_item_t * ftab_data,	// the FST data
    FilePattern_t	* pat,		// NULL or a valid filter
    bool		ignore_dir	// true: ignore directories
);

void DumpFilesFST
(
    FILE		* f,		// NULL or output file
    int			indent,		// indention of the output
    WiiFst_t		* fst,		// valid FST pointer
    wd_pfst_t		pfst,		// print fst mode
    ccp			prefix		// NULL or path prefix
);

u32 ScanPartFST ( WiiFstPart_t * part, ccp path, u32 cur_offset4, wd_boot_t * boot );

u64 GenPartFST
(
    SuperFile_t		* sf,		// valid file pointer
    WiiFstPart_t	* part,		// valid partition pointer
    ccp			path,		// path of partition directory
    u64			good_off,	// standard partition offset
    u64			min_off		// minimal possible partition offset
);

//-----------------------------------------------------------------------------

enumError CreateFST	( WiiFstInfo_t *wfi, ccp dest_path );
enumError CreatePartFST	( WiiFstInfo_t *wfi, ccp dest_path );
enumError CreateFileFST	( WiiFstInfo_t *wfi, ccp dest_path, WiiFstFile_t * file );

enumError ReadFileFST
(
	WiiFstPart_t *	part,		// valid fst partition pointer
	const WiiFstFile_t * file,	// valid fst file pointer
	u32		off,		// file offset
	void		* buf,		// destination buffer with at least 'size' bytes
	u32		size		// number of bytes to read
);

//-----------------------------------------------------------------------------

void SortFST ( WiiFst_t * fst, SortMode sort_mode, SortMode default_sort_mode );
void SortPartFST ( WiiFstPart_t * part, SortMode sort_mode, SortMode default_sort_mode );
void ReversePartFST ( WiiFstPart_t * part );

//-----------------------------------------------------------------------------

enumFileType IsFST     ( ccp path, char * id6_result );
enumFileType IsFSTPart ( ccp path, char * id6_result );

int SearchPartitionsFST
(
	ccp base_path,
	char * id6_result,	// NULL or result pointer
	char * data_part,	// NULL or result pointer
	char * update_part,	// NULL or result pointer
	char * channel_part	// NULL or result pointer
);

void PrintFstIM ( WiiFst_t * fst, FILE * f, int indent, bool print_part, ccp title );

enumError SetupReadFST ( SuperFile_t * sf );
enumError UpdateSignatureFST ( WiiFst_t * fst );

enumError ReadFST ( SuperFile_t * sf, off_t off, void * buf, size_t count );
enumError ReadPartFST ( SuperFile_t * sf, WiiFstPart_t * part,
			off_t off, void * buf, size_t count );
enumError ReadPartGroupFST ( SuperFile_t * sf, WiiFstPart_t * part,
				u32 group_no, void * buf, u32 n_groups );

void EncryptSectorGroup
(
    const aes_key_t	* akey,
    wd_part_sector_t	* sect0,
    size_t		n_sectors,
    void		* h3
);

void EncryptSectors
(
    const aes_key_t	* akey,
    const void		* sect_src,
    void		* sect_dest,
    size_t		n_sectors
);

void DecryptSectors
(
    const aes_key_t	* akey,
    const void		* sect_src,
    void		* sect_dest,
    size_t		n_sectors
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Verify_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct Verify_t
{
	// data structures

	SuperFile_t		* sf;
	u8			* usage_tab;
	wd_part_t		* part;

	// options, default are global options

	wd_select_t		* psel;		// NULL or partition selector
	int			verbose;	// general verbosity level
	int			long_count;	// verbosity for each message
	int			max_err_msg;	// max message per partition

	// statistical values, used for messages

	int			indent;		// indention of messages
	int			disc_index;	// disc index
	int			disc_total;	// number of total discs
	ccp			fname;		// printed filename

	// internal data

	u32			group;		// index of current sector group
	int			info_indent;	// indention of following messages

} Verify_t;

//-----------------------------------------------------------------------------

void InitializeVerify ( Verify_t * ver, SuperFile_t * sf );
void ResetVerify ( Verify_t * ver );
enumError VerifyPartition ( Verify_t * ver );
enumError VerifyDisc ( Verify_t * ver );

//
///////////////////////////////////////////////////////////////////////////////
///////////////                        END                      ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_ISO_INTERFACE_H
