
/***************************************************************************
 *                    __            __ _ ___________                       *
 *                    \ \          / /| |____   ____|                      *
 *                     \ \        / / | |    | |                           *
 *                      \ \  /\  / /  | |    | |                           *
 *                       \ \/  \/ /   | |    | |                           *
 *                        \  /\  /    | |    | |                           *
 *                         \/  \/     |_|    |_|                           *
 *                                                                         *
 *                           Wiimms ISO Tools                              *
 *                         http://wit.wiimm.de/                            *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project.                                 *
 *   Visit http://wit.wiimm.de/ for project details and sources.           *
 *                                                                         *
 *   Copyright (c) 2009-2014 by Dirk Clemens <wiimm@wiimm.de>              *
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

#ifndef WIT_LIB_WDF_H
#define WIT_LIB_WDF_H 1

#include "types.h"
#include "lib-std.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			WDF definitions			///////////////
///////////////////////////////////////////////////////////////////////////////

// First a magic is defined to identify a WDF clearly. The magic should never be
// a possible Wii ISO image identification. Wii ISO images starts with the ID6.
// And so the WDF magic contains one control character (CTRL-A).

#define WDF_MAGIC		"WII\1DISC"
#define WDF_MAGIC_SIZE		8

//--- these tools are compatible to >=version
#define WDF_COMPATIBLE		1

//--- WDF head sizes
#define WDF_VERSION1_SIZE	sizeof(WDF_Header_t)
#define WDF_VERSION2_SIZE	sizeof(WDF_Header_t)
#define WDF_VERSION3_SIZE	sizeof(WDF_Header_t)

//--- the minimal size of holes in bytes that will be detected.
#define WDF_MIN_HOLE_SIZE	(sizeof(WDF1_Chunk_t)+sizeof(WDF_Hole_t))

//--- WDF hole detection type
typedef u32 WDF_Hole_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct WDF_Header_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// This is the header of a WDF v1.
// Remember: Within a file the data is stored in network byte order (big endian)

typedef struct WDF_Header_t
{
	char magic[WDF_MAGIC_SIZE];	// WDF_MAGIC, what else!

	u32 wdf_version;		// WDF_DEF_VERSION

    #if 0 // first head definition, WDF v1 before 2012-09

	// split file support (never used, values were *,0,1)
	u32 split_file_id;		// for plausibility checks
	u32 split_file_index;		// zero based index of this file, always 0
	u32 split_file_total;		// number of split files, always 1

    #else

	u32 head_size;			// size of version related WDF_Header_t

	u32 align_factor;		// info: Alignment forced on creation
					//	 Number is always power of 2
					//       If NULL: no alignment forced.  

	u32 wdf_compatible;		// this file is compatible down to version #

    #endif

	u64 file_size;			// the size of the virtual file

	u64 data_size;			// the ISO data size in this file
					// (without header and chunk table)

    #if 0 // first head definition, WDF v1 before 2012-09
	u32 chunk_split_file;		// which split file contains the chunk table
    #else
	u32 reserved;			// always 0
    #endif

	u32 chunk_n;			// total number of data chunks

	u64 chunk_off;			// the 'MAGIC + chunk_table' file offset
}
__attribute__ ((packed)) WDF_Header_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct WDF*_Chunk_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// This is the chunk info of WDF.
// Remember: Within a file the data is stored in network byte order (big endian)

typedef struct WDF1_Chunk_t
{
	// split_file_index is obsolete in WDF v2
	u32 ignored_split_file_index;	// which split file contains that chunk
					// (WDF v1: always 0, ignored)
	u64 file_pos;			// the virtual ISO file position
	u64 data_off;			// the data file offset
	u64 data_size;			// the data size

} __attribute__ ((packed)) WDF1_Chunk_t;

//-----------------------------------------------------------------------------

typedef struct WDF2_Chunk_t
{
	u64 file_pos;			// the virtual ISO file position
	u64 data_off;			// the data file offset
	u64 data_size;			// the data size

} __attribute__ ((packed)) WDF2_Chunk_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface for WDF_Header_t		///////////////
///////////////////////////////////////////////////////////////////////////////

// initialize WH
void InitializeWH ( WDF_Header_t * wh );

// convert WH data, src + dest may point to same structure
void ConvertToNetworkWH ( WDF_Header_t * dest, const WDF_Header_t * src );
void ConvertToHostWH    ( WDF_Header_t * dest, const WDF_Header_t * src );

// helpers
bool FixHeaderWDF
(
    WDF_Header_t	*wh,	    // dest header
    const WDF_Header_t	*wh_src,    // src header, maybe NULL
    bool		setup	    // true: setup all paramaters
);

static inline uint GetHeaderSizeWDF ( uint vers )
	{ return sizeof(WDF_Header_t); }

///////////////////////////////////////////////////////////////////////////////
///////////////		    interface for WDF*_Chunk_t		///////////////
///////////////////////////////////////////////////////////////////////////////

// convert WC.v1 data, src + dest may point to same structure
void ConvertToNetworkWC1 ( WDF1_Chunk_t * dest, const WDF1_Chunk_t * src );
void ConvertToHostWC1    ( WDF1_Chunk_t * dest, const WDF1_Chunk_t * src );

static inline uint GetChunkSizeWDF ( uint vers )
	{ return vers < 2 ? sizeof(WDF1_Chunk_t) : sizeof(WDF2_Chunk_t); }

//-----------------------------------------------------------------------------
#if WDF2_ENABLED

// convert WC.v2 data, src + dest may point to same structure
void ConvertToNetworkWC2 ( WDF2_Chunk_t * dest, const WDF2_Chunk_t * src );
void ConvertToHostWC2    ( WDF2_Chunk_t * dest, const WDF2_Chunk_t * src );

#endif // WDF2_ENABLED
//-----------------------------------------------------------------------------

void ConvertToNetworkWC1n
(
    WDF1_Chunk_t	*dest,		// dest data, must not overlay 'src'
    const WDF2_Chunk_t	*src,		// source data, network byte order
    uint		n_elem		// number of elements to convert
);

void ConvertToNetworkWC2n
(
    WDF2_Chunk_t	*dest,		// dest data, may overlay 'src'
    const WDF2_Chunk_t	*src,		// source data, network byte order
    uint		n_elem		// number of elements to convert
);

void ConvertToHostWC
(
    WDF2_Chunk_t	*dest,		// dest data, may overlay 'src'
    const void		*src_data,	// source data, network byte order
    uint		wdf_version,	// related wdf version
    uint		n_elem		// number of elements to convert
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SuperFile_t interface		///////////////
///////////////////////////////////////////////////////////////////////////////

#undef SUPERFILE
#define SUPERFILE struct SuperFile_t
SUPERFILE;

// WDF reading support
enumError SetupReadWDF	( SUPERFILE * sf );
enumError ReadWDF	( SUPERFILE * sf, off_t off, void * buf, size_t size );
off_t     DataBlockWDF	( SUPERFILE * sf, off_t off, size_t hint_align, off_t * block_size );

// WDF writing support
enumError SetupWriteWDF	( SUPERFILE * sf );
enumError TermWriteWDF	( SUPERFILE * sf );
enumError WriteWDF	( SUPERFILE * sf, off_t off, const void * buf, size_t size );
enumError WriteSparseWDF( SUPERFILE * sf, off_t off, const void * buf, size_t size );
enumError WriteZeroWDF	( SUPERFILE * sf, off_t off, size_t size );

// chunk management
WDF2_Chunk_t * NeedChunkWDF ( SUPERFILE * sf, int at_index );

#undef SUPERFILE

//
///////////////////////////////////////////////////////////////////////////////
///////////////				options			///////////////
///////////////////////////////////////////////////////////////////////////////

extern uint opt_wdf_version;
extern uint opt_wdf_align;
extern uint use_wdf_version;
extern uint use_wdf_align;

void SetupOptionsWDF();
int SetWDF2Mode ( uint c, ccp align );
int ScanOptWDFAlign ( ccp arg );

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_WDF_H

