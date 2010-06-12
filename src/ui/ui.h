
#ifndef WIT_UI_H
#define WIT_UI_H

#include "lib-std.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  InfoOption_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct InfoOption_t
{
	int  id;
	char short_name;
	ccp  long_name;
	ccp  param;
	ccp  help;
	
} InfoOption_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  InfoCommand_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct InfoCommand_t
{
	int  id;
	bool hidden;
	bool separator;
	ccp  name1;
	ccp  name2;
	ccp  param;
	ccp  help;
	int  n_opt;
	const InfoOption_t ** opt;

} InfoCommand_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    InfoUI_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct InfoUI_t
{
	ccp tool_name;			// name of the tool

	//----- commands -----

	int n_cmd;			// == CMD__N
	const CommandTab_t * cmd_tab;	// NULL or pointer to command table
	const InfoCommand_t * cmd_info;	// pointer to 'CommandInfo[]'

	//----- options -----

	int n_opt_specific;		// == OPT__N_SPECIFIC
	int n_opt_total;		// == OPT__N_TOTAL
	const InfoOption_t * opt_info;	// pointer to 'OptionInfo[]'
	u8 * opt_used;			// pointer to 'OptionUsed[]'
	const u8 * opt_index;		// pointer to 'OptionIndex[]'
	ccp opt_short;			// pointer to 'OptionShort[]'
	const struct option * opt_long;	// pointer to 'OptionLong[]'

} InfoUI_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Interface			///////////////
///////////////////////////////////////////////////////////////////////////////

enum // some const
{
	OPT_MAX		=   100,	// max number of options
	OPT_USED_MASK	=  0x7f,	// mask to calculate usage count
	OPT_LONG_BASE	=  0x80,	// first index for "only long options"
	OPT_INDEX_SIZE	=  0xb0,	// size of OptionIndex[]
};

///////////////////////////////////////////////////////////////////////////////

enumError RegisterOption ( const InfoUI_t * iu, int option, int level, bool is_env );
enumError VerifySpecificOptions ( const InfoUI_t * iu, const CommandTab_t * cmd );
int GetOptionCount ( const InfoUI_t * iu, int option );
void DumpUsedOptions ( const InfoUI_t * iu, FILE * f, int indent );

///////////////////////////////////////////////////////////////////////////////

void PrintHelp ( const InfoUI_t * iu, FILE * f, int indent );
void PrintHelpCmd ( const InfoUI_t * iu, FILE * f, int indent, int cmd, ccp info );

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_UI_H

