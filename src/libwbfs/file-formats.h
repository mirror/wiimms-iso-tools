
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

#ifndef FILE_FORMATS_H
#define FILE_FORMATS_H

// some info urls:
//  - http://wiibrew.org/wiki/Wii_Disc
//  - http://wiibrew.org/wiki/Ticket
//  - http://wiibrew.org/wiki/Tmd_file_structure
//  - http://hitmen.c02.at/files/yagcd/yagcd/chap13.html -> GameCube

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    setup			///////////////
///////////////////////////////////////////////////////////////////////////////

#include "tools.h"

int validate_file_format_sizes ( int trace_sizes );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  constants			///////////////
///////////////////////////////////////////////////////////////////////////////

enum // some constants
{
    HD_SECTOR_SIZE		= 0x200,

    WII_SECTOR_SIZE_SHIFT	= 15,
    WII_SECTOR_SIZE		= 1 << WII_SECTOR_SIZE_SHIFT,
    WII_SECTOR_SIZE4		= WII_SECTOR_SIZE >> 2,
    WII_SECTORS_PER_MIB		= 1024*1024/WII_SECTOR_SIZE,

    WII_SECTOR_IV_OFF		= 0x3d0,
    WII_SECTOR_HASH_SIZE	= 0x400,
    WII_SECTOR_HASH_SIZE4	= WII_SECTOR_HASH_SIZE >> 2,
    WII_SECTOR_DATA_SIZE	= WII_SECTOR_SIZE - WII_SECTOR_HASH_SIZE,
    WII_SECTOR_DATA_SIZE4	= WII_SECTOR_DATA_SIZE >> 2,

    WII_HASH_SIZE		= 20,
    WII_H0_DATA_SIZE		= 0x400,
    WII_H3_SIZE			= 0x18000,
    WII_N_ELEMENTS_H0		= WII_SECTOR_DATA_SIZE/WII_H0_DATA_SIZE,
    WII_N_ELEMENTS_H1		= 8,
    WII_N_ELEMENTS_H2		= 8,
    WII_N_ELEMENTS_H3		= WII_H3_SIZE/WII_HASH_SIZE,
    WII_MAX_PART_SECTORS	= WII_N_ELEMENTS_H1
				* WII_N_ELEMENTS_H2
				* WII_N_ELEMENTS_H3,

    WII_GROUP_SECTORS		= WII_N_ELEMENTS_H1 * WII_N_ELEMENTS_H2,
    WII_GROUP_SIZE		= WII_GROUP_SECTORS * WII_SECTOR_SIZE,
    WII_GROUP_SIZE4		= WII_GROUP_SIZE >> 2,
    WII_GROUP_HASH_SIZE		= WII_GROUP_SECTORS * WII_SECTOR_HASH_SIZE,
    WII_GROUP_HASH_SIZE4	= WII_GROUP_HASH_SIZE >> 2,
    WII_GROUP_DATA_SIZE		= WII_GROUP_SECTORS * WII_SECTOR_DATA_SIZE,
    WII_GROUP_DATA_SIZE4	= WII_GROUP_DATA_SIZE >> 2,

    WII_N_HASH_SECTOR		= WII_N_ELEMENTS_H0
				+ WII_N_ELEMENTS_H1
				+ WII_N_ELEMENTS_H2,
    WII_N_HASH_GROUP		= WII_N_HASH_SECTOR * WII_GROUP_SECTORS,

    WII_CERT_ALIGN		= 0x40,		// alignment in Wii certificates

    WII_TICKET_SIZE		= 0x2a4,
    WII_TICKET_SIG_OFF		= 0x140, // do SHA1 up to end of ticket
    WII_TICKET_KEY_OFF		= 0x1bf,
    WII_TICKET_IV_OFF		= 0x1dc,
    WII_TICKET_BRUTE_FORCE_OFF	= 0x24c, // this u32 will be iterated

    WII_TMD_GOOD_SIZE		= 0x208, // tmd with 1 content (usual)
    WII_TMD_SIG_OFF		= 0x140, // do SHA1 up to end of tmd
    WII_TMD_BRUTE_FORCE_OFF	= 0x19a, // this u32 will be iterated
    WII_PARTITION_BIN_SIZE	= 0x20000,

