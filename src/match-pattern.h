
#ifndef WIT_MATCH_PATTERN_H
#define WIT_MATCH_PATTERN_H 1

#include "types.h"

///////////////////////////////////////////////////////////////////////////////

typedef struct FilePattern_t
{
	StringField_t rules;	// rules db

	bool is_active;		// true if at least one pattern set
	bool is_dirty;		// true if setup is needed
	bool match_all;		// true if all files allowed
	bool match_none;	// true if no files allowed

} FilePattern_t;

///////////////////////////////////////////////////////////////////////////////

typedef enum enumPattern
{
	PAT_OPT_FILES,	// ruleset of option --files
	PAT_DEFAULT,	// default ruleset if PAT_OPT_FILES is empty

	PAT__N,		// number of patterns

} enumPattern;

extern FilePattern_t file_pattern[PAT__N];

///////////////////////////////////////////////////////////////////////////////
// pattern db

void InitializeAllFilePattern();
int  AddFilePattern ( ccp arg, int pattern_index );
FilePattern_t * GetDefaultFilePattern();
bool SetupFilePattern ( FilePattern_t * pat );
bool MatchFilePattern ( FilePattern_t * pat, ccp text );

///////////////////////////////////////////////////////////////////////////////
// low level match pattern function

bool MatchPattern ( ccp pattern, ccp text );

///////////////////////////////////////////////////////////////////////////////

#endif // WIT_MATCH_PATTERN_H

