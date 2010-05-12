
#define _GNU_SOURCE 1

#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "lib-std.h"
#include "match-pattern.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    variables                    ///////////////
///////////////////////////////////////////////////////////////////////////////

FilePattern_t file_pattern[PAT__N];

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    pattern db                   ///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeAllFilePattern()
{
    memset(file_pattern,0,sizeof(file_pattern));

    FilePattern_t * pat = file_pattern;
    FilePattern_t * end = pat + PAT__N;
    for ( ; pat < end; pat++ )
    {
	InitializeStringField(&pat->rules);
	pat->is_dirty	= false;
	pat->is_active	= false;
	pat->match_none	= false;
	pat->match_all	= true;
    }
}

///////////////////////////////////////////////////////////////////////////////

struct macro_tab_t
{
    int len;
    ccp name;
    ccp expand;
};

static const struct macro_tab_t macro_tab[] =
{
    { 4, "base",	"+/*$" },
    { 6, "nobase",	"-/*$" },
    { 4, "disc",	"+/disc/" },
    { 6, "nodisc",	"-/disc/" },
    { 3, "sys",		"+/sys/" },
    { 5, "nosys",	"-/sys/" },
    { 5, "files",	"+/files/" },
    { 7, "nofiles",	"-/files/" },
    { 3, "wit",		"2+/h3.bin;1+/sys/fst.bin;+" },
    { 3, "wwt",		"2+/h3.bin;1+/sys/fst.bin;+" },
    { 5, "sneek",	"2+/h3.bin;1+/disc/;+" },

    {0,0,0}
};

///////////////////////////////////////////////////////////////////////////////

