
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
 *   Copyright (c) 2009-2011 by Dirk Clemens <wiimm@wiimm.de>              *
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
///////////////			  text controls			///////////////
///////////////////////////////////////////////////////////////////////////////

//	\1 : the following text is only for the built in help
//	\2 : the following text is only for the web site
//	\3 : the following text is for both, built in help and web site
//	\4 : replace by '<' for html tags on web site

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
	F_OPT_OPTPARAM	=  0x100000,  // option accepts a optional parameter
	F_SEPARATOR	=  0x200000,  // separator element
	F_SUPERSEDE	=  0x400000,  // supersedes all other commands and options

	F_OPT_XPARAM	=  F_OPT_PARAM | F_OPT_OPTPARAM,


	//----- global flags

	F_HIDDEN	= 0x1000000,  // option is hidden from help


	//----- option combinations
	
	T_OPT_C		= T_DEF_OPT | F_OPT_COMMAND,
	T_OPT_CM	= T_DEF_OPT | F_OPT_COMMAND | F_OPT_MULTIUSE,
	T_OPT_CP	= T_DEF_OPT | F_OPT_COMMAND                  | F_OPT_PARAM,
	T_OPT_CMP	= T_DEF_OPT | F_OPT_COMMAND | F_OPT_MULTIUSE | F_OPT_PARAM,
	T_OPT_CO	= T_DEF_OPT | F_OPT_COMMAND                  | F_OPT_OPTPARAM,
	T_OPT_CMO	= T_DEF_OPT | F_OPT_COMMAND | F_OPT_MULTIUSE | F_OPT_OPTPARAM,

	T_OPT_G		= T_DEF_OPT | F_OPT_GLOBAL,
	T_OPT_GM	= T_DEF_OPT | F_OPT_GLOBAL  | F_OPT_MULTIUSE,
	T_OPT_GP	= T_DEF_OPT | F_OPT_GLOBAL                   | F_OPT_PARAM,
	T_OPT_GMP	= T_DEF_OPT | F_OPT_GLOBAL  | F_OPT_MULTIUSE | F_OPT_PARAM,
	T_OPT_GO	= T_DEF_OPT | F_OPT_GLOBAL                   | F_OPT_OPTPARAM,
	T_OPT_GMO	= T_DEF_OPT | F_OPT_GLOBAL  | F_OPT_MULTIUSE | F_OPT_OPTPARAM,

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
	H_OPT_CO	= F_HIDDEN | T_OPT_CO,
	H_OPT_CMO	= F_HIDDEN | T_OPT_CMO,

	H_OPT_G		= F_HIDDEN | T_OPT_G,
	H_OPT_GM	= F_HIDDEN | T_OPT_GM,
	H_OPT_GP	= F_HIDDEN | T_OPT_GP,
	H_OPT_GMP	= F_HIDDEN | T_OPT_GMP,
	H_OPT_GO	= F_HIDDEN | T_OPT_GO,
	H_OPT_GMO	= F_HIDDEN | T_OPT_GMO,

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

	int index;		// calculated index
	
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
	"\n " \
	" All keywords can be prefixed by '+' to enable that option," \
	" by a '-' to disable it or" \
	" by a '=' to enable that option and disable all others."

#define TEXT_OPT_CHUNK_MODE(def) \
	"Defines an operation mode for {--chunk-size} and {--max-chunks}." \
	" Allowed keywords are @'ANY'@ to allow any values," \
	" @'32K'@ to force chunk sizes with a multiple of 32 KiB," \
	" @'POW2'@ to force chunk sizes >=32K and with a power of 2" \
	" or @'ISO'@ for ISO images (more restrictive as @'POW2'@," \
	" best for USB loaders)." \
	" The case of the keyword is ignored." \
	" The default key is @'" def "'@." \
	"\n " \
	" @--chm@ is a shortcut for @--chunk-mode@."