    WII_SECTORS_SINGLE_LAYER	= 143432,
    WII_SECTORS_DOUBLE_LAYER	= 2 * WII_SECTORS_SINGLE_LAYER,
    WII_MAX_SECTORS		= WII_SECTORS_DOUBLE_LAYER,
    WII_MAX_U32_SECTORS		= 0x40000000/WII_SECTOR_SIZE4,

    WII_MAGIC			= 0x5d1c9ea3,
    WII_MAGIC_DELETED		= 0x2a44454c,
    WII_MAGIC_OFF		=       0x18,
    WII_MAGIC_LEN		=       0x04,

    WII_MAGIC2			= 0xc3f81a8e,
    WII_MAGIC2_OFF		=    0x4fffc,
    WII_MAGIC2_LEN		=       0x04,

    WII_TITLE_OFF		= 0x20,
    WII_TITLE_SIZE		= 0x40,

    WII_KEY_SIZE		=   16,
    WII_FILE_PATH_SIZE		= 1000,

    WII_MAX_PTAB		=       4,	// maximal partition tables
    WII_PTAB_REF_OFF		= 0x40000,	// disc offset of partition table ref
    WII_PTAB_SECTOR		= WII_PTAB_REF_OFF / WII_SECTOR_SIZE,
						// sector index of partition table
    WII_MAX_PTAB_SIZE		=   0x400,	// maximal size of all ptabs + entries
    WII_MAX_PARTITIONS		= WII_MAX_PTAB_SIZE / 8 - WII_MAX_PTAB,
						// maximal supported partitions

    WII_REGION_OFF		= 0x4e000,
    WII_REGION_SIZE		=    0x20,

    WII_BOOT_OFF		=      0,
    WII_BI2_OFF			=  0x440,
    WII_APL_OFF			= 0x2440,
    WII_BOOT_SIZE		= WII_BI2_OFF - WII_BOOT_OFF,
    WII_BI2_SIZE		= WII_APL_OFF - WII_BI2_OFF,
    WII_BI2_REGION_OFF		=   0x18,

    WII_PART_OFF		=   0x50000,
    WII_GOOD_DATA_PART_OFF	= 0xf800000,

    GC_MAGIC			= 0xc2339f3d,
    GC_MAGIC_OFF		=       0x1c,
    GC_MAGIC_LEN		=       0x04,
    GC_DISC_SIZE		= 1459978240,	// standard GameCube disc size

    GC_MULTIBOOT_PTAB_OFF	=  0x40,
    GC_MULTIBOOT_PTAB_SIZE	= 0x100 - GC_MULTIBOOT_PTAB_OFF,
    GC_MULTIBOOT_MAX_PART	= GC_MULTIBOOT_PTAB_SIZE/4,
    GC_GOOD_PART_ALIGN		= 0x20000,	// alignment (= min off) of GC partitions

    DOL_N_TEXT_SECTIONS		=     7,
    DOL_N_DATA_SECTIONS		=    11,
    DOL_N_SECTIONS		= DOL_N_TEXT_SECTIONS + DOL_N_DATA_SECTIONS,
    DOL_HEADER_SIZE		= 0x100,

    WBFS_MIN_SECTOR_SHIFT	=  6,	// needed for wbfs_calc_size_shift()
    WBFS_MAX_SECTOR_SHIFT	= 11,	// needed for wbfs_calc_size_shift()
    WBFS_MIN_SECTOR_SIZE	= 1 << WBFS_MIN_SECTOR_SHIFT + WII_SECTOR_SIZE_SHIFT,
    WBFS_MAX_SECTOR_SIZE	= 1 << WBFS_MAX_SECTOR_SHIFT + WII_SECTOR_SIZE_SHIFT,

    WBFS_INODE_INFO_VERSION	=    1,
    WBFS_INODE_INFO_HEAD_SIZE	=   12,
    WBFS_INODE_INFO_CMP_SIZE	=   10,
    WBFS_INODE_INFO_OFF		= 0x80,
    WBFS_INODE_INFO_SIZE	= 0x100 - WBFS_INODE_INFO_OFF,
};

