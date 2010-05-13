
#ifndef WIT_LIB_CISO_H
#define WIT_LIB_CISO_H 1

#include "types.h"
#include "libwbfs.h"
#include "lib-std.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    CISO support                 ///////////////
///////////////////////////////////////////////////////////////////////////////

enum // some constants
{
    CISO_HEAD_SIZE		= 0x8000,		// total header size
    CISO_MAP_SIZE		= CISO_HEAD_SIZE - 8,	// map size
    CISO_MIN_BLOCK_SIZE		= WII_SECTOR_SIZE,	// minimal allowed block size

    CISO_WR_MIN_BLOCK_SIZE	= 1*MiB,		// minimal block size if writing
    CISO_WR_MAX_BLOCK		= 0x2000,		// maximal blocks if writing
    CISO_WR_MIN_HOLE_SIZE	= 0x1000,		// min hole size for sparse cheking
};

typedef u16 CISO_Map_t;
#define CISO_UNUSED_BLOCK ((CISO_Map_t)~0)

//-----------------------------------------------------------------------------

typedef struct CISO_Head_t
{
	u8  magic[4];		// "CISO"
	u32 block_size;		// stored as litte endian (not network byte order)
	u8  map[CISO_MAP_SIZE];	// 0=unused, 1=used

} __attribute__ ((packed)) CISO_Head_t;

//-----------------------------------------------------------------------------

typedef struct CISO_Info_t
{
	u32 block_size;		// the block size
	u32 used_blocks;	// number of used blocks
	u32 needed_blocks;	// number of needed blocks
	u32 map_size;		// number of alloced elements for 'map'
	CISO_Map_t * map;	// NULL or map with 'map_size' elements
	off_t max_file_off;	// maximal file offset
	off_t max_virt_off;	// maximal virtiual iso offset

} CISO_Info_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////              interface for CISO files           ///////////////
///////////////////////////////////////////////////////////////////////////////

// Initialize CISO_Info_t, copy and validate data from CISO_Head_t if not NULL
enumError InitializeCISO ( CISO_Info_t * ci, CISO_Head_t * ch );

// Setup CISO_Info_t, copy and validate data from CISO_Head_t if not NULL
enumError SetupCISO ( CISO_Info_t * ci, CISO_Head_t * ch );

// free all dynamic data and Clear data
void ResetCISO ( CISO_Info_t * ci );

//-----------------------------------------------------------------------------

#undef SUPERFILE
#define SUPERFILE struct SuperFile_t
SUPERFILE;

// CISO reading support
enumError SetupReadCISO	( SUPERFILE * sf );
enumError ReadCISO	( SUPERFILE * sf, off_t off, void * buf, size_t size );

// CISO writing support
enumError SetupWriteCISO ( SUPERFILE * sf );
enumError TermWriteCISO	 ( SUPERFILE * sf );
enumError WriteCISO	 ( SUPERFILE * sf, off_t off, const void * buf, size_t size );
enumError WriteSparseCISO( SUPERFILE * sf, off_t off, const void * buf, size_t size );

#undef SUPERFILE

//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_CISO_H

