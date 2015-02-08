
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
 *   Copyright (c) 2009-2015 by Dirk Clemens <wiimm@wiimm.de>              *
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
 ***************************************************************************
 *                                                                         *
 *   >>>  This file is automatically generated by './src/gen-ui.c'.  <<<   *
 *   >>>                   Do not edit this file!                    <<<   *
 *                                                                         *
 ***************************************************************************/


#ifndef WIT_UI_WIT_H
#define WIT_UI_WIT_H
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
	OPT_NO_EXPAND,
	OPT_RECURSE,
	OPT_RDEPTH,
	OPT_AUTO,
	OPT_EXCLUDE,
	OPT_EXCLUDE_PATH,
	OPT_INCLUDE,
	OPT_INCLUDE_PATH,
	OPT_INCLUDE_FIRST,
	OPT_ONE_JOB,
	OPT_JOB_LIMIT,
	OPT_FAKE_SIGN,
	OPT_IGNORE,
	OPT_IGNORE_FST,
	OPT_IGNORE_SETUP,
	OPT_LINKS,
	OPT_NULL,
	OPT_PSEL,
	OPT_RAW,
	OPT_PMODE,
	OPT_FLAT,
	OPT_COPY_GC,
	OPT_NO_LINK,
	OPT_NEEK,
	OPT_ENC,
	OPT_MODIFY,
	OPT_NAME,
	OPT_ID,
	OPT_DISC_ID,
	OPT_BOOT_ID,
	OPT_TICKET_ID,
	OPT_TMD_ID,
	OPT_TT_ID,
	OPT_WBFS_ID,
	OPT_REGION,
	OPT_COMMON_KEY,
	OPT_IOS,
	OPT_HTTP,
	OPT_DOMAIN,
	OPT_WIIMMFI,
	OPT_TWIIMMFI,
	OPT_RM_FILES,
	OPT_ZERO_FILES,
	OPT_OVERLAY,
	OPT_REPL_FILE,
	OPT_ADD_FILE,
	OPT_IGNORE_FILES,
	OPT_TRIM,
	OPT_ALIGN,
	OPT_ALIGN_PART,
	OPT_ALIGN_FILES,
	OPT_DEST,
	OPT_DEST2,
	OPT_AUTO_SPLIT,
	OPT_NO_SPLIT,
	OPT_SPLIT,
	OPT_SPLIT_SIZE,
	OPT_DISC_SIZE,
	OPT_PREALLOC,
	OPT_TRUNC,
	OPT_CHUNK_MODE,
	OPT_CHUNK_SIZE,
	OPT_MAX_CHUNKS,
	OPT_BLOCK_SIZE,
	OPT_COMPRESSION,
	OPT_MEM,
	OPT_PRESERVE,
	OPT_UPDATE,
	OPT_OVERWRITE,
	OPT_DIFF,
	OPT_REMOVE,
	OPT_WDF,
	OPT_WDF1,
	OPT_WDF2,
	OPT_ISO,
	OPT_CISO,
	OPT_WBFS,
	OPT_WIA,
	OPT_GCZ,
	OPT_FST,
	OPT_FILES,
	OPT_ITIME,
	OPT_MTIME,
	OPT_CTIME,
	OPT_ATIME,
	OPT_TIME,
	OPT_LONG,
	OPT_BRIEF,
	OPT_NUMERIC,
	OPT_TECHNICAL,
	OPT_REALPATH,
	OPT_SHOW,
	OPT_UNIT,
	OPT_UNIQUE,
	OPT_NO_HEADER,
	OPT_OLD_STYLE,
	OPT_SECTIONS,
	OPT_SORT,
	OPT_LIMIT,
	OPT_FILE_LIMIT,
	OPT_PATCH_FILE,

	OPT__N_SPECIFIC, // == 103 

	//----- global options -----

	OPT_VERSION,
	OPT_HELP,
	OPT_XHELP,
	OPT_WIDTH,
	OPT_QUIET,
	OPT_VERBOSE,
	OPT_PROGRESS,
	OPT_SCAN_PROGRESS,
	OPT_LOGGING,
	OPT_ESC,
	OPT_IO,
	OPT_FORCE,
	OPT_DIRECT,
	OPT_TITLES,
	OPT_UTF_8,
	OPT_NO_UTF_8,
	OPT_LANG,
	OPT_CERT,
	OPT_TEST,
	OPT_OLD,
	OPT_NEW,
	OPT_HOOK,
	OPT_ALIGN_WDF,
	OPT_GCZ_ZIP,
	OPT_GCZ_BLOCK,

	OPT__N_TOTAL // == 128

} enumOptions;