#define WII_MAX_DISC_SIZE ((u64)WII_MAX_SECTORS*(u64)WII_SECTOR_SIZE)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_disc_type_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_disc_type_t
{
    //**********************************************************************
    //***  never change this values, because they are used in archives!  ***
    //**********************************************************************
 
    WD_DT_UNKNOWN	= 0,	// unknown disc type
    WD_DT_GAMECUBE,		// GameCube disc
    WD_DT_WII,			// Wii disc

    WD_DT__N			// number of defined disc types

} wd_disc_type_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_disc_attrib_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_disc_attrib_t
{
    //--- disc type attributes, detected by get_header_disc_type()

    WD_DA_GAMECUBE	= 1 << WD_DT_GAMECUBE,
    WD_DA_WII		= 1 << WD_DT_WII,
    

    //--- real attributes, detected by get_header_disc_type()

    WD_DA_GC_MULTIBOOT	= 0x0100,	// gamecube multiboot disc
    WD_DA_GC_DVD9	= 0x0200,	// gc-mb disc with DVD9 part-tab


    //--- more real attributes

    WD_DA_GC_START_PART	= 0x0400,	// gc-mb disc with valid start partition

} wd_disc_attrib_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_compression_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_compression_t
{
    //**********************************************************************
    //***  never change this values, because they are used in archives!  ***
    //**********************************************************************

    WD_COMPR_NONE	= 0,	// data is not compressed
    WD_COMPR_PURGE,		// a WDF like compression: manage holes

    WD_COMPR_BZIP2,		// use BZIP2 compression
    WD_COMPR_LZMA,		// use LZMA compression
    WD_COMPR_LZMA2,		// use LZMA2 compression

    WD_COMPR__N,		// number of compressions

    WD_COMPR__FIRST_REAL	= WD_COMPR_BZIP2,	// first real compression
 #ifdef NO_BZIP2
    WD_COMPR__FAST		= WD_COMPR_LZMA,	// a fast compression
 #else
    WD_COMPR__FAST		= WD_COMPR_BZIP2,	// a fast compression
 #endif
    WD_COMPR__GOOD		= WD_COMPR_LZMA,	// a good compression
    WD_COMPR__BEST		= WD_COMPR_LZMA,	// the best compression
    WD_COMPR__DEFAULT		= WD_COMPR_LZMA,	// the default compression

} wd_compression_t;

//-----------------------------------------------------------------------------

ccp wd_get_compression_name
(
    wd_compression_t	compr,		// compression method
    ccp			invalid_result	// return value if 'compr' is invalid
);

ccp wd_print_compression
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    wd_compression_t	compr_method,	// compression method
    int			compr_level,	// compression level
    u32			chunk_size,	// compression chunk size, multiple of MiB
    int			mode		// 1=number, 2=name, 3=number and name
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct dol_header_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct dol_header_t
{
    /* 0x00 */	u32 sect_off [DOL_N_SECTIONS];
    /* 0x48 */	u32 sect_addr[DOL_N_SECTIONS];
    /* 0x90 */	u32 sect_size[DOL_N_SECTIONS];
    /* 0xd8 */	u32 bss_addr;
    /* 0xdc */	u32 bss_size;
    /* 0xe0 */	u32 entry_addr;
    /* 0xd4 */	u8  padding[DOL_HEADER_SIZE-0xe4];

}
__attribute__ ((packed)) dol_header_t;

