
#ifndef WIT_UI_WIT_H
#define WIT_UI_WIT_H

// *************************************************************************
// *****   This file is automatically generated by the tool 'gen-ui'   *****
// *************************************************************************
// *****                 ==> DO NOT EDIT THIS FILE <==                 *****
// *************************************************************************

#include "lib-std.h"
#include "ui.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                enum enumOptions                 ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumOptions
{
	OPT_NONE,

	//----- command specific options -----

	OPT_SOURCE,
	OPT_RECURSE,
	OPT_RDEPTH,
	OPT_INCLUDE,
	OPT_INCLUDE_PATH,
	OPT_EXCLUDE,
	OPT_EXCLUDE_PATH,
	OPT_IGNORE,
	OPT_IGNORE_FST,
	OPT_PSEL,
	OPT_RAW,
	OPT_PMODE,
	OPT_SNEEK,
	OPT_ENC,
	OPT_DEST,
	OPT_DEST2,
	OPT_SPLIT,
	OPT_SPLIT_SIZE,
	OPT_PRESERVE,
	OPT_UPDATE,
	OPT_OVERWRITE,
	OPT_REMOVE,
	OPT_WDF,
	OPT_ISO,
	OPT_CISO,
	OPT_WBFS,
	OPT_FST,
	OPT_FILES,
	OPT_ITIME,
	OPT_MTIME,
	OPT_CTIME,
	OPT_ATIME,
	OPT_TIME,
	OPT_LONG,
	OPT_UNIQUE,
	OPT_NO_HEADER,
	OPT_SECTIONS,
	OPT_SORT,
	OPT_LIMIT,

	OPT__N_SPECIFIC, // == 40 

	//----- global options -----

	OPT_VERSION,
	OPT_HELP,
	OPT_XHELP,
	OPT_QUIET,
	OPT_VERBOSE,
	OPT_PROGRESS,
	OPT_LOGGING,
	OPT_ESC,
	OPT_IO,
	OPT_TITLES,
	OPT_UTF_8,
	OPT_NO_UTF_8,
	OPT_LANG,
	OPT_TEST,

	OPT__N_TOTAL // == 54

} enumOptions;