//
///////////////////////////////////////////////////////////////////////////////
///////////////               enum enumOptionsBit               ///////////////
///////////////////////////////////////////////////////////////////////////////

//	*****  only for verification  *****

//typedef enum enumOptionsBit
//{
//	//----- command specific options -----
//
//	OB_SOURCE		= 1llu << OPT_SOURCE,
//	OB_NO_EXPAND		= 1llu << OPT_NO_EXPAND,
//	OB_RECURSE		= 1llu << OPT_RECURSE,
//	OB_RDEPTH		= 1llu << OPT_RDEPTH,
//	OB_AUTO			= 1llu << OPT_AUTO,
//	OB_EXCLUDE		= 1llu << OPT_EXCLUDE,
//	OB_EXCLUDE_PATH		= 1llu << OPT_EXCLUDE_PATH,
//	OB_INCLUDE		= 1llu << OPT_INCLUDE,
//	OB_INCLUDE_PATH		= 1llu << OPT_INCLUDE_PATH,
//	OB_INCLUDE_FIRST	= 1llu << OPT_INCLUDE_FIRST,
//	OB_ONE_JOB		= 1llu << OPT_ONE_JOB,
//	OB_JOB_LIMIT		= 1llu << OPT_JOB_LIMIT,
//	OB_FAKE_SIGN		= 1llu << OPT_FAKE_SIGN,
//	OB_IGNORE		= 1llu << OPT_IGNORE,
//	OB_IGNORE_FST		= 1llu << OPT_IGNORE_FST,
//	OB_IGNORE_SETUP		= 1llu << OPT_IGNORE_SETUP,
//	OB_LINKS		= 1llu << OPT_LINKS,
//	OB_NULL			= 1llu << OPT_NULL,
//	OB_PSEL			= 1llu << OPT_PSEL,
//	OB_RAW			= 1llu << OPT_RAW,
//	OB_PMODE		= 1llu << OPT_PMODE,
//	OB_FLAT			= 1llu << OPT_FLAT,
//	OB_COPY_GC		= 1llu << OPT_COPY_GC,
//	OB_NO_LINK		= 1llu << OPT_NO_LINK,
//	OB_NEEK			= 1llu << OPT_NEEK,
//	OB_ENC			= 1llu << OPT_ENC,
//	OB_MODIFY		= 1llu << OPT_MODIFY,
//	OB_NAME			= 1llu << OPT_NAME,
//	OB_ID			= 1llu << OPT_ID,
//	OB_DISC_ID		= 1llu << OPT_DISC_ID,
//	OB_BOOT_ID		= 1llu << OPT_BOOT_ID,
//	OB_TICKET_ID		= 1llu << OPT_TICKET_ID,
//	OB_TMD_ID		= 1llu << OPT_TMD_ID,
//	OB_TT_ID		= 1llu << OPT_TT_ID,
//	OB_WBFS_ID		= 1llu << OPT_WBFS_ID,
//	OB_REGION		= 1llu << OPT_REGION,
//	OB_COMMON_KEY		= 1llu << OPT_COMMON_KEY,
//	OB_IOS			= 1llu << OPT_IOS,
//	OB_HTTP			= 1llu << OPT_HTTP,
//	OB_DOMAIN		= 1llu << OPT_DOMAIN,
//	OB_WIIMMFI		= 1llu << OPT_WIIMMFI,
//	OB_TWIIMMFI		= 1llu << OPT_TWIIMMFI,
//	OB_RM_FILES		= 1llu << OPT_RM_FILES,
//	OB_ZERO_FILES		= 1llu << OPT_ZERO_FILES,
//	OB_OVERLAY		= 1llu << OPT_OVERLAY,
//	OB_REPL_FILE		= 1llu << OPT_REPL_FILE,
//	OB_ADD_FILE		= 1llu << OPT_ADD_FILE,
//	OB_IGNORE_FILES		= 1llu << OPT_IGNORE_FILES,
//	OB_TRIM			= 1llu << OPT_TRIM,
//	OB_ALIGN		= 1llu << OPT_ALIGN,
//	OB_ALIGN_PART		= 1llu << OPT_ALIGN_PART,
//	OB_ALIGN_FILES		= 1llu << OPT_ALIGN_FILES,
//	OB_DEST			= 1llu << OPT_DEST,
//	OB_DEST2		= 1llu << OPT_DEST2,
//	OB_AUTO_SPLIT		= 1llu << OPT_AUTO_SPLIT,
//	OB_NO_SPLIT		= 1llu << OPT_NO_SPLIT,
//	OB_SPLIT		= 1llu << OPT_SPLIT,
//	OB_SPLIT_SIZE		= 1llu << OPT_SPLIT_SIZE,
//	OB_DISC_SIZE		= 1llu << OPT_DISC_SIZE,
//	OB_PREALLOC		= 1llu << OPT_PREALLOC,
//	OB_TRUNC		= 1llu << OPT_TRUNC,
//	OB_CHUNK_MODE		= 1llu << OPT_CHUNK_MODE,
//	OB_CHUNK_SIZE		= 1llu << OPT_CHUNK_SIZE,
//	OB_MAX_CHUNKS		= 1llu << OPT_MAX_CHUNKS,
//	OB_BLOCK_SIZE		= 1llu << OPT_BLOCK_SIZE,
//	OB_COMPRESSION		= 1llu << OPT_COMPRESSION,
//	OB_MEM			= 1llu << OPT_MEM,
//	OB_PRESERVE		= 1llu << OPT_PRESERVE,
//	OB_UPDATE		= 1llu << OPT_UPDATE,
//	OB_OVERWRITE		= 1llu << OPT_OVERWRITE,
//	OB_DIFF			= 1llu << OPT_DIFF,
//	OB_REMOVE		= 1llu << OPT_REMOVE,
//	OB_WDF			= 1llu << OPT_WDF,
//	OB_WDF1			= 1llu << OPT_WDF1,
//	OB_WDF2			= 1llu << OPT_WDF2,
//	OB_ISO			= 1llu << OPT_ISO,
//	OB_CISO			= 1llu << OPT_CISO,
//	OB_WBFS			= 1llu << OPT_WBFS,
//	OB_WIA			= 1llu << OPT_WIA,
//	OB_GCZ			= 1llu << OPT_GCZ,
//	OB_FST			= 1llu << OPT_FST,
//	OB_FILES		= 1llu << OPT_FILES,
//	OB_ITIME		= 1llu << OPT_ITIME,
//	OB_MTIME		= 1llu << OPT_MTIME,
//	OB_CTIME		= 1llu << OPT_CTIME,
//	OB_ATIME		= 1llu << OPT_ATIME,
//	OB_TIME			= 1llu << OPT_TIME,
//	OB_LONG			= 1llu << OPT_LONG,
//	OB_BRIEF		= 1llu << OPT_BRIEF,
//	OB_NUMERIC		= 1llu << OPT_NUMERIC,
//	OB_TECHNICAL		= 1llu << OPT_TECHNICAL,
//	OB_REALPATH		= 1llu << OPT_REALPATH,
//	OB_SHOW			= 1llu << OPT_SHOW,
//	OB_UNIT			= 1llu << OPT_UNIT,
//	OB_UNIQUE		= 1llu << OPT_UNIQUE,
//	OB_NO_HEADER		= 1llu << OPT_NO_HEADER,
//	OB_OLD_STYLE		= 1llu << OPT_OLD_STYLE,
//	OB_SECTIONS		= 1llu << OPT_SECTIONS,
//	OB_SORT			= 1llu << OPT_SORT,
//	OB_LIMIT		= 1llu << OPT_LIMIT,
//	OB_FILE_LIMIT		= 1llu << OPT_FILE_LIMIT,
//	OB_PATCH_FILE		= 1llu << OPT_PATCH_FILE,
//
//	//----- group & command options -----
//
//	OB_GRP_TITLES		= 0,
//
//	OB_GRP_FST_OPTIONS	= OB_IGNORE_FST
//				| OB_IGNORE_SETUP
//				| OB_LINKS,
//
//	OB_GRP_FST_SELECT	= OB_PMODE
//				| OB_FLAT
//				| OB_FILES
//				| OB_COPY_GC
//				| OB_NO_LINK
//				| OB_NEEK,
//
//	OB_GRP_SOURCE		= OB_SOURCE
//				| OB_NO_EXPAND
//				| OB_RECURSE
//				| OB_RDEPTH,
//
//	OB_GRP_EXCLUDE		= OB_EXCLUDE
//				| OB_EXCLUDE_PATH
//				| OB_INCLUDE
//				| OB_INCLUDE_PATH
//				| OB_INCLUDE_FIRST
//				| OB_ONE_JOB
//				| OB_JOB_LIMIT,
//
//	OB_GRP_XSOURCE		= OB_GRP_SOURCE
//				| OB_GRP_EXCLUDE,
//
//	OB_GRP_XXSOURCE		= OB_GRP_SOURCE
//				| OB_GRP_EXCLUDE
//				| OB_IGNORE
//				| OB_GRP_FST_OPTIONS,
//
//	OB_GRP_OUTMODE_EDIT	= OB_WDF
//				| OB_WDF1
//				| OB_WDF2
//				| OB_ISO
//				| OB_CISO
//				| OB_WBFS,
//
//	OB_GRP_OUTMODE		= OB_GRP_OUTMODE_EDIT
//				| OB_WIA
//				| OB_GCZ,
//
//	OB_GRP_OUTMODE_FST	= OB_GRP_OUTMODE
//				| OB_FST,
//
//	OB_GRP_XTIME		= OB_ITIME
//				| OB_MTIME
//				| OB_CTIME
//				| OB_ATIME,
//
//	OB_GRP_TIME		= OB_GRP_XTIME
//				| OB_TIME,
//
//	OB_GRP_PARTITIONS	= OB_PSEL
//				| OB_RAW,
//
//	OB_GRP_PATCH_ID		= OB_MODIFY
//				| OB_ID
//				| OB_DISC_ID
//				| OB_BOOT_ID
//				| OB_TICKET_ID
//				| OB_TMD_ID
//				| OB_TT_ID
//				| OB_WBFS_ID,
//
//	OB_GRP_PATCH		= OB_ENC
//				| OB_MODIFY
//				| OB_NAME
//				| OB_ID
//				| OB_DISC_ID
//				| OB_BOOT_ID
//				| OB_TICKET_ID
//				| OB_TMD_ID
//				| OB_TT_ID
//				| OB_WBFS_ID
//				| OB_REGION
//				| OB_COMMON_KEY
//				| OB_IOS
//				| OB_HTTP
//				| OB_DOMAIN
//				| OB_WIIMMFI
//				| OB_TWIIMMFI
//				| OB_RM_FILES
//				| OB_ZERO_FILES,
//
//	OB_GRP_RELOCATE		= OB_REPL_FILE
//				| OB_ADD_FILE
//				| OB_IGNORE_FILES
//				| OB_TRIM
//				| OB_ALIGN
//				| OB_ALIGN_PART
//				| OB_ALIGN_FILES,
//
//	OB_GRP_SPLIT_CHUNK	= OB_AUTO_SPLIT
//				| OB_NO_SPLIT
//				| OB_SPLIT
//				| OB_SPLIT_SIZE
//				| OB_DISC_SIZE
//				| OB_PREALLOC
//				| OB_TRUNC
//				| OB_CHUNK_MODE
//				| OB_CHUNK_SIZE
//				| OB_MAX_CHUNKS
//				| OB_COMPRESSION
//				| OB_MEM,
//
//	OB_CMD_VERSION		= OB_BRIEF
//				| OB_SECTIONS
//				| OB_LONG,
//
//	OB_CMD_HELP		= ~(u64)0,
//
//	OB_CMD_INFO		= OB_SECTIONS
//				| OB_LONG,
//
//	OB_CMD_TEST		= ~(u64)0,
//
//	OB_CMD_ERROR		= OB_SECTIONS
//				| OB_NO_HEADER
//				| OB_LONG,
//
//	OB_CMD_COMPR		= OB_MEM
//				| OB_SECTIONS
//				| OB_NO_HEADER
//				| OB_LONG
//				| OB_NUMERIC,
//
//	OB_CMD_FEATURES		= 0,
//
//	OB_CMD_ANAID		= OB_NO_HEADER,
//
//	OB_CMD_EXCLUDE		= OB_EXCLUDE
//				| OB_EXCLUDE_PATH,
//
//	OB_CMD_TITLES		= OB_GRP_TITLES,
//
//	OB_CMD_GETTITLES	= 0,
//
//	OB_CMD_CERT		= OB_FILES
//				| OB_FAKE_SIGN
//				| OB_DEST
//				| OB_DEST2
//				| OB_LONG,
//
//	OB_CMD_FILELIST		= OB_AUTO
//				| OB_GRP_XXSOURCE
//				| OB_LONG,
//
//	OB_CMD_FILETYPE		= OB_AUTO
//				| OB_GRP_XXSOURCE
//				| OB_NO_HEADER
//				| OB_LONG,
//
//	OB_CMD_ISOSIZE		= OB_AUTO
//				| OB_GRP_XXSOURCE
//				| OB_GRP_PARTITIONS
//				| OB_NO_HEADER
//				| OB_LONG
//				| OB_UNIT,
//
//	OB_CMD_CREATE		= OB_DEST
//				| OB_DEST2
//				| OB_OVERWRITE
//				| OB_ID
//				| OB_IOS,
//
//	OB_CMD_DOLPATCH		= OB_LONG
//				| OB_SOURCE
//				| OB_DEST
//				| OB_DEST2
//				| OB_OVERWRITE,
//
//	OB_CMD_CODE		= 0,
//
//	OB_CMD_DUMP		= OB_GRP_TITLES
//				| OB_AUTO
//				| OB_GRP_XSOURCE
//				| OB_GRP_FST_OPTIONS
//				| OB_GRP_PARTITIONS
//				| OB_GRP_FST_SELECT
//				| OB_LONG
//				| OB_SHOW
//				| OB_GRP_PATCH
//				| OB_GRP_RELOCATE
//				| OB_DISC_SIZE,
//
//	OB_CMD_ID6		= OB_AUTO
//				| OB_GRP_XSOURCE
//				| OB_GRP_FST_OPTIONS
//				| OB_LONG
//				| OB_GRP_PATCH_ID,
//
//	OB_CMD_FRAGMENTS	= OB_AUTO
//				| OB_GRP_XSOURCE
//				| OB_GRP_FST_OPTIONS
//				| OB_LONG
//				| OB_BRIEF,
//
//	OB_CMD_LIST		= OB_GRP_TITLES
//				| OB_AUTO
//				| OB_GRP_XSOURCE
//				| OB_GRP_FST_OPTIONS
//				| OB_UNIQUE
//				| OB_SORT
//				| OB_SECTIONS
//				| OB_NO_HEADER
//				| OB_LONG
//				| OB_REALPATH
//				| OB_UNIT
//				| OB_GRP_TIME,
//
//	OB_CMD_LIST_L		= OB_CMD_LIST,
//
//	OB_CMD_LIST_LL		= OB_CMD_LIST_L,
//
//	OB_CMD_LIST_LLL		= OB_CMD_LIST_LL,
//
//	OB_CMD_FILES		= OB_AUTO
//				| OB_GRP_XXSOURCE
//				| OB_GRP_PARTITIONS
//				| OB_GRP_FST_SELECT
//				| OB_GRP_RELOCATE
//				| OB_LONG
//				| OB_NO_HEADER
//				| OB_SHOW
//				| OB_SORT,
//
//	OB_CMD_FILES_L		= OB_CMD_FILES,
//
//	OB_CMD_FILES_LL		= OB_CMD_FILES,
//
//	OB_CMD_DIFF		= OB_FILES
//				| OB_PATCH_FILE
//				| OB_GRP_TITLES
//				| OB_AUTO
//				| OB_GRP_XXSOURCE
//				| OB_GRP_PARTITIONS
//				| OB_GRP_FST_SELECT
//				| OB_FILE_LIMIT
//				| OB_LIMIT
//				| OB_LONG
//				| OB_BLOCK_SIZE
//				| OB_SECTIONS
//				| OB_DEST
//				| OB_DEST2
//				| OB_GRP_OUTMODE_FST,
//
//	OB_CMD_FDIFF		= OB_CMD_DIFF,
//
//	OB_CMD_EXTRACT		= OB_GRP_TITLES
//				| OB_AUTO
//				| OB_GRP_XXSOURCE
//				| OB_GRP_PARTITIONS
//				| OB_GRP_FST_SELECT
//				| OB_PREALLOC
//				| OB_SORT
//				| OB_LONG
//				| OB_SECTIONS
//				| OB_GRP_PATCH
//				| OB_GRP_RELOCATE
//				| OB_DEST
//				| OB_DEST2
//				| OB_PRESERVE
//				| OB_OVERWRITE,
//
//	OB_CMD_COPY		= OB_CMD_EXTRACT
//				| OB_UPDATE
//				| OB_DIFF
//				| OB_REMOVE
//				| OB_GRP_SPLIT_CHUNK
//				| OB_GRP_OUTMODE_FST,
//
//	OB_CMD_CONVERT		= OB_GRP_TITLES
//				| OB_GRP_XXSOURCE
//				| OB_GRP_PARTITIONS
//				| OB_SECTIONS
//				| OB_GRP_PATCH
//				| OB_GRP_RELOCATE
//				| OB_GRP_SPLIT_CHUNK
//				| OB_PRESERVE
//				| OB_GRP_OUTMODE,
//
//	OB_CMD_EDIT		= OB_GRP_TITLES
//				| OB_GRP_XSOURCE
//				| OB_IGNORE
//				| OB_SECTIONS
//				| OB_PRESERVE
//				| OB_GRP_PARTITIONS
//				| OB_GRP_PATCH
//				| OB_WDF1
//				| OB_WDF2,
//
//	OB_CMD_IMGFILES		= OB_GRP_TITLES
//				| OB_GRP_XSOURCE
//				| OB_IGNORE
//				| OB_SECTIONS
//				| OB_NULL,
//
//	OB_CMD_REMOVE		= OB_GRP_TITLES
//				| OB_GRP_XSOURCE
//				| OB_IGNORE
//				| OB_SECTIONS,
//
//	OB_CMD_MOVE		= OB_GRP_TITLES
//				| OB_GRP_XSOURCE
//				| OB_IGNORE
//				| OB_SECTIONS
//				| OB_DEST
//				| OB_DEST2
//				| OB_OVERWRITE,
//
//	OB_CMD_RENAME		= OB_GRP_TITLES
//				| OB_AUTO
//				| OB_GRP_XSOURCE
//				| OB_IGNORE
//				| OB_ISO
//				| OB_WBFS,
//
//	OB_CMD_SETTITLE		= OB_CMD_RENAME,
//
//	OB_CMD_VERIFY		= OB_GRP_TITLES
//				| OB_AUTO
//				| OB_GRP_XXSOURCE
//				| OB_GRP_PARTITIONS
//				| OB_IGNORE_FILES
//				| OB_LIMIT
//				| OB_LONG
//				| OB_TECHNICAL,
//
//	OB_CMD_SKELETON		= OB_GRP_TITLES
//				| OB_AUTO
//				| OB_GRP_XXSOURCE
//				| OB_GRP_PARTITIONS
//				| OB_GRP_OUTMODE_EDIT
//				| OB_DEST
//				| OB_DEST2,
//
//	OB_CMD_MIX		= OB_DEST
//				| OB_DEST2
//				| OB_OVERWRITE
//				| OB_GRP_SPLIT_CHUNK
//				| OB_ALIGN_PART
//				| OB_GRP_OUTMODE_EDIT
//				| OB_ID
//				| OB_NAME
//				| OB_REGION
//				| OB_OVERLAY,
//
//} enumOptionsBit;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                enum enumCommands                ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumCommands
{
	CMD__NONE,

	CMD_VERSION,
	CMD_HELP,
	CMD_INFO,
	CMD_TEST,
	CMD_ERROR,
	CMD_COMPR,
	CMD_FEATURES,
	CMD_ANAID,
	CMD_EXCLUDE,
	CMD_TITLES,
	CMD_GETTITLES,
	CMD_CERT,
	CMD_CREATE,
	CMD_DOLPATCH,
	CMD_CODE,

	CMD_FILELIST,
	CMD_FILETYPE,
	CMD_ISOSIZE,

	CMD_DUMP,
	CMD_ID6,
	CMD_FRAGMENTS,
	CMD_LIST,
	CMD_LIST_L,
	CMD_LIST_LL,
	CMD_LIST_LLL,

	CMD_FILES,
	CMD_FILES_L,
	CMD_FILES_LL,

	CMD_DIFF,
	CMD_FDIFF,
	CMD_EXTRACT,
	CMD_COPY,
	CMD_CONVERT,
	CMD_EDIT,
	CMD_IMGFILES,
	CMD_REMOVE,
	CMD_MOVE,
	CMD_RENAME,
	CMD_SETTITLE,

	CMD_VERIFY,
	CMD_SKELETON,
	CMD_MIX,

	CMD__N // == 43

} enumCommands;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   enumGetOpt                    ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumGetOpt
{
	GO_SHOW			= '+',

	GO_NULL			= '0',
	GO_ONE_JOB		= '1',

	GO__ERR			= '?',

	GO_WBFS			= 'B',
	GO_CISO			= 'C',
	GO_DEST2		= 'D',
	GO_ESC			= 'E',
	GO_FILES		= 'F',
	GO_GCZ			= 'G',
	GO_NO_HEADER		= 'H',
	GO_ISO			= 'I',
	GO_LOGGING		= 'L',
	GO_INCLUDE_PATH		= 'N',
	GO_PROGRESS		= 'P',
	GO_REMOVE		= 'R',
	GO_SORT			= 'S',
	GO_TITLES		= 'T',
	GO_UNIQUE		= 'U',
	GO_VERSION		= 'V',
	GO_WDF			= 'W',
	GO_EXCLUDE_PATH		= 'X',
	GO_SPLIT_SIZE		= 'Z',

	GO_AUTO			= 'a',
	GO_BRIEF		= 'b',
	GO_DEST			= 'd',
	GO_FORCE		= 'f',
	GO_HELP			= 'h',
	GO_IGNORE		= 'i',
	GO_LONG			= 'l',
	GO_INCLUDE		= 'n',
	GO_OVERWRITE		= 'o',
	GO_PRESERVE		= 'p',
	GO_QUIET		= 'q',
	GO_RECURSE		= 'r',
	GO_SOURCE		= 's',
	GO_TEST			= 't',
	GO_UPDATE		= 'u',
	GO_VERBOSE		= 'v',
	GO_EXCLUDE		= 'x',
	GO_SPLIT		= 'z',

	GO_XHELP		= 0x80,
	GO_WIDTH,
	GO_SCAN_PROGRESS,
	GO_IO,
	GO_DIRECT,
	GO_UTF_8,
	GO_NO_UTF_8,
	GO_LANG,
	GO_CERT,
	GO_OLD,
	GO_NEW,
	GO_NO_EXPAND,
	GO_RDEPTH,
	GO_INCLUDE_FIRST,
	GO_JOB_LIMIT,
	GO_FAKE_SIGN,
	GO_IGNORE_FST,
	GO_IGNORE_SETUP,
	GO_LINKS,
	GO_PSEL,
	GO_RAW,
	GO_PMODE,
	GO_FLAT,
	GO_COPY_GC,
	GO_NO_LINK,
	GO_NEEK,
	GO_HOOK,
	GO_ENC,
	GO_MODIFY,
	GO_NAME,
	GO_ID,
	GO_DISC_ID,
	GO_BOOT_ID,
	GO_TICKET_ID,
	GO_TMD_ID,
	GO_TT_ID,
	GO_WBFS_ID,
	GO_REGION,
	GO_COMMON_KEY,
	GO_IOS,
	GO_HTTP,
	GO_DOMAIN,
	GO_WIIMMFI,
	GO_TWIIMMFI,
	GO_RM_FILES,
	GO_ZERO_FILES,
	GO_OVERLAY,
	GO_REPL_FILE,
	GO_ADD_FILE,
	GO_IGNORE_FILES,
	GO_TRIM,
	GO_ALIGN,
	GO_ALIGN_PART,
	GO_ALIGN_FILES,
	GO_AUTO_SPLIT,
	GO_NO_SPLIT,
	GO_DISC_SIZE,
	GO_PREALLOC,
	GO_TRUNC,
	GO_CHUNK_MODE,
	GO_CHUNK_SIZE,
	GO_MAX_CHUNKS,
	GO_BLOCK_SIZE,
	GO_COMPRESSION,
	GO_MEM,
	GO_DIFF,
	GO_WDF1,
	GO_WDF2,
	GO_ALIGN_WDF,
	GO_WIA,
	GO_GCZ_ZIP,
	GO_GCZ_BLOCK,
	GO_FST,
	GO_ITIME,
	GO_MTIME,
	GO_CTIME,
	GO_ATIME,
	GO_TIME,
	GO_NUMERIC,
	GO_TECHNICAL,
	GO_REALPATH,
	GO_UNIT,
	GO_OLD_STYLE,
	GO_SECTIONS,
	GO_LIMIT,
	GO_FILE_LIMIT,
	GO_PATCH_FILE,

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

