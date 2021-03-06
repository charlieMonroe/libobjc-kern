#if defined(__WIN32__) || defined(__APPLE__)
	#define CDECL(symbol) _##symbol
#else
	#define CDECL(symbol) symbol
#endif

#ifndef HAS_TYPE_DIRECTIVE
  #define HAS_TYPE_DIRECTIVE 0
#endif

#if HAS_TYPE_DIRECTIVE
	#define TYPE_DIRECTIVE(symbol, symboltype) .type symbol, symboltype
#else
	#define TYPE_DIRECTIVE(symbol, symboltype)
#endif

#if __x86_64
	#include "objc_msgSend.x86-64.S"
#elif __i386
	#include "objc_msgSend.x86-32.S"
#else
	#warning objc_msgSend() not implemented for your architecture
#endif
