#if !defined(NOC_DETECT_H_INCLUDED)
///
/// Defines:
///
///   - NOC_DETECT_LANGUAGE_%LANG%: C, CXX
///   - NOC_DETECT_PLATFORM_%PLATFORM%: WINDOWS, WINDOWS_X86, WINDOWS_X86_64, LINUX
///   - NOC_DETECT_COMPILER_%COMPILER%: MSVC, GCC, CLANG
///   - NOC_DETECT_ARCH_%ARCH%: X86_64, X86
///   - NOC_DETECT_CXX_VERSION: current c++ standard version. TBD.
///   - NOC_DETECT_CXX_VERSION_%VERSION%: c++ version values. TBD.
///   - NOC_DETECT_C_VERSION: current c standard version. TBD.
///   - NOC_DETECT_C_VERSION_%VERSION%: c standard version. TBD.
///
#define NOC_DETECT_H_INCLUDED

#if defined(__cplusplus)
    #define NOC_DETECT_LANGUAGE_CXX 1

#else
    #define NOC_DETECT_LANGUAGE_C 1

#endif

#if defined(_WIN32) || defined(_WIN64)

    #define NOC_DETECT_PLATFORM_WINDOWS 1

    #if defined(_WIN32)
        #define NOC_DETECT_PLATFORM_WINDOWS_X86 1

    #endif

    #if defined(_WIN64)
        #define NOC_DETECT_PLATFORM_WINDOWS_X86_64 1

    #endif

#elif defined(__linux__)
    #define NOC_DETECT_PLATFORM_LINUX 1

#endif

#if defined(_MSC_VER)
    #define NOC_DETECT_COMPILER_MSVC 1

#elif defined(__GNUC__)
    #define NOC_DETECT_COMPILER_GCC 1
    
#elif defined(__clang__)
    #define NOC_DETECT_COMPILER_CLANG 1

#endif

// NOTE(gr3yknigh1): Thanks:
// https://stackoverflow.com/questions/152016/detecting-cpu-architecture-compile-time
// [2025/06/01]

#if defined(_M_X64) || defined(__x86_64__)
    #define NOC_DETECT_ARCH_X86_64 1
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
    #define NOC_DETECT_ARCH_X86 1
#endif

#if defined(NOC_DETECT_LANGUAGE_CXX)

    #if defined(NOC_DETECT_COMPILER_MSVC)
        #define NOC_DETECT_CXX_VERSION _MSVC_LANG

    #else
        #define NOC_DETECT_CXX_VERSION __cplusplus

    #endif

#else
    #define NOC_DETECT_CXX_VERSION 0

#endif

#if defined(NOC_DETECT_LANGUAGE_C)
    #define NOC_DETECT_C_VERSION __STDC_VERSION__
#else
    #define NOC_DETECT_C_VERSION 0
#endif

#define NOC_DETECT_C_VERSION_1989 199409L
#define NOC_DETECT_C_VERSION_1999 199901L
#define NOC_DETECT_C_VERSION_2011 201112L
#define NOC_DETECT_C_VERSION_2017 201710L
#define NOC_DETECT_C_VERSION_2023 202311L

#define NOC_DETECT_CXX_VERSION_1997 199711L
#define NOC_DETECT_CXX_VERSION_2011 201103L
#define NOC_DETECT_CXX_VERSION_2014 201402L
#define NOC_DETECT_CXX_VERSION_2017 201703L
#define NOC_DETECT_CXX_VERSION_2020 202002L
#define NOC_DETECT_CXX_VERSION_2023 202302L

#if !defined(NOC_DETECT_ARCH_X86_64)
    #error "Detected arch, which doesn't supported."
#endif

#if defined(NOC_DETECT_LANGUAGE_C) && (NOC_DETECT_C_VERSION < NOC_DETECT_C_VERSION_1999)
    #error "Detected standard which is older than C99. Or environment is messed up."
#endif

#if defined(NOC_DETECT_LANGUAGE_CXX) && (NOC_DETECT_CXX_VERSION < NOC_DETECT_CXX_VERSION_2011)
    #error "Detected standard which is older than C++11. Or environment is messed up."
#endif

#endif // NOC_DETECT_H_INCLUDED
