
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

#include <getopt.h>
#include "ui-wdf-dump.h"

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

    {	OPT_WIDTH, 0, "width",
	"width",
	"Define the width (number of columns) for help and some other"
	" messages. This option disables the automatic detection of the"
	" terminal width."
    },

    {	OPT_QUIET, 'q', "quiet",
	0,
	"Be quiet and print only error messages."
    },

    {	OPT_VERBOSE, 'v', "verbose",
	0,
	"Be verbose -> print program name."
    },

    {	OPT_IO, 0, "io",
	"flags",
	"Setup the IO mode for experiments. The standard file IO is based on"
	" open() function. The value '1' defines that WBFS IO is based on"
	" fopen() function. The value '2' defines the same for ISO files and"
	" the value '3' for both, WBFS and ISO."
    },

    {	OPT_CHUNK, 'c', "chunk",
	0,
	"Print table with chunk header."
    },

    {	OPT_LONG, 'l', "long",
	0,
	"Alternative for --chunk: Print table with chunk header."
    },

    {0,0,0,0,0} // OPT__N_TOTAL == 10

};

//
///////////////////////////////////////////////////////////////////////////////
///////////////            OptionShort & OptionLong             ///////////////
///////////////////////////////////////////////////////////////////////////////

const char OptionShort[] = "Vhqvcl";

const struct option OptionLong[] =
{
	{ "version",		0, 0, 'V' },
	{ "help",		0, 0, 'h' },
	{ "xhelp",		0, 0, GO_XHELP },
	{ "width",		1, 0, GO_WIDTH },
	{ "quiet",		0, 0, 'q' },
	{ "verbose",		0, 0, 'v' },
	{ "io",			1, 0, GO_IO },
	{ "chunk",		0, 0, 'c' },
	{ "long",		0, 0, 'l' },

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
	/*40*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*50*/	 0,0,0,0, 0,0,
	/*56*/	OPT_VERSION,
	/*57*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	/*63*/	OPT_CHUNK,
	/*64*/	 0,0,0,0, 
	/*68*/	OPT_HELP,
	/*69*/	 0,0,0,
	/*6c*/	OPT_LONG,
	/*6d*/	 0,0,0,0, 
	/*71*/	OPT_QUIET,
	/*72*/	 0,0,0,0, 
	/*76*/	OPT_VERBOSE,
	/*77*/	 0,0,0,0, 0,0,0,0, 0,
	/*80*/	OPT_XHELP,
	/*81*/	OPT_WIDTH,
	/*82*/	OPT_IO,
	/*83*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,
	/*90*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	/*a0*/	 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
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
	OptionInfo + OPT_WIDTH,
	OptionInfo + OPT_QUIET,
	OptionInfo + OPT_VERBOSE,
	OptionInfo + OPT_IO,

	OptionInfo + OPT_NONE, // separator

	OptionInfo + OPT_CHUNK,
	OptionInfo + OPT_LONG,

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
	false,
	"wdf-dump",
	0,
	"wdf-dump [option]... files...",
	"Dump the data structure of WDF and CISO files for analysis.",
	9,
	option_tab_tool
    },

    {0,0,0,0,0,0,0,0}
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     InfoUI                      ///////////////
///////////////////////////////////////////////////////////////////////////////

const InfoUI_t InfoUI =
{
	"wdf-dump",
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

