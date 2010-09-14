
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

#ifndef WIT_LIB_WIA_H
#define WIT_LIB_WIA_H 1

#include "types.h"
#include "lib-std.h"
#include "libwbfs/wiidisc.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			WIA file layout			///////////////
///////////////////////////////////////////////////////////////////////////////

// *** file layout ***
//
//	+-----------------------+
//	| wia_file_head_t	|  data structure will never changed
//	+-----------------------+
//	| wia_disc_t		|  disc data
//	+-----------------------+
//	| wia_part_t #0		|  fixed size partition header 
//	| wia_part_t #1		|  wia_disc_t::n_part elements
//	| ...			|   -> data stored in wia_group_t elements
//	+-----------------------+
//	| data chunks		|  stored in any order
//	| ...			|
//	+-----------------------+


// *** data chunks (any order) ***
//
//	+-----------------------+
//	| compressor data	|  data for compressors (e.g properties)
//	| compressed chunks	|
//	+-----------------------+


// *** compressed chunks (any order) ***
//
//	+-----------------------+
//	| raw data table	|
//	| group table		|
//	| raw data		|  <- divided in groups (e.g partition .header)
//	| partition data	|  <- divided in groups
//	+-----------------------+


// *** partition data (Wii format) ***
//
//	+-----------------------+
//	| wia_except_list	|  wia_group_t::n_exceptions hash exceptions
//	+-----------------------+
//	| (compressed) data	|  compression dependent data
//	+-----------------------+


// *** other data (non partition data) ***
//
//	+-----------------------+
//	| (compressed) data	|  compression dependent data
//	+-----------------------+

//
///////////////////////////////////////////////////////////////////////////////
///////////////			WIA definitions			///////////////
///////////////////////////////////////////////////////////////////////////////

// First a magic is defined to identify a WIA clearly. The magic should never be
// a possible Wii ISO image identification. Wii ISO images starts with the ID6.
// And so the WIA magic contains one contral character (CTRL-A) within.

#define WIA_MAGIC		"WIA\1"
#define WIA_MAGIC_SIZE		4

//-----------------------------------------------------
// Format of version number: AABBCCDD = A.BB | A.BB.CC
// If D != 0x00 && D != 0xff => append: 'beta' D
//-----------------------------------------------------

#define WIA_VERSION		0x00070000  // current writing version
#define WIA_VERSION_COMPATIBLE	0x00060000  // down compatible

// the minimal size of holes in bytes that will be detected.

#define WIA_MIN_HOLE_SIZE	0x400

// the maximal supported iso size

#define WIA_MAX_SUPPORTED_ISO_SIZE (50ull*GiB)

///////////////////////////////////////////////////////////////////////////////

typedef enum wia_compression_t
{

    //**********************************************************************
    //***  never change the values, because they are used in arvchives!  ***
    //**********************************************************************

    WIA_COMPR_NONE	= 0,	// data is not compressed
    WIA_COMPR_PURGE,		// data is not compressed but zero holes are purged
    WIA_COMPR_BZIP2,		// use bzip2 compression

    WIA_COMPR__N,		// number of compressions

    WIA_COMPR__DEFAULT	= WIA_COMPR_BZIP2,	// default compression
    WIA_COMPR__FAST	= WIA_COMPR_PURGE,	// fast compression
    WIA_COMPR__BEST	= WIA_COMPR_BZIP2,	// best compression

} wia_compression_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_file_head_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_file_head_t
{
    // This data structure is never changed.
    // If additional info is needed, others structures will be expanded.
    // All values are stored in network byte order (big endian)

    char		magic[WIA_MAGIC_SIZE];	// 0x00: WIA_MAGIC, what else!

    u32			version;		// 0x04: WIA_VERSION
    u32			version_compatible;	// 0x08: compatible down to

    u32			disc_size;		// 0x0c: size of wia_disc_t
    sha1_hash		disc_hash;		// 0x10: hash of wia_disc_t

    u64			iso_file_size;		// 0x24: size of ISO image
    u64			wia_file_size;		// 0x2c: size of WIA file

    sha1_hash		file_head_hash;		// 0x34: hash of wia_file_head_t

} __attribute__ ((packed)) wia_file_head_t;	// 0x48 = 72 = sizeof(wia_file_head_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_disc_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_disc_t
{
    // All values are stored in network byte order (big endian)

    //--- base infos

    u32			disc_type;		// 0x00: wd_disc_type_t
    u32			compression;		// 0x04: wia_compression_t

    //--- disc header, first 0x80 bytes of source for easy detection 

    u8			dhead[0x80];		// 0x08: 128 bytes of disc header
						//	 for a fast data access
    //--- partition data, direct copy => hash needed

    u32			n_part;			// 0x88: number or partitions
    u32			part_t_size;		// 0x8c: size of 1 element of wia_part_t
    u64			part_off;		// 0x90: file offset wia_part_t[n_part]
    sha1_hash		part_hash;		// 0x98: hash of wia_part_t[n_part]

    //--- raw data, compressed

    u32			n_raw_data;		// 0xac: number of wia_raw_data_t elements 
    u64			raw_data_off;		// 0xb0: offset of wia_raw_data_t[n_raw_data]
    u32			raw_data_size;		// 0xb8: conpressed size of raw data

    //--- group header, compressed

    u32			n_groups;		// 0xbc: number of wia_group_t elements
    u64			group_off;		// 0xc0: offset of wia_group_t[n_groups]
    u32			group_size;		// 0xc8: conpressed size of groups

    //--- compressor dependent data (e.g. LZMA properties)

    u8			compr_data_len;		// 0xcc: used length of 'compr_data'
    u8			compr_data[7];		// 0xcd: compressor specific data 

} __attribute__ ((packed)) wia_disc_t;		// 0xd4 = 212 = sizeof(wia_disc_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_part_t		///////////////
///////////////////////////////////////////////////////////////////////////////