#define TEXT_EXTRACT_LONG \
	"Print a summary line while extracting files." \
	" If set at least twice, print a status line for each extracted files."

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
		" patch, mix, extract, compose, rename and compare"
		" Wii and GameCube discs."
		" It can create and dump different other Wii file formats." },

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

  { T_DEF_CMD,	"INFO",		"INFO",
		    "wit INFO [keyword]...",
		"Print some internal information about the keywords."
		" If the keyword @ALL@ is set or no keyword is entered"
		" information for all possible keywords are printed."
		" Possible keywords are: @FILE-FORMAT@ and @ALL@." },

  { T_DEF_CMD,	"TEST",		"TEST",
		    "wit TEST [ignored]...",
		"Test options: All options are allowed, some are printed." },

  { T_DEF_CMD,	"ERROR",	"ERROR|ERR",
		    "wit ERROR [error_code]",
		"Translate exit code to message or print a table"
		" with all error messages if not exit code is given." },

  { T_DEF_CMD,	"COMPR",	"COMPR",
		    "wit COMPR [mode]...",
		"Scan compression modes and print the normalized names."
		" See option {--compression} for syntax details."
		" If no mode is given than print a table"
		" with all available compression modes and alternative mode names." },

  { T_DEF_CMD,	"EXCLUDE",	"EXCLUDE",
		    "wit EXCLUDE [additional_excludes]...",
		"Dump the internal exclude database to standard output (stdout)." },

  { T_DEF_CMD,	"TITLES",	"TITLES",
		    "wit TITLES [additional_title_file]...",
		"Dump the internal title database to standard output (stdout)." },

  { T_DEF_CMD,	"CERT",		"CERT",
		    "wit CERT [additional_cert_file]...",
		"Collect certificates"
		" and eliminate multiple entires of the same certificate."
		" Dump all collected certificates to standard output (stdout)"
		" and/or write the certificate to a new binary cert file."
		" The optional parameters are handled like parameters of option {--cert}." },

  { T_DEF_CMD,	"CREATE",	"CREATE",
		    "wit CREATE TICKET outfile [--id id] [title_id] [decrypted_key]\n"
		    "wit CREATE TMD outfile [--id id] [--ios ios] [hash_val]",
		"Create a system file." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"FILELIST",	"FILELIST|FLIST",
		    "wit FILELIST [source]...",
		"List all source files in a table." },

  { T_DEF_CMD,	"FILETYPE",	"FILETYPE|FTYPE",
		    "wit FILETYPE [source]...",
		"Print a status line for each source file." },

  { T_DEF_CMD,	"ISOSIZE",	"ISOSIZE|SIZE",
		"wit ISOSIZE [source]...",
		"Print a status line with size infos for each source file." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"DUMP",		"DUMP|D",
		    "wit DUMP [source]...",
		"Dump the data structure and content of Wii and GameCube ISO files,"
		" @cert.bin@, @ticket.bin@, @tmd.bin@,"
		" @header.bin@, @boot.bin@, @fst.bin@ and of DOL-files."
		" The file type is detected automatically by analyzing the content." },

  { T_DEF_CMD,	"ID6",		"ID6|ID",
		    "wit ID6 [id]...",
		"Print ID6 of all found ISO files."
		" If the ID list is set use it as selector." },

  { T_DEF_CMD,	"LIST",		"LIST|LS",
		"wit LIST [source]...",
		"List all found ISO files." },

  { T_DEF_CMD,	"LIST_L",	"LIST-L|LL|LISTL",
		    "wit LIST-L [source]...",
		"List all found ISO files."
		" 'LIST-L' is a shortcut for {LIST --long}." },

  { T_DEF_CMD,	"LIST_LL",	"LIST-LL|LLL|LISTLL",
		    "wit LIST-LL [source]...",
		"List all found ISO files."
		" 'LIST-LL' is a shortcut for {LIST --long --long}." },

  { T_DEF_CMD,	"LIST_LLL",	"LIST-LLL|LLLL|LISTLLL",
		    "wit LIST-LLL [source]...",
		"List all found ISO files."
		" 'LIST-LLL' is a shortcut for {LIST --long --long --long}." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

	// [2do] old name 'ILIST' is obsolete since 2010-08-15

  { T_DEF_CMD,	"FILES",	"FILES|F|ILIST|IL",  
		    "wit FILES [source]...",
		"List all files of all discs." },

  { T_DEF_CMD,	"FILES_L",	"FILES-L|FL|FILESL|ILIST-L|ILL|ILISTL",
		    "wit FILES-L [source]...",
		"List all files of all discs."
		" 'FILES-L' is a shortcut for {FILES --long}." },

  { T_DEF_CMD,	"FILES_LL",	"FILES-LL|FLL|FILESLL|ILIST-LL|ILLL|ILISTLL",
		    "wit FILES-LL [source]...",
		"List all files of all discs."
		" 'FILES-LL' is a shortcut for {FILES --long --long}." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"DIFF",		"DIFF|CMP",
		    "wit DIFF source dest\n"
		    "wit DIFF [[--source] source]... [--recurse source]... [-d|-D] dest",
		"DIFF compares ISO images in scrubbed or raw mode or on file level."
		" Images, WBFS partitions and directories are accepted as source."
		" DIFF works like {COPY} but comparing source and destination." },

  { T_DEF_CMD,	"FDIFF",	"FDIFF|FCMP",
		    "wit FDIFF source dest\n"
		    "wit FDIFF [[--source] source]... [--recurse source]... [-d|-D] dest",
		"FDIFF compares ISO images on file level."
		" Images, WBFS partitions and directories are accepted as source."
		" 'FDIFF' is a shortcut for {DIFF --files +}." },

  { T_DEF_CMD,	"EXTRACT",	"EXTRACT|X",
		    "wit EXTRACT source dest\n"
		    "wit EXTRACT [[--source] source]... [--recurse source]... [-d|-D] dest",
		"Extract all files from the source discs."
		" Images, WBFS partitions and directories are accepted as source."  },

  { T_DEF_CMD,	"COPY",		"COPY|CP",
		    "wit COPY source dest\n"
		    "wit COPY [[--source] source]... [--recurse source]... [-d|-D] dest",
		"Copy, scrub, convert, join, split, compose, extract,"
		" patch, encrypt and decrypt Wii and GameCube disc images."
		" Images, WBFS partitions and directories are accepted as source." },

  { T_DEF_CMD,	"CONVERT",	"CONVERT|CV|SCRUB|SB",
		    "wit CONVERT source\n"
		    "wit CONVERT [[--source] source]... [--recurse source]...",
		"Convert, scrub, join, split, compose, extract,"
		" patch, encrypt and decrypt Wii and GameCube disc images"
		" and replace the source with the result."
		" Images, WBFS partitions and directories are accepted as source."
		" The former command name was @SCRUB@."
		"\n "
		" {wit CONVERT} is like {wit COPY} but removes the source"
		" and replace it with the new file if copying is successful."
		" It have been implemented as replacement of the @SCRUB@ command"
		" of other tools. {wit CONVERT does more than only scrubbing"
		" and therefor it was renamed from @'SCRUB'@ to @'CONVERT'@." },

  { T_DEF_CMD,	"EDIT",		"EDIT|ED",
		    "wit EDIT source\n"
		    "wit EDIT [[--source] source]... [--recurse source]...",
		"Edit an existing Wii and GameCube ISO image"
		" and patch some values."
		" Images, WBFS partitions and directories are accepted as source." },

  { T_DEF_CMD,	"MOVE",		"MOVE|MV",
		    "wit MOVE source dest\n"
		    "wit MOVE [[--source] source]... [--recurse source]... [-d|-D] dest",
		"Move and rename Wii and GameCube ISO images."
		" Images, WBFS partitions and directories are accepted as source." },

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

  { H_DEF_CMD,	"SKELETON",	"SKELETON|SKEL",
		    "wit SKELETON [source]...",
		"Create skeletons of ISO images, which are much smaller"
		" than complete copies. This skeletons contains only disc"
		" and partiton header for further analysis"
		" and are not playable because all files are zeroed."
		" If no destination directory is set with --dest or --DEST"
		" then the skeleton is stored in @'./wit-skel/'@." },

  { T_DEF_CMD,	"MIX",		"MIX",
		    "wit MIX SOURCE... --dest|--DEST outfile\n"
		    "  where SOURCE    = infile [QUALIFIER]...\n"
		    "  where QUALIFIER = 'select' part_type\n"
		    "                  | 'as' [part_table '.'] [part_type]\n"
		    "                  | 'ignore' ruleset\n"
		    "                  | 'header'\n"
		    "                  | 'region'\1\n"
		    "Read http://wit.wiimm.de/cmd/wit/mix for more details.",
		"Mix the partitions from different sources into one new"
		" Wii or GameCube disc." },

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
		"Define the width (number of columns) for help and some other messages"
		" and disable the automatic detection of the terminal width." },

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
		"This debug option enables the logging of internal memory maps."
		" If set twice second level memory maps are printed too." },

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
		" and value '4' for WIA files."
		" You can combine the values by adding them." },

  { H_OPT_G,	"DIRECT",	"direct",
		0,
		"This option allows the tools to use direct file io for some file types."
		" Therefore the flag @O_DIRECT@ is set while opening files."
		"\n"
		">>> DIRECT IO IS EXPERIMENTAL! <<<" },


  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_GMP,	"TITLES",	"T|titles",
		"file", "Read file for disc titles. -T0 disables titles lookup." },

  { T_OPT_G,	"UTF_8",	"utf-8|utf8",
		0,
		"Enables UTF-8 support for filenames (default)." },

  { T_OPT_G,	"NO_UTF_8",	"no-utf-8|no-utf8|noutf8",
		0, 
		"Disables UTF-8 support for filenames." },

  { T_OPT_GP,	"LANG",		"lang",
		"lang", 
		"Define the language for titles." },

  { T_OPT_GMP,	"CERT",		"cert",
		"file",
		"Scan a file for certificates"
		" and add them to the internal certificate database."
		" Valid sources are CERT, TICKET, TMD and ISO files."
		" All partitions of ISO images are scanned for certificates."
		" Files without certificates are ignored without notification." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_GM,	"TEST",		"t|test",
		0,
		"Run in test mode, modify nothing."
		"\n"
		">>> USE THIS OPTION IF UNSURE! <<<" },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CMP,	"SOURCE",	"s|source",
		"path",
		"Use the entered file or directory as source."
		"\n "
		" Directories are expanded to all containing files"
		" but hidden files (file names begins with a point) are ignored."
		" If a command needs only images then non image files"
		" of the directory are ignored without notification."
		" The option {--no-expand} supresses the directory expansion." },

  { T_OPT_C,	"NO_EXPAND",	"no-expand|noexpand",
		0,
		"Do not expand directories to the containing files or images."
		" This option does not change the behavior of {--recurse}." },

  { T_OPT_CMP,	"RECURSE",	"r|recurse",
		"path",
		" If @path@ is not a directory then it is used as a simple"
		" source file like {--source}."
		"\n "
		" Directories are scanned for source files recursively."
		" The option {--rdepth} limits the search depth."
		" Hidden files and hidden sub directories (file names begins"
		" with a point) and files with non supported file types (non ISO"
		" files for most commands) are ignored without notification." },

  { T_OPT_CP,	"RDEPTH",	"rdepth",
		"depth",
		"Set the maximum recurse depth for option {--recurse}."
		" The default search depth is 10." },

  { T_OPT_C,	"AUTO",		"a|auto",
		0,
		"Search WBFS partitions using '/proc/partitions'"
		" or searching hard disks in '/dev/' and use all readable as source."
		" This works like {wwt --auto --all}." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CMP,	"INCLUDE",	"n|include",
		"id",
		"A comma separated list with ID values is expected."
		" @'.'@ is a wildcard for exact 1 character and @'+'@"
		" is a wildcard for any number characters."
		" If the parameter begins with a '@@' the given file is read"
		" and each line is scanned for one ID."
		" Only images with the given ID are included into the operation."
		" Each use of this option expands the include list."
		" The exclude list takes precedence." },

  { T_OPT_CMP,	"INCLUDE_PATH",	"N|include-path|includepath",
		"file_or_dir",
		"Scan the ID of the source and add it to the include list."
		" If the source is a directory then scan all images of the directory."
		" Only images with the given ID are included into the operation."
		" Each use of this option expands the include list."
		" The exclude list takes precedence." },

  { T_OPT_CMP,	"EXCLUDE",	"x|exclude",
		"id",
		"A comma separated list with ID4 and ID6 values is expected."
		" @'.'@ is a wildcard for exact 1 character and @'+'@"
		" is a wildcard for any number characters."
		" If the parameter begins with a '@@' the given file is read"
		" and each line is scanned for one ID."
		" Images with the given ID are excluded from operation."
		" Each use of this option expands the exclude list." },

  { T_OPT_CMP,	"EXCLUDE_PATH",	"X|exclude-path|excludepath",
		"file_or_dir",
		"Scan the ID of the source and add it to the exclude list."
		" If the source is a directory then scan all images of the directory."
		" Images with the given ID are excluded from operation."
		" Each use of this option expands the exclude list." },

  { T_OPT_C,	"ONE_JOB",	"1|one-job|onejob",
		0,
		"Execute only the first job and exit."
		" This is a shortcut for {--job-limit 1}." },

  { T_OPT_CP,	"JOB_LIMIT",	"job-limit|joblimit",
		"num",
		"Execute only the first @'num'@ jobs and exit."
		" If done without errors the exit status is OK (zero)." },

  { T_OPT_CP,	"FAKE_SIGN",	"fake-sign|fakesign",
		"ruleset",
		"Add a certificate selection rule."
		" All certificates that matches the ruleset will be fake signed."
		"\1\n "
		" See http://wit.wiimm.de/info/file-filter.html"
		" for more details about filters." },

  { T_OPT_CM,	"IGNORE",	"i|ignore",
		0,
		"Ignore non existing files/discs without warning."
		" If set twice then all non Wii and GameCube ISO images are ignored too." },

  { T_OPT_C,	"IGNORE_FST",	"ignore-fst|ignorefst",
		0, 
		"Disable composing and ignore FST directories as input." },

  { T_OPT_C,	"IGNORE_SETUP",	"ignore-setup|ignoresetup",
		0, 
		"While composing ignore the file @'setup.txt'@,"
		" which defines some partition parameters." },
		
  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CP,	"PSEL",		"psel",
		"list",
		"This option set the scrubbing mode and defines,"
		" which disc partitions are handled."
		" It expects a comma separated list of keywords, numbers and names;"
		" all together called parameter. All parameters are case insensitive"
		" and non ambiguous abbreviations of keyword are allowed."
		"\n "
		" Each parameter becomes a rule and each rule is appended to a rule list."
		" Rules prefixed by a minus sign are DENY rules."
		" Rules prefixed by a plus sign or without a prefix are ALLOW rules."
		" Each partition is compared with each rule until a rule matches the partition."
		" If a match it found, the partition is enabled for a ALLOW rule"
		" or disabled for a DENY rule."
		"\n "
		" The allowed keywords are: @DATA@, @UPDATE@, @CHANNEL@,"
		" @PTAB0@ .. @PTAB3@, @ID@, @ALL@, @WHOLE@ and @RAW@."
		" The following input formats are accepted too:"
		" @ptype@, @#index@, @#<index@, @#<=index@, @#>index@, @#>=index@"
		" and @#tab_index.part_index@."
		"\1\n "
		" See http://wit.wiimm.de/opt/psel for more details." },

  { T_OPT_C,	"RAW",		"raw",
		0, "Abbreviation of {--psel RAW}." },

  { T_OPT_CP,	"PMODE",	"pmode",
		"p-mode",
		"This options set the prefix mode for listed or extracted files."
		" One of the following values is allowed:"
		" @AUTO, NONE, POINT, ID, NAME, INDEX, COMBI@."
		" The default value is @'AUTO'@."
		"\1 See http://wit.wiimm.de/opt/pmode for more details." },

  { T_OPT_C,	"SNEEK",	"sneek",
		0, "Abbreviation of {--psel data --pmode none --files :sneek}." },

  { H_OPT_G,	"HOOK",		"hook",
		0,
		"Force relocation hook while reading iso images." },

  { T_OPT_CP,	"ENC",		"enc",
		"encoding",
		"Define the encoding mode."
		" The mode is one of NONE, HASHONLY, DECRYPT, ENCRYPT, SIGN or AUTO."
		" The case of the keywords is ignored."
		" The default mode is 'AUTO'." },

  { T_OPT_CP,	"ID",		"id",
		"id",
		"This $patching$ option changes the ID of the disc"
		" to the given parameter. 1 to 6 characters are expected."
		" Only defined characters not equal '.' are modified."
		" The disc header, boot.bin, ticket.bin and tmd.bin are "
		" objects to modify. The option {--modify} selects the objects."
		"\1\n"
		"See http://wit.wiimm.de/opt/id for more details." },

  { T_OPT_CP,	"NAME",		"name",
		"name",
		"This $patching$ option changes the name (disc title) of the disc"
		" to the given parameter. Up to 63 characters are expected."
		" The disc header and boot.bin are objects to modify."
		" The option {--modify} selects the objects." },

  { T_OPT_CP,	"MODIFY",	"modify",
		"list",
		" This $patching$ option expects a comma separated list"
		" of the following keywords (case ignored) as parameter:"
		" @NONE, DISC, BOOT, TICKET, TMD, WBFS, ALL@ and @AUTO@ (default)."
		"\n "
		" All keywords can be prefixed by @'+'@ to enable that option,"
		" by a @'-'@ to disable it or"
		" by a @'='@ to enable that option and disable all others." },

  { T_OPT_CP,	"REGION",	"region",
		"region",
		"This $patching$ option defines the region of the disc."
		" The region is one of @JAPAN, USA, EUROPE, KOREA, FILE@"
		" or @AUTO@ (default). The case of the keywords is ignored."
		" Unsigned numbers are also accepted." },

  { T_OPT_CP,	"COMMON_KEY",	"common-key",
		"index",
		"This $patching$ option defines the common key index as part"
		" of the TICKET. Keywords @0@, @STANDARD@, @1@ and @KOREAN@"
		" are accepted." },

  { T_OPT_CP,	"IOS",		"ios",
		"ios",
		"This $patching$ option defines the system version (IOS to load)"
		" within TMD. The format is @'HIGH:LOW'@ or @'HIGH-LOW'@ or @'LOW'@."
		" If only @LOW@ is set than @HIGH@ is assumed as 1 (standard IOS)." },

  { T_OPT_CP,	"RM_FILES",	"rm-files|rm-file|rmfiles|rmfile",
		"ruleset",
		"This patching option defines filter rules to remove real files"
		" and directories from the FST of the DATA partition."
		" $Fake signing$ of the TMD is necessary."
		" The processing order of file options is:"
		" {--rm-files --zero-files --ignore-files}."
		"\1\n "
		" See http://wit.wiimm.de/info/file-filter.html"
		" for more details about file filters." },

  { T_OPT_CP,	"ZERO_FILES",	"zero-files|zero-file|zerofiles|zerofile",
		"ruleset",
		"This patching option defines filter rules to zero (set size to zero)"
		" real files of the FST of the DATA partition."
		" $Fake signing$ of the TMD is necessary."
		" The processing order of file options is:"
		" {--rm-files --zero-files --ignore-files}."
		"\1\n "
		" See http://wit.wiimm.de/info/file-filter.html"
		" for more details about file filters." },

  { T_OPT_C,	"OVERLAY",	"overlay",
		0,
		"Most partitions have holes (unused areas) in the data section."
		" If combining multiple partitions into one disc it is possible"
		" to overlay the partitions so that the data of one partition"
		" resides in the hole of other partitions."
		" This option enables this feature."
		" It also limits the number of input partitions to 12,"
		" because the calculation is rated as O(2\1^N\2\4sup>N\4/sup>\3)."
		" 12 partitions can be combined in 479 millions permutations and"
		" all are tested with a back tracking algorithm to find the best one." },

  { H_OPT_CP,	"REPL_FILE",	"repl-file|repl-files|replfile|replfiles",
		"filedef",
		"This relocation option ???"
		" The processing order of file options is:"
		" {--rm-files --zero-files --repl-file --add-file --ignore-files}." },

  { H_OPT_CP,	"ADD_FILE",	"add-file|add-files|addfile|addfiles",
		"filedef",
		"This relocation option ???"
		" The processing order of file options is:"
		" {--rm-files --zero-files --repl-file --add-file --ignore-files}." },

  { T_OPT_CP,	"IGNORE_FILES",	"ignore-files|ignore-file|ignorefiles|ignorefile",
		"ruleset",
		"This option defines filter rules to ignore"
		" real files of the FST of the DATA partition."
		" $Fake signing$ is not necessary, but the partition becomes invalid,"
		" because the content of some files is not copied."
		" If such file is accessed the Wii will halt immediately,"
		" because the verification of the check sum calculation fails."
		" The processing order of file options is:"
		" {--rm-files --zero-files --ignore-files}."
		"\1\n "
		" See http://wit.wiimm.de/info/file-filter.html"
		" for more details about file filters." },

  { H_OPT_CP,	"TRIM",		"trim",
		"keylist",
		"This relocation option ???" },

  { H_OPT_CP,	"ALIGN",	"align",
		"size1[,size2][,size3]",
		"???" },

  { T_OPT_CP,	"ALIGN_PART",	"align-part|alignpart",
		"size",
		"If creating or moving partitions the beginning of each partition"
		" is set to an offset that is a multiple of the align size."
		" Size must be a power of 2 and at least 32 KiB (=default)." },

  { T_OPT_CP,	"DEST",		"d|dest",
		"path",
		"Define a destination path (directory/file)."
		" The destination path is scanned for escape sequences"
		" (see option {--esc}) to allow generic paths." },

  { T_OPT_CP,	"DEST2",	"D|DEST",
		"path",
		"Like {--dest}, but create the directory path automatically." },

  { T_OPT_C,	"SPLIT",	"z|split",
		0,
		"Enable output file splitting. The default split size is 4 GB." },

  { T_OPT_CP,	"SPLIT_SIZE",	"Z|split-size|splitsize",
		"sz",
		"Enable output file splitting and define a split size."
		" The parameter 'sz' is a floating point number followed"
		" by an optional unit factor (one of 'cb' [=1] or "
		" 'kmgtpe' [base=1000] or 'KMGTPE' [base=1024])."
		" The default unit is 'G' (GiB)." },

  { T_OPT_CP,	"DISC_SIZE",	"disc-size|discsize",
		"size",
		"Define a minimal (virtual) ISO disc size." },

  { T_OPT_CO,	"PREALLOC",	"prealloc",
		"[=mode]",
		"This option enables or disables the disc space preallocation."
		" If enabled the tools try to allocate disc space for the new files"
		" before writing the data. This reduces the fragmentation but also"
		" disables the sparse effect for prealocated areas."
		"\n "
		" The optional parameter decides the preallocation mode:"
		" @OFF@ (or @0@), @SMART@ (or @1@), @ALL@ (or @2@)."
		" If no parameter is set, @ALL@ is used."
		"\n "
		" Mode @'OFF'@ disables the preallocation."
		" Mode @'SMART'@ looks into the source disc to find out the writing areas."
		" @SMART@ is only avalable for $ISO$, $CISO$ and $WBFS$ file types."
		" For other file types @ALL@ is used instead."
		" Mode @'ALL'@ (the default) preallocate the whole destination file."
		" Because of the large holes in plain ISO images,"
		" the @SMART@ mode is used for ISOs instead."
		"\n "
		" Mac ignores this option"
		" because the needed preallocation function is not avaialable." },

  { T_OPT_C,	"TRUNC",	"trunc",
		0, "Truncate a $PLAIN ISO$ images to the needed size while creating." },

  { T_OPT_CP,	"CHUNK_MODE",	"chunk-mode|chunkmode|chm",
		"mode", TEXT_OPT_CHUNK_MODE("ISO") },

  { T_OPT_CP,	"CHUNK_SIZE",	"chunk-size|chunksize|chs",
		"sz",
		"Define the minimal chunk size if creating a CISO"
		" or WIA file (for WIA details see option --compression})."
		" The default is to calculate the chunk size from the input file size"
		" and find a good value by using a minimal value of 1 MiB"
		" for {--chunk-mode ISO} and @32 KiB@ for modes @32K@ and @POW2@."
		" For the modes @ISO@ and @POW2@ the value is rounded"
		" up to the next power of 2."
		" This calculation also depends from option {--max-chunks}."
		"\n "
		" The parameter 'sz' is a floating point number followed"
		" by an optional unit factor (one of 'cb' [=1] or "
		" 'kmgtpe' [base=1000] or 'KMGTPE' [base=1024])."
		" The default unit is 'M' (MiB)."
		" If the number is prefixed with a @'='@ then options"
		" {--chunk-mode} and {--max-chunks} are ignored"
		" and the given value is used without any rounding or changing."
		"\n "
		" If the input file size is not known (e.g. reading from pipe),"
		" its size is assumed as @12 GiB@."
		"\n "
		" @--chs@ is a shortcut for @--chunk-size@." },

  { T_OPT_CP,	"MAX_CHUNKS",	"max-chunks|maxchunks|mch",
		"n",
		"Define the maximal number of chunks if creating a CISO file."
		" The default value is 8192 for {--chunk-mode ISO}"
		" and 32760 (maximal value) for all other modes."
		" If this value is set than the automatic calculation "
		" of {--chunk-size} will be modified too."
		"\n "
		" @--mch@ is a shortcut for @--max-chunks@." },

  { T_OPT_CP,	"COMPRESSION",	"compression|compr",
		"mode",
		"Select one compression method, level and chunk size for new WIA files."
		" The syntax for mode is: @[method] [.level] [@@factor]@"
		"\n "
		" @'method'@ is the name of the method."
		" Possible compressions method are @NONE@, @PURGE@, @BZIP2@,"
		" @LZMA@ and @LZMA2@."
		" There are additional keywords: @DEFAULT@ (=@LZMA.5@@20@),"
		" @FAST@ (=@BZIP2.3@@10@), @GOOD@ (=@LZMA.5@@20@) @BEST@ (=@LZMA.7@@50@),"
		" and @MEM@ (use best mode in respect to memory limit set by {--mem})."
		" Additionally the single digit modes @0@ (=@NONE@), "
		" @1@ (=fast @LZMA@) .. @9@ (=@BEST@)are defined."
		" These additional keywords may change their meanings"
		" if a new compression method is implemented."
		"\n "
		" @'.level'@ is a point followed by one digit."
		" It defines the compression level."
		" The special value @.0@ means: Use default compression level (=@.5@)."
		"\n "
		" @'@@factor'@ is a factor for the chunk size. The base size is 2 MiB."
		" The value@ @@0@ is replaced by the default factor@ @@20@ (40 MiB)."
		" If the factor is not set but option {--chunk-size} is set,"
		" the factor will be calculated by using a rounded value of that option."
		"\n "
		" All three parts are optional."
		" All default values may be changed in the future."
		" @--compr@ is a shortcut for @--compression@"
		" and {--wia=mode} a shortcut for {--wia --compression mode}."
		" The command {wit COMPR} prints an overview about all compression modes." },

  { T_OPT_CP,	"MEM",		"mem",
		"size",
		"This option defines a memory usage limit for compressing files."
		" When compressing a file with method @MEM@ (see {--compression})"
		" the the compression method, level and chunk size"
		" are selected with respect to this limit."
		"\n "
		" If this option is not set or the value is 0,"
		" then the environment @WIT_MEM@ is tried to read instead."
		" If this fails, the tool tries to find out the total memory"
		" by reading @/proc/meminfo@."
		" The limit is set to 80% of the total memory minus 50 MiB." },

  { T_OPT_C,	"PRESERVE",	"p|preserve",
		0,
		"Preserve file times (atime+mtime) while copying an image."
		" This option is enabled by default if an unmodified disc image is copied." },

  { T_OPT_C,	"UPDATE",	"u|update",
		0,
		"Copy only files that does not exist."
		" Already existing files are ignored without warning." },

  { T_OPT_C,	"OVERWRITE",	"o|overwrite",
		0,
		"Overwrite already existing files without warning." },

  { T_OPT_C,	"DIFF",		"diff",
		0,
		"Diff source and destination after copying." },

  { T_OPT_C,	"REMOVE",	"R|remove",
		0,
		"Remove source files/discs if operation is successful."
		" If the source is an extracted file systems (FST) it isn't removed." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"WDF",		"W|wdf",
		0, "Set ISO output file type to WDF (Wii Disc Format, default)" },

  { T_OPT_C,	"ISO",		"I|iso",
		0, "Set ISO output file type to PLAIN ISO." },

  { T_OPT_C,	"CISO",		"C|ciso",
		0, "Set ISO output file type to CISO (Compact ISO, same as WBI)." },

  { T_OPT_C,	"WBFS",		"B|wbfs",
		0, "Set ISO output file type to WBFS (Wii Backup File System) container." },

  { T_OPT_CO,	"WIA",		"wia",
		"[=compr]",
		"Set ISO output file type to WIA (Wii ISO Archive)."
		" The optional parameter is a compression mode and"
		" {--wia=mode} is a shortcut for {--wia --compression mode}." },

  { T_OPT_C,	"FST",		"fst",
		0,
		"Set ISO output mode to 'file system' (extracted ISO)." },

  { T_OPT_CMP,	"FILES",	"F|files",
		"ruleset",
		"Append a file select rules."
		" This option can be used multiple times to extend the rule list."
		" Rules beginning with a '+' or a '-' are allow or deny rules rules."
		" Rules beginning with a ':' are macros for predefined rule sets."
		"\1\n "
		" See http://wit.wiimm.de/info/file-filter.html"
		" for more details about file filters." },

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
		"Set time printing and sorting mode."
		" The parameter is a comma separated list of the following keywords,"
		" case is ignored:"
		" RESET, OFF, ON, SINGLE, MULTI, NONE, ALL,"
		" I, M, C, A, DATE, TIME, SEC,"
		" IDATE, MDATE, CDATE, ADATE,"
		" ITIME, MTIME, CTIME, ATIME,"
		" ISEC, MSEC, CSEC, ASEC." },

  { T_OPT_CM,	"LONG",		"l|long",
		0, "Print in long format. Multiple usage possible." },

  { H_OPT_C,	"NUMERIC",	"numeric",		// *** [2do] *** not used ***
		0, "Force numeric output instead of printing names." },

  { T_OPT_C,	"REALPATH",	"real-path|realpath",
		0, "Print real path instead of entered path." },

  { T_OPT_CP,	"SHOW",		"show",
		"list",
		"This option allows fine control over the things that are to be printed."
		" The parameter is a comma separated list of the"
		" following keywords, case is ignored: "
		" @NONE, INTRO, D-ID, P-ID, P-TAB, P-INFO, P-MAP, D-MAP, TICKET, TMD, USAGE,"
		" PATCH, RELOCATE, FILES, OFFSET, SIZE, PATH@ and @ALL@."
		" There are some combined keys:"
		" @ID := D-ID,P-ID@,"
		" @PART := P-INFO,P-ID,P-MAP,TICKET,TMD@,"
		" @MAP := P-MAP,D-MAP@."
		"\n "
		" All keywords can be prefixed by '+' to enable that option,"
		" by a '-' to disable it or"
		" by a '=' to enable that option and disable all others."
		"\n "
		" The additional keywords @DEC@ and @HEX@ can be used to set"
		" a prefered number format."
		" @-HEADER@ suppresses the output of header lines."
		"\n "
		" The commands recognize only some of these keywords"
		" and ignore the others."
		" If @--show@ is set, option {--long} is ignored"
		" for selecting output elements." },

  { T_OPT_CP,	"UNIT",		"unit",
		"list",
		"This option set the output unit for sizes."
		" The parameter is a comma separated list of the"
		" following keywords, case is ignored: "
		" @1000=10, 1024=2, BYTES, K, M, G, T, P, E,"
		" KB, MB, GB, TB, PB, EB, KIB, MIB, GIB, TIB, PIB, EIB,"
		" HDS, WDS, GAMECUBE=GC, WII, AUTO@ and @DEFAULT@."
		"\n "
		" The values @1000@ and @1024@ (=default base) set the base factor"
		" and @BYTES, K, M, G, T, P, E@ the SI factor."
		" @MB@ is a shortcut for @1000,M@ and @MIB@ for @1024,M@; "
		" this is also valid for the other SI factors."
		" @AUTO@ selects a value dependent SI factor."
		"\n "
		" @HDS@ and @WDS@ forces the output"
		" as multiple of the HD or Wii disc sector size (512 or 32768 bytes)."
		" @GAMECUBE@ and @WII@ forces the output of a floating point value"
		" as multiple of the single layer ISO images size of the given type."
		"\n "
		" @DEFAULT@ allows the command to select a adequate size unit." },
 
  { T_OPT_C,	"UNIQUE",	"U|unique",
		0, "Eliminate multiple entries with same ID6." },

  { T_OPT_C,	"NO_HEADER",	"H|no-header|noheader",
		0, "Suppress printing of header and footer." },

  { T_OPT_C,	"OLD_STYLE",	"old-style|oldstyle",
		0,
		"Print in old style."
		" This is important for tools and GUIs that are scanning the output." },

  { T_OPT_C,	"SECTIONS",	"sections",
		0,
		"Print in machine readable sections and parameter lines."
		"\1 Read http://wit.wiimm.de/opt/sections for more details." },

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

  //---------- wit GROUP FST_IGNORE ----------

  { T_GRP_BEG,	"FST_IGNORE",	0,0,0 },

  { T_COPT,	"IGNORE_FST",	0,0,0 },
  { T_COPT,	"IGNORE_SETUP",	0,0,0 },

  //---------- wit GROUP FST_SELECT ----------

  { T_GRP_BEG,	"FST_SELECT",	0,0,0 },

  { T_COPT,	"PMODE",	0,0,0 },
  { T_COPT_M,	"FILES",	0,0,0 },
  { T_COPT,	"SNEEK",	0,0,0 },

  //---------- wit GROUP SOURCE ----------

  { T_GRP_BEG,	"SOURCE",	0,0,0 },

  { T_COPT_M,	"SOURCE",	0,0,0 },
  { T_COPT,	"NO_EXPAND",	0,0,0 },
  { T_COPT_M,	"RECURSE",	0,0,0 },
  { T_COPT,	"RDEPTH",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wit GROUP EXCLUDE ----------

  { T_GRP_BEG,	"EXCLUDE",	0,0,0 },

  { T_COPT_M,	"INCLUDE",	0,0,0 },
  { T_COPT_M,	"INCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"ONE_JOB",	0,0,0 },
  { T_COPT_M,	"JOB_LIMIT",	0,0,0 },

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
  { T_COPY_GRP,	"FST_IGNORE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wit GROUP OUTMODE_EDIT ----------

  { T_GRP_BEG,	"OUTMODE_EDIT",	0,0,0 },

  { T_COPT,	"WDF",		0,0,0 },
  { T_COPT,	"ISO",		0,0,0 },
  { T_COPT,	"CISO",		0,0,0 },
  { T_COPT,	"WBFS",		0,0,0 },

  //---------- wit GROUP OUTMODE ----------

  { T_GRP_BEG,	"OUTMODE",	0,0,0 },

  { T_COPY_GRP,	"OUTMODE_EDIT",	0,0,0 },
  { T_COPT,	"WIA",		0,0,0 },

  //---------- wit GROUP OUTMODE_FST ----------

  { T_GRP_BEG,	"OUTMODE_FST",	0,0,0 },

  { T_COPY_GRP,	"OUTMODE",	0,0,0 },
  { T_COPT,	"FST",		0,0,0 },

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

  //---------- wit GROUP PATCH ----------

  { T_GRP_BEG,	"PATCH",	0,0,0 },

  { H_COPT,	"HOOK",		0,0,0 },
  { T_COPT,	"ENC",		0,0,0 },
  { T_COPT,	"ID",		0,0,0 },
  { T_COPT,	"NAME",		0,0,0 },
  { T_COPT,	"MODIFY",	0,0,0 },
  { T_COPT,	"REGION",	0,0,0 },
  { T_COPT,	"COMMON_KEY",	0,0,0 },
  { T_COPT,	"IOS",		0,0,0 },
  { T_COPT,	"RM_FILES",	0,0,0 },
  { T_COPT,	"ZERO_FILES",	0,0,0 },

  //---------- wit GROUP RELOCATE ----------

  { T_GRP_BEG,	"RELOCATE",	0,0,0 },

  { H_COPT,	"REPL_FILE",	0,0,0 },
  { H_COPT,	"ADD_FILE",	0,0,0 },
  { T_COPT,	"IGNORE_FILES",	0,0,0 },
  { H_COPT,	"TRIM",		0,0,0 },
  { H_COPT,	"ALIGN",	0,0,0 },
  { T_COPT,	"ALIGN_PART",	0,0,0 },

  //---------- wit GROUP SPLIT_CHUNK ----------

  { T_GRP_BEG,	"SPLIT_CHUNK",	0,0,0 },

  { T_COPT,	"SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT_SIZE",	0,0,0 },
  { T_COPT,	"DISC_SIZE",	0,0,0 },
  { T_COPT,	"PREALLOC",	0,0,0 },
  { T_COPT,	"TRUNC",	0,0,0 },
  { T_COPT,	"CHUNK_MODE",	0,0,0 },
  { T_COPT,	"CHUNK_SIZE",	0,0,0 },
  { T_COPT,	"MAX_CHUNKS",	0,0,0 },
  { T_COPT,	"COMPRESSION",	0,0,0 },
  { T_COPT,	"MEM",		0,0,0 },

  //
  //---------- COMMAND wit VERSION ----------

  { T_CMD_BEG,	"VERSION",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print in long format. Ignored if option {--sections} is set." },

  //---------- COMMAND wit HELP ----------

  { T_CMD_BEG,	"HELP",		0,0,0 },

  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wit INFO ----------

  { T_CMD_BEG,	"INFO",		0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },

  //---------- COMMAND wit TEST ----------

  { T_CMD_BEG,	"TEST",		0,0,0 },

  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wit ERROR ----------

  { T_CMD_BEG,	"ERROR",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print extended message instead of error name." },

  //---------- COMMAND wit COMPR ----------

  { T_CMD_BEG,	"COMPR",	0,0,0 },

  { T_COPT,	"MEM",		0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print a table with the normalized mode name,"
	" compression level, chunk size factor and memory usage." },
  { T_COPT,	"VERBOSE",	0,0,
	"Print always compression level and chunk size factor."
	" Standard is to suppress these values if not explicitly set." },
  { H_COPT,	"NUMERIC",	0,0,0 },

  //---------- COMMAND wit EXCLUDE ----------

  { T_CMD_BEG,	"EXCLUDE",	0,0,0 },

  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },

  //---------- COMMAND wit TITLES ----------

  { T_CMD_BEG,	"TITLES",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },

  //---------- COMMAND wit CERT ----------

  { T_CMD_BEG,	"CERT",		0,0,0 },

  { T_COPT_M,	"CERT",		0,0,0 },
  { T_COPT_M,	"FILES",	0,0,
	"Filter the certificates by rules."
	" Therefor the certificate name is build in the form 'issuer.keyid'."
	"\1\n "
	" See http://wit.wiimm.de/info/file-filter.html"
	" for more details about filters." },
  { T_COPT_M,	"FAKE_SIGN",	0,0,0 },
  { T_COPT,	"DEST",		0,0,
	"Define a destination file."
	" All selected certificates are written to this new created file." },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"VERBOSE",	0,0,
	"Dump the content of all certificates to standard output."
	" This is the default if neiter --dest nor --DEST are set." },


  //---------- COMMAND wit FILELIST ----------

  { T_CMD_BEG,	"FILELIST",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print the real path instead of given path." },

  //---------- COMMAND wit FILETYPE ----------

  { T_CMD_BEG,	"FILETYPE",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set at least once or twide additional columns with ID6 (1x)"
	" or ther region (2x) are enabled."
	" If set three or more times the real path instead of given path"
	" is printed." },

  //---------- COMMAND wit ISOSIZE ----------

  { T_CMD_BEG,	"ISOSIZE",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set the size is printed in MiB too."
	" If set twice two columns with WBFS calculations are added."
	" If set three times the real path of the source is printed." },
  { T_COPT_M,	"UNIT",		0,0,0 },

  //---------- COMMAND wit CREATE ----------

  { T_CMD_BEG,	"CREATE",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },
  { T_COPT,	"DEST",		0,0,
	"Define a destination path (directory/file)."
	" This path is concatenated with the @outfile@." },
  { T_COPT,	"DEST2",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"ID",		0,0,
	"Define an ID for the TICKET or TMD." },
  { T_COPT,	"IOS",		0,0,
	"Define an IOS/SYS-VERSION for the TMD." },

  //---------- COMMAND wit DUMP ----------

  { T_CMD_BEG,	"DUMP",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPY_GRP,	"FST_IGNORE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FST_SELECT",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set at least once a memory map for each partition is printed."
	" If set twice or more a memory map for whole ISO image is printed." },
  { T_COPT,	"SHOW",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPY_GRP,	"RELOCATE",	0,0,0 },
  { T_COPT,	"DISC_SIZE",	0,0,0 },

  //---------- COMMAND wit ID6 ----------

  { T_CMD_BEG,	"ID6",		0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPY_GRP,	"FST_IGNORE",	0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set, the disc name of the title db is printed too." },

  //---------- COMMAND wit LIST ----------

  { T_CMD_BEG,	"LIST",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPY_GRP,	"FST_IGNORE",	0,0,0 },

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
  { T_COPT,	"REALPATH",	0,0,0 },
  { T_COPT_M,	"UNIT",		0,0,0 },
  { T_COPT,	"PROGRESS",	0,0,0 },

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

  //---------- COMMAND wit FILES ----------

  { T_CMD_BEG,	"FILES",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FST_SELECT",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"SHOW",		0,0,0 },
  { T_COPT,	"SORT",		0,0,
	"Define the sort mode for the file listing."
	" The parameter is a comma separated list of the following keywords:"
	" NONE, NAME, SIZE, OFFSET, ASCENDING, DESCENDING = REVERSE." },

  //---------- COMMAND wit FILES-L ----------

  { T_CMD_BEG,	"FILES_L",	0,0,0 },
  { T_COPY_CMD,	"FILES",	0,0,0 },

  //---------- COMMAND wit FILES-LL ----------

  { T_CMD_BEG,	"FILES_LL",	0,0,0 },
  { T_COPY_CMD,	"FILES",	0,0,0 },

  //---------- COMMAND wit DIFF ----------

  { T_CMD_BEG,	"DIFF",		0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FST_SELECT",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"PROGRESS",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPY_GRP,	"OUTMODE_FST",	0,0,0 },

  //---------- COMMAND wit FDIFF ----------

  { T_CMD_BEG,	"FDIFF",	0,0,0 },
  { T_COPY_CMD,	"DIFF",		0,0,0 },

  //---------- COMMAND wit EXTRACT ----------

  { T_CMD_BEG,	"EXTRACT",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FST_SELECT",	0,0,0 },
  { T_COPT,	"PREALLOC",	0,0,0 },
  { T_COPT,	"SORT",		0,0,
	"Define the extracting order."
	" The parameter is a comma separated list of the following keywords:"
	" NONE, NAME, SIZE, OFFSET, ASCENDING, DESCENDING = REVERSE." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0, TEXT_EXTRACT_LONG },
  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"PROGRESS",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPY_GRP,	"RELOCATE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPT,	"PRESERVE",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },


  //---------- COMMAND wit COPY ----------

  { T_CMD_BEG,	"COPY",		0,0,0 },

  { T_COPY_CMD,	"EXTRACT",	0,0,0 },

  { T_COPT,	"UPDATE",	0,0,0 },
  { T_COPT,	"DIFF",		0,0,0 },
  { T_COPT,	"REMOVE",	0,0,0 },
  { T_COPY_GRP,	"SPLIT_CHUNK",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"OUTMODE_FST",	0,0,0 },


  //---------- COMMAND wit CONVERT ----------

  { T_CMD_BEG,	"CONVERT",	0,0,0 },

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
  { T_COPT,	"SECTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPY_GRP,	"RELOCATE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"SPLIT_CHUNK",	0,0,0 },
  { T_COPT,	"PRESERVE",	0,0,0 },
  { T_COPY_GRP,	"OUTMODE",	0,0,0 },


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
  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"PRESERVE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"PATCH",	0,0,0 },


  //---------- COMMAND wit MOVE ----------

  { T_CMD_BEG,	"MOVE",		0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

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
  { T_COPT,	"AUTO",		0,0,0 },
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
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPT,	"IGNORE_FILES",	0,0,0 },

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

  //---------- COMMAND wit SKELETON ----------

  { T_CMD_BEG,	"SKELETON",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },
  { T_COPT_M,	"QUIET",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"OUTMODE_EDIT",	0,0,0 },
  { T_COPT,	"FST",		0,0,0 },
  { T_COPT_M,	"DEST",		0,0,0 },
  { T_COPT_M,	"DEST2",	0,0,0 },

  //---------- COMMAND wit MIX ----------

  { T_CMD_BEG,	"MIX",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },
  { T_COPY_GRP,	"SPLIT_CHUNK",	0,0,0 },
  { T_COPT,	"ALIGN_PART",	0,0,
	"The beginning of each partition is set to an offset that is"
	" a multiple of the align size."
	" Size must be a power of 2 and at least 32 KiB (=default)."
	" If option {--overlay} is set only the first partition is aligned." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"OUTMODE_EDIT",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"ID",		0,0,
	"Define an ID for the disc header."
	" The default is 'WIT000'." },
  { T_COPT,	"NAME",		0,0,
	"Define a disc title for the disc header."
	" The default is a generic title based on all sources." },
  { T_COPT,	"REGION",	0,0,0 },
  { T_COPT,	"OVERLAY",	0,0,0 },

  //
  ///////////////////////////////////////////////////////////////////////////
  /////////////			    TOOL wwt			/////////////
  ///////////////////////////////////////////////////////////////////////////

  { T_DEF_TOOL,	"wwt", 0,
		"wwt [option]... command [option|parameter|@file]...",
		"Wiimms WBFS Tool (WBFS manager) :"
		" It can create, check, repair, verify and clone WBFS files"
		" and partitions. It can list, add, extract, remove, rename"
		" and recover ISO images as part of a WBFS." },

  //
  //---------- list of all wwt commands ----------

  { T_DEF_CMD,	"VERSION",	"VERSION",
		    "wwt VERSION [ignored]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"HELP",		"HELP|H|?",
		    "wwt HELP [command] [ignored]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"INFO",		"INFO",
		    "wit INFO [keyword]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"TEST",		"TEST",
		    "wwt TEST [ignored]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"ERROR",	"ERROR|ERR",
		    "wwt ERROR [error_code]",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"COMPR",	"COMPR",
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
		    "wwt ID6 [id]...",
		"List all ID6 of all discs of WBFS partitions."
		" If the ID list is set use it as selector."},

  { T_DEF_CMD,	"LIST",		"LIST|LS",
		    "wwt LIST [wbfs_partition]...",
		"List all discs of WBFS partitions." },

  { T_DEF_CMD,	"LIST_L",	"LIST-L|LL|LISTL",
		    "wwt LIST-L [wbfs_partition]...",
		"List all discs of WBFS partitions."
		" 'LIST-L' is a shortcut for {LIST --long}." },

  { T_DEF_CMD,	"LIST_LL",	"LIST-LL|LLL|LISTLL",
		    "wwt LIST-LL [wbfs_partition]...",
		"List all discs of WBFS partitions."
		" 'LIST-LL' is a shortcut for {LIST --long --long}." },

  { T_DEF_CMD,	"LIST_A",	"LIST-A|LA|LISTA",
		    "wwt LIST-A [wbfs_partition]...",
		"List all discs of all WBFS partitions."
		" 'LIST-A' is a shortcut for {LIST --long --long --auto}." },

  { T_DEF_CMD,	"LIST_M",	"LIST-M|LM|LISTM",
		    "wwt LIST-M [wbfs_partition]...",
		"List all discs of WBFS partitions in mixed view."
		" 'LIST-M' is a shortcut for {LIST --long --long --mixed}." },

  { T_DEF_CMD,	"LIST_U",	"LIST-U|LU|LISTU",
		    "wwt LIST-U [wbfs_partition]...",
		"List all discs of WBFS partitions in mixed view."
		" 'LIST-U' is a shortcut for {LIST --long --long --unique}." },

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
		    "wwt ADD [[--source] source]... [--recurse source]...",
		"Add Wii and GameCube ISO discs to WBFS partitions."
		" Images, WBFS partitions and directories are accepted as source." },

  { T_DEF_CMD,	"UPDATE",	"UPDATE|U",
		    "wwt UPDATE [[--source] source]... [--recurse source]...",
		"Add missing Wii and GameCube ISO discs to WBFS partitions."
		" Images, WBFS partitions and directories are accepted as source."
		" 'UPDATE' is a shortcut for {ADD --update}." },

  { T_DEF_CMD,	"SYNC",		"SYNC",
		    "wwt SYNC [[--source] source]... [--recurse source]...",
		"Modify primary WBFS (REMOVE and ADD)"
		" until it contains exactly the same discs as all sources together."
		" Images, WBFS partitions and directories are accepted as source."
		" 'SYNC' is a shortcut for {ADD --sync}." },

  { T_DEF_CMD,	"EXTRACT",	"EXTRACT|X",
		    "wwt EXTRACT id6[=dest]...",
		"Extract discs from WBFS partitions and store them"
		" as Wii or GameCube images." },

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
		    "wwt VERIFY [id6]...",
		"Verify all discs of WBFS (calculate and compare SHA1 check sums)"
		" to find bad dumps." },

  { H_DEF_CMD,	"SKELETON",	"SKELETON|SKEL",
		    "wit SKELETON [id6]...",
		"Create skeletons of ISO images, which are much smaller"
		" than complete copies. This skeletons contains only disc"
		" and partiton header for further analysis"
		" and are not playable because all files are zeroed."
		" If no destination directory is set with --dest or --DEST"
		" then the skeleton is stored in @'./wit-skel/'@." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"FILETYPE",	"FILETYPE|FTYPE",
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

  { H_OPT_G,	"DIRECT",	"direct",
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

  { T_OPT_CMP,	"SOURCE",	"source",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"NO_EXPAND",	"no-expand|noexpand",
		0, 0 /* copy of wit */ },

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

  { T_OPT_C,	"ONE_JOB",	"1|one-job|onejob",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"JOB_LIMIT",	"job-limit|joblimit",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"IGNORE",	"i|ignore",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"IGNORE_FST",	"ignore-fst|ignorefst",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"IGNORE_SETUP",	"ignore-setup|ignoresetup",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CP,	"PMODE",	"pmode",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"SNEEK",	"sneek",
		0, 0 /* copy of wit */ },

  { H_OPT_G,	"HOOK",		"hook",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"ENC",		"enc",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"ID",		"id",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"NAME",		"name",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MODIFY",	"modify",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"REGION",	"region",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"COMMON_KEY",	"common-key",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"IOS",		"ios",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"RM_FILES",	"rm-files|rm-file|rmfiles|rmfile",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"ZERO_FILES",	"zero-files|zero-file|zerofiles|zerofile",
		0, 0 /* copy of wit */ },

  { H_OPT_CP,	"REPL_FILE",	"repl-file|repl-files|replfile|replfiles",
		0, 0 /* copy of wit */ },

  { H_OPT_CP,	"ADD_FILE",	"add-file|add-files|addfile|addfiles",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"IGNORE_FILES",	"ignore-files|ignore-file|ignorefiles|ignorefile",
		0, 0 /* copy of wit */ },

  { H_OPT_CP,	"TRIM",		"trim",
		0, 0 /* copy of wit */ },

  { H_OPT_CP,	"ALIGN",	"align",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"ALIGN_PART",	"part-align",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"DEST",		"d|dest",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"DEST2",	"D|DEST",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"SPLIT",	"z|split",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SPLIT_SIZE",	"Z|split-size|splitsize",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"DISC_SIZE",	"disc-size|discsize",
		0, 0 /* copy of wit */ },

  { T_OPT_CO,	"PREALLOC",	"prealloc",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"TRUNC",	"trunc",
		0, 0 /* copy of wit */ },

  // [2do] [obsolte] hide after 2011-02, remove after 2011-07
  { T_OPT_C,	"FAST",		"fast",
		0,
		"Enables fast writing (disables searching for blocks with zeroed data)."
		" Don't use this option because it will be discontinued." },

  { T_OPT_CP,	"CHUNK_MODE",	"chunk-mode|chunkmode|chm",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"CHUNK_SIZE",	"chunk-size|chunksize|chs",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MAX_CHUNKS",	"max-chunks|maxchunks|mch",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"COMPRESSION",	"compression|compr",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MEM",		"mem",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SIZE",		"s|size",
		"size",
		"Define then total size of a WBFS file."
		" @'size'@ is a floating number optionally followed"
		" by one of the single letter factors 'kKmMgGtT'."
		" This value is only used while creating a new WBFS file." },

  { T_OPT_CP,	"HSS",		"hss|sector-size|sectorsize",
		"size",
		"Define HD sector size."
		" The parameter 'size' is a floating point number followed"
		" by an optional unit factor (one of 'cb' [=1] or "
		" 'kmgtpe' [base=1000] or 'KMGTPE' [base=1024])."
		" Only power of 2 values larger or equal 512 are accepted."
		" The default value is 512."  },

  { T_OPT_CP,	"WSS",		"wss",
		"size",
		"Define WBFS sector size."
		" The parameter 'size' is a floating point number followed"
		" by an optional unit factor (one of 'cb' [=1] or "
		" 'kmgtpe' [base=1000] or 'KMGTPE' [base=1024])."
		" Only power of 2 values larger or equal 1024 are accepted."
		" If not set the WBFS sector size is calculated automatically." },

  { T_OPT_C,	"RECOVER",	"recover",
		0,
		"Format a WBFS in recover mode: "
		" Write the WBFS sector, but don't reset the disc info area."
		" Then look into each disc slot to find valid discs and restore them." },

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
		"Synchronize the destination with all sources:"
		" Remove and copy discs until the destination WBFS"
		" contains exactly the same discs as all sources together." },

  { T_OPT_C,	"NEWER",	"e|newer|new",
		0,
		"If source and destination have valid mtimes:"
		" Copy only if source is newer." },

  { T_OPT_C,	"OVERWRITE",	"o|overwrite",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"REMOVE",	"R|remove",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"WDF",		"W|wdf",
		0, 0 /* copy of wit */ },

  { T_OPT_CO,	"WIA",		"wia",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"ISO",		"I|iso",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"CISO",		"C|ciso",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"WBFS",		"B|wbfs",
		0, 0 /* copy of wit */ },

  { T_OPT_CO,	"FST",		"fst",
		0, 0 /* copy of wit */ },

  { T_OPT_CO,	"FILES",	"files",
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

  { H_OPT_C,	"NUMERIC",	"numeric",		// *** [2do] *** not used ***
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"INODE",	"inode",
		0,
		"Print information for all inodes (invalid discs too)." },

  { T_OPT_C,	"MIXED",	"M|mixed",
		0, "Print disc infos of all WBFS in one combined table." },

  { T_OPT_C,	"UNIQUE",	"U|unique",
		0, "Eliminate multiple entries with same values." },

  { T_OPT_C,	"NO_HEADER",	"H|no-header|noheader",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"OLD_STYLE",	"old-style|oldstyle",
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

  //---------- wwt GROUP FST_IGNORE ----------

  { T_GRP_BEG,	"FST_IGNORE",	0,0,0 },

  { T_COPT,	"IGNORE_FST",	0,0,0 },
  { T_COPT,	"IGNORE_SETUP",	0,0,0 },

  //---------- wwt GROUP EXCLUDE ----------

  { T_GRP_BEG,	"EXCLUDE",	0,0,0 },

  { T_COPT_M,	"INCLUDE",	0,0,0 },
  { T_COPT_M,	"INCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"ONE_JOB",	0,0,0 },
  { T_COPT_M,	"JOB_LIMIT",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wwt GROUP IGN_EXCLUDE ----------

  { T_GRP_BEG,	"IGN_EXCLUDE",	0,0,0 },

  { T_COPT_M,	"INCLUDE",	0,0,0 },
  { T_COPT_M,	"INCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"ONE_JOB",	0,0,0 },
  { T_COPT_M,	"JOB_LIMIT",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,0 },
  { T_COPY_GRP,	"FST_IGNORE",	0,0,0 },

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

  //---------- wwt GROUP OUTMODE_EDIT ----------

  { T_GRP_BEG,	"OUTMODE_EDIT",	0,0,0 },

  { T_COPT,	"WDF",		0,0,0 },
  { T_COPT,	"ISO",		0,0,0 },
  { T_COPT,	"CISO",		0,0,0 },
  { T_COPT,	"WBFS",		0,0,0 },

  //---------- wwt GROUP OUTMODE ----------

  { T_GRP_BEG,	"OUTMODE",	0,0,0 },

  { T_COPY_GRP,	"OUTMODE_EDIT",	0,0,0 },
  { T_COPT,	"WIA",		0,0,0 },
  { T_COPT,	"FST",		0,0,0 },

  //---------- wwt GROUP PARTITIONS ----------

  { T_GRP_BEG,	"PARTITIONS",	0,0,0 },

  { T_COPT,	"PSEL",		0,0,0 },
  { T_COPT,	"RAW",		0,0,0 },

  //---------- wwt GROUP FST_SELECT ----------

  { T_GRP_BEG,	"FST_SELECT",	0,0,0 },

  { T_COPT,	"PMODE",	0,0,0 },
  { T_COPT_M,	"FILES",	0,0,0 },
  { T_COPT,	"SNEEK",	0,0,0 },

  //---------- wwt GROUP PATCH ----------

  { T_GRP_BEG,	"PATCH",	0,0,0 },

  { H_COPT,	"HOOK",		0,0,0 },
  { T_COPT,	"ENC",		0,0,0 },
  { T_COPT,	"ID",		0,0,0 },
  { T_COPT,	"NAME",		0,0,0 },
  { T_COPT,	"MODIFY",	0,0,0 },
  { T_COPT,	"REGION",	0,0,0 },
  { T_COPT,	"COMMON_KEY",	0,0,0 },
  { T_COPT,	"IOS",		0,0,0 },
  { T_COPT,	"RM_FILES",	0,0,0 },
  { T_COPT,	"ZERO_FILES",	0,0,0 },

  //---------- wwt GROUP RELOCATE ----------

  { T_GRP_BEG,	"RELOCATE",	0,0,0 },

  { H_COPT,	"REPL_FILE",	0,0,0 },
  { H_COPT,	"ADD_FILE",	0,0,0 },
  { T_COPT,	"IGNORE_FILES",	0,0,0 },
  { H_COPT,	"TRIM",		0,0,0 },
  { H_COPT,	"ALIGN",	0,0,0 },
  { T_COPT,	"ALIGN_PART",	0,0,0 },

  //---------- wwt GROUP SPLIT_CHUNK ----------

  { T_GRP_BEG,	"SPLIT_CHUNK",	0,0,0 },

  { T_COPT,	"SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT_SIZE",	0,0,0 },
  { T_COPT,	"DISC_SIZE",	0,0,0 },
  { T_COPT,	"PREALLOC",	0,0,0 },
  { T_COPT,	"TRUNC",	0,0,0 },
  { T_COPT,	"CHUNK_MODE",	0,0,0 },
  { T_COPT,	"CHUNK_SIZE",	0,0,0 },
  { T_COPT,	"MAX_CHUNKS",	0,0,0 },
  { H_COPT,	"COMPRESSION",	0,0,0 },
  { H_COPT,	"MEM",		0,0,0 },


  //
  //---------- COMMAND wwt VERSION ----------

  { T_CMD_BEG,	"VERSION",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"LONG",		0,0,0 },

  //---------- COMMAND wwt HELP ----------

  { T_CMD_BEG,	"HELP",		0,0,0 },

  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wwt INFO ----------

  { T_CMD_BEG,	"INFO",		0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },

  //---------- COMMAND wwt TEST ----------

  { T_CMD_BEG,	"TEST",		0,0,0 },

  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wwt ERROR ----------

  { T_CMD_BEG,	"ERROR",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print extended message instead of error name." },

  //---------- COMMAND wwt COMPR ----------

  { T_CMD_BEG,	"COMPR",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print a table with the normalized mode name,"
	" compression level, chunk size factor and memory usage." },
  { T_COPT,	"VERBOSE",	0,0,
	"Print always compression level and chunk size factor."
	" Standard is to suppress these values if not explicitly set." },
  { H_COPT,	"NUMERIC",	0,0,0 },

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
	" each partition, WBFS or not (this includes output via --sections)."
	" If option @--long@ is set at least twice"
	" the real path and the size in bytes are printed." },
  { T_COPT,	"OLD_STYLE",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

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
	"This option is needed for leaving test mode and for real formatting!" },

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
	" If set once then more details are printed."
	" If set twice a info dump of all coruppted discs is included."
	" If set three times a info dump of all discs is included if a error is found."
	" If set four times a full memory map is included." },

  { T_COPT_M,	"LONG",		0,0,
	"Option @--long@ does the same as option {--verbose}."
	" If set at least once it overwrites the {--verbose} level." },

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

  { T_COPT_M,	"SOURCE",	0,0,0 },
  { T_COPT,	"NO_EXPAND",	0,0,0 },
  { T_COPT_M,	"RECURSE",	0,0,0 },
  { T_COPT,	"RDEPTH",	0,0,0 },
  { T_COPY_GRP,	"IGN_EXCLUDE",	0,0,0 },
  { T_COPY_GRP,	"VERBOSE",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"LOGGING",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPY_GRP,	"RELOCATE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },

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
  { T_COPT_M,	"LONG",		0,0, TEXT_EXTRACT_LONG },
  { T_COPT,	"SECTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPY_GRP,	"SPLIT_CHUNK",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"OUTMODE",	0,0,0 },
  { T_COPY_GRP,	"FST_SELECT",	0,0,0 },

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
  { T_COPT,	"SECTIONS",	0,0,0 },

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
  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"LIMIT",	0,0,
	"Maximal printed errors of each partition."
	" A zero means unlimitted. The default is 10." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPT,	"IGNORE_FILES",	0,0,0 },
  { T_COPT,	"UNIQUE",	0,0, "Eliminate multiple ID6 from the source list." },
  { T_COPT,	"IGNORE",	0,0, "Ignore non existing discs without any warning." },
  { T_COPT,	"REMOVE",	0,0, "Remove bad discs from WBFS." },
  { T_COPT,	"NO_FREE",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"On error print an additional line to localize the exact"
	" position where the error is found."
	" If set twice a hexdump of the hash values is printed too." },


  //---------- COMMAND wwt SKELETON ----------

  { T_CMD_BEG,	"SKELETON",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0, "Ignore non existing discs without any warning." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"OUTMODE_EDIT",	0,0,0 },
  { T_COPT,	"FST",		0,0,0 },
  { T_COPT_M,	"DEST",		0,0,0 },
  { T_COPT_M,	"DEST2",	0,0,0 },


  //---------- COMMAND wwt FILETYPE ----------

  { T_CMD_BEG,	"FILETYPE",	0,0,0 },

  { T_COPT_M,	"IGNORE",	0,0,0 },
  { T_COPY_GRP,	"FST_IGNORE",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set then ID6 and split file count are printed too."
	" If set twice the region is printed too." },

  //
  ///////////////////////////////////////////////////////////////////////////
  /////////////			  TOOL wdf			/////////////
  ///////////////////////////////////////////////////////////////////////////

  { T_DEF_TOOL,	"wdf", 0,
		"wdf [options]... [+command] [options]... files...",
		"wdf is a support tool for WDF, WIA and CISO archives."
		" It convert (pack and unpack), compare"
		" and dump WDF, WIA (dump and cat only) and CISO archives."
		" The default command depends on the program file name"
		" (see command descriptions). Usual names are"
		" @wdf@, @unwdf@, @wdf-cat@, @wdf-cmp@ and @wdf-dump@"
		" (with or without minus signs)."
		"\n "
		" {wdf +CAT} replaces the old tool @wdf-cat@"
		" and {wdf +DUMP} the old tool @wdf-dump@." },

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
		"\n "
		" This is the default command, when the program name"
		" starts with the two letters @'un'@ in any case." },

  { T_DEF_CMD,	"CAT",		"+CAT|+C",
		    "wdf +CAT [option]... files...",
		"Concatenate files and print on the standard output."
		" WDF and CISO files are extracted before printing,"
		" all other files are copied byte by byte."
		"\n "
		" This is the default command, when the program name"
		" contains the sub string @'cat'@ in any case."
		" {wdf +CAT} replaces the old tool @wdf-cat@." },

  { H_DEF_CMD,	"CMP",		"+DIFF|+CMP",
		    "wdf +DIFF [option]... files...",
		"Compare files and unpack WDF and CISO while comparing."
		"\n "
		" The standard is to compare two source files."
		" If {--dest} or {--DEST} is set, than all source files"
		" are compared against files in the destination path with equal names."
		" If the second source file is mising then standard input"
		" (stdin) is used instead."
		"\n "
		" This is the default command, when the program name"
		" contains the sub string @'diff'@ or @'cmp'@ in any case." },

  { T_DEF_CMD,	"DUMP",		"+DUMP|+D",
		    "wdf +DUMP [option]... files...",
		"Dump the data structure of WDF, WIA and CISO archives"
		" and ignore other files."
		"\n "
		" This is the default command, when the program"
		" contains the sub string @'dump'@ in any case."
		" {wdf +DUMP} replaces the old tool @wdf-dump@." },

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

  { T_OPT_GM,	"LOGGING",	"L|logging",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"IO",		"io",
		0, 0 /* copy of wit */ },

  { H_OPT_G,	"DIRECT",	"direct",
		0, 0 /* copy of wit */ },

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

  { T_OPT_CO,	"WIA",		"wia",
		"[=compr]",
		"Force WIA output mode if packing"
		" and set the default suffix to @'.wia'@."
		" The optional parameter is a compression mode and"
		" {--wia=mode} is a shortcut for {--wia --compression mode}."
		"\n "
		" WIA output is the default, when the program name contains"
		" the sub string @'wia'@ in any case." },

  { T_OPT_C,	"CISO",		"C|ciso",
		0,
		"Force CISO output mode if packing"
		" and set the default suffix to @'.ciso'@."
		"\n "
		"  This is the default, when the program name contains"
		" the sub string @'ciso'@ in any case." },

  { T_OPT_C,	"WBI",		"wbi",
		0,
		"Force CISO output mode if packing"
		" and set the default suffix to @'.wbi'@."
		"\n "
		"  This is the default, when the program name contains"
		" the sub string @'wbi'@ but not @'ciso'@ in any case." },

  { T_OPT_CP,	"SUFFIX",	"s|suffix",
		".suf",
		"Use suffix @'.suf'@ instead of @'.wdf'@, @'.wia'@,"
		" or @'.ciso'@ for packed files." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CP,	"DEST",		"d|dest",
		"path",
		"Define a destination path (directory/file)." },

  { T_OPT_CP,	"DEST2",	"D|DEST",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"STDOUT",	"c|stdout",
		0,
		"Write to standard output (stdout)"
		" and keep (don't delete) input files."
		"\n "
		" This is the default, when the program"
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

  { T_OPT_CO,	"PREALLOC",	"prealloc",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"CHUNK_MODE",	"chunk-mode|chunkmode|chm",
		"mode", TEXT_OPT_CHUNK_MODE("32K") },

  { T_OPT_CP,	"CHUNK_SIZE",	"chunk-size|chunksize|chs",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MAX_CHUNKS",	"max-chunks|maxchunks|mch",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"COMPRESSION",	"compression|compr",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MEM",		"mem",
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

  //---------- wdf GROUP DEST_PLUS ----------

  { T_GRP_BEG,	"DEST_PLUS",	0,0,0 },

  { T_COPY_GRP,	"DEST",		0,0,0 },
  { T_COPT,	"SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT_SIZE",	0,0,0 },
  { T_COPT,	"PREALLOC",	0,0,0 },
  { T_COPT,	"CHUNK_MODE",	0,0,0 },
  { T_COPT,	"CHUNK_SIZE",	0,0,0 },
  { T_COPT,	"MAX_CHUNKS",	0,0,0 },
  { T_COPT,	"COMPRESSION",	0,0,0 },
  { T_COPT,	"MEM",		0,0,0 },

  //---------- wdf GROUP FILETYPE ----------

  { T_GRP_BEG,	"FILETYPE",	0,0,0 },

  { T_COPT,	"WDF",		0,0,0 },
  { T_COPT,	"WIA",		0,0,0 },
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

  { T_COPY_GRP,	"DEST_PLUS",	0,0,0 },
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

  { T_COPY_GRP,	"DEST",		0,0,0 },

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
  /////////////			    table end			/////////////
  ///////////////////////////////////////////////////////////////////////////

  { T_END, 0,0,0,0 }
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////
