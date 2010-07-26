
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

#include "types.h"
#include "stdio.h"
#include "wiidisc.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global options			///////////////
///////////////////////////////////////////////////////////////////////////////

extern bool hook_enabled; // [2do] for testing only, [obsolete]
extern bool opt_hook; // true: force relocation hook

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

//
///////////////////////////////////////////////////////////////////////////////
///////////////                        END                      ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_PATCH_H
