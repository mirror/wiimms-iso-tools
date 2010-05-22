
#define _GNU_SOURCE 1

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "debug.h"
#include "lib-std.h"
#include "wbfs-interface.h"
#include "titles.h"
#include "dclib-utf8.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    variables                    ///////////////
///////////////////////////////////////////////////////////////////////////////

int title_mode = 1;

StringList_t * first_title_fname = 0;
StringList_t ** append_title_fname = &first_title_fname;

ID_DB_t title_db = {0,0,0};	// title database

static bool load_default_titles = true;
const int tdb_grow_size = 1000;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 titles interface                ///////////////
///////////////////////////////////////////////////////////////////////////////

int AddTitleFile ( ccp arg, int unused )
{
    if ( arg && *arg )
    {
	if ( !arg[1] && arg[0] >= '0' && arg[0] <= '9' )
	{
	    TRACE("#T# set title mode: %d -> %d\n",title_mode,arg[0]-'0');
	    title_mode =  arg[0] - '0';
	}
	else if ( !arg[1] && arg[0] == '/' )
	{
	    TRACE("#T# disable default titles\n");
	    load_default_titles = false;
	    while (first_title_fname)
	    {
		StringList_t * sl = first_title_fname;
		first_title_fname = sl->next;
		free((char*)sl->str);
		free(sl);
	    }
	}
	else
	{
	    TRACE("#T# append title file: %s\n",arg);
	    StringList_t * sl = calloc(1,sizeof(StringList_t));
	    if (!sl)
		OUT_OF_MEMORY;
	    sl->str = strdup(arg);
	    if (!sl->str)
		OUT_OF_MEMORY;

	    *append_title_fname = sl;
	    append_title_fname = &sl->next;
	    ASSERT(!sl->next);
	}
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static int LoadTitleFile ( ccp fname, bool warn )
{
    ASSERT( fname && *fname );
    TRACE("#T# LoadTitleFile(%s)\n",fname);

    const int max_title_size = 200; // not a exact value
    char buf[PATH_MAX+max_title_size], title_buf[max_title_size+10];
    buf[0] = 0;

    FILE * f = 0;
    const bool use_stdin = fname[0] == '-' && !fname[1];
    if (use_stdin)
    {
	f = stdin;
	TRACE("#T#  - use stdin, f=%p\n",f);
    }
    else if (strchr(fname,'/'))
    {
     #ifdef __CYGWIN__
	NormalizeFilenameCygwin(buf,sizeof(buf),fname);
	f = fopen(buf,"r");
	TRACE("#T#  - f=%p: %s\n",f,buf);
	buf[0] = 0;
     #else
	f = fopen(fname,"r");
	TRACE("#T#  - f=%p: %s\n",f,fname);
     #endif
    }
    else
    {
	// no path found ==> use search_path[]
	ccp * sp;
	for ( sp = search_path; *sp && !f; sp++ )
	{
	    snprintf(buf,sizeof(buf),"%s%s",*sp,fname);
	    f = fopen(buf,"r");
	    TRACE("#T#  - f=%p: %s\n",f,buf);
	}
    }

    if (!f)
    {
	if ( warn || verbose > 3 )
	    ERROR0(ERR_WARNING,"Title file not found: %s\n",fname);
	return ERR_READ_FAILED;
    }

    if ( verbose > 3 )
	printf("SCAN TITLE FILE %s\n", *buf ? buf : fname );

    while (fgets(buf,sizeof(buf),f))
    {
	ccp ptr = buf;
	noTRACE("LINE: %s\n",ptr);

	// skip blanks
	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr++;

	const int idtype = CountIDChars(ptr,false);
	if ( idtype != 4 && idtype != 6 )
	    continue;

	char * id = (char*)ptr;
	ptr += idtype;

	// skip blanks and find '='
	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr++;
	if ( *ptr != '=' )
	    continue;
	ptr++;

	// title found, skip blanks
	id[idtype] = 0;
	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr++;

	char *dest = title_buf;
	char *dend = dest + sizeof(title_buf) - 6; // enough for SPACE + UTF8 + NULL

	bool have_blank = false;
	while ( dest < dend && *ptr )
	{
	    ulong ch = ScanUTF8AnsiChar(&ptr);
	    if ( ch <= ' ' )
		have_blank = true;
	    else
	    {
		// real char found
		if (have_blank)
		{
		    have_blank = false;
		    *dest++ = ' ';
		}

		if ( ch >= 0x100 )
		{
		    const dcUnicodeTripel * trip = DecomposeUnicode(ch);
		    if (trip)
			ch = trip->code2;
		}

		if (use_utf8)
		    dest = PrintUTF8Char(dest,ch);
		else
		    *dest++ = ch < 0xff ? ch : '?';
	    }
	}
	*dest = 0;
	if (*title_buf)
	    InsertID(&title_db,id,title_buf);
    }

    fclose(f);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

void InitializeTDB()
{
    static bool tdb_initialized = false;
    if (!tdb_initialized)
    {
	tdb_initialized = true;

	title_db.list	= 0;
	title_db.used	= 0;
	title_db.size	= 0;

	if (load_default_titles)
	{
	    LoadTitleFile("titles.txt",false);

	    if (lang_info)
	    {
		char lang[100];
		snprintf(lang,sizeof(lang),"titles-%s.txt",lang_info);
		LoadTitleFile(lang,false);
	    }

	    LoadTitleFile("titles.local.txt",false);
	}

	while (first_title_fname)
	{
	    StringList_t * sl = first_title_fname;
	    LoadTitleFile(sl->str,true);
	    first_title_fname = sl->next;
	    free((char*)sl->str);
	    free(sl);
	}

     #ifdef xxDEBUG
	TRACE("Title DB with %d titles:\n",title_db.used);
	DumpIDDB(&title_db,TRACE_FILE);
     #endif
    }
}

///////////////////////////////////////////////////////////////////////////////

ccp GetTitle ( ccp id6, ccp default_if_failed )
{
    if ( !title_mode || !id6 || !*id6 )
	return default_if_failed;

    InitializeTDB();
    TDBfind_t stat;
    int idx = FindID(&title_db,id6,&stat,0);
    TRACE("#T# GetTitle(%s) tm=%d  idx=%d/%d/%d  stat=%d -> %s %s\n",
		id6, title_mode, idx, title_db.used, title_db.size, stat,
		idx < title_db.used ? title_db.list[idx]->id : "",
		idx < title_db.used ? title_db.list[idx]->title : "" );
    ASSERT( stat == IDB_NOT_FOUND || idx < title_db.used );
    return stat == IDB_NOT_FOUND
		? default_if_failed
		: title_db.list[idx]->title;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 exclude interface               ///////////////
///////////////////////////////////////////////////////////////////////////////

bool include_db_enabled = false;
ID_DB_t include_db = {0,0,0};	// include database
ID_DB_t exclude_db = {0,0,0};	// exclude database

StringField_t include_fname = {0,0,0};
StringField_t exclude_fname = {0,0,0};

int disable_exclude_db = 0;	// disable exclude db at all if > 0

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int AddIncludeID ( ccp arg, int unused )
{
    char id[7];
    int idlen;
    ScanID(id,&idlen,arg);
    if ( idlen ==4 || idlen == 6 )
	InsertID(&include_db,id,0);

    include_db_enabled = true;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int AddIncludePath ( ccp arg, int unused )
{
    char buf[PATH_MAX];
    if (realpath(arg,buf))
	arg = buf;

    InsertStringField(&include_fname,arg,false);

    include_db_enabled = true;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int AddExcludeID ( ccp arg, int unused )
{
    char id[7];
    int idlen;
    ScanID(id,&idlen,arg);
    if ( idlen ==4 || idlen == 6 )
	InsertID(&exclude_db,id,0);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int AddExcludePath ( ccp arg, int unused )
{
    char buf[PATH_MAX];
    if (realpath(arg,buf))
	arg = buf;

    InsertStringField(&exclude_fname,arg,false);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void CheckExcludePath
	( ccp path, StringField_t * sf, ID_DB_t * db, int max_dir_depth )
{
    TRACE("CheckExcludePath(%s,%p,%d)\n",path,sf,max_dir_depth);
    ASSERT(sf);
    ASSERT(db);

    File_t f;
    InitializeFile(&f);
    if (OpenFile(&f,path,IOM_NO_STREAM))
	return;

    AnalyzeFT(&f);
    ClearFile(&f,false);

    if ( f.id6[0] )
    {
	TRACE(" - exclude id %s\n",f.id6);
	InsertID(db,f.id6,0);
    }
    else if ( max_dir_depth > 0 && f.ftype == FT_ID_DIR )
    {
	char real_path[PATH_MAX];
	if (realpath(path,real_path))
	    path = real_path;
	if (InsertStringField(sf,path,false))
	{
	    TRACE(" - exclude dir %s\n",path);
	    DIR * dir = opendir(path);
	    if (dir)
	    {
		char buf[PATH_MAX], *bufend = buf+sizeof(buf);
		char * dest = StringCopyE(buf,bufend-1,path);
		if ( dest > buf && dest[-1] != '/' )
		    *dest++ = '/';

		max_dir_depth--;

		for(;;)
		{
		    struct dirent * dent = readdir(dir);
		    if (!dent)
			break;
		    ccp n = dent->d_name;
		    if ( n[0] != '.' )
		    {
			StringCopyE(dest,bufend,dent->d_name);
			CheckExcludePath(buf,sf,db,max_dir_depth);
		    }
		}
		closedir(dir);
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void SetupExcludeDB()
{
    TRACE("SetupExcludeDB()");

    if (include_fname.used)
    {
	TRACELINE;
	StringField_t sf;
	InitializeStringField(&sf);
	ccp * ptr = include_fname.field + include_fname.used;
	while ( ptr-- > include_fname.field )
	    CheckExcludePath(*ptr,&sf,&include_db,15);
	ResetStringField(&sf);
	ResetStringField(&include_fname);
    }

    if (exclude_fname.used)
    {
	TRACELINE;
	StringField_t sf;
	InitializeStringField(&sf);
	ccp * ptr = exclude_fname.field + exclude_fname.used;
	while ( ptr-- > exclude_fname.field )
	    CheckExcludePath(*ptr,&sf,&exclude_db,15);
	ResetStringField(&sf);
	ResetStringField(&exclude_fname);
    }
}

///////////////////////////////////////////////////////////////////////////////

void DefineExcludePath ( ccp path, int max_dir_depth )
{
    TRACE("DefineExcludePath(%s,%d)\n",path,max_dir_depth);

    if (exclude_fname.used)
	SetupExcludeDB();

    StringField_t sf;
    InitializeStringField(&sf);
    CheckExcludePath(path,&sf,&exclude_db,max_dir_depth);
    ResetStringField(&sf);
}

///////////////////////////////////////////////////////////////////////////////

bool IsExcluded ( ccp id6 )
{
    noTRACE("IsExcluded(%s) dis=%d ena=%d, n=%d+%d\n",
		id6, disable_exclude_db, include_db_enabled,
		exclude_fname.used, include_fname.used );

    if ( disable_exclude_db > 0 )
	return false;

    if ( exclude_fname.used || include_fname.used )
	SetupExcludeDB();

    TDBfind_t stat;
    FindID(&exclude_db,id6,&stat,0);

    if ( stat == IDB_ID_FOUND || stat == IDB_ABBREV_FOUND )
	return true;

    if (!include_db_enabled)
	return false;

    FindID(&include_db,id6,&stat,0);
    return stat != IDB_ID_FOUND && stat != IDB_ABBREV_FOUND;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 low level interface             ///////////////
///////////////////////////////////////////////////////////////////////////////

// disable extended tracing

#undef xTRACE

#if 0
    #define xTRACE TRACE
#else
    #define xTRACE noTRACE
#endif

///////////////////////////////////////////////////////////////////////////////

int FindID ( ID_DB_t * db, ccp id, TDBfind_t * p_stat, int * p_num )
{
    ASSERT(db);
    if ( !db || !db->used )
    {
	if (p_stat)
	    *p_stat = IDB_NOT_FOUND;
	if  (p_num)
	    *p_num = 0;
	return 0;
    }

    ID_t * elem = 0;
    size_t id_len  = strlen(id);
    if ( id_len > sizeof(elem->id)-1 )
	id_len = sizeof(elem->id)-1;

    int beg = 0, end = db->used-1;
    while ( beg <= end )
    {
	int idx = (beg+end)/2;
	elem = db->list[idx];
	const int cmp_stat = memcmp(id,elem->id,id_len);
	noTRACE(" - check: %d..%d..%d: %d = %s\n",beg,idx,end,cmp_stat,elem->id);
	if ( cmp_stat < 0 )
	    end = idx-1;
	else if ( cmp_stat > 0 )
	    beg = idx + 1;
	else
	{
	    beg = idx;
	    break;
	}
    }
    ASSERT( beg >= 0 && beg <= db->used );
    elem = db->list[beg];

    TDBfind_t stat = IDB_NOT_FOUND;
    if ( beg < db->used )
    {
	if (!memcmp(id,elem->id,id_len))
	    stat = elem->id[id_len] ? IDB_EXTENSION_FOUND : IDB_ID_FOUND;
	else if (!memcmp(id,elem->id,strlen(elem->id)))
	    stat = IDB_ABBREV_FOUND;
	else if ( beg > 0 )
	{
	    elem = db->list[beg-1];
	    xTRACE("cmp-1[%s,%s] -> %d\n",id,elem->id,memcmp(id,elem->id,strlen(elem->id)));
	    if (!memcmp(id,elem->id,strlen(elem->id)))
	    {
		stat = IDB_ABBREV_FOUND;
		beg--;
	    }
	}
	xTRACE("cmp[%s,%s] %d %d -> %d\n", id, elem->id,
		memcmp(id,elem->id,id_len), memcmp(id,elem->id,strlen(elem->id)), stat );
    }
    xTRACE("#T# FindID(%p,%s,%p,%p) idx=%d/%d/%d, stat=%d, found=%s\n",
		db, id, p_stat, p_num, beg, db->used, db->size,
		stat, beg < db->used && elem ? elem->id : "-" );

    if (p_stat)
	*p_stat = stat;

    if (p_num)
    {
	int idx = beg;
	while ( idx < db->used && !memcmp(id,db->list[idx],id_len) )
	    idx++;
	xTRACE(" - num = %d\n",idx-beg);
	*p_num = idx - beg;;
    }

    return beg;
}

///////////////////////////////////////////////////////////////////////////////

int InsertID ( ID_DB_t * db, ccp id, ccp title )
{
    ASSERT(db);
    xTRACE("-----\n");

    // remove all previous definitions first
    int idx = RemoveID(db,id,true);

    if ( db->used == db->size )
    {
	db->size += tdb_grow_size;
	db->list = (ID_t**)realloc(db->list,db->size*sizeof(*db->list));
	if (!db->list)
	    OUT_OF_MEMORY;
    }
    ASSERT( db->list );
    ASSERT( idx >= 0 && idx <= db->used );

    ID_t ** elem = db->list + idx;
    xTRACE(" - insert: %s|%s|%s\n",
		idx>0 ? elem[-1]->id : "-", id, idx<db->used ? elem[0]->id : "-");
    memmove( elem+1, elem, (db->used-idx)*sizeof(*elem) );
    db->used++;
    ASSERT( db->used <= db->size );

    int tlen = title ? strlen(title) : 0;
    ID_t * t = *elem = (ID_t*)malloc(sizeof(ID_t)+tlen);
    if (!t)
	OUT_OF_MEMORY;

    StringCopyS(t->id,sizeof(t->id),id);
    StringCopyS(t->title,tlen+1,title);

    xTRACE("#T# InsertID(%p,%s,) [%d], id=%s title=%s\n",
		db, id, idx, t->id, t->title );

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int RemoveID ( ID_DB_t * db, ccp id, bool remove_extended )
{
    ASSERT(db);

    TDBfind_t stat;
    int count;
    int idx = FindID(db,id,&stat,&count);

    switch(stat)
    {
	case IDB_NOT_FOUND:
	    break;

	case IDB_ABBREV_FOUND:
	    idx++;
	    break;

	case IDB_ID_FOUND:
	case IDB_EXTENSION_FOUND:
	    xTRACE("#T# RemoveID(%p,%s,%d) idx=%d, count=%d\n",
			db, id, remove_extended, idx, count );
	    ASSERT(count>0);
	    ASSERT(db->used>=count);
	    ID_t ** elem = db->list + idx;
	    int c;
	    for ( c = count; c > 0; c--, elem++ )
	    {
		xTRACE(" - remove %s = %s\n",(*elem)->id,(*elem)->title);
		free(*elem);
	    }

	    db->used -= count;
	    elem = db->list + idx;
	    memmove( elem, elem+count, (db->used-idx)*sizeof(*elem) );
	    break;

	default:
	    ASSERT(0);
    }

    return idx;
}

///////////////////////////////////////////////////////////////////////////////

void DumpIDDB ( ID_DB_t * db, FILE * f )
{
    if ( !db || !db->list || !f )
	return;

    ID_t ** list = db->list, **end;
    for ( end = list + db->used; list < end; list++ )
    {
	ID_t * elem = *list;
	if (*elem->title)
	    fprintf(f,"%-6s = %s\n",elem->id,elem->title);
	else
	    fprintf(f,"%-6s\n",elem->id);
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////

