
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
//	| wia_disc_t		|  disc data + partition table
//	+-----------------------+
//	| wia_part_t #0		|  fixed size partition header 
//	| wia_part_t #1		|  wia_disc_t::n_part elements
//	| ...			|
//	+-----------------------+
//	| partition info #0	|  dynamic length partition header
//	| partition info #1	|
//	| ...			|
//	+-----------------------+
//	| sector group data	|  partition data, managed in sector groups
//	| & raw disc data	|  & raw disc data
//	| ...			|  This data segments are stored in any order
//	+-----------------------+

// ***  partition info ***
//
//	+-----------------------+
//	| wd_part_header_t	|  copy of partition header incl. ticket
//	+-----------------------+
//	| tmd			|  copy of tmd
//	+-----------------------+
//	| cert			|  copy of cert
//	+-----------------------+
//	| h3			|  copy of h3
//	+-----------------------+
//	| wia_group_t[*]	|  wia_part_t::n_groups elements
//	+-----------------------+

// *** sector group data ***
//
//	+-----------------------+
//	| wia_exception_t[*]	|  wia_group_t::n_exceptions hash exceptions
//	+-----------------------+
//	| (un-)compressed data	|  compression dependent data
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

#define WIA_VERSION		0x00020000  // AABBCCDD = A.BB | A.BB.CC
#define WIA_VERSION_COMPATIBLE	0x00020000  // if D != 0xff => append 'beta' D

// the minimal size of holes in bytes that will be detected.

#define WIA_MIN_HOLE_SIZE	0x400

///////////////////////////////////////////////////////////////////////////////

typedef enum wia_compression_t
{
    WIA_COMPR_NONE,		// data is not compressed
    WIA_COMPR_BZIP2,		// use bzip2 compression

    WIA_COMPR__N,		// number of compressions
    WIA_COMPR__DEFAULT		= WIA_COMPR_BZIP2

} wia_compression_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_file_head_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_file_head_t
{
    // This data structure is never changed.
    // If additional info is needed, others will be expanded.
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

    u32			compression;		// 0x00: wia_compression_t


    //--- disc header, first WII_PART_OFF bytes of a disc

    wd_header_128_t	dhead;			// 0x04: 128 bytes of disc header
						//	 for a fast data access

    u64			disc_data_off;		// 0x84: data of offset 0x80 .. 0x50000
    u32			disc_data_size;		// 0x8c: compressed size of disc_data
						//	 -> a list of wia_data_segment_t


    //--- non specific raw disc data
    //--- not used yet, but reserved for future extensions

    u32			n_raw_data;		// 0x90: number of wia_raw_data_t elements 
    u64			raw_data_off;		// 0x94: offset of wia_raw_data_t[n_raw_data]
    sha1_hash		raw_data_hash;		// 0x9c: hash of wia_raw_data_t[n_raw_data]


    //--- partition data

    u32			n_part;			// 0xb0: number or partitions
    u32			part_t_size;		// 0xb4: size of 1 element of wia_part_t

    u64			part_off;		// 0xb8: file offset wia_part_t[n_part]
    sha1_hash		part_hash;		// 0xc0: hash of wia_part_t[n_part]

    u64			part_info_off;		// 0xd4: file offset of all partition info
    u32			part_info_size;		// 0xdc: size of all partition info
    sha1_hash		part_info_hash;		// 0xe0: hash of all partition info

} __attribute__ ((packed)) wia_disc_t;		// 0xf4 = 244 = sizeof(wia_disc_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_part_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_part_t
{
    // All values are stored in network byte order (big endian)

    //--- partition info

    u32			ptab_index;		// 0x00: zero based index of ptab
    u32			ptab_part_index;	// 0x04: zero based index within ptab
    u32			part_type;		// 0x08: partition type
    u64			part_off;		// 0x0c: disc offset of partition data
    u8			part_key[WII_KEY_SIZE];	// 0x14: partition key => build aes key

    //--- data sectors

    u32			first_sector;		// 0x24: first data sector
    u32			n_sectors;		// 0x28: number of sectors
    u32			n_groups;		// 0x2c: number of sector groups

    //--- all offsets are relative to wia_disc_t::part_info_off

    u32			ticket_off;		// 0x30: part_info offset of partition ticket
    u32			tmd_off;		// 0x34: part_info offset of partition tmd
    u32			tmd_size;		// 0x38: size of tmd
    u32			cert_off;		// 0x3c: part_info offset of partition cert
    u32			cert_size;		// 0x40: size of cert
    u32			h3_off;			// 0x44: part_info offset of partition h3
    u32			group_off;		// 0x48: p.i. offset of wia_group_t[n_groups]

} __attribute__ ((packed)) wia_part_t;		// 0x4c = 76 = sizeof(wia_part_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_raw_data_t		///////////////
///////////////////////////////////////////////////////////////////////////////