// This data structure is only used for Wii like partitions but nor for GC.
// It supports decrypting and hash removing.

typedef struct wia_part_t
{
    // All values are stored in network byte order (big endian)

    u8			part_key[WII_KEY_SIZE];	// 0x00: partition key => build aes key

    u32			first_sector;		// 0x10: first data sector
    u32			n_sectors;		// 0x14: number of sectors

    u32			group_index;		// 0x18: index of first wia_group_t 
    u32			n_groups;		// 0x1c: number of wia_group_t elements
    
} __attribute__ ((packed)) wia_part_t;		// 0x20 = 32 = sizeof(wia_part_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_raw_data_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_raw_data_t
{
    // All values are stored in network byte order (big endian)

    u64			raw_data_off;		// 0x00: disc offset of raw data
    u64			raw_data_size;		// 0x08: size of raw data
    
    u32			group_index;		// 0x10: index of first wia_group_t 
    u32			n_groups;		// 0x14: number of wia_group_t elements

} __attribute__ ((packed)) wia_raw_data_t;	// 0x18 = 24 = sizeof(wia_raw_data_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_group_t		///////////////
///////////////////////////////////////////////////////////////////////////////

// Each group points to a data segment list. Size of last element is null.
// Partition data groups are headed by a hash exeption list (wia_except_list_t).

typedef struct wia_group_t
{
    // All values are stored in network byte order (big endian)

    u32			data_off4;		// 0x00: file offset/4 of data
    u32			data_size;		// 0x04: file size of data

} __attribute__ ((packed)) wia_group_t;		// 0x08 = 8 = sizeof(wia_group_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_exception_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_exception_t
{
    // All values are stored in network byte order (big endian)

    u16			offset;			// 0x00: sector offset of hash
    sha1_hash		hash;			// 0x02: hash value

} __attribute__ ((packed)) wia_exception_t;	// 0x16 = 22 = sizeof(wia_exception_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_except_list_t	///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_except_list_t
{
    u16			n_exceptions;		// 0x00: number of hash exceptions
    wia_exception_t	exception[0];		// 0x02: hash exceptions

} __attribute__ ((packed)) wia_except_list_t;	// 0x02 = 2 = sizeof(wia_except_list_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_segment_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_segment_t
{
    // All values are stored in network byte order (big endian)
    // Segments are used to reduce the size of uncompressed data (method PURGE)
    // This is done by eleminatin holes (zeroed areas)

    u32			offset;			// 0x00: offset relative to group
    u32			size;			// 0x04: size of 'data'
    u8			data[0];		// 0x08: data

} __attribute__ ((packed)) wia_segment_t;	// 0x08 = 8 = sizeof(wia_segment_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_controller_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_controller_t
{
    wd_disc_t		* wdisc;	// valid if writing

    wia_file_head_t	fhead;		// header in local endian
    wia_disc_t		disc;		// disc data

    wia_part_t		* part;		// NULL or pointer to partition info

    wia_raw_data_t	* raw_data;	// NULL or pointer to raw data list
    u32			raw_data_used;	// number of used 'group' elements
    u32			raw_data_size;	// number of alloced 'group' elements
    wia_raw_data_t	* growing;	// NULL or pointer to last element of 'raw_data'
					// used for writing behind expected file size

    wia_group_t		* group;	// NULL or pointer to group list
    u32			group_used;	// number of used 'group' elements
    u32			group_size;	// number of alloced 'group' elements

    wd_memmap_t		memmap;		// memory mapping

    bool		encrypt;	// true: encrypt data if reading
    bool		is_writing;	// false: read a WIA / true: write a WIA
    bool		is_valid;	// true: WIA is valid

    u64			write_data_off;	// writing file offset for the next data

    //----- group data cache

    u8			gdata[WII_GROUP_SIZE];	// group data
    u32			gdata_size;	// relevant size of 'gdata'
    int			gdata_group;	// index of current group, -1:invalid
    int			gdata_part;	// partition index of current group data

    //----- temporary iobuf

    u8  iobuf [ WII_GROUP_SIZE + WII_N_HASH_GROUP * sizeof(wia_exception_t) ];

} wia_controller_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    interface			///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetWIA
(
    wia_controller_t	* wia		// NULL or valid pointer
);