void ntoh_dol_header ( dol_header_t * dest, const dol_header_t * src );
void hton_dol_header ( dol_header_t * dest, const dol_header_t * src );

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    struct wbfs_inode_info_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wbfs_inode_info_t
{
	// A complete copy of the first WBFS_INODE_INFO_HEAD_SIZE (12) bytes
	// of the WBFS header. The first WBFS_INODE_INFO_CMP_SIZE (10)
	// bytes can be used to validate the existence of this info block.
	// They are also good for recovery.

	be32_t magic;		// the magic (char*)"WBFS"
	be32_t n_hd_sec;	// total number of hd_sec in this partition
	u8  hd_sec_sz_s;	// sector size in this partition
	u8  wbfs_sec_sz_s;	// size of a wbfs sec

	u8  wbfs_version;	// informative version number
	u8  head_padding;

	// The version number of this data structure.
	// I may be importand for future extensions

	be32_t info_version;	// == WBFS_INODE_INFO_VERSION

	// 64 bit time stamps: They are only informative but nice to have.
	//  - itime is ths disc inserting time.
	//    If 2 discs uses the same wbfs block a repair function knows
	//    which one disc are newer and which is definitly bad.
	//  - mtime is a copy of the mtime of the source file.
	//    It is also changed if the the ISO-header is modified (renamed).
	//    While extrating the mtime of dest file is set by this mtime.
	//  - ctime is updated if adding, renaming.
	//  - atime can be updated by usb loaders when loading the disc.
	//  - dtime is only set for deleted games.

	be64_t itime;		// the disc insertion time
	be64_t mtime;		// the last modification time (copied from source)
	be64_t ctime;		// the last status changed time
	be64_t atime;		// the last access time
	be64_t dtime;		// the deletion time

	// there is enough space for more information like a game load counter
	// or other statistics and game settings. This infos can be share across
	// usb loaders.

	// EXAMPLES:
	//	be32_t	load_count;
	//	u8	favorite
	//	u8	ios

	// padding up to WBFS_INODE_INFO_SIZE bytes, always filled with zeros

	u8 padding[ WBFS_INODE_INFO_SIZE - WBFS_INODE_INFO_HEAD_SIZE
			- 0 /* num of  8 bit parameters */ * sizeof(u8)
			- 0 /* num of 16 bit parameters */ * sizeof(be16_t)
			- 1 /* num of 32 bit parameters */ * sizeof(be32_t)
			- 5 /* num of 64 bit parameters */ * sizeof(be64_t) ];

}
__attribute__ ((packed)) wbfs_inode_info_t;

void ntoh_inode_info ( wbfs_inode_info_t * dest, const wbfs_inode_info_t * src );
void hton_inode_info ( wbfs_inode_info_t * dest, const wbfs_inode_info_t * src );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_header_128_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_header_128_t
{
	// -> http://www.wiibrew.org/wiki/Wiidisc#Header

  /* 0x00 */	char	disc_id;
  /* 0x01 */	char	game_code[2];
  /* 0x03 */	char	region_code;
  /* 0x04 */	char	marker_code[2];

  /* 0x06 */	u8	disc_number;
  /* 0x07 */	u8	disc_version;

  /* 0x08 */	u8	audio_streaming;
  /* 0x09 */	u8	streaming_buffer_size;

  /* 0x0a */	u8	unknown1[0x0e];
  /* 0x18 */	u32	wii_magic;		// off=WII_MAGIC_OFF, val=WII_MAGIC
  /* 0x1c */	u32	gc_magic;		// off=GC_MAGIC_OFF, val=GC_MAGIC

  /* 0x20 */	char	disc_title[WII_TITLE_SIZE];	// off=WII_TITLE_OFF

  /* 0x60 */	u8	diable_hash;
  /* 0x61 */	u8	diable_encryption;
  
  /* 0x62 */	u8	padding[0x1e];

} __attribute__ ((packed)) wd_header_128_t;

//-----------------------------------------------------------------------------

void header_128_setup
(
    wd_header_128_t	* dhead,	// valid pointer
    const void		* id6,		// NULL or pointer to ID
    ccp			disc_title,	// NULL or pointer to disc title (truncated)
    bool		is_gc		// true: GameCube setup
);

//-----------------------------------------------------------------------------

