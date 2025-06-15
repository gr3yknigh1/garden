#if !defined(NOSTDLIB_TYPES_H_INCLUDED)

#define NOSTDLIB_TYPES_H_INCLUDED

#include <noc/macros.h>
#include <noc/detect.h>


#if NOC_DETECT_LANGUAGE_C

    #if NOC_DETECT_C_VERSION >= NOC_DETECT_C_VERSION_2011
	typedef _Bool bool;

	#if !defined(true)
	    #define true 1

	#endif

	#if !defined(false)
	    #define false 0

	#endif

    #else
	typedef enum { false, true } bool;

    #endif

#endif

typedef signed char    Int8S;
typedef signed short   Int16S;
typedef signed int     Int32S;

typedef unsigned char  Int8U;
typedef unsigned short Int16U;
typedef unsigned int   Int32U;

#if defined(NOC_DETECT_PLATFORM_WINDOWS)
    typedef signed   long long Int64S;
    typedef unsigned long long Int64U;
#else
    typedef signed   long Int64S;
    typedef unsigned long Int64U;
#endif


typedef char        Char8;

#if NOC_DETECT_LANGUAGE_C
    typedef Int16S  Char16;

#else
    typedef wchar_t Char16;
#endif

typedef Int32S      Char32;


typedef const Char8  *Str8Z;
typedef const Char16 *Str16Z;
typedef const Char32 *Str32Z;

typedef float  Float32;
typedef double Float64;

typedef Int8U  Byte;
typedef Int64U SizeU;
typedef Int64S SizeS;

typedef SizeU PointerDiff;
typedef void *Pointer;

#endif // NOSTDLIB_TYPES_H_INCLUDED