int AddFilePattern ( ccp arg, int pattern_index )
{
    TRACE("AddFilePattern(%s,%d)\n",arg,pattern_index);
    ASSERT( pattern_index >= 0 );
    ASSERT( pattern_index < PAT__N );
    
    if ( !arg || (u32)pattern_index >= PAT__N )
	return 0;

    FilePattern_t * pat = file_pattern + pattern_index;
   
    pat->is_active = true;

    while (*arg)
    {
	ccp start = arg;
	bool ok = false;
	if ( *arg >= '1' && *arg <= '9' )
	{
	    while ( *arg >= '0' && *arg <= '9' )
		arg++;
	    if ( *arg == '+' || *arg == '-' )
		ok = true;
	    else
		arg = start;
	}

	if ( !ok && *arg != '+' && *arg != '-' && *arg != '=' )
	    return ERROR0(ERR_SYNTAX,
		"File pattern rule must begin with '+', '-' or '=' => %.20s\n",arg);

	while ( *arg && *arg != ';' )
	    arg++;

	if ( *start == '=' )
	{
	    const size_t len = arg - ++start;
	    const struct macro_tab_t *tab;
	    for ( tab = macro_tab; tab->len; tab++ )
		if ( tab->len == len && !memcmp(start,tab->name,len) )
		{
		    AddFilePattern(tab->expand,pattern_index);
		    break;
		}
	    if (!tab->len)
		return ERROR0(ERR_SYNTAX,
			"Macro '%.*s' not found: =%.20s\n",len,start,start);
	}
	else
	{
	    const size_t len = arg - start;
	    char * pattern = malloc(len+1);
	    memcpy(pattern,start,len);
	    pattern[len] = 0;
	    TRACE(" - ADD |%s|\n",pattern);
	    AppendStringField(&pat->rules,pattern,true);
	    pat->is_dirty = true;
	}
	
	while ( *arg == ';' )
	    arg++;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

FilePattern_t * GetDefaultFilePattern()
{
    FilePattern_t * pat = file_pattern + PAT_OPT_FILES;
    if (!pat->rules.used)
	pat = file_pattern + PAT_DEFAULT;
    return pat;
}

///////////////////////////////////////////////////////////////////////////////

bool SetupFilePattern ( FilePattern_t * pat )
{
    ASSERT(pat);
    if (pat->is_dirty)
    {
	pat->is_active	= true;
	pat->is_dirty	= false;
	pat->match_all	= false;
	pat->match_none	= false;

	if (!pat->rules.used)
	    pat->match_all = true;
	else
	{
	    ccp first = *pat->rules.field;
	    ASSERT(first);
	    if (   !strcmp(first,"+")
		|| !strcmp(first,"+*")
		|| !strcmp(first,"+**") )
	    {
		pat->match_all = true;
	    }
	    else if (   !strcmp(first,"-")
		     || !strcmp(first,"-*")
		     || !strcmp(first,"-**") )
	    {
		pat->match_none = true;
	    }
	}
     #ifdef DEBUG
	TRACE("FILE PATTERN: N=%u, all=%d, none=%d\n",
		pat->rules.used, pat->match_all, pat->match_none );
	
	ccp * ptr = pat->rules.field;
	ccp * end = ptr +  pat->rules.used;
	while ( ptr < end )
	    TRACE("  |%s|\n",*ptr++);
     #endif
    }

    return pat->is_active && !pat->match_none;
}

///////////////////////////////////////////////////////////////////////////////

bool MatchFilePattern ( FilePattern_t * pat, ccp text )
{
    if (!pat)
	pat = GetDefaultFilePattern(); // use default pattern if not set

    if (pat->is_dirty)
	SetupFilePattern(pat);
    if (pat->match_all)
	return true;
    if (pat->match_none)
	return false;

    bool default_result = true;
    int skip = 0;

    ccp * ptr = pat->rules.field;
    ccp * end = ptr +  pat->rules.used;
    while ( ptr < end )
    {
	char * pattern = (char*)*ptr++;
	switch (*pattern++)
	{
	    case '-':
		if ( skip-- <= 0 && MatchPattern(pattern,text) )
		    return false;
		default_result = true;
		break;

	    case '+':
		if ( skip-- <= 0 && MatchPattern(pattern,text) )
		    return true;
		default_result = false;
		break;

	    default:
		if ( skip-- <= 0 )
		{
		    pattern--;
		    ulong num = strtoul(pattern,&pattern,10);
		    switch (*pattern++)
		    {
			case '-':
			    if (!MatchPattern(pattern,text))
				skip = num;
			    break;

			case '+':
			    if (MatchPattern(pattern,text))
				skip = num;
			    break;
		    }
		}
		break;
	}
    }

    return default_result;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 MatchPattern()                  ///////////////
///////////////////////////////////////////////////////////////////////////////

static ccp AnalyseBrackets
	( ccp pattern, ccp * p_start, bool * p_negate, int * p_multiple )
{
    ASSERT(pattern);

    bool negate = false;
    if ( *pattern == '^' )
    {
	pattern++;
	negate = true;
    }
    if (p_negate)
	*p_negate = negate;

    int multiple = 0;
    if ( *pattern == '+' )
    {
	pattern++;
	multiple = 1;
    }
    else if ( *pattern == '*' )
    {
	pattern++;
	multiple = 2;
    }
    if (p_multiple)
	*p_multiple = multiple;

    if (p_start)
	*p_start = pattern;

    if (*pattern) // ']' allowed in first position
	pattern++;
    while ( *pattern && *pattern++ != ']' ) // find end
	;

    return pattern;
}

//-----------------------------------------------------------------------------

static bool MatchBracktes ( char ch, ccp pattern, bool negate )
{
    if (!ch)
	return false;

    bool ok = false;
    ccp p = pattern;
    for (; !ok && *p && ( p == pattern || *p != ']' ); p++ )
    {
	if ( *p == '-' )
	{
	    if ( ch <= *++p && ch >= p[-2] )
	    {
		if (negate)
		    return false;
		ok = true;
	    }
	}
	else
	{
	    if ( *p == '\\' )
		p++;

	    if ( *p == ch )
	    {
		if (negate)
		    return false;
		ok = true;
	    }
	}
    }
    return ok || negate;
}

//-----------------------------------------------------------------------------

static bool MatchPatternHelper ( ccp pattern, ccp text, bool skip_end, int alt_depth )
{
    ASSERT(pattern);
    ASSERT(text);
    noTRACE(" - %d,%d |%s|%s|\n",skip_end,alt_depth,pattern,text);

    char ch;
    while ( ( ch = *pattern++ ) != 0 )
    {
	switch (ch)
	{
	   case '*':
		if ( *pattern == '*' )
		{
		    pattern++;
		    if (*pattern)
			while (!MatchPatternHelper(pattern,text,skip_end,alt_depth))
			    if (!*text++)
				return false;
		}
		else
		{
		    while (!MatchPatternHelper(pattern,text,skip_end,alt_depth))
			if ( *text == '/' || !*text++ )
			    return false;
		}
		return true;

	    case '#':
	 	if ( *text < '0' || *text > '9' )
		    return false;
		while ( *text >= '0' && *text <= '9' )
			if (MatchPatternHelper(pattern,++text,skip_end,alt_depth))
			    return true;
		return false;

	    case ' ':
		if ( *text < 1 || * text > ' ' )
		    return false;
		text++;
		break;

	    case '?':
		if ( !*text || *text == '/' )
		    return false;
		text++;
		break;

	    case '[':
		{
		    ccp start;
		    bool negate;
		    int multiple;
		    TRACELINE;
		    pattern = AnalyseBrackets(pattern,&start,&negate,&multiple);
		    TRACELINE;

		    if ( multiple < 2 && !MatchBracktes(*text++,start,negate) )
			return false;

		    if (multiple)
		    {
			while (!MatchPatternHelper(pattern,text,skip_end,alt_depth))
			    if (!MatchBracktes(*text++,start,negate))
				return false;
			return true;
		    }
		}
		break;

	   case '{':
		for (;;)
		{
		    if (MatchPatternHelper(pattern,text,skip_end,alt_depth+1))
			return true;
		    // skip until next ',' || '}'
		    int skip_depth = 1;
		    while ( skip_depth > 0 )
		    {
			ch = *pattern++;
			switch(ch)
			{
			    case 0:
				return false;

			    case '\\':
				if (!*pattern)
				    return false;
				pattern++;
				break;

			    case '{':
				skip_depth++;
				break;

			    case ',':
				if ( skip_depth == 1 )
				    skip_depth--;
				break;

			    case '}':
				if (!--skip_depth)
				    return false;
				break;

			    case '[': // [2do]
				pattern = AnalyseBrackets(pattern,0,0,0);
				break;
			}
		    }
		}

	   case ',':
		if (alt_depth)
		{
		    alt_depth--;
		    int skip_depth = 1;
		    while ( skip_depth > 0 )
		    {
			ch = *pattern++;
			switch(ch)
			{
			    case 0:
				return false;

			    case '\\':
				if (!*pattern)
				    return false;
				pattern++;
				break;

			    case '{':
				skip_depth++;
				break;

			    case '}':
				skip_depth--;
				break;

			    case '[': // [2do]
				pattern = AnalyseBrackets(pattern,0,0,0);
				break;
			}
		    }
		}
		else if ( *text++ != ch )
		    return false;
		break;

	   case '}':
		if ( !alt_depth && *text++ != ch )
		    return false;
		break;

	   case '$':
		if ( !*pattern && !*text )
		    return true;
		if ( *text++ != ch )
		    return false;
		break;

	   case '\\':
		ch = *pattern++;
		// fall through

	   default:
		if ( *text++ != ch )
		    return false;
		break;
	}
    }
    return skip_end || *text == 0;
}

//-----------------------------------------------------------------------------

bool MatchPattern ( ccp pattern, ccp text )
{
    TRACE("MatchPattern(|%s|%s|)\n",pattern,text);
    if ( !pattern || !*pattern )
	return true;

    if (!text)
	text = "";

    const size_t plen = strlen(pattern);
    ccp last = pattern + plen - 1;
    char last_ch = *last;
    int count = 0;
    while ( last > pattern && *--last == '\\' )
	count++;
    if ( count & 1 )
	last_ch = 0; // no special char!
	
    if ( *pattern == '/' )
    {
	pattern++;
	return MatchPatternHelper(pattern,text++,last_ch!='$',0);
    }

    while (*text)
	if (MatchPatternHelper(pattern,text++,0,0))
	    return true;

    return false;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////
