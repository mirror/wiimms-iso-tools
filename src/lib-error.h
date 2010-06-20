
#ifndef WIT_ERROR_H
#define WIT_ERROR_H 1

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  error messages                 ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumError
{
	ERR_OK,
	ERR_DIFFER,
	ERR_NOTHING_TO_DO,
	ERR_NO_SOURCE_FOUND,
	ERR_JOB_IGNORED,
	ERR_WARNING,	// separator: below = real errors and not warnings

	ERR_INVALID_FILE,

	ERR_NO_WDF,
	ERR_WDF_VERSION,
	ERR_WDF_SPLIT,
	ERR_WDF_INVALID,

	ERR_NO_CISO,
	ERR_CISO_INVALID,

	ERR_WDISC_NOT_FOUND,
	ERR_NO_WBFS_FOUND,
	ERR_TO_MUCH_WBFS_FOUND,
	ERR_WBFS_INVALID,

	ERR_ALREADY_EXISTS,
	ERR_CANT_OPEN,
	ERR_CANT_CREATE,
	ERR_CANT_CREATE_DIR,
	ERR_WRONG_FILE_TYPE,
	ERR_READ_FAILED,
	ERR_REMOVE_FAILED,
	ERR_WRITE_FAILED,

	ERR_WBFS,

	ERR_MISSING_PARAM,
	ERR_SEMANTIC,
	ERR_SYNTAX,

	ERR_INTERRUPT,

	ERR_NOT_IMPLEMENTED, // separator: this+below = hard errors => exit

	ERR_INTERNAL,
	ERR_OUT_OF_MEMORY,
	ERR_FATAL,

	ERR__N

} enumError;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  error interface                ///////////////
///////////////////////////////////////////////////////////////////////////////

extern enumError last_error;
extern enumError max_error;
extern u32 error_count;

///////////////////////////////////////////////////////////////////////////////

const char * GetErrorName ( int stat );
const char * GetErrorText ( int stat );
int PrintError ( const char * func, const char * file, unsigned int line,
		int syserr, enumError err_code, const char * format, ... );

#define OUT_OF_MEMORY PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_OUT_OF_MEMORY,0)

#define ERROR(se,code,...) PrintError(__FUNCTION__,__FILE__,__LINE__,se,code,__VA_ARGS__)
#define ERROR0(code,...) PrintError(__FUNCTION__,__FILE__,__LINE__,0,code,__VA_ARGS__)
#define ERROR1(code,...) PrintError(__FUNCTION__,__FILE__,__LINE__,errno,code,__VA_ARGS__)

///////////////////////////////////////////////////////////////////////////////

void HexDump ( FILE * f, int indent, u64 addr, int addr_fw, int row_len,
		const void * data, size_t count );
void HexDump16 ( FILE * f, int indent, u64 addr,
		const void * data, size_t count );

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_ERROR_H 1
