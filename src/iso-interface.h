
#ifndef WIT_ISO_INTERFACE_H
#define WIT_ISO_INTERFACE_H 1

#include "types.h"
#include "lib-sf.h"
#include "match-pattern.h"
#include "libwbfs/libwbfs.h"
#include "libwbfs/rijndael.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  wii iso discs                  ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct WDPartInfo_t
{
	u32 ptable;			// index of partition table
	u32 index;			// index within partition table
	u32 ptype;			// partition type
	off_t off;			// offset of the partition
	off_t size;			// partition size
	wd_part_header_t ph;		// partition header
	u8  part_key[16];		// partition key

	// status: -1:unknown, 0:false, 1:true
	char is_marked_not_enc;		// is partition marked 'not encrypted'? (<0: unknown)
	char is_encrypted;		// is partition encrypted? (<0: unknown)
	char is_trucha_signed;		// is partition trucha signed? (<0: unknown)

} __attribute__ ((packed)) WDPartInfo_t;

///////////////////////////////////////////////////////////////////////////////

typedef struct WDiscInfo_t
{
	wd_header_t dhead;

	uint disc_index;
	char id6[7];
	u64  size;
	u64  iso_size;
	u32  used_blocks;
	ccp  region4;
	ccp  region;
	ccp  title;			// pointer to title DB

	// partitions
	u32 n_ptab;			// number f active partition tables
	u32 n_part;			// number or partitions
	WDPartInfo_t	* pinfo;	// field with 'n_part' elements

	// raw data
	wd_part_count_t	pcount[WII_MAX_PART_INFO];
	wd_region_set_t	regionset;

} WDiscInfo_t;

//-----------------------------------------------------------------------------

typedef struct WDiscListItem_t
{
	u32  size_mib;		// size of the source in MiB
	u32  used_blocks;	// number of used ISO blocks
	char id6[7];		// ID6
	char name64[65];	// disc name from header
	char region4[5];	// region info
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
	FILE * f,		// output stream
	int indent,		// indent
	SuperFile_t * sf,	// file to dump
	ccp real_path,		// NULL or pointer to real path
	int dump_level		// dump level: 0..2
);

enumError Dump_DOL
(
	FILE * f,		// output stream
	int indent,		// indent
	SuperFile_t * sf,	// file to dump
	ccp real_path,		// NULL or pointer to real path
	int dump_level		// dump level: 0..2
);

enumError Dump_BOOT_BIN
(
	FILE * f,		// output stream
	int indent,		// indent
	SuperFile_t * sf,	// file to dump
	ccp real_path,		// NULL or pointer to real path
	int dump_level		// dump level: 0..2
);

enumError Dump_FST_BIN
(
	FILE * f,		// output stream
	int indent,		// indent
	SuperFile_t * sf,	// file to dump
	ccp real_path,		// NULL or pointer to real path
	int dump_level		// dump level: 0..2
);

enumError Dump_FST
(
	FILE * f,				// output stream or NULL if silent
	int indent,				// indent
	const wd_fst_item_t * ftab_data,	// the FST data
	size_t ftab_size,			// size of FST data
	ccp fname				// filename or hint
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
	int depth;			// current directory depth
	int max_depth;			// maximal directory depth
	IteratorFunc func;		// call back function
	ccp real_path;			// pointer to real_path;

	// options

	bool open_modify;		// open in modify mode
	enumAction act_non_exist;	// action for non existing files
	enumAction act_non_iso;		// action for non iso files
	enumAction act_known;		// action for non iso files but well known files
	enumAction act_wbfs;		// action for wbfs files with n(disc) != 1
	enumAction act_fst;		// action for fst
	enumAction act_open;		// action for open output files

	// source file list

	StringField_t source_list;	// collect first than run
	int source_index;		// informative: index of current file

	// user defined parameters, ignores by SourceIterator()

	bool scrub_it;		// SCRUB instead of COPY
	bool update;		// update option set
	bool newer;		// newer option set
	bool overwrite;		// overwrite option set
	bool remove_source;	// remove option set
	int  real_filename;	// set real filename without any selector
	int  long_count;	// long counter for output
	uint done_count;	// done counter
	uint diff_count;	// diff counter
	uint exists_count;	// 'file alread exists' counter
	WDiscList_t * wlist;	// pointer to WDiscList_t to collect data
	struct WBFS_t * wbfs;	// open WBFS
	dev_t open_dev;		// dev_t of open output file
	ino_t open_ino;		// ino_t of open output file

} Iterator_t;

