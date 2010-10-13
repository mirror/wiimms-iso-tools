
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

#ifndef WIT_LIB_STD_H
#define WIT_LIB_STD_H 1

#define _GNU_SOURCE 1

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "types.h"
#include "system.h"
#include "lib-error.h"
#include "libwbfs/file-formats.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     some defs                   ///////////////
///////////////////////////////////////////////////////////////////////////////

#if defined(DEBUG) || defined(TEST)
    #undef EXTENDED_ERRORS
    #define EXTENDED_ERRORS 1		// undef | 1 | 2
#endif

#ifndef EXTENDED_IO_FUNC
    #define EXTENDED_IO_FUNC 1		// 0 | 1
#endif

///////////////////////////////////////////////////////////////////////////////

typedef enum enumProgID
{
	PROG_UNKNOWN,
	PROG_WIT,
	PROG_WWT,
	PROG_WDF,
	PROG_WDF_CAT,
	PROG_WDF_DUMP,

} enumProgID;

typedef enum enumRevID
{
	REVID_UNKNOWN		= 0x00000000,
	REVID_WIIMM		= 0x10000000,
	REVID_WIIMM_TRUNK	= 0x20000000,

} enumRevID;

///////////////////////////////////////////////////////////////////////////////

#define TRACE_SEEK_FORMAT "%-20.20s f=%d,%p %9llx%s\n"
#define TRACE_RDWR_FORMAT "%-20.20s f=%d,%p %9llx..%9llx %8zx%s\n"

#define FILE_PRELOAD_SIZE	0x200
#define MIN_SPARSE_HOLE_SIZE	4096 // bytes
#define MAX_SPLIT_FILES		100
#define MIN_SPLIT_SIZE		100000000
#define ISO_SPLIT_DETECT_SIZE	(4ull*GiB)

#define DEF_SPLIT_SIZE		4000000000ull //  4 GB (not GiB)
#define DEF_SPLIT_SIZE_ISO	0xffff8000ull //  4 GiB - 32 KiB
#define DEF_SPLIT_FACTOR	         0ull // not set
#define DEF_SPLIT_FACTOR_ISO	    0x8000ull // 32 KiB

#define DEF_RECURSE_DEPTH	 10
#define MAX_RECURSE_DEPTH	100

//
///////////////////////////////////////////////////////////////////////////////
///////////////                       Setup                     ///////////////
///////////////////////////////////////////////////////////////////////////////

void SetupLib ( int argc, char ** argv, ccp p_progname, enumProgID prid );

typedef enumError (*check_opt_func) ( int argc, char ** argv, bool mode );

enumError CheckEnvOptions ( ccp varname, check_opt_func );

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  EXTENDED_ERRORS                ///////////////
///////////////////////////////////////////////////////////////////////////////

#undef XPARM
#undef XCALL
#undef XERROR0
#undef XERROR1

#if EXTENDED_IO_FUNC

 #define XPARM ccp func, ccp file, uint line,
 #define XCALL func,file,line,
 #define XERROR0 func,file,line,0
 #define XERROR1 func,file,line,errno

 #define ResetFile(f,r)		XResetFile	(__FUNCTION__,__FILE__,__LINE__,f,r)
 #define ClearFile(f,r)		XClearFile	(__FUNCTION__,__FILE__,__LINE__,f,r)
 #define CloseFile(f,r)		XCloseFile	(__FUNCTION__,__FILE__,__LINE__,f,r)
 #define SetFileTime(f,s)	XSetFileTime	(__FUNCTION__,__FILE__,__LINE__,f,s)
 #define OpenFile(f,n,i)	XOpenFile	(__FUNCTION__,__FILE__,__LINE__,f,n,i)
 #define OpenFileModify(f,n,i)	XOpenFileModify	(__FUNCTION__,__FILE__,__LINE__,f,n,i)
 #define CheckCreated(f,d)	XCheckCreated	(__FUNCTION__,__FILE__,__LINE__,f,d)
 #define CreateFile(f,n,i,o)	XCreateFile	(__FUNCTION__,__FILE__,__LINE__,f,n,i,o)
 #define OpenStreamFile(f)	XOpenStreamFile	(__FUNCTION__,__FILE__,__LINE__,f)
 #define SetupSplitFile(f,m,s)	XSetupSplitFile	(__FUNCTION__,__FILE__,__LINE__,f,m,s)
 #define CreateSplitFile(f,i)	XCreateSplitFile(__FUNCTION__,__FILE__,__LINE__,f,i)
 #define FindSplitFile(f,i,o)	XFindSplitFile	(__FUNCTION__,__FILE__,__LINE__,f,i,o)
 #define PrintErrorFT(f,m)	XPrintErrorFT	(__FUNCTION__,__FILE__,__LINE__,f,m)
 #define AnalyzeWH(f,h,p)	XAnalyzeWH	(__FUNCTION__,__FILE__,__LINE__,f,h,p)
 #define TellF(f)		XTellF		(__FUNCTION__,__FILE__,__LINE__,f)
 #define SeekF(f,o)		XSeekF		(__FUNCTION__,__FILE__,__LINE__,f,o)
 #define SetSizeF(f,o)		XSetSizeF	(__FUNCTION__,__FILE__,__LINE__,f,o)
 #define ReadF(f,b,c)		XReadF		(__FUNCTION__,__FILE__,__LINE__,f,b,c)
 #define WriteF(f,b,c)		XWriteF		(__FUNCTION__,__FILE__,__LINE__,f,b,c)
 #define ReadAtF(f,o,b,c)	XReadAtF	(__FUNCTION__,__FILE__,__LINE__,f,o,b,c)
 #define WriteAtF(f,o,b,c)	XWriteAtF	(__FUNCTION__,__FILE__,__LINE__,f,o,b,c)
 #define WriteZeroAtF(f,o,c)	XWriteZeroAtF	(__FUNCTION__,__FILE__,__LINE__,f,o,c)
 #define ZeroAtF(f,o,c)		XZeroAtF	(__FUNCTION__,__FILE__,__LINE__,f,o,c)
 #define CreateOutFile(o,f,m,w)	XCreateOutFile	(__FUNCTION__,__FILE__,__LINE__,o,f,m,w)
 #define CloseOutFile(o,s)	XCloseOutFile	(__FUNCTION__,__FILE__,__LINE__,o,s)
 #define RemoveOutFile(o)	XRemoveOutFile	(__FUNCTION__,__FILE__,__LINE__,o)

