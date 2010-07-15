
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

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  declarations			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumType
{
	//----- base types

	T_END		=    0x0001,  // end of list
	T_DEF_TOOL	=    0x0002,  // start of a new tool
	T_DEF_CMD	=    0x0004,  // define a command
	T_SEP_CMD	=    0x0008,  // define a command separator
	T_DEF_OPT	=    0x0010,  // define an option
	T_SEP_OPT	=    0x0020,  // define an option separator
	T_GRP_BEG	=    0x0040,  // start option definitions for a group
	T_CMD_BEG	=    0x0080,  // start option definitions for a command
	T_CMD_OPT	=    0x0100,  // allowed option for command
	T_COPY_CMD	=    0x0200,  // copy options of other command
	T_COPY_GRP	=    0x0400,  // copy options of group
	T_ALL_OPT	=    0x0800,  // allow all options

	//----- option flags
	
	F_OPT_COMMAND	=  0x010000,  // option is command specific
	F_OPT_GLOBAL	=  0x020000,  // option is global
	F_OPT_MULTIUSE	=  0x040000,  // multiple usage of option possible
	F_OPT_PARAM	=  0x080000,  // option needs a parameter
	F_SEPARATOR	=  0x100000,  // separator element
	F_SUPERSEDE	=  0x200000,  // supersedes all other comamnds and options

	//----- global flags

	F_HIDDEN	= 0x1000000,  // option is hidden from help

	//----- option combinations
	
	T_OPT_C		= T_DEF_OPT | F_OPT_COMMAND,
	T_OPT_CM	= T_DEF_OPT | F_OPT_COMMAND | F_OPT_MULTIUSE,
	T_OPT_CP	= T_DEF_OPT | F_OPT_COMMAND                  | F_OPT_PARAM,
	T_OPT_CMP	= T_DEF_OPT | F_OPT_COMMAND | F_OPT_MULTIUSE | F_OPT_PARAM,

	T_OPT_G		= T_DEF_OPT | F_OPT_GLOBAL,
	T_OPT_GM	= T_DEF_OPT | F_OPT_GLOBAL  | F_OPT_MULTIUSE,
	T_OPT_GP	= T_DEF_OPT | F_OPT_GLOBAL                   | F_OPT_PARAM,
	T_OPT_GMP	= T_DEF_OPT | F_OPT_GLOBAL  | F_OPT_MULTIUSE | F_OPT_PARAM,

	T_OPT_S		= T_DEF_OPT | F_OPT_GLOBAL | F_SUPERSEDE,


	T_COPT		= T_CMD_OPT,
	T_COPT_M	= T_CMD_OPT | F_OPT_MULTIUSE,

	//----- hidden options and commands (hide from help)

	H_DEF_TOOL	= F_HIDDEN | T_DEF_TOOL,
	H_DEF_CMD	= F_HIDDEN | T_DEF_CMD,

	H_OPT_C		= F_HIDDEN | T_OPT_C,
	H_OPT_CM	= F_HIDDEN | T_OPT_CM,
	H_OPT_CP	= F_HIDDEN | T_OPT_CP,
	H_OPT_CMP	= F_HIDDEN | T_OPT_CMP,

	H_OPT_G		= F_HIDDEN | T_OPT_G,
	H_OPT_GM	= F_HIDDEN | T_OPT_GM,
	H_OPT_GP	= F_HIDDEN | T_OPT_GP,
	H_OPT_GMP	= F_HIDDEN | T_OPT_GMP,

	H_COPT		= F_HIDDEN | T_COPT,
	H_COPT_M	= F_HIDDEN | T_COPT_M,

} enumType;

///////////////////////////////////////////////////////////////////////////////

typedef struct info_t
{
	enumType type;		// entry type
	ccp c_name;		// the C name
	ccp namelist;		// list of names
	ccp param;		// name of parameter
	ccp help;		// help text
	
} info_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			some helper macros		///////////////
///////////////////////////////////////////////////////////////////////////////

#define TEXT_WWT_OPT_REPAIR \
	"This option defines how to repair WBFS errors." \
	" The parameter is a comma separated list of the following keywords," \
	" case is ignored:" \
	" @NONE, FBT, INODES, STANDARD," \
	" RM-INVALID, RM-OVERLAP, RM-FREE, RM-EMPTY, RM-ALL, ALL@." \
	"\n" \
	"All keywords can be prefixed by '+' to enable that option," \
	" by a '-' to disable it or" \
	" by a '=' to enable that option and disable all others."

#define TEXT_OPT_CHUNK_MODE(def) \
	"Defines an operation mode for {--chunk-size} and {--max-chunks}." \
	" Allowed keywords are @'ANY'@ to allow any values,," \
	" @'32K'@ to force chunk sizes with a multiple of 32 KiB," \
	" @'POW2'@ to force chunk sizes >=32K and with a power of 2" \
	" or @'ISO'@ for ISO images (more restrictive as @'POW2'@," \
	" best for USB loaders)." \
	" The case of the keyword is ignored." \
	" The default key is @'" def "'@." \
	"\n" \
	"@--chm@ is a shortcut for @--chunk-mode@."

//
///////////////////////////////////////////////////////////////////////////////
///////////////			the info table			///////////////
///////////////////////////////////////////////////////////////////////////////

