
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

#ifndef WIT_LIB_WDF_H
#define WIT_LIB_WDF_H 1

#include "types.h"
#include "lib-std.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  WDF definitions                ///////////////
///////////////////////////////////////////////////////////////////////////////

// First a magic is defined to identify a WDF clearly. The magic should never be
// a possible Wii ISO image identification. Wii ISO images starts with the ID6.
// And so the WDF magic contains one contral character (CTRL-A) within.

#define WDF_MAGIC		"WII\1DISC"
#define WDF_MAGIC_SIZE		8

#ifdef NEW_FEATURES
 #define WDF2_ENABLED		0	// [obsolete] [2do]
 #define WDF_VERSION		1
 #define WDF_COMPATIBLE		1
#else
 #define WDF2_ENABLED		0
 #define WDF_VERSION		1
 #define WDF_COMPATIBLE		1
#endif

// WDF head sizes

#define WDF_VERSION1_SIZE	56
#define WDF_VERSION2_SIZE	sizeof(WDF_Head_t)

// the minimal size of holes in bytes that will be detected.

#define WDF_MIN_HOLE_SIZE	(sizeof(WDF_Chunk_t)+sizeof(WDF_Hole_t))

// WDF hole detection type
typedef u32 WDF_Hole_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 struct WDF_Head_t               ///////////////
///////////////////////////////////////////////////////////////////////////////
// This is the header of a WDF.
// Remember: Within a file the data is stored in network byte order (big endian)

typedef struct WDF_Head_t
{
	// magic and version number
	char magic[WDF_MAGIC_SIZE];	// WDF_MAGIC, what else!
	u32 wdf_version;		// WDF_VERSION

	// split file support
	u32 split_file_id;		// for plausibility checks
	u32 split_file_index;		// zero based index ot this file
	u32 split_file_num_of;		// number of split files

	// virtual file infos
	u64 file_size;			// the size of the virtual file

	// data size of this file
	u64 data_size;			// the ISO data size in this file
					// (without header and chunk table)
	// chunks
	u32 chunk_split_file;		// which spit file contains the chunk table
	u32 chunk_n;			// total number of data chunks
	u64 chunk_off;			// the 'MAGIC + chunk_table' file offset

	//---------- here ends the header of WDF version 1 ----------

 #if WDF2_ENABLED
	u32 wdf_compatible;		// this file is compatible down to version #
	u32 align_factor;		// all data is aligned with a multiple of #
	u8  bca[64];			// BCA data
 #endif

} __attribute__ ((packed)) WDF_Head_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                struct WDF_Chunk_t               ///////////////
///////////////////////////////////////////////////////////////////////////////
// This is the chunk info of WDF.
// Remember: Within a file the data is stored in network byte order (big endian)

typedef struct WDF_Chunk_t
{
	u32 split_file_index;		// which split file conatins that chunk
	u64 file_pos;			// the virtual ISO file position
	u64 data_off;			// the data file offset
	u64 data_size;			// the data size

} __attribute__ ((packed)) WDF_Chunk_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////               interface for WDF_Head_t          ///////////////
///////////////////////////////////////////////////////////////////////////////

// initialize WH
void InitializeWH ( WDF_Head_t * wh );

// convert WH data, src + dest may point to same structure
void ConvertToNetworkWH ( WDF_Head_t * dest, WDF_Head_t * src );
void ConvertToHostWH    ( WDF_Head_t * dest, WDF_Head_t * src );

// helpers
size_t GetHeadSizeWDF ( u32 version );

///////////////////////////////////////////////////////////////////////////////
///////////////               interface for WDF_Chunk_t         ///////////////
///////////////////////////////////////////////////////////////////////////////

// initialize WC
void InitializeWC ( WDF_Chunk_t * wc, int n_elem );

// convert WC data, src + dest may point to same structure
void ConvertToNetworkWC ( WDF_Chunk_t * dest, WDF_Chunk_t * src );
void ConvertToHostWC    ( WDF_Chunk_t * dest, WDF_Chunk_t * src );

//
///////////////////////////////////////////////////////////////////////////////
///////////////               SuperFile_t interface             ///////////////
///////////////////////////////////////////////////////////////////////////////

#undef SUPERFILE
#define SUPERFILE struct SuperFile_t
SUPERFILE;

// WDF reading support
enumError SetupReadWDF	( SUPERFILE * sf );
enumError ReadWDF	( SUPERFILE * sf, off_t off, void * buf, size_t size );

// WDF writing support
enumError SetupWriteWDF	( SUPERFILE * sf );
enumError TermWriteWDF	( SUPERFILE * sf );
enumError WriteWDF	( SUPERFILE * sf, off_t off, const void * buf, size_t size );
enumError WriteSparseWDF( SUPERFILE * sf, off_t off, const void * buf, size_t size );
enumError WriteZeroWDF	( SUPERFILE * sf, off_t off, size_t size );

// chunk managment
WDF_Chunk_t * NeedChunkWDF ( SUPERFILE * sf, int at_index );

#undef SUPERFILE

//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_WDF_H