typedef struct wia_raw_data_t
{
    // All values are stored in network byte order (big endian)

    u32			file_off4;		// 0x00: file offset/4 of raw data
    u32			iso_off4;		// 0x04: iso offset/4 of raw dat
    u32			data_size;		// 0x08: size of raw data

} __attribute__ ((packed)) wia_raw_data_t;	// 0x0c = 12 = sizeof(wia_raw_data_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_group_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_group_t
{
    // All values are stored in network byte order (big endian)

    u32			data_off4;		// 0x00: file offset/4 of wia_data_t
    u32			data_size;		// 0x04: total wia_data_t size

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
///////////////			struct wia_data_segment_t	///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_data_segment_t
{
    // All values are stored in network byte order (big endian)

    u32			offset;			// 0x00: offset relative to group
    u32			size;			// 0x04: size of 'data'
    u8			data[0];		// 0x08: data

} __attribute__ ((packed)) wia_data_segment_t;	// 0x08 = 8 = sizeof(wia_data_segment_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_data_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_data_t
{
    u32			seg_offset;		// 0x00: offset of first segment
    u16			n_exceptions;		// 0x04: number of hash exceptions
    wia_exception_t	exception[0];		// 0x06: hash exceptions

//  wia_data_segment_t	data[0];		// ----: list of data segments
//  wia_data_segment_t	term;			// ----: terminating data segment (0,0)

//  sha1_hash		hash;			// ----: a hash value
						//	 only for uncompressed data

} __attribute__ ((packed)) wia_data_t;		// 0x06 = 6 = sizeof(wia_data_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_controller_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_controller_t
{
    wd_disc_t		* wdisc;	// valid if writing

    wia_file_head_t	fhead;		// header in local endian
    wia_disc_t		disc;		// disc data

    wia_part_t		* part;		// partition data
    u8			* part_info;	// partition info of all partitions
    u32			part_info_size;	// size of partition info

    wd_patch_t		memmap;		// memory mapping
    bool		encrypt;	// true: encrypt dats if reading
    bool		is_valid;	// true: WIA header is valid

    u64			write_data_off;	// file offset for the next data

    u8  disc_data[WII_PART_OFF-sizeof(wd_header_128_t)];
					// expanded disc header

    u8	gdata[WII_GROUP_SIZE];		// group data
    u32	gdata_part_index;		// partition index of current group data
    u32	gdata_group;			// group index of current group data

    u8  iobuf [ WII_GROUP_DATA_SIZE	// temporary iobuf
		+ WII_N_HASH_GROUP * sizeof(wia_exception_t)
		+ 0x100 ];

} wia_controller_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    	interface		///////////////
///////////////////////////////////////////////////////////////////////////////

struct SuperFile_t;

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

ccp GetCompressionNameWIA
(
    wia_compression_t	compr,		// compression mode
    ccp			invalid_result	// return value if 'compr' is invalid
);

//-----------------------------------------------------------------------------

bool IsWIA
(
    const void		* data,		// data to check
    size_t		data_size,	// size of data
    void		* id6_result	// not NULL: store ID6 (6 bytes without null term)
);

//-----------------------------------------------------------------------------

void SetupMemMap
(
    wia_controller_t	* wia		// valid pointer
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SuperFile_t interface		///////////////
///////////////////////////////////////////////////////////////////////////////
// WIA reading support

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
    struct SuperFile_t	* src		// NULL or source file
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

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_WIA_H
