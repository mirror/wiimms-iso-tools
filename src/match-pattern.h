
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

#ifndef WIT_MATCH_PATTERN_H
#define WIT_MATCH_PATTERN_H 1

#include "types.h"
#include "wiidisc.h"

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

int MatchFilePatternFST
(
	struct wd_iterator_t *it	// iterator struct with all infos
);

///////////////////////////////////////////////////////////////////////////////
// low level match pattern function

bool MatchPattern ( ccp pattern, ccp text );

///////////////////////////////////////////////////////////////////////////////

#endif // WIT_MATCH_PATTERN_H

