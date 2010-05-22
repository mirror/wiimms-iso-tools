
// *************************************************************************
// *****   This file is automatically generated by the tool 'gen-ui'   *****
// *************************************************************************
// *****                 ==> DO NOT EDIT THIS FILE <==                 *****
// *************************************************************************

#include <getopt.h>
#include "ui-iso2wbfs.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  OptionInfo[]                   ///////////////
///////////////////////////////////////////////////////////////////////////////

const InfoOption_t OptionInfo[OPT__N_TOTAL+1] =
{
    {0,0,0,0,0}, // OPT_NONE,

    {	OPT_VERSION, 'V', "version",
	0,
	"Stop parsing the command line, print a version info and exit."
    },

    {	OPT_HELP, 'h', "help",
	0,
	"Stop parsing the command line, print a help message and exit."
    },

    {	OPT_XHELP, 0, "xhelp",
	0,
	"Same as --help."
    },

    {	OPT_QUIET, 'q', "quiet",
	0,
	"Be quiet and print only error messages."
    },

    {	OPT_VERBOSE, 'v', "verbose",
	0,
	"Be verbose -> program name and processed files."
    },

    {	OPT_PROGRESS, 'P', "progress",
	0,
	"Print progress counter independent of verbose level."
    },

    {	OPT_IO, 0, "io",
	"flags",
	"Setup the IO mode for experiments. The standard file IO is based on"
	" open() function. The value '1' defines that WBFS IO is based on"
	" fopen() function. The value '2' defines the same for ISO files and"
	" the value '3' for both, WBFS and ISO."
    },

    {	OPT_TEST, 't', "test",
	0,
	"Run in test mode, modify nothing.\n"
	">>> USE THIS OPTION IF UNSURE! <<<"
    },

    {	OPT_PSEL, 0, "psel",
	"p-type",
	"This option set the scrubbing mode and defines, which disc partitions"
	" are handled. One of the following values is allowed, case is"
	" ignored: DATA, NO-DATA, UPDATE, NO-UPDATE, CHANNEL, NO-CHANNEL ALL,"
	" WHOLE, RAW. The default value is 'ALL'."
    },

    {	OPT_RAW, 0, "raw",
	0,
	"Abbreviation of --psel=RAW."
    },

    {	OPT_DEST, 'd', "dest",
	"path",
	"Define a destination path (directory/file). The destination path is"
	" scanned for escape sequences (see option --esc) to allow generic"
	" pathes."
    },

    {	OPT_DEST2, 'D', "DEST",
	"path",
	"Like --dest, but create the directory path automatically."
    },

    {	OPT_PRESERVE, 'p', "preserve",
	0,
	"Preserve file times (atime+mtime)."
    },

    {	OPT_SPLIT, 'z', "split",
	0,
	"Enable output file splitting, default split size = 4 GB."
    },

    {	OPT_SPLIT_SIZE, 'Z', "split-size",
	"sz",
	"Enable output file splitting and define split size. The parameter"
	" 'sz' is a floating point number followed by an optional unit factor"
	" (one of 'cb' [=1] or  'kmgtpe' [base=1000] or 'KMGTPE' [base=1024])."
	" The default unit is 'G' (GiB)."
    },

    {	OPT_OVERWRITE, 'o', "overwrite",
	0,
	"Overwrite already existing files."
    },

    {0,0,0,0,0} // OPT__N_TOTAL == 17

};

//
///////////////////////////////////////////////////////////////////////////////
///////////////            OptionShort & OptionLong             ///////////////
///////////////////////////////////////////////////////////////////////////////

const char OptionShort[] = "VhqvPtd:D:pzZ:o";