info_t info_tab[] =
{
  //
  ///////////////////////////////////////////////////////////////////////////
  /////////////			    TOOL wit			/////////////
  ///////////////////////////////////////////////////////////////////////////

  { T_DEF_TOOL,	"wit", 0,
		"wit [option]... command [option|parameter|@file]...",
		"Wiimms ISO Tool :"
		" It can list, analyze, verify, convert, split, join,"
		" extract, compose, rename and compare Wii discs." },

  //
  //---------- list of all wit commands ----------

  { T_DEF_CMD,	"VERSION",	"VERSION",
		"wit VERSION [ignored]...",
		"Print program name and version and exit." },

  { T_DEF_CMD,	"HELP",		"HELP|H|?",
		"wit HELP [command] [ignored]...",
		"Print help and exit."
		" If the first non option is a valid command name,"
		" then a help for the given command is printed." },

  { T_DEF_CMD,	"TEST",		"TEST",
		"wit TEST [ignored]...",
		"Test options: All options are allowed, some are printed." },

  { T_DEF_CMD,	"ERROR",	"ERROR|ERR",
		"wit ERROR [error_code]",
		"Translate exit code to message or print a table with all error messages." },

  { T_DEF_CMD,	"EXCLUDE",	"EXCLUDE",
		"wit EXCLUDE [additional_excludes]...",
		"Dump the internal exclude database to standard output (stdout)." },

  { T_DEF_CMD,	"TITLES",	"TITLES",
		"wit TITLES [additional_title_file]",
		"Dump the internal title database to standard output (stdout)." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"FILELIST",	"FILELIST|FL",
		"wit FILELIST [source]...",
		"List all source files decared by the options"
		" {--source} and {--recurse}." },

  { T_DEF_CMD,	"FILETYPE",	"FILETYPE|FT",
		"wit FILETYPE [source]...",
		"Print a status line for each source file." },

  { T_DEF_CMD,	"ISOSIZE",	"ISOSIZE|SIZE",
		"wit ISOSIZE [source]...",
		"Print a status line with size infos for each source file." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"DUMP",		"DUMP|D",
		"wit DUMP [source]...",
		"Dump the data structure of Wii ISO files, ticket.bin, tmd.bin,"
		" header.bin, boot.bin, fst.bin and of DOL-files."
		" The file type is detected automatically by analyzing the content." },

  { H_DEF_CMD,	"DREGION",	"DREGION|DR",
		"wit DREGION [source]...",
		"Dump the region settings of Wii ISO files." },

  { T_DEF_CMD,	"ID6",		"ID6|ID",
		"wit ID6 [source]...",
		"Print ID6 of all found ISO files." },

  { T_DEF_CMD,	"LIST",		"LIST|LS",
		"wit LIST [source]...",
		"List all found ISO files." },

  { T_DEF_CMD,	"LIST_L",	"LIST-L|LL|LISTL",
		"wit LIST-L [source]...",
		"List all found ISO files."
		" Same as {LIST --long}." },

  { T_DEF_CMD,	"LIST_LL",	"LIST-LL|LLL|LISTLL",
		"wit LIST-LL [source]...",
		"List all found ISO files."
		" Same as {LIST --long --long}." },

  { T_DEF_CMD,	"LIST_LLL",	"LIST-LLL|LLLL|LISTLLL",
		"wit LIST-LLL [source]...",
		"List all found ISO files."
		" Same as {LIST --long --long --long}." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"ILIST",	"ILIST|IL",
		"wit ILIST [source]...",
		"List all files of all discs." },

  { T_DEF_CMD,	"ILIST_L",	"ILIST-L|ILL|ILISTL",
		"wit ILIST-L [source]...",
		"List all files of all discs."
		" Same as {ILIST --long}." },

  { T_DEF_CMD,	"ILIST_LL",	"ILIST-LL|ILLL|ILISTLL",
		"wit ILIST-LL [source]...",
		"List all files of all discs."
		" Same as {ILIST --long --long}." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"DIFF",		"DIFF|CMP",
		"wit DIFF source dest"
		"\n"
		"wit DIFF [-s path]... [-r path]... [source]... [-d|-D] dest",
		"DIFF compares ISO images in  scrubbed or raw mode or on file level."
		" DIFF works like {COPY} but comparing source and destination." },

  { T_DEF_CMD,	"EXTRACT",	"EXTRACT|X",
		"wit EXTRACT source dest"
		"\n"
		"wit EXTRACT [-s path]... [-r path]... [source]... [-d|-D] dest",
		"Extract all files from the source discs." },

  { T_DEF_CMD,	"COPY",		"COPY|CP",
		"wit COPY source dest"
		"\n"
		"wit COPY [-s path]... [-r path]... [source]... [-d|-D] dest",
		"Copy, scrub, convert, split, encrypt and decrypt Wii ISO images." },

  { T_DEF_CMD,	"SCRUB",	"SCRUB|SB",
		"wit SCRUB source"
		"\n"
		"wit SCRUB [-s path]... [-r path]... [source]...",
		"Scrub, convert, split, encrypt and decrypt Wii ISO images." },

  { H_DEF_CMD,	"EDIT",		"EDIT",
		"wit EDIT source"
		"\n"
		"wit EDIT [-s path]... [-r path]... [source]...",
		"Edit an existing Wii ISO images and patch some values." },

  { T_DEF_CMD,	"MOVE",		"MOVE|MV",
		"wit MOVE source dest"
		"\n"
		"wit MOVE [-s path]... [-r path]... [source]... [-d|-D] dest",
		"Move and rename Wii ISO images." },

  { T_DEF_CMD,	"RENAME",	"RENAME|REN",
		"wit RENAME id6=[new][,title]...",
		"Rename the ID6 of discs. Disc title can also be set." },

  { T_DEF_CMD,	"SETTITLE",	"SETTITLE|ST",
		"wit SETTITLE id6=title...",
		"Set the disc title of discs." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"VERIFY",	"VERIFY|V",
		"wit VERIFY [source]...",
		"Verify ISO images (calculate and compare SHA1 check sums)"
		" to find bad dumps." },

  //
  //---------- list of all wit options ----------

  { T_OPT_S,	"VERSION",	"V|version",
		0,
		"Stop parsing the command line, print a version info and exit." },

  { T_OPT_S,	"HELP",		"h|help",
		0,
		"Stop parsing the command line, print a help message and exit." },

  { T_OPT_S,	"XHELP",	"xhelp",
		0,
		"Stop parsing the command line and print a help message"
		" with all commands included. Exit after printing." },

  { T_OPT_GP,	"WIDTH",	"width",
		"width",
		"Define the width (number of columns) for help and some other messages."
		" This option disables the automatic detection of the terminal width." },

  { T_OPT_GM,	"QUIET",	"q|quiet",
		0,
		"Be quiet and print only error messages." },

  { T_OPT_GM,	"VERBOSE",	"v|verbose",
		0,
		"Be verbose and print more progress information."
		" Multiple usage is possible:"
		" Progress counter is enabled if set at least two times."
		" Extended logging is enabled if set at least four times."
		" The impact of the other verbose levels are command dependent." },

  { T_OPT_G,	"PROGRESS",	"P|progress",
		0, "Print progress counter independent of verbose level." },

  { T_OPT_GM,	"LOGGING",	"L|logging",
		0,
		"Special logging for $patching$ and $composing$ Wii discs."
		" If set at least once the disc layout is printed."
		" If set at least twice the partition layout is printed too." },

  { T_OPT_GP,	"ESC",		"E|esc",
		"char",
		"Define an alternative escape character for destination files."
		" The default is '%'."
		" For Windows (CYGWIN) it is a good choice to set @'-E$$'@." },

  { T_OPT_GP,	"IO",		"io",
		"flags",
		"Setup the IO mode for experiments."
		" The standard file IO is based on open() function."
		" The value '1' defines that WBFS IO is based on fopen() function."
		" The value '2' defines the same for ISO files"
		" and the value '3' for both, WBFS and ISO." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_GMP,	"TITLES",	"T|titles",
		"file", "Read file for disc titles. -T0 disables titles lookup." },

  { T_OPT_G,	"UTF_8",	"utf-8|utf8",
		0, "Enables UTF-8 support (default)." },

  { T_OPT_G,	"NO_UTF_8",	"no-utf-8|no-utf8|noutf8",
		0, "Disables UTF-8 support (CYGWIN default)." },

  { T_OPT_GP,	"LANG",		"lang",
		"lang", "Define the language for titles." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_GM,	"TEST",		"t|test",
		0,
		"Run in test mode, modify nothing."
		"\n"
		">>> USE THIS OPTION IF UNSURE! <<<" },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CMP,	"SOURCE",	"s|source",
		"path", "ISO file or directory with ISO files." },

  { T_OPT_CMP,	"RECURSE",	"r|recurse",
		"path", "ISO file or base of a directory tree with ISO files." },

  { T_OPT_CP,	"RDEPTH",	"rdepth",
		"depth", "Set the maximum recurse depth for option"
		" {--recurse} (default=10)." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CMP,	"INCLUDE",	"n|include",
		"id",
		"Include only discs with given ID4 or ID6 from operation."
		" If the parameter begins with a '@@' the given file is read"
		" and each line is scanned for IDs." },

  { T_OPT_CMP,	"INCLUDE_PATH",	"N|include-path|includepath",
		"file_or_dir",
		"ISO file or base of directory tree -> scan their ID6." },

  { T_OPT_CMP,	"EXCLUDE",	"x|exclude",
		"id",
		"Exclude discs with given ID4 or ID6 from operation."
		" If the parameter begins with a '@@' the given file is read"
		" and each line is scanned for IDs." },

  { T_OPT_CMP,	"EXCLUDE_PATH",	"X|exclude-path|excludepath",
		"file_or_dir",
		"ISO file or base of directory tree -> scan their ID6." },

  { T_OPT_CM,	"IGNORE",	"i|ignore",
		0,
		"Ignore non existing files/discs without warning."
		" If set twice then all non Wii ISO images are ignored too." },

  { T_OPT_C,	"IGNORE_FST",	"ignore-fst|ignorefst",
		0, "Disable composing and ignore FST directories as input." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CP,	"PSEL",		"psel",
		"list",
		"This option set the scrubbing mode and defines,"
		" which disc partitions are handled."
		" The parameter is a comma separated list of keywords." \
		" The keywords are divided in three functional groups:" \
		"\n"
		"The first group selects partition types."
		" The names @DATA@, @UDDATE@, @CHANNEL@, @ID@"
		" and the numbers between @0@ and @50@ for partition type are allowed."
		" @'ID'@ is a placeholder for all ID types like the VC channels of SSBB."
		" The prefix @'-'@ means: disable this partition type."
		" The special keyword @'NONE'@ diables all partition types."
		"\n"
		"The second group selects partition tables."
		" The names @PTAB0..PTAB3@ (and @T0..T3@) are allowed."
		" The prefix @'-'@ means: Disable all partitions of that partition table."
		"\n"
		"The third group are additional flags:"
		" @'WHOLE'@ means that the whole partition data is used."
		" @'RAW'@ means that the whole disc is selected."
		"\n"
		"The special keyword @'ALL'@ resets all settings to the default." },

  { T_OPT_C,	"RAW",		"raw",
		0, "Abbreviation of {--psel RAW}." },

  { T_OPT_CP,	"PMODE",	"pmode",
		"p-mode",
		"This options set the prefix mode for listed or extracted files."
		" One of the following values is allowed:"
		" @AUTO, NONE, POINT, NAME, INDEX@."
		" The default value is @'auto'@." },

  { T_OPT_C,	"SNEEK",	"sneek",
		0, "Abbreviation of {--psel data --pmode none --files =sneek}." },

  { H_OPT_G,	"HOOK",		"hook",
		0,
		" [2do] for tests only." },

  { T_OPT_CP,	"ID",		"id",
		"id",
		"This $patching$ option changes the ID of the disc"
		" to the given parameter. 1 to 6 characters are expected."
		" Only defined characters not equal '.' are modified."
		" The disc header, boot.bin, ticket.bin and tmd.bin are "
		" objects to modify. The option {--modify} selects the objects."
		"\n"
		"This patching option is only recognized while $composing$ a disc." },

  { T_OPT_CP,	"NAME",		"name",
		"name",
		"This $patching$ option changes the name (disc title) of the disc"
		" to the given parameter. Up to 63 characters are expected."
		" The disc header and boot.bin are objects to modify."
		" The option {--modify} selects the objects."
		"\n"
		"This patching option is only recognized while $composing$ a disc." },

  { T_OPT_CP,	"MODIFY",	"modify",
		"list",
		" This $patching$ option expects a comma separated list"
		" of the following keywords (case ignored) as parameter:"
		" @NONE, DISC, BOOT, TICKET, TMD, WBFS, ALL@ and @AUTO@ (default)."
		"\n"
		"All keywords can be prefixed by @'+'@ to enable that option,"
		" by a @'-'@ to disable it or"
		" by a @'='@ to enable that option and disable all others."
		"\n"
		"This patching option is only recognized while $composing$ a disc." },

  { T_OPT_CP,	"REGION",	"region",
		"region",
		"This $patching$ option defines the region of the disc. "
		" The region is one of @JAPAN, USA, EUROPE, KOREA, FILE@"
		" or @AUTO@ (default). The case of the keywords is ignored."
		" Unsigned numbers are also accepted."
		"\n"
		"This patching option is only recognized while $composing$ a disc." },

  { T_OPT_CP,	"IOS",		"ios",
		"ios",
		"This $patching$ option defines the system version (IOS to load)"
		" within TMD. The format is @'HIGH:LOW'@ or @'HIGH-LOW'@ or @'LOW'@."
		" If only @LOW@ is set than @HIGH@ is assumed as 1 (standard IOS)."
		"\n"
		"This patching option is only recognized while $composing$ a disc." },

  { T_OPT_CP,	"ENC",		"enc",
		"encoding",
		"Define the encoding mode."
		" The mode is one of NONE, HASHONLY, DECRYPT, ENCRYPT, SIGN or AUTO."
		" The case of the keywords is ignored."
		" The default mode is 'AUTO'." },

  { T_OPT_CP,	"DEST",		"d|dest",
		"path",
		"Define a destination path (directory/file)."
		" The destination path is scanned for escape sequences"
		" (see option {--esc}) to allow generic pathes." },

  { T_OPT_CP,	"DEST2",	"D|DEST",
		"path",
		"Like {--dest}, but create the directory path automatically." },

  { T_OPT_C,	"SPLIT",	"z|split",
		0,
		"Enable output file splitting, default split size = 4 GB." },

  { T_OPT_CP,	"SPLIT_SIZE",	"Z|split-size|splitsize",
		"sz",
		"Enable output file splitting and define split size."
		" The parameter 'sz' is a floating point number followed"
		" by an optional unit factor (one of 'cb' [=1] or "
		" 'kmgtpe' [base=1000] or 'KMGTPE' [base=1024])."
		" The default unit is 'G' (GiB)." },

  { T_OPT_C,	"TRUNC",	"trunc",
		0, "Truncate PLAIN ISO images to the needed size while creating." },

  { T_OPT_CP,	"CHUNK_MODE",	"chunk-mode|chunkmode|chm",
		"mode", TEXT_OPT_CHUNK_MODE("ISO") },

  { T_OPT_CP,	"CHUNK_SIZE",	"chunk-size|chunksize|chz",
		"sz",
		"Define the minimal chunk size if creating a CISO file."
		" The default is to calculate the chunk size from the input file size"
		" and find a good value by using a minimal value of 1 MiB"
		" for {--chunk-mode @ISO@} and @32 KiB@ for modes @32K@ and @POW2@."
		" For the modes @ISO@ and @POW2@ the value is rounded"
		" up to the next power of 2."
		" This calculation also depends from option {--max-chunks}."
		"\n"
		"The parameter 'sz' is a floating point number followed"
		" by an optional unit factor (one of 'cb' [=1] or "
		" 'kmgtpe' [base=1000] or 'KMGTPE' [base=1024])."
		" The default unit is 'M' (MiB)."
		" If the number is prefixed with a @'='@ then options"
		" {--chunk-mode} and {--max-chunks} are ignored"
		" and the given value is used without any rounding or changing."
		"\n"
		"If the input file size is not known (e.g. reading from pipe),"
		" its size is assumed as @12 GiB@."
		"\n"
		" @--chz@ is a shortcut for @--chunk-size@." },

  { T_OPT_CP,	"MAX_CHUNKS",	"max-chunks|maxchunks|mch",
		"n",
		"Define the maximal number of chunks if creating a CISO file."
		" The default value is 8192 for {--chunk-mode ISO}"
		" and 32760 (maximal value) for all other modes."
		" If this value is set than the automatic calculation "
		" of {--chunk-size} will be modified too."
		"\n"
		" @--mch@ is a shortcut for @--max-chunks@." },

  { T_OPT_C,	"PRESERVE",	"p|preserve",
		0, "Preserve file times (atime+mtime)." },

  { T_OPT_C,	"UPDATE",	"u|update",
		0, "Copy only non existing files, ignore other without warning." },

  { T_OPT_C,	"OVERWRITE",	"o|overwrite",
		0, "Overwrite already existing files." },

  { T_OPT_C,	"REMOVE",	"R|remove",
		0, "Remove source files/discs if operation is successful." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"WDF",		"W|wdf",
		0, "Set ISO output file type to WDF. (default)" },

  { T_OPT_C,	"ISO",		"I|iso",
		0, "Set ISO output file type to PLAIN ISO." },

  { T_OPT_C,	"CISO",		"C|ciso",
		0, "Set ISO output file type to CISO (=WBI)." },

  { T_OPT_C,	"WBFS",		"B|wbfs",
		0, "Set ISO output file type to WBFS container." },

  { T_OPT_C,	"FST",		"fst",
		0, "Set ISO output mode to 'file system' (extract ISO)." },

  { T_OPT_CMP,	"FILES",	"F|files",
		"rules",
		"Append a file select rules."
		" This option can be used multiple times to extend the rule list."
		" Rules beginning with a '+' or a '-' are allow or deny rules rules."
		" Rules beginning with a '=' are macros for internal rule sets." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"ITIME",	"itime",
		0,
		"Select 'itime' (insertion time) for printing."
		" @--itime@ is an abbreviation of {--time i}." },

  { T_OPT_C,	"MTIME",	"mtime",
		0,
		"Select 'mtime' (last modification time) for printing."
		" @--mtime@ is an abbreviation of {--time m}." },

  { T_OPT_C,	"CTIME",	"ctime",
		0,
		"Select 'ctime' (last status change time) for printing."
		" @--ctime@ is an abbreviation of {--time c}." },

  { T_OPT_C,	"ATIME",	"atime",
		0,
		"Select 'atime' (last access time) for printing."
		" @--atime@ is an abbreviation of {--time a}." },

  { T_OPT_CMP,	"TIME",		"time",
		"list",
		"Set time printing and sorting mode"
		" The parameter is a comma separated list of the following keywords,"
		" case is ignored:"
		" RESET, OFF, ON, SINGLE, MULTI, NONE, ALL,"
		" I, M, C, A, DATE, TIME, SEC,"
		" IDATE, MDATE, CDATE, ADATE,"
		" ITIME, MTIME, CTIME, ATIME,"
		" ISEC, MSEC, CSEC, ASEC." },

  { T_OPT_CM,	"LONG",		"l|long",
		0, "Print in long format. Multiple usage possible." },

  { T_OPT_CP,	"SHOW",		"show",
		"list",
		"This option allows fine control over the things that are to be printed."
		" The parameter is a comma separated list of the"
		" following keywords, case is ignored: "
		" @NONE, INTRO, P-TAB, P-INFO, P-MAP, D-MAP, TICKET, TMD, USAGE, PATCH,"
		" FILES, OFFSET, SIZE, PATH@ and @ALL@."
		" There are some combined keys:"
		" @PART := P-INFO,P-MAP,TICKET,TMD@,"
		" @MAP := P-MAP,D-MAP@."
		"\n"
		"All keywords can be prefixed by '+' to enable that option,"
		" by a '-' to disable it or"
		" by a '=' to enable that option and disable all others."
		"\n"
		"The additional keywords @DEC@ and @HEX@ can be used to set"
		" a prefered number format."
		" @-HEADER@ suppresses the output of header lines."
		"\n"
		"The commands recognize only some of these keywords"
		" and ignore the others."
		" If @--show@ is set, option {--long} is ignored"
		" for selecting output elements." },

  { T_OPT_C,	"UNIQUE",	"U|unique",
		0, "Eliminate multiple entries with same ID6." },

  { T_OPT_C,	"NO_HEADER",	"H|no-header|noheader",
		0, "Suppress printing of header and footer." },

  { T_OPT_C,	"SECTIONS",	"sections",
		0, "Print in machine readable sections and parameter lines." },

  { T_OPT_CP,	"SORT",		"S|sort",
		"list",
		"Define the sort mode for listings."
		" The parameter is a comma separated list of the following keywords:"
		" @NONE, NAME, TITLE, FILE, SIZE, OFFSET, REGION, WBFS, NPART,"
		" ITIME, MTIME, CTIME, ATIME, TIME = DATE, DEFAULT,"
		" ASCENDING, DESCENDING = REVERSE@." },

  { T_OPT_CP,	"LIMIT",	"limit",
		"num", "Limit the output to NUM messages." },

  //
  //---------- wit GROUP TITLES ----------

  { T_GRP_BEG,	"TITLES",	0,0,0 },

  { T_COPT_M,	"TITLES",	0,0,0 },
  { T_COPT,	"UTF_8",	0,0,0 },
  { T_COPT,	"NO_UTF_8",	0,0,0 },
  { T_COPT,	"LANG",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },


  //---------- wit GROUP SOURCE ----------

  { T_GRP_BEG,	"SOURCE",	0,0,0 },

  { T_COPT_M,	"SOURCE",	0,0,0 },
  { T_COPT_M,	"RECURSE",	0,0,0 },
  { T_COPT,	"RDEPTH",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wit GROUP EXCLUDE ----------

  { T_GRP_BEG,	"EXCLUDE",	0,0,0 },

  { T_COPT_M,	"INCLUDE",	0,0,0 },
  { T_COPT_M,	"INCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },

  //---------- wit GROUP XSOURCE ----------

  { T_GRP_BEG,	"XSOURCE",	0,0,0 },

  { T_COPY_GRP,	"SOURCE",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wit GROUP XXSOURCE ----------

  { T_GRP_BEG,	"XXSOURCE",	0,0,0 },

  { T_COPY_GRP,	"SOURCE",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"IGNORE",	0,0,0 },
  { T_COPT,	"IGNORE_FST",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wit GROUP XTIME ----------

  { T_GRP_BEG,	"XTIME",	0,0,0 },

  { T_COPT,	"ITIME",	0,0,0 },
  { T_COPT,	"MTIME",	0,0,0 },
  { T_COPT,	"CTIME",	0,0,0 },
  { T_COPT,	"ATIME",	0,0,0 },

  //---------- wit GROUP TIME ----------

  { T_GRP_BEG,	"TIME",		0,0,0 },

  { T_COPY_GRP,	"XTIME",	0,0,0 },
  { T_COPT_M,	"TIME",		0,0,0 },


  //---------- wit GROUP PARTITIONS ----------

  { T_GRP_BEG,	"PARTITIONS",	0,0,0 },

  { T_COPT,	"PSEL",		0,0,0 },
  { T_COPT,	"RAW",		0,0,0 },

  //---------- wit GROUP FILES ----------

  { T_GRP_BEG,	"FILES",	0,0,0 },

  { T_COPT,	"PMODE",	0,0,0 },
  { T_COPT_M,	"FILES",	0,0,0 },
  { T_COPT,	"SNEEK",	0,0,0 },

  //---------- wit GROUP PATCH ----------

  { T_GRP_BEG,	"PATCH",	0,0,0 },

  { H_COPT,	"HOOK",		0,0,0 },
  { T_COPT,	"ID",		0,0,0 },
  { T_COPT,	"NAME",		0,0,0 },
  { T_COPT,	"MODIFY",	0,0,0 },
  { T_COPT,	"REGION",	0,0,0 },
  { T_COPT,	"IOS",		0,0,0 },
  { T_COPT,	"ENC",		0,0,0 },

  //---------- wit GROUP SPLIT_CHUNK ----------

  { T_GRP_BEG,	"SPLIT_CHUNK",	0,0,0 },

  { T_COPT,	"SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT_SIZE",	0,0,0 },
  { T_COPT,	"TRUNC",	0,0,0 },
  { T_COPT,	"CHUNK_MODE",	0,0,0 },
  { T_COPT,	"CHUNK_SIZE",	0,0,0 },
  { T_COPT,	"MAX_CHUNKS",	0,0,0 },

  //
  //---------- COMMAND wit HELP ----------

  { T_CMD_BEG,	"HELP",		0,0,0 },

  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wit VERSION ----------

  { T_CMD_BEG,	"VERSION",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print in long format. Ignored if option {--sections} is set." },

  //---------- COMMAND wit TEST ----------

  { T_CMD_BEG,	"TEST",		0,0,0 },

  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wit ERROR ----------

  { T_CMD_BEG,	"ERROR",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print extended message instead of error name." },

  //---------- COMMAND wit EXCLUDE ----------

  { T_CMD_BEG,	"EXCLUDE",	0,0,0 },

  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },

  //---------- COMMAND wit TITLES ----------

  { T_CMD_BEG,	"TITLES",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },

  //---------- COMMAND wit FILELIST ----------

  { T_CMD_BEG,	"FILELIST",	0,0,0 },

  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print the real path instead of given path." },

  //---------- COMMAND wit FILETYPE ----------

  { T_CMD_BEG,	"FILETYPE",	0,0,0 },

  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set at least once or twide additional columns with ID6 (1x)"
	" or ther region (2x) are enabled."
	" If set three or more times the real path instead of given path"
	" is printed." },

  //---------- COMMAND wit ISOSIZE ----------

  { T_CMD_BEG,	"ISOSIZE",	0,0,0 },

  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set the size is printed in MiB too."
	" If set twice two columns with WBFS calculations are added."
	" If set three times the real path of the source is printed." },

  //---------- COMMAND wit DUMP ----------

  { T_CMD_BEG,	"DUMP",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT,	"IGNORE_FST",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FILES",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set at least once a memory map for each partition is printed."
	" If set twice or more a memory map for whole ISO image is printed." },
  { T_COPT,	"SHOW",		0,0,0 },

  //---------- COMMAND wit DREGION ----------

  { T_CMD_BEG,	"DREGION",	0,0,0 },

  { T_COPY_GRP,	"XSOURCE",	0,0,0 },

  //---------- COMMAND wit ID6 ----------

  { T_CMD_BEG,	"ID6",		0,0,0 },

  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT,	"IGNORE_FST",	0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"UNIQUE",	0,0,0 },
  { T_COPT,	"SORT",		0,0,0 },

  //---------- COMMAND wit LIST ----------

  { T_CMD_BEG,	"LIST",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT,	"IGNORE_FST",	0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"UNIQUE",	0,0,0 },
  { T_COPT,	"SORT",		0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set the size in MiB and the region is printed too."
	" If set twice at least on time columns is added."
	" If set three times a second line with number or partitions,"
	" file type and real path is added." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TIME",		0,0,0 },

  //---------- COMMAND wit LIST-L ----------

  { T_CMD_BEG,	"LIST_L",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wit LIST-LL ----------

  { T_CMD_BEG,	"LIST_LL",	0,0,0 },
  { T_COPY_CMD,	"LIST_L",	0,0,0 },

  //---------- COMMAND wit LIST-LLL ----------

  { T_CMD_BEG,	"LIST_LLL",	0,0,0 },
  { T_COPY_CMD,	"LIST_LL",	0,0,0 },

  //---------- COMMAND wit ILIST ----------

  { T_CMD_BEG,	"ILIST",	0,0,0 },

  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FILES",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"SHOW",		0,0,0 },
  { T_COPT,	"SORT",		0,0,
	"Define the sort mode for the file listing."
	" The parameter is a comma separated list of the following keywords:"
	" NONE, NAME, SIZE, OFFSET, ASCENDING, DESCENDING = REVERSE." },

  //---------- COMMAND wit ILIST-L ----------

  { T_CMD_BEG,	"ILIST_L",	0,0,0 },
  { T_COPY_CMD,	"ILIST",	0,0,0 },

  //---------- COMMAND wit ILIST-LL ----------

  { T_CMD_BEG,	"ILIST_LL",	0,0,0 },
  { T_COPY_CMD,	"ILIST_L",	0,0,0 },

  //---------- COMMAND wit DIFF ----------

  { T_CMD_BEG,	"DIFF",		0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FILES",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"PROGRESS",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPT,	"WDF",		0,0,0 },
  { T_COPT,	"ISO",		0,0,0 },
  { T_COPT,	"CISO",		0,0,0 },
  { T_COPT,	"WBFS",		0,0,0 },
  { T_COPT,	"FST",		0,0,0 },

  //---------- COMMAND wit EXTRACT ----------

  { T_CMD_BEG,	"EXTRACT",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FILES",	0,0,0 },
  { T_COPT,	"SORT",		0,0,
	"Define the exracting order."
	" The parameter is a comma separated list of the following keywords:"
	" NONE, NAME, SIZE, OFFSET, ASCENDING, DESCENDING = REVERSE." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"PROGRESS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPT,	"PRESERVE",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },

  //---------- COMMAND wit COPY ----------

  { T_CMD_BEG,	"COPY",		0,0,0 },

  { T_COPY_CMD,	"EXTRACT",	0,0,0 },

  { T_COPT,	"UPDATE",	0,0,0 },
  { T_COPT,	"REMOVE",	0,0,0 },
  { T_COPY_GRP,	"SPLIT_CHUNK",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"WDF",		0,0,0 },
  { T_COPT,	"ISO",		0,0,0 },
  { T_COPT,	"CISO",		0,0,0 },
  { T_COPT,	"WBFS",		0,0,0 },
  { T_COPT,	"FST",		0,0,0 },

  //---------- COMMAND wit SCRUB ----------

  { T_CMD_BEG,	"SCRUB",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"PROGRESS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"SPLIT_CHUNK",	0,0,0 },
  { T_COPT,	"PRESERVE",	0,0,0 },
  { T_COPY_GRP,	"PATCH",	0,0,0 },

  { T_COPT,	"WDF",		0,0,0 },
  { T_COPT,	"ISO",		0,0,0 },
  { T_COPT,	"CISO",		0,0,0 },
  { T_COPT,	"WBFS",		0,0,0 },


  //---------- COMMAND wit EDIT ----------

  { T_CMD_BEG,	"EDIT",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT_M,	"IGNORE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT,	"PRESERVE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },


  //---------- COMMAND wit MOVE ----------

  { T_CMD_BEG,	"MOVE",		0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },

  //---------- COMMAND wit RENAME ----------

  { T_CMD_BEG,	"RENAME",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"ISO",		0,0,
	"Modify ID and title of the ISO image."
	" If neither @--iso@ nor {--wbfs} is set, then both are assumed as active." },
  { T_COPT,	"WBFS",		0,0,
	"Modify ID and title of the inode in the WBFS management area."
	" Option --wbfs make only sense for images within WBFS."
	" If neither {--iso} nor @--wbfs@ is set, then both are assumed as active." },

  //---------- COMMAND wit SETTITLE ----------

  { T_CMD_BEG,	"SETTITLE",	0,0,0 },
  { T_COPY_CMD,	"RENAME",	0,0,0 },

  //---------- COMMAND wit VERIFY ----------

  { T_CMD_BEG,	"VERIFY",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT_M,	"QUIET",	0,0,
	"Be quiet and report only errors. If set twice"
	" then wit will print nothing and only the exit status is set." },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT,	"PROGRESS",	0,0,0 },
  { T_COPT,	"LIMIT",	0,0,
	"Maximal printed errors of each partition."
	" A zero means unlimitted. The default is 10." },
  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"On error print an additional line to localize the exact"
	" position where the error is found."
	" If set twice a hexdump of the hash values is printed too." },

  //
  ///////////////////////////////////////////////////////////////////////////
  /////////////			    TOOL wwt			/////////////
  ///////////////////////////////////////////////////////////////////////////

  { T_DEF_TOOL,	"wwt", 0,
		"wwt [option]... command [option|parameter|@file]...",
		"Wiimms WBFS Tool (WBFS manager) :"
		" It can create, check, repair, verify and clone WBFS files"
		" and partitions. It can list, add, extract, remove and rename"
		" ISO images as part of a WBFS." },

  //
  //---------- list of all wwt commands ----------

  { T_DEF_CMD,	"VERSION",	"VERSION",
		"wwt VERSION [ignored]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"HELP",		"HELP|H|?",
		"wwt HELP [command] [ignored]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"TEST",		"TEST",
		"wwt TEST [ignored]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"ERROR",	"ERROR|ERR",
		"wwt ERROR [error_code]",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"EXCLUDE",	"EXCLUDE",
		"wwt EXCLUDE [additional_excludes]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"TITLES",	"TITLES",
		"wwt TITLES [additional_title_file]",
		0 /* copy of wit */ },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"FIND",		"FIND|F",
		"wwt FIND [wbfs_partition]...",
		"Find WBFS partitions and optionally print some geometric values." },

  { T_DEF_CMD,	"SPACE",	"SPACE|DF",
		"wwt SPACE [wbfs_partition]...",
		"Print disk space of WBFS partitions." },

  { T_DEF_CMD,	"ANALYZE",	"ANALYZE|ANA|ANALYSE",
		"wwt ANALYZE [wbfs_partition]...",
		"Analyze files and partitions for WBFS usage."
		" Try to find old WBFS structures and make calculations for new WBFS." },

  { T_DEF_CMD,	"DUMP",		"DUMP|D",
		"wwt DUMP [wbfs_partition]...",
		"Dump the data structure of WBFS partitions." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"ID6",		"ID6|ID",
		"wwt ID6 [wbfs_partition]...",
		"List all ID6 of all discs of WBFS partitions." },

  { T_DEF_CMD,	"LIST",		"LIST|LS",
		"wwt LIST [wbfs_partition]...",
		"List all discs of WBFS partitions." },

  { T_DEF_CMD,	"LIST_L",	"LIST-L|LL|LISTL",
		"wwt LIST-L [wbfs_partition]...",
		"List all discs of WBFS partitions."
		" Same as {LIST --long}." },

  { T_DEF_CMD,	"LIST_LL",	"LIST-LL|LLL|LISTLL",
		"wwt LIST-LL [wbfs_partition]...",
		"List all discs of WBFS partitions."
		" Same as {LIST --long --long}." },

  { T_DEF_CMD,	"LIST_A",	"LIST-A|LA|LISTA",
		"wwt LIST-A [wbfs_partition]...",
		"List all discs of all WBFS partitions."
		" Same as {LIST --long --long --auto}." },

  { T_DEF_CMD,	"LIST_M",	"LIST-M|LM|LISTM",
		"wwt LIST-M [wbfs_partition]...",
		"List all discs of WBFS partitions in mixed view."
		" Same as {LIST --long --long --mixed}." },

  { T_DEF_CMD,	"LIST_U",	"LIST-U|LU|LISTU",
		"wwt LIST-U [wbfs_partition]...",
		"List all discs of WBFS partitions in mixed view."
		" Same as {LIST --long --long --unique}." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"FORMAT",	"FORMAT|INIT",
		"wwt FORMAT file|blockdev...",
		"Initialize (=format) WBFS partitions and files."
		" Combine with {--recover} to recover discs." },

  { T_DEF_CMD,	"RECOVER",	"RECOVER",
		"wwt RECOVER [wbfs_partition]..",
		"Recover deleted discs of WBFS partitions." },

  { T_DEF_CMD,	"CHECK",	"CHECK|FSCK",
		"wwt CHECK [wbfs_partition]..",
		"Check WBFS partitions and print error listing."
		" To repair WBFS partitions use the option {--repair modelist}."  },

  { T_DEF_CMD,	"REPAIR",	"REPAIR",
		"wwt REPAIR [wbfs_partition]..",
		"Check WBFS partitions and repair errors."
		" 'REPAIR' is a shortcut for {CHECK --repair standard}." },

  { T_DEF_CMD,	"EDIT",		"EDIT",
		"wwt EDIT [sub_command]...",
		"Edit slot and block assignments. Dangerous! Read docu!" },

  { T_DEF_CMD,	"PHANTOM",	"PHANTOM",
		"wwt PHANTOM [sub_command]...",
		"Add phantom discs."
		" Phantom discs have no content and only a header is written."
		" This makes adding discs very fast and this is good for testing." },

  { T_DEF_CMD,	"TRUNCATE",	"TRUNCATE|TR",
		"wwt TRUNCATE [wbfs_partition]..",
		"Truncate WBFS partitions to the really used size." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"ADD",		"ADD|A",
		"wwt ADD iso|wbfs|dir...",
		"Add Wii ISO discs to WBFS partitions." },

  { T_DEF_CMD,	"UPDATE",	"UPDATE|U",
		"wwt UPDATE iso|wbfs|dir...",
		"Add missing Wii ISO discs to WBFS partitions."
		" 'UPDATE' is a shortcut for {ADD --update}."},

  { T_DEF_CMD,	"SYNC",		"SYNC",
		"wwt SYNC iso|wbfs|dir...",
		"Modify primary WBFS (REMOVE and ADD)"
		" until it contains exactly the same discs as all sources together."
		" 'SYNC' is a shortcut for {ADD --sync}."},

  { T_DEF_CMD,	"EXTRACT",	"EXTRACT|X",
		"wwt EXTRACT id6[=dest]...",
		"Extract discs from WBFS partitions and store them as Wii ISO images." },

  { T_DEF_CMD,	"REMOVE",	"REMOVE|RM",
		"wwt REMOVE id6...",
		"Remove discs from WBFS partitions." },

  { T_DEF_CMD,	"RENAME",	"RENAME|REN",
		"wwt RENAME id6=[new][,title]...",
		"Rename the ID6 of WBFS discs. Disc title can also be set." },

  { T_DEF_CMD,	"SETTITLE",	"SETTITLE|ST",
		"wwt SETTITLE id6=title...",
		"Set the disc title of WBFS discs." },

  { T_DEF_CMD,	"TOUCH",	"TOUCH",
		"wwt TOUCH id6...",
		"Set time stamps of WBFS discs." },

  { T_DEF_CMD,	"VERIFY",	"VERIFY|V",
		"wwt VERIFY id6...",
		"Verify all discs of WBFS (calculate and compare SHA1 check sums)"
		" to find bad dumps." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"FILETYPE",	"FILETYPE|FT",
		"wwt FILETYPE filename...",
		"Print a status line for each source file." },

  //
  //---------- list of all wwt options ----------

  { T_OPT_S,	"VERSION",	"V|version",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"HELP",		"h|help",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"XHELP",	"xhelp",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"WIDTH",	"width",
		0, 0 /* copy of wit */ },

  { T_OPT_GM,	"QUIET",	"q|quiet",
		0, 0 /* copy of wit */ },

  { T_OPT_GM,	"VERBOSE",	"v|verbose",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"PROGRESS",	"P|progress",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"LOGGING",	"L|logging",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"ESC",		"E|esc",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"IO",		"io",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_GMP,	"TITLES",	"T|titles",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"UTF_8",	"utf-8|utf8",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"NO_UTF_8",	"no-utf-8|no-utf8|noutf8",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"LANG",		"lang",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_GM,	"TEST",		"t|test",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"AUTO",		"a|auto",
		0,
		"Search for WBFS partitions using '/proc/partitions'"
		" or searching hard disks in '/dev/'." },

  { T_OPT_C,	"ALL",		"A|all",
		0,
		"Use all WBFS partitions found." },

  { T_OPT_CMP,	"PART",		"p|part",
		"part",
		"Define a primary WBFS partition. Multiple usage possible." },

  { T_OPT_CMP,	"RECURSE",	"r|recurse",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"RDEPTH",	"rdepth",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"PSEL",		"psel",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"RAW",		"raw",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CMP,	"INCLUDE",	"n|include",
		0, 0 /* copy of wit */ },

  { T_OPT_CMP,	"INCLUDE_PATH",	"N|include-path|includepath",
		0, 0 /* copy of wit */ },

  { T_OPT_CMP,	"EXCLUDE",	"x|exclude",
		0, 0 /* copy of wit */ },

  { T_OPT_CMP,	"EXCLUDE_PATH",	"X|exclude-path|excludepath",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"IGNORE",	"i|ignore",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"IGNORE_FST",	"ignore-fst|ignorefst",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { H_OPT_G,	"HOOK",		"hook",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"ENC",		"enc",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"REGION",	"region",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"IOS",		"ios",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"ID",		"id",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"NAME",		"name",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MODIFY",	"modify",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"INODE",	"inode",
		0,
		"Print information for all inodes (invalid discs too)." },

  { T_OPT_CP,	"DEST",		"d|dest",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"DEST2",	"D|DEST",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"SPLIT",	"z|split",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SPLIT_SIZE",	"Z|split-size|splitsize",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"TRUNC",	"trunc",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"FAST",		"F|fast",
		0, "Enables fast writing (disables searching for blocks with zeroed data)." },

  { T_OPT_CP,	"CHUNK_MODE",	"chunk-mode|chunkmode|chm",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"CHUNK_SIZE",	"chunk-size|chunksize|chz",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MAX_CHUNKS",	"max-chunks|maxchunks|mch",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SIZE",		"s|size",
		"size", "Floating point size. Factors: bckKmMgGtT, default=G." },

  { T_OPT_CP,	"HSS",		"hss|sector-size|sectorsize",
		"size", "Define HD sector size, default=512. Factors: kKmMgGtT" },

  { T_OPT_CP,	"WSS",		"wss",
		"size", "Define WBFS sector size, no default. Factors: kKmMgGtT" },

  { T_OPT_C,	"RECOVER",	"recover",
		0, "Format a WBFS in recover mode." },

  { T_OPT_C,	"FORCE",	"f|force",
		0, "Force operation." },

  { T_OPT_C,	"NO_CHECK",	"no-check|nocheck",
		0, "Disable automatic check of WBFS before modificastions." },

  { T_OPT_CP,	"REPAIR",	"repair",
		"mode",
		TEXT_WWT_OPT_REPAIR },

  { T_OPT_C,	"NO_FREE",	"no-free|nofree",
		0,
		"The discs are only dropped (slot is marked free),"
		" but the correspondent blocks are not freed."
		" You should run CHECK or REPAIR to repair the WBFS"
		" after using this option." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"UPDATE",	"u|update",
		0, "Copy only non existing discs." },

  { T_OPT_C,	"SYNC",		"y|sync",
		0,
		"Remove and copy discs until the destination WBFS"
		" contains exactly the same discs as all sources together." },

  { T_OPT_C,	"NEWER",	"e|newer|new",
		0, "If source and dest have valid mtimes: copy if source is newer." },

  { T_OPT_C,	"OVERWRITE",	"o|overwrite",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"REMOVE",	"R|remove",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"WDF",		"W|wdf",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"ISO",		"I|iso",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"CISO",		"C|ciso",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"WBFS",		"B|wbfs",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"ITIME",	"itime",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"MTIME",	"mtime",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"CTIME",	"ctime",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"ATIME",	"atime",
		0, 0 /* copy of wit */ },

  { T_OPT_CMP,	"TIME",		"time",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SET_TIME",	"set-time|settime",
		"time", "Use given time instead of current time." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CM,	"LONG",		"l|long",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"MIXED",	"M|mixed",
		0, "Print disc infos of all WBFS in one combined table." },

  { T_OPT_C,	"UNIQUE",	"U|unique",
		0, "Eliminate multiple entries with same values." },

  { T_OPT_C,	"NO_HEADER",	"H|no-header|noheader",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"SECTIONS",	"sections",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SORT",		"S|sort",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"LIMIT",	"limit",
		0, 0 /* copy of wit */ },


  //
  //---------- wwt GROUP TITLES ----------

  { T_GRP_BEG,	"TITLES",	0,0,0 },

  { T_COPT_M,	"TITLES",	0,0,0 },
  { T_COPT,	"UTF_8",	0,0,0 },
  { T_COPT,	"NO_UTF_8",	0,0,0 },
  { T_COPT,	"LANG",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wwt GROUP READ_WBFS ----------

  { T_GRP_BEG,	"READ_WBFS",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPT,	"ALL",		0,0,0 },
  { T_COPT_M,	"PART",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wwt GROUP MOD_WBFS ----------

  { T_GRP_BEG,	"MOD_WBFS",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPT,	"ALL",		0,0,0 },
  { T_COPT_M,	"PART",		0,0,0 },
  { T_COPT,	"FORCE",	0,0,
	"If the automatic check finds problematic errors,"
	" then the report is printed but the operation is *not* canceled." },
  { T_COPT,	"NO_CHECK",	0,0,
	"Disable automatic check of WBFS before modifications." },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wwt GROUP EXCLUDE ----------

  { T_GRP_BEG,	"EXCLUDE",	0,0,0 },

  { T_COPT_M,	"INCLUDE",	0,0,0 },
  { T_COPT_M,	"INCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wwt GROUP IGN_EXCLUDE ----------

  { T_GRP_BEG,	"IGN_EXCLUDE",	0,0,0 },

  { T_COPT_M,	"INCLUDE",	0,0,0 },
  { T_COPT_M,	"INCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,0 },
  { T_COPT,	"IGNORE_FST",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wwt GROUP VERBOSE ----------

  { T_GRP_BEG,	"VERBOSE",	0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"VERBOSE",	0,0,
	"Show a runtime summary for each job."
	" If set twice enable progress information."
	" If set three times the progress information is more detailed." },
  { T_COPT,	"PROGRESS",	0,0,0 },

  //---------- wwt GROUP XTIME ----------

  { T_GRP_BEG,	"XTIME",	0,0,0 },

  { T_COPT,	"ITIME",	0,0,0 },
  { T_COPT,	"MTIME",	0,0,0 },
  { T_COPT,	"CTIME",	0,0,0 },
  { T_COPT,	"ATIME",	0,0,0 },

  //---------- wwt GROUP TIME ----------

  { T_GRP_BEG,	"TIME",		0,0,0 },

  { T_COPY_GRP,	"XTIME",	0,0,0 },
  { T_COPT_M,	"TIME",		0,0,0 },

  //---------- wwt GROUP PATCH ----------

  { T_GRP_BEG,	"PATCH",	0,0,0 },

  { H_COPT,	"HOOK",		0,0,0 },
  { T_COPT,	"ID",		0,0,0 },
  { T_COPT,	"NAME",		0,0,0 },
  { T_COPT,	"MODIFY",	0,0,0 },
  { T_COPT,	"REGION",	0,0,0 },
  { T_COPT,	"IOS",		0,0,0 },
  { T_COPT,	"ENC",		0,0,0 },

  //---------- wwt GROUP SPLIT_CHUNK ----------

  { T_GRP_BEG,	"SPLIT_CHUNK",	0,0,0 },

  { T_COPT,	"SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT_SIZE",	0,0,0 },
  { T_COPT,	"TRUNC",	0,0,0 },
  { T_COPT,	"CHUNK_MODE",	0,0,0 },
  { T_COPT,	"CHUNK_SIZE",	0,0,0 },
  { T_COPT,	"MAX_CHUNKS",	0,0,0 },


  //
  //---------- COMMAND wwt VERSION ----------

  { T_CMD_BEG,	"VERSION",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"LONG",		0,0,0 },

  //---------- COMMAND wwt HELP ----------

  { T_CMD_BEG,	"HELP",		0,0,0 },

  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wwt TEST ----------

  { T_CMD_BEG,	"TEST",		0,0,0 },

  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wwt ERROR ----------

  { T_CMD_BEG,	"ERROR",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print extended message instead of error name." },

  //---------- COMMAND wwt EXCLUDE ----------

  { T_CMD_BEG,	"EXCLUDE",	0,0,0 },

  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },

  //---------- COMMAND wwt TITLES ----------

  { T_CMD_BEG,	"TITLES",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },

  //---------- COMMAND wwt FIND ----------

  { T_CMD_BEG,	"FIND",		0,0,0 },

  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,
	"Be absoulte quiet and report the find status as exit code only." },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"Without @--long@ only partition names of WBFS partitions are printed."
	" If option @--long@ is set then additional infos are printed for"
	" each partition, WBFS or not."
	" If option @--long@ is set at least twice the real path is printed." },

  //---------- COMMAND wwt SPACE ----------

  { T_CMD_BEG,	"SPACE",	0,0,0 },

  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	" If option @--long@ is set the real path is printed." },

  //---------- COMMAND wwt ANALYZE ----------

  { T_CMD_BEG,	"ANALYZE",	0,0,0 },

  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"If option @--long@ is set then calculated values"
	" are printed too if other values are available."
	" If option @--long@ is set twice calculated values are always printed." },

  //---------- COMMAND wwt DUMP ----------

  { T_CMD_BEG,	"DUMP",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPT,	"INODE",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set then print a status for each valid disc within WBFS."
	" If set twice print a memory map for each shown disc too."
	" If set three times print an additional memory map for the whole WBFS."
	" If set four times activate {--inode} too." },

  //---------- COMMAND wwt ID6 ----------

  { T_CMD_BEG,	"ID6",		0,0,0 },

  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },

  { T_COPT,	"UNIQUE",	0,0,0 },
  { T_COPT,	"SORT",		0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Prefix each printed ID6 with the WBFS filename." },

  //---------- COMMAND wwt LIST ----------

  { T_CMD_BEG,	"LIST",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },

  { T_COPT,	"MIXED",	0,0,0 },
  { T_COPT,	"UNIQUE",	0,0,0 },
  { T_COPT,	"SORT",		0,0,0 },
  { T_COPY_GRP,	"TIME",		0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set the size in MiB and the region is printed too."
	" If set twice at least on time columns is added." },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

  //---------- COMMAND wwt LIST-L ----------

  { T_CMD_BEG,	"LIST_L",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt LIST-LL ----------

  { T_CMD_BEG,	"LIST_LL",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt LIST-A ----------

  { T_CMD_BEG,	"LIST_A",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt LIST-M ----------

  { T_CMD_BEG,	"LIST_M",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt LIST-U ----------

  { T_CMD_BEG,	"LIST_U",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt FORMAT ----------

  { T_CMD_BEG,	"FORMAT",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },

  { T_COPT,	"VERBOSE",	0,0,
	"Be verbose and explain the actions." },
  { T_COPT,	"SIZE",		0,0,0 },
  { T_COPT,	"SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT_SIZE",	0,0,0 },
  { T_COPT,	"HSS",		0,0,0 },
  { T_COPT,	"WSS",		0,0,0 },
  { T_COPT,	"RECOVER",	0,0,0 },
  { T_COPT,	"INODE",	0,0,
	"Force creating inode infos with predefined timestamps."
	" The timestamps reduce effect of sparce files."
	" This option is set for devices automatically." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,
	"Force test mode and ignore {--force}." },
  { T_COPT,	"FORCE",	0,0,
	"This option is needed for leaving test mode and for formatting!" },

  //---------- COMMAND wwt RECOVER ----------

  { T_CMD_BEG,	"RECOVER",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPT,	"TEST",		0,0,0 },

  //---------- GROUP wwt CHECK ----------

  { T_GRP_BEG,	"CHECK",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"VERBOSE",	0,0,
	"Be verbose."
	" If set once then more detaiuls are printed."
	" If set twice a info dump of all coruppted discs is included."
	" If set three times a info dump of all discs is included if a error is found."
	" If set four times a full memory mal is included." },

  { T_COPT_M,	"LONG",		0,0,
	"Option @--long@ does the same as option {--verbose}."
	" If set at last once it overwrites the {--verbose} level." },

  //---------- COMMAND wwt CHECK ----------

  { T_CMD_BEG,	"CHECK",	0,0,0 },

  { T_COPY_GRP,	"CHECK",	0,0,0 },
  { T_COPT,	"REPAIR",	0,0,
	TEXT_WWT_OPT_REPAIR " The default is 'NONE'." },

  { T_SEP_OPT,	0,0,0,0 },
   
  { T_COPT,	"TEST",		0,0,0 },

  //---------- COMMAND wwt REPAIR ----------

  { T_CMD_BEG,	"REPAIR",	0,0,0 },

  { T_COPY_GRP,	"CHECK",	0,0,0 },
  { T_COPT,	"REPAIR",	0,0,
	TEXT_WWT_OPT_REPAIR " The default is 'STANDARD' (FBT,INODES)." },

  { T_SEP_OPT,	0,0,0,0 },
   
  { T_COPT,	"TEST",		0,0,0 },

  //---------- COMMAND wwt EDIT ----------

  { T_CMD_BEG,	"EDIT",		0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPT,	"PART",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },
	
  { T_COPT,	"TEST",		0,0,
	"Force test mode and ignore {--force}." },
  { T_COPT,	"FORCE",	0,0,
	"This option is needed for leaving test mode and for formatting!" },

  //---------- COMMAND wwt PHANTOM ----------

  { T_CMD_BEG,	"PHANTOM",	0,0,0 },

  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"VERBOSE",	0,0,
	"Print a status line for each added disc." },
  { T_COPT,	"TEST",		0,0,0 },

  //---------- COMMAND wwt TRUNCATE ----------

  { T_CMD_BEG,	"TRUNCATE",	0,0,0 },

  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"TEST",		0,0,0 },

  //---------- COMMAND wwt SYNC ----------

  { T_CMD_BEG,	"SYNC",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },

  { T_COPT_M,	"RECURSE",	0,0,0 },
  { T_COPT,	"RDEPTH",	0,0,0 },
  { T_COPY_GRP,	"IGN_EXCLUDE",	0,0,0 },
  { T_COPY_GRP,	"VERBOSE",	0,0,0 },
  { T_COPT,	"LOGGING",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"PSEL",		0,0,0 },
  { T_COPT,	"RAW",		0,0,0 },
  { T_COPY_GRP,	"PATCH",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"REMOVE",	0,0,0 },
  { T_COPT,	"TRUNC",	0,0,
	"Truncate WBFS until operation finished." },
  { T_COPT,	"NEWER",	0,0,0 },

  //---------- COMMAND wwt UPDATE ----------

  { T_CMD_BEG,	"UPDATE",	0,0,0 },

  { T_COPY_CMD,	"SYNC",		0,0,0 },
  { T_COPT,	"SYNC",		0,0,0 },

  //---------- COMMAND wwt ADD ----------

  { T_CMD_BEG,	"ADD",		0,0,0 },

  { T_COPY_CMD,	"UPDATE",	0,0,0 },
  { T_COPT,	"UPDATE",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,
	"Overwrite already existing discs." },

  //---------- COMMAND wwt EXTRACT ----------

  { T_CMD_BEG,	"EXTRACT",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },
  { T_COPY_GRP,	"VERBOSE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPY_GRP,	"SPLIT_CHUNK",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"WDF",		0,0,0 },
  { T_COPT,	"ISO",		0,0,0 },
  { T_COPT,	"CISO",		0,0,0 },
  { T_COPT,	"WBFS",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"UNIQUE",	0,0,
	"Eliminate multiple ID6 from the source list." },
  { T_COPT,	"IGNORE",	0,0,
	"Ignore non existing discs without any warning." },
  { T_COPT,	"REMOVE",	0,0,0 },
  { T_COPT,	"UPDATE",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },
  { T_COPT,	"TRUNC",	0,0,0 },
  { T_COPT,	"FAST",		0,0,0 },

  //---------- COMMAND wwt REMOVE ----------

  { T_CMD_BEG,	"REMOVE",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },
  { T_COPY_GRP,	"VERBOSE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"UNIQUE",	0,0,
	"Eliminate multiple ID6 from the source list." },
  { T_COPT,	"IGNORE",	0,0,
	"Ignore non existing discs without any warning." },
  { T_COPT,	"NO_FREE",	0,0,0 },

  //---------- COMMAND wwt RENAME ----------

  { T_CMD_BEG,	"RENAME",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,
	"Ignore non existing discs without any warning." },
  { T_COPT,	"ISO",		0,0,
	"Modify ID and title of the ISO image."
	" If neither @--iso@ nor {--wbfs} is set, then both are assumed as active." },
  { T_COPT,	"WBFS",		0,0,
	"Modify ID and title of the inode in the WBFS management area."
	" If neither {--iso} nor @--wbfs@ is set, then both are assumed as active." },

  //---------- COMMAND wwt SETTITLE ----------

  { T_CMD_BEG,	"SETTITLE",	0,0,0 },
  { T_COPY_CMD,	"RENAME",	0,0,0 },

  //---------- COMMAND wwt TOUCH ----------

  { T_CMD_BEG,	"TOUCH",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"UNIQUE",	0,0,
	"Eliminate multiple ID6 from the source list." },
  { T_COPT,	"IGNORE",	0,0,
	"Ignore non existing discs without any warning." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"ITIME",	0,0, "Touch the 'itime' (insertion time)." },
  { T_COPT,	"MTIME",	0,0, "Touch the 'mtime' (last modification time)." },
  { T_COPT,	"CTIME",	0,0, "Touch the 'ctime' (last status change time)." },
  { T_COPT,	"ATIME",	0,0, "Touch the 'atime' (last access time)." },
  { T_COPT,	"SET_TIME",	0,0,0 },

  //---------- COMMAND wwt VERIFY ----------

  { T_CMD_BEG,	"VERIFY",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },
  { T_COPY_GRP,	"VERBOSE",	0,0,0 },
  { T_COPT,	"LIMIT",	0,0,
	"Maximal printed errors of each partition."
	" A zero means unlimitted. The default is 10." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"PSEL",		0,0,0 },
  { T_COPT,	"RAW",		0,0,0 },
  { T_COPT,	"UNIQUE",	0,0, "Eliminate multiple ID6 from the source list." },
  { T_COPT,	"IGNORE",	0,0, "Ignore non existing discs without any warning." },
  { T_COPT,	"REMOVE",	0,0, "Remove bad discs from WBFS." },
  { T_COPT,	"NO_FREE",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"On error print an additional line to localize the exact"
	" position where the error is found."
	" If set twice a hexdump of the hash values is printed too." },

  //---------- COMMAND wwt FILETYPE ----------

  { T_CMD_BEG,	"FILETYPE",	0,0,0 },

  { T_COPT_M,	"IGNORE",	0,0,0 },
  { T_COPT,	"IGNORE_FST",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set then ID6 and split file count are printed too."
	" If set twice the region is printed too." },

  //
  ///////////////////////////////////////////////////////////////////////////
  /////////////			  TOOL wdf			/////////////
  ///////////////////////////////////////////////////////////////////////////

  { H_DEF_TOOL,	"wdf", 0,
		"wdf [options]... [+command] [options]... files...",
		"wdf is a support tool for WDF and CISO archives."
		" It convert (pack and unpack), compare"
		" and dump WDF and CISO archives."
		" The default command depends on the program file name"
		" (see command descriptions). Usual names are"
		" @wdf@, @unwdf@, @wdf-cat@, @wdf-cmp@ and @wdf-dump@"
		" (with or without minus signs)." },

  //
  //---------- list of all wdf commands ----------

  { T_DEF_CMD,	"VERSION",	"+VERSION|+V",
		"wdf +VERSION [ignored]...",
		"Print program name, version and the defaults and exit." },

  { T_DEF_CMD,	"HELP",		"+HELP|+H",
		"wdf +HELP [+command] [ignored]...",
		"Print help and exit."
		" If the first non option is a valid command name,"
		" then a help for the given command is printed." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"PACK",		"+PACK|+P",
		"wdf +PACK [option]... files...",
		"Pack sources into WDF or CISO archives."
		" This is the general default." },

  { T_DEF_CMD,	"UNPACK",	"+UNPACK|+U",
		"wdf +UNPACK [option]... files...",
		"Unpack WDF and CISO archives."
		"\n"
		"This is the default command, when the program name starts"
		" with the two letters @'un'@ in any case." },

  { T_DEF_CMD,	"CAT",		"+CAT|+A",
		"wdf +CAT [option]... files...",
		"Concatenate files and print on the standard output."
		" WDF and CISO files are extracted before printing,"
		" all other files are copied byte by byte."
		"\n"
		"This is the default command, when the program name"
		"contains the three letter @'cat'@ in any case." },

  { T_DEF_CMD,	"CMP",		"+CMP|+C|+DIFF",
		"wdf +CAT [option]... files...",
		"Compare files and unpack WDF and CISO while comparing."
		"\n"
		"The standard is to compare two source files."
		" If {--dest} or {--DEST} is set, than all source files"
		" are compared against files in the destination path with equal names."
		" If the second source file is mising then standard input"
		" (stdin) is used instead."
		"\n"
		"This is the default command, when the program name"
		"contains the letters @'cmp'@ or @'diff'@ in any case." },

  { T_DEF_CMD,	"DUMP",		"+DUMP|+D",
		"wdf +DUMP [option]... files...",
		"Dump the data structure of all archives"
		" and ignore non WDF and non CISO files."
		"\n"
		"This is the default command, when the program contains"
		" the sub string @'dump'@ in any case." },

  //
  //---------- list of all wdf options ----------

  { T_OPT_S,	"VERSION",	"V|version",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"HELP",		"h|help",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"XHELP",	"xhelp",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"WIDTH",	"width",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_G,	"QUIET",	"q|quiet",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"VERBOSE",	"v|verbose",
		0, "Be verbose -> print program name." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"CHUNK",	"chunk",
		0,
		"Print table with chunk header too." },

  { T_OPT_C,	"LONG",		"l|long",
		0,
		"Print (un)pack statistics, 1 line for each source."
		" In dump mode: Print table with chunk header too (same as {--chunk})." },

  { T_OPT_C,	"MINUS1",	"1|minus-1|minus1",
		0,
		"If set the end address is the last address of a range."
		" The standard is to print the first address"
		" that is not part of the address of a range." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"WDF",		"W|wdf",
		0,
		"Force WDF output mode if packing"
		" and set the default suffix to @'.wdf'@."
		" This is the general default." },

  { T_OPT_C,	"CISO",		"C|ciso",
		0,
		"Force CISO output mode if packing"
		" and set the default suffix to @'.ciso'@."
		"\n"
		" This is the default, when the program name contains"
		" the sub string @'ciso'@ in any case." },

  { T_OPT_C,	"WBI",		"wbi",
		0,
		"Force CISO output mode if packing"
		" and set the default suffix to @'.wbi'@."
		"\n"
		" This is the default, when the program name contains"
		" the sub string @'wbi'@ but not @'ciso'@ in any case." },

  { T_OPT_CP,	"SUFFIX",	"s|suffix",
		".suf",
		"Use suffix @'.suf'@ instead of @'.wdf'@"
		" or @'.ciso'@ for packed files." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CP,	"DEST",		"d|dest",
		"path",
		"Define a destination path (directory/file)." },

  { T_OPT_GP,	"DEST2",	"D|DEST",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"STDOUT",	"c|stdout",
		0,
		"Write to standard output (stdout)"
		" and keep (don't delete) input files."
		"\n"
		"This is the default, when the program"
		" is reading from standard input (stdin)." },

  { T_OPT_C,	"KEEP",		"k|keep",
		0,
		"Keep (don't delete) input files during (un-)packing." },

  { T_OPT_C,	"OVERWRITE",	"o|overwrite",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"PRESERVE",	"p|preserve",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"SPLIT",	"z|split",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SPLIT_SIZE",	"Z|split-size|splitsize",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"CHUNK_MODE",	"chunk-mode|chunkmode|chm",
		"mode", TEXT_OPT_CHUNK_MODE("32K") },

  { T_OPT_CP,	"CHUNK_SIZE",	"chunk-size|chunksize|chz",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MAX_CHUNKS",	"max-chunks|maxchunks|mch",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_G,	"TEST",		"t|test",
		0, 0 /* copy of wit */ },

  //
  //---------- wdf GROUP BASE ----------

  { T_GRP_BEG,	"BASE",	0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"VERBOSE",	0,0,0 },

  //---------- wdf GROUP DEST ----------

  { T_GRP_BEG,	"DEST",	0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },

  //---------- wdf GROUP SPLIT_DEST ----------

  { T_GRP_BEG,	"SPLIT_DEST",	0,0,0 },

  { T_COPY_GRP,	"DEST",		0,0,0 },
  { T_COPT,	"SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT_SIZE",	0,0,0 },

  //---------- wdf GROUP CHUNK ----------

  { T_GRP_BEG,	"CHUNK",	0,0,0 },

  { T_COPT,	"CHUNK_MODE",	0,0,0 },
  { T_COPT,	"CHUNK_SIZE",	0,0,0 },
  { T_COPT,	"MAX_CHUNKS",	0,0,0 },

  //---------- wdf GROUP CHUNK_DEST ----------

  { T_GRP_BEG,	"CHUNK_DEST",	0,0,0 },

  { T_COPY_GRP,	"SPLIT_DEST",	0,0,0 },
  { T_COPY_GRP,	"CHUNK",	0,0,0 },

  //---------- wdf GROUP FILETYPE ----------

  { T_GRP_BEG,	"FILETYPE",	0,0,0 },

  { T_COPT,	"WDF",		0,0,0 },
  { T_COPT,	"CISO",		0,0,0 },
  { T_COPT,	"WBI",		0,0,0 },
  { T_COPT,	"SUFFIX",	0,0,0 },

  //
  //---------- COMMAND wdf VERSION ----------

  { T_CMD_BEG,	"VERSION",	0,0,0 },

  { T_COPT,	"LONG",		0,0, "Print in long format." },

  //---------- COMMAND wdf HELP ----------

  { T_CMD_BEG,	"HELP",		0,0,0 },

  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wdf UNPACK ----------

  { T_CMD_BEG,	"UNPACK",	0,0,0 },

  { T_COPY_GRP,	"SPLIT_DEST",	0,0,0 },
  { T_COPT,	"STDOUT",	0,0,0 },
  { T_COPT,	"KEEP",		0,0,0 },
  { T_COPT,	"PRESERVE",	0,0,0 },

  //---------- COMMAND wdf PACK ----------

  { T_CMD_BEG,	"PACK",		0,0,0 },

  { T_COPY_CMD,	"UNPACK",	0,0,0 },
  { T_SEP_OPT,	0,0,0,0 },
  { T_COPY_GRP,	"FILETYPE",	0,0,0 },

  //---------- COMMAND wdf CAT ----------

  { T_CMD_BEG,	"CAT",		0,0,0 },

  { T_COPY_GRP,	"SPLIT_DEST",	0,0,0 },

  //---------- COMMAND wdf CMP ----------

  { T_CMD_BEG,	"CMP",		0,0,0 },

  //---------- COMMAND wdf DUMP ----------

  { T_CMD_BEG,	"DUMP",		0,0,0 },

  { T_COPY_GRP,	"DEST",		0,0,0 },
  { T_COPT,	"CHUNK",	0,0,0 },
  { T_COPT,	"LONG",		0,0, "Same as {--chunk}" },
  { T_COPT,	"MINUS1",	0,0,0 },

  //
  ///////////////////////////////////////////////////////////////////////////
  /////////////			  TOOL wdf-cat			/////////////
  ///////////////////////////////////////////////////////////////////////////

  { T_DEF_TOOL,	"wdf-cat", 0,
		"wdf-cat [option]... files...",
		"Works like the 'cat' command:"
		" Dump all file contents to standard output (stdout)"
		" and extract WDF and CISO files on the fly."
		" All other files are copied byte by byte." },

  //---------- list of all options ----------

  { T_OPT_S,	"VERSION",	"V|version",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"HELP",		"h|help",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"XHELP",	"xhelp",
		0, "Same as {--help}." },

  { T_OPT_GP,	"WIDTH",	"width",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"IO",		"io",
		0, 0 /* copy of wit */ },

  //
  ///////////////////////////////////////////////////////////////////////////
  /////////////			  TOOL wdf-dump			/////////////
  ///////////////////////////////////////////////////////////////////////////

  { T_DEF_TOOL,	"wdf-dump", 0,
		"wdf-dump [option]... files...",
		"Dump the data structure of WDF and CISO files for analysis." },

  //---------- list of all options ----------

  { T_OPT_S,	"VERSION",	"V|version",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"HELP",		"h|help",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"XHELP",	"xhelp",
		0, 0 /* copy of wdf-cat */ },

  { T_OPT_GP,	"WIDTH",	"width",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"QUIET",	"q|quiet",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"VERBOSE",	"v|verbose",
		0, "Be verbose -> print program name." },

  { T_OPT_GP,	"IO",		"io",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_G,	"CHUNK",	"c|chunk",
		0, "Print table with chunk header." },

  { T_OPT_G,	"LONG",		"l|long",
		0, "Alternative for {--chunk}: Print table with chunk header." },

  //
  ///////////////////////////////////////////////////////////////////////////
  /////////////			    table end			/////////////
  ///////////////////////////////////////////////////////////////////////////

  { T_END, 0,0,0,0 }
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////
