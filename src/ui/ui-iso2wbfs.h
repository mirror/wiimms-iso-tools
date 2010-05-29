
#ifndef WIT_UI_ISO2WBFS_H
#define WIT_UI_ISO2WBFS_H

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

	OPT_VERSION,
	OPT_HELP,
	OPT_XHELP,
	OPT_QUIET,
	OPT_VERBOSE,
	OPT_PROGRESS,
	OPT_IO,
	OPT_TEST,
	OPT_PSEL,
	OPT_RAW,
	OPT_DEST,
	OPT_DEST2,
	OPT_PRESERVE,
	OPT_SPLIT,
	OPT_SPLIT_SIZE,
	OPT_OVERWRITE,

	OPT__N_TOTAL // == 17

} enumOptions;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                enum enumCommands                ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumCommands
{
	CMD__NONE,
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
	GO_IO			= 0x81,
	GO_TEST			= 't',
	GO_PSEL			= 0x82,
	GO_RAW			= 0x83,
	GO_DEST			= 'd',
	GO_DEST2		= 'D',
	GO_PRESERVE		= 'p',
	GO_SPLIT		= 'z',
	GO_SPLIT_SIZE		= 'Z',
	GO_OVERWRITE		= 'o',

	GO__ERR			= '?'

} enumGetOpt;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  external vars                  ///////////////
///////////////////////////////////////////////////////////////////////////////

extern const InfoOption_t OptionInfo[OPT__N_TOTAL+1];
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

#endif // WIT_UI_ISO2WBFS_H