//-----------------------------------------------------------------------------

void InitializeIterator ( Iterator_t * it );
void ResetIterator ( Iterator_t * it );

enumError SourceIterator
	( Iterator_t * it, bool current_dir_is_default, bool collect_fnames );

enumError SourceIteratorCollected ( Iterator_t * it );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global options			///////////////
///////////////////////////////////////////////////////////////////////////////

extern partition_selector_t partition_selector;
extern u8 wdisc_usage_tab [WII_MAX_SECTORS];
extern u8 wdisc_usage_tab2[WII_MAX_SECTORS];

partition_selector_t ScanPartitionSelector ( ccp arg );

//-----------------------------------------------------------------------------

extern iterator_prefix_mode_t prefix_mode;
iterator_prefix_mode_t ScanPrefixMode ( ccp arg );
void SetupSneekMode();

//-----------------------------------------------------------------------------

typedef enum enumEncoding
{
	// some flags

	ENCODE_F_FAST		= 0x0001, // fast encoding wanted

	// the basic jobs

	ENCODE_CLEAR_HASH	= 0x0010, // clear hash area of each sector
	ENCODE_CALC_HASH	= 0x0020, // calc hash values for each sector
	ENCODE_DECRYPT		= 0x0100, // decrypt sectors
	ENCODE_ENCRYPT		= 0x0200, // encrypt sectors
	ENCODE_NO_SIGN		= 0x1000, // clear signing area
	ENCODE_SIGN		= 0x2000, // trucha sign
	
	// the masks

	ENCODE_M_HASH		= ENCODE_CLEAR_HASH | ENCODE_CALC_HASH,
	ENCODE_M_CRYPT		= ENCODE_DECRYPT | ENCODE_ENCRYPT,
	ENCODE_M_SIGN		= ENCODE_NO_SIGN | ENCODE_SIGN,
	
	ENCODE_MASK		= ENCODE_M_HASH | ENCODE_M_CRYPT | ENCODE_M_SIGN
				| ENCODE_F_FAST,

	ENCODE_DEFAULT		= 0,	// initial value

} enumEncoding;

extern enumEncoding encoding;
enumEncoding ScanEncoding ( ccp arg );
enumEncoding SetEncoding
	( enumEncoding val, enumEncoding set_mask, enumEncoding default_mask );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  Iso Mapping			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumIsoMapType
{
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
	u64		offset;		// offset/4
	u64		size;		// size/4
	struct WiiFstPart_t *part;	// NULL or relevant partition
	void		*data;		// NULL or pointer to data
	bool		data_alloced;	// true if data must be freed.
	ccp		comment;	// comment for dumps

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

void DumpIM ( IsoMapping_t * im, FILE * f, int indent );

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
	u16 icm;			// iterator_call_mode_t
	u32 offset4;			// offset in 4*bytes steps
	u32 size;			// size of file
	ccp path;			// alloced path name
	u8 *data;			// raw data

} WiiFstFile_t;

//-----------------------------------------------------------------------------

typedef struct WiiFstPart_t
{
	// partition info

	u64  part_offset;		// disc offset of partition header
	u32  part_index;		// partition index
	u32  part_type;			// partition type
	u8   part_key[WII_KEY_SIZE];	// partition key
	aes_key_t part_akey;		// partition aes key
	char is_marked_not_enc;		// is partition marked 'not encrypted'? (<0: unknown)
	char is_encrypted;		// is partition encrypted? (<0: unknown)
	char is_trucha_signed;		// is partition trucha signed? (<0: unknown)
	ccp  path;			// prefix path to partition
	wd_part_control_t * pc;		// ticket + cert + tmd + h3;

	// files

	WiiFstFile_t * file;		// alloced list of files
	u32  file_used;			// number of used elements in 'file'
	u32  file_size;			// number of allocated elements in 'file'
	SortMode sort_mode;		// current sort mode
	StringField_t include_list;	// list of files with trailing '.'
	u64 total_file_size;		// total size of all files

	// generator data

	u8  *ftab;			// file table (fst.bin)
	u32 ftab_size;			// size of file table
	IsoMapping_t im;		// iso mapping

	// status
	
	int done;			// set if operation was done

} WiiFstPart_t;