wd_disc_type_t get_header_128_disc_type
(
    wd_header_128_t	* dhead,	// valid pointer
    wd_disc_attrib_t	* attrib	// not NULL: store disc attributes
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_header_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_header_t
{
	// -> http://www.wiibrew.org/wiki/Wiidisc#Header

  /* 0x00 */	char	disc_id;
  /* 0x01 */	char	game_code[2];
  /* 0x03 */	char	region_code;
  /* 0x04 */	char	marker_code[2];

  /* 0x06 */	u8	disc_number;
  /* 0x07 */	u8	disc_version;

  /* 0x08 */	u8	audio_streaming;
  /* 0x09 */	u8	streaming_buffer_size;

  /* 0x0a */	u8	unknown1[0x0e];
  /* 0x18 */	u32	wii_magic;		// off=WII_MAGIC_OFF, val=WII_MAGIC
  /* 0x1c */	u32	gc_magic;		// off=GC_MAGIC_OFF, val=GC_MAGIC

  /* 0x20 */	char	disc_title[WII_TITLE_SIZE];	// off=WII_TITLE_OFF

  /* 0x60 */	u8	diable_hash;
  /* 0x61 */	u8	diable_encryption;
  
  /* 0x62 */	u8	padding[0x1e];

  /* 0x80 */	wbfs_inode_info_t iinfo;		// off=WBFS_INODE_INFO_OFF

} __attribute__ ((packed)) wd_header_t;

//-----------------------------------------------------------------------------

void header_setup
(
    wd_header_t		* dhead,	// valid pointer
    const void		* id6,		// NULL or pointer to ID
    ccp			disc_title,	// NULL or pointer to disc title (truncated)
    bool		is_gc		// true: GameCube setup
);

//-----------------------------------------------------------------------------

wd_disc_type_t get_header_disc_type
(
    wd_header_t		* dhead,	// valid pointer
    wd_disc_attrib_t	* attrib	// not NULL: store disc attributes
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_boot_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_boot_t
{
  /*     0 */	wd_header_t	dhead;
  /* 0x100 */	u8		unknown1[0x420-sizeof(wd_header_t)];
  /* 0x420 */	u32		dol_off4;
  /* 0x424 */	u32		fst_off4;
  /* 0x428 */	u32		fst_size4;
  /* 0x42c */	u32		max_fst_size4;  // >= fst_size4 (max of multi discs)
  /* 0x430 */	u8		unknown2[WII_BOOT_SIZE-0x430];
}
__attribute__ ((packed)) wd_boot_t;

void ntoh_boot ( wd_boot_t * dest, const wd_boot_t * src );
void hton_boot ( wd_boot_t * dest, const wd_boot_t * src );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_region_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_region_t
{
  /*    0 */	u32	region;
  /* 0x04 */	u8	padding1[12];
  /* 0x10 */	u8	region_info[8];
  /* 0x18 */	u8	padding2[8];
}
__attribute__ ((packed)) wd_region_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    struct wd_ptab_info_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_ptab_info_t
{
	u32 n_part;	// number of partitions in this table
	u32 off4;	// offset/4 of partition table relative to disc start
}
__attribute__ ((packed)) wd_ptab_info_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    struct wd_ptab_entry_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_ptab_entry_t
{
	u32 off4;	// offset/4 of partition relative to disc start
	u32 ptype;	// partitions type
}
__attribute__ ((packed)) wd_ptab_entry_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_ptab_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_ptab_t // example for a good partition table
{
  /*    0 */	wd_ptab_info_t  info[WII_MAX_PTAB];
  /* 0x20 */	wd_ptab_entry_t	entry[WII_MAX_PARTITIONS];
}
__attribute__ ((packed)) wd_ptab_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_ticket_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_ticket_t
{
  // --> http://wiibrew.org/wiki/Ticket

  /* 0x000 */	u32 sig_type;		// signature type (always 0x10001 for RSA-2048)
  /* 0x004 */	u8  sig[0x100];		// signature by a certificate's key
  /* 0x104 */	u8  sig_padding[0x3c];	// always 0

  // the signature calculations starts here (WII_TICKET_SIG_OFF)

  /* 0x140 */	u8  issuer[0x40]; 	// signature issuer
  /* 0x180 */	u8  unknown1[0x3f]; 	// always 0, unless it is a VC game
  /* 0x1bf */	u8  title_key[0x10];	// encrypted title key, offset=WII_TICKET_KEY_OFF
  /* 0x1cf */	u8  unknown2; 		// ?
  /* 0x1d0 */	u8  ticket_id[8];	// ticket ID
  /* 0x1d8 */	u8  console_id[4];	// console ID
  /* 0x1dc */	u8  title_id[8];	// title ID, offset=WII_TICKET_IV_OFF
  /* 0x1e4 */	u16 unknown3;		// unknown, mostly 0xFFFF
  /* 0x1e6 */	u16 n_dlc; 		// amount of bought DLC contents
  /* 0x1e8 */	u8  unknown4;		// ERROR in http://wiibrew.org/wiki/Ticket
  /* 0x1e9 */	u8  unknown5[0x08];	// ?
  /* 0x1f1 */	u8  common_key_index;	// 1=Korean Common key, 0="normal" Common key
  /* 0x1f2 */	u8  unknown6[0x30]; 	// Is all 0 for non-VC, for VC, all 0 except last byte is 1
  /* 0x222 */	u8  unknown7[0x20]; 	// always 0xff (?)
  /* 0x242 */	u8  padding2[2]; 	// always 0
  /* 0x244 */	u32 enable_time_limit;	// 1=enabled, 0=disabled
  /* 0x248 */	u32 time_limit;		// seconds (what is the epoch?)
  /* 0x24c */	u8  fake_sign[0x58];	// padding, always 0 => used for fake signing

}
__attribute__ ((packed)) wd_ticket_t;

//----- encryption helpers

extern const char not_encrypted_marker[];

void ticket_setup ( wd_ticket_t * ticket, const void * id4 );

void ticket_clear_encryption ( wd_ticket_t * ticket, int mark_not_encrypted );
bool ticket_is_marked_not_encrypted ( const wd_ticket_t * ticket );
u32  ticket_fake_sign ( wd_ticket_t * ticket, u32 ticket_size );
bool ticket_is_fake_signed ( const wd_ticket_t * ticket, u32 ticket_size );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_part_header_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_part_header_t
{
  /* 0x000 */	wd_ticket_t ticket;
  /* 0x2a4 */	u32 tmd_size;
  /* 0x2a8 */	u32 tmd_off4;
  /* 0x2ac */	u32 cert_size;
  /* 0x2b0 */	u32 cert_off4;
  /* 0x2b4 */	u32 h3_off4;
  /* 0x2b8 */	u32 data_off4;
  /* 0x2bc */	u32 data_size4;
}
__attribute__ ((packed)) wd_part_header_t;

void ntoh_part_header ( wd_part_header_t * dest, const wd_part_header_t * src );
void hton_part_header ( wd_part_header_t * dest, const wd_part_header_t * src );

//
///////////////////////////////////////////////////////////////////////////////
///////////////		      struct wd_tmd_content_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_tmd_content_t
{
  /* 0x00 */	u32 content_id;
  /* 0x04 */	u16 index;
  /* 0x06 */	u16 type;
  /* 0x08 */	u64 size;
  /* 0x10 */	u8  hash[WII_HASH_SIZE]; // SHA1 hash
}
__attribute__ ((packed)) wd_tmd_content_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    struct wd_tmd_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_tmd_t
{
  // --> http://wiibrew.org/wiki/Tmd_file_structure

  /* 0x000 */	u32 sig_type;
  /* 0x004 */	u8  sig[0x100];
  /* 0x104 */	u8  sig_padding[0x3c];

  // the signature calculations starts here (WII_TMD_SIG_OFF)

  /* 0x140 */	u8  issuer[0x40];
  /* 0x180 */	u8  version;
  /* 0x181 */	u8  ca_crl_version;
  /* 0x182 */	u8  signer_crl_version;
  /* 0x183 */	u8  padding2;
  /* 0x184 */	u64 sys_version;	// system version (the ios that the title need)
  /* 0x18c */	u8  title_id[8];
  /* 0x194 */	u32 title_type;
  /* 0x198 */	u16 group_id;
  /* 0x19a */	u8  fake_sign[0x3e]; 	// padding => place of fake signing
  /* 0x1d8 */	u32 access_rights;
  /* 0x1dc */	u16 title_version;
  /* 0x1de */	u16 n_content;
  /* 0x1e0 */	u16 boot_index;
  /* 0x1e2 */	u8  padding3[2];
  /* 0x1e4 */	wd_tmd_content_t content[0]; // n_contents elements
}
__attribute__ ((packed)) wd_tmd_t;

//----- encryption helpers

void tmd_setup ( wd_tmd_t * tmd, u32 tmd_size, const void * id4 );

void tmd_clear_encryption ( wd_tmd_t * tmd, int mark_not_encrypted );
bool tmd_is_marked_not_encrypted ( const wd_tmd_t * tmd );
u32  tmd_fake_sign ( wd_tmd_t * tmd, u32 tmd_size );
bool tmd_is_fake_signed ( const wd_tmd_t * tmd, u32 tmd_size );

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    struct wd_part_control_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_part_control_t
{
    u8 part_bin[WII_PARTITION_BIN_SIZE]; // this is the real data

    // this are pointers into part_bin

    wd_part_header_t	* head;		// pointer to header
    wd_tmd_t		* tmd;		// pointer to tmd
    wd_tmd_content_t	* tmd_content;	// NULL or pointer to first tmd content
    u8			* cert;		// pointer to cert
    u8			* h3;		// pointer to h3

    // the following values are informative; format is host endian

    int is_valid;	// is structure valid

    u32 head_size;	// always sizeof(wd_part_header_t)
    u32 tmd_size;	// set by user
    u32 cert_size;	// set by user
    u32 h3_size;	// always WII_H3_SIZE

    u64 data_off;	// = sizeof(part_bin) if cleared
    u64 data_size;	// set by user
}
wd_part_control_t; // packing not needed

//----- setup

// 0:ok, 1:error, sizes to large
int clear_part_control
	( wd_part_control_t * pc, u32 tmd_size, u32 cert_size, u64 data_size );

// 0:ok, 1:error => pc->part_bin must be valid content
int setup_part_control ( wd_part_control_t * pc );

//----- encryption helpers

u32 part_control_fake_sign ( wd_part_control_t * pc, int calc_h4 );
int part_control_is_fake_signed ( const wd_part_control_t * pc );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_part_sector_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_part_sector_t
{
  /* 0x000 */	u8 h0 [WII_N_ELEMENTS_H0][WII_HASH_SIZE];
  /* 0x26c */	u8 padding0[0x14];
  /* 0x280 */	u8 h1 [WII_N_ELEMENTS_H1][WII_HASH_SIZE];
  /* 0x320 */	u8 padding1[0x20];
  /* 0x340 */	u8 h2 [WII_N_ELEMENTS_H2][WII_HASH_SIZE];
  /* 0x3e0 */	u8 padding2[0x20];

  /* 0x400 */	u8 data[WII_N_ELEMENTS_H0][WII_H0_DATA_SIZE];
}
__attribute__ ((packed)) wd_part_sector_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_fst_item_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wd_fst_item_t
{
    union
    {
	u8  is_dir;
	u32 name_off;	// mask with 0x00ffffff
    };

    u32 offset4;
    u32 size;
}
__attribute__ ((packed)) wd_fst_item_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wbfs_head_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wbfs_head_t
{
 /*    0 */	be32_t	magic;		// the magic (char*)"WBFS"

    // the 3 main parameters -> they are used to calculate the geometry

 /* 0x04 */	be32_t	n_hd_sec;	// total number of hd_sec in this partition
 /* 0x08 */	u8	hd_sec_sz_s;	// sector size in this partition
 /* 0x09 */	u8	wbfs_sec_sz_s;	// size of a wbfs sec

    // more parameters

 /* 0x0a */	u8	wbfs_version;	// informative version number
 /* 0x0b */	u8	padding;
 /* 0x0c */	u8	disc_table[0];	// size depends on hd sector size
}
__attribute__ ((packed)) wbfs_head_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wbfs_disc_info_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wbfs_disc_info_t
{
 /*    0 */	u8	dhead[0x100];
 /* 0x100 */	be16_t	wlba_table[0];	// wbfs_t::n_wbfs_sec_per_disc elements

}
__attribute__ ((packed)) wbfs_disc_info_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // FILE_FORMATS_H