#else

 #define XPARM
 #define XCALL
 #define XERROR0 __FUNCTION__,__FILE__,__LINE__,0
 #define XERROR1 __FUNCTION__,__FILE__,__LINE__,errno

 #define ResetFile(f,r)		XResetFile	(f,r)
 #define ClearFile(f,r)		XClearFile	(f,r)
 #define CloseFile(f,r)		XCloseFile	(f,r)
 #define SetFileTime(f,s)	XSetFileTime	(f,s)
 #define OpenFile(f,n,i)	XOpenFile	(f,n,i)
 #define OpenFileModify(f,n,i)	XOpenFileModify	(f,n,i)
 #define CheckCreated(f,d)	XCheckCreated	(f,d)
 #define CreateFile(f,n,i,o)	XCreateFile	(f,n,i,o)
 #define OpenStreamFile(f)	XOpenStreamFile	(f)
 #define SetupSplitFile(f,m,s)	XSetupSplitFile	(f,m,s)
 #define CreateSplitFile(f,i)	XCreateSplitFile(f,i)
 #define FindSplitFile(f,i,o)	XFindSplitFile	(f,i,o)
 #define PrintErrorFT(f,m)	XPrintErrorFT	(f,m)
 #define AnalyzeWH(f,h,p)	XAnalyzeWH	(f,h,p)
 #define TellF(f)		XTellF		(f)
 #define SeekF(f,o)		XSeekF		(f,o)
 #define SetSizeF(f,o)		XSetSizeF	(f,o)
 #define ReadF(f,b,c)		XReadF		(f,b,c)
 #define WriteF(f,b,c)		XWriteF		(f,b,c)
 #define ReadAtF(f,o,b,c)	XReadAtF	(f,o,b,c)
 #define WriteAtF(f,o,b,c)	XWriteAtF	(f,o,b,c)
 #define WriteZeroAtF(f,o,c)	XWriteZeroAtF	(f,o,c)
 #define ZeroAtF(f,o,c)		XZeroAtF	(f,o,c)
 #define CreateOutFile(o,f,m,w)	XCreateOutFile	(o,f,m,w)
 #define CloseOutFile(o,s)	XCloseOutFile	(o,s)
 #define RemoveOutFile(o)	XRemoveOutFile	(o)

#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			terminal cap			///////////////
///////////////////////////////////////////////////////////////////////////////

extern u32 opt_width;
int ScanOptWidth ( ccp source ); // returns '1' on error, '0' else

int GetTermWidth ( int default_value, int min_value );
int GetTermWidthFD ( int fd, int default_value, int min_value );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    timer			///////////////
///////////////////////////////////////////////////////////////////////////////

u32 GetTimerMSec();
ccp PrintMSec ( char * buf, int bufsize, u32 msec, bool PrintMSec );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Open File Mode			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumIOMode
{
	IOM_IS_WBFS_PART	= 0x01, // is a WBFS partition
	IOM_IS_IMAGE		= 0x02, // is a disc image (PLAIN, WDF, CISO, ...)
	IOM_IS_WIA		= 0x04, // is a WIA file

	IOM__IS_MASK		= 0x07,
	IOM__IS_DEFAULT		= 0,

	IOM_FORCE_STREAM	= IOM__IS_MASK + 1,
	IOM_NO_STREAM		= 0

} enumIOMode;

extern enumIOMode opt_iomode;
void ScanIOMode ( ccp arg );

//-----------------------------------------------------------------------------

typedef enum enumOFT // open file mode
{
    OFT_UNKNOWN,		// not specified yet

    OFT_PLAIN,			// plain (ISO) file
    OFT_WDF,			// WDF file
    OFT_CISO,			// CISO file
    OFT_WBFS,			// WBFS disc
    OFT_WIA,			// WIA file
    OFT_FST,			// file system

    OFT__N,			// number of variants
    OFT__DEFAULT = OFT_WDF	// default value

} enumOFT;

//-----------------------------------------------------------------------------

typedef enum attribOFT // OFT attributes
{
    OFT_A_READ		= 0x01,		// format can be read
    OFT_A_WRITE		= 0x02,		// format can be written
    OFT_A_EXTEND	= 0x04,		// format can be extended
    OFT_A_EDIT		= 0x08,		// format can be edited
    OFT_A_FST		= 0x10,		// format is an extracted file system

} attribOFT;

//-----------------------------------------------------------------------------

typedef struct OFT_info_t
{
    enumOFT		oft;	// = index
    attribOFT		attrib;	// attributes
    enumIOMode		iom;	// preferred IO mode

    ccp			name;	// name
    ccp			ext1;	// standard file extension (maybe an empty string)
    ccp			ext2;	// NULL or alternative file extension
    ccp			info;	// short text info
    
} OFT_info_t;

//-----------------------------------------------------------------------------

extern const OFT_info_t oft_info[OFT__N+1];
extern enumOFT output_file_type;
extern int opt_truncate;

