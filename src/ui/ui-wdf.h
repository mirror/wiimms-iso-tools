
/***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project. It is automatically generated   *
 *   by the project helper 'gen-ui'.                                       *
 *                                                                         *
 *                      ==> DO NOT EDIT THIS FILE <==                      *
 *                                                                         *
 *   Visit http://wit.wiimm.de/ for project details and sources.           *
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


#ifndef WIT_UI_WDF_H
#define WIT_UI_WDF_H
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

	OPT_CHUNK,
	OPT_LONG,
	OPT_MINUS1,
	OPT_WDF,
	OPT_CISO,
	OPT_WBI,
	OPT_SUFFIX,
	OPT_DEST,
	OPT_KEEP,
	OPT_OVERWRITE,
	OPT_PRESERVE,
	OPT_SPLIT,
	OPT_SPLIT_SIZE,
	OPT_CHUNK_MODE,
	OPT_CHUNK_SIZE,
	OPT_MAX_CHUNKS,

	OPT__N_SPECIFIC, // == 17 

	//----- global options -----

	OPT_VERSION,
	OPT_HELP,
	OPT_XHELP,
	OPT_WIDTH,
	OPT_QUIET,
	OPT_VERBOSE,
	OPT_DEST2,
	OPT_STDOUT,
	OPT_TEST,

	OPT__N_TOTAL // == 26

} enumOptions;

//
///////////////////////////////////////////////////////////////////////////////
///////////////               enum enumOptionsBit               ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumOptionsBit
{
	//----- command specific options -----

	OB_CHUNK		= 1llu << OPT_CHUNK,
	OB_LONG			= 1llu << OPT_LONG,
	OB_MINUS1		= 1llu << OPT_MINUS1,
	OB_WDF			= 1llu << OPT_WDF,
	OB_CISO			= 1llu << OPT_CISO,
	OB_WBI			= 1llu << OPT_WBI,
	OB_SUFFIX		= 1llu << OPT_SUFFIX,
	OB_DEST			= 1llu << OPT_DEST,
	OB_KEEP			= 1llu << OPT_KEEP,
	OB_OVERWRITE		= 1llu << OPT_OVERWRITE,
	OB_PRESERVE		= 1llu << OPT_PRESERVE,
	OB_SPLIT		= 1llu << OPT_SPLIT,
	OB_SPLIT_SIZE		= 1llu << OPT_SPLIT_SIZE,
	OB_CHUNK_MODE		= 1llu << OPT_CHUNK_MODE,
	OB_CHUNK_SIZE		= 1llu << OPT_CHUNK_SIZE,
	OB_MAX_CHUNKS		= 1llu << OPT_MAX_CHUNKS,

	//----- group & command options -----

	OB_GRP_BASE		= 0,

	OB_GRP_DEST		= OB_DEST
				| OB_OVERWRITE,

	OB_GRP_SPLIT_DEST	= OB_GRP_DEST
				| OB_SPLIT
				| OB_SPLIT_SIZE,

	OB_GRP_CHUNK		= OB_CHUNK_MODE
				| OB_CHUNK_SIZE
				| OB_MAX_CHUNKS,

	OB_GRP_CHUNK_DEST	= OB_GRP_SPLIT_DEST
				| OB_GRP_CHUNK,

	OB_GRP_FILETYPE		= OB_WDF
				| OB_CISO
				| OB_WBI
				| OB_SUFFIX,

	OB_CMD_VERSION		= OB_LONG,

	OB_CMD_HELP		= ~(option_t)0,

	OB_CMD_UNPACK		= OB_GRP_SPLIT_DEST
				| OB_KEEP
				| OB_PRESERVE,

	OB_CMD_PACK		= OB_CMD_UNPACK
				| OB_GRP_FILETYPE,

	OB_CMD_CAT		= OB_GRP_SPLIT_DEST,

	OB_CMD_CMP		= 0,

	OB_CMD_DUMP		= OB_GRP_DEST
				| OB_CHUNK
				| OB_LONG
				| OB_MINUS1,

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

	CMD_PACK,
	CMD_UNPACK,
	CMD_CAT,
	CMD_CMP,
	CMD_DUMP,

	CMD__N

} enumCommands;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   enumGetOpt                    ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumGetOpt
{
	GO_MINUS1		= '1',

	GO__ERR			= '?',

	GO_CISO			= 'C',
	GO_DEST2		= 'D',
	GO_VERSION		= 'V',
	GO_WDF			= 'W',
	GO_SPLIT_SIZE		= 'Z',

	GO_STDOUT		= 'c',
	GO_DEST			= 'd',
	GO_HELP			= 'h',
	GO_KEEP			= 'k',
	GO_LONG			= 'l',
	GO_OVERWRITE		= 'o',
	GO_PRESERVE		= 'p',
	GO_QUIET		= 'q',
	GO_SUFFIX		= 's',
	GO_TEST			= 't',
	GO_VERBOSE		= 'v',
	GO_SPLIT		= 'z',

	GO_XHELP		= 0x80,
	GO_WIDTH,
	GO_CHUNK,
	GO_WBI,
	GO_CHUNK_MODE,
	GO_CHUNK_SIZE,
	GO_MAX_CHUNKS,

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

#endif // WIT_UI_WDF_H

