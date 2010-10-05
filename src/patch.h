
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

#ifndef WIT_PATCH_H
#define WIT_PATCH_H 1

#include <stdio.h>
#include "types.h"
#include "lib-std.h"
#include "wiidisc.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global options			///////////////
///////////////////////////////////////////////////////////////////////////////

extern int opt_hook; // true: force relocation hook

//-----------------------------------------------------------------------------

typedef enum enumEncoding
{
	// some flags

	ENCODE_F_FAST		= 0x0001, // fast encoding wanted
	ENCODE_F_ENCRYPT	= 0x0002, // encryption and any signing wanted

	// the basic jobs

	ENCODE_CLEAR_HASH	= 0x0010, // clear hash area of each sector
	ENCODE_CALC_HASH	= 0x0020, // calc hash values for each sector
	ENCODE_DECRYPT		= 0x0100, // decrypt sectors
	ENCODE_ENCRYPT		= 0x0200, // encrypt sectors
	ENCODE_NO_SIGN		= 0x1000, // clear signing area
	ENCODE_SIGN		= 0x2000, // fake sign
	
	// the masks

	ENCODE_M_HASH		= ENCODE_CLEAR_HASH | ENCODE_CALC_HASH,
	ENCODE_M_CRYPT		= ENCODE_DECRYPT | ENCODE_ENCRYPT,
	ENCODE_M_SIGN		= ENCODE_NO_SIGN | ENCODE_SIGN,
	
	ENCODE_MASK		= ENCODE_M_HASH | ENCODE_M_CRYPT | ENCODE_M_SIGN
				| ENCODE_F_FAST | ENCODE_F_ENCRYPT,

	ENCODE_DEFAULT		= 0,	// initial value

} enumEncoding;

extern enumEncoding encoding;
enumEncoding ScanEncoding ( ccp arg );
int ScanOptEncoding ( ccp arg );
enumEncoding SetEncoding
	( enumEncoding val, enumEncoding set_mask, enumEncoding default_mask );

//-----------------------------------------------------------------------------

typedef enum enumRegion
{
	REGION_JAP	= 0,	// Japan
	REGION_USA	= 1,	// USA
	REGION_EUR	= 2,	// Europe
	REGION_KOR	= 4,	// Korea

	REGION__AUTO	= 0x7ffffffe,	// auto detect
	REGION__FILE	= 0x7fffffff,	// try to load from file
	REGION__ERROR	= -1,		// hint: error while scanning

} enumRegion;

extern enumRegion opt_region;
enumRegion ScanRegion ( ccp arg );
int ScanOptRegion ( ccp arg );
ccp GetRegionName ( enumRegion region, ccp unkown_value );

//-----------------------------------------------------------------------------

typedef struct RegionInfo_t
{
	enumRegion reg;	    // the region
	bool mandatory;	    // is 'reg' mandatory?
	char name4[4];	    // short name with maximal 4 characters
	ccp name;	    // long region name

} RegionInfo_t;

const RegionInfo_t * GetRegionInfo ( char region_code );

//-----------------------------------------------------------------------------

extern enumRegion opt_common_key;
wd_ckey_index_t ScanCommonKey ( ccp arg );
int ScanOptCommonKey ( ccp arg );

//-----------------------------------------------------------------------------

extern u64 opt_ios;
extern bool opt_ios_valid;

bool ScanSysVersion ( u64 * ios, ccp arg );
int ScanOptIOS ( ccp arg );

//-----------------------------------------------------------------------------

extern wd_modify_t opt_modify;
wd_modify_t ScanModify ( ccp arg );
int ScanOptModify ( ccp arg );

//-----------------------------------------------------------------------------

extern ccp modify_id;
extern ccp modify_name;

int ScanOptId ( ccp arg );
int ScanOptName ( ccp arg );

bool PatchId ( void * id, int skip, int maxlen, wd_modify_t condition );
bool CopyPatchedDiscId ( void * dest, const void * src );
bool PatchName ( void * name, wd_modify_t condition );

//-----------------------------------------------------------------------------

typedef enum enumTrim
{
	//--- main trimming modes

	TRIM_DEFAULT	= 0x001,	// default mode (no user value)

	TRIM_DISC	= 0x002,	// trim disc: move whole partitions
	TRIM_PART	= 0x004,	// trim partition: move sectors
	TRIM_FST	= 0x008,	// trim filesystem: move files

	TRIM_NONE	= 0x000,
	TRIM_ALL	= 0x00f,
	TRIM_FAST	= TRIM_DISC | TRIM_PART,

	//--- trimming flags

	TRIM_F_END	= 0x100,	// flags for TRIM_DISC: move to disc end

	TRIM_M_FLAGS	= 0x100,

	//--- all valid bits

	TRIM_M_ALL	= TRIM_ALL | TRIM_M_FLAGS

} enumTrim;

extern enumTrim opt_trim;

enumTrim ScanTrim
(
    ccp arg,			// argument to scan
    ccp err_text_extend		// error message extention
);

int ScanOptTrim
(
    ccp arg			// argument to scan
);

//-----------------------------------------------------------------------------

extern u32 opt_align1;
extern u32 opt_align2;
extern u32 opt_align3;
extern u32 opt_align_part;

int ScanOptAlign ( ccp arg );
int ScanOptAlignPart ( ccp arg );

//-----------------------------------------------------------------------------

extern u64 opt_disc_size;

int ScanOptDiscSize ( ccp arg );

//-----------------------------------------------------------------------------

extern StringField_t add_file;
extern StringField_t repl_file;

int ScanOptFile ( ccp arg, bool add );

//
///////////////////////////////////////////////////////////////////////////////
///////////////                        END                      ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_PATCH_H