enumOFT CalcOFT ( enumOFT force, ccp fname_dest, ccp fname_src, enumOFT def );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			file support			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumFileType
{
    // 1. file types

	FT_UNKNOWN	= 0,     // not analyzed yet

	FT_ID_DIR	= 0x0001,  // is a directory
	FT_ID_FST	= 0x0002,  // is a directory with a FST
	FT_ID_WBFS	= 0x0004,  // file is a WBFS
	FT_ID_GC_ISO	= 0x0008,  // file is a GC ISO image
	FT_ID_WII_ISO	= 0x0010,  // file is a WII ISO image

	FT_ID_DOL	= 0x0100,  // file is a DOL file
	FT_ID_TIK_BIN	= 0x0200,  // 'ticket.bin' like file
	FT_ID_TMD_BIN	= 0x0400,  // 'tmd.bin' like file
	FT_ID_HEAD_BIN	= 0x0800,  // 'header.bin' like file
	FT_ID_BOOT_BIN	= 0x1000,  // 'boot.bin' like file
	FT_ID_FST_BIN	= 0x2000,  // 'fst.bin' like file
	 FT__SPC_MASK	= 0x3f00,  // mask of all special files

	FT_ID_OTHER	= 0x4000,  // unknown file
	 FT__ID_MASK	= 0x7f1f,  // mask of all 'FT_ID_' values

    // 2. attributes

	FT_A_ISO	= 0x00010000,  // file is some kind of a ISO image
	FT_A_GC_ISO	= 0x00020000,  // file is some kind of a GC ISO image
	FT_A_WII_ISO	= 0x00040000,  // file is some kind of a WII ISO image

	FT_A_WDISC	= 0x00100000,  // flag: specific disc of an WBFS file
	FT_A_WDF	= 0x00200000,  // flag: file is a packed WDF
	FT_A_WIA	= 0x00400000,  // flag: file is a packed WIA
	FT_A_CISO	= 0x00800000,  // flag: file is a packed CISO
	FT_A_REGFILE	= 0x01000000,  // flag: file is a regular file
	FT_A_BLOCKDEV	= 0x02000000,  // flag: file is a block device
	FT_A_CHARDEV	= 0x04000000,  // flag: file is a block device
	FT_A_SEEKABLE	= 0x08000000,  // flag: using of seek() is possible
	FT_A_WRITING	= 0x10000000,  // is opened for writing
	FT_A_PART_DIR	= 0x20000000,  // FST is a partition
	FT__A_MASK	= 0x3ff70000,  // mask of all 'FT_F_' values

    // 3. mask of all 'FT_ values

	FT__MASK	= FT__ID_MASK | FT__A_MASK,

} enumFileType;

//-----------------------------------------------------------------------------

typedef struct FileCache_t
{
	off_t	off;			// file offset
	size_t	count;			// size of cached data
	ccp	data;			// pointer to cached data (alloced)
	struct FileCache_t * next;	// NULL or pointer to next element

} FileCache_t;

//-----------------------------------------------------------------------------

typedef struct FileAttrib_t
{
	// attributes of a (virtual) file

	off_t  size;	// size
	time_t itime;	// itime
	time_t mtime;	// mtime
	time_t ctime;	// ctime
	time_t atime;	// atime

} FileAttrib_t;

//---------------

struct WDiscInfo_t;
struct wbfs_inode_info_t;

FileAttrib_t * NormalizeFileAttrib ( FileAttrib_t * fa );
FileAttrib_t * CopyFileAttrib      ( FileAttrib_t * dest, const FileAttrib_t * src );
FileAttrib_t * CopyFileAttribStat  ( FileAttrib_t * dest, const struct stat * src );
FileAttrib_t * CopyFileAttribDiscInfo
		( FileAttrib_t * dest, const struct WDiscInfo_t * src );
FileAttrib_t * CopyFileAttribInode
		( FileAttrib_t * dest, const struct wbfs_inode_info_t * src, off_t size );

//-----------------------------------------------------------------------------

typedef struct File_t
{
	// file handles and status

	int fd;			// file handle, -1=invalid
	FILE * fp;		// stream handle, 0=invalid
	struct stat st;		// file status after OpenFile()
	int  active_open_flags;	// active open flags.
	enumFileType ftype;	// the type of the file
	bool is_reading;	// file opened in read mode;
	bool is_writing;	// file opened in write mode;
	bool is_stdfile;	// file is stdin or stdout
	bool seek_allowed;	// seek is allowed (regular file or block device)
	char id6[7];		// ID6 of a iso image

	// virtual file atributes, initialized by a copy of 'struct stat st'

	FileAttrib_t fatt;	// size, itime, mtime, ctime, atime

	// file names, alloced

	ccp fname;		// current virtual filename
	ccp path;		// not NULL: path of real file (not realpath)
	ccp rename;		// not NULL: rename rename to fname if closed
	ccp outname;		// not NULL: hint for a good output filename
				// outname is without path/directory or extension

	// options set by user, not resetted by ResetFile()

	int  open_flags;	// proposed open flags; if zero then ignore
	bool disable_errors;	// don't print error messages
	bool create_directory;	// create direcotries automatically

	// error codes

	enumError last_error;	// error code of last operation
	enumError max_error;	// max error code since open/create

	// offset handling

	off_t file_off;		// current real file offset
	off_t cur_off;		// current virtual file offset
	off_t max_off;		// max file offset
	int   read_behind_eof;	// 0:disallow, 1:allow+print warning, 2:allow silently

	// read cache

	bool is_caching;	// true if cache is active
	FileCache_t *cache;	// data cache
	FileCache_t *cur_cache;	// pointer to current cache entry
	off_t  cache_info_off;	// info for cache missed message
	size_t cache_info_size;	// info for cache missed message

	// split file support

	struct File_t **split_f; // list with pointers to the split files
	int split_used;		 // number of used split files in 'split_f'
	off_t split_filesize;	 // max file size for new files
	ccp split_fname_format;	 // format with '%u' at the end for 'fname'
	ccp split_rename_format; // format with '%u' at the end for 'rename'

	// wbfs vars

	u32 sector_size;	// size of one hd sector, default = 512

	// statistics

	u32 tell_count;		// number of successfull tell operations
	u32 seek_count;		// number of successfull seek operations
	u32 setsize_count;	// number of successfull set-size operations
	u32 read_count;		// number of successfull read operations
	u32 write_count;	// number of successfull write operations
	u64 bytes_read;		// number of bytes read
	u64 bytes_written;	// number of bytes written

} File_t;