const struct option OptionLong[] =
{
	{ "version",		0, 0, 'V' },
	{ "help",		0, 0, 'h' },
	{ "xhelp",		0, 0, GO_XHELP },
	{ "quiet",		0, 0, 'q' },
	{ "verbose",		0, 0, 'v' },
	{ "progress",		0, 0, 'P' },
	{ "io",			1, 0, GO_IO },
	{ "test",		0, 0, 't' },
	{ "psel",		1, 0, GO_PSEL },
	{ "raw",		0, 0, GO_RAW },
	{ "dest",		1, 0, 'd' },
	{ "DEST",		1, 0, 'D' },
	{ "preserve",		0, 0, 'p' },
	{ "split",		0, 0, 'z' },
	{ "split-size",		1, 0, 'Z' },
	 { "splitsize",		1, 0, 'Z' },
	{ "overwrite",		0, 0, 'o' },

	{0,0,0,0}
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////            OptionUsed & OptionIndex             ///////////////
///////////////////////////////////////////////////////////////////////////////

u8 OptionUsed[OPT__N_TOTAL+1] = {0};

const u8 OptionIndex[OPT_INDEX_SIZE] = 
{
	/*00*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*10*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*20*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*30*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*40*/	 0,0,0,0, 
	/*44*/	OPT_DEST2,
	/*45*/	 0,0,0,0, 0,0,0,0, 0,0,0,
	/*50*/	OPT_PROGRESS,
	/*51*/	 0,0,0,0, 0,
	/*56*/	OPT_VERSION,
	/*57*/	 0,0,0,
	/*5a*/	OPT_SPLIT_SIZE,
	/*5b*/	 0,0,0,0, 0,0,0,0, 0,
	/*64*/	OPT_DEST,
	/*65*/	 0,0,0,
	/*68*/	OPT_HELP,
	/*69*/	 0,0,0,0, 0,0,
	/*6f*/	OPT_OVERWRITE,
	/*70*/	OPT_PRESERVE,
	/*71*/	OPT_QUIET,
	/*72*/	 0,0,
	/*74*/	OPT_TEST,
	/*75*/	 0,
	/*76*/	OPT_VERBOSE,
	/*77*/	 0,0,0,
	/*7a*/	OPT_SPLIT,
	/*7b*/	 0,0,0,0, 0,
	/*80*/	OPT_XHELP,
	/*81*/	OPT_IO,
	/*82*/	OPT_PSEL,
	/*83*/	OPT_RAW,
	/*84*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	/*90*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 InfoOption tabs                 ///////////////
///////////////////////////////////////////////////////////////////////////////

const InfoOption_t * option_tab_tool[] =
{
	OptionInfo + OPT_VERSION,
	OptionInfo + OPT_HELP,
	OptionInfo + OPT_XHELP,
	OptionInfo + OPT_QUIET,
	OptionInfo + OPT_VERBOSE,
	OptionInfo + OPT_PROGRESS,
	OptionInfo + OPT_IO,

	OptionInfo + OPT_NONE, // separator

	OptionInfo + OPT_TEST,

	OptionInfo + OPT_NONE, // separator

	OptionInfo + OPT_PSEL,
	OptionInfo + OPT_RAW,
	OptionInfo + OPT_DEST,
	OptionInfo + OPT_DEST2,
	OptionInfo + OPT_PRESERVE,
	OptionInfo + OPT_SPLIT,
	OptionInfo + OPT_SPLIT_SIZE,
	OptionInfo + OPT_OVERWRITE,

	0
};


//
///////////////////////////////////////////////////////////////////////////////
///////////////                   InfoCommand                   ///////////////
///////////////////////////////////////////////////////////////////////////////

const InfoCommand_t CommandInfo[CMD__N+1] =
{
    {	0,
	false,
	"iso2wbfs",
	0,
	"iso2wbfs [option]... files...",
	"Convert Wii ISO images into WBFS files and split at 4 GB.",
	16,
	option_tab_tool
    },

    {0,0,0,0,0,0,0}
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     InfoUI                      ///////////////
///////////////////////////////////////////////////////////////////////////////

const InfoUI_t InfoUI =
{
	"iso2wbfs",
	0, // n_cmd
	0, // cmd_tab
	CommandInfo,
	0, // n_opt_specific
	OPT__N_TOTAL,
	OptionInfo,
	OptionUsed,
	OptionIndex,
	OptionShort,
	OptionLong
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////                       END                       ///////////////
///////////////////////////////////////////////////////////////////////////////

