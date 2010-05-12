#ifndef WIIDISC_H
#define WIIDISC_H

#include <stdio.h>
#include "file-formats.h"
#include "rijndael.h"

///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
#if 0 //removes extra automatic indentation by editors
}
#endif

///////////////////////////////////////////////////////////////////////////////

// callback definition. Return 1 on fatal error
// (callback is supposed to make retries until no hopes..)
// offset points 32bit words, count counts bytes

typedef int (*read_wiidisc_callback_t)( void *fp, u32 offset, u32 count, void* iobuf );

///////////////////////////////////////////////////////////////////////////////

typedef enum partition_selector_t
{
    DATA_PARTITION_TYPE		= 0,
    UPDATE_PARTITION_TYPE	= 1,
    CHANNEL_PARTITION_TYPE	= 2,

    // value in between selects partition types of that value

    WHOLE_DISC			= -6,   // copy whole disc
    ALL_PARTITIONS		= -5,   // copy all partitions

    REMOVE_DATA_PARTITION	= -4,	// all but DATA_PARTITION_TYPE
    REMOVE_UPDATE_PARTITION	= -3,	// all but UPDATE_PARTITION_TYPE
    REMOVE_CHANNEL_PARTITION	= -2,	// all but CHANNEL_PARTITION_TYPE
				//-1 = reserved as error indicator

} partition_selector_t;

///////////////////////////////////////////////////////////////////////////////

typedef enum iterator_call_mode_t
{
    ICM_OPEN_DISC,		// called if disc opened
    ICM_CLOSE_DISC,		// called if disc closed
    ICM_OPEN_PARTITION,		// called for each openening of a partition
    ICM_CLOSE_PARTITION,	// called if partitions closed

    ICM_DIRECTORY,		// item is a directory; this is the first non hint message

    ICM_FILE,			// item is a partition file -> extract with key
    ICM_COPY,			// item is a disc area -> disc raw copy
    ICM_DATA,			// item contains pure data [not used yet]

} iterator_call_mode_t;


// callback definition for file iteration. if return != 0 => abort
struct wiidisc_s;
typedef int (*file_callback_t)
(
    struct wiidisc_s *d,
    iterator_call_mode_t icm,
    u32 offset4,
    u32 size,
    const void * data
);

///////////////////////////////////////////////////////////////////////////////

typedef enum iterator_prefix_mode_t
{
    IPM_DEFAULT,	// not defined -> like AUTO in wiidisc
    IPM_AUTO,		// for single partitions: IPM_PART_NAME, else: IPM_POINT

    IPM_NONE,		// no prefix: ""
    IPM_POINT,		// prefix with "./"
    IPM_PART_IDENT,	// prefix with 'P' and partition id: "P%u/"
    IPM_PART_NAME,	// prefix with partition name or "P<id>": "NAME/"

} iterator_prefix_mode_t;

///////////////////////////////////////////////////////////////////////////////

typedef struct wiidisc_s
{
    read_wiidisc_callback_t read;	// read-data-function
    void *fp;				// file handle (black box)
    u8 *usage_table;			// if not NULL: calculate usage
					//	size = WII_MAX_SECTORS
    u8  usage_marker;			// use value to sector_usage_table

    // everything points 32bit words.
    u32 disc_raw_offset;
    u32 partition_raw_offset4;		// offset/4 of beginning of current partition
    u32 partition_data_offset4;		// data offset/4 relative to begin of partition
    u32 partition_data_size4;		// data offset/4 relative to begin of partition
    u32 partition_block;		// offset/WII_SECTOR_SIZE of current data
    u32 partition_offset4;		// := partition_block * (0x7c00>>2)
    u8  partition_key[WII_KEY_SIZE];	// partition key, informative
    aes_key_t partition_akey;		// partition aes key, needed for decryption
    int is_marked_not_enc;	// not 0 if partition marked 'not encrypted'
    int is_encrypted;			// not 0 if partition is encrypted
    int is_trucha_signed;		// not 0 if partition is trucha signed

    u8 *block_buffer;			// block read buffer
    u32 last_block;			// last readed block
    
    u8 *tmp_buffer;			// temp buffer with WII_SECTOR_SIZE bytes
    u8 *tmp_buffer2;			// temp buffer with WII_SECTOR_SIZE bytes

    int open_partition;			// <0:off | >=0: open partition with index
    partition_selector_t part_sel;	// partition selector
    int partition_index;		// zero based partition index
    u32 partition_type;			// current partition type

    iterator_prefix_mode_t prefix_mode;	// prefix mode
    char path_prefix[20];		// prefix of all pathes
    char path[WII_FILE_PATH_SIZE];	// current path
    u32  path_len;			// current length of path -> strlen()
    u32  max_path_len;			// maximal length of path -> strlen()
    file_callback_t file_iterator;	// if not NULL: call for each file
    void * user_param;			// user defined parameter
    u32  dir_count;			// number of iterated real directories
    u32  file_count;			// number of iterated real files

} wiidisc_t;

///////////////////////////////////////////////////////////////////////////////

extern const char * wd_partition_name[];
const char * wd_get_partition_name ( u32 ptype, const char * result_if_not_found );
char * wd_print_partition_name ( char * buf, u32 buf_size, u32 ptype, int print_num );

const u8 * wd_get_common_key();
const u8 * wd_set_common_key ( const u8 * new_key );
void wd_decrypt_title_key ( wd_ticket_t * tik, u8 * title_key );

///////////////////////////////////////////////////////////////////////////////

wiidisc_t *wd_open_disc
(
	read_wiidisc_callback_t read,
	void * fp
);

wiidisc_t *wd_open_partition
(
	read_wiidisc_callback_t read,
	void * fp,
	int open_part_index,
	partition_selector_t selector
);

int wd_read_raw ( wiidisc_t * d, u32 offset4, void * data, u32 len );
int wd_read_block ( wiidisc_t *d, const aes_key_t * akey, u32 blockno, u8 * block );
int wd_read ( wiidisc_t *d, const aes_key_t * akey,
		u32 offset4, void * destbuf, u32 len, int do_not_read );
int wd_is_block_encrypted
	( wiidisc_t *d, const aes_key_t * akey, u32 block, int unknown_result );

void wd_close_disc( wiidisc_t * );

void wd_build_disc_usage
(
	wiidisc_t * d,
	partition_selector_t selector,
	u8 * usage_table,
	u64 iso_size	// only used if selector==WHOLE_DISC, zero if not known
);

void wd_iterate_files
(
	wiidisc_t * d,
	partition_selector_t selector,
	iterator_prefix_mode_t prefix_mode,
	file_callback_t file_iterator,
	void * param,
	u8 * usage_table,
	u64 iso_size
);

// effectively remove not copied partition from the partition table.
void wd_fix_partition_table
	( wiidisc_t * d, partition_selector_t selector, u8 * partition_table );

int wd_rename
(
	void * data,		// pointer to ISO data
	const char * new_id,	// if !NULL: take the first 6 chars as ID
	const char * new_title	// if !NULL: take the first 0x39 chars as title
);

///////////////////////////////////////////////////////////////////////////////

#if 0
{
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

///////////////////////////////////////////////////////////////////////////////

#endif // WIIDISC_H