//-----------------------------------------------------------------------------

// initialize, reset and close files
void InitializeFile	( File_t * f );
enumError XResetFile	( XPARM File_t * f, bool remove_file );
enumError XClearFile	( XPARM File_t * f, bool remove_file );
enumError XCloseFile	( XPARM File_t * f, bool remove_file );
enumError XSetFileTime	( XPARM File_t * f, FileAttrib_t * set_time );

// open files
enumError XOpenFile       ( XPARM File_t * f, ccp fname, enumIOMode iomode );
enumError XOpenFileModify ( XPARM File_t * f, ccp fname, enumIOMode iomode );
enumError XCreateFile     ( XPARM File_t * f, ccp fname, enumIOMode iomode, int overwrite );
enumError XCheckCreated   ( XPARM             ccp fname, bool disable_errors );
enumError XOpenStreamFile ( XPARM File_t * f );
enumError XSetupSplitFile ( XPARM File_t *f, enumOFT oft, off_t file_size );
enumError XCreateSplitFile( XPARM File_t *f, uint split_idx );
enumError XFindSplitFile  ( XPARM File_t *f, uint * index, off_t * off );

// copy filedesc
void CopyFD ( File_t * dest, File_t * src );

// read cache support
void ClearCache		 ( File_t * f );
void DefineCachedArea    ( File_t * f, off_t off, size_t count );
void DefineCachedAreaISO ( File_t * f, bool head_only );

struct WDF_Head_t;
enumError XAnalyzeWH ( XPARM File_t * f, struct WDF_Head_t * wh, bool print_err );

enumError StatFile ( struct stat * st, ccp fname, int fd );

//-----------------------------------------------------------------------------
// file io with error messages

enumError XTellF	 ( XPARM File_t * f );
enumError XSeekF	 ( XPARM File_t * f, off_t off );
enumError XSetSizeF	 ( XPARM File_t * f, off_t off );
enumError XReadF	 ( XPARM File_t * f,                  void * iobuf, size_t count );
enumError XWriteF	 ( XPARM File_t * f,            const void * iobuf, size_t count );
enumError XReadAtF	 ( XPARM File_t * f, off_t off,       void * iobuf, size_t count );
enumError XWriteAtF	 ( XPARM File_t * f, off_t off, const void * iobuf, size_t count );
enumError XWriteZeroAtF	 ( XPARM File_t * f, off_t off,                     size_t count );
enumError XZeroAtF	 ( XPARM File_t * f, off_t off,                     size_t count );

//-----------------------------------------------------------------------------
// wrapper functions

int WrapperReadSector  ( void * handle, u32 lba, u32 count, void * iobuf );
int WrapperWriteSector ( void * handle, u32 lba, u32 count, void * iobuf );

//-----------------------------------------------------------------------------
// split file support

int CalcSplitFilename ( char * buf, size_t buf_size, ccp path, enumOFT oft );
char * AllocSplitFilename ( ccp path, enumOFT oft );

//-----------------------------------------------------------------------------
// filename generation

char * NormalizeFileName ( char * buf, char * buf_end, ccp source, bool allow_slash );

void SetFileName      ( File_t * f, ccp source, bool allow_slash );
void GenFileName      ( File_t * f, ccp path, ccp name, ccp title, ccp id6, ccp ext );
void GenDestFileName  ( File_t * f, ccp dest, ccp default_name, ccp ext, bool rm_std_ext );
void GenImageFileName ( File_t * f, ccp dest, ccp default_name, enumOFT oft );

//-----------------------------------------------------------------------------
// etc

int    GetFD ( const File_t * f );
FILE * GetFP ( const File_t * f );
char   GetFT ( const File_t * f );

bool IsOpenF ( const File_t * f );
bool IsSplittedF ( const File_t * f );

bool IsDirectory ( ccp fname, bool answer_if_empty );
enumError CreatePath ( ccp fname );

typedef enum enumFileMode { FM_OTHER, FM_PLAIN, FM_BLKDEV, FM_CHRDEV } enumFileMode;
enumFileMode GetFileMode ( mode_t mode );
ccp GetFileModeText ( enumFileMode mode, bool longtext, ccp fail_text );

#ifdef __CYGWIN__
    int IsWindowsDriveSpec ( ccp src );
    int NormalizeFilenameCygwin ( char * buf, size_t bufsize, ccp source );
    char * AllocNormalizedFilenameCygwin ( ccp source );
#endif

void SetDest ( ccp arg, bool mkdir );

s64 GetFileSize
(
    ccp		path1,		// NULL or part 1 of path
    ccp		path2,		// NULL or part 2 of path
    s64		not_found_value	// return value if no regular file found
);

enumError LoadFile ( ccp path1, ccp path2, size_t skip,
		     void * data, size_t size, bool silent );
enumError SaveFile ( ccp path1, ccp path2, bool create_dir,
		     void * data, size_t size, bool silent );

//
///////////////////////////////////////////////////////////////////////////////
///////////////                string functions                 ///////////////
///////////////////////////////////////////////////////////////////////////////

extern const char EmptyString[]; // ""
extern const char MinusString[]; // "-"

