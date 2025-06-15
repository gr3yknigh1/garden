
#if !defined(NOC_MACROS_H_INCLUDED)
#define NOC_MACROS_H_INCLUDED

#include <noc/detect.h>

#if NOC_DETECT_LANGUAGE_C

    #if !defined(NULL)
	#define NULL ((void *)0)
    #endif


    // XXX
    #if !defined(static_assert)
	#define static_assert(...)
    #endif

#endif

#if !defined(NOC_DEFINE)

    #if defined(NOC_DO_EXPORT)

	#if NOC_DETECT_PLATFORM_WINDOWS
	    #define NOC_DEFINE2 __declspec(dllexport)
	#else
	    #define NOC_DEFINE2
	#endif

    #else

	#if NOC_DETECT_PLATFORM_WINDOWS
	    #define NOC_DEFINE2 __declspec(dllimport)
	#else
	    #define NOC_DEFINE2
	#endif

    #endif


    #if NOC_DETECT_LANGUAGE_CXX
	#define NOC_DEFINE NOC_CLINKAGE
	//NOC_DEFINE2
    #else
	#define NOC_DEFINE NOC_EXTERN
	//NOC_DEFINE2
    #endif

#endif

#ifndef NOC_INLINE
    #define NOC_INLINE inline
#endif // NOC_INLINE


// NOTE(gr3yknigh1): This is stupid [2025/05/30]
#define NOC_NOP ;

#if ((NOC_DETECT_LANGUAGE_CXX && NOC_DETECT_CXX_VERSION >= NOC_DETECT_CXX_VERSION_2017) || (NOC_DETECT_LANGUAGE_C && NOC_DETECT_C_VERSION >= NOC_DETECT_C_VERSION_2023)) && !defined(NOC_DETECT_COMPILER_MSVC)



    #define NOC_NODISCARD [[nodiscard]]

#else

    #define NOC_NODISCARD

    // TODO(gr3yknigh1): Push a warning? [2025/05/29]

#endif

#if defined(NOC_DETECT_LANGUAGE_CXX) && (NOC_DETECT_CXX_VERSION >= NOC_DETECT_CXX_VERSION_2011)
    #define NOC_NORETURN [[noreturn]]
#elif defined(NOC_DETECT_LANGUAGE_C) &&  (NOC_DETECT_C_VERSION >= NOC_DETECT_C_VERSION_2011) 
    #define NOC_NORETURN _Noreturn
#else 
    #define NOC_NORETURN
    // TODO(gr3yknigh1): Push a warning? [2025/05/29]
#endif


#if defined(NOC_DETECT_LANGUAGE_CXX)
    #define NOC_LITERAL(T) T
#else
    #define NOC_LITERAL(T) (T)
#endif

#define NOC_STATIC_ARRAY_COUNT(ARR_PTR) (sizeof((ARR_PTR)) / sizeof(*(ARR_PTR)))


#if !defined(NOC_MIN)
    #define NOC_MIN(A, B) (((A) < (B)) ? (A) : (B))
#endif

#if !defined(NOC_MAX)
    #define NOC_MAX(A, B) (((A) > (B)) ? (A) : (B))
#endif

#if !defined(NOC_IN_RANGE)
    #define NOC_IN_RANGE(MIN, VALUE, MAX) ((MIN) >= (VALUE) && (VALUE) <= (MAX))
#endif

#if !defined(NOC_IN_RANGE_S)
    #define NOC_IN_RANGE_S(MIN, VALUE, MAX) ((MIN) >= (VALUE) && (VALUE) < (MAX))
#endif

#if !defined(NOC_UNUSED)
    #define NOC_UNUSED(X) ((void)(X))
#endif

#if !defined(NOC_EXTERN)
    #define NOC_EXTERN extern
#endif

#if !defined(NOC_STRINGIFY2)
    #define NOC_STRINGIFY2(X) #X
#endif

#if !defined(NOC_STRINGIFY)
    #define NOC_STRINGIFY(X) NOC_STRINGIFY2(X)
#endif

#if !defined(NOC_CLINKAGE)
    #define NOC_CLINKAGE extern "C"
#endif

#if !defined(NOC_CLINKAGE_BEGIN)
    #define NOC_CLINKAGE_BEGIN extern "C" {
#endif

#if !defined(NOC_CLINKAGE_END)
    #define NOC_CLINKAGE_END }
#endif

#endif // NOC_MACROS_H_INCLUDED
