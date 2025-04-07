//!
//! FILE          code\base\macros.h
//!
//! AUTHORS
//!               Ilya Akkuzin <gr3yknigh1@gmail.com>
//!
//! NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//!
#pragma once

#if !defined(MAKE_FLAG)
    #define MAKE_FLAG(INDEX) (1 << (INDEX))
#endif

#if !defined(HAS_FLAG)
    #define HAS_FLAG(MASK, FLAG) (((MASK) & (FLAG)) == (FLAG))
#endif

#if !defined(LITERAL)
    #if defined(__cplusplus)
        #define LITERAL(X) X
    #else
        #define LITERAL(X) (X)
    #endif
#endif

#if !defined(STATIC_ARRAY_COUNT)
    #define STATIC_ARRAY_COUNT(ARRAY_PTR) (sizeof((ARRAY_PTR)) / sizeof(*(ARRAY_PTR)))
#endif

#if !defined(STRINGIFY_IMPL)
    #define STRINGIFY_IMPL(X) #X
#endif

#if !defined(STRINGIFY)
    #define STRINGIFY(X) STRINGIFY_IMPL(X)
#endif

#if !defined(FOR_LINKED_LIST)
    #define FOR_LINKED_LIST(ITER, LINKED_LIST)                                         \
        for (auto ITER = (LINKED_LIST)->head; ITER != nullptr; ITER = ITER->next)
#endif

