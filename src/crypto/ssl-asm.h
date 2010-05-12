
#ifdef __CYGWIN__
    #define SHA1_ASM
    #ifdef WWT_INCLUDE_SSL_ASM
	#include "sha1-586-cygwin.s"
    #endif
#elif defined(__linux__)
    #if defined(__i386__)
	#define SHA1_ASM
	#ifdef WWT_INCLUDE_SSL_ASM
	    #include "sha1-586-elf.s"
	#endif
    #elif defined(__x86_64__)
	#define SHA1_ASM
	#ifdef WWT_INCLUDE_SSL_ASM
	    #include "sha1-x86_64.s"
	#endif
    #endif
#endif