//-----

// frees string if str is not NULL|EmptyString|MinusString
void FreeString ( ccp str );

// works like strdup();
void * MemDup ( const void * src, size_t copylen );

//-----

// StringCopy(), StringCopyE(), StringCat*()
//	RESULT: end of copied string pointing to NULL.
//	'src*' may be a NULL pointer.

char * StringCopyS ( char * buf, size_t bufsize, ccp src );
char * StringCat2S ( char * buf, size_t bufsize, ccp src1, ccp src2 );
char * StringCat3S ( char * buf, size_t bufsize, ccp src1, ccp src2, ccp src3 );

char * StringCopyE ( char * buf, char * buf_end, ccp src );
char * StringCat2E ( char * buf, char * buf_end, ccp src1, ccp src2 );
char * StringCat3E ( char * buf, char * buf_end, ccp src1, ccp src2, ccp src3 );

ccp PathCatPP  ( char * buf, size_t bufsize, ccp path1, ccp path2 );
ccp PathCatPPE ( char * buf, size_t bufsize, ccp path1, ccp path2, ccp ext );

//-----

int NormalizeIndent ( int indent );

//-----

int CheckIDHelper // helper for all other id functions
(
	const void	* id,		// valid pointer to test ID
	int		max_len,	// max length of ID, a NULL terminates ID too
	bool		allow_any_len,	// if false, only length 4 and 6 are allowed
	bool		ignore_case,	// lower case letters are allowed
	bool		allow_point	// the wildcard '.' is allowed
);

int CheckID // check up to 7 chars for ID4|ID6
(
	const void	* id,		// valid pointer to test ID
	bool		ignore_case,	// lower case letters are allowed
	bool		allow_point	// the wildcard '.' is allowed
);

bool CheckID4 // check exact 4 chars
(
	const void	* id,		// valid pointer to test ID
	bool		ignore_case,	// lower case letters are allowed
	bool		allow_point	// the wildcard '.' is allowed
);

bool CheckID6 // check exact 6 chars
(
	const void	* id,		// valid pointer to test ID
	bool		ignore_case,	// lower case letters are allowed
	bool		allow_point	// the wildcard '.' is allowed
);

int CountIDChars // count number of valid ID chars, max = 1000
(
	const void	* id,		// valid pointer to test ID
	bool		ignore_case,	// lower case letters are allowed
	bool		allow_point	// the wildcard '.' is allowed
);

char * ScanID	    ( char * destbuf7, int * destlen, ccp source );

//-----

char * ScanNumU32   ( ccp arg, u32 * p_stat, u32 * p_num,            u32 min, u32 max );
char * ScanRangeU32 ( ccp arg, u32 * p_stat, u32 * p_n1, u32 * p_n2, u32 min, u32 max );

//-----

char * ScanHexHelper
(
    void	* buf,		// valid pointer to result buf
    int		buf_size,	// number of byte to read
    int		* bytes_read,	// NULL or result: number of read bytes
    ccp		arg,		// source string
    int		err_level	// error level (print message):
				//	 = 0: don't print errors
				//	>= 1: print syntax errors
				//	>= 2: msg if bytes_read<buf_size
				//	>= 3: msg if arg contains more characters
);

enumError ScanHex
(
    void	* buf,		// valid pointer to result buf
    int		buf_size,	// number of byte to read
    ccp		arg		// source string
);

///////////////////////////////////////////////////////////////////////////////
///////////////                     scan size                   ///////////////
///////////////////////////////////////////////////////////////////////////////

u64 ScanSizeFactor ( char ch_factor, int force_base );

char * ScanSizeTerm ( double * num, ccp source,
		      u64 default_factor, int force_base );

char * ScanSize ( double * num, ccp source,
		  u64 default_factor1, u64 default_factor2, int force_base );

char * ScanSizeU32 ( u32 * num, ccp source,
		     u64 default_factor1, u64 default_factor2, int force_base );

char * ScanSizeU64 ( u64 * num, ccp source,
		     u64 default_factor1, u64 default_factor2, int force_base );

enumError ScanSizeOpt
	( double * num, ccp source,
	  u64 default_factor1, u64 default_factor2, int force_base,
	  ccp opt_name, u64 min, u64 max, bool print_err );

enumError ScanSizeOptU64
	( u64 * num, ccp source, u64 default_factor1, int force_base,
	  ccp opt_name, u64 min, u64 max, u32 multiple, u32 pow2, bool print_err );

enumError ScanSizeOptU32
	( u32 * num, ccp source, u64 default_factor1, int force_base,
	  ccp opt_name, u64 min, u64 max, u32 multiple, u32 pow2, bool print_err );

//-----------------------------------------------------------------------------

extern int opt_split;
extern u64 opt_split_size;

int ScanOptSplitSize ( ccp source ); // returns '1' on error, '0' else

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     sort mode                   ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum SortMode
{
	SORT_NONE,		// == 0
	SORT_ID,
	SORT_NAME,
	SORT_TITLE,
	SORT_FILE,
	SORT_SIZE,
	SORT_OFFSET,
	SORT_REGION,
	SORT_WBFS,
	SORT_NPART,
	 SORT_ITIME,
	 SORT_MTIME,
	 SORT_CTIME,
	 SORT_ATIME,
	 SORT_TIME,
	SORT_DEFAULT,
	 SORT__MODE_MASK	= 0x1f,

	SORT_REVERSE		= 0x20,
	SORT__MASK		= SORT_REVERSE | SORT__MODE_MASK,

	SORT__ERROR		= -1 // not a mode but an error message

} SortMode;

