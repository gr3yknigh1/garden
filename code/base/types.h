//!
//! FILE          code\base\types.h
//!
//! AUTHORS
//!               Ilya Akkuzin <gr3yknigh1@gmail.com>
//!
//! NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//!
#pragma once

#include "base/static_assert.h"

constexpr int pointer_size = 8;
EXPECT_TYPE_SIZE(void *, pointer_size);

typedef signed char I8;
typedef signed short I16;
typedef signed int I32;

#if defined(_WIN32)
    typedef signed long long I64;
#else
    typedef signed long I64;
#endif

EXPECT_TYPE_SIZE(I8, 1);
EXPECT_TYPE_SIZE(I16, 2);
EXPECT_TYPE_SIZE(I32, 4);
EXPECT_TYPE_SIZE(I64, 8);

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

#if defined(_WIN32)
    typedef unsigned long long U64;
#else
    typedef unsigned long U64;
#endif

EXPECT_TYPE_SIZE(U8, 1);
EXPECT_TYPE_SIZE(U16, 2);
EXPECT_TYPE_SIZE(U32, 4);
EXPECT_TYPE_SIZE(U64, 8);

typedef float  F32;
typedef double F64;

EXPECT_TYPE_SIZE(F32, 4);
EXPECT_TYPE_SIZE(F64, 8);

typedef U8  Byte;
typedef U64 Size;

EXPECT_TYPE_SIZE(Byte, 1);
EXPECT_TYPE_SIZE(Size, pointer_size);

typedef char     Char8;
EXPECT_TYPE_SIZE(Char8, 1);

typedef wchar_t  Char16;
EXPECT_TYPE_SIZE(Char16, 2);

// NOTE(gr3yknigh1): Explicitly distiguasing C style
// string (null terminated).  [2024/05/26]
typedef const Char8  *ZStr8; 
typedef const Char16 *ZStr16; 
                            
EXPECT_TYPE_SIZE(ZStr8, pointer_size);
EXPECT_TYPE_SIZE(ZStr16, pointer_size);

#if !defined(__cplusplus)
    // TODO(gr3yknigh1): Typedef to _Bool if C11
    #if !defined(bool)
        typedef I8 bool;
    #endif

    #if !defined(true)
        #define true 1
    #endif // true

    #if !defined(false)
        #define false 0
    #endif // false

    #if !defined(NULL)
        #define NULL ((void *)0)
    #endif // NULL
#endif

EXPECT_TYPE_SIZE(bool, 1);

