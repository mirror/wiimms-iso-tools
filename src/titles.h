
#ifndef WWT_TITELS_H
#define WWT_TITELS_H 1

#include "types.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                      docu                       ///////////////
///////////////////////////////////////////////////////////////////////////////
#if 0

//---------------------------------------------------------------------------
//
//  The title data base is a list of pointers. Each pointer points to a
//  dynamic allocated memory location structured by this:
//
//      6 bytes : For the ID. The ID have 1 to 6 characters padded with NULL.
//      1 byte  : A NULL character (needed by strcmp())
//    >=1 byte  : The title, NULL terminated.
//
//  The list is orderd by the ID which is unique in the data base.
//
//  While inserting an ID an existing entry will be removed. Additional all
//  entries that begins with the new ID are removed.
//
//  While looking up for an ID, the ID itself is searched. If not found an
//  abbreviaton for that ID is searched. Binary searching is used.
//
//---------------------------------------------------------------------------

#endif
//
///////////////////////////////////////////////////////////////////////////////
///////////////                   declarations                  ///////////////
///////////////////////////////////////////////////////////////////////////////

extern int title_mode;

extern struct StringList_t * first_title_fname;
extern struct StringList_t ** append_title_fname;


///////////////////////////////////////////////////////////////////////////////

typedef struct ID_t
{
	char id[7];		// the ID padded with at least 1 NULL
	char title[2];		// the title

} ID_t;

///////////////////////////////////////////////////////////////////////////////

typedef struct ID_DB_t
{
	ID_t ** list;		// pointer to the title field
	int used;		// number of used titles in the title field
	int size;		// number of allocated pointer in 'title'

} ID_DB_t;

///////////////////////////////////////////////////////////////////////////////
///////////////                 titles interface                ///////////////
///////////////////////////////////////////////////////////////////////////////

extern ID_DB_t title_db;	// title database

// file support
void InitializeTDB();
int AddTitleFile ( ccp arg, int unused );

// title lookup
ccp GetTitle ( ccp id6, ccp default_if_failed );


///////////////////////////////////////////////////////////////////////////////
///////////////                 exclude interface               ///////////////
///////////////////////////////////////////////////////////////////////////////

extern bool include_db_enabled;
extern ID_DB_t include_db;	// include database
extern ID_DB_t exclude_db;	// exclude database

extern StringField_t include_fname;
extern StringField_t exclude_fname;

extern int disable_exclude_db;	// disable exclude db at all if > 0

int  AddIncludeID ( ccp arg, int unused );
int  AddIncludePath ( ccp arg, int unused );

int  AddExcludeID ( ccp arg, int unused );
int  AddExcludePath ( ccp arg, int unused );

void SetupExcludeDB();
void DefineExcludePath ( ccp path, int max_dir_depth );

bool IsExcluded ( ccp id6 );


///////////////////////////////////////////////////////////////////////////////
///////////////                 low level interface             ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum {			// all values are well orders

	IDB_NOT_FOUND,		// id not found
	IDB_ABBREV_FOUND,	// id not found but an abbreviation
	IDB_ID_FOUND,		// if found
	IDB_EXTENSION_FOUND,	// id not found but an extended version

} TDBfind_t;

int FindID   ( ID_DB_t * db, ccp id, TDBfind_t * stat, int * num_of_matching );
int InsertID ( ID_DB_t * db, ccp id, ccp title );
int RemoveID ( ID_DB_t * db, ccp id, bool remove_extended );

void DumpIDDB ( ID_DB_t * db, FILE * f );

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WWT_TITELS_H