extern SortMode sort_mode;
SortMode ScanSortMode ( ccp arg );
int ScanOptSort ( ccp arg );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			options show + unit		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum ShowMode
{
	//----- base values

	SHOW__NONE	= 0,

	SHOW_INTRO	= 0x00000001, // introduction
	SHOW_P_TAB	= 0x00000002, // partition table
	SHOW_P_INFO	= 0x00000004, // partition info
	SHOW_P_MAP	= 0x00000008, // memory map of partitions
	SHOW_D_MAP	= 0x00000010, // memory map of discs
	SHOW_TICKET	= 0x00000020, // ticket info
	SHOW_TMD	= 0x00000040, // tmd info
	SHOW_USAGE	= 0x00000080, // usage table
	SHOW_FILES	= 0x00000100, // file list
	SHOW_PATCH	= 0x00000200, // patching table
	SHOW_RELOCATE	= 0x00000400, // relocation table
	SHOW_PATH	= 0x00000800, // full path

	SHOW_OFFSET	= 0x00001000, // show offsets
	SHOW_SIZE	= 0x00002000, // show size
	
	SHOW__ALL	= 0x00003fff,

	//----- combinations

	SHOW__PART	= SHOW_P_INFO
			| SHOW_P_MAP
			| SHOW_TICKET
			| SHOW_TMD,

	SHOW__MAP	= SHOW_P_MAP
			| SHOW_D_MAP,

	//----- flags

	SHOW_F_DEC1	= 0x00010000, // prefer DEC, only one of DEC1,HEX1 is set
	SHOW_F_HEX1	= 0x00020000, // prefer HEX, only one of DEC1,HEX1 is set
	SHOW_F_DEC	= 0x00040000, // prefer DEC
	SHOW_F_HEX	= 0x00080000, // prefer HEX,
	SHOW_F__NUM	= 0x000f0000,

	SHOW_F_HEAD	= 0x00100000, // print header lines
	SHOW_F_PRIMARY	= 0x00200000, // print primary (unpatched) disc

	//----- etc

	SHOW__DEFAULT	= (int)0x80000000,
	SHOW__ERROR	= -1 // not a mode but an error message

} ShowMode;

extern ShowMode opt_show_mode;
ShowMode ScanShowMode ( ccp arg );
int ScanOptShow ( ccp arg );

int ConvertShow2PFST
(
	ShowMode show_mode,	// show mode
	ShowMode def_mode	// default mode
);

//-----------------------------------------------------------------------------

extern wd_size_mode_t opt_unit;

wd_size_mode_t ScanUnit ( ccp arg );
int ScanOptUnit ( ccp arg );

//
///////////////////////////////////////////////////////////////////////////////
///////////////             time printing % scanning            ///////////////
///////////////////////////////////////////////////////////////////////////////

enum enumPrintTime
{
	PT_USE_ITIME		= 1,
	PT_USE_MTIME		= 2,
	PT_USE_CTIME		= 3,
	PT_USE_ATIME		= 4,
	 PT__USE_MASK		= 7,

	PT_F_ITIME		= 0x10,
	PT_F_MTIME		= 0x20,
	PT_F_CTIME		= 0x40,
	PT_F_ATIME		= 0x80,
	 PT__F_MASK		= 0xf0,

	PT_PRINT_DATE		= 0x100,
	PT_PRINT_TIME		= 0x200,
	PT_PRINT_SEC		= 0x300,
	 PT__PRINT_MASK		= 0x300,

	PT_SINGLE		= 0x1000,
	PT_MULTI		= 0x2000,
	 PT__MULTI_MASK		= PT_SINGLE | PT_MULTI,

	PT_ENABLED		= 0x10000,
	PT_DISABLED		= 0x20000,
	 PT__ENABLED_MASK	= PT_ENABLED | PT_DISABLED,

	PT__MASK		= PT__USE_MASK | PT__F_MASK | PT__PRINT_MASK
				  | PT__MULTI_MASK | PT__ENABLED_MASK,

	PT__DEFAULT		= PT_USE_MTIME | PT_PRINT_DATE,

	PT__ERROR		= -1
};

///////////////////////////////////////////////////////////////////////////////

#define PT_BUF_SIZE 24

typedef struct PrintTime_t
{
	int  mode;			// active mode (enumPrintTime)
	int  nelem;			// number of printed elements
	int  wd1;			// width of single column includig leading space
	int  wd;			// width of all columns := nelem * wd1

	ccp  format;			// format for strftime (single time)
	ccp  undef;			// text for single undefined times

	char head[4*PT_BUF_SIZE];	// text of table header includig leading space
	char fill[4*PT_BUF_SIZE];	// 'wd' spaces, can be used as filler
	char tbuf[4*PT_BUF_SIZE];	// the formatted time includig leading space

} PrintTime_t;

///////////////////////////////////////////////////////////////////////////////

extern int opt_print_time;

int  ScanPrintTimeMode	( ccp argv, int prev_mode );
int  ScanAndSetPrintTimeMode ( ccp argv );
int  SetPrintTimeMode	( int prev_mode, int new_mode );
int  EnablePrintTime	( int opt_time );
void SetTimeOpt		( int opt_time );

void	SetupPrintTime	( PrintTime_t * pt, int opt_time );
char *	PrintTime	( PrintTime_t * pt, const FileAttrib_t * fa );
time_t	SelectTime	( const FileAttrib_t * fa, int opt_time );
SortMode SelectSortMode	( int opt_time );
time_t	ScanTime	( ccp arg );

//
///////////////////////////////////////////////////////////////////////////////
///////////////              string fields & lists              ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct StringField_t
{
	ccp * field;	// pointer to the string field
	uint used;	// number of used titles in the title field
	uint size;	// number of allocated pointer in 'field'

} StringField_t;

//-----------------------------------------------------------------------------

void InitializeStringField ( StringField_t * sf );
void ResetStringField ( StringField_t * sf );