//-----------------------------------------------------------------------------

char * PrintVersionWIA
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u32			version		// version number to print
);

//-----------------------------------------------------------------------------

bool IsWIA
(
    const void		* data,		// data to check
    size_t		data_size,	// size of data
    void		* id6_result,	// not NULL: store ID6 (6 bytes without null term)
    wd_disc_type_t	* disc_type,	// not NULL: store disc type
    wia_compression_t	* compression	// not NULL: store compression
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SuperFile_t interface		///////////////
///////////////////////////////////////////////////////////////////////////////
// WIA reading support

struct SuperFile_t;

enumError SetupReadWIA
(
    struct SuperFile_t	* sf		// file to setup
);

//-----------------------------------------------------------------------------

enumError ReadWIA
(
    struct SuperFile_t	* sf,		// source file
    off_t		off,		// file offset
    void		* buf,		// destination buffer
    size_t		count		// number of bytes to read
);

//-----------------------------------------------------------------------------
// WIA writing support

enumError SetupWriteWIA
(
    struct SuperFile_t	* sf,		// file to setup
    struct SuperFile_t	* src,		// NULL or source file
    u64			src_file_size	// NULL or source file size
);

//-----------------------------------------------------------------------------

enumError TermWriteWIA
(
    struct SuperFile_t	* sf		// file to terminate
);

//-----------------------------------------------------------------------------

enumError WriteWIA
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    const void		* buf,		// source buffer
    size_t		count		// number of bytes to write
);

//-----------------------------------------------------------------------------

enumError WriteSparseWIA
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    const void		* buf,		// source buffer
    size_t		count		// number of bytes to write
);

//-----------------------------------------------------------------------------

enumError WriteZeroWIA
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    size_t		count		// number of bytes to write
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			endian conversions		///////////////
///////////////////////////////////////////////////////////////////////////////

void wia_ntoh_file_head ( wia_file_head_t * dest, const wia_file_head_t * src );
void wia_hton_file_head ( wia_file_head_t * dest, const wia_file_head_t * src );

void wia_ntoh_disc ( wia_disc_t * dest, const wia_disc_t * src );
void wia_hton_disc ( wia_disc_t * dest, const wia_disc_t * src );

void wia_ntoh_part ( wia_part_t * dest, const wia_part_t * src );
void wia_hton_part ( wia_part_t * dest, const wia_part_t * src );

void wia_ntoh_raw_data ( wia_raw_data_t * dest, const wia_raw_data_t * src );
void wia_hton_raw_data ( wia_raw_data_t * dest, const wia_raw_data_t * src );

//
///////////////////////////////////////////////////////////////////////////////
///////////////		interface: compression option		///////////////
///////////////////////////////////////////////////////////////////////////////

extern wia_compression_t opt_compression; // = WIA_COMPR__DEFAULT

//-----------------------------------------------------------------------------

ccp GetCompressionName
(
    wia_compression_t	compr,		// compression mode
    ccp			invalid_result	// return value if 'compr' is invalid
);

//-----------------------------------------------------------------------------

wia_compression_t ScanCompression
(
    ccp			arg		// argument to scan
);

//-----------------------------------------------------------------------------

int ScanOptCompression
(
    ccp			arg		// argument to scan
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_WIA_H