//
///////////////////////////////////////////////////////////////////////////////
///////////////               enum enumOptionsBit               ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumOptionsBit
{
	//----- command specific options -----

	OB_SOURCE		= 1llu << OPT_SOURCE,
	OB_RECURSE		= 1llu << OPT_RECURSE,
	OB_RDEPTH		= 1llu << OPT_RDEPTH,
	OB_INCLUDE		= 1llu << OPT_INCLUDE,
	OB_INCLUDE_PATH		= 1llu << OPT_INCLUDE_PATH,
	OB_EXCLUDE		= 1llu << OPT_EXCLUDE,
	OB_EXCLUDE_PATH		= 1llu << OPT_EXCLUDE_PATH,
	OB_IGNORE		= 1llu << OPT_IGNORE,
	OB_IGNORE_FST		= 1llu << OPT_IGNORE_FST,
	OB_PSEL			= 1llu << OPT_PSEL,
	OB_RAW			= 1llu << OPT_RAW,
	OB_PMODE		= 1llu << OPT_PMODE,
	OB_SNEEK		= 1llu << OPT_SNEEK,
	OB_ENC			= 1llu << OPT_ENC,
	OB_DEST			= 1llu << OPT_DEST,
	OB_DEST2		= 1llu << OPT_DEST2,
	OB_SPLIT		= 1llu << OPT_SPLIT,
	OB_SPLIT_SIZE		= 1llu << OPT_SPLIT_SIZE,
	OB_PRESERVE		= 1llu << OPT_PRESERVE,
	OB_UPDATE		= 1llu << OPT_UPDATE,
	OB_OVERWRITE		= 1llu << OPT_OVERWRITE,
	OB_REMOVE		= 1llu << OPT_REMOVE,
	OB_WDF			= 1llu << OPT_WDF,
	OB_ISO			= 1llu << OPT_ISO,
	OB_CISO			= 1llu << OPT_CISO,
	OB_WBFS			= 1llu << OPT_WBFS,
	OB_FST			= 1llu << OPT_FST,
	OB_FILES		= 1llu << OPT_FILES,
	OB_ITIME		= 1llu << OPT_ITIME,
	OB_MTIME		= 1llu << OPT_MTIME,
	OB_CTIME		= 1llu << OPT_CTIME,
	OB_ATIME		= 1llu << OPT_ATIME,
	OB_TIME			= 1llu << OPT_TIME,
	OB_LONG			= 1llu << OPT_LONG,
	OB_UNIQUE		= 1llu << OPT_UNIQUE,
	OB_NO_HEADER		= 1llu << OPT_NO_HEADER,
	OB_SECTIONS		= 1llu << OPT_SECTIONS,
	OB_SORT			= 1llu << OPT_SORT,
	OB_LIMIT		= 1llu << OPT_LIMIT,

	//----- group & command options -----

	OB_GRP_TITLES		= 0,

	OB_GRP_SOURCE		= OB_SOURCE
				| OB_RECURSE
				| OB_RDEPTH,

	OB_GRP_EXCLUDE		= OB_INCLUDE
				| OB_INCLUDE_PATH
				| OB_EXCLUDE
				| OB_EXCLUDE_PATH,

	OB_GRP_XSOURCE		= OB_GRP_SOURCE
				| OB_GRP_EXCLUDE,

	OB_GRP_XXSOURCE		= OB_GRP_SOURCE
				| OB_GRP_EXCLUDE
				| OB_IGNORE
				| OB_IGNORE_FST,

	OB_GRP_XTIME		= OB_ITIME
				| OB_MTIME
				| OB_CTIME
				| OB_ATIME,

	OB_GRP_TIME		= OB_GRP_XTIME
				| OB_TIME,

	OB_GRP_PARTITIONS	= OB_PSEL
				| OB_RAW,

	OB_GRP_FILES		= OB_PMODE
				| OB_FILES
				| OB_SNEEK,

	OB_CMD_HELP		= ~(option_t)0,

	OB_CMD_VERSION		= OB_SECTIONS
				| OB_LONG,

	OB_CMD_TEST		= ~(option_t)0,

	OB_CMD_ERROR		= OB_SECTIONS
				| OB_NO_HEADER
				| OB_LONG,

	OB_CMD_EXCLUDE		= OB_EXCLUDE
				| OB_EXCLUDE_PATH,

	OB_CMD_TITLES		= OB_GRP_TITLES,

	OB_CMD_FILELIST		= OB_GRP_XXSOURCE
				| OB_LONG,

	OB_CMD_FILETYPE		= OB_GRP_XXSOURCE
				| OB_NO_HEADER
				| OB_LONG,

	OB_CMD_ISOSIZE		= OB_GRP_XXSOURCE
				| OB_NO_HEADER
				| OB_LONG,

	OB_CMD_DUMP		= OB_GRP_TITLES
				| OB_GRP_XSOURCE
				| OB_IGNORE_FST
				| OB_GRP_PARTITIONS
				| OB_GRP_FILES
				| OB_ENC
				| OB_LONG,

	OB_CMD_ID6		= OB_GRP_XSOURCE
				| OB_IGNORE_FST
				| OB_UNIQUE
				| OB_SORT,

	OB_CMD_LIST		= OB_GRP_TITLES
				| OB_GRP_XSOURCE
				| OB_IGNORE_FST
				| OB_UNIQUE
				| OB_SORT
				| OB_SECTIONS
				| OB_NO_HEADER
				| OB_LONG
				| OB_GRP_TIME,

	OB_CMD_LIST_L		= OB_CMD_LIST,

	OB_CMD_LIST_LL		= OB_CMD_LIST_L,

	OB_CMD_LIST_LLL		= OB_CMD_LIST_LL,

	OB_CMD_ILIST		= OB_GRP_XXSOURCE
				| OB_GRP_PARTITIONS
				| OB_GRP_FILES
				| OB_LONG
				| OB_NO_HEADER
				| OB_SORT,

	OB_CMD_ILIST_L		= OB_CMD_ILIST,

	OB_CMD_ILIST_LL		= OB_CMD_ILIST_L,

	OB_CMD_DIFF		= OB_GRP_TITLES
				| OB_GRP_XXSOURCE
				| OB_GRP_PARTITIONS
				| OB_GRP_FILES
				| OB_LONG
				| OB_DEST
				| OB_DEST2
				| OB_WDF
				| OB_ISO
				| OB_CISO
				| OB_WBFS
				| OB_FST,

	OB_CMD_EXTRACT		= OB_GRP_TITLES
				| OB_GRP_XXSOURCE
				| OB_GRP_PARTITIONS
				| OB_GRP_FILES
				| OB_SORT
				| OB_ENC
				| OB_DEST
				| OB_DEST2
				| OB_PRESERVE
				| OB_OVERWRITE,

	OB_CMD_COPY		= OB_CMD_EXTRACT
				| OB_UPDATE
				| OB_REMOVE
				| OB_SPLIT
				| OB_SPLIT_SIZE
				| OB_WDF
				| OB_ISO
				| OB_CISO
				| OB_WBFS
				| OB_FST,

	OB_CMD_SCRUB		= OB_GRP_TITLES
				| OB_GRP_XXSOURCE
				| OB_GRP_PARTITIONS
				| OB_SPLIT
				| OB_SPLIT_SIZE
				| OB_PRESERVE
				| OB_WDF
				| OB_ISO
				| OB_CISO
				| OB_WBFS,

	OB_CMD_MOVE		= OB_GRP_TITLES
				| OB_GRP_XSOURCE
				| OB_IGNORE
				| OB_DEST
				| OB_DEST2
				| OB_OVERWRITE,

	OB_CMD_RENAME		= OB_GRP_TITLES
				| OB_GRP_XSOURCE
				| OB_IGNORE
				| OB_ISO
				| OB_WBFS,

	OB_CMD_SETTITLE		= OB_CMD_RENAME,

	OB_CMD_VERIFY		= OB_GRP_TITLES
				| OB_GRP_XXSOURCE
				| OB_GRP_PARTITIONS
				| OB_LIMIT
				| OB_LONG,

} enumOptionsBit;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                enum enumCommands                ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumCommands
{
	CMD__NONE,

	CMD_VERSION,
	CMD_HELP,
	CMD_TEST,
	CMD_ERROR,
	CMD_EXCLUDE,
	CMD_TITLES,

	CMD_FILELIST,
	CMD_FILETYPE,
	CMD_ISOSIZE,

	CMD_DUMP,
	CMD_ID6,
	CMD_LIST,
	CMD_LIST_L,
	CMD_LIST_LL,
	CMD_LIST_LLL,

	CMD_ILIST,
	CMD_ILIST_L,
	CMD_ILIST_LL,

	CMD_DIFF,
	CMD_EXTRACT,
	CMD_COPY,
	CMD_SCRUB,
	CMD_MOVE,
	CMD_RENAME,
	CMD_SETTITLE,

	CMD_VERIFY,

	CMD__N

} enumCommands;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   enumGetOpt                    ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumGetOpt
{
	GO_VERSION		= 'V',
	GO_HELP			= 'h',
	GO_XHELP		= 0x80,
	GO_QUIET		= 'q',
	GO_VERBOSE		= 'v',
	GO_PROGRESS		= 'P',
	GO_LOGGING		= 'L',
	GO_ESC			= 'E',
	GO_IO			= 0x81,
	GO_TITLES		= 'T',
	GO_UTF_8		= 0x82,
	GO_NO_UTF_8		= 0x83,
	GO_LANG			= 0x84,
	GO_TEST			= 't',
	GO_SOURCE		= 's',
	GO_RECURSE		= 'r',
	GO_RDEPTH		= 0x85,
	GO_INCLUDE		= 'n',
	GO_INCLUDE_PATH		= 'N',
	GO_EXCLUDE		= 'x',
	GO_EXCLUDE_PATH		= 'X',
	GO_IGNORE		= 'i',
	GO_IGNORE_FST		= 0x86,
	GO_PSEL			= 0x87,
	GO_RAW			= 0x88,
	GO_PMODE		= 0x89,
	GO_SNEEK		= 0x8a,
	GO_ENC			= 0x8b,
	GO_DEST			= 'd',
	GO_DEST2		= 'D',
	GO_SPLIT		= 'z',
	GO_SPLIT_SIZE		= 'Z',
	GO_PRESERVE		= 'p',
	GO_UPDATE		= 'u',
	GO_OVERWRITE		= 'o',
	GO_REMOVE		= 'R',
	GO_WDF			= 'W',
	GO_ISO			= 'I',
	GO_CISO			= 'C',
	GO_WBFS			= 'B',
	GO_FST			= 0x8c,
	GO_FILES		= 'F',
	GO_ITIME		= 0x8d,
	GO_MTIME		= 0x8e,
	GO_CTIME		= 0x8f,
	GO_ATIME		= 0x90,
	GO_TIME			= 0x91,
	GO_LONG			= 'l',
	GO_UNIQUE		= 'U',
	GO_NO_HEADER		= 'H',
	GO_SECTIONS		= 0x92,
	GO_SORT			= 'S',
	GO_LIMIT		= 0x93,

	GO__ERR			= '?'

} enumGetOpt;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  external vars                  ///////////////
///////////////////////////////////////////////////////////////////////////////

extern const InfoOption_t OptionInfo[OPT__N_TOTAL+1];
extern const CommandTab_t CommandTab[];
extern const char OptionShort[];
extern const struct option OptionLong[];
extern u8 OptionUsed[OPT__N_TOTAL+1];
extern const u8 OptionIndex[OPT_INDEX_SIZE];
extern const InfoCommand_t CommandInfo[CMD__N+1];
extern const InfoUI_t InfoUI;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                       END                       ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_UI_WIT_H