// return: pointer to matched key if the key is in the field.
ccp FindStringField ( StringField_t * sf, ccp key );

// return: true if item inserted/deleted
bool InsertStringField ( StringField_t * sf, ccp key, bool move_key );
bool RemoveStringField ( StringField_t * sf, ccp key );

// append at the end an do not sort
void AppendStringField ( StringField_t * sf, ccp key, bool move_key );

// return the index of the (next) item
uint FindStringFieldHelper ( StringField_t * sf, bool * found, ccp key );

// file support
enumError ReadStringField ( StringField_t * sf, bool sort, ccp filename, bool silent );
enumError WriteStringField ( StringField_t * sf, ccp filename, bool rm_if_empty );

///////////////////////////////////////////////////////////////////////////////

typedef struct StringList_t
{
	ccp str;
	struct StringList_t * next;

} StringList_t;

///////////////////////////////////////////////////////////////////////////////

typedef struct ParamList_t
{
	char * arg;
	char id6[7];
	char selector[7];
	bool is_expanded;
	int count;
	struct ParamList_t * next;

} ParamList_t;

extern uint n_param, id6_param_found;
extern ParamList_t * first_param;
extern ParamList_t ** append_param;

///////////////////////////////////////////////////////////////////////////////

int AtFileHelper
(
    ccp arg,
    int mode,
    int mode_expand,
    int (*func)(ccp arg,int mode)
);

ParamList_t * AppendParam ( ccp arg, int is_temp );
int AddParam ( ccp arg, int is_temp );
void AtExpandParam ( ParamList_t ** param );
void AtExpandAllParam ( ParamList_t ** first_param );

//
///////////////////////////////////////////////////////////////////////////////
///////////////              string substitutions               ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct SubstString_t
{
	char c1, c2;		// up to 2 codes (lower+upper case)
	bool allow_slash;	// true: allow slash in replacement
	ccp  str;		// pointer to string

} SubstString_t;

char * SubstString
	( char * buf, size_t bufsize, SubstString_t * tab, ccp source, int * count );
int ScanEscapeChar ( ccp arg );

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   Memory Maps                   ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct MemMapItem_t
{
    u64		off;		// offset
    u64		size;		// size
    u8		overlap;	// system info: item overlaps other items
    u8		index;		// user defined index
    char	info[62];	// user defined info text

} MemMapItem_t;

//-----------------------------------------------------------------------------

typedef struct MemMap_t
{
    MemMapItem_t ** field;	// pointer to the item field
    uint	used;		// number of used titles in the item field
    uint	size;		// number of allocated pointer in 'field'
    u64		begin;		// first address

} MemMap_t;

//-----------------------------------------------------------------------------
//	Memory maps allow duplicate entries.
//	The off+size pair is used as key.
//	The entries are sorted by off and size (small values first).
//-----------------------------------------------------------------------------

void InitializeMemMap ( MemMap_t * mm );
void ResetMemMap ( MemMap_t * mm );

// Return: NULL or the pointer to the one! matched key (=off,size)
MemMapItem_t * FindMemMap ( MemMap_t * mm, off_t off, off_t size );

// Return pointer to new item (never NULL)
MemMapItem_t * InsertMemMap ( MemMap_t * mm, off_t off, off_t size );

void InsertMemMapWrapper
(
	void		* param,	// user defined parameter
	u64		offset,		// offset of object
	u64		size,		// size of object
	ccp		info		// info about object
);

struct wd_disc_t;
void InsertDiscMemMap
(
	MemMap_t	* mm,		// valid memore map pointer
	struct wd_disc_t * disc		// valid disc pointer
);

// Remove all entires with key. Return number of delete entries
//uint RemoveMemMap ( MemMap_t * mm, off_t off, off_t size );

// Return the index of the found or next item
uint FindMemMapHelper ( MemMap_t * mm, off_t off, off_t size );

// Calculate overlaps. Return number of items with overlap.
uint CalCoverlapMemMap ( MemMap_t * mm );

// Print out memory map
void PrintMemMap ( MemMap_t * mm, FILE * f, int indent );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			setup files			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct SetupDef_t
{
	ccp	name;		// name of parameter, NULL=list terminator
	u32	factor;		// factor, 0: text param
	ccp	param;		// malloced text param
	u64	value;		// numeric value of param

} SetupDef_t;

size_t ResetSetup
(
	SetupDef_t * list	// object list terminated with an element 'name=NULL'
);

