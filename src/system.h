
#ifndef WIT_SYSTEM_H
#define WIT_SYSTEM_H 1

///////////////////////////////////////////////////////////////////////////////

typedef enum enumSystemID
{
	SYSID_UNKNOWN		= 0x00000000,
	SYSID_I386		= 0x01000000,
	SYSID_X86_64		= 0x02000000,
	SYSID_CYGWIN		= 0x03000000,
	SYSID_APPLE		= 0x04000000,
	SYSID_LINUX		= 0x05000000,
	SYSID_UNIX		= 0x06000000,

} enumSystemID;

///////////////////////////////////////////////////////////////////////////////

#ifdef __CYGWIN__
	#define SYSTEM "cygwin"
	#define SYSTEMID SYSID_CYGWIN
#elif __APPLE__
	#define SYSTEM "mac"
	#define SYSTEMID SYSID_APPLE
#elif __linux__
  #ifdef __i386__
	#define SYSTEM "i386"
	#define SYSTEMID SYSID_I386
  #elif __x86_64__
	#define SYSTEM "x86_64"
	#define SYSTEMID SYSID_X86_64
  #else
	#define SYSTEM "linux"
	#define SYSTEMID SYSID_LINUX
  #endif
#elif __unix__
	#define SYSTEM "unix"
	#define SYSTEMID SYSID_UNIX
#else
	#define SYSTEM "unknown"
	#define SYSTEMID SYSID_UNKNOWN
#endif

///////////////////////////////////////////////////////////////////////////////

#endif // WIT_SYSTEM_H 1

