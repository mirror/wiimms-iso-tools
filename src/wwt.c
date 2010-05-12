/***************************************************************************
 *                                                                         *
 *   Copyright (c) 2009-2010 by Dirk Clemens <develop@cle-mens.de>         *
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

#define _GNU_SOURCE 1

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include "debug.h"
#include "version.h"
#include "wiidisc.h"
#include "lib-sf.h"
#include "titles.h"
#include "wbfs-interface.h"

///////////////////////////////////////////////////////////////////////////////

#define TITLE WWT_SHORT ": " WWT_LONG " v" VERSION " r" REVISION " " SYSTEM " - " AUTHOR " - " DATE

///////////////////////////////////////////////////////////////////////////////

typedef enum enumOptions
{
	// only the command specific options are named

	OPT_AUTO,
	OPT_ALL,
	OPT_PART,
	OPT_RECURSE,
	OPT_PSEL,
	OPT_ENC,
	OPT_RAW,
	OPT_DEST,
	OPT_DEST2,
	OPT_SIZE,
	OPT_SPLIT,
	OPT_SPLIT_SIZE,
	OPT_HSS,
	OPT_WSS,
	OPT_RECOVER,
	OPT_FORCE,
	OPT_NO_CHECK,
	OPT_REPAIR,
	OPT_NO_FREE,
	OPT_UPDATE,
	OPT_SYNC,
	OPT_NEWER,
	OPT_OVERWRITE,
	OPT_IGNORE,
	OPT_REMOVE,
	OPT_TRUNC,
	OPT_FAST,
	OPT_WDF,
	OPT_ISO,
	OPT_CISO,
	OPT_WBFS,
	OPT_LONG,
	OPT_INODE,
	OPT_ITIME,
	OPT_MTIME,
	OPT_CTIME,
	OPT_ATIME,
	OPT_TIME,
	OPT_SET_TIME,
	OPT_MIXED,
	OPT_UNIQUE,
	OPT_NO_HEADER,
	OPT_SECTIONS,
	OPT_SORT,

	OPT__N_SPECIFIC,

} enumOptions;

///////////////////////////////////////////////////////////////////////////////

typedef enum enumOptionsBit
{
	// bitmask fo all command specific options

	OB_AUTO		= 1llu << OPT_AUTO,
	OB_ALL		= 1llu << OPT_ALL,
	OB_PART		= 1llu << OPT_PART,
	OB_RECURSE	= 1llu << OPT_RECURSE,
	OB_PSEL		= 1llu << OPT_PSEL,
	OB_RAW		= 1llu << OPT_RAW,
	OB_ENC		= 1llu << OPT_ENC,
	OB_DEST		= 1llu << OPT_DEST,
	OB_DEST2	= 1llu << OPT_DEST2,
	OB_SIZE		= 1llu << OPT_SIZE,
	OB_SPLIT	= 1llu << OPT_SPLIT,
	OB_SPLIT_SIZE	= 1llu << OPT_SPLIT_SIZE,
	OB_HSS		= 1llu << OPT_HSS,
	OB_WSS		= 1llu << OPT_WSS,
	OB_RECOVER	= 1llu << OPT_RECOVER,
	OB_FORCE	= 1llu << OPT_FORCE,
	OB_NO_CHECK	= 1llu << OPT_NO_CHECK,
	OB_REPAIR	= 1llu << OPT_REPAIR,
	OB_NO_FREE	= 1llu << OPT_NO_FREE,
	OB_UPDATE	= 1llu << OPT_UPDATE,
	OB_SYNC		= 1llu << OPT_SYNC,
	OB_NEWER	= 1llu << OPT_NEWER,
	OB_OVERWRITE	= 1llu << OPT_OVERWRITE,
	OB_IGNORE	= 1llu << OPT_IGNORE,
	OB_REMOVE	= 1llu << OPT_REMOVE,
	OB_TRUNC	= 1llu << OPT_TRUNC,
	OB_FAST		= 1llu << OPT_FAST,
	OB_WDF		= 1llu << OPT_WDF,
	OB_ISO		= 1llu << OPT_ISO,
	OB_CISO		= 1llu << OPT_CISO,
	OB_WBFS		= 1llu << OPT_WBFS,
	OB_LONG		= 1llu << OPT_LONG,
	OB_INODE	= 1llu << OPT_INODE,
	OB_ITIME	= 1llu << OPT_ITIME,
	OB_MTIME	= 1llu << OPT_MTIME,
	OB_CTIME	= 1llu << OPT_CTIME,
	OB_ATIME	= 1llu << OPT_ATIME,
	OB_TIME		= 1llu << OPT_TIME,
	OB_SET_TIME	= 1llu << OPT_SET_TIME,
	OB_MIXED	= 1llu << OPT_MIXED,
	OB_UNIQUE	= 1llu << OPT_UNIQUE,
	OB_NO_HEADER	= 1llu << OPT_NO_HEADER,
	OB_SECTIONS	= 1llu << OPT_SECTIONS,
	OB_SORT		= 1llu << OPT_SORT,

	OB__MASK	= ( 1llu << OPT__N_SPECIFIC ) -1,
	OB__MASK_PART	= OB_AUTO|OB_ALL|OB_PART,
	OB__MASK_PSEL	= OB_PSEL|OB_RAW,
	OB__MASK_SPLIT	= OB_SPLIT|OB_SPLIT_SIZE,
	OB__MASK_OFT	= OB_WDF|OB_ISO|OB_CISO|OB_WBFS,
	OB__MASK_NO_CHK	= OB_NO_CHECK|OB_FORCE,
	OB__MASK_XTIME	= OB_ITIME|OB_MTIME|OB_CTIME|OB_ATIME,
	OB__MASK_TIME	= OB__MASK_XTIME|OB_TIME,

	// allowed options for each command

	OB_CMD_HELP	= OB__MASK,
	OB_CMD_VERSION	= OB__MASK,
	OB_CMD_TEST	= OB__MASK,
	OB_CMD_ERROR	= OB_LONG|OB_NO_HEADER|OB_SECTIONS,
	OB_CMD_EXCLUDE	= 0,
	OB_CMD_TITLES	= 0,
	OB_CMD_FIND	= OB__MASK_PART|OB_LONG|OB_NO_HEADER,
	OB_CMD_SPACE	= OB__MASK_PART|OB_LONG|OB_NO_HEADER,
	OB_CMD_ANALYZE	= OB__MASK_PART,
	OB_CMD_DUMP	= OB__MASK_PART|OB_LONG|OB_INODE,
	OB_CMD_ID6	= OB__MASK_PART|OB_LONG|OB_SORT|OB_UNIQUE,
	OB_CMD_LIST	= OB__MASK_PART|OB_SORT|OB_LONG|OB__MASK_TIME
			  |OB_MIXED|OB_UNIQUE|OB_NO_HEADER|OB_SECTIONS,
	OB_CMD_FORMAT	= OB__MASK_SPLIT|OB_SIZE|OB_HSS|OB_WSS
			  |OB_INODE|OB_RECOVER|OB_FORCE,
	OB_CMD_CHECK	= OB__MASK_PART|OB_REPAIR|OB_LONG,
	OB_CMD_REPAIR	= OB_CMD_CHECK,
	OB_CMD_EDIT	= OB_AUTO|OB_PART|OB_FORCE,
	OB_CMD_PHANTOM	= OB_AUTO|OB_PART|OB__MASK_NO_CHK,
	OB_CMD_TRUNCATE	= OB__MASK_PART|OB__MASK_NO_CHK,
	OB_CMD_SYNC	= OB__MASK_PART|OB__MASK_PSEL|OB_ENC|OB__MASK_NO_CHK
			  |OB_RECURSE|OB_NEWER|OB_IGNORE|OB_REMOVE|OB_TRUNC,
	OB_CMD_UPDATE	= OB_CMD_SYNC|OB_SYNC,
	OB_CMD_ADD	= OB_CMD_UPDATE|OB_OVERWRITE|OB_UPDATE,
	OB_CMD_EXTRACT	= OB__MASK_PART|OB__MASK_SPLIT|OB__MASK_OFT|OB__MASK_NO_CHK
			  |OB_UNIQUE|OB_DEST|OB_DEST2|OB_FAST|OB_TRUNC
			  |OB_IGNORE|OB_REMOVE|OB_OVERWRITE|OB_UPDATE,
	OB_CMD_REMOVE	= OB__MASK_PART|OB__MASK_NO_CHK|OB_UNIQUE|OB_IGNORE|OB_NO_FREE,
	OB_CMD_RENAME	= OB__MASK_PART|OB__MASK_NO_CHK|OB_IGNORE|OB_ISO|OB_WBFS,
	OB_CMD_SETTITLE	= OB_CMD_RENAME,
	OB_CMD_TOUCH	= OB__MASK_PART|OB__MASK_NO_CHK|OB__MASK_XTIME|OB_SET_TIME
			  |OB_UNIQUE|OB_IGNORE|OB_NO_FREE,
	OB_CMD_FILETYPE	= OB_LONG|OB_IGNORE|OB_NO_HEADER,

} enumOptionsBit;

///////////////////////////////////////////////////////////////////////////////

typedef enum enumCommands
{
	CMD_HELP,
	CMD_VERSION,
	CMD_TEST,
	CMD_ERROR,
	CMD_EXCLUDE,
	CMD_TITLES,

	CMD_FIND,
	CMD_SPACE,
	CMD_ANALYZE,
	CMD_DUMP,

	CMD_ID6,
	CMD_LIST,
	CMD_LIST_L,
	CMD_LIST_LL,
	CMD_LIST_A,
	CMD_LIST_M,
	CMD_LIST_U,

	CMD_FORMAT,
	CMD_CHECK,
	CMD_REPAIR,
	CMD_EDIT,
	CMD_PHANTOM,
	CMD_TRUNCATE,

	CMD_ADD,
	CMD_UPDATE,
	CMD_SYNC,
	CMD_EXTRACT,
	CMD_REMOVE,
	CMD_RENAME,
	CMD_SETTITLE,
	CMD_TOUCH,

	CMD_FILETYPE,

	CMD__N

} enumCommands;

///////////////////////////////////////////////////////////////////////////////

enumIOMode io_mode = 0;

///////////////////////////////////////////////////////////////////////////////

enum // const for long options without a short brothers
{
	GETOPT_BASE	= 0x1000,
	GETOPT_UTF8,
	GETOPT_NO_UTF8,
	GETOPT_LANG,
	GETOPT_PSEL,
	GETOPT_RAW,
	GETOPT_ENC,
	GETOPT_TRUNC,
	GETOPT_INODE,
	GETOPT_ITIME,
	GETOPT_MTIME,
	GETOPT_CTIME,
	GETOPT_ATIME,
	GETOPT_TIME,
	GETOPT_SET_TIME,
	GETOPT_SECTIONS,
	GETOPT_HSS,
	GETOPT_WSS,
	GETOPT_RECOVER,
	GETOPT_NO_CHECK,
	GETOPT_REPAIR,
	GETOPT_NO_FREE,
	GETOPT_IO,
};

char short_opt[] = "hVqvLPtE:n:N:x:X:T:aAr:p:d:D:s:zZ:fuyeoiRFWICBlMUHS:";
struct option long_opt[] =
{
	{ "help",		0, 0, 'h' },
	{ "version",		0, 0, 'V' },
	{ "quiet",		0, 0, 'q' },
	{ "verbose",		0, 0, 'v' },
	{ "logging",		0, 0, 'L' },
	{ "progress",		0, 0, 'P' },
	{ "test",		0, 0, 't' },
	{ "esc",		1, 0, 'E' },
	{ "include",		1, 0, 'n' },
	{ "include-path",	1, 0, 'N' },
	 { "includepath",	1, 0, 'N' },
	{ "exclude",		1, 0, 'x' },
	{ "exclude-path",	1, 0, 'X' },
	 { "excludepath",	1, 0, 'X' },
	{ "titles",		1, 0, 'T' },
	{ "utf-8",		0, 0, GETOPT_UTF8 },
	 { "utf8",		0, 0, GETOPT_UTF8 },
	{ "no-utf-8",		0, 0, GETOPT_NO_UTF8 },
	 { "no-utf8",		0, 0, GETOPT_NO_UTF8 },
	 { "noutf8",		0, 0, GETOPT_NO_UTF8 },
	{ "lang",		1, 0, GETOPT_LANG },

	{ "io",			1, 0, GETOPT_IO }, // [2do] hidden option for tests

	{ "auto",		0, 0, 'a' },
	{ "all",		0, 0, 'A' },
	{ "part",		1, 0, 'p' },
	{ "recurse",		1, 0, 'r' },
	{ "psel",		1, 0, GETOPT_PSEL },
	{ "raw",		0, 0, GETOPT_RAW },
	{ "enc",		1, 0, GETOPT_ENC },
	{ "inode",		0, 0, GETOPT_INODE },
	{ "dest",		1, 0, 'd' },
	{ "DEST",		1, 0, 'D' },
	{ "size",		1, 0, 's' },
	{ "split",		0, 0, 'z' },
	{ "split-size",		1, 0, 'Z' },
	 { "splitsize",		1, 0, 'Z' },
	{ "hss",		1, 0, GETOPT_HSS },
	{ "sector-size",	1, 0, GETOPT_HSS },
	 { "sectorsize",	1, 0, GETOPT_HSS },
	{ "wss",		1, 0, GETOPT_WSS },
	{ "recover",		0, 0, GETOPT_RECOVER },
	{ "force",		0, 0, 'f' },
	{ "no-check",		0, 0, GETOPT_NO_CHECK },
	 { "nocheck",		0, 0, GETOPT_NO_CHECK },
	{ "repair",		1, 0, GETOPT_REPAIR },
	{ "no-free",		0, 0, GETOPT_NO_FREE },
	 { "nofree",		0, 0, GETOPT_NO_FREE },
	{ "update",		0, 0, 'u' },
	{ "sync",		0, 0, 'y' },
	{ "newer",		0, 0, 'e' },
	 { "new",		0, 0, 'e' },
	{ "overwrite",		0, 0, 'o' },
	{ "ignore",		0, 0, 'i' },
	{ "remove",		0, 0, 'R' },
	{ "trunc",		0, 0, GETOPT_TRUNC },
	{ "fast",		0, 0, 'F' },
	{ "wdf",		0, 0, 'W' },
	{ "iso",		0, 0, 'I' },
	{ "ciso",		0, 0, 'C' },
	{ "wbfs",		0, 0, 'B' },
	{ "long",		0, 0, 'l' },
	{ "itime",		0, 0, GETOPT_ITIME },
	{ "mtime",		0, 0, GETOPT_MTIME },
	{ "ctime",		0, 0, GETOPT_CTIME },
	{ "atime",		0, 0, GETOPT_ATIME },
	{ "time",		1, 0, GETOPT_TIME },
	{ "set-time",		1, 0, GETOPT_SET_TIME },
	 { "settime",		1, 0, GETOPT_SET_TIME },
	{ "mixed",		0, 0, 'M' },
	{ "unique",		0, 0, 'U' },
	{ "no-header",		0, 0, 'H' },
	 { "noheader",		0, 0, 'H' },
	{ "sections",		0, 0, GETOPT_SECTIONS },
	{ "sort",		1, 0, 'S' },

	{0,0,0,0}
};

option_t  used_options	= 0;
option_t  env_options	= 0;
time_t    opt_set_time	= 0;

int  print_sections	= 0;
int  long_count		= 0;
int  testmode		= 0;
ccp  opt_dest		= 0;
bool opt_mkdir		= false;
u64  opt_size		= 0;
int  opt_split		= 0;
u64  opt_split_size	= 0;
u32  opt_hss		= 0;
u32  opt_wss		= 0;
bool opt_ignore_fst	= 0;

///////////////////////////////////////////////////////////////////////////////

static const CommandTab_t CommandTab[] =
{
	{ CMD_HELP,	"HELP",		"?",		OB_CMD_HELP },
	{ CMD_VERSION,	"VERSION",	0,		OB_CMD_VERSION },
	{ CMD_TEST,	"TEST",		0,		OB_CMD_TEST },
	{ CMD_ERROR,	"ERROR",	"ERR",		OB_CMD_ERROR },
	{ CMD_EXCLUDE,	"EXCLUDE",	0,		OB_CMD_EXCLUDE },
	{ CMD_TITLES,	"TITLES",	0,		OB_CMD_TITLES },

	{ CMD_FIND,	"FIND",		"F",		OB_CMD_FIND },
	{ CMD_SPACE,	"SPACE",	"DF",		OB_CMD_SPACE },
	{ CMD_ANALYZE,	"ANALYZE",	"ANA",		OB_CMD_ANALYZE },
	{ CMD_ANALYZE,	"ANALYSE",	0,		OB_CMD_ANALYZE },
	{ CMD_DUMP,	"DUMP",		"D",		OB_CMD_DUMP },

	{ CMD_ID6,	"ID6",		"ID",		OB_CMD_ID6 },
	{ CMD_LIST,	"LIST",		"LS",		OB_CMD_LIST },
	{ CMD_LIST_L,	"LIST-L",	"LL",		OB_CMD_LIST },
	{ CMD_LIST_LL,	"LIST-LL",	"LLL",		OB_CMD_LIST },
	{ CMD_LIST_A,	"LIST-A",	"LA",		OB_CMD_LIST },
	{ CMD_LIST_M,	"LIST-M",	"LM",		OB_CMD_LIST },
	{ CMD_LIST_U,	"LIST-U",	"LU",		OB_CMD_LIST },

	{ CMD_FORMAT,	"FORMAT",	"INIT",		OB_CMD_FORMAT },
	{ CMD_CHECK,	"CHECK",	"FSCK",		OB_CMD_CHECK },
	{ CMD_REPAIR,	"REPAIR",	0,		OB_CMD_REPAIR },
	{ CMD_EDIT,	"EDIT",		0,		OB_CMD_EDIT },
	{ CMD_PHANTOM,	"PHANTOM",	0,		OB_CMD_PHANTOM },
	{ CMD_TRUNCATE,	"TRUNCATE",	"TR",		OB_CMD_TRUNCATE },

	{ CMD_ADD,	"ADD",		"A",		OB_CMD_ADD },
	{ CMD_UPDATE,	"UPDATE",	"U",		OB_CMD_UPDATE },
	{ CMD_SYNC,	"SYNC",		0,		OB_CMD_SYNC },
	{ CMD_EXTRACT,	"EXTRACT",	"X",		OB_CMD_EXTRACT },
	{ CMD_REMOVE,	"REMOVE",	"RM",		OB_CMD_REMOVE },
	{ CMD_RENAME,	"RENAME",	"REN",		OB_CMD_RENAME },
	{ CMD_SETTITLE,	"SETTITLE",	"ST",		OB_CMD_SETTITLE },
	{ CMD_TOUCH,	"TOUCH",	0,		OB_CMD_TOUCH },

	{ CMD_FILETYPE,	"FILETYPE",	"FT",		OB_CMD_FILETYPE },

	{ CMD__N,0,0,0 }
};

//
///////////////////////////////////////////////////////////////////////////////

static const char help_text[] =
    "\n"
    TITLE "\n"
    "This is a command line tool to manage WBFS partitions and Wii ISO Images.\n"
    "\n"
    "Syntax: " WWT_SHORT " [option]... command [option|parameter|@file]...\n"
    "\n"
    "Commands:\n"
    "\n"
    "   HELP     | ?    : Print this help and exit.\n"
    "   VERSION         : Print program name and version and exit.\n"
    "   ERROR    | ERR  : Translate exit code to message.\n"
    "   EXCLUDE         : Print the internal exclude database to stdout.\n"
    "   TITLES          : Print the internal title database to stdout.\n"
    "\n"
    "   FIND     | F    : Find WBFS partitions.\n"
    "   SPACE    | DF   : Print disk space of WBFS partitions.\n"
    "   ANALYZE  | ANA  : Analyze files and partitions for WBFS usage.\n"
    "   DUMP     | D    : Dump the content of WBFS partitions.\n"
    "\n"
    "   ID6      | ID   : Print ID6 of all discs of WBFS partitions.\n"
    "   LIST     | LS   : List all discs of WBFS partitions.\n"
    "   LIST-L   | LL   : Same as 'LIST --long'.\n"
    "   LIST-LL  | LLL  : Same as 'LIST --long --long'.\n"
    "   LIST-A   | LA   : Same as 'LIST --long --long --auto'.\n"
    "   LIST-M   | LM   : Same as 'LIST --long --long --mixed'.\n"
    "   LIST-U   | LU   : Same as 'LIST --long --long --unique'.\n"
    "\n"
    "   FORMAT   | INIT : Format WBFS partitions.\n"
    "   CHECK    | FSCK : Check WBFS partitions.\n"
    "   REPAIR          : Shortcut for: CHECK --repair=fbt.\n"
    "   EDIT            : Edit block assignments (dangerous!).\n"
    "   PHANTOM         : Add phantom discs (no content; for tests; fast).\n"
    "   TRUNCATE | TR   : Truncate WBFS partitions.\n"
    "\n"
    "   ADD      | A    : Add ISO images to WBFS partitions.\n"
    "   UPDATE   | U    : Add missing ISO images to WBFS partitions.\n"
    "   SYNC            : Remove and copy until source and WBFS are identical.\n"
    "   EXTRACT  | X    : Extract discs from WBFS partitions as ISO images.\n"
    "   REMOVE   | RM   : Remove discs from WBFS partitions.\n"
    "   RENAME   | REN  : Rename the ID6 of WBFS discs. Disc title can also be set.\n"
    "   SETTITLE | ST   : Set the disc title of WBFS discs.\n"
    "   TOUCH           : Set time stamps of WBFS discs.\n"
    "\n"
    "   FILETYPE | FT   : Print a status line for each given file.\n"
    "\n"
    "General options (for all commands except 'ERROR'):\n"
    "\n"
    "   -h --help          Print this help and exit.\n"
    "   -V --version       Print program name and version and exit.\n"
    "   -q --quiet         Be quiet   -> print only error messages and needed output.\n"
    " * -v --verbose       Be verbose -> print more infos. Multiple usage possible.\n"
    "   -P --progress      Print progress counter independent of verbose level.\n"
    " * -t --test          Run in test mode, modify nothing.\n"
    "   -E --esc char      Define an alternative escape character, default is '%'.\n"
    " * -n --include id    Include oly discs with given ID4 or ID6 from operation.\n"
    " * -n --include @file Read include list from file.\n"
    " * -N --include-path file_or_dir\n"
    "                      ISO file or base of directory tree -> scan their ID6.\n"
    " * -x --exclude id    Exclude discs with given ID4 or ID6 from operation.\n"
    " * -x --exclude @file Read exclude list from file.\n"
    " * -X --exclude-path file_or_dir\n"
    "                      ISO file or base of directory tree -> scan their ID6.\n"
    " * -T --titles file   Read file for disc titles. -T0 disables titles lookup.\n"
 #ifdef __CYGWIN__
    "      --utf-8         Enables UTF-8 support.\n"
    "      --no-utf-8      Disables UTF-8 support (cygwin default).\n"
 #else
    "      --utf-8         Enables UTF-8 support (default).\n"
    "      --no-utf-8      Disables UTF-8 support.\n"
 #endif
    "      --lang lang     Define the language for titles.\n"
    "\n"
    "Command specific options:\n"
    "\n"
    "   -A --all           Use all WBFS partitions found.\n"
    "   -a --auto          Search for WBFS partitions using /proc/partitions.\n"
    " * -p --part  part    File of primary WBFS partition. Multiple usage allowed.\n"
    " * -p --part  @file   Special case: read partition list from 'file' ('-'=stdin).\n"
    " * -r --recurse path  Base of a directory tree with ISO files.\n"
    "      --psel  p-type  Partition selector: (no-)data|update|channel all(=def) raw.\n"
    "      --raw           Short cut for --psel=raw.\n"
    "      --enc  encoding Encoding: none|hash|decrypt|encrypt|sign|auto(=default).\n"
    "   -d --dest  path    Define a destination directory/file.\n"
    "   -D --DEST  path    Like --dest, but create directory path automatically.\n"
    "   -s --size  size    Floating point size. Factors: bckKmMgGtT, default=G.\n"
    "   -z --split         Enable output file splitting, default split size = 4 gb.\n"
    "   -Z --split-size sz Enable output file splitting and set split size.\n"
    "      --hss  size     Define HD sector size, default=512. Factors: kKmMgGtT\n"
    "      --wss  size     Define WBFS sector size, no default. Factors: kKmMgGtT\n"
    "      --recover       Format a WBFS in recover mode.\n"
    "   -f --force         Force operation.\n"
    "      --no-check      Disable automatic check of WBFS before modificastions.\n"
    "      --repair mode   Comma separated list of repair modes: NONE, FBT, INODES,\n"
    "                      RM-INVALID, RM-OVERLAP, RM-FREE, RM-EMPTY, RM-ALL, ALL.\n"
    "      --no-free       Do not free blocks when removing a disc.\n"
    "   -u --update        Copy only non existing discs.\n"
    "   -y --sync          Remove and copy until source and dest are identical.\n"
    "   -e --newer         If source and dest have valid mtimes: copy if source is newer.\n"
    "   -o --overwrite     Overwrite existing files.\n"
    "   -i --ignore        Ignore non existing files/discs without warning.\n"
    "   -R --remove        Remove source files/discs if operation is successful.\n"
    "      --trunc         Trunc ISO images while writing.\n"
    "   -F --fast          Enables fast writing (disables searching for zero blocks).\n"
    "   -W --wdf           Set ISO output file type to WDF. (default)\n"
    "   -I --iso           Set ISO output file type to PLAIN ISO.\n"
    "   -C --ciso          Set ISO output file type to CISO (=WBI).\n"
    "   -B --wbfs          Set ISO output file type to WBFS container.\n"
    " * -l --long          Print in long format. Multiple usage possible.\n"
    "      --inode         Print information for all inodes (invalid discs too).\n"
    "      --itime         Abbreviation of --time=i. Use 'itime' (insertion time).\n"
    "      --mtime         Abbreviation of --time=m. Use 'mtime' (last modification time).\n"
    "      --ctime         Abbreviation of --time=c. Use 'ctime' (last status change time).\n"
    "      --atime         Abbreviation of --time=a. Use 'atime' (last access time).\n"
    " *    --time  list    Set time modes (off,i,m,c,a,date,time,min,sec,...).\n"
    "      --set-time time Use given time instead of current time.\n"
    "   -M --mixed         Print disc infos of all WBFS in mixed mode.\n"
    "   -U --unique        Eliminate multiple entries with same ID6.\n"
    "   -H --no-header     Suppress printing of header and footer.\n"
    "      --sections      Print output in sections and parameter lines.\n"
    "   -S --sort  list    Sort by: id|title|name|size|*time|file|region|...|asc|desc\n"
 #ifdef TEST // [test]
    "\n"
    "      --io flags      IO mode (0=open or 1=fopen) &1=WBFS &2=IMAGE.\n"
 #endif
    "\n"
    "   Options marked with '*' can be used repeatedly to change the behavior.\n"
    "\n"
    "Usage:\n"
    "\n"
    "   HELP     | ?        [ignored]...\n"
    "   VERSION         -l  [ignored]...\n"
    "   ERROR    | ERR  -l  [error_code] // NO OTHER OPTIONS\n"
    "   EXCLUDE             [additional_excludes]...\n"
    "   TITLES              [additional_title_file]...\n"
    "\n"
    "   FIND     | F    -p part -aA -ll   -H                   [wbfs_partition]...\n"
    "   SPACE    | DF   -p part -aA -l    -H                   [wbfs_partition]...\n"
    "   ANALYZE  | ANA  -p part -aA                            [wbfs_partition]...\n"
    "   DUMP     | D    -p part -aA -llll --inode              [wbfs_partition]...\n"
    "\n"
    "   ID6      | ID   -p part -aA            -U -S?sort      [wbfs_partition]...\n"
    "   LIST     | LS   -p part -aA -lll  -H -M -U -S= --*time [wbfs_partition]...\n"
    "   LIST-*   | L*   -p part -aA -ll   -H -M -U -S= --*time [wbfs_partition]...\n"
    "\n"
    "   FORMAT   | INIT --size= --hss= --wss= --recover -f     file|blockdev...\n"
    "   CHECK    | FSCK -p part -aA -ll  --repair=             [wbfs_partition]...\n"
    "   REPAIR          -p part -aA -ll  --repair=             [wbfs_partition]...\n"
    "   EDIT            -p part -a       --force               [sub_command]...\n"
    "   PHANTOM         -p part -a                             [sub_command]...\n"
    "   TRUNCATE | TR   -p part -aA                            [wbfs_partition]...\n"
    "\n"
    "   ADD      | A    -p part -aA -iRCeyou -r --psel= --raw  iso|wbfs|dir...\n"
    "   UPDATE   | U    -p part -aA -iRCey   -r --psel= --raw  iso|wbfs|dir...\n"
    "   SYNC            -p part -aA -iRCe    -r --psel= --raw  iso|wbfs|dir...\n"
    "   EXTRACT  | X    -p part -aA -iRCeou -zZ -UF -d* -WIB   id6...\n"
    "   REMOVE   | RM   -p part -aA -i         -U              id6...\n"
    "   RENAME   | REN  -p part -aA -i -IB                     id6=[new][,title]...\n"
    "   SETTITLE | ST   -p part -aA -i -IB                     id6=title...\n"
    "   TOUCH           -p part -aA -iU --*time --set-time=    id6...\n"
    "\n"
    "   FILETYPE | FT               -i -H -l                   filename...\n"
    "\n";


///////////////////////////////////////////////////////////////////////////////

void help_exit()
{
    fputs(help_text,stdout);
    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

void version_exit()
{
    if (long_count)
	fputs(	"prog=" WWT_SHORT "\n"
		"name=\"" WWT_LONG "\"\n"
		"version=" VERSION "\n"
		"revision=" REVISION  "\n"
		"system=" SYSTEM "\n"
		"author=\"" AUTHOR "\"\n"
		"date=" DATE "\n"
		, stdout );
    else
	fputs( TITLE "\n", stdout );
    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

void print_title ( FILE * f )
{
    static bool done = false;
    if (!done)
    {
	done = true;
	if ( verbose >= 1 && f == stdout )
	    fprintf(f,"\n%s\n\n",TITLE);
	else
	    fprintf(f,"*****  %s  *****\n",TITLE);
    }
}

///////////////////////////////////////////////////////////////////////////////

void hint_exit ( enumError err )
{
    fprintf(stderr,
	    "-> Type '%s -h' (or better '%s -h|less') for more help.\n\n",
	    progname, progname );
    exit(err);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     commands                    ///////////////
///////////////////////////////////////////////////////////////////////////////

// common commands of 'wwt' and 'wit'
#define IS_WWT 1
#include "wwt+wit-cmd.c"

///////////////////////////////////////////////////////////////////////////////

enumError cmd_test()
{
 #if 1 || !defined(TEST) // test options

    return cmd_test_options();

 #elif 1

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	printf("arg = |%s|\n",param->arg);
	time_t tim = ScanTime(param->arg);
	if ( tim == (time_t)-1 )
	    printf(" -> ERROR\n");
	else
	{
	    struct tm * tm = localtime(&tim);
	    char timbuf[40];
	    strftime(timbuf,sizeof(timbuf),"%F %T",tm);
	    printf(" -> %s\n",timbuf);
	}
    }
    return ERR_OK;
    
 #elif 0 && defined __CYGWIN__ // test AllocNormalizedFilenameCygwin

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	char * arg = AllocNormalizedFilenameCygwin(param->arg);
	printf("> %s\n",arg);
	FreeString(arg);
    }
    return ERR_OK;

 #elif 1 // test SubstString

    SubstString_t tab[] =
    {
	{ '1', 0,   0, "1234567890" },
	{ 'a', 'A', 0, "AbCdEfGhIjKlMn" },
	{ 'x', 'X', 0, "xyz" },
	{0,0,0,0}
    };

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	char buf[1000];
	int count;
	SubstString(buf,sizeof(buf),tab,param->arg,&count);
	printf("%s -> %s [%d]\n",param->arg,buf,count);
    }
    return ERR_OK;

 #else // test INT

    int i, max = 5;
    for ( i=1; i <= max; i++ )
    {
	fprintf(stderr,"sleep 20 sec (%d/%d)\n",i,max);
	sleep(20);
    }
    return ERR_OK;

 #endif
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_find()
{
    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    if ( verbose < 0 )
	return AnalyzePartitions(0,true,true);
    
    const enumError err = AnalyzePartitions(stderr,true,true);
    if ( err || verbose < 0 )
    {
	// if quiet: only status will be given, no print out
	return err;
    }

    PartitionInfo_t * info;
    const bool print_header = !(used_options&OB_NO_HEADER);
    switch(long_count)
    {
	case 0:
	    for ( info = first_partition_info; info; info = info->next )
		if ( info->part_mode >= PM_WBFS )
		    printf("%s\n",info->path);
	    break;

	case 1:
	    if (print_header)
		printf("\n"
			"type  wbfs d.usage    size  file (sizes in MiB)\n"
			"-----------------------------------------------\n");
	    for ( info = first_partition_info; info; info = info->next )
		 printf("%-5s %s %7lld %7lld  %s\n",
				GetFileModeText(info->filemode,false,"-"),
				info->part_mode >= PM_WBFS ? "WBFS" : " -- ",
				(info->disk_usage+MiB/2)/MiB,
				(info->file_size+MiB/2)/MiB,
				info->path );
	    if (print_header)
		printf("\n");
	    break;

	default: // >= 2x long_count
	    if (print_header)
		printf("\n"
			"type  wbfs    disk usage     file size  full path\n"
			"-------------------------------------------------\n");
	    for ( info = first_partition_info; info; info = info->next )
		printf("%-5s %s %13lld %13lld  %s\n",
				GetFileModeText(info->filemode,false,"-"),
				info->part_mode >= PM_WBFS ? "WBFS" : " -- ",
				info->disk_usage,
				info->file_size,
				info->real_path );
	    if (print_header)
		printf("\n");
	    break;
    }

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_analyze()
{
    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    File_t F;
    InitializeFile(&F);

    PartitionInfo_t * info;
    for ( info = first_partition_info; info; info = info->next )
    {
	if ( !info->path || !*info->path )
	    continue;

	const enumError stat = OpenFile(&F,info->path,IOM_IS_WBFS);
	if (stat)
	    continue;

	printf("\nANALYZE %s\n",info->path);
	AWData_t awd;
	AnalyzeWBFS(&awd,&F);
	PrintAnalyzeWBFS(&awd,stdout,0);
    }
    putchar('\n');
    ResetFile(&F,0);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_dump()
{
    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    enumError err = AnalyzePartitions(stderr,false,true);
    if (err)
	return err;

    const int invalid = long_count > 4
				? 2
				: used_options & OB_INODE || long_count > 3;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	printf("\nDUMP of %s\n\n",info->path);
	DumpWBFS(&wbfs,stdout,2,long_count,invalid,0);
    }
    return ResetWBFS(&wbfs);
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_space()
{
    opt_all++; // --auto is implied

    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    enumError err = AnalyzePartitions(stderr,false,true);
    if (err)
	return err;

    const bool print_header = !(used_options&OB_NO_HEADER);
    if (print_header)
	printf("\n   size    used used%%   free   discs    file   (sizes in MiB)\n"
		 "--------------------------------------------------------------\n");

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	printf("%7d %7d %3d%% %7d %4d/%-4d  %s\n",
		wbfs.total_mib,
		wbfs.used_mib,
		100*wbfs.used_mib/(wbfs.total_mib),
		wbfs.free_mib,
		wbfs.used_discs,
		wbfs.total_discs,
		long_count ? info->real_path : info->path );
    }
    if (print_header)
	printf("\n");

    return ResetWBFS(&wbfs);
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_id6()
{
    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if ( used_options & OB_UNIQUE )
	opt_all++;

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    const int err = AnalyzePartitions(stderr,false,true);
    if (err)
	return err;

    ScanPartitionGames();
    SortWDiscList(&pi_wlist,sort_mode,SORT_TITLE, used_options&OB_UNIQUE ? 2 : 0 );

    int i;
    WDiscListItem_t * witem = pi_wlist.first_disc;
    if (long_count)
	for ( i = pi_wlist.used; i-- > 0; witem++ )
	    printf("%s/%s\n",pi_list[witem->part_index]->path,witem->id6);
    else
	for ( i = pi_wlist.used; i-- > 0; witem++ )
	    printf("%s\n",witem->id6);

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////

int print_list_mixed()
{
    ScanPartitionGames();
    SortWDiscList(&pi_wlist,sort_mode,SORT_TITLE, (used_options&OB_UNIQUE) != 0 );

    WDiscListItem_t * witem;
    char footer[200];
    int footer_len = 0;
    int i, max_name_wd = 9;

    if ( print_sections )
    {
	printf("\n[wbfs]\n");
	printf("total_wbfs=%u\n",pi_count);
	printf("total_discs=%u\n",pi_wlist.used);
	printf("used_mib=%u\n",pi_wlist.total_size_mib);
	printf("free_mib=%u\n",pi_free_mib);
	printf("total_mib=%u\n",pi_wlist.total_size_mib+pi_free_mib);

	char buf[10];
	snprintf(buf,sizeof(buf),"%u",pi_wlist.used-1);
	const int fw = strlen(buf);
	for ( i = 0, witem = pi_wlist.first_disc; i < pi_wlist.used; i++, witem++ )
	{
	    printf("\n[disc-%0*u]\n",fw,i);
	    const int pi = witem->part_index;
	    PrintSectWDiscListItem(stdout,witem,
			pi > 0 && pi <= pi_count ? pi_list[pi]->path : 0 );
	}
	return ERR_OK;
    }

    const bool print_header = !(used_options&OB_NO_HEADER);
    if (print_header)
    {
	if ( long_count > 1 )
	{
	    // find max path width
	    for ( i = 1; i <= pi_count; i++ )
	    {
		const int plen = strlen(pi_list[i]->path);
		if ( max_name_wd < plen )
		    max_name_wd = plen;
	    }

	    printf("\n WI  WBFS file\n%.*s\n",max_name_wd+6,sep_200);
	    for ( i = 1; i <= pi_count; i++ )
		printf("%3d  %s\n",i,pi_list[i]->path);
	    printf("%.*s\n",max_name_wd+6,sep_200);
	}

	// find max name width
	max_name_wd = 0;
	for ( i = pi_wlist.used, witem = pi_wlist.first_disc; i-- > 0; witem++ )
	{
	    const int plen = strlen( witem->title
					? witem->title : witem->name64 );
	    if ( max_name_wd < plen )
		max_name_wd = plen;
	}

	footer_len = snprintf(footer,sizeof(footer),
		"Total: %u discs, %u MiB ~ %u GiB used, %u MiB ~ %u GiB free.",
		pi_wlist.used,
		pi_wlist.total_size_mib, (pi_wlist.total_size_mib+512)/1024,
		pi_free_mib, (pi_free_mib+512)/1024 );
    }

    if ( long_count > 1 )
    {
	if (print_header)
	{
	    int n1, n2;
	    printf("\nID6     MiB Reg.  WI  %n%d discs (%d GiB)%n\n",
		    &n1, pi_wlist.used, (pi_wlist.total_size_mib+512)/1024, &n2 );
	    max_name_wd += n1;
	    if ( max_name_wd < n2 )
		max_name_wd = n2;
	    if ( max_name_wd < footer_len )
		max_name_wd = footer_len;
	    printf("%.*s\n", max_name_wd, sep_200);
	}

	for ( i = pi_wlist.used, witem = pi_wlist.first_disc; i-- > 0; witem++ )
	    printf("%s %4d %s %3d  %s\n",
		    witem->id6, witem->size_mib, witem->region4,
		    witem->part_index,
		    witem->title ? witem->title : witem->name64 );
    }
    else if (long_count)
    {
	if (print_header)
	{
	    int n1, n2;
	    printf("\nID6     MiB Reg.  %n%d discs (%d GiB)%n\n",
		    &n1, pi_wlist.used, (pi_wlist.total_size_mib+512)/1024, &n2 );
	    max_name_wd += n1;
	    if ( max_name_wd < n2 )
		max_name_wd = n2;
	    if ( max_name_wd < footer_len )
		max_name_wd = footer_len;
	    printf("%.*s\n", max_name_wd, sep_200);
	}

	for ( i = pi_wlist.used, witem = pi_wlist.first_disc; i-- > 0; witem++ )
	    printf("%s %4d %s  %s\n",
		    witem->id6, witem->size_mib, witem->region4,
		    witem->title ? witem->title : witem->name64 );
    }
    else
    {
	if (print_header)
	{
	    int n1, n2;
	    printf("\nID6    %n%d discs (%d GiB)%n\n",
		    &n1, pi_wlist.used, (pi_wlist.total_size_mib+512)/1024, &n2 );
	    max_name_wd += n1;
	    if ( max_name_wd < n2 )
		max_name_wd = n2;
	    printf("%.*s\n", max_name_wd, sep_200 );
	}

	for ( i = pi_wlist.used, witem = pi_wlist.first_disc; i-- > 0; witem++ )
	    printf("%s %s\n", witem->id6, witem->title ? witem->title : witem->name64 );
    }

    if (print_header)
	printf("%.*s\n%s\n\n", max_name_wd, sep_200, footer );

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_list()
{
    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto )
	ScanPartitions(true);

    if ( used_options & OB_UNIQUE )
	opt_all++;

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    enumError err = AnalyzePartitions(stderr,false,false);
    if (err)
	return err;

    if ( used_options & (OB_MIXED|OB_UNIQUE) )
	return print_list_mixed();

    const bool print_header = !print_sections && !(used_options&OB_NO_HEADER);

    int gt_wbfs = 0, gt_disc = 0, gt_max_disc = 0, gt_used_mib = 0, gt_free_mib = 0;

    PrintTime_t pt;
    int opt_time = opt_print_time;
    if ( long_count > 1 )
	opt_time = EnablePrintTime(opt_time);
    SetupPrintTime(&pt,opt_time);

    WBFS_t wbfs;
    int wbfs_index = 0;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	WDiscList_t * wlist = GenerateWDiscList(&wbfs,0);
	if (wlist)
	{
	    SortWDiscList(wlist,sort_mode,SORT_TITLE,false);
	    WDiscListItem_t * witem;

	    int i, max_name_wd = 0;
	    char footer[200];
	    int footer_len = 0;

	    if (print_header)
	    {
		// find max name width
		max_name_wd = 0;
		for ( i = wlist->used, witem = wlist->first_disc; i-- > 0; witem++ )
		{
		    const int plen = strlen( witem->title
						? witem->title : witem->name64 );
		    if ( max_name_wd < plen )
			max_name_wd = plen;
		}

		footer_len = snprintf(footer,sizeof(footer),
			"Total: %d/%d discs, %d MiB ~ %d GiB used, %d MiB ~ %d GiB free.",
			wlist->used, wbfs.total_discs,
			wlist->total_size_mib, (wlist->total_size_mib+512)/1024,
			wbfs.free_mib, (wbfs.free_mib+512)/1024 );
	    }

	    if (print_sections)
	    {
		printf("\n[wbfs-%u]\n",wbfs_index++);
		printf("file=%s\n",info->path);
		printf("used_discs=%u\n",wlist->used);
		printf("total_discs=%u\n",wbfs.total_discs);
		printf("used_mib=%u\n",wbfs.total_discs);
		printf("free_mib=%u\n",wbfs.free_mib);
		printf("total_mib=%u\n",wbfs.total_mib);

		char buf[10];
		snprintf(buf,sizeof(buf),"%u",wlist->used-1);
		const int fw = strlen(buf);
		for ( i = 0, witem = wlist->first_disc; i < wlist->used; i++, witem++ )
		{
		    printf("\n[disc-%0*u]\n",fw,i);
		    PrintSectWDiscListItem(stdout,witem,info->path);
		}
	    }
	    else if (long_count)
	    {
		if (print_header)
		{
		    int n1, n2;
		    putchar('\n');
		    printf("ID6   %s  MiB Reg. %n%d/%d discs (%d GiB)%n\n",
				pt.head, &n1, wlist->used, wbfs.total_discs,
				(wlist->total_size_mib+512)/1024, &n2 );
		    noTRACE("N1=%d N2=%d max_name_wd=%d\n",n1,n2,max_name_wd);
		    max_name_wd += n1;
		    if ( max_name_wd < n2 )
			max_name_wd = n2;
		    if ( max_name_wd < footer_len )
			max_name_wd = footer_len;
		    noTRACE(",%d\n",max_name_wd);
		    printf("%.*s\n", max_name_wd, sep_200);
		}
		for ( i = wlist->used, witem = wlist->first_disc; i-- > 0; witem++ )
		    printf("%s%s %4d %s %s\n",
				witem->id6, PrintTime(&pt,&witem->fatt),
				witem->size_mib, witem->region4,
				witem->title ? witem->title : witem->name64 );
	    }
	    else
	    {
		if (print_header)
		{
		    int n1, n2;
		    putchar('\n');
		    printf("ID6   %s  %n%d/%d discs (%d GiB)%n\n",
				pt.head, &n1, wlist->used, wbfs.total_discs,
				(wlist->total_size_mib+512)/1024, &n2 );
		    noTRACE("N1=%d N2=%d max_name_wd=%d\n",n1,n2,max_name_wd);
		    max_name_wd += n1;
		    if ( max_name_wd < n2 )
			max_name_wd = n2;
		    if ( max_name_wd < footer_len )
			max_name_wd = footer_len;
		    noTRACE(" -> %d\n",max_name_wd);
		    printf("%.*s\n", max_name_wd, sep_200 );
		}

		for ( i = wlist->used, witem = wlist->first_disc; i-- > 0; witem++ )
		    printf("%s%s  %s\n",witem->id6, PrintTime(&pt,&witem->fatt),
				witem->title ? witem->title : witem->name64 );
	    }

	    if (print_header)
		printf("%.*s\n%s\n\n", max_name_wd, sep_200, footer );

	    gt_wbfs++;
	    gt_disc	+= wlist->used;
	    gt_max_disc	+= wbfs.total_discs;
	    gt_used_mib	+= wlist->total_size_mib;
	    gt_free_mib	+= wbfs.free_mib;
	    FreeWDiscList(wlist);
	}
    }
    ResetWBFS(&wbfs);

    if ( print_header && gt_wbfs > 1 )
	printf("Grand total: %d WBFS, %d/%d discs, %d GiB used, %d GiB free.\n\n",
		gt_wbfs, gt_disc, gt_max_disc,
		(gt_used_mib+512)/1024, (gt_free_mib+512)/1024 );

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_list_l()
{
    used_options |= OB_LONG;
    long_count++;
    return cmd_list();
}

///////////////////////////////////////////////////////////////////////////////

enumError cmd_list_ll()
{
    used_options |= OB_LONG;
    long_count += 2;
    return cmd_list();
}

///////////////////////////////////////////////////////////////////////////////

enumError cmd_list_a()
{
    if (!opt_auto)
	ScanPartitions(true);
    used_options |= OB_LONG;
    long_count += 2;
    return cmd_list();
}

///////////////////////////////////////////////////////////////////////////////

enumError cmd_list_m()
{
    used_options |= OB_MIXED|OB_LONG;
    opt_all++;
    long_count += 2;
    return cmd_list();
}

///////////////////////////////////////////////////////////////////////////////

enumError cmd_list_u()
{
    used_options |= OB_UNIQUE|OB_LONG;
    opt_all++;
    long_count += 2;
    return cmd_list();
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_format()
{
    if (!(used_options&OB_FORCE))
    {
	testmode++;
	print_title(stderr);
	fprintf(stderr,
		"!! %s: Option --force must be set to formatting a WBFS!\n"
		"!! %s:   => test mode (like --test) enabled!\n\n",
		    progname, progname );

    }
    else if (verbose>=0)
	print_title(stdout);

    if (!n_param)
	return ERROR0(ERR_MISSING_PARAM,"missing parameters => abort.\n");

    int error_count = 0, create_count = 0, format_count = 0;
    const u32 create_size_gib = (opt_size+GiB/2)/GiB;
    const bool opt_recover = 0 != ( used_options & OB_RECOVER );

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	if ( !param->arg || !*param->arg )
	    continue;

	bool recover = opt_recover;
	u32 size_gib = create_size_gib;
	enumFileMode filemode = FM_PLAIN;

	struct stat st;
	if (StatFile(&st,param->arg,0))
	{
	    // no file found
	    recover = false;
	    if (!opt_size)
	    {
		error_count++;
		ERROR0(ERR_SEMANTIC,"Option --size needed to create a new WBFS file: %s",
			param->arg );
		continue;
	    }

	    if (testmode)
	    {
		TRACE("WOULD CREATE FILE %s [%d GiB]\n", param->arg, size_gib );
		printf("WOULD CREATE FILE %s [%d GiB]\n", param->arg, size_gib );
		create_count++;
	    }
	    else
	    {
		TRACE("CREATE FILE %s [%d GiB]\n", param->arg, size_gib );
		if (verbose>=0)
		    printf("CREATE FILE %s [%d GiB]\n", param->arg, size_gib );

		File_t f;
		InitializeFile(&f);
		enumError err = CreateFile(&f,param->arg,IOM_NO_STREAM,1);
		if (err)
		{
		    error_count++;
		    continue;
		}

		if (opt_split)
		    SetupSplitFile(&f,OFT_WBFS,opt_split_size);

		if (SetSizeF(&f,opt_size))
		{
		    ClearFile(&f,true);
		    error_count++;
		    continue;
		}

		ClearFile(&f,false);
		create_count++;
	    }
	}
	else
	{
	    size_gib = (st.st_size+GiB/2)/GiB;
	    filemode = GetFileMode(st.st_mode);
	    if ( filemode == FM_OTHER )
	    {
		ERROR0(ERR_WRONG_FILE_TYPE,
		    "%s: Neither regular file nor block device: %s\n",param->arg);
		continue;
	    }
	}

	ccp filetype = GetFileModeText(filemode,true,"");

	u32 hss = opt_hss, wss = opt_wss;
	if ( recover && ( !hss || !wss ) )
	{
	    if ( testmode || verbose >= 0 )
		printf("ANALYZE %s %s\n", filetype, param->arg );

	    File_t f;
	    InitializeFile(&f);
	    if ( OpenFile(&f,param->arg,IOM_IS_WBFS) == ERR_OK )
	    {
		AWData_t awd;
		AnalyzeWBFS(&awd,&f);
		if (awd.n_record)
		{
		    hss = awd.rec[0].hd_sector_size;
		    wss = awd.rec[0].wbfs_sector_size;
		}
		ResetFile(&f,0);
	    }
	}

	if ( hss < HD_SECTOR_SIZE )
	    hss = HD_SECTOR_SIZE;

	char buf_wss[30];
	if (wss)
	{
	    if ( wss <= hss )
		wss = 2 * hss;
	    if ( wss >= MiB )
		snprintf(buf_wss,sizeof(buf_wss),", wss=%u MiB",wss/MiB);
	    else
		snprintf(buf_wss,sizeof(buf_wss),", wss=%u KiB",wss/KiB);
	}
	else
	    buf_wss[0] = 0;

	if (testmode)
	{
	    TRACE("WOULD FORMAT %s %s [%u GiB, hss=%u%s]\n",
			filetype, param->arg, size_gib, hss, buf_wss );
	    printf("WOULD FORMAT %s %s [%u GiB, hss=%u%s]\n",
			filetype, param->arg, size_gib, hss, buf_wss );
	    format_count++;
	}
	else
	{
	    TRACE("FORMAT %s %s [%u GiB, hss=%u%s]\n",
			filetype, param->arg, size_gib, hss, buf_wss );
	    if (verbose>=0)
		printf("FORMAT %s %s [%u GiB, hss=%u%s]\n",
			filetype, param->arg, size_gib, hss, buf_wss );

	    WBFS_t wbfs;
	    InitializeWBFS(&wbfs);

	    wbfs_param_t par;
	    memset(&par,0,sizeof(par));
	    par.hd_sector_size	= hss;
	    par.wbfs_sector_size= wss;
	    par.reset		= 1;
	    par.clear_inodes	= !recover;
	    par.setup_iinfo	= !recover
				&& ( filemode > FM_PLAIN || used_options & OB_INODE );

	    if ( FormatWBFS(&wbfs,param->arg,true,&par,0,0) == ERR_OK )
	    {
		format_count++;
		if (recover)
		{
		    TRACELINE;
		    wbfs_t * w = wbfs.wbfs;
		    ASSERT(w);
		    ASSERT(w->head);
		    ASSERT(w->head->disc_table);
		    ASSERT(w->freeblks);
		    int slot;
		    for ( slot = 0; slot < w->max_disc; slot++ )
			if (!w->head->disc_table[slot])
			    w->head->disc_table[slot] = 5;
		    memset(w->freeblks,0,w->freeblks_size4*4);
		    SyncWBFS(&wbfs);

		    CheckWBFS_t ck;
		    InitializeCheckWBFS(&ck);
		    TRACELINE;
		    if (CheckWBFS(&ck,&wbfs,-1,0,0))
		    {
			ASSERT(ck.disc);
			bool dirty = false;
			for ( slot = 0; slot < w->max_disc; slot++ )
			    if (!w->head->disc_table[slot] & 4 )
			    {
				CheckDisc_t * cd = ck.disc + slot;
				if ( !cd->no_blocks
				    || cd->bl_overlap
				    || cd->bl_invalid )
				{
				    w->head->disc_table[slot] = 0;
				    dirty = true;
				}
				else
				    w->head->disc_table[slot] &= ~4;
			    }

			if (dirty)
			{
			    ResetCheckWBFS(&ck);
			    SyncWBFS(&wbfs);
			    CheckWBFS(&ck,&wbfs,-1,0,0);
			}

			TRACELINE;
			RepairWBFS(&ck,0,REPAIR_FBT|REPAIR_RM_INVALID|REPAIR_RM_EMPTY,-1,0,0);
			TRACELINE;
			ResetCheckWBFS(&ck);
			SyncWBFS(&wbfs);
			if (CheckWBFS(&ck,&wbfs,1,stdout,1))
			    printf(" *** Run REPAIR %s ***\n\n",param->arg);
			else
			    putchar('\n');
		    }
		    ResetCheckWBFS(&ck);
		}
	    }
	    else
		error_count++;
	    ResetWBFS(&wbfs);
	}
    }

    if (verbose>=0)
    {
	printf("** ");
	if ( create_count > 0 )
	    printf("%d file%s created, ",
		    create_count, create_count==1 ? "" : "s" );
	printf("%d file%s formatted.\n",
		    format_count, format_count==1 ? "" : "s" );
    }

    return error_count ? ERR_WRITE_FAILED : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_check()
{
    if (verbose>=0)
    {
	print_title(stdout);
	fputc('\n',stdout);
    }

    AddEnvPartitions();
    if ( !n_param && !opt_part && !opt_auto && !repair_mode )
	ScanPartitions(true);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    enumError err = AnalyzePartitions(stderr,false,true);
    if (err)
	return err;

    if (long_count)
	verbose = long_count;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    int err_count = 0;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	if ( verbose >= 0 )
	    printf("%sCHECK %s\n", verbose>0 ? "\n" : "", info->path );

	CheckWBFS_t ck;
	InitializeCheckWBFS(&ck);
	if (   CheckWBFS(&ck,&wbfs,verbose,stdout,1)
	    || repair_mode & REPAIR_FBT && wbfs.wbfs->head->wbfs_version < WBFS_VERSION
	    || repair_mode & REPAIR_INODES && ck.no_iinfo_count
	   )
	{
	    if (ck.err)
		err_count++;
	    if (repair_mode)
	    {
		if ( verbose >= 0 )
		    printf("%sREPAIR %s\n", verbose>0 ? "\n" : "", info->path );
		RepairWBFS(&ck,testmode,repair_mode,verbose,stdout,1);
	    }
	}
	ResetCheckWBFS(&ck);
    }
    ResetWBFS(&wbfs);

    if ( verbose > 0 )
	putchar('\n');

    return err_count ? ERR_WBFS_INVALID : max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_repair()
{
    if (!(used_options&OB_REPAIR))
    {
	used_options |= OB_REPAIR;
	repair_mode = REPAIR_DEFAULT;
    }
    return cmd_check();
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_edit()
{
    if (!(used_options&OB_FORCE))
    {
	testmode++;
	print_title(stderr);
    }
    else if (verbose>=0)
	print_title(stdout);

    if (!n_param)
    {
	printf("\n"
	 "EDIT is a dangerous command. It let you (de-)activate the disc slots and\n"
	 "edit the block assignments. Exactly 1 WBFS must be specified. All parameters\n"
	 "are sub commands. Modifications are only done if option --force is set.\n"
	 "\n"
	 "     **********************************************************\n"
	 "     *****  WARNING: This command can damage your WBFS!!  *****\n"
	 "%s"
	 "     *****  See documentation file 'wwt.txt' for details. *****\n"
	 "     **********************************************************\n"
	 "\n",
	used_options & OB_FORCE ? ""
		: "     *****     INFO: Use --force to leave the test mode.  *****\n" );

	return ERR_OK;
    }

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    if ( wbfs_count != 1 )
	return ERROR0( wbfs_count ? ERR_TO_MUCH_WBFS_FOUND : ERR_NO_WBFS_FOUND,
			"Exact one WBFS mus be selected, not %u\n",wbfs_count);

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    err = GetFirstWBFS(&wbfs,&info);
    if (err)
	return err;
    wbfs_t * w = wbfs.wbfs;

    printf("\n* MODIFY WBFS partition %s:\n"
	   "  > WBFS block size:  %x/hex = %u = %1.1f MiB\n"
	   "  > WBFS block range:   1..%u\n"
	   "  > ISO block range:    0..%u\n"
	   "  > Number of discs:  %3u\n"
	   "  > Number of slots:  %3u\n"
	   "\n",
	   info->path,
	   w->wbfs_sec_sz, w->wbfs_sec_sz,
	   (double)w->wbfs_sec_sz/MiB,
	   w->n_wbfs_sec - 1, w->n_wbfs_sec_per_disc,
	   wbfs.used_discs, w->max_disc );

    enum { DO_RM = -5, DO_ACT, DO_INV, DO_FREE, DO_USED };
    ASSERT( DO_USED < 0 );

    CommandTab_t * ctab = calloc(6+wbfs.used_discs,sizeof(*ctab));
    CommandTab_t * cptr = ctab;

    cptr->id	= DO_RM;
    cptr->name1	= "RM";
    cptr->name2	= "R";
    cptr++;

    cptr->id	= DO_ACT;
    cptr->name1	= "ACT";
    cptr->name2	= "A";
    cptr++;

    cptr->id	= DO_INV;
    cptr->name1	= "INV";
    cptr->name2	= "I";
    cptr++;

    cptr->id	= DO_FREE;
    cptr->name1	= "FREE";
    cptr->name2	= "F";
    cptr++;

    cptr->id	= DO_USED;
    cptr->name1	= "USE";
    cptr->name2	= "U";
    cptr++;

    int i;
    WDiscInfo_t dinfo;
    for ( i = 0; i < wbfs.used_discs; i++ )
	if ( GetWDiscInfo(&wbfs,&dinfo,i) == ERR_OK )
	{
	    cptr->id	= i;
	    cptr->name1	= strdup(dinfo.id6);
	    cptr++;
	}

    wbfs_load_freeblocks(w);

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	char * arg = param->arg;
	char cmd_buf[COMMAND_MAX];
	char *dest = cmd_buf;
	char *end  = cmd_buf + sizeof(cmd_buf) - 1;
	while ( *arg && *arg != '=' && dest < end )
	    *dest++ = *arg++;
	*dest = 0;
	int cstat;
	const CommandTab_t * cptr = ScanCommand(&cstat,cmd_buf,ctab);

	if (cstat)
	{
	    ERROR0(ERR_SYNTAX,"Unknown command: %s\n",cmd_buf);
	    putchar('\n');
	    continue;
	}
	ASSERT(cptr);

	if ( *arg != '=' )
	    continue;

	arg++;

	if ( cptr->id == DO_RM || cptr->id == DO_ACT || cptr->id == DO_INV )
	{
	    int mode;
	    ccp info;
	    switch(cptr->id)
	    {
		case DO_RM:  mode = 0; info = "remove"; break;
		case DO_ACT: mode = 1; info = "activate"; break;
		default:     mode = 2; info = "invalidate"; break;
	    }

	    while (*arg)
	    {
		u32 stat, n1, n2;
		arg = ScanRangeU32(arg,&stat,&n1,&n2,0,w->max_disc-1);
		if ( n1 >= 0 && n1 <= n2 )
		{
		    if ( n1 == n2 )
			printf("  - %s%s disc %u.\n",
				testmode ? "WOULD " : "", info, n1 );
		    else
			printf("  - %s%s discs %u..%u.\n",
				testmode ? "WOULD " : "", info, n1, n2 );
		    if (!testmode)
			while ( n1 <= n2 )
			    w->head->disc_table[n1++] = mode;
		}
		if ( *arg != ',' )
		    break;
		arg++;
	    }
	}
	else if ( cptr->id == DO_FREE || cptr->id == DO_USED )
	{
	    const bool is_free = cptr->id == DO_FREE;
	    while (*arg)
	    {
		u32 stat, n1, n2;
		arg = ScanRangeU32(arg,&stat,&n1,&n2,1,w->n_wbfs_sec-1);
		if ( n1 > 0 && n1 <= n2 )
		{
		    if ( n1 == n2 )
			printf("  - %smark block %u as %s.\n",
				testmode ? "WOULD " : "",
				n1, is_free ? "FREE" : "USED" );
		    else
			printf("  - %smark blocks %u..%u as %s.\n",
				testmode ? "WOULD " : "",
				n1, n2, is_free ? "FREE" : "USED" );
		    if (!testmode)
		    {
			if (is_free)
			    while ( n1 <= n2 )
				wbfs_free_block(w,n1++);
			else
			    while ( n1 <= n2 )
				wbfs_use_block(w,n1++);
		    }
		}
		if ( *arg != ',' )
		    break;
		arg++;
	    }
	}
	else
	{
	  wbfs_disc_t * disc = wbfs_open_disc_by_id6(w,(u8*)cptr->name1);
	  if (disc)
	  {
	    u16 * wlba_tab = disc->header->wlba_table;
	    while (*arg)
	    {
		u32 stat, n1, n2;
		arg = ScanRangeU32(arg,&stat,&n1,&n2,0,w->n_wbfs_sec_per_disc-1);
		if ( *arg != ':' )
		    break;
		arg++;

		if ( n1 > 0 && n1 <= n2 )
		{
		    u32 val;
		    arg = ScanNumU32(arg,&stat,&val,0,~(u16)0);
		    u32 add = val > 0, count = n2 - n1;

		    if ( n1 == n2 )
			printf("  - Disc %s: %sset block %u to WBFS block %u.\n",
				    cptr->name1, testmode ? "WOULD " : "",
				    n1, val );
		    else
			printf("  - Disc %s: %sset blocks %u..%u to WBFS blocks %u..%u.\n",
				cptr->name1, testmode ? "WOULD " : "",
				n1, n2, val, (u16)(val+count*add) );
		    if (!testmode)
			while ( n1 <= n2 )
			{
			    wlba_tab[n1] = htons(val);
			    n1++;
			    val += add;
			}
		}
		if ( *arg != ',' )
		    break;
		arg++;
	    }
	    wbfs_sync_disc_header(disc);
	    wbfs_close_disc(disc);
	  }
	}
	putchar('\n');
    }
    wbfs_sync(w);

    CheckWBFS_t ck;
    InitializeCheckWBFS(&ck);
    if (CheckWBFS(&ck,&wbfs,0,stdout,0))
	putchar('\n');
    ResetCheckWBFS(&ck);
    ResetWBFS(&wbfs);

    if (!(used_options&OB_FORCE))
	printf("* Use option --force to leave the test mode.\n\n" );

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_phantom()
{
    if (verbose>=0)
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    int total_add_count = 0, wbfs_count = 0, wbfs_mod_count = 0;

    const bool check_it	    = ( used_options & OB_NO_CHECK ) == 0;
    const bool ignore_check = ( used_options & OB_FORCE ) != 0;
    const u32 max_mib = (u64)WII_MAX_SECTORS * WII_SECTOR_SIZE / MiB;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_t * w = wbfs.wbfs;
	ASSERT(w);

	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	// scan WBFS for already existing phantom ids

	char phantom_used[1000];
	memset(phantom_used,0,sizeof(phantom_used));
	int slot;
	for ( slot = 0; slot < w->max_disc; slot++ )
	{
	    wbfs_disc_t * d = wbfs_open_disc_by_slot(w,slot,0);
	    if (d)
	    {
		u8 * id6 = d->header->disc_header_copy;
		if ( !memcmp(id6,"PHT",3) )
		{
		    const u32 idx = (id6[3]-'0') * 100 + (id6[4]-'0') * 10 + (id6[5]-'0');
		    if ( idx < sizeof(phantom_used) )
			phantom_used[idx]++;
		}
		wbfs_close_disc(d);
	    }
	}

	int wbfs_add_count = 0;
	bool abort = false;
	ParamList_t * param;
	for ( param = first_param;
	      param && !SIGINT_level && !abort;
	      param = param->next )
	{
	    char *arg = param->arg;
	    if ( !arg || !*arg )
		continue;

	    u32 stat, n1 = 1, n2 = 1, s1, s2;

	    arg = ScanRangeU32(arg,&stat,&s1,&s2,1,10000);
	    if ( stat && *arg == 'x' )
	    {
		arg++;
		n1 = s1;
		n2 = s2;
		arg = ScanRangeU32(arg,&stat,&s1,&s2,0,max_mib);
	    }

	    if ( *arg == 'm' || *arg == 'M' )
		arg++;
	    else
	    {
		if ( *arg == 'g' || *arg == 'G' )
		    arg++;
		s1 *= 1024;
		s2 *= 1024;
		if ( s2 > max_mib )
		    s2 = max_mib;
	    }

	    if ( !stat || *arg || n1 > n2 || s1 > s2 )
	    {
		printf(" - PHANTOM ARGUMENT IGNORED: %s\n",param->arg);
		continue;
	    }

	    if ( testmode || verbose >= 0 )
		printf(" - %sGENERATE %d..%d PHANTOMS with %d..%d MiB\n",
			testmode ? "WOULD " : "", n1, n2, s1, s2 );

	    u32 n = n1 + Random32(n2-n1+1);
	    while ( !abort && n-- > 0 )
	    {
		u32 size = s1 + Random32(s2-s1+1);
		u32 n_sect = (u64)size * MiB /WII_SECTOR_SIZE;

		int idx;
		for ( idx = 0; idx < sizeof(phantom_used); idx++ )
		    if (!phantom_used[idx])
			break;
		if ( idx >= sizeof(phantom_used) )
		{
		    n = 0;
		    continue;
		}
		phantom_used[idx]++;

		char id6[7];
		snprintf(id6,sizeof(id6),"PHT%03u",idx);
		if ( testmode || verbose >= 1 )
		    printf("   - %sGENERATE PHANTOM [%s] with %4u MiB = %u Wii sectors.\n",
			testmode ? "WOULD " : "", id6, size, n_sect );
		if (!testmode)
		{
		    if (wbfs_add_phantom(w,id6,n_sect))
			abort = true;
		    else
			wbfs_add_count++;
		}
	    }
	}

	if ( wbfs_add_count )
	{
	    wbfs_mod_count++;
	    total_add_count += wbfs_add_count;

	    if (verbose>=0)
		printf("* WBFS #%d: %d phantom%s added.\n",
		    wbfs_count, wbfs_add_count, wbfs_add_count==1 ? "" : "s" );
	}
    }
    ResetWBFS(&wbfs);

    if ( verbose >= 1 )
	printf("\n");

    if ( verbose >= 0 )
    {
	if ( wbfs_count > 1 )
	{
	    printf("** %d phantom%s added, %d of %d WBFS used.\n",
			total_add_count, total_add_count==1 ? "" : "s",
			wbfs_mod_count, wbfs_count );
	}
	if ( verbose >= 1 )
	    printf("\n");
    }

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_truncate()
{
    if (verbose>=0)
	print_title(stdout);

    if (n_param)
    {
	opt_part++;
	opt_all++;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    CreatePartitionInfo(param->arg,PS_PARAM);
    }

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    if (!n_param)
	return ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    int wbfs_count = 0, wbfs_mod_count = 0;
    const bool check_it	    = ( used_options & OB_NO_CHECK ) == 0;
    const bool ignore_check = ( used_options & OB_FORCE ) != 0;

    SuperFile_t sf;
    InitializeSF(&sf);

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	if (testmode)
	{
	    TRACE("WOULD truncate WBFS to minimal size: %s\n",info->path);
	    printf(" - WOULD truncate WBFS to minimal size: %s\n",info->path);
	    wbfs_mod_count++;
	}
	else if ( TruncateWBFS(&wbfs) == ERR_OK )
	{
	    if (verbose>=0)
		printf(" - WBFS truncated to minimal size: %s\n",info->path);
	    wbfs_mod_count++;
	}
	ResetWBFS(&wbfs);
    }
    ResetSF(&sf,0);

    if ( verbose >= 0 && wbfs_count > 1 )
	printf("** %d of %d WBFS truncated.\n",wbfs_mod_count,wbfs_count);

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

static ID_DB_t sync_list;

enumError exec_scan_id ( SuperFile_t * sf, Iterator_t * it )
{
    if (sf->f.id6[0])
	InsertID(&sync_list,sf->f.id6,0);
    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError exec_add ( SuperFile_t * sf, Iterator_t * it )
{
    ASSERT(sf);
    ASSERT(it);
    ASSERT(it->wbfs);

    if (SIGINT_level)
	return ERR_INTERRUPT;

    enumFileType ft_test = FT_A_ISO|FT_A_SEEKABLE;
    if ( !(sf->f.ftype&FT_A_WDF) && !sf->f.seek_allowed )
	ft_test = FT_A_ISO;

    if ( IsExcluded(sf->f.id6) || PrintErrorFT(&sf->f,ft_test) )
	return ERR_OK;

    bool update    = it->update;
    bool overwrite = it->overwrite;
    int  exists    = -1; // -1=unknown, 0=nonexist, 1=exist

    if (   it->newer
	&& sf->f.fatt.mtime
	&& OpenWDiscID6(it->wbfs,sf->f.id6) == ERR_OK )
    {
	exists = 1;
	ASSERT(it->wbfs->disc);
	wbfs_inode_info_t * iinfo = wbfs_get_disc_inode_info(it->wbfs->disc,1);
	ASSERT(iinfo);
	const time_t mtime = ntoh64(iinfo->mtime);
	if (mtime)
	{
	    overwrite = sf->f.fatt.mtime > mtime;
	    update = !overwrite;
	}
	CloseWDisc(it->wbfs);
    }
    
    if ( exists>0 || exists < 0 && !ExistsWDisc(it->wbfs,sf->f.id6))
    {
	if (update)
	    return ERR_OK;

	if (overwrite)
	{
	    ccp title = GetTitle(sf->f.id6,"");
	    if (testmode)
	    {
		TRACE("WOULD REMOVE [%s] %s\n",sf->f.id6,title);
		printf(" - WOULD REMOVE [%s] %s\n",sf->f.id6,title);
	    }
	    else
	    {
		TRACE("REMOVE [%s] %s\n",sf->f.id6,title);
		if (verbose>=0)
		    printf(" - REMOVE [%s] %s\n",sf->f.id6,title);
		if (RemoveWDisc(it->wbfs,sf->f.id6,0))
		    return ERR_WBFS;
	    }
	}
	else
	{
	    printf(" - DISC %s [%s] already exists -> ignore\n",
		    sf->f.fname, sf->f.id6 );
	    if ( max_error < ERR_JOB_IGNORED )
		max_error = ERR_JOB_IGNORED;
	    return ERR_OK;
	}
    }

    snprintf(iobuf,sizeof(iobuf),"%d",it->source_list.used);
    if (testmode)
    {
	TRACE("WOULD ADD [%s] %s\n",sf->f.id6,sf->f.fname);
	printf(" - WOULD ADD %*u/%u [%s] %s:%s\n",
		(int)strlen(iobuf), it->source_index+1, it->source_list.used,
		sf->f.id6, oft_name[sf->oft], sf->f.fname );
    }
    else
    {
	TRACE("ADD [%s] %s\n",sf->f.id6,sf->f.fname);
	if ( verbose >= 0 || progress > 0 )
	    printf(" - ADD %*u/%u [%s] %s:%s\n",
			(int)strlen(iobuf), it->source_index+1, it->source_list.used,
			sf->f.id6, oft_name[sf->oft], sf->f.fname );
	fflush(stdout);

	sf->indent		= 5;
	sf->show_progress	= verbose > 1 || progress > 0;
	sf->show_summary	= verbose > 0 || progress > 0;
	sf->show_msec		= verbose > 2;
	sf->f.read_behind_eof	= verbose > 1 ? 1 : 2;

	if ( AddWDisc(it->wbfs,sf,partition_selector) > ERR_WARNING )
	    return ERROR0(ERR_WBFS,"Error while creating disc [%s] @%s\n",
			sf->f.id6, it->wbfs->sf->f.fname );
    }
    it->done_count++;
    return ERR_OK;
}

//-----------------------------------------------------------------------------

enumError cmd_add()
{
    if (verbose>=0)
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    // fill the source_list
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AppendStringField(&source_list,param->arg,true);

    if ( !source_list.used && !recurse_list.used )
	return ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    if ( used_options & OB_SYNC )
	used_options |= OB_UPDATE;

    Iterator_t it;
    InitializeIterator(&it);
    it.act_non_iso	= used_options & OB_IGNORE ? ACT_IGNORE : ACT_WARN;
    it.act_non_exist	= it.act_non_iso;
    it.act_open		= it.act_non_iso;
    it.act_wbfs		= ACT_EXPAND;
    it.act_fst		= opt_ignore_fst ? ACT_IGNORE : ACT_EXPAND;
    it.update		= used_options & OB_UPDATE	? 1 : 0;
    it.newer		= used_options & OB_NEWER	? 1 : 0;
    it.overwrite	= used_options & OB_OVERWRITE	? 1 : 0;
    it.remove_source	= used_options & OB_REMOVE	? 1 : 0;

    err = SourceIterator(&it,false,true);
    if (err)
    {
	ResetIterator(&it);
	return err;
    }

    if ( used_options & OB_SYNC )
    {
	it.func = exec_scan_id;
	err = SourceIteratorCollected(&it);
	if (err)
	{
	    ResetIterator(&it);
	    return err;
	}
    }
    it.func = exec_add;

    uint copy_count = 0, rm_count = 0, wbfs_count = 0, wbfs_mod_count = 0;

    const bool check_it	    = ( used_options & OB_NO_CHECK ) == 0;
    const bool ignore_check = ( used_options & OB_FORCE ) != 0;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	int wbfs_rm_count = 0;
	if ( used_options & OB_SYNC )
	{
	    disable_exclude_db++;
	    WDiscList_t * wlist = GenerateWDiscList(&wbfs,0);
	    disable_exclude_db--;

	    WDiscListItem_t * ptr = wlist->first_disc;
	    WDiscListItem_t * end = ptr + wlist->used;
	    for ( ; ptr < end; ptr++ )
	    {
		TDBfind_t stat;
		FindID(&sync_list,ptr->id6,&stat,0);
		if ( stat != IDB_ID_FOUND || IsExcluded(ptr->id6) )
		{
		    if (testmode)
		    {
			printf(" - WOULD REMOVE [%s] %s\n",
				ptr->id6, GetTitle(ptr->id6,ptr->name64) );
			wbfs_rm_count++;
		    }
		    else
		    {
			printf(" - REMOVE [%s] %s\n",
				ptr->id6, GetTitle(ptr->id6,ptr->name64) );
			if (!RemoveWDisc(&wbfs,ptr->id6,false))
			    wbfs_rm_count++;
		    }
		}
	    }
	    FreeWDiscList(wlist);
	}

	it.done_count = 0;
	it.wbfs = &wbfs;
	it.open_dev = wbfs.sf->f.st.st_dev;
	it.open_ino = wbfs.sf->f.st.st_ino;
	err = SourceIteratorCollected(&it);
	if (err)
	    break;

	if ( it.done_count || wbfs_rm_count )
	{
	    wbfs_mod_count++;
	    copy_count += it.done_count;
	    rm_count += wbfs_rm_count;
	    if (verbose>=0)
	    {
		*iobuf = 0;
		if ( wbfs_rm_count )
		    snprintf(iobuf,sizeof(iobuf)," %d disc%s removed,",
			wbfs_rm_count, wbfs_rm_count == 1 ? "" : "s" );
		printf("* WBFS #%d:%s %d disc%s added.\n",
		    wbfs_count, iobuf,
		    it.done_count, it.done_count==1 ? "" : "s" );
	    }
	}

	if ( used_options & OB_TRUNC )
	{
	    if (testmode)
	    {
		TRACE("WOULD truncate WBFS #%d to minimal size.\n",wbfs_count);
		if (!it.done_count)
		    wbfs_mod_count++;
	    }
	    else if ( TruncateWBFS(&wbfs) == ERR_OK )
	    {
		if (verbose>=0)
		    printf("* WBFS #%d truncated to minimal size.\n",wbfs_count);
		if (!it.done_count)
		    wbfs_mod_count++;
	    }
	}
    }
    ResetIterator(&it);
    ResetWBFS(&wbfs);
    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_update()
{
    used_options |= OB_UPDATE;
    return cmd_add();
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_sync()
{
    used_options |= OB_SYNC;
    return cmd_add();
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_extract()
{
    if ( verbose >= 0 && testmode < 2 )
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    CheckParamID6( ( used_options & OB_UNIQUE ) != 0, testmode > 1 );
    if ( testmode > 1 )
	return PrintParamID6();

    if (!id6_param_found)
	return used_options & OB_IGNORE
		? ERR_OK 
		: ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    const int update = used_options & OB_UPDATE;
    if (update)
	DefineExcludePath( opt_dest && *opt_dest ? opt_dest : ".", 1 );

    //----- extract discs

    int extract_count = 0, rm_count = 0;
    int wbfs_count = 0, wbfs_used_count = 0, wbfs_mod_count = 0;
    const int overwrite = update ? -1 : used_options & OB_OVERWRITE ? 1 : 0;
    const bool check_it	    = ( used_options & OB_NO_CHECK ) == 0;
    const bool ignore_check = ( used_options & OB_FORCE ) != 0;

    SuperFile_t fo;
    InitializeSF(&fo);

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	int wbfs_extract_count = 0, wbfs_rm_count = 0;
	ParamList_t * param;
	for ( param = first_param;
	      param && !SIGINT_level;
	      param = param->next )
	{
	    ccp id6 = param->id6;
	    if ( !*id6 || IsExcluded(id6) )
	    {
		param->count++;
		continue;
	    }

	    OpenWDiscID6(&wbfs,id6);
	    wd_header_t * dhead = GetWDiscHeader(&wbfs);
	    if (dhead)
	    {
		if (param->count)
		{
		    TRACE("ALREADY DONE %s @ %s\n",id6,info->path);
		    if ( verbose > 0 )
			printf(" - ALREADY EXTRACTED: [%s]\n",id6);
		    continue;
		}

		RemoveSF(&fo);
		enumOFT oft = CalcOFT(output_file_type,param->arg,0,OFT__DEFAULT);
		ccp title = GetTitle(id6,(ccp)dhead->game_title);
		ccp oname = param->arg || oft != OFT_WBFS ? 0 : id6;

		// calculate the default filename first ...
		GenFileName( &fo.f, 0, oname, title, id6, oft_ext[oft] );

		// and now the destination filename
		SubstString_t subst_tab[] =
		{
		    { 'i', 'I', 0, id6 },
		    { 'n', 'N', 0, (ccp)dhead->game_title },
		    { 't', 'T', 0, title },
		    { 'e', 'E', 0, oft_ext[oft]+1 },
		    { '+', '+', 1, fo.f.fname },
		    {0,0,0,0}
		};
		char dbuf[PATH_MAX], fbuf[PATH_MAX];
		int conv_count1, conv_count2;
		SubstString(dbuf,sizeof(dbuf),subst_tab,opt_dest,&conv_count1);

		char oname_buf[20];
		if ( param->arg )
		    oname = param->arg;
		else if ( oft == OFT_WBFS )
		{
		    snprintf(oname_buf,sizeof(oname_buf),"%s.wbfs",id6);
		    oname = oname_buf;
		}
		else
		    oname = fo.f.fname;

		SubstString(fbuf,sizeof(fbuf),subst_tab,oname,&conv_count2);
		fo.f.create_directory = conv_count1 || conv_count2 || opt_mkdir;
		GenFileName( &fo.f, dbuf, fbuf, 0, 0, 0 );

		if (testmode)
		{
		    if ( overwrite > 0 || stat(fo.f.fname,&fo.f.st) )
		    {
			printf(" - WOULD EXTRACT %s -> %s:%s\n",
				id6, oft_name[oft], fo.f.fname );
			wbfs_extract_count++;
		    }
		    else if (!update)
			printf(" - WOULD NOT OVERWRITE %s -> %s\n",id6,fo.f.fname);
		    param->count++;
		}
		else
		{
		    if ( verbose >= 0 || progress > 0 )
			printf(" - EXTRACT %s -> %s:%s\n",
				id6, oft_name[oft], fo.f.fname );
		    fflush(stdout);

		    if (CreateFile( &fo.f, 0, IOM_IS_IMAGE, overwrite ))
			goto abort;

		    if (opt_split)
			SetupSplitFile(&fo.f,oft,opt_split_size);

		    if (SetupWriteSF(&fo,oft))
		    {
			RemoveSF(&fo);
			goto abort;
		    }

		    fo.enable_fast	= (used_options&OB_FAST)  != 0;
		    fo.enable_trunc	= (used_options&OB_TRUNC) != 0;
		    fo.indent		= 5;
		    fo.show_progress	= verbose > 1 || progress > 0;
		    fo.show_summary	= verbose > 0 || progress > 0;
		    fo.show_msec	= verbose > 2;

		    FileAttrib_t fatt, *set_time_ref = 0;
		    const time_t mtime = ntoh64(dhead->iinfo.mtime);
		    if (mtime)
		    {
			memset(&fatt,0,sizeof(fatt));
			fatt.mtime = fatt.atime = mtime;
			set_time_ref = &fatt;
		    }

		    if (ExtractWDisc(&wbfs,&fo))
		    {
			RemoveSF(&fo);
			ERROR0(ERR_WBFS,"Can't extract disc [%s] @%s\n",id6,info->path);
		    }
		    else if (!ResetSF(&fo,set_time_ref))
		    {
			wbfs_extract_count++;
			param->count++;
		    }
		}
	    }
	  abort:
	    CloseWDisc(&wbfs);
	}

	if (used_options&OB_REMOVE)
	{
	    for ( param = first_param; param; param = param->next )
		if ( param->id6[0] && param->count )
		{
		    TRACE("REMOVE [%s] count=%d\n",param->id6,param->count);
		    if ( RemoveWDisc(&wbfs,param->id6,0) == ERR_OK )
			wbfs_rm_count++;
		}
	    TRACE("%d disc(s) removed\n",wbfs_rm_count);
	}

	if ( wbfs_extract_count || wbfs_rm_count )
	{
	    if (wbfs_extract_count)
	    {
		wbfs_used_count++;
		extract_count += wbfs_extract_count;
	    }
	    if (wbfs_rm_count)
	    {
		rm_count += wbfs_rm_count;
		wbfs_mod_count++;
	    }

	    if (verbose>=0)
	    {
		const int bufsize = 100;
		char buf[bufsize+1];

		if (wbfs_rm_count)
		    snprintf(buf,bufsize,", %d disc%s removed",
				wbfs_rm_count, wbfs_rm_count==1 ? "" : "s" );
		else
		    *buf = 0;
		printf("* WBFS #%d: %d disc%s extracted%s.\n",
		    wbfs_count, wbfs_extract_count, wbfs_extract_count==1 ? "" : "s", buf );
	    }
	}
    }
    ResetWBFS(&wbfs);
    ResetSF(&fo,0);

    if ( verbose >= 1 )
	printf("\n");

    if ( !(used_options&OB_IGNORE) && !SIGINT_level )
    {
	ParamList_t * param;
	int warn_count = 0;
	for ( param = first_param; param; param = param->next )
	    if ( param->id6[0] && !param->count )
	    {
		ERROR0(ERR_WARNING,"Disc [%s] not found.\n",param->id6);
		warn_count++;
	    }
	if ( warn_count && verbose >= 1 )
	    printf("\n");
    }

    if ( verbose >= 0 )
    {
	if ( wbfs_count > 1 )
	{
	    printf("** %d disc%s extracted, %d of %d WBFS used.\n",
			extract_count, extract_count==1 ? "" : "s",
			wbfs_used_count, wbfs_count );
	    if (rm_count)
		printf("** %d disc%s removed, %d of %d WBFS modified.\n",
			rm_count, rm_count==1 ? "" : "s",
			wbfs_mod_count, wbfs_count );
	}
	if ( verbose >= 1 )
	    printf("\n");
    }

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_remove()
{
    if ( verbose >= 0 && testmode < 2 )
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    CheckParamID6( ( used_options & OB_UNIQUE ) != 0, true );
    if ( testmode > 1 )
	return PrintParamID6();

    if (!id6_param_found)
	return used_options & OB_IGNORE
		? ERR_OK 
		: ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    //----- remove discs

    const bool check_it		= ( used_options & OB_NO_CHECK ) == 0;
    const bool ignore_check	= ( used_options & OB_FORCE ) != 0;
    const bool free_slot_only	= ( used_options & OB_NO_FREE ) != 0;
    int rm_count = 0, wbfs_count = 0, wbfs_mod_count = 0;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	int wbfs_rm_count = 0;
	bool wbfs_go = true;
	ParamList_t * param;
	for ( param = first_param;
	      param && wbfs_go && !SIGINT_level;
	      param = param->next )
	{
	    ccp id6 = param->id6;
	    if (!*id6)
		continue;
	    fflush(stdout);

	    if (!OpenWDiscID6(&wbfs,id6))
	    {
		char disc_title[WII_TITLE_SIZE+1];
		wd_header_t *dh = GetWDiscHeader(&wbfs);
		if (dh)
		    StringCopyS(disc_title,sizeof(disc_title),(ccp)dh->game_title);
		else
		    *disc_title = 0;
		CloseWDisc(&wbfs);
		ccp title = GetTitle(id6,disc_title);
		param->count++;
		if (testmode || verbose > 0 )
		{
		    TRACE("%s%s %s @ %s\n",
			testmode ? "WOULD " : "",
			free_slot_only ? "DROP" : "REMOVE", id6, info->path );
		    printf(" - %s%s [%s] %s\n",
			testmode ? "WOULD " : "",
			free_slot_only ? "DROP" : "REMOVE",
			id6, title );
		}
		if (testmode)
		    wbfs_rm_count++;
		else if (RemoveWDisc(&wbfs,id6,free_slot_only))
		    wbfs_go = false;
		else
		    wbfs_rm_count++;
	    }
	}

	if (wbfs_rm_count)
	{
	    wbfs_mod_count++;
	    rm_count += wbfs_rm_count;
	    if (verbose>=0)
		printf("* WBFS #%d: %d disc%s %s.\n",
		    wbfs_count, wbfs_rm_count, wbfs_rm_count==1 ? "" : "s",
		    free_slot_only ? "droped" : "removed" );
	}
    }
    ResetWBFS(&wbfs);

    if ( verbose >= 1 )
	printf("\n");

    if ( !(used_options&OB_IGNORE) && !SIGINT_level )
    {
	ParamList_t * param;
	int warn_count = 0;
	for ( param = first_param; param; param = param->next )
	    if ( param->id6[0] && !param->count )
	    {
		ERROR0(ERR_WARNING,"Disc [%s] not found.\n",param->id6);
		warn_count++;
	    }
	if ( warn_count && verbose >= 1 )
	    printf("\n");
    }

    if ( verbose >= 0 )
    {
	if ( wbfs_count > 1 )
	    printf("** %d disc%s %s, %d of %d WBFS modified.\n",
			rm_count, rm_count==1 ? "" : "s",
			free_slot_only ? "droped" : "removed",
			wbfs_mod_count, wbfs_count );

	if ( wbfs_mod_count > 0 && free_slot_only )
	    printf("*%s Run CHECK to fix the free blocks table%s!\n",
			wbfs_count > 1 ? "*" : "",
			wbfs_mod_count == 1 ? "" : "s" );

	if ( verbose >= 1 )
	    printf("\n");
    }

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_rename ( bool rename_id )
{
    if ( verbose >= 0 )
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    if (!n_param)
	return ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    err = CheckParamRename(rename_id,!rename_id,true);
    if (err)
	return err;

    //----- rename

    const bool check_it		= 0 == ( used_options & OB_NO_CHECK );
    const bool ignore_check	= 0 != ( used_options & OB_FORCE );
    const bool change_wbfs	= 0 != ( used_options & OB_WBFS );
    const bool change_iso	= 0 != ( used_options & OB_ISO );

    int mv_count = 0, wbfs_count = 0, wbfs_mod_count = 0;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	int wbfs_mv_count = 0;
	bool wbfs_go = true;
	ParamList_t * param;
	for ( param = first_param;
	      param && wbfs_go && !SIGINT_level;
	      param = param->next )
	{
	    ccp sel = param->selector;
	    if (!*sel)
		continue;
	    fflush(stdout);

	    if ( *sel == '+' )
	    {
		int idx;
		for ( idx = 0; idx < wbfs.used_discs; idx++ )
		{
		    enumError err = OpenWDiscIndex(&wbfs,idx);
		    if (err)
			return err;
		    RenameWDisc( &wbfs, param->id6, param->arg,
				 change_wbfs, change_iso, verbose, testmode );
		    wbfs_mv_count++;
		}
		param->count++;
		continue;
	    }

	    enumError err;
	    switch (*sel)
	    {
		case '#': // disc slot
		    {
			u32 slot = strtoul(sel+1,0,10);
			err = OpenWDiscSlot(&wbfs,slot,0);
			if ( err == ERR_WDISC_NOT_FOUND )
			    ERROR0(err, "No disc at slot #%u; %s\n",slot,info->path);
		    }
		    break;

		case '$': // disc index
		    {
			u32 idx = strtoul(sel+1,0,10);
			err = OpenWDiscIndex(&wbfs,idx);
			if ( err == ERR_WDISC_NOT_FOUND )
			    ERROR0(err, "No disc at index %u; %s\n",idx,info->path);
		    }
		    break;

		default: // ID6
		    err = OpenWDiscID6(&wbfs,sel);
		    break;
	    }
	    if (!err)
	    {
		RenameWDisc( &wbfs, param->id6, param->arg,
				change_wbfs, change_iso, verbose, testmode );
		param->count++;
		wbfs_mv_count++;
	    }
	}

	if (wbfs_mv_count)
	{
	    wbfs_mod_count++;
	    mv_count += wbfs_mv_count;
	    if (verbose>=0)
		printf("* WBFS #%d: %d disc%s changed.\n",
		    wbfs_count, wbfs_mv_count, wbfs_mv_count==1 ? "" : "s" );
	}
    }
    ResetWBFS(&wbfs);

    if ( verbose >= 1 )
	printf("\n");

    if ( !(used_options&OB_IGNORE) && !SIGINT_level )
    {
	ParamList_t * param;
	int warn_count = 0;
	for ( param = first_param; param; param = param->next )
	    if ( param->selector && !param->count )
	    {
		ERROR0(ERR_WARNING,"Disc [%s] not found.\n",param->selector);
		warn_count++;
	    }
	if ( warn_count && verbose >= 1 )
	    printf("\n");
    }

    if ( verbose >= 0 )
    {
	if ( wbfs_count > 1 )
	    printf("** %d disc%s changed, %d of %d WBFS modified.\n",
			mv_count, mv_count==1 ? "" : "s",
			wbfs_mod_count, wbfs_count );

	if ( verbose >= 1 )
	    printf("\n");
    }

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_touch ( bool rename_id )
{
    if ( verbose >= 0 && testmode < 2 )
	print_title(stdout);

    enumError err = AnalyzePartitions(stdout,false,false);
    if (err)
	return err;

    CheckParamID6( ( used_options & OB_UNIQUE ) != 0, true );
    if ( testmode > 1 )
	return PrintParamID6();

    if (!id6_param_found)
	return used_options & OB_IGNORE
		? ERR_OK 
		: ERROR0(ERR_MISSING_PARAM,"missing parameters\n");

    //----- setup time modes

    int touch_count = 0, wbfs_count = 0, wbfs_mod_count = 0;
    const u64 set_time = !opt_set_time || opt_set_time == (time_t)-1
				? time(0)
				: opt_set_time;
	
    u64 opt = used_options & OB__MASK_XTIME;
    if (!opt)
	opt = OB__MASK_XTIME;

    const u64 itime = opt & OB_ITIME ? set_time : 0;
    const u64 mtime = opt & OB_MTIME ? set_time : 0;
    const u64 ctime = opt & OB_CTIME ? set_time : 0;
    const u64 atime = opt & OB_ATIME ? set_time : 0;
   
    //----- touch discs

    const bool check_it		= 0 == ( used_options & OB_NO_CHECK );
    const bool ignore_check	= 0 != ( used_options & OB_FORCE );

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( err = GetFirstWBFS(&wbfs,&info);
	  !err && !SIGINT_level;
	  err = GetNextWBFS(&wbfs,&info) )
    {
	wbfs_count++;
	if (verbose>=0)
	    printf("%sWBFSv%u #%d opened: %s\n",
		    verbose>0 ? "\n" : "",
		    wbfs.wbfs->head->wbfs_version, wbfs_count, info->path );

	if ( !info->is_checked && check_it )
	{
	    info->is_checked = true;
	    if ( AutoCheckWBFS(&wbfs,ignore_check) > ERR_WARNING )
	    {
		ERROR0(ERR_WBFS_INVALID,"Ignore invalid WBFS: %s\n\n",info->path);
		ResetWBFS(&wbfs);
		continue;
	    }
	}

	int wbfs_touch_count = 0;
	bool wbfs_go = true;
	ParamList_t * param;
	for ( param = first_param;
	      param && wbfs_go && !SIGINT_level;
	      param = param->next )
	{
	    ccp id6 = param->id6;
	    if (!*id6)
		continue;
	    fflush(stdout);

	    if (!OpenWDiscID6(&wbfs,id6))
	    {
		char disc_title[WII_TITLE_SIZE+1];
		wd_header_t *dh = GetWDiscHeader(&wbfs);
		if (dh)
		    StringCopyS(disc_title,sizeof(disc_title),(ccp)dh->game_title);
		else
		    *disc_title = 0;
		ccp title = GetTitle(id6,disc_title);
		param->count++;
		if (testmode || verbose > 0 )
		{
		    TRACE("%sTOUCH %s @ %s\n",
			testmode ? "WOULD " : "", id6, info->path );
		    printf(" - %sTOUCH [%s] %s\n", testmode ? "WOULD " : "", id6, title );
		}
		if (testmode)
		    wbfs_touch_count++;
		else if (wbfs_touch_disc(wbfs.disc,itime,mtime,ctime,atime))
		    wbfs_go = false;
		else
		    wbfs_touch_count++;
		CloseWDisc(&wbfs);
	    }
	}

	if (wbfs_touch_count)
	{
	    wbfs_mod_count++;
	    touch_count += wbfs_touch_count;
	    if (verbose>=0)
		printf("* WBFS #%d: %d disc%s touched.\n",
		    wbfs_count, wbfs_touch_count, wbfs_touch_count==1 ? "" : "s" );
	}
    }
    ResetWBFS(&wbfs);

    if ( verbose >= 1 )
	printf("\n");

    if ( !(used_options&OB_IGNORE) && !SIGINT_level )
    {
	ParamList_t * param;
	int warn_count = 0;
	for ( param = first_param; param; param = param->next )
	    if ( param->id6[0] && !param->count )
	    {
		ERROR0(ERR_WARNING,"Disc [%s] not found.\n",param->id6);
		warn_count++;
	    }
	if ( warn_count && verbose >= 1 )
	    printf("\n");
    }

    if ( verbose >= 0 )
    {
	if ( wbfs_count > 1 )
	    printf("** %d disc%s touched, %d of %d WBFS modified.\n",
			touch_count, touch_count==1 ? "" : "s",
			wbfs_mod_count, wbfs_count );

	if ( verbose >= 1 )
	    printf("\n");
    }

    return max_error;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_filetype()
{
    SuperFile_t sf;
    InitializeSF(&sf);

    if (!(used_options&OB_NO_HEADER))
    {
	if ( long_count > 1 )
	    printf("\n"
		"file     disc   size reg split\n"
		"type     ID6     MIB ion   N  %s\n"
		"%s\n",
		long_count > 2 ? "real path" : "filename", sep_79 );
	else if (long_count)
	    printf("\n"
		"file     disc  split\n"
		"type     ID6     N  filename\n"
		"%s\n", sep_79 );
	else
	    printf("\n"
		"file\n"
		"type     filename\n"
		"%s\n", sep_79 );
    }

    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	ResetSF(&sf,0);
	sf.f.disable_errors = true;
	OpenSF(&sf,param->arg,true,false);
	AnalyzeFT(&sf.f);
	TRACELINE;

	ccp ftype = GetNameFT( sf.f.ftype, used_options & OB_IGNORE ? 1 : 0 );
	if (ftype)
	{
	    if ( long_count > 1 )
	    {
		char split[10] = " -";
		if ( sf.f.split_used > 1 )
		    snprintf(split,sizeof(split),"%2d",sf.f.split_used);

		ccp region = "-   ";
		char size[10] = "   -";
		if (sf.f.id6[0])
		{
		    region = *GetRegionInfo(sf.f.id6[3]);
		    u32 count = CountUsedIsoBlocksSF(&sf,partition_selector);
		    if (count)
			snprintf(size,sizeof(size),"%4u",
				(count+WII_SECTORS_PER_MIB/2)/WII_SECTORS_PER_MIB);
		}

		printf("%-8s %-6s %s %s %s  %s\n",
			ftype, sf.f.id6[0] ? sf.f.id6 : "-",
			size, region, split, sf.f.fname );
	    }
	    else if (long_count)
	    {
		char split[10] = " -";
		if ( sf.f.split_used > 1 )
		    snprintf(split,sizeof(split),"%2d",sf.f.split_used);
		printf("%-8s %-6s %s  %s\n",
			ftype, sf.f.id6[0] ? sf.f.id6 : "-",
			split, sf.f.fname );
	    }
	    else
		printf("%-8s %s\n", ftype, sf.f.fname );
	}
	CloseSF(&sf,0);
    }

    ResetSF(&sf,0);

    if (!(used_options&OB_NO_HEADER))
	putchar('\n');

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   check options                 ///////////////
///////////////////////////////////////////////////////////////////////////////

ccp opt_name_tab[OPT__N_SPECIFIC] = {0};

void SetOption ( int opt_idx, ccp name )
{
    if ( opt_idx > 0 && opt_idx < OPT__N_SPECIFIC )
    {
	used_options |= 1ull << opt_idx;
	opt_name_tab[opt_idx] = name;
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError CheckOptions ( int argc, char ** argv, int is_env )
{
    TRACE("CheckOptions(%d,%p,%d) optind=%d\n",argc,argv,is_env,optind);

    used_options = 0;
    optind = 0;
    int err = 0;

    for(;;)
    {
	const int opt_stat = getopt_long(argc,argv,short_opt,long_opt,0);
	if ( opt_stat == -1 )
	    break;

	noTRACE("CHECK OPTION %02x\n",opt_stat);
	switch (opt_stat)
	{
	  case '?': err++; break;
	  case 'V': version_exit();
	  case 'h': help_exit();
	  case 'q': verbose = -1; break;
	  case 'v': verbose++; break;
	  case 'L': logging++; break;
	  case 'P': progress++; break;
	  case 't': testmode++; break;
	  case 'n': AtFileHelper(optarg,0,AddIncludeID); break;
	  case 'N': AtFileHelper(optarg,0,AddIncludePath); break;
	  case 'x': AtFileHelper(optarg,0,AddExcludeID); break;
	  case 'X': AtFileHelper(optarg,0,AddExcludePath); break;
	  case 'T': AtFileHelper(optarg,true,AddTitleFile); break;

	  case 'A': SetOption(OPT_ALL,"all"); opt_all++; break;
	  case 'd': SetOption(OPT_DEST,"dest"); opt_dest = optarg; break;
	  case 'D': SetOption(OPT_DEST2,"DEST"); opt_dest = optarg; opt_mkdir = true; break;
   	  case 'z': SetOption(OPT_SPLIT,"split"); opt_split++; break;
	  case 'f': SetOption(OPT_FORCE,"force"); break;
	  case 'u': SetOption(OPT_UPDATE,"update"); break;
	  case 'y': SetOption(OPT_SYNC,"sync"); break;
	  case 'e': SetOption(OPT_NEWER,"newer"); break;
   	  case 'o': SetOption(OPT_OVERWRITE,"overwrite"); break;
	  case 'i': SetOption(OPT_IGNORE,"ignore"); break;
	  case 'R': SetOption(OPT_REMOVE,"remove"); break;
	  case 'F': SetOption(OPT_FAST,"fast"); break;
	  case 'W': SetOption(OPT_WDF,"wdf");   output_file_type = OFT_WDF; break;
	  case 'I': SetOption(OPT_ISO,"iso");   output_file_type = OFT_PLAIN; break;
	  case 'C': SetOption(OPT_ISO,"ciso");  output_file_type = OFT_CISO; break;
	  case 'B': SetOption(OPT_WBFS,"wbfs"); output_file_type = OFT_WBFS; break;
	  case 'l': SetOption(OPT_LONG,"long"); long_count++; break;
	  case 'M': SetOption(OPT_MIXED,"mixes");break;
	  case 'U': SetOption(OPT_UNIQUE,"unique"); break;
	  case 'H': SetOption(OPT_NO_HEADER,"no-header"); break;

	  case GETOPT_UTF8:	use_utf8 = true; break;
	  case GETOPT_NO_UTF8:	use_utf8 = false; break;
	  case GETOPT_LANG:	lang_info = optarg; break;
	  case GETOPT_TRUNC:	SetOption(OPT_TRUNC,"trunc"); break;
	  case GETOPT_INODE:	SetOption(OPT_INODE,"inode"); break;
	  case GETOPT_NO_CHECK:	SetOption(OPT_NO_CHECK,"no-check"); break;
	  case GETOPT_NO_FREE:	SetOption(OPT_NO_FREE,"no-free"); break;
	  case GETOPT_RECOVER:	SetOption(OPT_RECOVER,"recover"); break;
	  case GETOPT_SECTIONS:	SetOption(OPT_SECTIONS,"sections"); print_sections++; break;

	  case 'a':
	    if (!opt_auto)
	    {
		SetOption(OPT_AUTO,"");
		ScanPartitions(false);
	    }
	    break;

	  case 'p':
	    SetOption(OPT_PART,"part");
	    AtFileHelper(optarg,false,AddPartition);
	    break;

	  case 'E':
	    if ( ScanEscapeChar(optarg) < 0 )
		err++;
	    break;

	  case 's':
	    SetOption(OPT_SIZE,"size");
	    if (ScanSizeOptU64(&opt_size,optarg,GiB,0,
				"size",MIN_WBFS_SIZE,0,0,0,true))
		err++;
	    break;

	  case 'Z':
	    SetOption(OPT_SPLIT_SIZE,"split-size");
	    if (ScanSizeOptU64(&opt_split_size,optarg,GiB,0,
				"split-size",MIN_SPLIT_SIZE,0,DEF_SPLIT_FACTOR,0,true))
		err++;
	    opt_split++;
	    break;

	  case GETOPT_HSS:
	    SetOption(OPT_HSS,"hss");
	    if (ScanSizeOptU32(&opt_hss,optarg,1,0,
				"hss",512,WII_SECTOR_SIZE,0,1,true))
		err++;
	    break;

	  case GETOPT_WSS:
	    SetOption(OPT_WSS,"wss");
	    if (ScanSizeOptU32(&opt_wss,optarg,1,0,
				"wss",1024,0,0,1,true))
		err++;
	    break;

	  case 'r':
	    SetOption(OPT_RECURSE,"recurse");
	    AppendStringField(&recurse_list,optarg,false);
	    break;

	  case GETOPT_PSEL:
	    {
		SetOption(OPT_PSEL,"psel");
		const int new_psel = ScanPartitionSelector(optarg);
		if ( new_psel == -1 )
		    err++;
		else
		    partition_selector = new_psel;
	    }
	    break;

	  case GETOPT_RAW:
	    partition_selector = WHOLE_DISC;
	    break;

	  case GETOPT_ENC:
	    {
		SetOption(OPT_ENC,"enc");
		const int new_encoding = ScanEncoding(optarg);
		if ( new_encoding == -1 )
		    err++;
		else
		    encoding = new_encoding;
	    }
	    break;

	  case GETOPT_ITIME:
	    SetOption(OPT_ITIME,"itime");
	    opt_print_time = SetPrintTimeMode(opt_print_time,PT_USE_ITIME|PT_F_ITIME|PT_ENABLED);
	    break;

	  case GETOPT_MTIME:
	    SetOption(OPT_MTIME,"mtime");
	    opt_print_time = SetPrintTimeMode(opt_print_time,PT_USE_MTIME|PT_F_MTIME|PT_ENABLED);
	    break;

	  case GETOPT_CTIME:
	    SetOption(OPT_CTIME,"ctime");
	    opt_print_time = SetPrintTimeMode(opt_print_time,PT_USE_CTIME|PT_F_CTIME|PT_ENABLED);
	    break;

	  case GETOPT_ATIME:
	    SetOption(OPT_ATIME,"atime");
	    opt_print_time = SetPrintTimeMode(opt_print_time,PT_USE_ATIME|PT_F_ATIME|PT_ENABLED);
	    break;

	  case GETOPT_TIME:
	    SetOption(OPT_TIME,"time");
	    if ( ScanAndSetPrintTimeMode(optarg) == PT__ERROR )
		err++;
	    break;

	  case GETOPT_SET_TIME:
	    SetOption(OPT_SET_TIME,"set-time");
	    {
		const time_t tim = ScanTime(optarg);
		if ( tim == (time_t)-1 )
		    err++;
		else
		    opt_set_time = tim;
	    }
	    break;

	  case GETOPT_REPAIR:
	    {
		SetOption(OPT_REPAIR,"repair");
		const int new_repair = ScanRepairMode(optarg);
		if ( new_repair == -1 )
		    err++;
		else
		    repair_mode = new_repair;
	    }
	    break;

	  case 'S':
	    {
		SetOption(OPT_SORT,"sort");
		const SortMode new_mode = ScanSortMode(optarg);
		if ( new_mode == SORT__ERROR )
		    err++;
		else
		{
		    TRACE("SORT-MODE set: %d -> %d\n",sort_mode,new_mode);
		    sort_mode = new_mode;
		}
	    }
	    break;

	  case GETOPT_IO:
	    {
		const enumIOMode new_io = strtol(optarg,0,0); // [2do] error handling
		opt_iomode = new_io & IOM__IS_MASK;
		if ( verbose > 0 || opt_iomode != new_io )
		    printf("IO mode set to %#0x.\n",opt_iomode);
		opt_iomode |= IOM_FORCE_STREAM;
	    }
	    break;

	  default:
	    ERROR0(ERR_INTERNAL,"Internal error: unhandled option: '%c'\n",opt_stat);
	    ASSERT(0); // line never reached
	    break;
	}
    }
    TRACELINE;

    if (is_env)
    {
	env_options = used_options;
    }
    else if ( verbose > 3 )
    {
	print_title(stdout);
	printf("PROGRAM_NAME   = %s\n",progname);
	if (lang_info)
	    printf("LANG_INFO      = %s\n",lang_info);
	ccp * sp;
	for ( sp = search_path; *sp; sp++ )
	    printf("SEARCH_PATH[%td] = %s\n",sp-search_path,*sp);
	printf("\n");
    }

    return !err ? ERR_OK : max_error ? max_error : ERR_SYNTAX;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   check command                 ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CheckCommand ( int argc, char ** argv )
{
    TRACE("CheckCommand(%d,) optind=%d\n",argc,optind);

    if ( optind >= argc )
    {
	ERROR0(ERR_SYNTAX,"Missing command.\n");
	hint_exit(ERR_SYNTAX);
    }

    int cmd_stat;
    const CommandTab_t * cmd_ct = ScanCommand(&cmd_stat,argv[optind],CommandTab);
    if (!cmd_ct)
    {
	if ( cmd_stat > 0 )
	    ERROR0(ERR_SYNTAX,"Command abbreviation is ambiguous: %s\n",argv[optind]);
	else
	    ERROR0(ERR_SYNTAX,"Unknown command: %s\n",argv[optind]);
	hint_exit(ERR_SYNTAX);
    }

    TRACE("COMMAND FOUND: #%d = %s\n",cmd_ct->id,cmd_ct->name1);

    option_t forbidden_mask = used_options & ~cmd_ct->opt;
    TRACE("COMMAND: %s\n",cmd_ct->name1);
    TRACE("  - environ options:   %12llx\n",env_options);
    TRACE("  - used options:      %12llx\n",used_options);
    TRACE("  - allowed options:   %12llx\n",cmd_ct->opt);
    TRACE("  - forbidden options: %12llx\n",forbidden_mask);
    if ( forbidden_mask )
    {
	int i;
	for ( i=0; long_opt[i].name; i++, forbidden_mask >>= 1 )
	    if ( forbidden_mask & 1 )
		ERROR0(ERR_SYNTAX,"Command '%s' don't allow the option --%s\n",
				cmd_ct->name1, opt_name_tab[i] );
	hint_exit(ERR_SEMANTIC);
    }
    used_options |= env_options & cmd_ct->opt;

    argc -= optind+1;
    argv += optind+1;

    while ( argc-- > 0 )
	AtFileHelper(*argv++,false,AddParam);

    enumError err = 0;
    switch(cmd_ct->id)
    {
	case CMD_VERSION:	version_exit();
	case CMD_TEST:		err = cmd_test(); break;
	case CMD_ERROR:		err = cmd_error(); break;
	case CMD_EXCLUDE:	err = cmd_exclude(); break;
	case CMD_TITLES:	err = cmd_titles(); break;

	case CMD_FIND:		err = cmd_find(); break;
	case CMD_SPACE:		err = cmd_space(); break;
	case CMD_ANALYZE:	err = cmd_analyze(); break;
	case CMD_DUMP:		err = cmd_dump(); break;

	case CMD_ID6:		err = cmd_id6(); break;
	case CMD_LIST:		err = cmd_list(); break;
	case CMD_LIST_L:	err = cmd_list_l(); break;
	case CMD_LIST_LL:	err = cmd_list_ll(); break;
	case CMD_LIST_A:	err = cmd_list_a(); break;
	case CMD_LIST_M:	err = cmd_list_m(); break;
	case CMD_LIST_U:	err = cmd_list_u(); break;

	case CMD_FORMAT:	err = cmd_format(); break;
	case CMD_CHECK:		err = cmd_check(); break;
	case CMD_REPAIR:	err = cmd_repair(); break;
	case CMD_EDIT:		err = cmd_edit(); break;
	case CMD_PHANTOM:	err = cmd_phantom(); break;
	case CMD_TRUNCATE:	err = cmd_truncate(); break;

	case CMD_ADD:		err = cmd_add(); break;
	case CMD_UPDATE:	err = cmd_update(); break;
	case CMD_SYNC:		err = cmd_sync(); break;
	case CMD_EXTRACT:	err = cmd_extract(); break;
	case CMD_REMOVE:	err = cmd_remove(); break;
	case CMD_RENAME:	err = cmd_rename(true); break;
	case CMD_SETTITLE:	err = cmd_rename(false); break;
	case CMD_TOUCH:		err = cmd_touch(true); break;

	case CMD_FILETYPE:	err = cmd_filetype(); break;

	default:
	    help_exit();
    }

    return PrintErrorStat(err,cmd_ct->name1);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                       main()                    ///////////////
///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    SetupLib(argc,argv,WWT_SHORT,PROG_WWT);

    TRACE_SIZEOF(TDBfind_t);
    TRACE_SIZEOF(ID_DB_t);
    TRACE_SIZEOF(ID_t);

    //----- process arguments

    if ( argc < 2 )
    {
	printf("\n%s\nVisit %s for more infos.\n\n",TITLE,URI_HOME);
	hint_exit(ERR_OK);
    }

    enumError err = CheckEnvOptions("WWT_OPT",CheckOptions,1);
    if (err)
	hint_exit(err);

    err = CheckOptions(argc,argv,0);
    if (err)
	hint_exit(err);

    err = CheckCommand(argc,argv);

    if (SIGINT_level)
	err = ERROR0(ERR_INTERRUPT,"Program interrupted by user.");
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////