enumError ScanSetupFile
(
	SetupDef_t * list,	// object list terminated with an element 'name=NULL'
	ccp path1,		// filename of text file, part 1
	ccp path2,		// filename of text file, part 2
	bool silent		// true: suppress error message if file not found
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    scan compression option		///////////////
///////////////////////////////////////////////////////////////////////////////

extern wd_compression_t opt_compr_method; // = WD_COMPR__DEFAULT
extern int opt_compr_level;		  // = 0=default, 1..9=valid
extern u32 opt_compr_chunk_size;	  // = 0=default

//-----------------------------------------------------------------------------

wd_compression_t ScanCompression
(
    ccp			arg,		// argument to scan
    bool		silent,		// don't print error message
    int			* level,	// not NULL: appendix '.digit' allowed
					// The level will be stored in '*level'
    u32			* chunk_size	// not NULL: appendix '@size' allowed
					// The size will be stored in '*chunk_size'
);

//-----------------------------------------------------------------------------

int ScanOptCompression
(
    ccp			arg		// argument to scan
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan mem option			///////////////
///////////////////////////////////////////////////////////////////////////////

extern u64 opt_mem;			  // = 0

int ScanOptMem
(
    ccp			arg,		// argument to scan
    bool		print_err	// true: print error messages
);

u64 GetMemLimit();

//
///////////////////////////////////////////////////////////////////////////////
///////////////			commands			///////////////
///////////////////////////////////////////////////////////////////////////////

#define COMMAND_NAME_MAX 100

typedef struct CommandTab_t
{
    s64			id;		// id
    ccp			name1;		// first name
    ccp			name2;		// NULL or second name
    s64			opt;		// option

} CommandTab_t;

typedef s64 (*CommandCallbackFunc)
(
    void		* param,	// NULL or user defined parameter
    ccp			name,		// normalized name of option
    const CommandTab_t	* cmd_tab,	// valid pointer to command table
    const CommandTab_t	* cmd,		// valid pointer to found command
    char		prefix,		// 0 | '-' | '+' | '='
    s64			result		// current value of result
);

const CommandTab_t * ScanCommand
(
    int			* res_abbrev,	// NULL or pointer to result 'abbrev_count'
    ccp			arg,		// argument to scan
    const CommandTab_t	* cmd_tab	// valid pointer to command table
);

s64 ScanCommandList
(
    ccp			arg,		// argument to scan
    const CommandTab_t	* cmd_tab,	// valid pointer to command table
    CommandCallbackFunc	func,		// NULL or calculation function
    bool		allow_prefix,	// allow '-' | '+' | '=' as prefix
    u32			max_number,	// allow numbers < 'max_number' (0=disabled)
    s64			result		// start value for result
);

enumError ScanCommandListFunc
(
    ccp			arg,		// argument to scan
    const CommandTab_t	* cmd_tab,	// valid pointer to command table
    CommandCallbackFunc	func,		// calculation function
    void		* param,	// used define parameter for 'func'
    bool		allow_prefix	// allow '-' | '+' | '=' as prefix
);

s64 ScanCommandListMask
(
    ccp			arg,		// argument to scan
    const CommandTab_t	* cmd_tab	// valid pointer to command table
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			data area & list		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct DataArea_t
{
    const u8		* data;		// pointer to data area
					// for lists: NULL is the end of list marker
    size_t		size;		// size of data area

} DataArea_t;

//-----------------------------------------------------------------------------

typedef struct DataList_t
{
    const DataArea_t	* area;		// pointer to a source list
					//  terminated with an element where addr==NULL
    DataArea_t		current;	// current element

} DataList_t;

//-----------------------------------------------------------------------------

void SetupDataList
(
    DataList_t		* dl,		// Object for setup
    const DataArea_t	* da		// Source list,
					//  terminated with an element where addr==NULL
					// The content of this area must not changed
					//  while accessing the data list
);

size_t ReadDataList // returns number of writen bytes
(
    DataList_t		* dl,		// NULL or pointer to data list
    void		* buf,		// destination buffer
    size_t		size		// size of destination buffer
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  enum RepairMode		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum RepairMode
{
	REPAIR_NONE		=     0,

	REPAIR_FBT		= 0x001, // repair free blocks table
	REPAIR_INODES		= 0x002, // repair invalid inode infos
	 REPAIR_DEFAULT		= 0x003, // standard value

	REPAIR_RM_INVALID	= 0x010, // remove discs with invalid blocks
	REPAIR_RM_OVERLAP	= 0x020, // remove discs with overlaped blocks
	REPAIR_RM_FREE		= 0x040, // remove discs with free marked blocks
	REPAIR_RM_EMPTY		= 0x080, // remove discs without any valid blocks
	 REPAIR_RM_ALL		= 0x0f0, // remove all discs with errors

	REPAIR_ALL		= 0x0f3, // repair all
	
	REPAIR__ERROR		= -1 // not a mode but an error message

} RepairMode;

extern RepairMode repair_mode;

RepairMode ScanRepairMode ( ccp arg );

///////////////////////////////////////////////////////////////////////////////
///////////////			random mumbers			///////////////
///////////////////////////////////////////////////////////////////////////////

u32 Random32 ( u32 max );
u64 Seed32Time();
u64 Seed32 ( u64 base );

void RandomFill ( void * buf, size_t size );

///////////////////////////////////////////////////////////////////////////////
///////////////			    bit handling		///////////////
///////////////////////////////////////////////////////////////////////////////

extern const uchar TableBitCount[0x100];

uint Count1Bits   ( const void * data, size_t len );
uint Count1Bits8  ( u8  data );
uint Count1Bits16 ( u16 data );
uint Count1Bits32 ( u32 data );
uint Count1Bits64 ( u64 data );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    etc				///////////////
///////////////////////////////////////////////////////////////////////////////

size_t AllocTempBuffer ( size_t needed_size );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    vars			///////////////
///////////////////////////////////////////////////////////////////////////////

extern enumProgID	prog_id;
extern u32		revision_id;
extern ccp		progname;
extern ccp		search_path[];
extern ccp		lang_info;
extern volatile int	SIGINT_level;
extern volatile int	verbose;
extern volatile int	logging;
extern int		progress;
extern bool		use_utf8;
extern char		escape_char;
extern ccp		opt_clone;
extern int		testmode;
extern ccp		opt_dest;
extern bool		opt_mkdir;
extern int		opt_limit;
extern int		print_sections;
extern int		long_count;
extern enumIOMode	io_mode;
extern u32		opt_recurse_depth;

extern StringField_t	source_list;
extern StringField_t	recurse_list;
extern StringField_t	created_files;
extern       char	iobuf [0x400000];	// global io buffer
extern const char	zerobuf[0x40000];	// global zero buffer

// 'tempbuf' is only for short usage
//	==> don't call other functions while using tempbuf
extern u8		* tempbuf;		// global temp buffer -> AllocTempBuffer()
extern size_t		tempbuf_size;		// size of 'tempbuf'

extern const char	sep_79[80];		//  79 * '-' + NULL

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_STD_H 1