//-----------------------------------------------------------------------------

typedef struct WiiFst_t
{
	//----- partitions

	WiiFstPart_t * part;		// partition infos
	u32 part_used;			// number of used elements in 'part'
	u32 part_size;			// number of allocated elements in 'part'
	WiiFstPart_t * part_active;	// active partition

	// statistics

	u32 total_file_count;		// number of all files of all partition
	u64 total_file_size;		// total size of all files of all partition

	//----- options

	bool ignore_dir;		// if true: don't collect dir names

	//----- file statistics
	
	u32 files_served;		// total number of files served
	u32 dirs_served;		// total number of files served
	u32 max_path_len;		// max path len of file[].path

	//----- generator data

	IsoMapping_t im;		// iso mapping
	wd_header_t disc_header;	// disc header
	wd_region_set_t region_set;	// region settings

	u8		*cache;		// cache with WII_GROUP_SECTORS elements
	WiiFstPart_t	*cache_part;	// partition of valid cache data
	u32		cache_group;	// partition group of cache data

	enumEncoding encoding;		// the encoding mode

} WiiFst_t;

//-----------------------------------------------------------------------------

typedef struct WiiFstInfo_t
{
	SuperFile_t	* sf;		// NULL or pointer to input file
	wiidisc_t	* disc;		// NULL or pointer to input disc
	WiiFst_t	* fst;		// NULL or pointer to file system
	WiiFstPart_t	* part;		// NULL or pointer to partion

	u32 total_count;		// total files to proceed
	u32 done_count;			// preceeded files
	u32 fw_done_count;		// field width of 'done_count'
	u64 done_file_size;		// helper variable for progress statstics

	FileAttrib_t	* set_time;	// NULL or set time attrib
	bool		overwrite;	// allow ovwerwriting
	int		verbose;	// the verbosity level
} WiiFstInfo_t;

//-----------------------------------------------------------------------------

void InitializeFST ( WiiFst_t * fst );
void ResetFST ( WiiFst_t * fst );
void ResetPartFST ( WiiFstPart_t * part );

WiiFstPart_t * AppendPartFST ( WiiFst_t * fst );
WiiFstFile_t * AppendFileFST ( WiiFstPart_t * part );
WiiFstFile_t * FindFileFST ( WiiFstPart_t * part, u32 offset4 );

int CollectFST ( wiidisc_t * disc, iterator_call_mode_t imode,
			u32 offset4, u32 size, const void * data );
int CollectPartitions ( wiidisc_t * disc, iterator_call_mode_t imode,
			u32 offset4, u32 size, const void * data );

u32 ScanPartFST ( WiiFstPart_t * part, ccp path, u32 cur_offset4, wd_boot_t * boot );
u64 GenPartFST  ( SuperFile_t * sf, WiiFstPart_t * part, ccp path, u64 base_off );

//-----------------------------------------------------------------------------

enumError CreateFST	( WiiFstInfo_t *wfi, ccp dest_path );
enumError CreatePartFST	( WiiFstInfo_t *wfi, ccp dest_path );
enumError CreateFileFST	( WiiFstInfo_t *wfi, ccp dest_path, WiiFstFile_t * file );

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
	wiidisc_t		* disc;
	u8			* usage_tab;
	WiiFst_t		* fst;
	WiiFstPart_t		* part;

	// options, default are global options

	partition_selector_t	selector;	// partiton selector
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
///////////////			IsoFileIterator_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct IsoFileIterator_t
{
	SuperFile_t	* sf;
	FilePattern_t	* pat;
	WiiFst_t	* fst;

} IsoFileIterator_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                        END                      ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_ISO_INTERFACE_H
