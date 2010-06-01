
#ifndef WIT_PATCH_H
#define WIT_PATCH_H 1

#include "types.h"
#include "stdio.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global options			///////////////
///////////////////////////////////////////////////////////////////////////////

extern bool hook_enabled; // [2do] for testing only, [obsolete]

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

typedef enum enumModify
{
	MODIFY__NONE	= 0,	  // modify nothing

	MODIFY_DISC	= 0x001,  // modify disc header
	MODIFY_BOOT	= 0x002,  // modify boot.bin
	MODIFY_TICKET	= 0x004,  // modify ticket.bin
	MODIFY_TMD	= 0x008,  // modify tmd.bin
	MODIFY_WBFS	= 0x010,  // modify WBFS inode

	MODIFY__ALL	= 0x01f,  // modify all
	MODIFY__AUTO	= 0x100,  // automatic mode
	MODIFY__MASK	= MODIFY__ALL | MODIFY__AUTO,

	MODIFY__ERROR	= -1, // hint: error while scanning
	
} enumModify;

extern enumModify opt_modify;
enumModify ScanModify ( ccp arg );
int ScanOptModify ( ccp arg );

//-----------------------------------------------------------------------------

extern ccp modify_id;
extern ccp modify_name;

int ScanOptId ( ccp arg );
int ScanOptName ( ccp arg );

bool PatchId ( void * id, int maxlen, enumModify condition );
bool CopyPatchedDiscId ( void * dest, const void * src );
bool PatchName ( void * name, enumModify condition );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			patch structs			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumPatchMode
{
	PATCHMD_ID,		// ??? [unused] patch ID, '.' -> leave data unchanged
	PATCHMD_DATA,		// ??? [unused] copy data
	PATCHMD_DATA_ALLOCED,	// ??? [unused] copy data, free data pointer
	PATCHMD_DATA_FILE,	// ??? [unused] 'data' is a filename

} enumPatchMode;

///////////////////////////////////////////////////////////////////////////////

typedef struct PatchItem_t
{
	// patch control

	enumPatchMode pmode;		// patch mode
	u32 offset;			// sector offset
	u32 size;			// patch size
	ccp comment;			// pointer to comment

	// patch data

	char * data;			// data or filename
	u32 skip;			// skip first bytes of loaded files

} PatchItem_t;

///////////////////////////////////////////////////////////////////////////////

typedef struct PatchMap_t
{
	u64 offset;			// disc offset
	u32 size;			// block size
	struct WiiFstPart_t * part;	// NULL or pointer to partition

	PatchItem_t * item;		// first item of item list
	u32 item_used;			// number of used elements in 'item'
	u32 item_size;			// number of allocated elements in 'item'

} PatchMap_t;

///////////////////////////////////////////////////////////////////////////////

typedef struct Patch_t
{
	PatchMap_t * map;		// first item of map list
	u32 map_used;			// number of used elements in 'map'
	u32 map_size;			// number of allocated elements in 'map'

} Patch_t;

///////////////////////////////////////////////////////////////////////////////

void InitializePatch ( Patch_t * pat );
void ResetPatch ( Patch_t * pat );
void PrintPatch ( Patch_t * pat, FILE * f, int indent, int verbose );

struct SuperFile_t;

PatchItem_t * InsertDiscDataPM
(
	struct SuperFile_t * sf,
	void * data,
	u64 offset,
	u32 size,
	bool alloced
);

PatchItem_t * InsertPartDataPM
(
	struct SuperFile_t * sf,
	struct PatchItem_t * part,
	void * data,
	u64 offset,
	u32 size,
	bool alloced
);

PatchItem_t * InsertPartFilePM
(
	struct SuperFile_t * sf,
	struct PatchItem_t * part,
	ccp path,
	u64 offset,
	u32 size,
	bool alloced
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////                        END                      ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_PATCH_H
